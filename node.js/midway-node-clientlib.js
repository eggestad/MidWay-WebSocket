var WebSocketClient = require('websocket').client;
var client = new WebSocketClient();


console.info("client", client);

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
    console.info("on message", msg);
    
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


exports.acall = function (svcname, data, successfunc, errorfunc, flags = 0) {
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

exports.subscribe = function (pattern, evfunc, flags = 0) {
    if (! pattern) throw ("required pattern argument is undefined");
    if (! evfunc ) throw ("required callback argument is undefined");
    
    console.info("mwsubscribe", pattern, flags);
    var submesg = {};
    submesg.command = "SUBSCRIBEREQ";
    submesg.pattern = pattern;
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
    console.info("mwunsubscribe", srcname, data, flags);
    var submesg = {};
    submesg.command = "UNSUBSCRIBEREQ";
    submesg.handle = getHandle();

//    submesg.flags = flags;
    websocket.send(JSON.stringify(submesg));
};


