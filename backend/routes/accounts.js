const express = require('express');
const router = express.Router();
const db = require('../db'); // luodaan kohta

// GET last 10 transactions
router.get('/:id/transactions', async (req, res) => {
  const accountId = req.params.id;

  try {
    const [rows] = await db.execute(
      `
      SELECT
        t.id,
        t.tx_type,
        t.amount,
        t.created_at
      FROM transactions t
      WHERE t.account_id = ?
      ORDER BY t.created_at DESC, t.id DESC
      LIMIT 10
      `,
      [accountId]
    );

    res.json(rows);
  } catch (err) {
    console.error(err);
    res.status(500).json({ error: 'Database error' });
  }
});

module.exports = router;