-- ============================================
-- schema.sql (IDEMPOTENT)
-- Bank Automat System - MySQL/MariaDB
-- Supports:
-- - cards.card_number + bcrypt pin_hash + status
-- - card_accounts role debit/credit
-- - accounts debit/credit with credit_limit
-- - transactions with created_at
-- ============================================

CREATE DATABASE IF NOT EXISTS bank_automat
  DEFAULT CHARACTER SET utf8mb4
  DEFAULT COLLATE utf8mb4_unicode_ci;

USE bank_automat;

-- -------------------------
-- customers
-- -------------------------
CREATE TABLE IF NOT EXISTS customers (
  id           INT AUTO_INCREMENT PRIMARY KEY,
  first_name   VARCHAR(100) NOT NULL,
  last_name    VARCHAR(100) NOT NULL,
  address      VARCHAR(255) NOT NULL,
  created_at   TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP
) ENGINE=InnoDB;

-- -------------------------
-- accounts
-- -------------------------
CREATE TABLE IF NOT EXISTS accounts (
  id            INT AUTO_INCREMENT PRIMARY KEY,
  customer_id   INT NOT NULL,
  account_type  ENUM('debit','credit') NOT NULL DEFAULT 'debit',
  balance       DECIMAL(12,2) NOT NULL DEFAULT 0.00,
  credit_limit  DECIMAL(12,2) NOT NULL DEFAULT 0.00,
  created_at    TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,

  CONSTRAINT fk_accounts_customer
    FOREIGN KEY (customer_id) REFERENCES customers(id)
    ON DELETE RESTRICT ON UPDATE CASCADE,

  INDEX idx_accounts_customer_id (customer_id),
  INDEX idx_accounts_type (account_type)
) ENGINE=InnoDB;

-- -------------------------
-- cards
-- -------------------------
CREATE TABLE IF NOT EXISTS cards (
  id           INT AUTO_INCREMENT PRIMARY KEY,
  card_number  VARCHAR(32) NOT NULL,
  customer_id  INT NOT NULL,
  pin_hash     VARCHAR(100) NOT NULL,
  status       ENUM('active','locked') NOT NULL DEFAULT 'active',
  failed_pin_attempts INT NOT NULL DEFAULT 0,
  locked_at    DATETIME NULL,
  created_at   TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,

  CONSTRAINT fk_cards_customer
    FOREIGN KEY (customer_id) REFERENCES customers(id)
    ON DELETE RESTRICT ON UPDATE CASCADE,

  UNIQUE KEY uq_cards_card_number (card_number),
  INDEX idx_cards_customer_id (customer_id)
) ENGINE=InnoDB;

-- -------------------------
-- card_accounts (join)
-- -------------------------
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

  INDEX idx_ca_account_id (account_id),
  INDEX idx_ca_card_role (card_id, role)
) ENGINE=InnoDB;

-- -------------------------
-- transactions
-- -------------------------
CREATE TABLE IF NOT EXISTS transactions (
  id            BIGINT AUTO_INCREMENT PRIMARY KEY,
  account_id    INT NOT NULL,
  amount        DECIMAL(12,2) NOT NULL,
  tx_type       ENUM('withdrawal','deposit','balance') NOT NULL DEFAULT 'withdrawal',
  created_at    TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,

  CONSTRAINT fk_transactions_account
    FOREIGN KEY (account_id) REFERENCES accounts(id)
    ON DELETE RESTRICT ON UPDATE CASCADE,

  INDEX idx_tx_account_time (account_id, created_at),
  INDEX idx_tx_created_id (created_at, id)
) ENGINE=InnoDB;

-- ============================================
-- "Migrations": add missing columns/indexes
-- (safe to re-run on existing DB)
-- ============================================

-- cards.card_number
SET @col_exists := (
  SELECT COUNT(*)
  FROM INFORMATION_SCHEMA.COLUMNS
  WHERE TABLE_SCHEMA = DATABASE()
    AND TABLE_NAME = 'cards'
    AND COLUMN_NAME = 'card_number'
);
SET @sql := IF(@col_exists = 0,
  'ALTER TABLE cards ADD COLUMN card_number VARCHAR(32) NOT NULL AFTER id',
  'SELECT 1'
);
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

-- cards.status
SET @col_exists := (
  SELECT COUNT(*)
  FROM INFORMATION_SCHEMA.COLUMNS
  WHERE TABLE_SCHEMA = DATABASE()
    AND TABLE_NAME = 'cards'
    AND COLUMN_NAME = 'status'
);
SET @sql := IF(@col_exists = 0,
  'ALTER TABLE cards ADD COLUMN status ENUM(''active'',''locked'') NOT NULL DEFAULT ''active'' AFTER pin_hash',
  'SELECT 1'
);
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

-- cards.failed_pin_attempts
SET @col_exists := (
  SELECT COUNT(*)
  FROM INFORMATION_SCHEMA.COLUMNS
  WHERE TABLE_SCHEMA = DATABASE()
    AND TABLE_NAME = 'cards'
    AND COLUMN_NAME = 'failed_pin_attempts'
);
SET @sql := IF(@col_exists = 0,
  'ALTER TABLE cards ADD COLUMN failed_pin_attempts INT NOT NULL DEFAULT 0 AFTER status',
  'SELECT 1'
);
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

-- cards.locked_at
SET @col_exists := (
  SELECT COUNT(*)
  FROM INFORMATION_SCHEMA.COLUMNS
  WHERE TABLE_SCHEMA = DATABASE()
    AND TABLE_NAME = 'cards'
    AND COLUMN_NAME = 'locked_at'
);
SET @sql := IF(@col_exists = 0,
  'ALTER TABLE cards ADD COLUMN locked_at DATETIME NULL AFTER failed_pin_attempts',
  'SELECT 1'
);
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

-- unique index on cards.card_number
SET @idx_exists := (
  SELECT COUNT(*)
  FROM INFORMATION_SCHEMA.STATISTICS
  WHERE TABLE_SCHEMA = DATABASE()
    AND TABLE_NAME = 'cards'
    AND INDEX_NAME = 'uq_cards_card_number'
);
SET @sql := IF(@idx_exists = 0,
  'ALTER TABLE cards ADD UNIQUE KEY uq_cards_card_number (card_number)',
  'SELECT 1'
);
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

-- card_accounts.role
SET @col_exists := (
  SELECT COUNT(*)
  FROM INFORMATION_SCHEMA.COLUMNS
  WHERE TABLE_SCHEMA = DATABASE()
    AND TABLE_NAME = 'card_accounts'
    AND COLUMN_NAME = 'role'
);
SET @sql := IF(@col_exists = 0,
  'ALTER TABLE card_accounts ADD COLUMN role ENUM(''debit'',''credit'') NOT NULL DEFAULT ''debit'' AFTER account_id',
  'SELECT 1'
);
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

-- idx_ca_card_role
SET @idx_exists := (
  SELECT COUNT(*)
  FROM INFORMATION_SCHEMA.STATISTICS
  WHERE TABLE_SCHEMA = DATABASE()
    AND TABLE_NAME = 'card_accounts'
    AND INDEX_NAME = 'idx_ca_card_role'
);
SET @sql := IF(@idx_exists = 0,
  'ALTER TABLE card_accounts ADD INDEX idx_ca_card_role (card_id, role)',
  'SELECT 1'
);
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

-- accounts.account_type
SET @col_exists := (
  SELECT COUNT(*)
  FROM INFORMATION_SCHEMA.COLUMNS
  WHERE TABLE_SCHEMA = DATABASE()
    AND TABLE_NAME = 'accounts'
    AND COLUMN_NAME = 'account_type'
);
SET @sql := IF(@col_exists = 0,
  'ALTER TABLE accounts ADD COLUMN account_type ENUM(''debit'',''credit'') NOT NULL DEFAULT ''debit'' AFTER customer_id',
  'SELECT 1'
);
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

-- accounts.credit_limit
SET @col_exists := (
  SELECT COUNT(*)
  FROM INFORMATION_SCHEMA.COLUMNS
  WHERE TABLE_SCHEMA = DATABASE()
    AND TABLE_NAME = 'accounts'
    AND COLUMN_NAME = 'credit_limit'
);
SET @sql := IF(@col_exists = 0,
  'ALTER TABLE accounts ADD COLUMN credit_limit DECIMAL(12,2) NOT NULL DEFAULT 0.00 AFTER balance',
  'SELECT 1'
);
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;