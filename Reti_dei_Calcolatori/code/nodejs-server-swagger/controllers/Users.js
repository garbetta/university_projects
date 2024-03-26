'use strict';

var utils = require('../utils/writer.js');
var Users = require('../service/UsersService');

module.exports.codeGET = function codeGET (req, res, next) {
  var code = req.swagger.params['code'].value;
  Users.codeGET(code)
    .then(function (response) {
      utils.writeJson(res, response);
    })
    .catch(function (response) {
      utils.writeJson(res, response);
    });
};

module.exports.inSearchGET = function inSearchGET (req, res, next) {
  var title = req.swagger.params['title'].value;
  var author = req.swagger.params['author'].value;
  Users.inSearchGET(title,author)
    .then(function (response) {
      utils.writeJson(res, response);
    })
    .catch(function (response) {
      utils.writeJson(res, response);
    });
};

module.exports.loginPOST = function loginPOST (req, res, next) {
  var inputEmail = req.swagger.params['inputEmail'].value;
  var inputPassword = req.swagger.params['inputPassword'].value;
  Users.loginPOST(inputEmail,inputPassword)
    .then(function (response) {
      utils.writeJson(res, response);
    })
    .catch(function (response) {
      utils.writeJson(res, response);
    });
};

module.exports.registerPOST = function registerPOST (req, res, next) {
  var inputEmail = req.swagger.params['inputEmail'].value;
  var inputPassword = req.swagger.params['inputPassword'].value;
  Users.registerPOST(inputEmail,inputPassword)
    .then(function (response) {
      utils.writeJson(res, response);
    })
    .catch(function (response) {
      utils.writeJson(res, response);
    });
};

module.exports.searchGET = function searchGET (req, res, next) {
  var loggedin = req.swagger.params['loggedin'].value;
  Users.searchGET(loggedin)
    .then(function (response) {
      utils.writeJson(res, response);
    })
    .catch(function (response) {
      utils.writeJson(res, response);
    });
};
