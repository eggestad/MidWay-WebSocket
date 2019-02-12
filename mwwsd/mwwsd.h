

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
   size_t bindata;
} PendingCall ;

#define SUBTYPE_GLOB 10
#define SUBTYPE_REGEX 11



#define debug(...) do { lwsl_debug( __VA_ARGS__  ); } while(0);

#define info(...) do { lwsl_notice( __VA_ARGS__  ); } while(0);

#define warn(...) do { lwsl_warn( __VA_ARGS__  );} while(0);

#define error(...) do { lwsl_err( __VA_ARGS__  );} while(0);

/*
#define Debug(...) do {  mwlog(MWLOG_DEBUG, __VA_ARGS__  ); } while(0);

#define Debug2(...) do {  mwlog(MWLOG_DEBUG2, __VA_ARGS__  ); } while(0);

#define Info(...) do { mwlog(MWLOG_INFO,  __VA_ARGS__  ); } while(0);

#define Warn(...) do { mwlog(MWLOG_WARNING, __VA_ARGS__  );} while(0);

#define Error(...) do { mwlog(MWLOG_ERROR, __VA_ARGS__  );} while(0);
*/
extern struct lws_context *context;

int callback_midway_ws(
		       struct lws *wsi,
		       enum lws_callback_reasons reason,
		       void *user,
		       void *in,
		       size_t len
		       );

void logwrapper(int level, const char *line) ;

// sender.c
void * sender_thread_main(void *);

// protocol.c
char * lbl_lws_callback_reasons(int);
int queueMessage(struct lws *wsi,  json_object * jobj, void * data, size_t len);

// pendingcalls.c
void init_pendingcall_store();
void pendingcalls_lock();
void pendingcalls_unlock();

int addPendingCall(PendingCall *) ;
void clearPendingCalls(struct lws * wsi) ;
void deliver_svcreply(int32_t handle, char * data, size_t datalen, int appreturncode, int returncode);

// subscriptions.c

int addSubScription(struct lws *, int32_t, const char *, int) ;
int delSubScription(struct lws *, int32_t) ;
void clearSubscriptions(struct lws *) ;
void processEvent(char *, char *, size_t) ;
const char * getSubscriptionError() ;
void init_subscription_store(void) ;


// undocumented internal libMidWay functions we'll need.
