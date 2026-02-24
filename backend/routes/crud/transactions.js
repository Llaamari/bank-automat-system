const express = require('express');
const router = express.Router();
const db = require('../../db');

const allowedTypes = new Set(['withdrawal', 'deposit', 'balance']);

router.get('/', async (_req, res) => {
  try {
    const [rows] = await db.execute(
      `SELECT id, account_id, amount, tx_type, created_at FROM transactions ORDER BY id DESC`
    );
    res.json(rows);
  } catch (err) {
    console.error('transactions GET all:', err);
    res.status(500).json({ error: 'Database error' });
  }
});

router.get('/:id', async (req, res) => {
  const id = Number(req.params.id);
  if (!Number.isInteger(id) || id <= 0) return res.status(400).json({ error: 'Invalid id' });

  try {
    const [rows] = await db.execute(
      `SELECT id, account_id, amount, tx_type, created_at FROM transactions WHERE id = ?`,
      [id]
    );
    if (rows.length === 0) return res.status(404).json({ error: 'Not found' });
    res.json(rows[0]);
  } catch (err) {
    console.error('transactions GET by id:', err);
    res.status(500).json({ error: 'Database error' });
  }
});

// POST /crud/transactions
router.post('/', async (req, res) => {
  const accountId = Number(req.body.account_id);
  const amount = Number(req.body.amount);
  const txType = String(req.body.tx_type ?? 'withdrawal');

  if (!Number.isInteger(accountId) || accountId <= 0) return res.status(400).json({ error: 'Invalid account_id' });
  if (!Number.isFinite(amount) || amount < 0) return res.status(400).json({ error: 'Invalid amount' });
  if (!allowedTypes.has(txType)) return res.status(400).json({ error: 'Invalid tx_type' });

  try {
    const [result] = await db.execute(
      `INSERT INTO transactions (account_id, amount, tx_type) VALUES (?, ?, ?)`,
      [accountId, amount, txType]
    );
    res.status(201).json({ id: result.insertId, account_id: accountId, amount, tx_type: txType });
  } catch (err) {
    console.error('transactions POST:', err);
    res.status(500).json({ error: 'Database error' });
  }
});

// PUT /crud/transactions/:id
router.put('/:id', async (req, res) => {
  const id = Number(req.params.id);
  const accountId = Number(req.body.account_id);
  const amount = Number(req.body.amount);
  const txType = String(req.body.tx_type ?? 'withdrawal');

  if (!Number.isInteger(id) || id <= 0) return res.status(400).json({ error: 'Invalid id' });
  if (!Number.isInteger(accountId) || accountId <= 0) return res.status(400).json({ error: 'Invalid account_id' });
  if (!Number.isFinite(amount) || amount < 0) return res.status(400).json({ error: 'Invalid amount' });
  if (!allowedTypes.has(txType)) return res.status(400).json({ error: 'Invalid tx_type' });

  try {
    const [result] = await db.execute(
      `UPDATE transactions SET account_id = ?, amount = ?, tx_type = ? WHERE id = ?`,
      [accountId, amount, txType, id]
    );
    if (result.affectedRows === 0) return res.status(404).json({ error: 'Not found' });
    res.json({ id, account_id: accountId, amount, tx_type: txType });
  } catch (err) {
    console.error('transactions PUT:', err);
    res.status(500).json({ error: 'Database error' });
  }
});

router.delete('/:id', async (req, res) => {
  const id = Number(req.params.id);
  if (!Number.isInteger(id) || id <= 0) return res.status(400).json({ error: 'Invalid id' });

  try {
    const [result] = await db.execute(`DELETE FROM transactions WHERE id = ?`, [id]);
    if (result.affectedRows === 0) return res.status(404).json({ error: 'Not found' });
    res.status(204).send();
  } catch (err) {
    console.error('transactions DELETE:', err);
    res.status(500).json({ error: 'Database error' });
  }
});

module.exports = router;