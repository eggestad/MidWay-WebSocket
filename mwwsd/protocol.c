
// Std C lib
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// extra libs
#include <libwebsockets.h>
#include <json-c/json.h>
#include <glib.h>

// MidWay libs
#include <MidWay.h>
#include <ipcmessages.h>
#include "mwwsd.h"


// TEST 
struct lws *wsi_sub = NULL;
int sub_handle = 0;
int sendev = 0;
G_LOCK_DEFINE (sendqueues );

GHashTable * sendqueues = NULL;

#define TEXTMSG 0xf
#define BLOBMSG 0xf0
typedef union {
   struct   {
      int type;
      char * text;
   } text;
   struct   {
      int type;
      char * blob;
      size_t len;
   } blob ;
} mesgQueueElem_t;

      

static int _send_response(struct lws *wsi, const char * text) {


   
   debug("sending response = %s\n", text);
   int len = strlen(text);

   // Create a buffer to hold our response
   // it has to have some pre and post padding.
   // You don't need to care what comes there, libwebsockets
   // will do everything for you. For more info see
   // http://git.warmcat.com/cgi-bin/cgit/libwebsockets/tree/lib/libwebsockets.h#n597
   unsigned char *buf = (unsigned char*)
      malloc(LWS_SEND_BUFFER_PRE_PADDING + len
	     + LWS_SEND_BUFFER_POST_PADDING);

   strncpy(&buf[LWS_SEND_BUFFER_PRE_PADDING], text, len);
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
}


static int _send_blob_response(struct lws *wsi, const char * blob, size_t len) {
   debug("sending blob response len %d\n", len);

   // Create a buffer to hold our response
   // it has to have some pre and post padding.
   // You don't need to care what comes there, libwebsockets
   // will do everything for you. For more info see
   // http://git.warmcat.com/cgi-bin/cgit/libwebsockets/tree/lib/libwebsockets.h#n597
   unsigned char *buf = (unsigned char*)
      malloc(LWS_SEND_BUFFER_PRE_PADDING + len
	     + LWS_SEND_BUFFER_POST_PADDING);

   memcpy(&buf[LWS_SEND_BUFFER_PRE_PADDING], blob, len);
   // send response
   // just notice that we have to tell where exactly our response
   // starts. That's why there's buf[LWS_SEND_BUFFER_PRE_PADDING]
   // and how long it is. We know that our response has the same
   // length as request because it's the same message 
   // in reverse order.
   lws_write(wsi, &buf[LWS_SEND_BUFFER_PRE_PADDING],
	     len, LWS_WRITE_BINARY);
           
   // release memory back into the wild
   free(buf);
}

/**
 * add to an internal send queue per wsi the message 
 * described by jobj. 
 * this function serialized jobj before queueing, jobj may be destroyed
 */
int queueMessage(struct lws *wsi,  json_object * jobj, void * data, size_t len ) {
   G_LOCK (sendqueues);
   if (sendqueues == NULL)
      sendqueues = g_hash_table_new (g_direct_hash, g_direct_equal);

   json_object * jobj_len;
   json_bool havebindata = json_object_object_get_ex(jobj, "bindata", &jobj_len);
   if (data != NULL && !havebindata) {
      jobj_len = json_object_new_int64(len) ;  
      json_object_object_add (jobj, "bindata", jobj_len);
   }
   
   GQueue * q = g_hash_table_lookup (sendqueues, wsi);
   if (q == NULL) {
      q = g_queue_new ();
      g_hash_table_insert (sendqueues, wsi, q);
   }
   mesgQueueElem_t * textmsg = malloc (sizeof(mesgQueueElem_t));
   textmsg->text.type = TEXTMSG;
   textmsg->text.text = strdup(json_object_to_json_string(jobj));
   g_queue_push_head(q, textmsg);
   lws_callback_on_writable (wsi);

   debug("enqueued %p %d", textmsg, textmsg->text.type);
   
   debug("enqueued text on wsi fd:%d pending %d msg %s\n",
	 lws_get_socket_fd(wsi), g_queue_get_length(q), textmsg->text.text);

   if (data != NULL) {
      mesgQueueElem_t * blobmsg = malloc (sizeof(mesgQueueElem_t));
      blobmsg->blob.type = BLOBMSG;
      blobmsg->blob.blob = malloc(len);
      memcpy(blobmsg->blob.blob, data, len);
      blobmsg->blob.len = len;
      g_queue_push_head(q, blobmsg);
      debug("enqueued blob on wsi fd:%d pending %d len %d\n",
	    lws_get_socket_fd(wsi), g_queue_get_length(q), blobmsg->blob.len);
	 
   }
	 
   G_UNLOCK (sendqueues);
   return 1;
}

/**
 * called when we get a writeable event on a wsi 
 * dequeue a message from te sendqueue and send it on the wsi
 */
static int doWritable(struct lws *wsi) {
   G_LOCK (sendqueues);
   GQueue * q = g_hash_table_lookup (sendqueues, wsi);
   if (q == NULL) {
      warn("Got spurious WRITEABLE");
      return 0;
   }
   gpointer gp = g_queue_pop_tail(q);
   mesgQueueElem_t * msg = gp;
   if (msg == NULL) {
      warn("Got spurious WRITEABLE");
      return 0;
   }
   debug("dequeued %p", msg);
   int rc = 0;
   if (msg->text.type == TEXTMSG) {
      rc = _send_response(wsi, msg->text.text);
      free(msg->text.text);
      free(msg);
   } else if (msg->blob.type == BLOBMSG) {
      rc += _send_blob_response(wsi, msg->blob.blob, msg->blob.len);
      free(msg->blob.blob);
      free(msg);
   } else {
      Error("dequeued unknown message from send queue, %p %d",
	    msg, msg->text.type);
      // Can't happen
   }
   guint restlen = g_queue_get_length (q);
   if (restlen  > 0) 
      lws_callback_on_writable (wsi);
   G_UNLOCK (sendqueues);
   return rc;
}

/**
 * set error  indicator on a message described by jobj
 */
static int setError(struct json_object *jobj, const char * reason) {
   json_object_object_add (jobj, "RC", json_object_new_string ("FAIL"));
   json_object_object_add (jobj, "reason", json_object_new_string (reason));
};

static int doError(struct lws *wsi, struct json_object *jobj, const char * reason) {
   struct json_object *ret_jobj ;
   struct json_object *o;

   //ret_jobj = json_object_new_object();

   json_object_object_add (jobj, "RC", json_object_new_string ("error"));
   json_object_object_add (jobj, "error", json_object_new_string (reason));

   queueMessage(wsi, jobj, NULL, 0);   
};

/**
 * on an Attach message from client
 */
static int doAttach(struct lws *wsi, struct json_object *jobj ) {

   struct json_object *field;
   int rc;


   if (json_object_object_get_ex(jobj, "domain", &field)) {
      
      if (! json_object_is_type(field, json_type_string)) {
	 setError(jobj, "domain is not a string");
	 queueMessage(wsi, jobj, NULL, 0);
	 return -1;
      }
      const char * domain = json_object_get_string (field);
      debug("attaching domain  %s\n",  domain);
   };
   
   json_object_object_add (jobj, "RC", json_object_new_string ("OK"));
   queueMessage(wsi, jobj, NULL, 0);
   return 0;
};

/**
 * called when a client closes the connection, or is otherwise broken
 */
static int doClose(struct lws *wsi) {

   // remove all pending messages waiting to be sent
   G_LOCK (sendqueues);
   GQueue * q = g_hash_table_lookup (sendqueues, wsi);
   if (q != NULL) {
      debug("removed send queue\n");
      g_queue_free_full (q, free);
      g_hash_table_remove (sendqueues, wsi);
   }
   G_UNLOCK (sendqueues);

   // TODO: clear all subscriptions
   clearSubscriptions(wsi);
   
   // TODO: clear all pending calls
   clearPendingCalls(wsi);
   return 0;
}

/**
 * used to get a handle for IPC MidWay calls
 */ 
int  getNextHandle() {
   static int hdl = 0xbefa;
   if (hdl > 0x6fffffff) hdl = 0xbefa;
   return hdl++;
}

/**
 * called on either Subscribe or UnSubscribe messages from client
 */
static int doSubscribe(struct lws *wsi, struct json_object *jobj ) {

   struct json_object *field;
   int rc;
   const char * pattern = NULL;
   const char * type = "glob";
   int subtype = SUBTYPE_GLOB;
   int handle = 0;
   
   char * replycmd = "SUBSCRIBERPL";
   

   json_object_object_del(jobj, "command");
   json_object_object_add (jobj, "command", json_object_new_string (replycmd));
   
   if (json_object_object_get_ex(jobj, "pattern", &field)) {
      
      if (! json_object_is_type(field, json_type_string)) {
	 setError(jobj, "pattern is not a string");
	 queueMessage(wsi, jobj, NULL, 0);	 
	 return -1;
      }
      pattern = json_object_get_string (field);
      debug("subscribing to pattern %s\n",  pattern);
   } else {
      setError(jobj, "pattern is missing");
      queueMessage(wsi, jobj, NULL, 0);	 
      return -1;
   }
   
   if (json_object_object_get_ex(jobj, "handle", &field)) {
      
      if ( json_object_is_type(field, json_type_int)) {
	 handle  = json_object_get_int (field);
	 debug("int handle = %d\n", handle);
      } else if ( json_object_is_type(field, json_type_string)) {
	 const char * s  = json_object_get_string (field);
	 debug("str handle = %s\n", s);
	 handle = atoi(s);
	 debug("int handle = %d\n", handle);
      } else {
	 setError(jobj, "handle is not a string or long");
	 queueMessage(wsi, jobj, NULL, 0);	 
	 return -1;
      }      
   }

   if (json_object_object_get_ex(jobj, "type", &field)) {
      
      if (! json_object_is_type(field, json_type_string)) {
	 setError(jobj, "type is not a string");
	 return -1;
      }
      type = json_object_get_string (field);
      if (strcmp(type, "regex") )
	 subtype = SUBTYPE_REGEX;
   };
   rc = addSubScription(wsi, handle, pattern,  subtype);
   if (rc == 0) 
      json_object_object_add (jobj, "RC", json_object_new_string ("OK"));
   else {
      setError(jobj, getSubscriptionError());
   }

   queueMessage(wsi, jobj, NULL, 0);
   return 0;
};
/**
 * called on either Subscribe or UnSubscribe messages from client
 */
static int doUnSubscribe(struct lws *wsi, struct json_object *jobj ) {

   struct json_object *field;
   char * replycmd ="UNSUBSCRIBERPL";  
   int handle = 0;

   json_object_object_del(jobj, "command");
   json_object_object_add (jobj, "command", json_object_new_string (replycmd));
   if (json_object_object_get_ex(jobj, "handle", &field)) {
      
      if ( json_object_is_type(field, json_type_int)) {
	 handle  = json_object_get_int (field);
	 debug("int handle = %d\n", handle);
      } else if ( json_object_is_type(field, json_type_string)) {
	 const char * s  = json_object_get_string (field);
	 debug("str handle = %s\n", s);
	 handle = atoi(s);
	 debug("int handle = %d\n", handle);
      } else {
	 setError(jobj, "handle is not a string or long");
	 queueMessage(wsi, jobj, NULL, 0);	 
	 return -1;
      }      
   }
   delSubScription(wsi, handle);
   
   json_object_object_add (jobj, "RC", json_object_new_string ("OK"));
   return 0;
}
 
/**
 * called on a call request from client 
 **/
static int doCallReq(struct lws *wsi, struct json_object *jobj ) {

   struct json_object *field;
   int rc;

   const char * service  = NULL;;
   const char * data = NULL;
   long handle = 0;
   size_t bindata = 0;

   
   if (json_object_object_get_ex(jobj, "service", &field)) {
      
      if (! json_object_is_type(field, json_type_string)) {
	 setError(jobj, "service is not a string");
	 queueMessage(wsi, jobj, NULL, 0);	 
	 return -1;
      }
      service = json_object_get_string (field);
      debug("about to call service  %s\n",  service);
      
   } else {
      setError(jobj, "servicename missing");
      queueMessage(wsi, jobj, NULL, 0);	 
      return -1;
   }

   if (json_object_object_get_ex(jobj, "data", &field)) {
      
      if (! json_object_is_type(field, json_type_string)) {
	 setError(jobj, "data is not a string");
	 queueMessage(wsi, jobj, NULL, 0);	 
	 return -1;
      }
      data = json_object_get_string (field);
   }
   
   if (json_object_object_get_ex(jobj, "bindata", &field)) {
      
      if ( json_object_is_type(field, json_type_int)) {
	 bindata  = json_object_get_int (field);
      } else if ( json_object_is_type(field, json_type_string)) {
	 const char * s  = json_object_get_string (field);
	 bindata = atol(s);
      } else {
	 setError(jobj, "bindata is not a string or long");
	 queueMessage(wsi, jobj, NULL, 0);	 
	 return -1;
      }      
   }

   if (json_object_object_get_ex(jobj, "handle", &field)) {
      
      if ( json_object_is_type(field, json_type_int)) {
	 handle  = json_object_get_int (field);
      } else if ( json_object_is_type(field, json_type_string)) {
	 const char * s  = json_object_get_string (field);
	 handle = atoi(s);
      } else {
	 setError(jobj, "handle is not a string or long");
	 queueMessage(wsi, jobj, NULL, 0);	 
	 return -1;
      }      
   }
   if (data == NULL) data = "";
   debug("call request valid\n");

   int flags = 0;
   if (handle == 0) {
      flags |= MWNOREPLY;
      
      _mwacallipc (service,  data, strlen(data), flags, 
		   UNASSIGNED, NULL, NULL, UNASSIGNED, 1);
      return  0;
   }

   /* we have to place the mwacallipc unside the lock at we don't
      have the handle yet. It happens that the kernel
      do a context switch on the underlying msgsnd(), in which case
      we're likely to get the reply on the other thread before we
      continue here*/
   pendingcalls_lock();

   int hdl =  _mwacallipc (service,  data, strlen(data), flags, 
			   UNASSIGNED, NULL, NULL, UNASSIGNED, 1);
   PendingCall * pc = malloc(sizeof(PendingCall));
   debug("pc @ %p\n", pc);
   pc->wsi = wsi;
   pc->clienthandle = handle;
   pc->internalhandle = hdl;
   pc->jobj = jobj;
   pc->bindata = bindata;
   
   json_object_get(jobj);
   addPendingCall(pc);
   pendingcalls_unlock();

   return 0;
};

struct json_object * xxmakeCallReply(json_object * jobj, int32_t clienthandle, char * data, size_t datalen, int rc, int apprc) {
   if (jobj == NULL)
      jobj = json_object_new_object ();

   json_object_object_del(jobj, "command");
   json_object_object_del(jobj, "data");
   json_object_object_add (jobj, "handle", json_object_new_int (clienthandle));

   json_object_object_add (jobj, "command", json_object_new_string ("CALLRPL"));
   if (data != NULL) {
      if (datalen <= 0)
	 datalen = strlen(data);
      json_object_object_add (jobj, "data", json_object_new_string_len (data, datalen));
   }
   json_object * rcObj;
   if (rc == MWSUCCESS)
      rcObj = json_object_new_string ("OK");
   else if (rc == MWMORE)
      rcObj = json_object_new_string ("MORE");
   else if (rc == MWFAIL)
      rcObj = json_object_new_string ("FAIL");
   else {
      rcObj = json_object_new_string ("MORE");
      warn("we got a call reply with illegal RC %d\n", rc);
   }
   json_object_object_add (jobj, "RC", rcObj);
   json_object_object_add (jobj, "apprc", json_object_new_int (apprc));

   return jobj;
}

static inline void print_header(struct lws *wsi, int token) {

   char buf[8000] = {0};
   int len = lws_hdr_copy(wsi, buf, 7000, token);
   
   DEBUG2 ("   %s = %s", lws_token_to_string( token), buf);
   //printf ("   %d of %d\n", strlen(buf), len);
}
static inline char **  get_query_params_header(struct lws *wsi, int token) {
   char buf[8000] = {0};
   int fragment = 0;
   while (lws_hdr_copy_fragment(wsi, buf, 8000, token, fragment ++) > 0) {
   }
}

static inline void inspect_headers_debug(struct lws *wsi) {
   print_header(wsi, WSI_TOKEN_GET_URI);
   print_header(wsi, WSI_TOKEN_POST_URI);
   print_header(wsi, WSI_TOKEN_OPTIONS_URI);
   print_header(wsi, WSI_TOKEN_UPGRADE);
   print_header(wsi, WSI_TOKEN_CONNECTION);
   print_header(wsi, WSI_TOKEN_VERSION);
   print_header(wsi, WSI_TOKEN_HTTP_AUTHORIZATION);
   print_header(wsi, WSI_TOKEN_HTTP_COOKIE);
   print_header(wsi, WSI_TOKEN_HTTP_USER_AGENT);
   print_header(wsi, WSI_TOKEN_HTTP_CONTENT_ENCODING);
   print_header(wsi, WSI_TOKEN_HTTP_URI_ARGS);
};

/**
 * the heavy lifter, called on every event on the WebSocket
 */
int callback_midway_ws(
		       struct lws *wsi,
		       enum lws_callback_reasons reason,
		       void *user,
		       void *in,
		       size_t len
		       ) {

   enum json_tokener_error jerr;
   debug(">>>> midway connection event fd=%d EV:%d(%s) len of in=%d user = %p\n",
	 lws_get_socket_fd(wsi),
	 reason, lbl_lws_callback_reasons(reason),  len, user);

   switch (reason) {

      // send on startup, we might do init stuff here later
   case LWS_CALLBACK_PROTOCOL_INIT:
      
      break;

      // just log message that someone is connecting
      
   case LWS_CALLBACK_ESTABLISHED: 
      debug("connection established %s\n", in);
      inspect_headers_debug(wsi);
      break;

      // when websocket protocol go thru iinitialization
   case LWS_CALLBACK_FILTER_PROTOCOL_CONNECTION:
      {
	 debug("protocol connection established %s\n", in);
	 inspect_headers_debug(wsi);
	 char domain[100] = {0};
	 char clientname[100] = {0};
	 char username[100] = {0};
	 char credentials[100] = {0};
	 const char * rc_clientname = lws_get_urlarg_by_name(wsi, "clientname", clientname, 100);
	 const char * rc_username = lws_get_urlarg_by_name(wsi, "username", username, 100);
	 const char * rc_credentials = lws_get_urlarg_by_name(wsi, "credentials", credentials, 100);

	 int len = lws_hdr_copy(wsi, domain, 100,  WSI_TOKEN_GET_URI);

	 debug(" domain %d %s\n", len , domain);
	 debug(" client %s %s\n", rc_clientname ,clientname );
	 debug(" user   %s %s\n", rc_username ,username );
	 debug(" creden %s %s\n", rc_credentials ,credentials );

	 char cname[256];
	 char rip[32];
	 lws_get_peer_addresses	(wsi, lws_get_socket_fd (wsi),
				 cname, 255, rip, 31);
	 info("Got connection fd:%d from %s(%s) for domain %s clientname %s user %s cred %s\n", lws_get_socket_fd(wsi), cname, rip, domain, rc_clientname, rc_username, rc_credentials);
	 
      }
      break;
      
   case LWS_CALLBACK_RECEIVE:  // the funny part
 
      // Log what we received and what we're going to send as a 
      // response that disco syntax `%.*s` is used to print just a
      // part of our buffer.
      // http://stackoverflow.com/questions/5189071/print-part-of-char-array
      debug("received data: %s\n", (char *) in, (int) len);


      
      struct json_object *jobj  = json_tokener_parse_verbose	 (in, &jerr);

      if (jerr != 0)
	 error(" invalid json %s\n", json_tokener_error_desc(jerr));

      struct json_object *field;
      json_object_object_get_ex(jobj, "command", &field);

      if (! json_object_is_type(field, json_type_string)) {
	 error( "Got message with no command");
      } else {
	 const char * command = json_object_get_string (field);
	 debug("got command %s\n",  command);


	 if (strcmp("ATTACH", command) == 0)
	    doAttach(wsi, jobj);

	 else if (strcmp("CALLREQ", command) == 0)
	    doCallReq(wsi, jobj);

	 else if (strcmp("SUBSCRIBEREQ", command) == 0)
	    doSubscribe(wsi, jobj);

	 else if (strcmp("UNSUBSCRIBEREQ", command) == 0)
	    doUnSubscribe(wsi, jobj);

	 else
	    doError(wsi, jobj, "Unknown command");
      }

      // free json object
      json_object_put(jobj);
      break;
   
   case LWS_CALLBACK_CLIENT_WRITEABLE:
   case LWS_CALLBACK_SERVER_WRITEABLE:
      debug("got writeable event\n");
      doWritable(wsi);
      break;

   case LWS_CALLBACK_WS_PEER_INITIATED_CLOSE :
      break;
   case LWS_CALLBACK_CLOSED:
      {
	 info("Closing connection of fd %d \n", lws_get_socket_fd(wsi));
	 doClose(wsi);
      }
      break;
      
   default:
      info("got unexpected event\n");
      break;
   }
   debug("==== midway connection event done\n");
   return 0;
}
