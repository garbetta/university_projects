API REST

• GOOGLE BOOKS

– Richiesta HTTP GET https://www.googleapis.com/books/v1/volumes

Ottengo i libri ricercati

Parametri:

printType: books
maxResults: '40'
key: chiave privata (required)
inTitle: titolo libro
inAuthor: autore libro

Documentazione ufficiale:
https://developers.google.com/books/docs/v1/using
https://developers.google.com/books/docs/v1/reference/volumes


• GOOGLE OAUTH 2.0

– Richiesta HTTP GET https://accounts.google.com/o/oauth2/auth

Ottengo l'email dell'utente da inserire nel database come registrazione dell'utente

Parametri:

client_id: Client ID dell'applicazione (required)
scope: userinfo.email, per poter accedere all'email dell'utente (required)
redirect_uri: URL di callback (required)
response_type: Tipo di risposta (required)

– Richiesta HTTP GET https://www.googleapis.com/oauth2/v3/token

Ottengo il Token per accedere alle informazioni dell'utente tramite Codice ricevuto da Google dopo l'autenticazione dell'utente stesso

Parametri:

code: Codice ricevuto dopo l'autenticazione dell'utente su Google (required)
client_id: Client ID dell'applicazione  (required)
client_secret: Segreto dell'applicazione (required)
redirect_uri: URL di callback (required)
grant_type: tipo di autorizzazione (required)

– Richiesta HTTP GET https://www.googleapis.com/oauth2/v1/userinfo?access_token= + access_token

Ottengo l'email dell'utente loggato tramite Token

Parametri:

access_token: Token preso alla chiamata precedente (required)