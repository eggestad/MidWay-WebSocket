

var mw = require('./midway-node-clientlib');

mw.attach("url")

function ev1(evname, evdata) {
    console.info("EV:", evname, evdata);
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
    subhdl = mw.subscribe("1*", ev1);
}


function function9() {
    mw.unsubscribe(subhdl);
    mw.acall("testdate", "clientdata\306jjj", (rpl, apprc, rc) => {
	console.debug("call replied: ", rpl, " with ", apprc, rc);
    });

}

setTimeout(function3, 500);


setTimeout(function9, 3000);


process.stdin.setEncoding('utf8');

process.stdin.on('readable', () => {
  const chunk = process.stdin.read();
  if (chunk !== null) {

      mw.acall("testdate", chunk, function(rply, apprc, rc) {
	  console.debug("call replied: ", rply, " with ", apprc, rc);
      }, function(err) {
	  console.debug("call replied error", err);
      });;
      
  }
});

process.stdin.on('end', () => {
   mw.detach();
    
});
