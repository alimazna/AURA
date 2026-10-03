//+------------------------------------------------------------------+
//|                                          StreamManager.mqh       |
//|  AURA MT5 boundary - the nine logical streams over one transport. |
//|                                                                  |
//|  MT5-0005. Owns EXACTLY nine CTimeframeStream objects and the ONE  |
//|  shared CSocketClient. Streams are independently identified by     |
//|  their explicit timeframe; there is no positional multiplexing,    |
//|  no shared mutable bar state, and no stream can reset another.      |
//|  No trading functions.                                            |
//+------------------------------------------------------------------+
#property copyright "AURA"
#property version   "1.00"

#ifndef AURA_MT5_STREAMMANAGER_MQH
#define AURA_MT5_STREAMMANAGER_MQH

#include "Common.mqh"
#include "Protocol.mqh"
#include "SocketClient.mqh"
#include "TimeframeStream.mqh"

//+------------------------------------------------------------------+
//| Manages the nine streams and the single transport.               |
//+------------------------------------------------------------------+
class CStreamManager
  {
private:
   CTimeframeStream m_streams[AURA_STREAM_COUNT];
   CSocketClient    m_transport;      // the ONE transport for all nine streams
   string           m_symbol;
   bool             m_started;
   long             m_total_sent;
   datetime         m_last_heartbeat;

public:
   CStreamManager()
     {
      m_symbol         = "";
      m_started        = false;
      m_total_sent     = 0;
      m_last_heartbeat = 0;
     }

   //--- Configure the single transport endpoint.
   void ConfigureTransport(const string host, const int port)
     {
      m_transport.Configure(host, port);
     }

   //--- Initialise exactly AURA_STREAM_COUNT (9) independent streams.
   bool Init(const string symbol)
     {
      m_symbol = symbol;
      for(int i = 0; i < AURA_STREAM_COUNT; ++i)
         m_streams[i].Init(i, symbol);   // explicit identity, never positional
      m_started = true;
      return(true);
     }

   //--- Connect the one transport. A transport failure does not erase any
   //--- stream's logical state (TimeframeStream keeps it).
   bool Connect() { return(m_transport.Connect()); }

   //--- Per-tick processing: capture any newly closed bar per stream and
   //--- multiplex the frames over the single transport. Each stream is handled
   //--- in isolation; a failure on one never aborts the others.
   void ProcessTick(const long receive_ns)
     {
      if(!m_started)
         return;
      if(!m_transport.EnsureConnected())
        {
         // Transport down: streams keep their state and are marked degraded,
         // but not reset. Recovery is explicit via the next successful send.
         for(int i = 0; i < AURA_STREAM_COUNT; ++i)
            if(m_streams[i].IsHealthy())
               m_streams[i].BeginRecovery();
         return;
        }

      for(int i = 0; i < AURA_STREAM_COUNT; ++i)
        {
         AuraBar bar;
         if(!m_streams[i].CaptureClosedBar(bar))
            continue; // no new closed bar (or duplicate) for this stream
         const string frame = m_streams[i].EmitClosedBar(bar, receive_ns);
         if(m_transport.SendFrame(frame))
            ++m_total_sent;
         // A send failure affects only this stream's health reporting; the
         // closed-bar progress already recorded on the stream is retained.
         else
            m_streams[i].MarkError("send failed", AURA_H_DEGRADED);
        }
     }

   //--- Periodic heartbeat: one frame per stream, over the same one transport.
   void ProcessHeartbeat()
     {
      if(!m_started)
         return;
      const datetime now = TimeCurrent();
      if(now == m_last_heartbeat)
         return;
      m_last_heartbeat = now;
      if(!m_transport.EnsureConnected())
         return;
      const long receive_ns = (long)now * 1000000000;
      for(int i = 0; i < AURA_STREAM_COUNT; ++i)
        {
         const string frame = m_streams[i].EmitHeartbeat(receive_ns, receive_ns);
         if(m_transport.SendFrame(frame))
            ++m_total_sent;
        }
     }

   //--- Stream access by explicit index (0..8). Never by packet position.
   //--- Returns a JSON-like descriptor string for inspection; no pointer is
   //--- exposed (streams are internal and independent).
   string DescribeStream(const int index) const
     {
      if(index < 0 || index >= AURA_STREAM_COUNT)
         return("");
      return(m_streams[index].Label() + "|seq=" + IntegerToString(m_streams[index].Sequence()) +
             "|q=" + IntegerToString((int)m_streams[index].Quality()) +
             "|h=" + IntegerToString((int)m_streams[index].Health()) +
             "|accepted=" + IntegerToString(m_streams[index].AcceptedBars()) +
             "|rejected=" + IntegerToString(m_streams[index].RejectedBars()));
     }

   //--- Send an already-encoded frame over the one transport.
   bool SendRaw(const string frame) { return(m_transport.SendFrame(frame)); }

   //--- Emit a real-time TICK-context quote for one stream (health only).
   string EmitQuoteForStream(const int index, const long event_ns, const long receive_ns)
     {
      if(index < 0 || index >= AURA_STREAM_COUNT)
         return("");
      return(m_streams[index].EmitQuote(event_ns, receive_ns));
     }

   string StreamLabel(const int index) const
     {
      return(index < 0 || index >= AURA_STREAM_COUNT) ? "" : m_streams[index].Label();
     }

   int    StreamCount()  const { return(AURA_STREAM_COUNT); }
   long   TotalSent()    const { return(m_total_sent); }
   bool   IsConnected()  const { return(m_transport.IsConnected()); }

   //--- Count of streams currently healthy (independent per stream).
   int HealthyCount() const
     {
      int count = 0;
      for(int i = 0; i < AURA_STREAM_COUNT; ++i)
         if(m_streams[i].IsHealthy())
            ++count;
      return(count);
     }
  };

#endif // AURA_MT5_STREAMMANAGER_MQH
//+------------------------------------------------------------------+
