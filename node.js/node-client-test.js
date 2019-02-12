
"use strict";

var midway = require('./midway').MidWay;

//mw.attach("url")

var mw = new midway('ws://localhost:9000/domain');
var mw2 = new midway('ws://localhost:9000/domain');

function ev1(evname, evdata) {
    console.info("EV:", evname, evdata);
}
function function1() {
    //console.info("MW: ", mw);
    mw.acall("testtime", "my data", (rpl, apprc, rc) => {
	console.debug("call replied", rpl, apprc, rc);

    });
    console.info("MW: ", mw.SUCCESS);
}

function function2() {
    mw.acall("testdate", "data", function(rply) {
	console.debug("call replied", rply);
    }, function(err) {
	console.debug("call replied error", err);
    });;
}
var subhdl ;
function function3() {
    subhdl = mw2.subscribe("1*", ev1);
}


function function9() {
    mw2.unsubscribe(subhdl);
    mw2.acall("testdate", "clientdata", (rpl, apprc, rc) => {
	console.debug("call replied: ", rpl, " with ", apprc, rc);
    });

}

setTimeout(function1, 500);

setTimeout(function3, 500);


setTimeout(function9, 10000);
