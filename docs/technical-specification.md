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

## 9. Reverse Proxy Logging (Nginx)

Nginx access and error logging was enabled to support debugging and operational visibility.

- Access log: `logs/access.log`
- Error log: `logs/error.log`

**Example access log entry (GET /api/health):**
```text
127.0.0.1 - - [25/Feb/2026:12:34:56 +0200] "GET /api/health HTTP/1.1" 200 27 "-" "PostmanRuntime/7.36.0" rt=0.012 uct=0.001 urt=0.010 uaddr=127.0.0.1:3000 ustatus=200
```
Example error log entry (backend down, 502):
```text
[error] ... connect() failed (...) while connecting to upstream, client: 127.0.0.1, server: localhost, request: "GET /api/health HTTP/1.1", upstream: "http://127.0.0.1:3000/health", host: "localhost"
```

##  10. Demo: API Behind Reverse Proxy (Nginx)

### Objective

Demonstrate that the backend API runs behind a reverse proxy (Nginx) and that all routing works correctly through the proxy entrypoint.

### Architecture Overview (Demo Explanation)

In development, the system uses the following architecture:

Client (Browser / Postman / Qt)
→ http://localhost (Nginx reverse proxy)
→ http://localhost:3000 (Node.js / Express backend)
→ MySQL database

The reverse proxy:

- Serves a static landing page at `/`
- Forwards `/api/*` to the backend
- Strips the `/api` prefix before forwarding
- Adds standard proxy headers (X-Forwarded-For, X-Forwarded-Proto)
- Logs access and error events

### Step 1 – Show Reverse Proxy Entry Point

Open browser:

http://localhost

Show that:
- Nginx is running
- Static landing page is served
- Backend is NOT directly exposed on port 80

### Step 2 – Show API Through Proxy

Call:

GET http://localhost/api/health

Explain:
- Request goes to Nginx
- Nginx forwards to backend (localhost:3000)
- Backend returns JSON response
- Response is returned through proxy

Expected result:
200 OK

### Step 3 – Authentication Test

POST http://localhost/api/auth/login

Body:
{
  "cardNumber": "...",
  "pin": "...."
}

Show:
- Successful login returns accountId
- Incorrect PIN returns 401
- Card status is validated

### Step 4 – Account Operations

Demonstrate:

GET /api/accounts/{id}/balance  
POST /api/accounts/{id}/withdraw  
GET /api/accounts/{id}/transactions?limit=10  

Explain:
- All routes function through proxy
- No backend route changes were required
- Business logic is handled by backend
- Proxy only handles routing and infrastructure

### Step 5 – Failure Scenario (Backend Down)

Stop backend.

Call:
GET /api/health

Show:
- Nginx returns 502 Bad Gateway
- Error is logged in error.log
- Proxy fails safely

Restart backend and show recovery.

### Step 6 – Logging Demonstration

Open access.log and show:

- Client IP
- Request path
- HTTP status
- Upstream address
- Upstream response time

Explain:
- Logging enables debugging and monitoring
- Proxy and backend responsibilities are separated

### Conclusion

The API runs successfully behind a reverse proxy.

✔ Routing works via /api/*  
✔ Backend is isolated behind Nginx  
✔ Errors are handled predictably  
✔ Logging is enabled  
✔ Architecture reflects a production-style deployment model  

The implementation was tested using Postman and verified through proxy logs.