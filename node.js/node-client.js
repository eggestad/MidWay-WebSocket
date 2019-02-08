

var mw = require('./midway-node-clientlib');



mw.attach("url")


function function2() {
    mw.acall("testdate", "data", function(rply) {
	console.debug("call replied", rply);
    }, function(err) {
	console.debug("call replied error", err);
    });;
}

function function9() {
    mw.detach();
}

//setTimeout(function2, 2000);


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
