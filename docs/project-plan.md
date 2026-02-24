# Project Plan – Bank Automat System

## 1. Goals

The goal of the project is to implement a functional ATM simulation using:

- Qt (C++)
- Node.js + Express
- MySQL

The application must:
- Support authentication
- Handle transactions
- Follow clean architecture
- Include full CRUD operations
- Provide documentation

## 2. Scope

### Included
- Login with card number and PIN
- Balance check
- Withdraw fixed amounts
- View last 10 transactions
- CRUD API for all database tables
- Documentation
- Weekly reporting

### Not Included
- Real payment integration
- GUI design polishing beyond functional requirements
- Multi-language support

## 3. Timeline (Example 6-week plan)

### Week 1
- Environment setup
- GitHub repository
- Database schema design

### Week 2
- Backend API basic setup
- Authentication
- Balance endpoint

### Week 3
- Withdraw logic
- Transactions endpoint
- CRUD endpoints

### Week 4
- Qt UI implementation
- API integration

### Week 5
- Error handling
- UI improvements
- Testing

### Week 6
- Documentation
- Final polishing
- Submission preparation

## 4. Work Distribution

This is a solo project.

Roles handled by one developer:

- Project Manager
- Backend Developer
- Database Designer
- Frontend Developer (Qt)
- Tester
- Documentation Writer

## 5. Risks

- Integration issues between Qt and API
- Database transaction logic errors
- Time constraints

Mitigation:
- Incremental testing
- Version control
- Early integration testing

## 6. Reverse Proxy (Extra Feature)

### Selected Solution: Nginx

**Decision:**  
The backend API (Node.js / Express) will be placed behind a reverse proxy using **Nginx**.

**Rationale:**
- Nginx is a widely used and well-documented reverse proxy solution.
- It is commonly used in production environments in front of Node/Express applications.
- Configuration is clear and flexible (proxy_pass, headers, timeouts, logging).
- Suitable for both local development and potential VPS deployment.
- Enables future support for TLS (e.g., Let's Encrypt) if the application is deployed publicly.

**Goal:**  
The API will be accessed through the reverse proxy (e.g., `http://localhost` → Nginx → `http://localhost:3000`).

This setup improves architectural clarity and simulates a real-world production deployment model.