//+------------------------------------------------------------------+
//|                                     ShadowOrderGateway.mqh       |
//|  AURA MT5 boundary - shadow-only execution gateway.              |
//|                                                                  |
//|  MT5-0006. The system is SHADOW-ONLY (V3-32). This gateway         |
//|  produces only shadow/paper execution intents and logs them; it    |
//|  MUST NOT place, modify or close any broker order. There is no     |
//|  OrderSend / OrderSendAsync / PositionModify path anywhere here.    |
//|  A live-execution request is explicitly BLOCKED/UNAVAILABLE.        |
//+------------------------------------------------------------------+
#property copyright "AURA"
#property version   "1.00"

#ifndef AURA_MT5_SHADOWORDERGATEWAY_MQH
#define AURA_MT5_SHADOWORDERGATEWAY_MQH

#include "Common.mqh"

//--- Why a live execution request was refused. The system is shadow-only.
enum ENUM_AURA_LIVE_BLOCK
  {
   AURA_LIVE_BLOCK_NONE        = 0,
   AURA_LIVE_BLOCK_SHADOW_ONLY = 1
  };

//--- Result of an attempted live execution. `executed` is always false here.
struct AuraLiveExecutionResult
  {
   bool                 executed;
   ENUM_AURA_LIVE_BLOCK reason;
  };

//--- A shadow (paper) execution intent. Not a broker order.
struct AuraShadowIntent
  {
   string symbol;
   int    tf_index;
   int    direction;   // -1 short, 0 none, +1 long
   double size;
   double reference_price;
   long   decision_id; // caller-supplied deterministic decision identity
  };

//+------------------------------------------------------------------+
//| Shadow-only execution gateway.                                   |
//+------------------------------------------------------------------+
class CShadowOrderGateway
  {
private:
   long   m_shadow_intents;
   string m_log[1024];
   int    m_log_count;

   void Log(const string line)
     {
      if(m_log_count < 1024)
         m_log[m_log_count++] = line;
     }

public:
   CShadowOrderGateway()
     {
      m_shadow_intents = 0;
      m_log_count      = 0;
     }

   //--- Record a shadow intent. This performs NO broker interaction.
   bool RecordShadowIntent(const AuraShadowIntent &intent)
     {
      if(intent.symbol == "")
         return(false);
      ++m_shadow_intents;
      Log("SHADOW|" + intent.symbol + "|" + AuraTimeframeLabel(intent.tf_index) + "|dir=" +
          IntegerToString(intent.direction) + "|size=" + AuraFormatFixed(intent.size) +
          "|ref=" + AuraFormatFixed(intent.reference_price) + "|decision=" +
          IntegerToString(intent.decision_id));
      return(true);
     }

   //--- Attempt a live execution. ALWAYS blocked: the system is shadow-only and
   //--- there is no live mode to enable. Places no order, does no I/O.
   AuraLiveExecutionResult RequestLiveExecution(const AuraShadowIntent &intent)
     {
      AuraLiveExecutionResult result;
      result.executed = false;
      result.reason   = AURA_LIVE_BLOCK_SHADOW_ONLY;
      Log("LIVE_BLOCKED|shadow_only|" + intent.symbol + "|" + AuraTimeframeLabel(intent.tf_index));
      return(result);
     }

   long   ShadowIntentCount() const { return(m_shadow_intents); }
   int    LogSize()           const { return(m_log_count); }
   string LogEntry(const int i) const { return(i >= 0 && i < m_log_count) ? m_log[i] : ""; }
  };

#endif // AURA_MT5_SHADOWORDERGATEWAY_MQH
//+------------------------------------------------------------------+
