//+------------------------------------------------------------------+
//|                                              Protocol.mqh        |
//|  AURA MT5 boundary - deterministic message framing.              |
//|                                                                  |
//|  MT5-0002. Produces the same canonical frame layout as the C++   |
//|  ProtocolCodec:                                                  |
//|                                                                  |
//|   protocol|schema|message_id|type|symbol|timeframe|sequence|     |
//|   event_ns|recv_ns|payload|checksum                              |
//|                                                                  |
//|  message_id uses ':' internally so it never contains the frame   |
//|  delimiter '|'. The timeframe is always explicit; identity is    |
//|  never positional. No trading functions.                         |
//+------------------------------------------------------------------+
#property copyright "AURA"
#property version   "1.00"

#ifndef AURA_MT5_PROTOCOL_MQH
#define AURA_MT5_PROTOCOL_MQH

#include "Common.mqh"

//+------------------------------------------------------------------+
//| One framed message ready for the single transport.               |
//+------------------------------------------------------------------+
struct AuraFrame
  {
   string              message_id;
   ENUM_AURA_MSG_TYPE  type;
   string              symbol;
   int                 tf_index;
   long                sequence;
   long                event_time_ns;    // event time (bar close), nanoseconds
   long                receive_time_ns;  // receive time, nanoseconds, kept separate
   string              payload;
   string              frame;            // fully encoded wire frame
  };

//+------------------------------------------------------------------+
//| Deterministic message identity (stable across processes).        |
//+------------------------------------------------------------------+
string AuraMessageId(const ENUM_AURA_MSG_TYPE type, const string symbol,
                     const int tf_index, const long sequence, const long event_ns)
  {
   return("mt5:" + AuraTimeframeLabel(tf_index) + ":" + IntegerToString(sequence) + ":" +
          IntegerToString(event_ns) + ":" + AuraMessageTypeLabel(type) + ":" + symbol);
  }

//+------------------------------------------------------------------+
//| Encodes a frame. `event_ns` is the authoritative event time;      |
//| `receive_ns` is the local receive time and is never substituted   |
//| for the event time.                                              |
//+------------------------------------------------------------------+
string AuraEncodeFrame(const ENUM_AURA_MSG_TYPE type, const string symbol,
                       const int tf_index, const long sequence,
                       const long event_ns, const long receive_ns,
                       const string payload)
  {
   const string mid = AuraMessageId(type, symbol, tf_index, sequence, event_ns);
   const string canon = AURA_PROTOCOL_VERSION + "|" + AURA_SCHEMA_VERSION + "|" + mid + "|" +
                        AuraMessageTypeLabel(type) + "|" + symbol + "|" +
                        AuraTimeframeLabel(tf_index) + "|" + IntegerToString(sequence) + "|" +
                        IntegerToString(event_ns) + "|" + IntegerToString(receive_ns) + "|" +
                        payload;
   const string checksum = AuraSha256Hex(canon);
   return(canon + "|" + checksum);
  }

//+------------------------------------------------------------------+
//| Encodes a closed-bar stream frame from a captured bar.           |
//+------------------------------------------------------------------+
string AuraEncodeClosedBar(const AuraBar &bar, const long sequence, const long receive_ns)
  {
   const long event_ns = (long)bar.close_time * 1000000000; // server seconds -> ns
   return(AuraEncodeFrame(AURA_MSG_BAR_CLOSED, bar.symbol, bar.tf_index, sequence, event_ns,
                          receive_ns, AuraBarPayload(bar)));
  }

//+------------------------------------------------------------------+
//| Encodes a heartbeat frame so the receiver can track stream health|
//| independently of bar delivery.                                  |
//+------------------------------------------------------------------+
string AuraEncodeHeartbeat(const string symbol, const int tf_index, const long sequence,
                           const long event_ns, const long receive_ns)
  {
   return(AuraEncodeFrame(AURA_MSG_HEARTBEAT, symbol, tf_index, sequence, event_ns, receive_ns, ""));
  }

//+------------------------------------------------------------------+
//| Encodes a stream health frame (per-stream health reporting).     |
//+------------------------------------------------------------------+
string AuraEncodeHealth(const string symbol, const int tf_index, const long sequence,
                        const long event_ns, const long receive_ns, const ENUM_AURA_HEALTH health)
  {
   return(AuraEncodeFrame(AURA_MSG_HEALTH, symbol, tf_index, sequence, event_ns, receive_ns,
                          IntegerToString((int)health)));
  }

#endif // AURA_MT5_PROTOCOL_MQH
//+------------------------------------------------------------------+
