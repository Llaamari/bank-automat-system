# Technical Specification – Bank Automat System

## 1. System Overview

The system is an ATM (Automated Teller Machine) application consisting of three main components:

- **Qt Desktop Application (C++ / Qt Widgets)**  
  Handles user interaction and communicates with the backend via HTTP.

- **REST API (Node.js + Express)**  
  Implements business logic and provides endpoints for the client.

- **MySQL / MariaDB Database**  
  Stores customer, account, card, and transaction data.

### High-Level Architecture

Qt Client → HTTP → Node.js / Express API → MySQL Database

## 2. Use Cases

### UC1 – User Authentication
- User enters card ID and PIN.
- Backend validates the PIN (hashed comparison).
- If correct → access granted.
- If incorrect → error message returned.

### UC2 – View Account Balance
- Authenticated user requests account balance.
- Backend retrieves balance from database.
- Current balance is displayed in the Qt client.

### UC3 – Withdraw Money
- User selects withdrawal amount (20 / 40 / 50 / 100 €).
- Backend validates sufficient balance.
- If valid:
  - Balance is updated.
  - Transaction is stored in database.
- Updated balance is returned to client.

### UC4 – View Last 10 Transactions
- User requests transaction history.
- Backend returns the 10 most recent transactions.
- Transactions are displayed in the Qt client.

## 3. Data Model (Initial Draft)

### Entities

#### Customer
- id (PK)
- first_name
- last_name
- address

#### Account
- id (PK)
- customer_id (FK)
- balance

#### Card
- id (PK)
- account_id (FK)
- pin_hash
- card_type (debit / credit)
- credit_limit (0 for debit cards)

#### Transaction
- id (PK)
- account_id (FK)
- amount
- type (withdrawal)
- timestamp

## 4. Database Design Considerations

- One customer can have multiple accounts.
- One account has exactly one owner (minimum requirement).
- A customer may have multiple cards.
- A card is linked to one account (minimum implementation).
- PIN codes are stored hashed using bcrypt.
- Transactions are stored in a separate table.

Possible extension:
- Many-to-many relationship between cards and accounts (for dual debit/credit cards).
- Account access rights for multiple users.

## 5. REST API Design (Draft)

### Authentication

POST /auth/login  
Body:
{
  "cardId": "string",
  "pin": "string"
}

Response:
- 200 OK (authenticated)
- 401 Unauthorized

### Account

GET /accounts/:id/balance  
Returns account balance.

POST /accounts/:id/withdraw  
Body:
{
  "amount": 50
}

Returns updated balance.

GET /accounts/:id/transactions?limit=10  
Returns last 10 transactions.

## 6. Security

- PIN codes are stored hashed (bcrypt).
- No plaintext PIN storage.
- Backend validates all business rules.
- Input validation is implemented on the API layer.
- Sensitive configuration (e.g., database credentials) is stored in environment variables (.env).

## 7. Technology Stack

- Qt 6 (C++)
- Node.js
- Express.js
- MySQL / MariaDB
- Git & GitHub
- Postman (API testing)

## 8. Future Extensions (Optional for Higher Grades)

- Credit card support with credit limits
- Card lock after three failed PIN attempts
- 30-second inactivity timeout
- Transaction pagination
- Dual debit/credit card support
- Swagger API documentation
- Dockerized deployment
- CI/CD pipeline