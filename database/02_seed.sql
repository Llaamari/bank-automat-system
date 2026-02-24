-- 02_seed.sql
USE bank_automat;

-- Customers
INSERT INTO customers (first_name, last_name, address)
VALUES
  ('Alice', 'Andersson', 'Example Street 1, Oulu'),
  ('Bob', 'Berg', 'Example Street 2, Oulu');

-- Accounts (Alice: debit, Bob: debit)
INSERT INTO accounts (customer_id, account_type, balance, credit_limit)
VALUES
  (1, 'debit', 500.00, 0.00),
  (2, 'debit', 1200.00, 0.00);

-- Cards (PIN hashes with bcrypt)
-- Alice PIN = 1234
-- Bob   PIN = 4321
INSERT INTO cards (customer_id, pin_hash, status)
VALUES
  (1, '$2b$10$qiL11XNOvjWhN0QA0pMQuuxhgqkF9skwn1NOTx00plaJwHZcSqB5G', 'active'),
  (2, '$2b$10$t4TMDi011yzTMH5eNMsPa.yxWjvx/DS/AsivYEqKCOVuoko8wZbaa', 'active');

-- Link each card to the owner’s account (minimum implementation)
INSERT INTO card_accounts (card_id, account_id, role)
VALUES
  (1, 1, 'debit'),
  (2, 2, 'debit');

-- A few transactions for demo
INSERT INTO transactions (account_id, amount, tx_type)
VALUES
  (1,  50.00, 'withdrawal'),
  (1,  20.00, 'withdrawal'),
  (1, 100.00, 'withdrawal'),
  (2,  40.00, 'withdrawal'),
  (2,  20.00, 'withdrawal');

UPDATE cards SET card_number = '1234567890123456' WHERE id = 1;
UPDATE cards SET card_number = '9876543210987654' WHERE id = 2;