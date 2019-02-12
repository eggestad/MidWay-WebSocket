
MidWay over WebSocket
=====================


This contain the WebSocket interfaces for MidWay. WebSockets implement
a lot of the protocol that is native to MidWay, SRBP. This allows for a
very thin client side on top of WebSockets.

Client side support both the browser WebSocket interface as
well as 'websocket' library for node.js

The mwwsd directory contain the server side MidWay WebSocket Daemon.
Compiling require access to the internal header files of the Core MidWay
sourcecode, so you need that source code to compile.
Simple Makefile to build

