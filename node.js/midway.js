
"use strict";

//var WebSocketClient = require('websocket').client;
//var client = new WebSocketClient();


if (typeof(window) !== 'undefined') {
    var ___scope = window;
} else if (typeof(exports) !== 'undefined') {
    var ___scope = exports;
} else {
    throw ("Looks like runtime is neither a browser or node.js");
}
	    
var mw = (function(scope) {
    var onerrorhandler = function(error) {
	throw(error);
    }
    var onclosehandler = function(cause) {
	
    }
    function debug(msg) {
	var args = Array.prototype.slice.call(arguments);
	args.unshift("DEBUG");
	console.debug.apply(console, args);
    }
    
    scope.MidWay = function(url, onready, onerror, onclose) {
	
	if ( typeof(onerror) === 'function') onerrorhandler = onerror;
	if ( typeof(onclose) === 'function') onclosehandler = onclose;
	
	if (typeof(window) !== 'undefined') {
	    debug("constructor browser")
	    window.WebSocket = window.WebSocket || window.MozWebSocket;
	    this.websocket = new window.WebSocket(url, 'midway-1');
	    this.websocket.midway = this;
	    let websocket = this.websocket;
	    
	    websocket.onopen = (event) => {
		let mesg = {};
		mesg.command = "ATTACH"
		let m = JSON.stringify(mesg)
		let rc = websocket.send(m);
		debug(" send", m, " rc", rc);
		if ( typeof(onready) === 'function') onready();
	    };
	    websocket.onerror = (event) => {
		console.info("on error", error);
		onerrorhandler(error);
	    };
	    websocket.onmessage = onMessage;
	    websocket.onclose = (event) => {
		debug("on close", reasonCode, desc);
		onclosehandler(reasonCode, desc)
		websocket = undefined;
	    };
	} else {
	    debug("constrctor node.js")
	    this.WebSocketClient = require('websocket').client;
	    this.client = new this.WebSocketClient();
	    let client = this.client;
	    client.connect(url, 'midway-1', null, null, null);
	    client.midway = this;
	    this.websocket = undefined;
	    /*
	     * This do the heavy lifting on attach 
	     */
	    client.on("connect", function(conn) {
       		client.midway.websocket = conn;
		conn.client = this;
		conn.midway = client.midway;
		
		conn.on("message", onMessage);
		
		conn.on("frame", function(frame) {
		    debug("on frame, this should never happen", frame);
		});
		conn.on("error", function(error) {
		    console.info("on error", error);
		    onerrorhandler(error);
		});
		conn.on("close", function(reasonCode, desc) {
		    debug("on close", reasonCode, desc);
		    onclosehandler(reasonCode, desc)
		    conn.midway.websocket = undefined;
		});
		
		
		let mesg = {};
		mesg.command = "ATTACH"
		let m = JSON.stringify(mesg)
		let rc = conn.send(m);
		debug(" send", m, " rc", rc);
		if ( typeof(onready) === 'function') onready();

	    });

	    client.on("connectFailed", (errdesc) => {
		onerrorhandler("connection failed: " + errdesc);
	    });
	}

	
	var handle = 0x1234;
	function getHandle() {
	    if (handle > 0x0fffffff) handle = 0x1234;
	    return handle++;
	}

	this.pendingcalls = new Object();
	this.subscriptions = new Object();

	// constants
	this.GLOB = 10;
	this.REGEXP = 11;	
	this.SUCCESS = 1;
	this.FAIL = 0;
	this.MORE = 2;

	/* this is the heavy lifting on all messages from server */
	function onMessage(msg) {
	    debug("on message", msg);
	    // if node.js
	    if (msg.hasOwnProperty('type') && msg.type == 'utf8') {
		msg  = JSON.parse(msg.utf8Data);
		// if browser
	    } else if ( typeof(msg.data) !== 'undefined' ){
		msg  = JSON.parse(msg.data);
	    } else {
		debug("on message", msg.data);
		onerrorhandler("can't find data in websockt message");
	    }
	    debug("on message", msg);
	    //console.debug("on message this ", this);
	    //console.debug("on message this.client ", this.client);
	    //console.debug("on message this.midway ", this.midway);

	    if (msg.RC == "error") {
		onerrorhandler("server rejected a message from us: " + msg.error);
		return;
	    }

	    try {
		let command = msg.command;
		debug("command: ", msg.command);
		if (command == "ATTACH") {
		    if (msg.RC == "OK") {
			debug ("connected OK to MidWay server");
		    } else {
			onerrorhandler ("MidWay server rejected connection: " +
					msg.reason);
			this.midway.websocket.close();
			websocket = undefined;
		    }
		    
		} else if (command == "EVENT") {
		    let handle = msg.handle;
		    let event_endpoint = this.midway.subscriptions[handle];
		    if (! event_endpoint)
			onerrorhandler("got an unexpected event")
		    else 
			event_endpoint.evfunc(msg.event, msg.data);
		}
		
		else if (command == "CALLRPL") {
		    let handle = msg.handle;
		    
		    let call_endpoint = this.midway.pendingcalls[handle];
		    if (call_endpoint.errfunc && msg.RC == "FAIL")
			call_endpoint.errfunc(data, apprc);
		    else
			call_endpoint.successfunc(msg.data, msg.apprc, msg.RC);
		    if (msg.RC == "OK")
			delete this.midway.pendingcalls[handle];
		    
		}
		else if (command == "SUBSCRIBERPL") {
		    if (msg.RC == "OK") {
		    } else if (msg.RC == "FAIL") {
			onerrorhandler("server rejected subscripion, reason: "+ msg.reason);
			delete this.midway.subscriptions[msg.handle];
		    }
		    
		    
		}
		else if (command == "UNSUBSCRIBERPL") {
		    delete this.midway.subscriptions[msg.handle];
		    
		} else {
		    onerrorhandler("received a message with unknown command: "+ command);
		}
		
	    } catch (e) {
		onerrorhandler("while handling a message from server" + e);
	    }
	}


	this.detach = function () {
	    this.websocket.close();
	    this.websocket = undefined;
	};


	this.acall = function (svcname, data = undefined,
				  successfunc = undefined, errorfunc = undefined,
				  flags = 0) {
	    if (! svcname) throw ("required service name argument is undefined");
	    
	    debug("mwacall", svcname, data, flags);
	    let callmesg = {};
	    callmesg.command = "CALLREQ";
	    callmesg.service = svcname;
	    if (data) 
		callmesg.data = data;

	    if (successfunc) {
		callmesg.handle = getHandle();
		let pc = { "successfunc": successfunc, "errorfunc": errorfunc};
		this.pendingcalls[callmesg.handle] = pc;
	    }
	    //    callmesg.flags = flags;
	    this.websocket.send(JSON.stringify(callmesg));
	};

	this.subscribe = function (pattern, evfunc) {
	    if (! pattern) throw ("required pattern argument is undefined");
	    if (! evfunc ) throw ("required callback argument is undefined");

	    if (typeof(evfunc) != 'function')
		throw ("required callback argument is not a function");

	    debug("mwsubscribe", pattern);
	    let submesg = {};
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
	    this.subscriptions[submesg.handle] = sub;

	    //    submesg.flags = flags;
	    this.websocket.send(JSON.stringify(submesg));
	    return submesg.handle;
	};

	this.unsubscribe = function (pattern_or_handle) {
	    let handle = undefined;
	    if (typeof(pattern_or_handle) == "string") {
		for (var hdl in this.subscriptions) {
		    let sub = this.subscriptions[hdl];
		    if (sub.pattern == pattern_or_handle){
			handle = hdl;
			break;
		    }
		    if (!handle) 
			throw ("have no matching subscriptions")
		}
	    } else {
		handle = pattern_or_handle;
		if (! this.subscriptions.hasOwnProperty(handle))
		    throw ("have no matching subscriptions")
	    }
	    debug("mwunsubscribe", pattern_or_handle, handle);
	    var submesg = {};
	    submesg.command = "UNSUBSCRIBEREQ";
	    submesg.handle = handle;

	    //    submesg.flags = flags;
	    this.websocket.send(JSON.stringify(submesg));
	};

	
    }
    
})(___scope);

if (typeof(window) !== 'undefined') {
    ___scope;
}
