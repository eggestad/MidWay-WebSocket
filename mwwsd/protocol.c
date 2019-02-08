
// Std C lib
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// extra libs
#include <libwebsockets.h>
#include <json-c/json.h>

// MIdWay libs
#include <MidWay.h>
#include "mwwsd.h"


// TEST 
struct lws *wsi_sub = NULL;
int sub_handle = 0;
int sendev = 0;


static int _send_response(struct lws *wsi, const char * text) {


   
   debug("sending response = %s", text);
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



static int doError(struct lws *wsi, struct json_object *jobj, const char * reason) {
   struct json_object *ret_jobj ;
   struct json_object *o;

   //ret_jobj = json_object_new_object();

   json_object_object_add (jobj, "RC", json_object_new_string ("error"));
   json_object_object_add (jobj, "error", json_object_new_string (reason));
   const char * json_text = json_object_to_json_string(jobj);
   _send_response(wsi, json_text);
   
};


static int doAttach(struct lws *wsi, struct json_object *jobj ) {

   struct json_object *field;
   int rc;


   if (json_object_object_get_ex(jobj, "domain", &field)) {
      
      if (! json_object_is_type(field, json_type_string)) {
	 doError(wsi, jobj, "domain is not a string");
	 return -1;
      }
      const char * domain = json_object_get_string (field);
      debug("attaching domain  %s",  domain);
   };
   
   json_object_object_add (jobj, "RC", json_object_new_string ("OK"));
    const char * json_text = json_object_to_json_string(jobj);
   _send_response(wsi, json_text);

};

static int doSubscribe(struct lws *wsi, struct json_object *jobj ) {

   struct json_object *field;
   int rc;


   if (json_object_object_get_ex(jobj, "glob", &field)) {
      
      if (! json_object_is_type(field, json_type_string)) {
	 doError(wsi, jobj, "globis not a string");
	 return -1;
      }
      const char * glob = json_object_get_string (field);
      debug("subscribing to glob %s",  glob);
   };

   if (json_object_object_get_ex(jobj, "regex", &field)) {
      
      if (! json_object_is_type(field, json_type_string)) {
	 doError(wsi, jobj, "regexp is not a string");
	 return -1;
      }
      const char * regexp = json_object_get_string (field);
      debug("subscribing to regexp %s",  regexp);
   };

   if (json_object_object_get_ex(jobj, "handle", &field)) {
      
      if (! json_object_is_type(field, json_type_int)) {
	 doError(wsi, jobj, "handle is not an int");
	 return -1;
      }
      int  handle = json_object_get_int (field);
      debug("handle %d",  handle);
      sub_handle = handle;
   };
   
   json_object_object_add (jobj, "RC", json_object_new_string ("OK"));
   json_object_object_del(jobj, "command");
   json_object_object_del(jobj, "data");

   json_object_object_add (jobj, "command", json_object_new_string ("SUBSCRIBERPL"));

   wsi_sub = wsi;
   
   const char * json_text = json_object_to_json_string(jobj);
   _send_response(wsi, json_text);

};

static int doUnSubscribe(struct lws *wsi, struct json_object *jobj ) {

};

static int doCallReq(struct lws *wsi, struct json_object *jobj ) {

   struct json_object *field;
   int rc;

   const char * service  = NULL;;
   const char * data = NULL;
   long handle = 0;
   
   if (json_object_object_get_ex(jobj, "service", &field)) {
      
      if (! json_object_is_type(field, json_type_string)) {
	 doError(wsi, jobj, "service is not a string");
	 return -1;
      }
      service = json_object_get_string (field);
      debug("about to call service  %s",  service);
      
   } else {
      doError(wsi, jobj, "servicename missing");
      return -1;
   }

   if (json_object_object_get_ex(jobj, "data", &field)) {
      
      if (! json_object_is_type(field, json_type_string)) {
	 doError(wsi, jobj, "data is not a string");
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
	 doError(wsi, jobj, "handle is not a string or long");
	 return -1;
      }      
   }
   if (data == NULL) data = "";
   
   char buf[strlen(data) + 100]; 
   sprintf(buf, "reply to data: %s from %s ", data, service);

   json_object_object_del(jobj, "command");
   json_object_object_del(jobj, "data");

   json_object_object_add (jobj, "command", json_object_new_string ("CALLRPL"));
   json_object_object_add (jobj, "data", json_object_new_string (buf));

   json_object_object_add (jobj, "RC", json_object_new_string ("OK"));
   json_object_object_add (jobj, "apprc", json_object_new_int (42));
   
   const char * json_text = json_object_to_json_string(jobj);
   _send_response(wsi, json_text);

};

static inline void print_header(struct lws *wsi, int token) {

   char buf[8000] = {0};
   int len = lws_hdr_copy(wsi, buf, 7000, token);
   
   debug ("   %s = %s", lws_token_to_string( token), buf);
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
   debug("midway connection event %d  %d user = 0x%lx", reason,  len, user);

   switch (reason) {
      // just log message that someone is connecting

   case LWS_CALLBACK_ESTABLISHED: 
      debug("connection established %s", in);
      inspect_headers_debug(wsi);
      break;

   case LWS_CALLBACK_FILTER_PROTOCOL_CONNECTION:
      {
	 debug("protocol connection established %s", in);
	 inspect_headers_debug(wsi);
	 char domain[100] = {0};
	 char clientname[100] = {0};
	 char username[100] = {0};
	 char credentials[100] = {0};
	 const char * rc_clientname = lws_get_urlarg_by_name(wsi, "clientname", clientname, 100);
	 const char * rc_username = lws_get_urlarg_by_name(wsi, "username", username, 100);
	 const char * rc_credentials = lws_get_urlarg_by_name(wsi, "credentials", credentials, 100);

	 int len = lws_hdr_copy(wsi, domain, 100,  WSI_TOKEN_GET_URI);

	 debug(" domain %d %s", len , domain);
	 debug(" client %s %s", rc_clientname ,clientname );
	 debug(" user   %s %s", rc_username ,username );
	 debug(" creden %s %s", rc_credentials ,credentials );
      }
      break;
      
   case LWS_CALLBACK_RECEIVE: { // the funny part
 
      // Log what we received and what we're going to send as a 
      // response that disco syntax `%.*s` is used to print just a
      // part of our buffer.
      // http://stackoverflow.com/questions/5189071/print-part-of-char-array
      debug("received data: %s", (char *) in, (int) len);


      
      struct json_object *jobj  = json_tokener_parse_verbose	 (in, &jerr);

      if (jerr != 0)
	 error(" invalid json %s", json_tokener_error_desc(jerr));

      struct json_object *field;
      json_object_object_get_ex(jobj, "command", &field);

      if (! json_object_is_type(field, json_type_string)) {
	 error( "Got message with no command");
      } else {
	 const char * command = json_object_get_string (field);
	 debug("got command %s",  command);


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
    }
    default:
      break;
  }
   
  return 0;
}
