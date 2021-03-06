#include <stdio.h>

// extra libs
#include <libwebsockets.h>

// MidWay libs
#include <MidWay.h>
#include "mwwsd.h"

void logwrapper(int level, const char *line)  {

   int len = strlen(line) ;
   if (line[len-1] == '\n') len--;
   
   switch (  level) {

   case LLL_ERR:
      level = MWLOG_ERROR;
      break;
   case LLL_WARN:
      level = MWLOG_WARNING;
      break;
   case LLL_INFO:
   case LLL_NOTICE:
      level = MWLOG_INFO;
      break;
   case LLL_DEBUG:
      level = MWLOG_DEBUG;
      break;

   default:
      mwlog(MWLOG_WARNING, " %d %.*s", level, len, line);
      return;
   }
   
   mwlog(level, "%.*s", len, line);
   return;
}


#define CASE(reason) case reason: return #reason;

char * lbl_lws_callback_reasons(int reason) {
      
   switch (reason) {
	   
      CASE( LWS_CALLBACK_PROTOCOL_INIT);
      CASE( LWS_CALLBACK_PROTOCOL_DESTROY);
      CASE( LWS_CALLBACK_WSI_CREATE);
      CASE( LWS_CALLBACK_WSI_DESTROY);
 
      CASE( LWS_CALLBACK_OPENSSL_LOAD_EXTRA_CLIENT_VERIFY_CERTS);
      CASE( LWS_CALLBACK_OPENSSL_LOAD_EXTRA_SERVER_VERIFY_CERTS);
      CASE( LWS_CALLBACK_OPENSSL_PERFORM_CLIENT_CERT_VERIFICATION);
      CASE( LWS_CALLBACK_OPENSSL_CONTEXT_REQUIRES_PRIVATE_KEY);
 
      CASE( LWS_CALLBACK_SSL_INFO);
      CASE( LWS_CALLBACK_OPENSSL_PERFORM_SERVER_CERT_VERIFICATION);
      CASE( LWS_CALLBACK_SERVER_NEW_CLIENT_INSTANTIATED);
      CASE( LWS_CALLBACK_HTTP);
 
      CASE( LWS_CALLBACK_HTTP_BODY);
      CASE( LWS_CALLBACK_HTTP_BODY_COMPLETION);
      CASE( LWS_CALLBACK_HTTP_FILE_COMPLETION);
      CASE( LWS_CALLBACK_HTTP_WRITEABLE);
 
      CASE( LWS_CALLBACK_CLOSED_HTTP);
      CASE( LWS_CALLBACK_FILTER_HTTP_CONNECTION);
      CASE( LWS_CALLBACK_ADD_HEADERS);
      CASE( LWS_CALLBACK_CHECK_ACCESS_RIGHTS);
 
      CASE( LWS_CALLBACK_PROCESS_HTML);
      CASE( LWS_CALLBACK_HTTP_BIND_PROTOCOL);
      CASE( LWS_CALLBACK_HTTP_DROP_PROTOCOL);
      //CASE( LWS_CALLBACK_HTTP_CONFIRM_UPGRADE);
 
      CASE( LWS_CALLBACK_ESTABLISHED_CLIENT_HTTP);
      CASE( LWS_CALLBACK_CLOSED_CLIENT_HTTP);
      CASE( LWS_CALLBACK_RECEIVE_CLIENT_HTTP_READ);
      CASE( LWS_CALLBACK_RECEIVE_CLIENT_HTTP);
 
      CASE( LWS_CALLBACK_COMPLETED_CLIENT_HTTP);
      CASE( LWS_CALLBACK_CLIENT_HTTP_WRITEABLE);
      //CASE( LWS_CALLBACK_CLIENT_HTTP_BIND_PROTOCOL);
      //CASE( LWS_CALLBACK_CLIENT_HTTP_DROP_PROTOCOL);
 
      CASE( LWS_CALLBACK_ESTABLISHED);
      CASE( LWS_CALLBACK_CLOSED);
      CASE( LWS_CALLBACK_SERVER_WRITEABLE);
      CASE( LWS_CALLBACK_RECEIVE);
 
      CASE( LWS_CALLBACK_RECEIVE_PONG);
      CASE( LWS_CALLBACK_WS_PEER_INITIATED_CLOSE);
      CASE( LWS_CALLBACK_FILTER_PROTOCOL_CONNECTION);
      CASE( LWS_CALLBACK_CONFIRM_EXTENSION_OKAY);
 
      //CASE( LWS_CALLBACK_WS_SERVER_BIND_PROTOCOL);
      //CASE( LWS_CALLBACK_WS_SERVER_DROP_PROTOCOL);
      CASE( LWS_CALLBACK_CLIENT_CONNECTION_ERROR);
      CASE( LWS_CALLBACK_CLIENT_FILTER_PRE_ESTABLISH);
 
      CASE( LWS_CALLBACK_CLIENT_ESTABLISHED);
      //CASE( LWS_CALLBACK_CLIENT_CLOSED);
      CASE( LWS_CALLBACK_CLIENT_APPEND_HANDSHAKE_HEADER);
      CASE( LWS_CALLBACK_CLIENT_RECEIVE);
 
      CASE( LWS_CALLBACK_CLIENT_RECEIVE_PONG);
      CASE( LWS_CALLBACK_CLIENT_WRITEABLE);
      CASE( LWS_CALLBACK_CLIENT_CONFIRM_EXTENSION_SUPPORTED);
      CASE( LWS_CALLBACK_WS_EXT_DEFAULTS);
 
      CASE( LWS_CALLBACK_FILTER_NETWORK_CONNECTION);
      //CASE( LWS_CALLBACK_WS_CLIENT_BIND_PROTOCOL);
      //CASE( LWS_CALLBACK_WS_CLIENT_DROP_PROTOCOL);
      CASE( LWS_CALLBACK_GET_THREAD_ID);
 
      CASE( LWS_CALLBACK_ADD_POLL_FD);
      CASE( LWS_CALLBACK_DEL_POLL_FD);
      CASE( LWS_CALLBACK_CHANGE_MODE_POLL_FD);
      CASE( LWS_CALLBACK_LOCK_POLL);
 
      CASE( LWS_CALLBACK_UNLOCK_POLL);
      CASE( LWS_CALLBACK_CGI);
      CASE( LWS_CALLBACK_CGI_TERMINATED);
      CASE( LWS_CALLBACK_CGI_STDIN_DATA);
 
      CASE( LWS_CALLBACK_CGI_STDIN_COMPLETED);
      //CASE( LWS_CALLBACK_CGI_PROCESS_ATTACH);
      CASE( LWS_CALLBACK_SESSION_INFO);
      CASE( LWS_CALLBACK_GS_EVENT);
 
      CASE( LWS_CALLBACK_HTTP_PMO);
      //CASE( LWS_CALLBACK_RAW_PROXY_CLI_RX);
      //CASE( LWS_CALLBACK_RAW_PROXY_SRV_RX);
      //CASE( LWS_CALLBACK_RAW_PROXY_CLI_CLOSE);
 
      //CASE( LWS_CALLBACK_RAW_PROXY_SRV_CLOSE);
      //CASE( LWS_CALLBACK_RAW_PROXY_CLI_WRITEABLE);
      //CASE( LWS_CALLBACK_RAW_PROXY_SRV_WRITEABLE);
      //CASE( LWS_CALLBACK_RAW_PROXY_CLI_ADOPT);
 
      //CASE( LWS_CALLBACK_RAW_PROXY_SRV_ADOPT);
      //CASE( LWS_CALLBACK_RAW_PROXY_CLI_BIND_PROTOCOL);
      //CASE( LWS_CALLBACK_RAW_PROXY_SRV_BIND_PROTOCOL);
      //CASE( LWS_CALLBACK_RAW_PROXY_CLI_DROP_PROTOCOL);
 
      //CASE( LWS_CALLBACK_RAW_PROXY_SRV_DROP_PROTOCOL);
      CASE( LWS_CALLBACK_RAW_RX);
      CASE( LWS_CALLBACK_RAW_CLOSE);
      CASE( LWS_CALLBACK_RAW_WRITEABLE);
 
      CASE( LWS_CALLBACK_RAW_ADOPT);
      //CASE( LWS_CALLBACK_RAW_SKT_BIND_PROTOCOL);
      //CASE( LWS_CALLBACK_RAW_SKT_DROP_PROTOCOL);
      CASE( LWS_CALLBACK_RAW_ADOPT_FILE);
 
      CASE( LWS_CALLBACK_RAW_RX_FILE);
      CASE( LWS_CALLBACK_RAW_WRITEABLE_FILE);
      CASE( LWS_CALLBACK_RAW_CLOSE_FILE);
      //CASE( LWS_CALLBACK_RAW_FILE_BIND_PROTOCOL);
 
      //CASE( LWS_CALLBACK_RAW_FILE_DROP_PROTOCOL);
      //CASE( LWS_CALLBACK_TIMER);
      //CASE( LWS_CALLBACK_EVENT_WAIT_CANCELLED);
      CASE( LWS_CALLBACK_CHILD_CLOSING);
 
      //CASE( LWS_CALLBACK_VHOST_CERT_AGING);
      //CASE( LWS_CALLBACK_VHOST_CERT_UPDATE);
      CASE( LWS_CALLBACK_USER);

      default:
	 return "N/A";
   }
}
 
