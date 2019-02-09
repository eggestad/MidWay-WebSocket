

var mw = require('./midway-node-clientlib');



mw.attach("url")

function ev1(evname, evdata) {
    console.info("EV:", evname, evedata);
}

function function2() {
    mw.acall("testdate", "data", function(rply) {
	console.debug("call replied", rply);
    }, function(err) {
	console.debug("call replied error", err);
    });;
}
function function3() {
    let rc = mw.subscribe("1*", ev1, ev1);
}


function function9() {
    mw.detach();
}

setTimeout(function3, 500);


//setTimeout(function9, 3000);


process.stdin.setEncoding('utf8');

process.stdin.on('readable', () => {
  const chunk = process.stdin.read();
  if (chunk !== null) {

      mw.acall("testdate", chunk, function(rply) {
	  console.debug("call replied", rply);
      }, function(err) {
	  console.debug("call replied error", err);
      });;
      
  }
});

process.stdin.on('end', () => {
   mw.detach();
    
});
