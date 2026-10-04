#ifndef AURA_DESKTOP_NAVIGATIONMODEL_H
#define AURA_DESKTOP_NAVIGATIONMODEL_H

// Professional navigation catalog for the AURA control center (V3-37).
//
// The sidebar groups the 19 canonical sections into purpose-built clusters
// (Monitoring, Intelligence, Governance, System) and gives each one a stable,
// human-readable title. The underlying section index (the order returned by
// DesktopModel::sections()) is preserved exactly, so this is presentation only:
// it does not add, remove, reorder or rename any canonical section, and it never
// changes which data a section is bound to.

#include <cstddef>
#include <string>
#include <vector>

namespace aura {
namespace desktop {

struct NavItem {
    std::string label;    // sidebar label (may be shortened for readability)
    std::string section;  // canonical section title (DesktopModel::sections())
    int index;            // canonical index in DesktopModel::sections()
};

struct NavGroup {
    std::string title;  // cluster header shown in the sidebar
    std::vector<NavItem> items;
};

// The canonical section order. Kept here as the single source of truth for the
// navigation indices so the sidebar cannot silently drift from the panels.
inline const std::vector<std::string>& canonical_sections() {
    static const std::vector<std::string> kSections = {
        "System Overview", "Market / Data Health", "Timeframes", "Signals",
        "Risk", "Shadow Positions", "Prediction / Observation", "Research",
        "Knowledge", "Candidates", "Validation", "Approval Center",
        "Evolution Graph", "Incidents", "Schedule / Operating Window",
        "Checkpoints / Recovery", "Health / Watchdog", "Audit",
        "Configuration / Version",
    };
    return kSections;
}

inline std::vector<NavGroup> navigation_groups() {
    return {
        {"MONITORING",
         {{"Dashboard", "System Overview", 0},
          {"Market", "Market / Data Health", 1},
          {"Timeframes", "Timeframes", 2},
          {"Signals", "Signals", 3},
          {"Risk", "Risk", 4},
          {"Shadow Positions", "Shadow Positions", 5},
          {"Observation", "Prediction / Observation", 6}}},
        {"INTELLIGENCE",
         {{"Research", "Research", 7},
          {"Knowledge", "Knowledge", 8},
          {"Candidates", "Candidates", 9},
          {"Validation", "Validation", 10}}},
        {"GOVERNANCE",
         {{"Approval Center", "Approval Center", 11},
          {"Evolution", "Evolution Graph", 12},
          {"Incidents", "Incidents", 13},
          {"Audit", "Audit", 17}}},
        {"SYSTEM",
         {{"Schedule", "Schedule / Operating Window", 14},
          {"Recovery", "Checkpoints / Recovery", 15},
          {"Health", "Health / Watchdog", 16},
          {"Configuration", "Configuration / Version", 18}}},
    };
}

// Total number of navigation entries. Must equal the canonical section count.
inline std::size_t navigation_count() {
    std::size_t n = 0;
    for (const NavGroup& g : navigation_groups()) n += g.items.size();
    return n;
}

// The canonical section title for a sidebar index, or the raw fallback if the
// index is out of range (never crashes the render loop).
inline const std::string& section_title_for(int index) {
    const std::vector<std::string>& secs = canonical_sections();
    static const std::string kUnknown = "Unknown";
    if (index < 0 || static_cast<std::size_t>(index) >= secs.size()) return kUnknown;
    return secs[static_cast<std::size_t>(index)];
}

}  // namespace desktop
}  // namespace aura

#endif  // AURA_DESKTOP_NAVIGATIONMODEL_H
