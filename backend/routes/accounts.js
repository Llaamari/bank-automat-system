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

module.exports = router;