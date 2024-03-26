'use strict';


/**
 * callback function from Google Authentication OAuth2.0
 * When logged in with Google, the email is added to the DataBase, and it automatically login to the site 
 *
 * code String code used to get token
 * no response value expected for this operation
 **/
exports.codeGET = function(code) {
  return new Promise(function(resolve, reject) {
    resolve();
  });
}


/**
 * search books
 * By passing in the appropriate options, you can search for available books in the system 
 *
 * title String pass an optional search string for looking up title in searched books (optional)
 * author String pass an optional search string for looking up author in searched books (optional)
 * returns List
 **/
exports.inSearchGET = function(title,author) {
  return new Promise(function(resolve, reject) {
    var examples = {};
    examples['application/json'] = [ {
  "books" : {
    "img" : "http://books.google.com/books/content?id=DnASAAAAYAAJ&printsec=frontcover&img=1&zoom=1&edge=curl&source=gbs_api",
    "link" : "https://play.google.com/store/books/details?id=DnASAAAAYAAJ&source=gbs_api",
    "title" : "Del possesso qual titolo di diritti ..."
  },
  "totalBooks" : 40
}, {
  "books" : {
    "img" : "http://books.google.com/books/content?id=DnASAAAAYAAJ&printsec=frontcover&img=1&zoom=1&edge=curl&source=gbs_api",
    "link" : "https://play.google.com/store/books/details?id=DnASAAAAYAAJ&source=gbs_api",
    "title" : "Del possesso qual titolo di diritti ..."
  },
  "totalBooks" : 40
} ];
    if (Object.keys(examples).length > 0) {
      resolve(examples[Object.keys(examples)[0]]);
    } else {
      resolve();
    }
  });
}


/**
 * login
 * log for user into site
 *
 * inputEmail String user's email (optional)
 * inputPassword String user's password (optional)
 * no response value expected for this operation
 **/
exports.loginPOST = function(inputEmail,inputPassword) {
  return new Promise(function(resolve, reject) {
    resolve();
  });
}


/**
 * registration
 * Adds user into DataBase
 *
 * inputEmail String user's email (optional)
 * inputPassword String user's password (optional)
 * no response value expected for this operation
 **/
exports.registerPOST = function(inputEmail,inputPassword) {
  return new Promise(function(resolve, reject) {
    resolve();
  });
}


/**
 * enter into search page
 * If user is logged, he can enter into search page, otherwise he redirected to login page 
 *
 * loggedin Boolean define if user is logged in or not (optional)
 * no response value expected for this operation
 **/
exports.searchGET = function(loggedin) {
  return new Promise(function(resolve, reject) {
    resolve();
  });
}

