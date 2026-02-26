const express = require('express');
const bcrypt = require('bcrypt');
const router = express.Router();
const db = require('../db');
// ============================================
// Runtime-only PIN attempt tracking
// - 1–2 wrong PIN: 401 { ok:false, attemptsLeft }
// - 3rd wrong PIN: 403 { error: 'Card locked (too many attempts)' }
// - Successful login resets attempts
// NOTE: No DB writes for this feature.
// ============================================
const MAX_PIN_ATTEMPTS = 3;
// cardNumber -> { fails: number, locked: boolean }
const pinAttempts = new Map();

function getAttemptState(cardNumber) {
  if (!pinAttempts.has(cardNumber)) {
    pinAttempts.set(cardNumber, { fails: 0, locked: false });
  }
  return pinAttempts.get(cardNumber);
}

function resetAttempts(cardNumber) {
  pinAttempts.delete(cardNumber);
}

function registerFail(cardNumber) {
  const st = getAttemptState(cardNumber);
  st.fails += 1;
  if (st.fails >= MAX_PIN_ATTEMPTS) st.locked = true;
  return st;
}

// POST /auth/login
// body: { "cardNumber": "12345678", "pin": "1234" }
router.post('/login', async (req, res) => {
  const cardNumber = String(req.body.cardNumber ?? '').trim();
  const pin = String(req.body.pin ?? '');

  if (!cardNumber || !pin) {
    return res.status(400).json({ error: 'cardNumber and pin required' });
  }

  // Runtime lock check first
  const state = getAttemptState(cardNumber);
  if (state.locked) {
    return res.status(403).json({ error: 'Card locked (too many attempts)' });
  }

  try {
    // Hae kortti card_numberin perusteella
    const [cards] = await db.execute(
      `SELECT id, pin_hash, status FROM cards WHERE card_number = ?`,
      [cardNumber]
    );

    // Card not found -> count as failed attempt for this cardNumber
    if (cards.length === 0) {
      const s = registerFail(cardNumber);
      if (s.locked) return res.status(403).json({ error: 'Card locked (too many attempts)' });
      return res.status(401).json({ ok: false, attemptsLeft: MAX_PIN_ATTEMPTS - s.fails });
    }

    // DB-level status lock (separate from runtime lock)
    if (cards[0].status !== 'active') {
      return res.status(403).json({ error: 'Card locked' });
    }

    // Ota sisäinen kortti-ID talteen
    const cardId = cards[0].id;

    // Tarkista PIN bcryptillä
    const ok = await bcrypt.compare(pin, cards[0].pin_hash);
    if (!ok) {
      const s = registerFail(cardNumber);
      if (s.locked) return res.status(403).json({ error: 'Card locked (too many attempts)' });
      return res.status(401).json({ ok: false, attemptsLeft: MAX_PIN_ATTEMPTS - s.fails });
    }

    // Success -> reset runtime attempts
    resetAttempts(cardNumber);

    // Hae kaikki linkitetyt tilit (debit/credit) card_accounts-taulusta
    const [links] = await db.execute(
      `SELECT account_id, role
       FROM card_accounts
       WHERE card_id = ?
       ORDER BY role ASC`,
      [cardId]
    );

    if (links.length === 0) {
      return res.status(500).json({ error: 'Card has no linked accounts' });
    }

    // Palauta roolit ja accountId:t
    return res.json({
      ok: true,
      accounts: links.map(l => ({ role: l.role, accountId: l.account_id }))
    });

  } catch (err) {
    console.error('Login error:', err);
    return res.status(500).json({ error: 'Database error' });
  }
});

module.exports = router;