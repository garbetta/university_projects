# BookShell 
## Progetto Reti di Calcolatori 2020

#### Requisiti
1. Il servizio Rest che implementiamo (lo chiameremo SERV) deve offrire delle API documentate
2. SERV si deve interfacciare con almeno due servizi Rest "esterni", non su localhost
3. Almeno uno di questi servizi deve essere "commerciale"
4. Almeno uno dei servizi Rest esterni deve richiedere oAuth
5. Si devono usare WebSocket e/o AMQP
6. Il progetto deve essere su GIT e documentato con un README
7. Le API REST implementate in SERV devono essere documentate su un GIT e devono essere validate con un caso di test

#### Tecnologie utilizzate

REST 1: Google Books

REST 2: Google (OAuth2.0)

WebSocket: Chat
Database: Postgres

Il back-end è scritto in Nodejs e si occupa di gestire la ricerca dei libri, il login, la chat, mentre il front-end è scritto in html, con metodi JS, Ajax, JQuery, e si occupa principalmente della parte grafica e degli script ad essa legati

#### Descrizione progettuale

La nostra applicazione web BookShell permette, dopo essersi loggati con un account Google, o tramite diretta registrazione al sito, di ricercare libri accedendo al catalogo messo a disposizione da Google stesso. Prima di effettuare la ricerca è messa a disposizione una chat dove gli utenti possono comunicare e scambiarsi consigli e pareri sulle prossime letture da fare o semplicemente creare un punto di ritrovo tra bibliofili. 
Si può effettuare la ricerca inserendo solamente il titolo o l'autore del libro desiderato, così come entrambi, in modo che sia più accurata. Cliccando sulla copertina del libro d'interesse si viene rediretti sul sito di Google ove si avrà la disponibilità del libro, l'edizione, la possibilità di scaricare l'ebook, leggere l'anteprima o semplicemente comprarlo.
 
#### Funzionamento
    
Si avvia inizialmente request.js, che farà partire il Server API in ascolto sulla porta 8888, server-chat.js che apre il websocket per poter permettere il corretto funzionamento della chat, e il DataBase postgres su PgAdmin. Scrivendo nel browser http://localhost:8888 verrà aperta la pagina iniziale del sito che contiene un'introduzione allo stesso. Vengono messi a disposizione vari collegamenti, tra cui i principali al login e alla pagina di ricerca. Se si clicca sul collegamento alla pagina di ricerca, si viene inizialmente reindirizzati alla pagina di login in quanto per poter usufruire delle funzionalità del sito occorre effettuare una registrazione. La pagina di login offre una funzione di registrazione nel caso in cui l'utente non lo abbia mai fatto (al termine della quale si sarà loggati), oppure una funzione di login che attraverso un collegamento al DataBase (Postegres) controlla che i dati inseriti dall'utente (email e password) siano presenti nello stesso e corrispondano con la coppia di dati inserita in fase di registrazione. In caso l'utente provi ad effettuare la registrazione manualmente, ma risulti essere già registrato, viene aperta una pagina di errore con un avviso relativo. In caso l'utente inserisca i dati sbagliati effettuando il login, viene aperta una pagina di errore con un avviso relativo. In alternativa, viene offerta la possibilità di loggarsi direttamente tramite Google OAuth. In fase di registrazione, viene prelevata in tutti i casi la mail dell'utente e viene settata una variabile che permette di instaurare una sessione in modo da poter verificare nella pagina di ricerca che l'utente sia effettivamente loggato. Di conseguenza, se si prova ad aprire direttamente la pagina di ricerca libri senza essere loggati, il sito riconoscerà questa mancanza e reindirizzerà l'utente alla pagina di login.
Una volta effettuato il login si aprirà quindi la pagina di ricerca request.html dove viene messa a disposizione una form e una chat di discussione subito dopo. Nella form si può inserire il titolo e l'autore del libro da ricercare, che vengono poi passati come parametri nella richiesta al servizio di Google Book, a cui vengono aggiunti anche altri due parametri, cioè il numero massimo di libri che è possibile visualizzare nei risultati, e un parametro che serve a restituire solo libri veri e propri (no riviste, no magazine...). La risposta alla request, che consiste in una lista visiva delle copertine dei libri corrispondenti in caso esistano, viene visualizzata dinamicamente subito sotto la form, facendo scalare la chat verso il basso. 
Come ultima funzionalità viene offerta una chat di discussione, che tramite WebSocket permette a tutti gli utenti presenti sulla pagina di ricerca di scegliere un nome, a cui verrà assegnato un colore randomico, e iniziare a parlare fra di loro.
Le API fornite e documentate tramite Swagger si possono accedere su http://localhost:8080/docs scrivendo "node index.js" su terminale dalla cartella node-server-swagger.

#### API documentazione

API LIBRI 
- https://developers.google.com/books/docs/v1/using
- https://developers.google.com/books/docs/v1/reference/volumes

API OAuth 
- https://developers.google.com/identity/protocols/oauth2/web-server

#### Come avviare il progetto

Per scaricare il progetto si può clonare la repository git con il comando git clone https://github.com/GrAnDozeN/ProgettoRdC

I moduli necessari all'applicazione sono:

•   npm install express

•   npm install request

•   npm install body-parser

•   npm install dotenv

•   npm install pg

•   npm install express-session

•   npm install websocket

•   npm install http

•   npm install cors

url server : http://localhost:8888

WEBSOCKET

Il server back-end mette a disposizione una WebSocket per implementare l'uso di una chat. 
La WebSocket è disponibile all'URL ws://localhost:1337

### Sviluppatori:

•   Federico Montanari 1762065

•   Alessandro Garbetta 1785139

•   Christian Realin Ramilo 1793265
