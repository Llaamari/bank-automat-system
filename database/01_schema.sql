-- 01_schema.sql
CREATE DATABASE IF NOT EXISTS bank_automat
  DEFAULT CHARACTER SET utf8mb4
  DEFAULT COLLATE utf8mb4_unicode_ci;

USE bank_automat;

-- Customers
CREATE TABLE IF NOT EXISTS customers (
  id           INT AUTO_INCREMENT PRIMARY KEY,
  first_name   VARCHAR(100) NOT NULL,
  last_name    VARCHAR(100) NOT NULL,
  address      VARCHAR(255) NOT NULL,
  created_at   TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP
) ENGINE=InnoDB;

-- Accounts
-- account_type: 'debit' or 'credit'
-- credit_limit: 0 for debit accounts (fits the course note)
CREATE TABLE IF NOT EXISTS accounts (
  id            INT AUTO_INCREMENT PRIMARY KEY,
  customer_id   INT NOT NULL,
  account_type  ENUM('debit','credit') NOT NULL DEFAULT 'debit',
  balance       DECIMAL(12,2) NOT NULL DEFAULT 0.00,
  credit_limit  DECIMAL(12,2) NOT NULL DEFAULT 0.00,
  is_locked     TINYINT(1) NOT NULL DEFAULT 0,
  created_at    TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  CONSTRAINT fk_accounts_customer
    FOREIGN KEY (customer_id) REFERENCES customers(id)
    ON DELETE RESTRICT ON UPDATE CASCADE,
  INDEX idx_accounts_customer_id (customer_id)
) ENGINE=InnoDB;

-- Cards
-- pin_hash stored with bcrypt (course requirement)
CREATE TABLE IF NOT EXISTS cards (
  id           INT AUTO_INCREMENT PRIMARY KEY,
  customer_id  INT NOT NULL,
  pin_hash     VARCHAR(100) NOT NULL,
  status       ENUM('active','locked') NOT NULL DEFAULT 'active',
  created_at   TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  CONSTRAINT fk_cards_customer
    FOREIGN KEY (customer_id) REFERENCES customers(id)
    ON DELETE RESTRICT ON UPDATE CASCADE,
  INDEX idx_cards_customer_id (customer_id)
) ENGINE=InnoDB;

-- Card ↔ Account link (supports dual card later)
CREATE TABLE IF NOT EXISTS card_accounts (
  card_id     INT NOT NULL,
  account_id  INT NOT NULL,
  role        ENUM('debit','credit') NOT NULL DEFAULT 'debit',
  created_at  TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (card_id, account_id),
  CONSTRAINT fk_ca_card
    FOREIGN KEY (card_id) REFERENCES cards(id)
    ON DELETE CASCADE ON UPDATE CASCADE,
  CONSTRAINT fk_ca_account
    FOREIGN KEY (account_id) REFERENCES accounts(id)
    ON DELETE RESTRICT ON UPDATE CASCADE,
  INDEX idx_ca_account_id (account_id)
) ENGINE=InnoDB;

-- Transactions
CREATE TABLE IF NOT EXISTS transactions (
  id            BIGINT AUTO_INCREMENT PRIMARY KEY,
  account_id    INT NOT NULL,
  amount        DECIMAL(12,2) NOT NULL,
  tx_type       ENUM('withdrawal','deposit','balance') NOT NULL DEFAULT 'withdrawal',
  created_at    TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  CONSTRAINT fk_transactions_account
    FOREIGN KEY (account_id) REFERENCES accounts(id)
    ON DELETE RESTRICT ON UPDATE CASCADE,
  INDEX idx_tx_account_time (account_id, created_at)
) ENGINE=InnoDB;