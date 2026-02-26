const express = require('express');
const router = express.Router();
const db = require('../db');

/**
 * Compute ATM bill breakdown using only 20€ and 50€ bills.
 * Prefer more 50€ bills (typical ATM behavior).
 * @param {number} amount
 * @returns {{ok:true, bills: {"50": number, "20": number}} | {ok:false}}
 */
function computeBills(amount) {
  // returns { ok: true, bills: { "50": x, "20": y } } or { ok:false }
  if (!Number.isInteger(amount) || amount <= 0) return { ok: false };
  if (amount % 10 !== 0) return { ok: false }; // quick reject

  // Prefer more 50s (typical ATM behavior), but any valid combo is fine.
  for (let fifties = Math.floor(amount / 50); fifties >= 0; fifties--) {
    const rest = amount - 50 * fifties;
    if (rest >= 0 && rest % 20 === 0) {
      const twenties = rest / 20;
      return { ok: true, bills: { "50": fifties, "20": twenties } };
    }
  }
  return { ok: false };
}

// GET /accounts/:id/transactions?limit=10&before=<created_at>|<id>&after=<created_at>|<id>
router.get('/:id/transactions', async (req, res) => {
  const accountId = Number(req.params.id);
  const limit = Number(req.query.limit ?? 10);
  const before = req.query.before ? String(req.query.before) : null;
  const after = req.query.after ? String(req.query.after) : null;

  // Validate inputs
  if (!Number.isInteger(accountId) || accountId <= 0) {
    return res.status(400).json({ error: 'Invalid account id' });
  }

  const safeLimit = Number.isInteger(limit) ? Math.min(Math.max(limit, 1), 100) : 10;

  if (before && after) {
    return res.status(400).json({ error: 'Use only one: before or after' });
  }

  function parseCursor(cur) {
    // format: "<epochMs>|<id>"
    const parts = String(cur).split('|');
    if (parts.length !== 2) return null;

    const ms = Number(parts[0]);
    const id = Number(parts[1]);

    if (!Number.isFinite(ms) || ms <= 0) return null;
    if (!Number.isInteger(id) || id <= 0) return null;

    return { ms, id };
  }

  function makeCursor(row) {
    // row.created_at can be Date or string
    const ms = row.created_at instanceof Date
      ? row.created_at.getTime()
      : Date.parse(String(row.created_at));

    return `${ms}|${row.id}`;
  }

  try {
    // Fetch one extra row to detect "hasMore"
    const pageSizePlusOne = safeLimit + 1;

    // Base query fields (keep same shape as before)
    const baseSelect = `
      SELECT t.id, t.tx_type, t.amount, t.created_at
      FROM transactions t
      WHERE t.account_id = ?
    `;

    let rows = [];

    if (!before && !after) {
      // First page: newest -> oldest
      const sql = `
        ${baseSelect}
        ORDER BY t.created_at DESC, t.id DESC
        LIMIT ${pageSizePlusOne}
      `;
      const [r] = await db.execute(sql, [accountId]);
      rows = r;
    } else if (before) {
      // Next page (older): created_at/id strictly less than cursor
      const c = parseCursor(before);
      if (!c) return res.status(400).json({ error: 'Invalid before cursor' });

      const sql = `
        ${baseSelect}
          AND (
            t.created_at < FROM_UNIXTIME(?/1000)
            OR (t.created_at = FROM_UNIXTIME(?/1000) AND t.id < ?)
          )
        ORDER BY t.created_at DESC, t.id DESC
        LIMIT ${pageSizePlusOne}
      `;
      const [r] = await db.execute(sql, [accountId, c.ms, c.ms, c.id]);
      rows = r;
    } else {
      // Prev page (newer): created_at/id strictly greater than cursor
      // We fetch ASC (oldest->newest) then reverse to keep response DESC.
      const c = parseCursor(after);
      if (!c) return res.status(400).json({ error: 'Invalid after cursor' });

      const sql = `
        ${baseSelect}
          AND (
          t.created_at > FROM_UNIXTIME(?/1000)
          OR (t.created_at = FROM_UNIXTIME(?/1000) AND t.id > ?)
        )
      ORDER BY t.created_at ASC, t.id ASC
      LIMIT ${pageSizePlusOne}
    `;
    const [r] = await db.execute(sql, [accountId, c.ms, c.ms, c.id]);
    rows = r.reverse();
    }

    // Determine hasMore in the requested direction
    const hasMore = rows.length > safeLimit;
    if (hasMore) rows = rows.slice(0, safeLimit);

    const items = rows;

    // Cursors:
    // - nextCursor: use last item (oldest on page) to fetch older (before=nextCursor)
    // - prevCursor: use first item (newest on page) to fetch newer (after=prevCursor)
    const nextCursor = (items.length > 0 && (before || (!before && !after))) // pages where "older" makes sense
      ? (hasMore ? makeCursor(items[items.length - 1]) : null)
      : null;

    const prevCursor = (items.length > 0 && (before || after)) // if not first page, allow going back
      ? makeCursor(items[0])
      : null;

    res.json({
      items,
      nextCursor, // pass as before=nextCursor
      prevCursor, // pass as after=prevCursor
    });
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

  if (!Number.isInteger(accountId) || accountId <= 0) {
    return res.status(400).json({ error: 'Invalid account id' });
  }

  // Basic validation: positive whole number
  if (!Number.isFinite(amount) || !Number.isInteger(amount) || amount <= 0) {
    return res.status(400).json({ error: 'Invalid amount' });
  }

  // Bill feasibility validation (20€ / 50€ only) + breakdown
  const billResult = computeBills(amount);
  if (!billResult.ok) {
    return res.status(400).json({ error: 'Invalid amount (allowed bills: 20€ and 50€)' });
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

    res.json({
      ok: true,
      accountId,
      withdrawn: amount,
      balance: newBalance,
      bills: billResult.bills
    });
  } catch (err) {
    await conn.rollback();
    console.error('Withdraw error:', err);
    res.status(500).json({ error: 'Database error' });
  } finally {
    conn.release();
  }
});

module.exports = router;