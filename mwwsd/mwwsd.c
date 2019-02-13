
#include <stdio.h>
#include <stdlib.h>

#include <pthread.h>

#include <libwebsockets.h>
#include <MidWay.h>
#include <ipctables.h>
#include <ipcmessages.h>
#include <mwclientapi.h>
#include <mwclientipcapi.h>
#include <address.h>

#include "mwwsd.h"


/**
 * HTTP handler is required by the libwebsocket 
 */
static int callback_http(
    struct lws *wsi,
    enum lws_callback_reasons reason,
    void *user,
    void *in,
    size_t len
) {
   
   /* char buf[8000] = {0}; */
   /* lws_hdr_copy(wsi, buf, 7000, WSI_TOKEN_GET_URI); */
   /* printf ("%s = %s\n", lws_token_to_string( WSI_TOKEN_GET_URI), buf); */
   /* printf("hhttps connection event %d  %d\n", reason,  len); */
  if ( reason == LWS_CALLBACK_GET_THREAD_ID) return 0;

   lws_sockfd_type sockfd = lws_get_socket_fd( wsi );
   DEBUG2("https connection fd %d reason %d(%s)", sockfd,
	 reason, lbl_lws_callback_reasons(reason) );
   
   return 0;
}

static struct lws_protocols protocols[] = {
  /* first protocol must always be HTTP handler */
  {
    "http-only",   // name
    callback_http, // callback
    0              // per_session_data_size
  },
  {
    "midway-1", // protocol name - very important!
    callback_midway_ws,   // callback
    0                     // we don't use any per session data
  },
  {
    NULL, NULL, 0   /* End of list */
  }
};

struct config_t config = { 0 };

struct lws_context *context;

void usage(char * arg0)
{
  printf ("%s: [-A ipcuri] [-l level] [-L logprefix]  [-p WSport] \n",
	  arg0);
  printf ("-A ipcuri    : The IPC url to a running MidWay instance\n");
  printf ("-l level     : log level\n");
  printf ("-L logprefix : The prefix for the logfile\n");
  printf ("-p WSport    : The port to listen for WebSocket connections\n");
  printf ("\nWebSockets server for MidWay.\n\n");
  printf ("This is needed for the simple javascript client library midway.js\n");
  printf ("either for browsers or node.js. node.js have a native\n");
  printf ("interface to the full MidWay API but requires that the full \n");
  printf ("MidWay is installed that required that the OS is either\n");
  printf ("Linux or MAX OSX.\n");
  printf ("\n");
  printf ("The WebSocket API/Protocol make it easy to implement the MidWay API.\n");
  printf ("The result is that midway.js become a very small JS library/module.\n");
  printf ("\n");
};


int doshutdown = 0;

void onSignal(int sig) {
   doshutdown = 1;
}
   

int main(int argc, char ** argv) {
  
  int port = 9000;
  const char *interface = NULL;
  char c, *name;
  char logprefix[PATH_MAX] = "mwwsd";
  char * uri = NULL;
  int rc;
  int llloglevel = LLL_ERR| LLL_WARN | LLL_NOTICE | LLL_INFO;
  int mwloglevel = MWLOG_DEBUG;
  mwaddress_t * mwaddress;

  // default config
  config.alwaysBinaryData = FALSE;
  config.useThreads = TRUE;
  //config.useThreads = FALSE;

  if (getenv("MWNOTHREADS")  != NULL) config.useThreads  = FALSE;
  if (getenv("MWBINDATA")  != NULL) config.alwaysBinaryData  = TRUE;

 
  // we're not using ssl (yet)
  const char *cert_path = NULL;
  const char *key_path = NULL;

  name = strrchr(argv[0], '/');
  if (name == NULL) name = argv[0];
  else name++;

    /* first of all do command line options */
  while((c = getopt(argc,argv, "A:l:p:L:")) != EOF ){
    switch (c) {       
    case 'l':
       rc =  _mwstr2loglevel(optarg);
       if (rc != -1) mwloglevel  = rc;
       else usage(argv[0]);
      break;

    case 'L':
      strncpy(logprefix, optarg, PATH_MAX);
      //printf("logprefix = \"%s\" at %s:%d\n", logprefix, __FUNCTION__, __LINE__);
      break;

    case 'A':
      uri = strdup(optarg);
      //printf("attaching %s\n", uri);
     break;
      
    case 'p':
      port = atoi(optarg);
      break;

    default:
      usage(argv[0]);
      break;
    }
  }
  
  // no special options
  int opts = 0;
  struct lws_context_creation_info info = {0};

  info.port = port;
  info.iface = NULL;
  info.protocols = protocols;
  
  
  // create libwebsocket context representing this server
  llloglevel |= LLL_DEBUG ;
  mwopenlog(name, "./", mwloglevel);
  _mw_copy_on_stderr(TRUE);

  
  lws_set_log_level(llloglevel,  logwrapper /* lwsl_emit_stderr */);
  context = lws_create_context(&info);
   
  if (context == NULL) {
    fprintf(stderr, "libwebsocket init failed\n");
    return -1;
  }
 
  signal(SIGINT, onSignal);
  signal(SIGTERM, onSignal);
  signal(SIGHUP, onSignal);
  signal(SIGQUIT, onSignal);
    
  init_subscription_store();
  init_pendingcall_store();

  mwlog(MWLOG_INFO, "Starting WebSockets Server");
  debug ("MWNOTHREADS=%s useThreads=%d",
	 getenv("MWNOTHREADS"), config.useThreads);
  debug ("MWBINDATA=%s alwaysBinaryData=%d",
	 getenv("MWBINDATA"), config.alwaysBinaryData);

  debug("attaching %s\n", uri);
  rc = mwattach(uri, "WebSocketServer", MWCLIENT);
  if (rc != 0) {
     Error("MidWay instance is not running");
     exit(rc);
  };

  debug("subscribing to all events \n");

  _mw_ipcsend_subscribe ("*", 1, MWEVGLOB);

  pthread_t sender_thread;
  if (config.useThreads)
     pthread_create(&sender_thread, NULL, sender_thread_main, NULL);
     
  // infinite loop, to end this server send SIGTERM. (CTRL+C)
  while (!doshutdown) {
    // libwebsocket_service will process all waiting events with
    // their callback functions and then wait 50 ms.
    // (this is a single threaded web server and this will keep our
    // server from generating load while there are not
    // requests to process)
    lws_service(context, 50);
    if (!config.useThreads) {
       int rc = doMidWayIPCMessage(1);
       if (rc < 0) {
	  doshutdown = 1;
       }
    }
       
  }
   
  lws_context_destroy(context);
  _mw_detach_ipc();
  Info("Bye!");
  
  return 0;
}
