
#include <stdio.h>
#include <stdlib.h>

#include <pthread.h>

#include <libwebsockets.h>
#include <MidWay.h>

#include "mwwsd.h"

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
   debug("https connection fd %d reason %d", sockfd, reason);
   
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
    0                          // we don't use any per session data
  },
  {
    NULL, NULL, 0   /* End of list */
  }
};


struct lws_context *context;

void * sender_thread_main(void * arg) {


   while(1) {

      debug("sender thread sleep");
      sleep(10);
      debug("sender thread wake");
      queue_message();
      lws_cancel_service(context) ;
   }

   return NULL;
}

int main(void) {
  // server url will be http://localhost:9000
  int port = 9000;
  const char *interface = NULL;
  
  // we're not using ssl
  const char *cert_path = NULL;
  const char *key_path = NULL;

  // no special options
  int opts = 0;
  struct lws_context_creation_info info = {0};

  info.port = port;
  info.iface = NULL;
  info.protocols = protocols;
  
  
  // create libwebsocket context representing this server
  context = lws_create_context(&info);
   
  if (context == NULL) {
    fprintf(stderr, "libwebsocket init failed\n");
    return -1;
  }
   
  printf("starting server...\n");
  pthread_t sender_thread;

  pthread_create(&sender_thread, NULL, sender_thread_main, NULL);
     
  // infinite loop, to end this server send SIGTERM. (CTRL+C)
  while (1) {
    lws_service(context, 50);
    // libwebsocket_service will process all waiting events with
    // their callback functions and then wait 50 ms.
    // (this is a single threaded web server and this will keep our
    // server from generating load while there are not
    // requests to process)

    drain_queue();
  }
   
  lws_context_destroy(context);
   
  return 0;
}
