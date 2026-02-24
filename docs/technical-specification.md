# Technical Specification – Bank Automat System

## 1. System Overview

The Bank Automat System is a desktop-based ATM simulation consisting of:

- **Qt 6 Desktop Application (C++ / Qt Widgets)**
- **Node.js + Express REST API**
- **MySQL Database**

The system allows users to:
- Log in using card number and PIN
- View account balance
- Withdraw fixed amounts
- View last 10 transactions

## 2. Architecture

### High-Level Architecture

Qt Client → HTTP (JSON) → Express API → MySQL Database

### Components

#### Qt Application
- UI implemented with Qt Widgets
- Uses QNetworkAccessManager for REST communication
- Session state: currentAccountId

#### Backend API
- Node.js + Express
- MySQL connection pool (mysql2)
- bcrypt for PIN hashing
- REST endpoints

#### Database
- Relational schema
- Primary/foreign keys
- PIN stored hashed

## 3. Use Cases

### UC1 – Login
User enters card number and PIN.
System validates PIN (bcrypt).
If valid → access granted.
If invalid → error shown.

### UC2 – View Balance
User requests account balance.
System returns current balance.

### UC3 – Withdraw Money
User selects amount (20, 40, 50, 100).
System checks sufficient funds.
Balance updated.
Transaction stored.

### UC4 – View Transactions
User views last 10 transactions.
System returns ordered list (DESC by date).

## 4. Data Model

### Tables

#### customers
- id (PK)
- first_name
- last_name
- address
- created_at

#### accounts
- id (PK)
- customer_id (FK)
- account_type (debit/credit)
- balance
- credit_limit
- is_locked
- created_at

#### cards
- id (PK)
- card_number (UNIQUE)
- customer_id (FK)
- pin_hash
- status (active/locked)
- created_at

#### transactions
- id (PK)
- account_id (FK)
- amount
- tx_type
- created_at

#### card_accounts
- card_id (FK)
- account_id (FK)
- role (debit/credit)
- PRIMARY KEY (card_id, account_id)

## 5. Security

- PIN codes stored hashed (bcrypt)
- No plaintext PIN storage
- Input validation in API layer
- Environment variables for DB credentials
- HTTP status codes used for error handling

## 6. API Endpoints

POST /auth/login  
GET /accounts/:id/balance  
POST /accounts/:id/withdraw  
GET /accounts/:id/transactions?limit=10  

CRUD endpoints for all tables under /crud/...

## 7. Non-Functional Requirements

- Responsive UI
- Clear error handling
- Atomic database transactions
- Clean code structure
- Git version control

## 8. Future Improvements

- Card lock after 3 failed PIN attempts
- Inactivity timeout
- Credit account logic
- Docker deployment
- Automated testing