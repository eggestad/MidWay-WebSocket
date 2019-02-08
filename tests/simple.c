
#define LWS_USE_LIBUV

#include <stdio.h>
#include <stdlib.h>
#include <libwebsockets.h>

static int callback_http(
    struct lws *wsi,
    enum lws_callback_reasons reason,
    void *user,
    void *in,
    size_t len
) {
    return 0;
}

static int callback_dumb_increment(
    struct lws *wsi,
    enum lws_callback_reasons reason,
    void *user,
    void *in,
    size_t len
) {
  switch (reason) {
    // just log message that someone is connecting
    case LWS_CALLBACK_ESTABLISHED: 
      printf("connection established\n");
      break;
    case LWS_CALLBACK_RECEIVE: { // the funny part
      // Create a buffer to hold our response
      // it has to have some pre and post padding.
      // You don't need to care what comes there, libwebsockets
      // will do everything for you. For more info see
      // http://git.warmcat.com/cgi-bin/cgit/libwebsockets/tree/lib/libwebsockets.h#n597
      unsigned char *buf = (unsigned char*)
          malloc(LWS_SEND_BUFFER_PRE_PADDING + len
              + LWS_SEND_BUFFER_POST_PADDING);
           
      int i;
           
      // Pointer to `void *in` holds the incoming request we're just
      // going to put it in reverse order and put it in `buf` with
      // correct offset. `len` holds length of the request.
      for (i=0; i < len; i++) {
        buf[LWS_SEND_BUFFER_PRE_PADDING + (len - 1) - i ] = 
            ((char *) in)[i];
      }
           
      // Log what we received and what we're going to send as a 
      // response that disco syntax `%.*s` is used to print just a
      // part of our buffer.
      // http://stackoverflow.com/questions/5189071/print-part-of-char-array
      printf("received data: %s, replying: %.*s\n", (char *) in,
          (int) len, buf + LWS_SEND_BUFFER_PRE_PADDING);
           
      // send response
      // just notice that we have to tell where exactly our response
      // starts. That's why there's buf[LWS_SEND_BUFFER_PRE_PADDING]
      // and how long it is. We know that our response has the same
      // length as request because it's the same message 
      // in reverse order.
      lws_write(wsi, &buf[LWS_SEND_BUFFER_PRE_PADDING],
          len, LWS_WRITE_TEXT);
           
      // release memory back into the wild
      free(buf);
      break;
    }
    default:
      break;
  }
   
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
    "dumb-increment-protocol", // protocol name - very important!
    callback_dumb_increment,   // callback
    0                          // we don't use any per session data
  },
  {
    "midway-1", // protocol name - very important!
    callback_dumb_increment,   // callback
    0                          // we don't use any per session data
  },
  {
    NULL, NULL, 0   /* End of list */
  }
};


int main(void) {
  // server url will be http://localhost:9000
  int port = 9000;
  const char *interface = NULL;
  struct lws_context *context;
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
   
  // infinite loop, to end this server send SIGTERM. (CTRL+C)
  while (1) {
    lws_service(context, 50);
    // libwebsocket_service will process all waiting events with
    // their callback functions and then wait 50 ms.
    // (this is a single threaded web server and this will keep our
    // server from generating load while there are not
    // requests to process)
  }
   
  lws_context_destroy(context);
   
  return 0;
}
