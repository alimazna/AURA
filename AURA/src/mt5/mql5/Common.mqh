//+------------------------------------------------------------------+
//|                                                Common.mqh        |
//|  AURA MT5 boundary - shared value types and pure helpers.        |
//|                                                                  |
//|  MT5-0001. One physical EA, nine explicit logical timeframe       |
//|  streams. Identity is explicit (never positional). No trading,    |
//|  no order functions, no I/O.                                      |
//+------------------------------------------------------------------+
#property copyright "AURA"
#property version   "1.00"

#ifndef AURA_MT5_COMMON_MQH
#define AURA_MT5_COMMON_MQH

//--- canonical wire contract
#define AURA_PROTOCOL_VERSION "1.0.0"
#define AURA_SCHEMA_VERSION   "1.0.0"
#define AURA_STREAM_COUNT     9

//--- message classes carried by the single transport
enum ENUM_AURA_MSG_TYPE
  {
   AURA_MSG_UNKNOWN     = 0,
   AURA_MSG_BAR_CLOSED  = 1,
   AURA_MSG_QUOTE       = 2,
   AURA_MSG_SYMBOL_SPEC = 3,
   AURA_MSG_HEARTBEAT   = 4,
   AURA_MSG_HEALTH      = 5,
   AURA_MSG_ERROR       = 6
  };

//--- per-stream data quality (mirrors foundation DataQualityState)
enum ENUM_AURA_QUALITY
  {
   AURA_Q_UNKNOWN  = 0,
   AURA_Q_VALID    = 1,
   AURA_Q_DEGRADED = 2,
   AURA_Q_INVALID  = 3,
   AURA_Q_STALE    = 4,
   AURA_Q_MISSING  = 5
  };

//--- per-stream health (mirrors foundation ServiceState)
enum ENUM_AURA_HEALTH
  {
   AURA_H_STARTING = 0,
   AURA_H_ONLINE   = 1,
   AURA_H_DEGRADED = 2,
   AURA_H_OFFLINE  = 3,
   AURA_H_BLOCKED  = 4
  };

//+------------------------------------------------------------------+
//| Canonical nine timeframe streams, in V3-29 order (M1 .. MN1).    |
//| Index is a stable enumeration position, not a message position.  |
//+------------------------------------------------------------------+
string AuraTimeframeLabel(const int index)
  {
   switch(index)
     {
      case 0: return("M1");
      case 1: return("M5");
      case 2: return("M15");
      case 3: return("M30");
      case 4: return("H1");
      case 5: return("H4");
      case 6: return("D1");
      case 7: return("W1");
      case 8: return("MN1");
     }
   return("UNKNOWN");
  }

ENUM_TIMEFRAMES AuraTimeframeEnum(const int index)
  {
   switch(index)
     {
      case 0: return(PERIOD_M1);
      case 1: return(PERIOD_M5);
      case 2: return(PERIOD_M15);
      case 3: return(PERIOD_M30);
      case 4: return(PERIOD_H1);
      case 5: return(PERIOD_H4);
      case 6: return(PERIOD_D1);
      case 7: return(PERIOD_W1);
      case 8: return(PERIOD_MN1);
     }
   return(PERIOD_CURRENT);
  }

string AuraMessageTypeLabel(const ENUM_AURA_MSG_TYPE type)
  {
   switch(type)
     {
      case AURA_MSG_BAR_CLOSED:  return("BAR_CLOSED");
      case AURA_MSG_QUOTE:       return("QUOTE");
      case AURA_MSG_SYMBOL_SPEC: return("SYMBOL_SPEC");
      case AURA_MSG_HEARTBEAT:   return("HEARTBEAT");
      case AURA_MSG_HEALTH:      return("HEALTH");
      case AURA_MSG_ERROR:       return("ERROR");
      default:                   return("UNKNOWN");
     }
  }

//+------------------------------------------------------------------+
//| A closed bar captured from one stream.                           |
//+------------------------------------------------------------------+
struct AuraBar
  {
   string            symbol;
   int               tf_index;     // 0..8, explicit stream identity
   datetime          open_time;    // authoritative bar open (server time)
   datetime          close_time;   // authoritative bar close (server time)
   double            open;
   double            high;
   double            low;
   double            close;
   double            volume;
  };

//+------------------------------------------------------------------+
//| Locale-independent fixed 8-decimal formatting. Mirrors the C++   |
//| ProtocolCodec::format_fixed so both endpoints encode identically |
//| for the same value (cross-language byte-exactness is UNVERIFIED  |
//| here because MetaEditor is unavailable in this environment).     |
//+------------------------------------------------------------------+
string AuraFormatFixed(double value)
  {
   const long scale = 100000000; // 10^8
   bool negative = (value < 0.0);
   double magnitude = negative ? -value : value;
   long scaled = (long)(magnitude * (double)scale + 0.5);
   long integer = scaled / scale;
   long fraction = scaled % scale;
   string frac = IntegerToString(fraction);
   while(StringLen(frac) < 8)
      frac = "0" + frac;
   string out = IntegerToString(integer) + "." + frac;
   if(negative)
      out = "-" + out;
   return(out);
  }

//+------------------------------------------------------------------+
//| Deterministic OHLCV payload: O,H,L,C,V,closed.                   |
//+------------------------------------------------------------------+
string AuraBarPayload(const AuraBar &bar)
  {
   return(AuraFormatFixed(bar.open) + "," + AuraFormatFixed(bar.high) + "," +
          AuraFormatFixed(bar.low) + "," + AuraFormatFixed(bar.close) + "," +
          AuraFormatFixed(bar.volume) + ",1");
  }

//+------------------------------------------------------------------+
//| Lowercase SHA-256 hex of a UTF-8 byte string via native hashing. |
//+------------------------------------------------------------------+
string AuraSha256Hex(const string text)
  {
   uchar src[];
   StringToCharArray(text, src, 0, StringLen(text), CP_UTF8);
   uchar key[];
   uchar digest[];
   if(CryptEncode(CRYPT_HASH_SHA256, src, key, digest) <= 0)
      return("");
   string hex = "";
   const string digits = "0123456789abcdef";
   for(int i = 0; i < ArraySize(digest); ++i)
     {
      const int b = (int)digest[i];
      hex += StringSubstr(digits, (b >> 4) & 0x0F, 1);
      hex += StringSubstr(digits, b & 0x0F, 1);
     }
   return(hex);
  }

#endif // AURA_MT5_COMMON_MQH
//+------------------------------------------------------------------+
