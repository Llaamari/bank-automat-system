const express = require('express');
const router = express.Router();
const db = require('../db');

router.get('/:id/transactions', async (req, res) => {
  const accountId = Number(req.params.id);
  const limit = Number(req.query.limit ?? 10);

  // Validate inputs
  if (!Number.isInteger(accountId) || accountId <= 0) {
    return res.status(400).json({ error: 'Invalid account id' });
  }

  const safeLimit = Number.isInteger(limit) ? Math.min(Math.max(limit, 1), 100) : 10;

  try {
    const sql = `
      SELECT t.id, t.tx_type, t.amount, t.created_at
      FROM transactions t
      WHERE t.account_id = ?
      ORDER BY t.created_at DESC, t.id DESC
      LIMIT ${safeLimit}
    `;

    const [rows] = await db.execute(sql, [accountId]);
    res.json(rows);
  } catch (err) {
    console.error('DB error in /accounts/:id/transactions:', err);
    res.status(500).json({ error: 'Database error' });
  }
});

// GET /accounts/:id/balance
router.get('/:id/balance', async (req, res) => {
  const accountId = Number(req.params.id);
  if (!Number.isInteger(accountId) || accountId <= 0) {
    return res.status(400).json({ error: 'Invalid account id' });
  }

  try {
    const [rows] = await db.execute(
      `SELECT id, account_type, balance, credit_limit FROM accounts WHERE id = ?`,
      [accountId]
    );
    if (rows.length === 0) return res.status(404).json({ error: 'Account not found' });

    res.json(rows[0]);
  } catch (err) {
    console.error('Balance error:', err);
    res.status(500).json({ error: 'Database error' });
  }
});

// POST /accounts/:id/withdraw
// body: { "amount": 50 }
router.post('/:id/withdraw', async (req, res) => {
  const accountId = Number(req.params.id);
  const amount = Number(req.body.amount);

  const allowed = new Set([20, 40, 50, 100]);
  if (!Number.isInteger(accountId) || accountId <= 0) {
    return res.status(400).json({ error: 'Invalid account id' });
  }
  if (!Number.isFinite(amount) || !Number.isInteger(amount)) {
    return res.status(400).json({ error: 'Invalid amount' });
  }
  if (!allowed.has(amount)) {
    return res.status(400).json({ error: 'Invalid amount (allowed: 20,40,50,100)' });
  }

  const conn = await db.getConnection();
  try {
    await conn.beginTransaction();

    // Lock the account row for update + read needed fields
    const [rows] = await conn.execute(
      `SELECT balance, account_type, credit_limit
       FROM accounts
       WHERE id = ? FOR UPDATE`,
      [accountId]
    );
    if (rows.length === 0) {
      await conn.rollback();
      return res.status(404).json({ error: 'Account not found' });
    }

    const balance = Number(rows[0].balance);
    const accountType = String(rows[0].account_type ?? 'debit');
    const creditLimit = Number(rows[0].credit_limit ?? 0);

    const newBalance = balance - amount;

    if (accountType === 'debit') {
      if (balance < amount) {
        await conn.rollback();
        return res.status(400).json({ error: 'Insufficient funds' });
      }
    } else if (accountType === 'credit') {
      // allow negative down to -creditLimit
      if (newBalance < -creditLimit) {
        await conn.rollback();
        return res.status(400).json({ error: 'Credit limit exceeded' });
      }
    } else {
      await conn.rollback();
      return res.status(500).json({ error: 'Invalid account type' });
    }

    await conn.execute(
      `UPDATE accounts SET balance = ? WHERE id = ?`,
      [newBalance, accountId]
    );

    await conn.execute(
      `INSERT INTO transactions (account_id, amount, tx_type) VALUES (?, ?, 'withdrawal')`,
      [accountId, amount]
    );

    await conn.commit();
    res.json({ ok: true, accountId, withdrawn: amount, balance: newBalance });
  } catch (err) {
    await conn.rollback();
    console.error('Withdraw error:', err);
    res.status(500).json({ error: 'Database error' });
  } finally {
    conn.release();
  }
});

module.exports = router;