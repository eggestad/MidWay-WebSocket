var WebSocketClient = require('websocket').client;
var client = new WebSocketClient();

var websocket ;
var attachInProgress = false;

   
/*
 * This do the heavy lifting on attach 
 */
client.on("connect", function(conn) {
    console.info("on connect");
    websocket = conn;
    
    conn.on("message", onMessage);
    
    conn.on("frame", function(frame) {
	console.info("on frame, this should never happen", frame);
    });
    conn.on("error", function(error) {
	console.info("on error", error);
    });
    conn.on("close", function(reasonCode, desc) {
	console.info("on close", reasonCode, desc);
	process.stdin.destroy();
	websocket = undefined;
    });
    
    
    let mesg = {};
    mesg.command = "ATTACH"
    let m = JSON.stringify(mesg)
    let rc = conn.send(m);
    console.info(" send", m, " rc", rc);
});

var handle = 0x1234;
function getHandle() {
    if (handle > 0x0fffffff) handle = 0x1234;
    return handle++;
}

/* this is the heavy lifting on all messages from server */
function onMessage(msg) {
    console.debug("on message", msg);
    if (msg.type == 'utf8')
	msg  = JSON.parse(msg.utf8Data);
    console.debug("on message", msg);

    if (msg.RC == "error") {
	console.error("server rejected a message from us, ", msg.error);
	return;
    }

    try {
	let command = msg.command;
	console.debug("command", msg.command, command);
	if (command == "ATTACH") {
	    if (msg.RC == "OK") {
		console.info ("connected OK to MidWay server");
	    } else {
		console.info ("MidWay server rejected connection,", msg.reason);
		websocket.close();
		websocket = undefined;
	    }
	    
	} else if (command == "EVENT") {
	    let handle = msg.handle;
	    let event_endpoint = subscriptions[handle];
	    if (! event_endpoint)
		console.error("got an unexpected event")
	    else 
		event_endpoint.evfunc(msg.event, msg.data);
	}
	
	else if (command == "CALLRPL") {
	    let handle = msg.handle;
	    
	    let call_endpoint = pendingcalls[handle];
	    if (call_endpoint.errfunc && msg.RC == "FAIL")
		call_endpoint.errfunc(data, apprc);
	    else
		call_endpoint.successfunc(msg.data, msg.apprc, msg.RC);
	    if (msg.RC == "OK")
		delete pendingcalls[handle];
	    
	}
	else if (command == "SUBSCRIBERPL") {
	    if (msg.RC == "OK") {
	    } else if (msg.RC == "FAIL") {
		console.error("server rejected subscripion, reason", msg.reason);
		delete subscriptions[msg.handle];
	    }
	    
	    
	}
	else if (command == "UNSUBSCRIBERPL") {
	    delete subscriptions[msg.handle];
	    
	} else {
	    console.error("received a message with unknown command, ", command);
	}
			
    } catch (e) {
	console.error("while handling a message from server", e);
    }
}


var pendingcalls = new Object();
var subscriptions = new Object();

// constants
exports.MWREGEXP = 11;

exports.MWSUCCESS = 1;
exports.MWFAIL = 0;
exports.MWMORE = 2;

// methods
exports.attach = function (url) {
    client.connect('ws://localhost:9000/', 'midway-1', null, null, null);
}

exports.detach = function () {
    console.info("det");
    websocket.close();
    websocket = undefined;
};


exports.acall = function (svcname, data = undefined,
			  successfunc = undefined, errorfunc = undefined,
			  flags = 0) {
    if (! svcname) throw ("required service name argument is undefined");
    
    console.info("mwacall", svcname, data, flags);
    var callmesg = {};
    callmesg.command = "CALLREQ";
    callmesg.service = svcname;
    if (data) 
	callmesg.data = data;

    if (successfunc) {
	callmesg.handle = getHandle();
	let pc = { "successfunc": successfunc, "errorfunc": errorfunc};
	pendingcalls[callmesg.handle] = pc;
    }
//    callmesg.flags = flags;
    websocket.send(JSON.stringify(callmesg));
};

exports.subscribe = function (pattern, evfunc) {
    if (! pattern) throw ("required pattern argument is undefined");
    if (! evfunc ) throw ("required callback argument is undefined");

    if (typeof(evfunc) != 'function')
	throw ("required callback argument is not a function");

    console.info("mwsubscribe", pattern);
    var submesg = {};
    submesg.command = "SUBSCRIBEREQ";
    if (typeof(pattern) == 'object' && pattern.source) {
    	submesg.pattern = pattern.source
	submesg = "regex";
    } else if (typeof(pattern) == 'string')
	submesg.pattern = pattern;
    else
	throw ("required pattern argument must be either a string of regex");
    
    submesg.handle = getHandle();
    let sub = { "pattern": pattern, "evfunc": evfunc };
    subscriptions[submesg.handle] = sub;

//    submesg.flags = flags;
    websocket.send(JSON.stringify(submesg));
    return submesg.handle;
};

exports.unsubscribe = function (pattern_or_handle) {
    let handle = undefined;
    if (typeof(pattern_or_handle) == "string") {
	for (var hdl in subscriptions) {
	    let sub = subscriptions[hdl];
	    if (sub.pattern == pattern_or_handle){
		handle = hdl;
		break;
	    }
	    if (!handle) 
		throw ("have no matching subscriptions")
	}
    } else {
	handle = pattern_or_handle;
	if (! subscriptions.hasOwnProperty(handle))
	    throw ("have no matching subscriptions")
    }
    console.info("mwunsubscribe", pattern_or_handle, handle);
    var submesg = {};
    submesg.command = "UNSUBSCRIBEREQ";
    submesg.handle = handle;

//    submesg.flags = flags;
    websocket.send(JSON.stringify(submesg));
};


