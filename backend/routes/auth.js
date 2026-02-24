const express = require('express');
const bcrypt = require('bcrypt');
const router = express.Router();
const db = require('../db');

// POST /auth/login
// body: { "cardId": 1, "pin": "1234" }
router.post('/login', async (req, res) => {
  const cardNumber = String(req.body.cardNumber ?? '').trim();
  const pin = String(req.body.pin ?? '');

  if (!cardNumber || !pin) {
    return res.status(400).json({ error: 'cardNumber and pin required' });
  }

  try {
    // Hae kortti card_numberin perusteella
    const [cards] = await db.execute(
      `SELECT id, pin_hash, status FROM cards WHERE card_number = ?`,
      [cardNumber]
    );

    if (cards.length === 0) {
      return res.status(401).json({ ok: false });
    }

    if (cards[0].status !== 'active') {
      return res.status(403).json({ error: 'Card locked' });
    }

    // Ota sisäinen kortti-ID talteen
    const cardId = cards[0].id;

    // Tarkista PIN bcryptillä
    const ok = await bcrypt.compare(pin, cards[0].pin_hash);
    if (!ok) {
      return res.status(401).json({ ok: false });
    }

    // Hae tilin ID card_accounts-taulusta
    const [links] = await db.execute(
      `SELECT account_id FROM card_accounts WHERE card_id = ? ORDER BY role ASC LIMIT 1`,
      [cardId]
    );

    if (links.length === 0) {
      return res.status(500).json({ error: 'Card has no linked account' });
    }

    // Palauta accountId
    res.json({
      ok: true,
      accountId: links[0].account_id
    });

  } catch (err) {
    console.error('Login error:', err);
    res.status(500).json({ error: 'Database error' });
  }
});

module.exports = router;