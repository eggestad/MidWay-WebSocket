
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>

#include <pthread.h>
#include <libwebsockets.h>
#include <MidWay.h>

#include "mwwsd.h"
GHashTable * pendingcalls = NULL;

void init_data_store() {
      
   pendingcalls = g_hash_table_new (g_direct_hash, g_direct_equal);

}

int addPendingCall(PendingCall * pc) {
   long hdl = pc->internalhandle;
   debug("pending call with handle %ld\n", hdl);
   gpointer inthandle = (gpointer) hdl;
   gboolean rc = g_hash_table_insert(pendingcalls, inthandle, pc);
   debug("pending calls now %d\n", g_hash_table_size(pendingcalls));
   return rc ;
}
static void testReplies() {
   guint length;
   debug("pending calls before  %d\n", g_hash_table_size(pendingcalls));

   gpointer * keys = 
      g_hash_table_get_keys_as_array (pendingcalls, &length);
   debug("pending calls %d\n", length);
   for (int i = 0; i < length; i++) {

      gpointer key = keys[i];
      gpointer val = 
	 g_hash_table_lookup (pendingcalls,  key);
      PendingCall * pc = val;

      struct json_object * jobj = pc->jobj;
      
      json_object_object_del(jobj, "command");
      json_object_object_add (jobj, "command", json_object_new_string ("CALLRPL"));
      json_object_object_del(jobj, "data");
      char * data = "reply data";
      size_t datalen = strlen(data);
      json_object_object_add (jobj, "data", json_object_new_string_len (data, datalen));

      json_object * rcObj;
      int apprc = 67;
      rcObj = json_object_new_string ("OK");
      json_object_object_add (jobj, "RC", rcObj);
      json_object_object_add (jobj, "apprc", json_object_new_int (apprc));

      queueMessage(pc->wsi, jobj);
      json_object_put(jobj);
      free(pc);
      g_hash_table_remove (pendingcalls,  key);
      debug("done one call\n");
   };
   debug("pending calls after replies %d\n", g_hash_table_size(pendingcalls));
   return ;
}

   
GAsyncQueue * testqueue ;

void * sender_thread_main(void * arg) {

   init_data_store();
   testqueue = g_async_queue_new ();
   
   while(1) {

      debug("sender thread sleep\n");
      sleep(10);
      debug("sender thread wake\n");
      testReplies();
      //queue_message();
      //      lws_cancel_service(context) ;
   }

   return NULL;
}
