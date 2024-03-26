require('dotenv').config();

var express = require('express');
var session = require('express-session');
var app = express();
var request2server = require('request');
var bodyparser = require('body-parser');
const {
    Client
} = require('pg');

const cors = require("cors");
var path = require("path");

app.use(cors());

app.use(express.json()); // per gestire file JSON
app.use(express.urlencoded({
    extended: true
})) // per gestire URL provenienti dal browser

//  Var locali
var port = 8888;
var code = "";
var token = "";
var client_id = process.env.ID_APP_G;
var client_secret = process.env.CLIENT_SECRET_G;

/**************CONTROLLO LOG UTENTE**************/
app.use(session({
    secret: 'secret',
    resave: true,
    saveUninitialized: true
}));

app.use('/public', express.static(__dirname + '/public'));

//  Accesso alla pagina di ricerca - verifica se utente loggato
app.get("/search", function (req, res) {
    console.log("Richiesta pagina di ricerca");
    if (!req.session.loggedin) {
        res.redirect("http://localhost:8888/login");
        return;
    } else {
        res.sendFile('./request.html', { root: __dirname });
    }
    console.log();
});

/**************PAGINE DI ERRORE**************/
app.get("/error_register", function(req, res){
    res.sendFile('./error_register.html', { root: __dirname });
})

app.get("/error_general", function(req, res){
    res.sendFile('./error_general.html', { root: __dirname + '/public/ErrorPages' });
});

/**************RICHIESTE PAGINE PRINCIPALI**************/
app.get("/homePage", function (req, res) {
    res.sendFile('./homepage.html', { root: __dirname + '/public/Homepage' });
});

app.get("/login", function (req, res) {
    res.sendFile('./login.html', { root: __dirname + '/public/Login' });
});

app.get("/", function (req, res) {
    res.redirect("http://localhost:8888/homePage");
});

/**************Ricerca libro (JSON) tramite GOOGLE BOOKS**************/
app.get('/inSearch', function (req, res) {

    //  Ricezione dei dati
    var quer = "";
    var data = {};

    if (req.query.title)
        quer += ("+intitle:" + req.query.title);
    if (req.query.author)
        quer += ("+inauthor:" + req.query.author);
    console.log("Ricercando Libri per: " + quer);

    request2server({
        url: 'https://www.googleapis.com/books/v1/volumes?q=' + quer + '&maxResults=40&printType=books&key=' + process.env.BOOK_KEY,
        method: 'GET',
    }, function (error, response, body) {
        if (error) console.log(error);
        else {
            var jsonBody = JSON.parse(body);

            //  Caso in cui libri trovati
            if (jsonBody.totalItems > 0) {
                data["totalBooks"] = 0;
                data["books"] = [];
                console.log("Libri trovati: " + jsonBody.totalItems);
                console.log();
                if (jsonBody.totalItems < 40)
                    var l = jsonBody.totalItems;
                else
                    var l = 40;
                for (var i = 0; i < l; i++) {
                    //inseriemento libri nella risposta JSON
                    if (jsonBody.items[i].volumeInfo.hasOwnProperty("imageLinks")) {
                        var title = jsonBody.items[i].volumeInfo.title;
                        var link = jsonBody.items[i].volumeInfo.infoLink;
                        var img = jsonBody.items[i].volumeInfo.imageLinks.thumbnail;
                        data["books"].push({
                            title: title,
                            link: link,
                            img: img
                        });
                        data["totalBooks"]++;
                    }
                }
                res.status(200);
                res.json(data);
            } else { //  Caso in cui libri non trovati
                console.log("Libri non trovati");
                data["totalBooks"] = 0;
                res.status(200);
                res.json(data);
            }
        }
    });
});

/**************LOGIN CON GOOGLE (Autorizathion Code Grant)**************/
//  Callback (Get Token from Code)
app.get('/code', function (req, res) {

    //  Var iniziali
    code = req.query.code;
    var url = 'https://www.googleapis.com/oauth2/v3/token';
    var headers = {
        'Content-Type': 'application/x-www-form-urlencoded'
    };
    var body = "code=" + code + "&client_id=" + client_id + "&client_secret=" + client_secret + "&redirect_uri=http%3A%2F%2Flocalhost%3A8888%2Fcode&grant_type=authorization_code";

    //  Chiamata API: Google OAuth 2 (per ottenere il token)
    request2server.post({

        //informazioni che invio all’Authorization Server
        headers: headers,
        url: url,
        body: body

    }, function (error, response, body) {

        //  Pagina dopo aver ottenuto il token
        my_obj = JSON.parse(body);
        token = my_obj.access_token;
        console.log("\n");

        //  Richiesta per la email dell'utente appena loggato
        request2server.get({

            //informazioni che invio all’Authorization Server
            headers: headers,
            url: "https://www.googleapis.com/oauth2/v1/userinfo?access_token=" + token,

        }, function (error, response, body) {

            my_obj = JSON.parse(body);
            email = my_obj.email;
            console.log("Email utente loggato: " + email);

            req.session.loggedin = true;
            req.session.email = email;
            console.log("Session: log_" + req.session.loggedin + " <> user_" + req.session.email);
            res.redirect('http://localhost:8888/Search');

            // Inserimento email utente in DB
            const client = new Client({
                user: 'postgres',
                host: process.env.HOST,
                database: process.env.DATABASE,
                password: process.env.PASSWORD,
                port: process.env.PORT,
            });

            //  Connessione a database Postgres
            client.connect(function (err) {
                if (err) {
                    console.error('Connessione non stabilita - error: ', err.stack)
                } else {
                    console.log('Connessione al DB stabilita')
                }
            });

            //  Inserimento email nel database
            const query2 = "INSERT INTO users (email, password) SELECT * FROM (SELECT '" + email + "', 'null') AS Tmp WHERE NOT EXISTS (SELECT email FROM users WHERE email = '" + email + "') LIMIT 1;"
            client.query(query2, function (err, res) {
                if (err) {
                    console.error("Inserimento fallito: " + err);
                    return;
                } else
                    console.error("Inserimento avvenuto con successo: " + email);
                client.end();
            });
        });
    });
});

/**************REGISTRAZIONE LOCALE**************/
app.post("/register", function (req, result) {
    console.log("Registrazione locale");
    var isRegister = false;

    // Inserimento email utente in DB
    const client = new Client({
        user: 'postgres',
        host: process.env.HOST,
        database: process.env.DATABASE,
        password: process.env.PASSWORD,
        port: process.env.PORT,
    });

    client.connect(function (err) {
        if (err) {
            console.log('Connessione non stabilita - error: ', err.stack);
            return;
        } else {
            console.log('Connessione al DB stabilita');
        }
    });

    const query3 = "SELECT * FROM users WHERE email='" + req.body.inputEmail + "'"
    client.query(query3, function (err, res) {
        if (err) { //  Errore durante la query
            console.log("Inserimento [1] fallito: " + err);
            result.sendFile('./error_general.html', { root: __dirname + '/public/ErrorPages' });
            client.end();
            return;
        } else if (res.rowCount > 0) { //  Utente già registrato
            console.log("Utente già registrato");
            result.sendFile('./error_register.html', { root: __dirname + '/public/ErrorPages' });
            client.end();
            return;
        } else { //  Utente ancora non presente nel database
            const query4 = "INSERT INTO users VALUES ('" + req.body.inputEmail + "','" + req.body.inputPassword + "')";
            client.query(query4, function (err, res) {
                if (err) { //  Errore durante la query
                    console.log("Inserimento [2] fallito: " + err);
                    result.sendFile('./error_general.html', { root: __dirname + '/public/ErrorPages' });
                    return;
                } else { //  Inserimento avvenuto con successo
                    console.log("Inserimento avvenuto con successo: " + req.body.inputEmail);
                    // Login automatico
                    req.session.loggedin = true;
                    req.session.email = req.body.inputEmail;
                    console.log("Session: log_" + req.session.loggedin + " <> user_" + req.session.email);
                    result.redirect('http://localhost:8888/Search');
                    return;
                }
                client.end();
            });
        }
    });

    console.log("email: " + req.body.inputEmail);
    console.log("password: " + req.body.inputPassword);
});


/**************LOGIN LOCALE**************/
app.post("/login", function (req, result) {

    console.log("Login locale");
    const client = new Client({
        user: 'postgres',
        host: process.env.HOST,
        database: process.env.DATABASE,
        password: process.env.PASSWORD,
        port: process.env.PORT,
    });

    client.connect(function (err) {
        if (err) {
            console.log('Connessione non stabilita - error: ', err.stack);
            return;
        } else {
            console.log('Connessione al DB stabilita');
        }
    });

    const query3 = "SELECT * FROM users WHERE email='" + req.body.inputEmail + "' and password='" + req.body.inputPassword + "'";
    client.query(query3, function (err, res) {
        if (err) {
            console.log("Ricerca in DB fallita: " + err);
            result.sendFile('./error_general.html', { root: __dirname + '/public/ErrorPages' });
            client.end();
            return;
        } else if (res.rowCount > 0) { 
            console.log("Utente trovato");
            req.session.loggedin = true;
            req.session.email = req.body.inputEmail;
            console.log("Session: log_" + req.session.loggedin + " <> user_" + req.session.email);
            result.redirect('http://localhost:8888/Search');
            client.end();
            return;
        } else {
            console.log("Utente non trovato");
            result.sendFile('./error_login.html', { root: __dirname + '/public/ErrorPages' });
        }
    });
});

/**************LOGOUT**************/
app.get("/logout", function(req,res){
    if (req.session.loggedin) {
        req.session.loggedin = false;
    };
    res.redirect('http://localhost:8888/');
});

var server = app.listen(port, function () {
    var host = server.address().address;
    var port = server.address().port;

    console.log('Server in ascolto su http://%s:%s', host, port);
});
