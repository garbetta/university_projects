$(document).ready(function () { 

    // Associa variabili a elementi HTML
    var content = $('#content');
    var input = $('#input');
    var status = $('#status');  // colore assegnato dal server
    var myColor = false;

    // invio nome al server
    var myName = false; 

    // Controllo supporto WebSocket
    window.WebSocket = window.WebSocket || window.MozWebSocket;
    if (!window.WebSocket) {
        content.html($('<p>',
            { text:'Sorry, but your browser doesn\'t support WebSocket.'}
        ));
        input.hide();
        $('span').hide();
        return;
    }  
    
    // Apertura Connessione 
    var connection = new WebSocket('ws://localhost:1337');  

    // Assegnazione nome
    connection.onopen = function () {
        if(sessionStorage.chatName){
            connection.send(sessionStorage.chatName);
            myName = sessionStorage.chatName;
        } else {
            status.text('Choose name:');
        }

        // dopo la connessione con il server è possibile scrivere
        input.removeAttr('disabled');
    };  

    // se ci sono problemi con la connesisone
    connection.onerror = function (error) {
        content.html($('<p>', {
            text: 'Sorry, but there\'s some problem with your '
            + 'connection or the server is down.\n'
        }));
    };
    
    // Gestione messaggi in entrata dal server
    connection.onmessage = function (message) {
        try {
            var json = JSON.parse(message.data);
        } catch (e) {
            console.log('Invalid JSON: ', message.data);
            return;
        }
        // prima risposta --> colore
        if (json.type === 'color') { 
            myColor = json.data;
            status.text(myName + ': ').css('color', myColor);
            input.removeAttr('disabled').focus();
            // da qui è possibile scrivere

        } else if (json.type === 'history') { // intera cronologia messaggi
            // inserimento cronologia nella chat
            for (var i=0; i < json.data.length; i++) {
            //funzione definita in seguito
            addMessage(json.data[i].author, json.data[i].text,
                json.data[i].color, new Date(json.data[i].time));
            }
        } else if (json.type === 'message') { // messaggio singolo
            input.removeAttr('disabled'); 
            addMessage(json.data.author, json.data.text,
                    json.data.color, new Date(json.data.time));
        } else {
            console.log('Hmm..., I\'ve never seen JSON like this:', json);
        }
    };  
    
    // Invio messaggio con Invio
    input.keydown(function(e) {
        if (e.keyCode === 13) {
            var msg = $(this).val();
            if (!msg) {
                return;
            }
            // invio msg 
            connection.send(msg);
            $(this).val('');
            // disabilito input finchè non ricevo risposta dal server
            input.attr('disabled', 'disabled');
            if (myName === false) {
                if (msg) {
                    myName = msg;
                    sessionStorage.chatName = msg;
                }
                else {
                    myName = sessionStorage.chatName;
                }
            }
            else {
                myName = sessionStorage.chatName;
            }
        }
    });  

    // Funzione timeout risposta server 
    setInterval(function() {
        if (connection.readyState !== 1) {
            status.text('Error');
        }
    }, 3000);  
    
    // Inserimento messaggio nella chat
    function addMessage(author, message, color, dt) {
        content.prepend('<p><span style="color:' + color + '">'
            + author + '</span> @ ' + (dt.getHours() < 10 ? '0'
            + dt.getHours() : dt.getHours()) + ':'
            + (dt.getMinutes() < 10
                ? '0' + dt.getMinutes() : dt.getMinutes())
            + ': ' + message + '</p>');
        }
    });