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

### Target Environment

**Primary environment:** Local development environment  
**Optional extension:** VPS deployment (future enhancement)

The reverse proxy (Nginx) will initially be configured and tested in a local development environment.

**Local setup architecture:**

Client → Nginx (localhost) → Node.js/Express API (localhost:3000) → MySQL (localhost)

This approach allows:
- Full testing of reverse proxy functionality
- Demonstration of production-like architecture
- Controlled debugging without network-related variables

VPS deployment is considered a possible future extension. If implemented later, the same Nginx configuration can be adapted for a remote Linux server, including optional TLS configuration.

Docker is not included in the initial scope in order to keep the infrastructure simple and focused on reverse proxy implementation.

### Development Domain / Hostname

The development environment will use:

**Hostname:** localhost  
**Reverse proxy port:** 80  
**Backend internal port:** 3000

The API will be accessed through:

http://localhost

Nginx will listen on port 80 and forward requests to:

http://localhost:3000

This approach:
- Avoids modifying the system hosts file
- Keeps the setup simple and portable
- Simulates a real production-like reverse proxy architecture
- Works consistently across operating systems

If a VPS deployment is implemented later, a proper domain name can be configured.

### Reverse Proxy Routing Design

**Selected URL model (development):**
- `http://localhost/` serves an optional static landing / API info page (from Nginx).
- `http://localhost/api/*` is forwarded to the backend API running at `http://localhost:3000`.

**Routing rule:**
Nginx maps `/api/<path>` → backend `/<path>`.

**Examples:**
- `GET /api/health` → backend `GET /health`
- `POST /api/auth/login` → backend `POST /auth/login`
- `GET /api/accounts/1/balance` → backend `GET /accounts/1/balance`

This keeps the reverse proxy entrypoint clean and production-like, while clearly separating static content (`/`) from API traffic (`/api/*`).

### TLS / HTTPS Plan (Optional)

**Decision:**
- **Local development:** TLS is not used. The system runs over plain HTTP to keep the setup simple and frictionless during development.
- **Optional VPS deployment:** If the API is deployed to a public VPS later, TLS/HTTPS will be enabled using **Let's Encrypt** certificates.

**What we do now:**
- Use `http://localhost` (Nginx on port 80) as the reverse proxy entrypoint.
- All API calls in development go through HTTP (no certificates required).

**What we may do later (VPS):**
- Configure a real domain name (e.g., `api.example.com`) pointing to the VPS.
- Enable HTTPS in Nginx and obtain certificates via Let's Encrypt (e.g., using Certbot).
- Redirect HTTP → HTTPS to ensure secure traffic by default.

This phased approach keeps development lightweight while still supporting a production-like secure deployment as an optional extension.

### Reverse Proxy Risks & Must-Have Settings

To ensure correct behavior and easier debugging, the reverse proxy configuration must include the following **must-have settings**:

**Forwarded headers (client info):**
- Forward the original client IP chain using `X-Forwarded-For`.
- Forward the client IP using `X-Real-IP`.
- Forward the original protocol using `X-Forwarded-Proto` (important for future HTTPS).
- Preserve the original `Host` header.

**Proxy stability settings:**
- Use HTTP/1.1 towards the upstream (`proxy_http_version 1.1`) and keep connections stable.
- Disable automatic upstream redirects where appropriate (`proxy_redirect off`).

**Timeouts (to avoid hanging requests and unclear failures):**
- `proxy_connect_timeout` (connecting to backend)
- `proxy_send_timeout` (sending request to backend)
- `proxy_read_timeout` (reading response from backend)

**Logging (operational visibility):**
- Enable access logs for all requests.
- Enable error logs for proxy/upstream failures (e.g., 502/504).
- Include upstream information and response time in logs to support troubleshooting.

**Main risks if these are missing or misconfigured:**
- Backend sees the proxy IP instead of the real client IP (incorrect auditing/debugging).
- Backend cannot detect HTTPS correctly later (wrong redirects / URL generation).
- Lack of logs makes troubleshooting slow (502/504 issues).
- Missing/incorrect timeouts may cause hung connections or premature failures.
- Incorrect `/api` path mapping can break endpoint routing.