"use strict";
// Client-side interactions with the browser for web interface

// Make connection to server when web page is fully loaded.
var socket = io.connect();
var communicationsTimeout = null;
$(document).ready(function() {
	setupServerMessageHandlers(socket);

	// Mode buttons
  $("#modeStandard").click(function () {
    sendCommandToServer("mode", "standard"); // server maps to 0
  });
  $("#modeDetailed").click(function () {
    sendCommandToServer("mode", "detailed"); // server maps to 1
  });
  $("#modeCustom").click(function () {
    sendCommandToServer("mode", "custom"); // server maps to 2
  });


  //  Apply custom changes
  $("#applyCustom").click(function () {
    const samplesPerRev = parseInt($("#samplesPerRev").val(), 10);
    const heightChange  = parseInt($("#heightChange").val(), 10);
    const numHeights    = parseInt($("#numHeights").val(), 10);

    sendCommandToServer("custom", {
      samplesPerRev: samplesPerRev,
      heightChange: heightChange,
      numHeights: numHeights,
    });
  });


  $("#startBtn").click(function () {
    sendCommandToServer("start");
    $("#stateid").text("Running");
  });

  $("#pauseBtn").click(function () {
    // server will treat anything other than true/false as toggle
    sendCommandToServer("pause", "toggle");
  });

  $("#stopBtn").click(function () {
    sendCommandToServer("stop");
    $("#stateid").text("Stopped");
  });

  $("#shutdownBtn").click(function () {
    sendCommandToServer("shutdown");
    $("#stateid").text("Shutting down");
  });
});



var hideErrorTimeout=null;
function setupServerMessageHandlers(socket) {
	// Hide error display:
	$('#error-box').hide(); 
	
	
	socket.on('mode-reply', function(message) {
		console.log("Receive Reply: mode-reply " + message);

  		var name = "Unknown!";
  		switch (Number(message)) {
			case 0: name = "Standard"; break;
    		case 1: name = "Detailed"; break;
    		case 2: name = "Custom"; break;
		}

  	if (typeof window.setMode === "function") {
    window.setMode(name);
  	}
	else {
    $("#modeid").text(name);
    $("#modePill").text(name);
  	}

  appendStatus("Mode set: " + name + " (reply=" + message + ")");
  clearServerTimeout();
	});

	
	socket.on('custom-reply', function(message) {
  		appendStatus("Custom reply: " + message);
  		clearServerTimeout();
	});

	socket.on('pause-reply', function(message) {
  		appendStatus("Pause reply: " + message);
  		clearServerTimeout();
	});

	socket.on('start-reply', function(message) {
  		appendStatus("Start reply: " + message);
  		clearServerTimeout();
	});

	
	socket.on("stop-reply", function (message) {
    appendStatus("Stop reply: " + message);
    clearServerTimeout();
 	});

  	socket.on("shutdown-reply", function (message) {
    appendStatus("Shutdown reply: " + message);
    clearServerTimeout();
  	});
	
	socket.on('3D-Scanner-error', errorHandler);
}

function sendCommandToServer(command, options) {
	if (communicationsTimeout == null) {
		communicationsTimeout = setTimeout(errorHandler, 1000, 
				"ERROR: Unable to communicate to HTTP server. Is nodeJS server running?");
	}
	socket.emit(command, options);
}

function appendStatus(line) {
  const $status = $("#status");
  const current = $status.text() || "";
  const next = (current ? current + "\n" : "") + line;
  $status.text(next);
}


function clearServerTimeout() {
	clearTimeout(communicationsTimeout);
	communicationsTimeout = null;
}

function errorHandler(message) {
	console.log("ERROR Handler: " + message);
	// Make linefeeds into <br> tag.
//	message = replaceAll(message, "\n", "<br/>");
	
	$('#error-text').html(message);	
	$('#error-box').show();
	
	// Hide it after a few seconds:
	window.clearTimeout(hideErrorTimeout);
	hideErrorTimeout = window.setTimeout(function() {$('#error-box').hide();}, 5000);
	clearServerTimeout();
}