#ifndef AURA_OBSERVATION_PREDICTIONLEDGER_H
#define AURA_OBSERVATION_PREDICTIONLEDGER_H

#include "foundation/EntityId.h"
#include "foundation/HashAlgorithm.h"
#include "foundation/HashDigest.h"
#include "foundation/IHasher.h"
#include "foundation/IPersistenceStore.h"
#include "foundation/PersistenceRecordMetadata.h"
#include "foundation/PersistenceStatus.h"
#include "foundation/SchemaVersion.h"
#include "foundation/Timestamp.h"
#include "runtime/AdapterManager.h"
#include "runtime/BarFinalizer.h"
#include "runtime/SignalEngine.h"

#include <cstddef>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace aura {
namespace observation {

// An immutable, recorded prediction.
//
// OB-0001 / Phase 2 observation foundation. A prediction is a recorded forward
// claim made at decision time, derived from a deterministic signal (RT-0010) and
// the closed bar it is based on. `score` is a deterministic ranking value only
// and is explicitly NOT a probability (V3-31). `predicted_at` is the
// processing/research time, kept distinct from the bar time (V3-24).
struct Prediction {
    foundation::EntityId prediction_id{};
    foundation::HashDigest decision_id{};
    std::string symbol{};
    runtime::Timeframe timeframe{runtime::Timeframe::UNKNOWN};
    runtime::SignalDirection direction{runtime::SignalDirection::NONE};
    runtime::BarIdentity based_on{};
    double reference_price{0.0};
    double stop_price{0.0};
    double target_price{0.0};
    double score{0.0};  // deterministic ranking value, NOT a probability
    foundation::Timestamp predicted_at{};
    bool valid{false};
};

// Append-only prediction ledger (V3-22: persistent ledger is the source of truth).
//
// OB-0001 / Phase 2 observation foundation. Records are appended and never
// mutated, reordered or erased. Writes are idempotent on prediction identity:
// re-appending an identical prediction is a no-op success, while a conflicting
// payload for the same identity is rejected. Prediction identity is a
// deterministic function of stable fields, so the same prediction always yields
// the same identity. Persistence delegates to the append-oriented
// IPersistenceStore (PER-0003). Deterministic; no clock, no threads, no I/O
// beyond the injected store.
class PredictionLedger {
public:
    PredictionLedger() = default;

    // Records a prediction, deriving its deterministic identity. Idempotent on
    // identity; rejects invalid predictions.
    bool record(Prediction prediction) {
        if (!prediction.valid) return false;
        if (!prediction.prediction_id.valid()) {
            prediction.prediction_id = make_prediction_id(prediction);
        }
        if (!prediction.prediction_id.valid()) return false;
        return append(std::move(prediction));
    }

    // Appends a pre-built prediction. Idempotent on prediction_id: an identical
    // duplicate returns true without a second copy; a conflicting prediction for
    // the same identity returns false with no change.
    bool append(Prediction prediction) {
        if (!prediction.valid || !prediction.prediction_id.valid()) return false;
        const auto it = predictions_.find(prediction.prediction_id);
        if (it != predictions_.end()) {
            return same_content(it->second, prediction);
        }
        order_.push_back(prediction.prediction_id);
        predictions_.emplace(prediction.prediction_id, std::move(prediction));
        return true;
    }

    bool contains(const foundation::EntityId& prediction_id) const {
        return predictions_.find(prediction_id) != predictions_.end();
    }

    const Prediction* find(const foundation::EntityId& prediction_id) const {
        const auto it = predictions_.find(prediction_id);
        return it == predictions_.end() ? nullptr : &it->second;
    }

    std::size_t size() const noexcept { return predictions_.size(); }
    bool empty() const noexcept { return predictions_.empty(); }

    // Predictions in append order (deterministic, not sorted).
    std::vector<Prediction> ordered() const {
        std::vector<Prediction> out;
        out.reserve(order_.size());
        for (const foundation::EntityId& id : order_) out.push_back(predictions_.at(id));
        return out;
    }

    // Persists every prediction. Idempotent: re-persisting unchanged predictions
    // appends nothing new. Returns the worst status observed.
    foundation::PersistenceStatus persist(foundation::IPersistenceStore& store,
                                          const foundation::SchemaVersion& schema) const {
        foundation::PersistenceStatus worst = foundation::PersistenceStatus::OK;
        const std::unique_ptr<foundation::IHasher> hasher =
            foundation::create_hasher(foundation::HashAlgorithm::SHA256);
        if (hasher == nullptr) return foundation::PersistenceStatus::UNAVAILABLE;
        for (const foundation::EntityId& id : order_) {
            const Prediction& prediction = predictions_.at(id);
            const foundation::HashDigest digest = hasher->hash(payload_of(prediction));
            if (digest.empty()) return foundation::PersistenceStatus::FAILED;
            const foundation::PersistenceStatus status = store.append(
                foundation::PersistenceRecordMetadata(prediction.prediction_id, schema,
                                                      prediction.predicted_at),
                digest);
            if (!foundation::is_success(status) && status != foundation::PersistenceStatus::CONFLICT) {
                worst = status;
            }
        }
        return worst;
    }

    // Deterministic prediction identity: Hash over stable decision fields.
    static foundation::EntityId make_prediction_id(const Prediction& prediction) {
        const std::unique_ptr<foundation::IHasher> hasher =
            foundation::create_hasher(foundation::HashAlgorithm::SHA256);
        if (hasher == nullptr) return {};
        const foundation::HashDigest digest = hasher->hash(payload_of(prediction));
        if (digest.empty()) return {};
        return foundation::EntityId(std::string("pred|") + digest.to_hex());
    }

private:
    static std::string payload_of(const Prediction& prediction) {
        return prediction.decision_id.to_hex() + "|" + prediction.symbol + "|" +
               std::string(runtime::to_string(prediction.timeframe)) + "|" +
               std::string(runtime::to_string(prediction.direction)) + "|" +
               prediction.based_on.canonical_string() + "|" +
               std::to_string(prediction.reference_price) + "|" +
               std::to_string(prediction.stop_price) + "|" +
               std::to_string(prediction.target_price) + "|" + std::to_string(prediction.score);
    }

    static bool same_content(const Prediction& a, const Prediction& b) {
        return payload_of(a) == payload_of(b) && a.predicted_at == b.predicted_at;
    }

    std::map<foundation::EntityId, Prediction> predictions_{};
    std::vector<foundation::EntityId> order_{};
};

}  // namespace observation
}  // namespace aura

#endif  // AURA_OBSERVATION_PREDICTIONLEDGER_H
