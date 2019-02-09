

#include <stdio.h>
#include <stdlib.h>

// extra libs
#include <libwebsockets.h>
#include <json-c/json.h>
#include <glib.h>

// MidWay libs
#include <MidWay.h>

typedef struct {
   struct lws *wsi;
   int32_t clienthandle;
   int32_t internalhandle;      
   struct json_object *jobj;
   
} PendingCall ;


#define debug(...) do { lwsl_debug( __VA_ARGS__  ); } while(0);


#define info(...) do { lwsl_notice( __VA_ARGS__  ); } while(0);

#define warn(...) do { lwsl_warn( __VA_ARGS__  );} while(0);

#define error(...) do { lwsl_err( __VA_ARGS__  );} while(0);


extern struct lws_context *context;

int callback_midway_ws(
		       struct lws *wsi,
		       enum lws_callback_reasons reason,
		       void *user,
		       void *in,
		       size_t len
		       );


int queue_message(void);

int drain_queue(void);

void * sender_thread_main(void *);

char * lbl_lws_callback_reasons(int);

int addPendingCall(PendingCall *) ;
void clearPendingCalls(struct lws * wsi) ;
int queueMessage(struct lws *wsi,  json_object * jobj ) ;
