
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>

#include <pthread.h>
#include <libwebsockets.h>
#include <MidWay.h>
#include <ipcmessages.h>
#include <ipctables.h>
#include <mwclientapi.h>
#include <shmalloc.h>

#include "mwwsd.h"


void testReplies() ;
void testEvents() ;

void do_svcreply(Call * cmsg, int len){
   debug("got IPC callreply \n");
   char * data;
   size_t datalen;

   _mw_getbuffer_from_call (cmsg, &data, &datalen);
   mwhandle_t handle = cmsg->handle;
   int32_t appreturncode = cmsg->appreturncode;
   int returncode = cmsg->returncode;

   deliver_svcreply(handle, data, datalen, appreturncode, returncode);
   
   mwfree(data);
}

void do_event_message(Event * evmsg, int len) {
   debug("got IPC event %s dataoff %d datalen %zu\n",
	 evmsg->event, evmsg->data, evmsg->datalen);
   
   char * data = NULL; 
   size_t datalen = evmsg->datalen;
   if (evmsg->datalen > 0) {	
      data = _mwoffset2adr(evmsg->data, _mw_getsegment_byid(evmsg->datasegmentid));
   }
   char * eventname = evmsg->event;
   char * username = evmsg->username;
   char * clientname = evmsg->clientname;

   processEvent(eventname, data, datalen);

   if (evmsg->datalen > 0) {
      evmsg->mtype = EVENTACK;
      evmsg->senderid = _mw_get_my_mwid();
      DEBUG1("Sending event ack");
      _mw_ipc_putmessage(0, (char *)evmsg, sizeof(Event), 0);
   };
}


int doMidWayIPCMessage(int noblock) {

  int rc, errors;
  size_t len = MWMSGMAX;
  char message[MWMSGMAX];
  int flags = 0;
  
  long *           mtype  = (long *)           message;
  
  Attach *         amsg   = (Attach *)         message;
  Administrative * admmsg = (Administrative *) message;
  Provide *        pmsg   = (Provide *)        message;
  Call *           cmsg   = (Call *)           message;

  if (noblock) flags |= MWNOBLOCK;

  debug("doing getipc message");
  rc = _mw_ipc_getmessage(message, &len, 0, flags);

  if (rc == -EIDRM) {
     return -1;
  };
  if (rc == -EINTR) 
     return 0;
  if (rc == -ENOMSG) 
     return 0;

  debug("got message of type %ld", *mtype);

  switch (*mtype) {

  case ATTACHREQ:
  case DETACHREQ: 
     /*  it  is  consiveable  that  we may  add  the  possibility  to
         disconnect  spesific clients, but  that may  just as  well be
         done  thru  a  ADMREQ.  We  have so  fare  no  provition  for
         disconnecting clients other than mwd marking a client dead in
         shm tables. */
     Warning("Got a attach/detach req from pid=%d, ignoring", amsg->pid);
     break;

  case ATTACHRPL:
     Warning("Got a Attach reply that I was not expecting! clientname=%s gwid=%#x srvid=%#x", amsg->cltname, amsg->gwid, amsg->srvid);
     break;

  case DETACHRPL:
     /* do I really care? */
     break;

  case SVCCALL:
  case SVCFORWARD:
     Error("received a call/forward request from IPC");
     // shall never happen
     break;
  case SVCREPLY:
     do_svcreply(cmsg, len);
     return 1;

  case ADMREQ:
     /* the only adm request we accept right now is shutdown, we may
	later accept reconfig, and connect/disconnect gateways. */
     if (admmsg->opcode == ADMSHUTDOWN) {
	Info("Received a shutdown command from clientid=%d", 
	     CLTID2IDX(admmsg->cltid));
	return -2; /* going down on command */
     }; 
     break;
      
  case PROVIDEREQ:
     Error("received a provide request from IPC");
     break;

  case PROVIDERPL:
     Error("received a provide reply from IPC");
     break;

  case UNPROVIDEREQ:
     Error("received a unprovide request from IPC");
     break;

  case UNPROVIDERPL:
     Error("received a unprovide reply from IPC");
     break;

     /* events */
  case EVENT:
     do_event_message((Event *) message, len);
     return 1;

  case EVENTSUBSCRIBEREQ:
  case EVENTSUBSCRIBERPL:
     break;
     
  default:
     Warning("got a unknown message with message type = %#lx", *mtype);
  } /* end switch */
  return 0;
}

extern int doshutdown;
void * sender_thread_main(void * arg) {
   while(!doshutdown) {
      int rc = doMidWayIPCMessage(0);
      if (rc < 0) {
	 doshutdown = 1;
	 return NULL;
      }
   }
   
   /* while(1) { */

   /*    debug("sender thread sleep\n"); */
   /*    sleep(10); */
   /*    debug("sender thread wake\n"); */
   /*    testReplies(); */
   /*    testEvents(); */
   /*    //      lws_cancel_service(context) ; */
   /* } */

   return NULL;
}
