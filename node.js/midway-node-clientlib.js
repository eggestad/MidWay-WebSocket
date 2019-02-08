var WebSocketClient = require('websocket').client;
var client = new WebSocketClient();


console.info("client", client);

var websocket ;
var attachInProgress = false;

function onMessage(msg) {
    console.info("on message", msg);
    
}
    
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
	websocket = undefined;
    });
    
    
    let mesg = {};
    mesg.command = "ATTACH"
    let m = JSON.stringify(mesg)
    let rc = conn.send(m);
    console.info(" send", m, " rc", rc);
});


exports.attach = function (url) {
    client.connect('ws://localhost:9000/', 'midway-1', null, null, null);
}

exports.detach = function () {
    console.info("det");
    websocket.close();
    websocket = undefined;
};

exports.acall = function (srcname, data, successfunc, errorfunc, flags = 0) {
    console.info("mwacall", srcname, data, flags);
    var calldata = {};
    calldata.command = "CALLREQ";
    calldata.service = srcname;
    calldata.data = data;
    calldata.flags = flags;
    websocket.send(JSON.stringify(calldata));
};


console.info("this", this);
