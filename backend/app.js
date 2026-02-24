var express = require('express');
var path = require('path');
var cookieParser = require('cookie-parser');
var logger = require('morgan');

var indexRouter = require('./routes/index');
const healthRouter = require('./routes/health');
const accountsRouter = require('./routes/accounts');
const authRouter = require('./routes/auth');
const crudCustomers = require('./routes/crud/customers');
const crudAccounts = require('./routes/crud/accounts');
const crudCards = require('./routes/crud/cards');
const crudTransactions = require('./routes/crud/transactions');
const crudCardAccounts = require('./routes/crud/card_accounts');

var app = express();

app.use(logger('dev'));
app.use(express.json());
app.use(express.urlencoded({ extended: false }));
app.use(cookieParser());
app.use(express.static(path.join(__dirname, 'public')));

app.use('/', indexRouter);
app.use('/health', healthRouter);
app.use('/accounts', accountsRouter);
app.use('/auth', authRouter);
app.use('/crud/customers', crudCustomers);
app.use('/crud/accounts', crudAccounts);
app.use('/crud/cards', crudCards);
app.use('/crud/transactions', crudTransactions);
app.use('/crud/card-accounts', crudCardAccounts);

module.exports = app;
