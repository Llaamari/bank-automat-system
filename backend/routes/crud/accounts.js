const express = require('express');
const router = express.Router();
const db = require('../../db');

const allowedTypes = new Set(['debit', 'credit']);

router.get('/', async (_req, res) => {
  try {
    const [rows] = await db.execute(
      `SELECT id, customer_id, account_type, balance, credit_limit, is_locked, created_at
       FROM accounts ORDER BY id DESC`
    );
    res.json(rows);
  } catch (err) {
    console.error('accounts GET all:', err);
    res.status(500).json({ error: 'Database error' });
  }
});

router.get('/:id', async (req, res) => {
  const id = Number(req.params.id);
  if (!Number.isInteger(id) || id <= 0) return res.status(400).json({ error: 'Invalid id' });

  try {
    const [rows] = await db.execute(
      `SELECT id, customer_id, account_type, balance, credit_limit, is_locked, created_at
       FROM accounts WHERE id = ?`,
      [id]
    );
    if (rows.length === 0) return res.status(404).json({ error: 'Not found' });
    res.json(rows[0]);
  } catch (err) {
    console.error('accounts GET by id:', err);
    res.status(500).json({ error: 'Database error' });
  }
});

router.post('/', async (req, res) => {
  const customerId = Number(req.body.customer_id);
  const accountType = String(req.body.account_type ?? 'debit');
  const balance = Number(req.body.balance ?? 0);
  const creditLimit = Number(req.body.credit_limit ?? 0);

  if (!Number.isInteger(customerId) || customerId <= 0) return res.status(400).json({ error: 'Invalid customer_id' });
  if (!allowedTypes.has(accountType)) return res.status(400).json({ error: 'Invalid account_type' });
  if (!Number.isFinite(balance) || balance < 0) return res.status(400).json({ error: 'Invalid balance' });
  if (!Number.isFinite(creditLimit) || creditLimit < 0) return res.status(400).json({ error: 'Invalid credit_limit' });

  try {
    const [result] = await db.execute(
      `INSERT INTO accounts (customer_id, account_type, balance, credit_limit)
       VALUES (?, ?, ?, ?)`,
      [customerId, accountType, balance, creditLimit]
    );
    res.status(201).json({ id: result.insertId, customer_id: customerId, account_type: accountType, balance, credit_limit: creditLimit });
  } catch (err) {
    console.error('accounts POST:', err);
    res.status(500).json({ error: 'Database error' });
  }
});

router.put('/:id', async (req, res) => {
  const id = Number(req.params.id);
  const customerId = Number(req.body.customer_id);
  const accountType = String(req.body.account_type ?? 'debit');
  const balance = Number(req.body.balance ?? 0);
  const creditLimit = Number(req.body.credit_limit ?? 0);
  const isLocked = Number(req.body.is_locked ?? 0);

  if (!Number.isInteger(id) || id <= 0) return res.status(400).json({ error: 'Invalid id' });
  if (!Number.isInteger(customerId) || customerId <= 0) return res.status(400).json({ error: 'Invalid customer_id' });
  if (!allowedTypes.has(accountType)) return res.status(400).json({ error: 'Invalid account_type' });
  if (!Number.isFinite(balance) || balance < 0) return res.status(400).json({ error: 'Invalid balance' });
  if (!Number.isFinite(creditLimit) || creditLimit < 0) return res.status(400).json({ error: 'Invalid credit_limit' });
  if (![0, 1].includes(isLocked)) return res.status(400).json({ error: 'Invalid is_locked' });

  try {
    const [result] = await db.execute(
      `UPDATE accounts
       SET customer_id = ?, account_type = ?, balance = ?, credit_limit = ?, is_locked = ?
       WHERE id = ?`,
      [customerId, accountType, balance, creditLimit, isLocked, id]
    );
    if (result.affectedRows === 0) return res.status(404).json({ error: 'Not found' });

    res.json({ id, customer_id: customerId, account_type: accountType, balance, credit_limit: creditLimit, is_locked: isLocked });
  } catch (err) {
    console.error('accounts PUT:', err);
    res.status(500).json({ error: 'Database error' });
  }
});

router.delete('/:id', async (req, res) => {
  const id = Number(req.params.id);
  if (!Number.isInteger(id) || id <= 0) return res.status(400).json({ error: 'Invalid id' });

  try {
    const [result] = await db.execute(`DELETE FROM accounts WHERE id = ?`, [id]);
    if (result.affectedRows === 0) return res.status(404).json({ error: 'Not found' });
    res.status(204).send();
  } catch (err) {
    console.error('accounts DELETE:', err);
    res.status(500).json({ error: 'Database error' });
  }
});

module.exports = router;