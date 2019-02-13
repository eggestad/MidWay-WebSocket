

Server side
===========

The WebSocker Server  is easy enough to use. 

`./mwwsd: [-A ipcuri] [-l level] [-L logprefix]  [-p WSport] `
`-A ipcuri    : The IPC url to a running MidWay instance`
`-l level     : log level`
`-L logprefix : The prefix for the logfile`
`-p WS-port    : The port to listen for WebSocket connections default 9000`


if you're running mwd without address parameter, you do the same with mwwsd.
Otherwise you'll need to give the instance name by -A ipc:/instancename
if running as the same user as mwd, else you need to get the IPCKEY
from mwadm, assuming that the IPC umask is set to allow the other user to
attach.

Default port for listening to websocke connections is port 9000



Javascript Client side
======================

If running in a browser all you need is to load the midway.js with a script tag.
If running node you need have websocket installed with
npm install websocket 
then all you need to do is:

`var MidWay = require('./midway').MidWay;`


Once midway.js is loaded then run

`var mw = new MidWay(url, [onconnect], [[onerror], [[onclose]]]);`
url is required and is on the form 'ws://127.0.0.1:9000'

Callbacks are not necessary, but if you try to do either subscribe or acall
before the websocket connection is established, you will get an exception.
Subscriptions done up front should be done in the onconnect callback.

## Todo a call:
`mw.acall("servicename", [data], [successfunc], [errorfunc]);`
if successfunc is undefined, the acall will be in the blind, you'll never
know if the service succeded, or even if it existed.
if errorfunc is undefined, the successfunc is called on failure
_successfunc_ get (data, apprc, rc)
_errorfunc_(data, apprc)

## Events are done by
`mw.subscribe(pattern, oneventfunc)`
Pattern are either a "glob" if a string (see fnmatch(3)) or a /regex/
oneventfunc get (eventname, eventdata)
it returns a subscription id that should be used with
`mw.unsubscribe(subid)`

## The connection is terminated with
`mw.detach()`

