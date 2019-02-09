
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
#include "mwwsd.h"


// TEST 
struct lws *wsi_sub = NULL;
int sub_handle = 0;
int sendev = 0;

GHashTable * sendqueues = NULL;

int queueMessage(struct lws *wsi,  json_object * jobj ) {
   if (sendqueues == NULL)
      sendqueues = g_hash_table_new (g_direct_hash, g_direct_equal);
   
   GQueue * q = g_hash_table_lookup (sendqueues, wsi);
   if (q == NULL) {
      q = g_queue_new ();
      g_hash_table_insert (sendqueues, wsi, q);
   }
   const char * text = json_object_to_json_string(jobj);
   char * msg = strdup(text);
   g_queue_push_head(q, msg);
   lws_callback_on_writable (wsi);
   debug("enqueued on wsi %p fd %d pending %d msg %s\n",
	 wsi, lws_get_socket_fd(wsi), g_queue_get_length(q), msg);
}
   
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

static int setError(struct json_object *jobj, const char * reason) {
   json_object_object_add (jobj, "RC", json_object_new_string ("FAIL"));
   json_object_object_add (jobj, "reason", json_object_new_string (reason));
};

static int doWritable(struct lws *wsi) {
   GQueue * q = g_hash_table_lookup (sendqueues, wsi);
   if (q == NULL) {
      warn("Got spurious WRITEABLE");
      return 0;
   }
   gpointer msg = g_queue_pop_tail(q);
   if (msg == NULL) {
      warn("Got spurious WRITEABLE");
      return 0;
   }
   
   int rc = _send_response(wsi, msg);
   free(msg);
   guint restlen = g_queue_get_length (q);
   if (restlen  > 0) 
      lws_callback_on_writable (wsi);
   return rc;
}

static int doError(struct lws *wsi, struct json_object *jobj, const char * reason) {
   struct json_object *ret_jobj ;
   struct json_object *o;

   //ret_jobj = json_object_new_object();

   json_object_object_add (jobj, "RC", json_object_new_string ("error"));
   json_object_object_add (jobj, "error", json_object_new_string (reason));

   queueMessage(wsi, jobj);   
};


static int doAttach(struct lws *wsi, struct json_object *jobj ) {

   struct json_object *field;
   int rc;


   if (json_object_object_get_ex(jobj, "domain", &field)) {
      
      if (! json_object_is_type(field, json_type_string)) {
	 setError(jobj, "domain is not a string");
	 queueMessage(wsi, jobj);
	 return -1;
      }
      const char * domain = json_object_get_string (field);
      debug("attaching domain  %s\n",  domain);
   };
   
   json_object_object_add (jobj, "RC", json_object_new_string ("OK"));
   queueMessage(wsi, jobj);

};

static int doClose(struct lws *wsi) {

   // remove all pending messages waiting to be sent
   GQueue * q = g_hash_table_lookup (sendqueues, wsi);
   if (q != NULL) {
      debug("removed send queue\n");
      g_queue_free_full (q, free);
      g_hash_table_remove (sendqueues, wsi);
   }

   // TODO: clear all subscriptions

   // TODO: clear all pending calls
   return 0;
}

int  getNextHandle() {
   static int hdl = 0xbefa;
   if (hdl > 0x6fffffff) hdl = 0xbefa;
   return hdl++;
}

static int doSubscribe(struct lws *wsi, struct json_object *jobj, int unsubscribe ) {

   struct json_object *field;
   int rc;
   const char * pattern = NULL;
   const char * type = "glob";

   if (json_object_object_get_ex(jobj, "pattern", &field)) {
      
      if (! json_object_is_type(field, json_type_string)) {
	 setError(jobj, "pattern is not a string");
	 queueMessage(wsi, jobj);	 
	 return -1;
      }
      const char * pattern = json_object_get_string (field);
      debug("subscribing to pattern %s\n",  pattern);
   } else {
      setError(jobj, "pattern is missing");
      queueMessage(wsi, jobj);	 
      return -1;
   }
   
   if (json_object_object_get_ex(jobj, "type", &field)) {
      
      if (! json_object_is_type(field, json_type_string)) {
	 setError(jobj, "type is not a string");
	 return -1;
      }
      type = json_object_get_string (field);
   };

   char * replycmd = "SUBSCRIBERPL";
   if (unsubscribe) {
      replycmd  = "SUBSCRIBERPL";
   } else {
      
   }
   
   json_object_object_add (jobj, "RC", json_object_new_string ("OK"));
   json_object_object_del(jobj, "command");
   json_object_object_add (jobj, "command", json_object_new_string (replycmd));
   

   wsi_sub = wsi;
   
   queueMessage(wsi, jobj);
   return 0;
};


static int doCallReq(struct lws *wsi, struct json_object *jobj ) {

   struct json_object *field;
   int rc;

   const char * service  = NULL;;
   const char * data = NULL;
   long handle = 0;
   
   if (json_object_object_get_ex(jobj, "service", &field)) {
      
      if (! json_object_is_type(field, json_type_string)) {
	 setError(jobj, "service is not a string");
	 queueMessage(wsi, jobj);	 
	 return -1;
      }
      service = json_object_get_string (field);
      debug("about to call service  %s\n",  service);
      
   } else {
      setError(jobj, "servicename missing");
      queueMessage(wsi, jobj);	 
      return -1;
   }

   if (json_object_object_get_ex(jobj, "data", &field)) {
      
      if (! json_object_is_type(field, json_type_string)) {
	 setError(jobj, "data is not a string");
	 queueMessage(wsi, jobj);	 
	 return -1;
      }
      data = json_object_get_string (field);
   }

   if (json_object_object_get_ex(jobj, "handle", &field)) {
      
      if ( json_object_is_type(field, json_type_int)) {
	 handle  = json_object_get_int (field);
      } else if ( json_object_is_type(field, json_type_string)) {
	 const char * s  = json_object_get_string (field);
	 handle = atoi(s);
      } else {
	 setError(jobj, "handle is not a string or long");
	 queueMessage(wsi, jobj);	 
	 return -1;
      }      
   }
   if (data == NULL) data = "";
   debug("call request valid\n");
   
   PendingCall * pc = malloc(sizeof(PendingCall));
   pc->wsi = wsi;
   pc->clienthandle = handle;
   pc->internalhandle = getNextHandle();
   pc->jobj = jobj;
   debug("jobj %p\n", jobj);
   debug("pending call %p\n", pc);
   json_object_get(jobj);
   addPendingCall(pc);
   return 0;
};

struct json_object * makeCallReply(json_object * jobj, int32_t clienthandle, char * data, size_t datalen, int rc, int apprc) {
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
   
   debug ("   %s = %s\n", lws_token_to_string( token), buf);
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



int queue_message() {
   sendev = 1;
}

int drain_queue() {

   if (!sendev) return 0;
   struct json_object * jobj = json_object_new_object();
   
   json_object_object_add (jobj, "command", json_object_new_string ("EVENT"));
   json_object_object_add (jobj, "name", json_object_new_string ("testev"));
   json_object_object_add (jobj, "data", json_object_new_string ("testdata"));
   json_object_object_add (jobj, "handle", json_object_new_int (sub_handle));

   const char * json_text = json_object_to_json_string(jobj);
	 
   if (wsi_sub != NULL) {
      _send_response(wsi_sub,json_text);
   }
   sendev = 0;
}



int callback_midway_ws(
		       struct lws *wsi,
		       enum lws_callback_reasons reason,
		       void *user,
		       void *in,
		       size_t len
		       ) {

   enum json_tokener_error jerr;
   debug("midway connection event %d (%s) len of in=%d user = %p\n",
	 reason, lbl_lws_callback_reasons(reason),  len, user);

   switch (reason) {
      // just log message that someone is connecting

   case LWS_CALLBACK_ESTABLISHED: 
      debug("connection established %s\n", in);
      inspect_headers_debug(wsi);
      break;

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
	 info("Got connection from %s(%s) for domain %s clientname %s user %s cred %s\n", cname, rip, domain, rc_clientname, rc_username, rc_credentials);
	 
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
	    doSubscribe(wsi, jobj, FALSE);

	 else if (strcmp("UNSUBSCRIBEREQ", command) == 0)
	    doSubscribe(wsi, jobj, TRUE);

	 else
	    doError(wsi, jobj, "Unknown command");
      }

      // free json object
      json_object_put(jobj);
      break;
   
   case LWS_CALLBACK_CLIENT_WRITEABLE:
   case LWS_CALLBACK_SERVER_WRITEABLE:
      info("got writeable event\n");
      doWritable(wsi);
      break;
      
   case LWS_CALLBACK_CLOSED:
      {
	 char cname[256];
	 char rip[32];
	 lws_get_peer_addresses	(wsi, lws_get_socket_fd (wsi),
				 cname, 255, rip, 31);
	 //info("Got connection from %s(%s) for domain %s clientname %s user %s cred %s\n", cname, rip, domain, rc_clientname, rc_username, rc_credentials);
	 info("Closing connection from %s(%s) \n", cname, rip);
	 doClose(wsi);
      }
      break;
      
   default:
      info("got unepected event\n");
      break;
   }
   
   return 0;
}
