//+------------------------------------------------------------------+
//|                                           SocketClient.mqh       |
//|  AURA MT5 boundary - the ONE shared transport.                   |
//|                                                                  |
//|  MT5-0003. A single socket carries every stream's frames for the  |
//|  one physical EA. There is no per-stream socket. Transport         |
//|  failure must not erase logical stream state (that state lives in  |
//|  TimeframeStream/StreamManager and survives a disconnect).         |
//|  No trading functions.                                            |
//+------------------------------------------------------------------+
#property copyright "AURA"
#property version   "1.00"

#ifndef AURA_MT5_SOCKETCLIENT_MQH
#define AURA_MT5_SOCKETCLIENT_MQH

//+------------------------------------------------------------------+
//| Single-connection TCP transport.                                 |
//+------------------------------------------------------------------+
class CSocketClient
  {
private:
   int      m_socket;
   string   m_host;
   int      m_port;
   bool     m_connected;
   datetime m_last_attempt;
   long     m_sent_count;
   long     m_bytes_sent;
   string   m_last_error;

public:
   CSocketClient()
     {
      m_socket      = INVALID_HANDLE;
      m_host        = "";
      m_port        = 0;
      m_connected   = false;
      m_last_attempt = 0;
      m_sent_count  = 0;
      m_bytes_sent  = 0;
      m_last_error  = "";
     }

   ~CSocketClient()
     {
      Disconnect();
     }

   //--- Configure the canonical endpoint (does not connect).
   void Configure(const string host, const int port)
     {
      m_host = host;
      m_port = port;
     }

   //--- Connect the single transport. Returns false on failure and never throws.
   bool Connect()
     {
      m_last_attempt = TimeCurrent();
      if(m_socket != INVALID_HANDLE)
        {
         SocketClose(m_socket);
         m_socket = INVALID_HANDLE;
        }
      m_socket = SocketCreate();
      if(m_socket == INVALID_HANDLE)
        {
         m_connected = false;
         m_last_error = "SocketCreate failed";
         return(false);
        }
      if(!SocketConnect(m_socket, m_host, m_port, 5000))
        {
         SocketClose(m_socket);
         m_socket = INVALID_HANDLE;
         m_connected = false;
         m_last_error = "SocketConnect failed";
         return(false);
        }
      m_connected = true;
      m_last_error = "";
      return(true);
     }

   //--- Close the transport without touching any logical stream state.
   void Disconnect()
     {
      if(m_socket != INVALID_HANDLE)
        {
         SocketClose(m_socket);
         m_socket = INVALID_HANDLE;
        }
      m_connected = false;
     }

   //--- Ensure connected, reconnecting if needed. Stream state is untouched.
   bool EnsureConnected()
     {
      if(m_connected && m_socket != INVALID_HANDLE)
         return(true);
      return(Connect());
     }

   //--- Send one already-encoded frame. Never throws.
   bool SendFrame(const string frame)
     {
      if(!EnsureConnected())
         return(false);
      uchar bytes[];
      const int len = StringToCharArray(frame, bytes, 0, StringLen(frame), CP_UTF8);
      if(len <= 0)
         return(false);
      const int sent = SocketSend(m_socket, bytes, len);
      if(sent < len)
        {
         m_connected = false;
         m_last_error = "partial/failed send";
         return(false);
        }
      ++m_sent_count;
      m_bytes_sent += (long)sent;
      m_last_error = "";
      return(true);
     }

   //--- Read any pending bytes (e.g. a server ACK). Returns bytes read.
   int Receive(uchar &buffer[])
     {
      if(!m_connected || m_socket == INVALID_HANDLE)
         return(0);
      uint len = 0;
      return(SocketRead(m_socket, buffer, len));
     }

   //--- Drive the socket state machine.
   void Poll()
     {
      if(m_socket != INVALID_HANDLE)
         SocketIsConnected(m_socket);
     }

   bool   IsConnected() const { return(m_connected && m_socket != INVALID_HANDLE); }
   long   SentCount()   const { return(m_sent_count); }
   long   BytesSent()   const { return(m_bytes_sent); }
   string LastError()   const { return(m_last_error); }
   string Host()        const { return(m_host); }
   int    Port()        const { return(m_port); }
  };

#endif // AURA_MT5_SOCKETCLIENT_MQH
//+------------------------------------------------------------------+
