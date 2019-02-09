
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>

#include <pthread.h>
#include <libwebsockets.h>
#include <MidWay.h>

#include "mwwsd.h"


void testReplies() ;
void testEvents() ;


void * sender_thread_main(void * arg) {

   while(1) {

      debug("sender thread sleep\n");
      sleep(10);
      debug("sender thread wake\n");
      testReplies();
      testEvents();
      //      lws_cancel_service(context) ;
   }

   return NULL;
}
