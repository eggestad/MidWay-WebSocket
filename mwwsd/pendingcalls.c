
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>

#include <pthread.h>
#include <libwebsockets.h>
#include <MidWay.h>

#include "mwwsd.h"



G_LOCK_DEFINE (pendingcalls);

/*
 * for pending calls we use the IPC handle that we assing  to mwcalls invards 
 * as key. It also means we get a direct map between hanlde and a PendingCall
 */  
static GHashTable * pendingcalls = NULL;

void init_pendingcall_store() {
      
   pendingcalls = g_hash_table_new (g_direct_hash, g_direct_equal);

}

void pendingcalls_lock(){
   G_LOCK (pendingcalls);
}
void pendingcalls_unlock(){
   G_UNLOCK (pendingcalls);
}


int addPendingCall(PendingCall * pc) {
   // we expect the lock to be already held when we get here.
   // see doCallReq in protocols.c. We have to have the locks
   // brefore _mwacallipc()
   
   //G_LOCK (pendingcalls);
   long hdl = pc->internalhandle;
   debug("add pending call with handle %ld %p\n", hdl, pc);
   gpointer inthandle = (gpointer) hdl;
   gboolean rc = g_hash_table_insert(pendingcalls, inthandle, pc);
   debug("add pending calls now %d\n", g_hash_table_size(pendingcalls));
   //G_UNLOCK (pendingcalls);
   return rc ;
}

void clearPendingCalls(struct lws * wsi) {
   G_LOCK (pendingcalls);
   guint length;
   debug("pending calls before clearing %d\n", g_hash_table_size(pendingcalls));

   gpointer * keys = 
      g_hash_table_get_keys_as_array (pendingcalls, &length);
   for (int i = 0; i < length; i++) {
      gpointer key = keys[i];
      gpointer val = 
	 g_hash_table_lookup (pendingcalls,  key);
      PendingCall * pc = val;
      if (pc->wsi == wsi) {

	 debug("clearing a pending cal\n");
	 free(pc);
	 g_hash_table_remove (pendingcalls,  key);
      }
   }
   g_free(keys);
   debug("pending calls now %d\n", g_hash_table_size(pendingcalls));
   G_UNLOCK (pendingcalls);
}

void deliver_svcreply(int32_t handle, char * data, size_t datalen,
		      int appreturncode, int returncode) {
   G_LOCK (pendingcalls);
   debug("delivering call internal handle = %d\n", handle);
   long hdl = handle;
   gpointer val = g_hash_table_lookup (pendingcalls,  (gpointer) hdl);
   if (val == 0) {
      error(" tried to deliver an unexpected service reply");
      goto out;
   }

   PendingCall * pc = val;
   debug("delivering pc %p\n", pc);
   struct json_object * jobj = pc->jobj;

   json_object_object_del(jobj, "command");
   json_object_object_add (jobj, "command", json_object_new_string ("CALLRPL"));
   json_object_object_del(jobj, "data");

#if STRDATA
   if (data != NULL &&datalen > 0) {
      json_object_object_add (jobj, "data", json_object_new_string_len (data, datalen));
   }
#endif 
   char * rctxt  = "FAIL";
   if (returncode == MWMORE) rctxt = "MORE";
   if (returncode == MWSUCCESS) rctxt = "OK";
      
   json_object * rcObj;
   rcObj = json_object_new_string (rctxt);
   json_object_object_add (jobj, "RC", rcObj);
   json_object_object_add (jobj, "apprc", json_object_new_int (appreturncode));
   queueMessage(pc->wsi, jobj, data, datalen);
   if (returncode != MWMORE) {
      json_object_put(jobj);
      free(pc);
      g_hash_table_remove (pendingcalls,  (gpointer) hdl);
   }

 out:
   G_UNLOCK (pendingcalls);
   debug("done one call\n");
};
/*
 * Just used for testing 
 */   
void testReplies() {
   G_LOCK (pendingcalls);
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

      queueMessage(pc->wsi, jobj, NULL, 0);
      json_object_put(jobj);
      free(pc);
      g_hash_table_remove (pendingcalls,  key);
      debug("done one call\n");
   };
   debug("pending calls after replies %d\n", g_hash_table_size(pendingcalls));
   G_UNLOCK (pendingcalls);
   return ;
}

   
 
