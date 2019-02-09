
#include <stdio.h>
#include <stdlib.h>
#include <stdlib.h>
#include <glib.h>

#include <pthread.h>
#include <libwebsockets.h>
#include <MidWay.h>

#include "mwwsd.h"
GHashTable * pendingcalls = NULL;

void init_data_store() {
      
   pendingcalls = g_hash_table_new (g_int_hash, g_int_equal);

}

int addPendingCall(PendingCall * pc) {
   gpointer inthandle = (gpointer) (long) pc->internalhandle;
   gboolean rc = g_hash_table_insert(pendingcalls, inthandle, pc);

   
   return rc ;
}
      
GAsyncQueue * testqueue ;

void * sender_thread_main(void * arg) {

   init_data_store();
   testqueue = g_async_queue_new ();
   
   while(1) {

      debug("sender thread sleep\n");
      sleep(10);
      debug("sender thread wake\n");
      queue_message();
      lws_cancel_service(context) ;
   }

   return NULL;
}
