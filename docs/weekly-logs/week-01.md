# Week 1 Report

## What was done
- Set up GitHub repository
- Installed Node.js, MySQL, Qt
- Designed initial database schema

## Problems encountered
- PATH issue with MySQL

## Solutions
- Added MySQL to environment variables

## Hours spent
8 hours

## Next week plan
- Implement backend authentication

## Reverse Proxy Routing Test Report

**Date:** 2026-02-24  
**Environment:** Local development  
**Reverse proxy:** Nginx  
**Entry point:** http://localhost (API via /api/*)  
**Backend upstream:** http://localhost:3000  

### Summary
All required API endpoints were successfully tested through the reverse proxy using Postman.
The `/api` prefix is handled by Nginx and mapped to backend routes without changing backend route paths.

### Tests (Postman via Reverse Proxy)

1) **GET /api/health**
- Request: `GET http://localhost/api/health`
- Result: ✅ 200 OK (JSON status ok)

2) **POST /api/auth/login**
- Request: `POST http://localhost/api/auth/login`
- Body: `{ "cardNumber": "...", "pin": "...." }`
- Result: ✅ 200 OK → `{ ok: true, accountId: <id> }` (valid credentials)
- Notes: Wrong PIN returns 401 `{ ok: false }` as expected.

3) **GET /api/accounts/:id/balance**
- Request: `GET http://localhost/api/accounts/{accountId}/balance`
- Result: ✅ 200 OK (balance returned)

4) **POST /api/accounts/:id/withdraw**
- Request: `POST http://localhost/api/accounts/{accountId}/withdraw`
- Body: `{ "amount": 50 }`
- Result: ✅ 200 OK (withdrawal succeeded, balance updated)
- Notes: Allowed amounts enforced (20/40/50/100); insufficient funds returns 400.

5) **GET /api/accounts/:id/transactions?limit=10**
- Request: `GET http://localhost/api/accounts/{accountId}/transactions?limit=10`
- Result: ✅ 200 OK (latest transactions returned)

### Conclusion
Reverse proxy routing works correctly:
- `/` serves a static page (optional)
- `/api/*` is forwarded to backend routes
- Required endpoints function through the proxy without backend route changes.