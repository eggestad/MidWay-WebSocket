
"use strict";
 

function MidWay(url) {

    console.info("in constructor");
    
    /* "contructor */
    window.WebSocket = window.WebSocket || window.MozWebSocket;

    if (! url) url = 'ws://127.0.0.1:9000';
    
    this.url = url;
    
    var websocket = new WebSocket(url, 'midway-1');
    websocket.onopen = onopen;
    websocket.onerror = onerror;
    websocket.onmessage = onmessage;
    websocket.onclose = onclose;
    
    console.debug(websocket);
    
    //if (!websocket) return -1;
    /* END Constructor */    
    
    /* Callback from Web socket */
    function onopen(event) {
        console.log("on open", event);
    };
    function onclose(event) {
	if (! event.wasClean)
            console.error("on close", event);
    };
    function onerror(event) {
        console.error("on error", event);
    };
    function onmessage(message) {
        console.log("onmessage",message.data);
    };
    

    /* API */
    this.acall = function (srcname, data, successfunc, errorfunc, flags = 0) {
	console.info("mwacall", srcname, data, flags);
	var calldata = {};
	calldata.function = "call";
	calldata.service = srcname;
	calldata.data = data;
	calldata.flags = flags;
	websocket.send(JSON.stringify(calldata));
    };

    this.subscribe = function (globpattern, callback) {
	console.info("mwsubscribe", globpattern, callback);	
    };
    this.attach = function () {
	console.info("att");
	websocket = new WebSocket(url, 'midway-1');
	websocket.onopen = onopen;
	websocket.onerror = onerror;
	websocket.onmessage = onmessage;
	websocket.onclose = onclose;
    };
    this.detach = function () {
	console.info("det");
	websocket.close();
	websocket = undefined;
    };
    console.info("done constructor");
    return this;
}

