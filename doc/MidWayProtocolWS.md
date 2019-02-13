

Description of the MidWay protocol over WebSocket
=================================================

The SRB protocol that MidWay normally uses with the MidWay gateway (mwgwd) is
ill suited for WebSocket. (see SRB doc in main MidWay project)

WebSocket is unlike that of TCP that WebSocket maintain message boundries,
as well as that you can send usefull information in the connection call to
indicate what sub system, like what domain you want to connect to.

The other is that formating and parsing SRBP style messages would
require a lot of JS code, while all JS vm's come with JSON parse, serializers.

The conclusion is that over WebSocket, the messages should be JSON encoded.

If the JSON contain additional fields they are ignored by server.
In the case of ATTACH, (UN)SUBSCRIBE they will be returned in the reply.
This will/may not be the case for CALL's

If the server fail to parse or find required fields in a message a general
error message is returned  
`{`  
`  "RC": "error",`        // _REQUIRED_  
`  "error": "..."`        // _REQUIRED_ a text describing the cause  
`}`  


## Attach:

Domain name is passed as a part of the path in the HTTP GET,
Client name, username, usercredentials are passed as query
params in the initial HTTP call.

The client shall still send a message  
`{`  
`  "command": "ATTACH", ` // _REQUIRED_  
`  "domain": "..." ,    ` // _OPTIONAL_  
}`

since it's a way to return the cause of a failure  
`{`  
`  "command": "ATTACH", ` // same as request
`  "domain": "..." ,    ` // same as request if exists
`  "RC": "OK"     ,     ` // OK, or FAIL
`  "reason": "...",     ` // if FAIL a text message of the reason
`}`  

A WebSocket connecting may not be reused, a Detach is done by closing.


## CALLS

A client send request with:  
`{`  
`  "command": "CALLREQ",` // _REQUIRED_  
`  "service": str,	` // _REQUIRED_  
`  "data":  "..."       ` // _OPTIONAL_ there is no limit (well 2^64) on a WebSocket  
`  	   		` // frame, so we don't break up large data  
`  "handle": int/str    ` // _OPTIONAL_ a handle ID. a 32 bit int (1..2^31-1) either  
`  	    		` // an int or decimal string  
`			` // is not present, this is a call with no reply  
`  "multiple": "YES"    ` // NYI _OPTIONAL_, server may send multiple replies  
`}`  

a call reply from server (mwwsd) to client

`{`  
`  "command": "CALLRPL",   `  // _REQUIRED_  
`  "data":  "...",         `  // _OPTIONAL_ see above on size  
`  "handle": int/str       `  // _REQUIRED_ must be the same as handle in CALLREQ  
`  "RC": "OK"|"FAIL"|"MORE"`  // _REQUIRED_ "MORE" iff "multiple" was set in CALLREQ  
`  "apprc": int/str	   `  // _REQUIRED_ a 32 bit int signed, as an int or decimal string  
`}`  


## SUBSCRIBE / UNSUBSCRIBE
Request from client:  
`{`  
`  "command": "SUBSCRIBEREQ"  ` // _REQUIRED_   
`  "pattern": str	      ` // _REQUIRED_  
`  "handle": int|str	      ` // _REQUIRED_  
`  "type": "glob"|"regex"     ` // _OPTIONAL_ default glob  
`}`  
An UNSUBCRIBE handle must match a SUBSCRIBE exactly  
`{`  
`  "command": "UNSUBSCRIBEREQ" ` // _REQUIRED_   
`  "handle": int|str	       ` // _REQUIRED_  
`}`  

Server will reply with  
`{`  
`  "command": "SUBSCRIBERPL"|"UNSUBSCRIBERPL" ` // _REQUIRED_  
`  "handle": int|str	      		      ` // _REQUIRED_      
`  "RC": "OK"|"FAIL"		       	      ` // _REQUIRED_  
`}`  


## EVENT
Sent from Server only for event matching a SUBSCRIBE, shall not be replied to  
`{`  
`  "command": "EVENT"     //` _REQUIRED_  
`  "event":  str          //` _REQUIRED_	  
`  "handle": [ int|str ]  //` _REQUIRED_  a list of the sub handle that matches  
`  "data": str            //` _OPTIONAL_  
`}`  
