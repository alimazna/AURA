//+------------------------------------------------------------------+
//|                                        TimeframeStream.mqh       |
//|  AURA MT5 boundary - one logical timeframe stream.               |
//|                                                                  |
//|  MT5-0004. Each of the nine streams owns its own independent      |
//|  state. A failure or recovery of one stream must never mutate      |
//|  another. Only CLOSED bars are captured; the forming bar is never  |
//|  authoritative. Duplicate finalized closed bars are rejected and   |
//|  never repainted. No future timestamps. No trading functions.      |
//+------------------------------------------------------------------+
#property copyright "AURA"
#property version   "1.00"

#ifndef AURA_MT5_TIMEFRAMESTREAM_MQH
#define AURA_MT5_TIMEFRAMESTREAM_MQH

#include "Common.mqh"
#include "Protocol.mqh"

//+------------------------------------------------------------------+
//| Per-stream initialization / recovery state.                      |
//+------------------------------------------------------------------+
enum ENUM_AURA_INIT_STATE
  {
   AURA_INIT_COLD     = 0,  // never started
   AURA_INIT_WARMING  = 1,  // connected, awaiting first closed bar
   AURA_INIT_READY    = 2,  // has processed at least one closed bar
   AURA_INIT_RECOVER  = 3,  // recovering after a gap/disconnect
   AURA_INIT_ERROR    = 4   // failed; independent of other streams
  };

//+------------------------------------------------------------------+
//| One logical timeframe stream.                                    |
//+------------------------------------------------------------------+
class CTimeframeStream
  {
private:
   int                m_tf_index;
   string             m_symbol;
   datetime           m_last_closed_open;
   datetime           m_last_closed_close;
   long               m_sequence;
   bool               m_initialized;
   ENUM_AURA_INIT_STATE m_init_state;
   ENUM_AURA_QUALITY  m_quality;
   ENUM_AURA_HEALTH   m_health;
   string             m_provenance;
   string             m_last_error;
   datetime           m_last_success;
   long               m_accepted_bars;
   long               m_rejected_bars;

public:
   CTimeframeStream()
     {
      m_tf_index        = -1;
      m_symbol          = "";
      m_last_closed_open  = 0;
      m_last_closed_close = 0;
      m_sequence        = 0;
      m_initialized     = false;
      m_init_state      = AURA_INIT_COLD;
      m_quality         = AURA_Q_UNKNOWN;
      m_health          = AURA_H_STARTING;
      m_provenance      = "";
      m_last_error      = "";
      m_last_success    = 0;
      m_accepted_bars   = 0;
      m_rejected_bars   = 0;
     }

   //--- Initialise the stream with its explicit identity. Independent per stream.
   void Init(const int tf_index, const string symbol)
     {
      m_tf_index   = tf_index;
      m_symbol     = symbol;
      m_initialized = true;
      m_init_state = AURA_INIT_WARMING;
      m_health     = AURA_H_STARTING;
      m_quality    = AURA_Q_UNKNOWN;
      m_provenance = "ea-1|" + AuraTimeframeLabel(tf_index) + "|" + symbol;
     }

   //--- Mark the stream ready after its first accepted closed bar.
   void MarkReady()
     {
      m_init_state = AURA_INIT_READY;
      m_health     = AURA_H_ONLINE;
      m_quality    = AURA_Q_VALID;
      m_last_error = "";
     }

   //--- Record a stream-local error. Does not affect other streams.
   void MarkError(const string message, const ENUM_AURA_HEALTH health)
     {
      m_last_error = message;
      m_health     = health;
      m_quality    = AURA_Q_DEGRADED;
      m_init_state = AURA_INIT_ERROR;
     }

   //--- Begin recovery for this stream only.
   void BeginRecovery()
     {
      m_init_state = AURA_INIT_RECOVER;
      m_health     = AURA_H_DEGRADED;
      m_quality    = AURA_Q_UNKNOWN;
     }

   //+---------------------------------------------------------------+
   //| Capture the most recent CLOSED bar for this stream.            |
   //|                                                                |
   //| Returns false when the resolved bar is not a new closed bar    |
   //| (same close_time already finalized), so duplicates and         |
   //| regressions are idempotently rejected and history is never     |
   //| repainted. The forming bar (index 0) is never used.            |
   //+---------------------------------------------------------------+
   bool CaptureClosedBar(AuraBar &out)
     {
      if(!m_initialized)
         return(false);

      const ENUM_TIMEFRAMES tf = AuraTimeframeEnum(m_tf_index);
      if(tf == PERIOD_CURRENT)
         return(false);

      // Wait until the current period has fully closed.
      if(!IsBarClosed(tf))
         return(false);

      MqlRates rates[];
      // Copy bar 0 (the just-closed bar) only; we never read the forming bar.
      const int copied = CopyRates(m_symbol, tf, 0, 1, rates);
      if(copied < 1)
        {
         MarkError("CopyRates failed", AURA_H_DEGRADED);
         return(false);
        }

      const datetime close_time = rates[0].time + PeriodSeconds(tf);
      // Duplicate / regression guard: the same closed bar is rejected.
      if(m_last_closed_close != 0 && close_time <= m_last_closed_close)
        {
         ++m_rejected_bars;
         m_quality = AURA_Q_STALE;
         return(false);
        }

      out.symbol     = m_symbol;
      out.tf_index   = m_tf_index;
      out.open_time  = rates[0].time;
      out.close_time = close_time;
      out.open       = rates[0].open;
      out.high       = rates[0].high;
      out.low        = rates[0].low;
      out.close      = rates[0].close;
      out.volume     = (double)rates[0].tick_volume;

      m_last_closed_open  = rates[0].time;
      m_last_closed_close = close_time;
      ++m_accepted_bars;
      return(true);
     }

   //+---------------------------------------------------------------+
   //| Build the closed-bar frame for this stream and advance its     |
   //| per-stream sequence. `receive_ns` is the local receive time.   |
   //+---------------------------------------------------------------+
   string EmitClosedBar(const AuraBar &bar, const long receive_ns)
     {
      ++m_sequence;
      const string frame = AuraEncodeClosedBar(bar, m_sequence, receive_ns);
      m_last_success = (datetime)(receive_ns / 1000000000);
      MarkReady();
      return(frame);
     }

   string EmitHeartbeat(const long event_ns, const long receive_ns)
     {
      const long seq = m_sequence; // heartbeat does not consume a bar sequence
      return(AuraEncodeHeartbeat(m_symbol, m_tf_index, seq, event_ns, receive_ns));
     }

   //+---------------------------------------------------------------+
   //| Build a real-time quote frame (TICK context is health/execution |
   //| only, never a directional timeframe).                          |
   //+---------------------------------------------------------------+
   string EmitQuote(const long event_ns, const long receive_ns)
     {
      MqlTick tick;
      if(!SymbolInfoTick(m_symbol, tick))
         return("");
      const string payload = "bid=" + AuraFormatFixed(tick.bid) + ",ask=" + AuraFormatFixed(tick.ask);
      return(AuraEncodeFrame(AURA_MSG_QUOTE, m_symbol, m_tf_index, m_sequence, event_ns, receive_ns,
                             payload));
     }

   //--- accessors (state inspection; each stream is independent)
   int                TfIndex()        const { return(m_tf_index); }
   string             Symbol()         const { return(m_symbol); }
   string             Label()          const { return(AuraTimeframeLabel(m_tf_index)); }
   long               Sequence()       const { return(m_sequence); }
   bool               Initialized()    const { return(m_initialized); }
   ENUM_AURA_INIT_STATE InitState()    const { return(m_init_state); }
   ENUM_AURA_QUALITY  Quality()        const { return(m_quality); }
   ENUM_AURA_HEALTH   Health()         const { return(m_health); }
   string             Provenance()     const { return(m_provenance); }
   string             LastError()      const { return(m_last_error); }
   datetime           LastClosedClose() const { return(m_last_closed_close); }
   datetime           LastSuccess()    const { return(m_last_success); }
   long               AcceptedBars()   const { return(m_accepted_bars); }
   long               RejectedBars()   const { return(m_rejected_bars); }
   bool               IsHealthy()      const { return(m_health == AURA_H_ONLINE); }

private:
   //--- True when the current bar of `tf` has finished forming, i.e. the next
   //--- period has begun. We never capture the still-forming bar.
   bool IsBarClosed(const ENUM_TIMEFRAMES tf) const
     {
      const datetime now = TimeCurrent();
      const datetime bar_open = (datetime)(now - (now % PeriodSeconds(tf)));
      // A bar is closed once at least one full period has elapsed since its open.
      return((now - bar_open) >= PeriodSeconds(tf));
     }
  };

#endif // AURA_MT5_TIMEFRAMESTREAM_MQH
//+------------------------------------------------------------------+
