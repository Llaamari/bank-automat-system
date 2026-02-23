const express = require('express');
const bcrypt = require('bcrypt');
const router = express.Router();
const db = require('../db');

// POST /auth/login
// body: { "cardId": 1, "pin": "1234" }
router.post('/login', async (req, res) => {
  const cardId = Number(req.body.cardId);
  const pin = String(req.body.pin ?? '');

  if (!Number.isInteger(cardId) || cardId <= 0) {
    return res.status(400).json({ error: 'Invalid cardId' });
  }
  if (pin.length < 3 || pin.length > 12) {
    return res.status(400).json({ error: 'Invalid pin' });
  }

  try {
    // Fetch card + status
    const [cards] = await db.execute(
      `SELECT id, pin_hash, status FROM cards WHERE id = ?`,
      [cardId]
    );

    if (cards.length === 0) return res.status(401).json({ ok: false });
    if (cards[0].status !== 'active') return res.status(403).json({ error: 'Card locked' });

    const ok = await bcrypt.compare(pin, cards[0].pin_hash);
    if (!ok) return res.status(401).json({ ok: false });

    // Return the linked debit account id for now (minimum implementation)
    const [links] = await db.execute(
      `SELECT account_id FROM card_accounts WHERE card_id = ? ORDER BY role ASC LIMIT 1`,
      [cardId]
    );

    if (links.length === 0) return res.status(500).json({ error: 'Card has no linked account' });

    res.json({ ok: true, cardId, accountId: links[0].account_id });
  } catch (err) {
    console.error('Login error:', err);
    res.status(500).json({ error: 'Database error' });
  }
});

module.exports = router;