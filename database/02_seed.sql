-- ============================================
-- seed.sql (IDEMPOTENT)
-- Creates 3 test cards:
-- 1) Debit only   -> 11111111 / 1234
-- 2) Credit only  -> 22222222 / 1234
-- 3) Both         -> 33333333 / 1234
--
-- Safe to run repeatedly (cleans only these fixed IDs)
-- ============================================

USE bank_automat;

-- Fixed IDs for seed data
-- Customers: 3001..3003
-- Cards:     1001..1003
-- Accounts:  2001..2004
-- (Dual user has 2 accounts: debit + credit)

SET FOREIGN_KEY_CHECKS = 0;

-- Remove dependent rows first (transactions -> card_accounts -> cards -> accounts -> customers)
DELETE FROM transactions WHERE account_id IN (2001, 2002, 2003, 2004);
DELETE FROM card_accounts WHERE card_id IN (1001, 1002, 1003);
DELETE FROM cards WHERE id IN (1001, 1002, 1003);
DELETE FROM accounts WHERE id IN (2001, 2002, 2003, 2004);
DELETE FROM customers WHERE id IN (3001, 3002, 3003);

SET FOREIGN_KEY_CHECKS = 1;

-- -------------------------
-- customers
-- -------------------------
INSERT INTO customers (id, first_name, last_name, address)
VALUES
(3001, 'Debit',  'Only', 'Debit Street 1'),
(3002, 'Credit', 'Only', 'Credit Street 1'),
(3003, 'Dual',   'User', 'Dual Street 1');

-- -------------------------
-- accounts
-- -------------------------
-- Debit-only user
INSERT INTO accounts (id, customer_id, account_type, balance, credit_limit)
VALUES
(2001, 3001, 'debit', 1000.00, 0.00);

-- Credit-only user
INSERT INTO accounts (id, customer_id, account_type, balance, credit_limit)
VALUES
(2002, 3002, 'credit', 0.00, 1000.00);

-- Dual user: one debit account + one credit account
INSERT INTO accounts (id, customer_id, account_type, balance, credit_limit)
VALUES
(2003, 3003, 'debit', 500.00, 0.00),
(2004, 3003, 'credit', 0.00, 1500.00);

-- -------------------------
-- cards
-- -------------------------
-- PIN = 1234 (bcrypt hash)
-- This is a common example bcrypt hash for "1234" with $2b$10$... cost 10
INSERT INTO cards (id, card_number, customer_id, pin_hash, status)
VALUES
(1001, '11111111', 3001, '$2b$10$7EqJtq98hPqEX7fNZaFWoOaZK0s8AjtKoa6HgMHqmpYyqn1nLvV6e', 'active'),
(1002, '22222222', 3002, '$2b$10$7EqJtq98hPqEX7fNZaFWoOaZK0s8AjtKoa6HgMHqmpYyqn1nLvV6e', 'active'),
(1003, '33333333', 3003, '$2b$10$7EqJtq98hPqEX7fNZaFWoOaZK0s8AjtKoa6HgMHqmpYyqn1nLvV6e', 'active');

-- -------------------------
-- card_accounts links
-- -------------------------
-- Card 1001: debit only
INSERT INTO card_accounts (card_id, account_id, role)
VALUES
(1001, 2001, 'debit');

-- Card 1002: credit only
INSERT INTO card_accounts (card_id, account_id, role)
VALUES
(1002, 2002, 'credit');

-- Card 1003: both
INSERT INTO card_accounts (card_id, account_id, role)
VALUES
(1003, 2003, 'debit'),
(1003, 2004, 'credit');

-- (Optional) A few seed transactions for nicer UI demo
INSERT INTO transactions (account_id, amount, tx_type)
VALUES
(2001, 20.00, 'withdrawal'),
(2001, 50.00, 'withdrawal'),
(2003, 20.00, 'withdrawal'),
(2004, 0.00,  'balance');