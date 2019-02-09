
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <fnmatch.h>
#include <sys/types.h>
#include <regex.h>

#include <pthread.h>
#include <libwebsockets.h>
#include <MidWay.h>

#include "mwwsd.h"


G_LOCK_DEFINE (subscriptions);

typedef struct  {
   struct lws * wsi;
   int32_t handle;
   char * pattern;
   int type;
   regex_t compiled_regex;
   GPatternSpec * glib_glob;
} subscription_t ; 
   
/*
 * The structure of subscriptions is that we use wsi as key
 * then a list of subscriptions as value
 */
static GHashTable * subscriptions = NULL;

void init_subscription_store() {
      
   subscriptions = g_hash_table_new (g_direct_hash, g_direct_equal);

}

static char suberror[1000];
const char * getSubscriptionError() {
   return suberror;
}

static subscription_t *  makeSubScription(struct lws * wsi, int32_t handle,
				   const char * pattern, int type) {
   subscription_t * sub = malloc(sizeof(subscription_t));
   sub->wsi = wsi;
   sub->handle = handle;
   sub->pattern = strdup(pattern);
   
   sub->type = type;
   if (sub->type == SUBTYPE_GLOB) {
   } else if (sub->type == SUBTYPE_REGEX) {
      int rc = regcomp(&sub->compiled_regex, pattern, REG_EXTENDED|REG_NOSUB);
      if (rc != 0) {
	 regerror( rc, &sub->compiled_regex, suberror, 1000);
	 return NULL;
      }
   } else {
      error("illegal subscription type %d\n", type);
      sprintf(suberror, "illegal subscription type %d");
      return NULL;
   }
	   
   return sub;
}

int addSubScription(struct lws * wsi, int32_t handle, const char * pattern, int type) {

   debug("add subscription with handle %d\n", handle);
   subscription_t * sub =  makeSubScription(wsi,  handle, pattern, type) ;
   if (sub == NULL) {
      errno = EINVAL;
      return -errno;
   };

   G_LOCK (subscriptions);

   GQueue * q = g_hash_table_lookup (subscriptions, wsi);
   if (q == NULL) {
      q = g_queue_new ();
      g_hash_table_insert (subscriptions, wsi, q);
   }
   g_queue_push_tail(q, sub);

   debug("wsi with subscriptions now %d\n", g_hash_table_size(subscriptions));
   debug("subscriptions with this wsi now %d\n", g_queue_get_length(q));
   G_UNLOCK (subscriptions);
   return 0 ;
}

static void removeSub(gpointer elm, gpointer queuep);

int delSubScription(struct lws * wsi, int32_t handle) {
   G_LOCK (subscriptions);
   int rc = 0;
   
   debug("del subscription with handle %d\n", handle);

   
   GQueue * q = g_hash_table_lookup (subscriptions, wsi);
   if (q == NULL) {
      warn("tried to remove a subscription but there is no subscriptions for wsi %p\n", wsi);
      rc = 0;
      goto out;
   }

   for (int i = 0; i <  g_queue_get_length(q); i++) {
      subscription_t * sub = g_queue_peek_nth (q, i);
      if (sub->handle == handle) {
	 gboolean b = g_queue_remove (q, sub);
	 rc =  (b == TRUE) ? 1 : 0;
	 debug("remove subscription returned %d\n", b);
      }
   }
  
 out: 
   debug("wsi with subscriptions now %d\n", g_hash_table_size(subscriptions));
   debug("subscriptions with this wsi now %d\n", g_queue_get_length(q));
   G_UNLOCK (subscriptions);
   return rc ;
}

void clearSubscriptions(struct lws * wsi) {

   guint length;
   debug("subscriptions before clearing %d\n", g_hash_table_size(subscriptions));

   GQueue * q = g_hash_table_lookup (subscriptions, wsi);
   if (q == NULL) {
      debug("tried to clean subscription but there is no subscriptions for wsi %p\n", wsi);
      return;
   }
   
   G_LOCK (subscriptions);
   subscription_t * sub;
   while (sub = g_queue_pop_head (q) ){
      debug("clearing a subscription with handle\n", sub->handle);
      free(sub->pattern);
      free(sub);
   };
   
   debug("subscriptions now %d\n", g_hash_table_size(subscriptions));
   G_UNLOCK (subscriptions);   
}

void testEvents() {

   processEvent("12345", "numbers", 0);
   processEvent("12abc", "alnum", 0);
   processEvent("abcde", "alpha", 0);
}

void processEvent(char * evname, char * evdata, size_t evdatalen) {

   G_LOCK (subscriptions);
   struct json_object * jobj = json_object_new_object();
   json_object_object_add (jobj, "command", json_object_new_string ("EVENT"));
   json_object_object_add (jobj, "event", json_object_new_string (evname));

   debug ("got event %s with data %10s...\n", evname, evdata);
   
   if (evdata != NULL) {
      if (evdatalen <= 0) evdatalen = strlen(evdata);
       json_object_object_add (jobj, "data",
			       json_object_new_string_len (evdata, evdatalen));
   }

   guint length;
   gpointer * keys = 
      g_hash_table_get_keys_as_array (subscriptions, &length);


   for (int i = 0; i < length; i++) {

      json_object * hdllist = json_object_new_array ();

      gpointer wsi = keys[i];
      debug("going thru  wsi %p\n", wsi);
      gpointer val = 
	 g_hash_table_lookup (subscriptions,  wsi);
      GQueue * q = val;

      for (int n = 0; n <  g_queue_get_length(q); n++) {
	 
	 subscription_t  * sub = g_queue_peek_nth(q, n);
	 if (sub == NULL) {
	    error("Can't happen, an element of a subscription queue for wsi %p was NULL\n", wsi);
	    continue; // can't happen but..
	 }
	 if (sub->type == SUBTYPE_GLOB) {
	    if (fnmatch(sub->pattern, evname, 0) == 0){
	       debug ("glob match %s handle %d\n", sub->pattern, sub->handle);
	       json_object_array_add (hdllist,  json_object_new_int (sub->handle));
	    };
	 } else if (sub->type == SUBTYPE_REGEX) {
	    int rc = regexec(&sub->compiled_regex, evname, 0, NULL, 0);
	    if (rc == 0) {
	       debug("regex match %s handle %d\n", sub->pattern);
	       json_object_array_add (hdllist,  json_object_new_int (sub->handle));
	    };
	 }
      }
      debug ("found %d matches for wsi\n", json_object_array_length(hdllist));
      if (json_object_array_length(hdllist) > 0) {


	 json_object_object_add (jobj, "handle", hdllist);
	 queueMessage(wsi, jobj);
	 json_object_object_del(jobj, "handle");
	 //	 json_object_put (hdllist);
      }
   }
   debug("done processing event\n");
   G_UNLOCK (subscriptions);
};
