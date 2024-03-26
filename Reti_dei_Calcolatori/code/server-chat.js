var webSocketServer = require('websocket').server;
var http = require('http');

// Porta websocket server
var webSocketsServerPort = 1337;

// Var locali
var history = [ ]; // salva 100 messaggi max
var clients = [ ]; // lista users connessi

// Gestisce caratteri speciali di HTML 
function htmlEntities(str) {
  return String(str)
      .replace(/&/g, '&amp;').replace(/</g, '&lt;')
      .replace(/>/g, '&gt;').replace(/"/g, '&quot;');
}

// Array colori usati per i nomi utente della chat
var colors = [ 'red', 'green', 'blue', 'magenta', 'purple', 'plum', 'orange' ];
// Assegnazione random 
colors.sort(function(a,b) { return Math.random() > 0.5; } );

// HTTP server
var server = http.createServer(function(request, response) {});
// Apertura server
var tempServer = server.listen(webSocketsServerPort, function() {
    var host = server.address().address;
    var port = server.address().port;

    console.log((new Date()) + " Server is listening on port "
        + webSocketsServerPort + " -> IP: " + host +":" + port);
});

// WebSocket server
var wsServer = new webSocketServer({
  httpServer: server
});

// Gestione richiesta connessione utente
wsServer.on('request', function(request) {
  console.log((new Date()) + ' Connection from origin '
      + request.origin + '.');
  // Accetta connessione 
  var connection = request.accept(null, request.origin); 

  // we need to know client index to remove them on 'close' event
  var index = clients.push(connection) - 1;

  var userName = false;
  var userColor = false;
  console.log((new Date()) + ' Connection accepted.');
  // visualizza chat già presente
  if (history.length > 0) {
    connection.sendUTF(
        JSON.stringify({ type: 'history', data: history} ));
  }

  // Gestione messaggi utente
  connection.on('message', function(message) {
    if (message.type === 'utf8') {
    // Primo messaggio utente --> username
     if (userName === false) {
        userName = htmlEntities(message.utf8Data);
        // Assegnamo colore random  e lo restituiamo all'utente
        userColor = colors.shift();
        connection.sendUTF(
            JSON.stringify({ type:'color', data: userColor }));
        console.log((new Date()) + ' User is known as: ' + userName
                    + ' with ' + userColor + ' color.');
      } else { // utente già presente nella chat
        console.log((new Date()) + ' Received Message from '
                    + userName + ': ' + message.utf8Data);
        
        // memorizzazione messaggi
        var obj = {
          time: (new Date()).getTime(),
          text: htmlEntities(message.utf8Data),
          author: userName,
          color: userColor
        };

        history.push(obj);
        history = history.slice(-100);

        // invio messaggi a tutti gli utenti connessi
        var json = JSON.stringify({ type:'message', data: obj });
        for (var i=0; i < clients.length; i++) {
          clients[i].sendUTF(json);
        }
      }
    }
  });

  // Disconnessione utente
  connection.on('close', function(connection) {
    if (userName !== false && userColor !== false) {
      console.log((new Date()) + " Peer "
          + connection.remoteAddress + " disconnected.");
      
      // rimozione utente dall'array clients
      clients.splice(index, 1);
      colors.push(userColor);
    }
  });
});