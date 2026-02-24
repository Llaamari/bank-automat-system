const express = require('express');
const router = express.Router();
const db = require('../../db');

router.get('/', async (_req, res) => {
  try {
    const [rows] = await db.execute(
      `SELECT card_id, account_id, role, created_at FROM card_accounts ORDER BY created_at DESC`
    );
    res.json(rows);
  } catch (err) {
    console.error('card_accounts GET:', err);
    res.status(500).json({ error: 'Database error' });
  }
});

// POST /crud/card-accounts
// body: { card_id: 1, account_id: 1, role: "debit"|"credit" }
router.post('/', async (req, res) => {
  const cardId = Number(req.body.card_id);
  const accountId = Number(req.body.account_id);
  const role = String(req.body.role ?? 'debit');

  if (!Number.isInteger(cardId) || cardId <= 0) return res.status(400).json({ error: 'Invalid card_id' });
  if (!Number.isInteger(accountId) || accountId <= 0) return res.status(400).json({ error: 'Invalid account_id' });
  if (!['debit', 'credit'].includes(role)) return res.status(400).json({ error: 'Invalid role' });

  try {
  await db.execute(
    `INSERT INTO card_accounts (card_id, account_id, role) VALUES (?, ?, ?)`,
    [cardId, accountId, role]
  );
  res.status(201).json({ card_id: cardId, account_id: accountId, role });
} catch (err) {
  if (err.code === 'ER_DUP_ENTRY') {
    return res.status(409).json({ error: 'Card already linked to this account' });
  }
  console.error('card_accounts POST:', err);
  res.status(500).json({ error: 'Database error' });
}
});

// DELETE /crud/card-accounts?cardId=1&accountId=1
router.delete('/', async (req, res) => {
  const cardId = Number(req.query.cardId);
  const accountId = Number(req.query.accountId);

  if (!Number.isInteger(cardId) || cardId <= 0) return res.status(400).json({ error: 'Invalid cardId' });
  if (!Number.isInteger(accountId) || accountId <= 0) return res.status(400).json({ error: 'Invalid accountId' });

  try {
    const [result] = await db.execute(
      `DELETE FROM card_accounts WHERE card_id = ? AND account_id = ?`,
      [cardId, accountId]
    );
    if (result.affectedRows === 0) return res.status(404).json({ error: 'Not found' });
    res.status(204).send();
  } catch (err) {
    console.error('card_accounts DELETE:', err);
    res.status(500).json({ error: 'Database error' });
  }
});

module.exports = router;