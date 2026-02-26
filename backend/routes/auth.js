const express = require('express');
const bcrypt = require('bcrypt');
const router = express.Router();
const db = require('../db');
// For security, we lock the card after 3 failed PIN attempts. This is a common practice to prevent brute-force attacks.
const MAX_PIN_ATTEMPTS = 3;

// POST /auth/login
// body: { "cardNumber": "12345678", "pin": "1234" }
router.post('/login', async (req, res) => {
  const cardNumber = String(req.body.cardNumber ?? '').trim();
  const pin = String(req.body.pin ?? '');

  if (!cardNumber || !pin) {
    return res.status(400).json({ error: 'cardNumber and pin required' });
  }

  // We use a transaction to ensure that the failed attempts count is updated safely under concurrent login attempts.
  const conn = await db.getConnection();
  try {
    await conn.beginTransaction();

    // Lock card row to make failed attempts safe under concurrency
    const [cards] = await conn.execute(
      `SELECT id, pin_hash, status, failed_pin_attempts
       FROM cards
       WHERE card_number = ?
       FOR UPDATE`,
      [cardNumber]
    );

    // Card not found -> do NOT update attempts in DB (no row)
    if (cards.length === 0) {
      await conn.rollback();
      return res.status(401).json({ ok: false, error: 'Invalid credentials' });
    }

    const card = cards[0];

    // DB-level lock persists across restarts
    if (card.status !== 'active') {
      await conn.rollback();
      return res.status(403).json({ error: 'Card locked' });
    }

    // Verify PIN (bcrypt)
    const ok = await bcrypt.compare(pin, card.pin_hash);
    if (!ok) {
      const currentFails = Number(card.failed_pin_attempts ?? 0);
      const newFails = currentFails + 1;

      // Lock on 3rd wrong attempt
      if (newFails >= MAX_PIN_ATTEMPTS) {
        await conn.execute(
          `UPDATE cards
           SET failed_pin_attempts = ?, status = 'locked'
           WHERE id = ?`,
          [newFails, card.id]
        );
        await conn.commit();
        return res.status(403).json({ error: 'Card locked (too many attempts)' });
      }

      await conn.execute(
        `UPDATE cards
         SET failed_pin_attempts = ?
         WHERE id = ?`,
        [newFails, card.id]
      );

      await conn.commit();
      return res.status(401).json({
        ok: false,
        attemptsLeft: MAX_PIN_ATTEMPTS - newFails,
      });
    }

    // Success -> reset failed attempts (if any)
    if (Number(card.failed_pin_attempts ?? 0) !== 0) {
      await conn.execute(
        `UPDATE cards
         SET failed_pin_attempts = 0
         WHERE id = ?`,
        [card.id]
      );
    }

    // Fetch linked accounts (debit/credit) from join table
    const [links] = await conn.execute(
      `SELECT account_id, role
       FROM card_accounts
       WHERE card_id = ?
       ORDER BY role ASC`,
      [card.id]
    );

    if (links.length === 0) {
      await conn.rollback();
      return res.status(500).json({ error: 'Card has no linked accounts' });
    }

    await conn.commit();

    return res.json({
      ok: true,
      accounts: links.map(l => ({ role: l.role, accountId: l.account_id })),
    });

  } catch (err) {
    await conn.rollback();
    console.error('Login error:', err);
    return res.status(500).json({ error: 'Database error' });
  } finally {
    conn.release();
  }
});

module.exports = router;