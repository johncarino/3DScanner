"use strict";
/*
 * Respond to commands over a websocket to access the 3D-Scanner Program 
 */

var fs       = require('fs');
var socketio = require('socket.io');
var io; 
var dgram    = require('dgram');

exports.listen = function(server) {
	io = socketio.listen(server);
	//io.set('log level 1');
	io.set('log level',1);
	
	io.sockets.on('connection', function(socket) {
		handleCommand(socket);
	});
};

function handleCommand(socket) 
{
	console.log("Setting up socket handlers.");

	socket.on('mode', function(modeValue)
	{
		let modeNumber= null;
		if(typeof modeValue == "string")
		{
			const m = modeValue.toLowerCase().trim();
			if(m== "standard")
			{
				modeNumber=0;
			}	
			else if(m== "detailed")
			{
				modeNumber=1;
			}
			else if(m == "custom")
			{
				modeNumber=2;
			}
			
		}
		else
		{
			modeNumber= parseInt(modeValue);
		}	

		if (modeNumber === null || isNaN(modeNumber) || modeNumber < 0 || modeNumber > 2) 
			{
				socket.emit("3D-Scanner-error", "Invalid mode: " + modeValue);
				return;
    		}

		console.log("Got mode command: " , modeValue, ": " , modeNumber);
		relayToLocalPort(socket, "mode " + modeNumber, "mode-reply");
	});


	socket.on('custom', function(options)
	{	

		const samples= parseInt(options.samplesPerRev);
		const height= parseInt(options.heightChange);
		const  heights= parseInt(options.numHeights);
		
		if (isNaN(samples) || samples < 1 || samples > 32) 
		{
      		socket.emit("3D-Scanner-error", "SamplesPerRev out of range [1-32]");
      		return;
		}

    	if (isNaN(height) || height < 1 || height > 50) 
		{
      		socket.emit("3D-Scanner-error", "HeightChange out of range [1-50]");
      		return;
    	}

    	if (isNaN(heights) || heights < 1 || heights > 3) 
		{
			socket.emit("3D-Scanner-error", "NumHeights out of range [1-3]");
      		return;
		}

		console.log("Got custom:", samples, height, heights);
    	relayToLocalPort(socket, "custom " + samples + " " + height + " " + heights, "custom-reply");
	});


	socket.on('start', function() 
	{
		console.log("Got start command: ");
		relayToLocalPort(socket, "start", "start-reply");
	});
	


	socket.on('stop', function()
	{
		console.log("Got stop command: ");
		relayToLocalPort(socket, "stop", "stop-reply");



	});


	socket.on('shutdown', function()
	{

		console.log("Got shutdown command");
    	relayToLocalPort(socket, "shutdown", "shutdown-reply");
	});

	socket.on('pause', function(pauseVal)
  	{
    	let cmd = "pause toggle";
    	if (pauseVal === true)
			{
				cmd = "pause 1";
			} 
    	if (pauseVal === false) 
			{
				cmd = "pause 0";
			}

    	console.log("Got pause command:", cmd);
    	relayToLocalPort(socket, cmd, "pause-reply");
  	});
	
};

function readAndSendFile(socket, absPath, commandString) {
	fs.exists(absPath, function(exists) {
		if (exists) {
			fs.readFile(absPath, function(err, fileData) {
				if (err) {
					socket.emit("3D-Scanner-error", 
							"ERROR: Unable to read file " + absPath);
				} else {
					// Don't send back empty files.
					if (fileData.length > 0) {
						socket.emit(commandString, fileData.toString('utf8'));;
					}
				}
			});
		} else {
			socket.emit("3D-Scanner-error", 
					"ERROR: File " + absPath + " not found.");
		}
	});
}

function relayToLocalPort(socket, data, replyCommandName) {
	console.log('relaying to local port command: ' + data);
	
	// Info for connecting to the local process via UDP
	var PORT = 12345;	// Port of local application
	var HOST = '127.0.0.1';
	var buffer = new Buffer(data);

	// Send an error if we have not got a reply in a second
    var errorTimer = setTimeout(function() {
    	console.log("ERROR: No reply from local application.");
    	socket.emit("3D-Scanner-error", "SERVER ERROR: No response from 3D-Scanner application. Is it running?");
    }, 1000);

	
	var client = dgram.createSocket('udp4');
	client.send(buffer, 0, buffer.length, PORT, HOST, function(err, bytes) {
	    if (err) 
	    	throw err;
	    console.log('UDP message sent to ' + HOST +':'+ PORT);
	});
	
	client.on('listening', function () {
	    var address = client.address();
	    console.log('UDP Client: listening on ' + address.address + ":" + address.port);
	});
	// Handle an incoming message over the UDP from the local application.
	client.on('message', function (message, remote) {
	    console.log("UDP Client: message Rx" + remote.address + ':' + remote.port +' - ' + message);
	    
	    var reply = message.toString('utf8')
	    socket.emit(replyCommandName, reply);
	    clearTimeout(errorTimer);
	    client.close();
	});
	
	client.on("UDP Client: close", function() {
	    console.log("closed");
	});
	client.on("UDP Client: error", function(err) {
	    console.log("error: ",err);
	});	
}