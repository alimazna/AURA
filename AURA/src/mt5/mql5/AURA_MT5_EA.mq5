//+------------------------------------------------------------------+
//|                                              AURA_MT5_EA.mq5     |
//|  AURA MT5 boundary - THE ONE physical Expert Advisor.            |
//|                                                                  |
//|  MT5-0007. This is the single physical EA entry point. It owns    |
//|  exactly one CStreamManager (nine logical timeframe streams) and  |
//|  exactly one CShadowOrderGateway, over exactly one transport.     |
//|                                                                  |
//|  There is NO second EA and NO per-timeframe EA. The nine logical  |
//|  streams (M1, M5, M15, M30, H1, H4, D1, W1, MN1) are explicit.   |
//|                                                                  |
//|  SHADOW-ONLY: this EA never places, modifies or closes a broker   |
//|  order. It contains no OrderSend/PositionModify path. Live         |
//|  execution is blocked/unavailable.                                |
//+------------------------------------------------------------------+
#property copyright   "AURA"
#property version     "1.00"
#property description "AURA MT5 boundary: one EA, one transport, nine logical timeframe streams. Shadow-only."

#include "Common.mqh"
#include "Protocol.mqh"
#include "SocketClient.mqh"
#include "TimeframeStream.mqh"
#include "StreamManager.mqh"
#include "ShadowOrderGateway.mqh"

//--- inputs (data-boundary configuration only; no trading inputs exist)
input string InpSymbol        = "";          // empty => current chart symbol
input string InpHost          = "127.0.0.1"; // C++ data/IPC layer host
input int    InpPort          = 9000;        // C++ data/IPC layer port
input bool   InpEmitQuotes    = true;        // publish TICK context (health only)

//--- the one physical EA's single stream manager and shadow gateway
CStreamManager       g_streams;
CShadowOrderGateway  g_shadow;
string               g_symbol = "";

//+------------------------------------------------------------------+
int OnInit()
  {
   g_symbol = (InpSymbol == "") ? _Symbol : InpSymbol;

   //--- exactly nine independent logical streams under this one EA
   if(!g_streams.Init(g_symbol))
     {
      Print("AURA_MT5_EA: failed to initialise the nine logical streams");
      return(INIT_FAILED);
     }
   if(g_streams.StreamCount() != AURA_STREAM_COUNT)
     {
      Print("AURA_MT5_EA: stream count mismatch (expected ", AURA_STREAM_COUNT, ")");
      return(INIT_FAILED);
     }

   g_streams.ConfigureTransport(InpHost, InpPort);
   //--- A transport failure at start does not abort the EA: streams stay
   //--- independent and reconnect is attempted on the timer.
   g_streams.Connect();

   EventSetTimer(5);
   Print("AURA_MT5_EA: one physical EA, ", g_streams.StreamCount(),
         " logical streams, symbol=", g_symbol, " shadow-only");
   return(INIT_SUCCEEDED);
  }

//+------------------------------------------------------------------+
void OnTick()
  {
   //--- TICK is execution/health context only; it is never a directional
   //--- timeframe. It drives closed-bar capture and heartbeat multiplexing.
   const long receive_ns = (long)TimeCurrent() * 1000000000;
   g_streams.ProcessTick(receive_ns);

   if(InpEmitQuotes && g_streams.IsConnected())
     {
      //--- TICK context frame belongs to the M1 stream (index 0) as broker/
      //--- execution health context, not structural or operational authority.
      const string quote = g_streams.EmitQuoteForStream(0, receive_ns, receive_ns);
      if(quote != "")
         g_streams.SendRaw(quote);
     }
  }

//+------------------------------------------------------------------+
void OnTimer()
  {
   //--- periodic heartbeat and explicit transport recovery attempt
   g_streams.ProcessHeartbeat();
   if(!g_streams.IsConnected())
      g_streams.Connect();
  }

//+------------------------------------------------------------------+
void OnDeinit(const int reason)
  {
   EventKillTimer();
   Print("AURA_MT5_EA: deinit reason=", reason, " shadow_intents=", g_shadow.ShadowIntentCount());
  }

//+------------------------------------------------------------------+
//| Shadow-mode helper: record a shadow intent and prove live execution
//| is unavailable. Retained so the shadow-only invariant is explicit and
//| auditable. This calls no broker function.
//+------------------------------------------------------------------+
void AuraRecordShadowDecision(const int tf_index, const int direction, const double size,
                              const double reference_price, const long decision_id)
  {
   AuraShadowIntent intent;
   intent.symbol          = g_symbol;
   intent.tf_index        = tf_index;
   intent.direction       = direction;
   intent.size            = size;
   intent.reference_price = reference_price;
   intent.decision_id     = decision_id;
   g_shadow.RecordShadowIntent(intent);

   //--- Attempting live execution must always be refused.
   const AuraLiveExecutionResult live = g_shadow.RequestLiveExecution(intent);
   if(live.executed)
      Print("AURA_MT5_EA: FATAL shadow-only invariant violated");
  }
//+------------------------------------------------------------------+
