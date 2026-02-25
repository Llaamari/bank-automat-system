const express = require('express');
const bcrypt = require('bcrypt');
const router = express.Router();
const db = require('../../db');

// Generate a random 16-digit card number (string)
function generateCardNumber() {
  let s = '';
  for (let i = 0; i < 16; i++) s += Math.floor(Math.random() * 10);
  return s;
}

async function generateUniqueCardNumber(maxAttempts = 10) {
  for (let attempt = 0; attempt < maxAttempts; attempt++) {
    const candidate = generateCardNumber();
    const [rows] = await db.execute(
      `SELECT id FROM cards WHERE card_number = ? LIMIT 1`,
      [candidate]
    );
    if (rows.length === 0) return candidate;
  }
  throw new Error('Failed to generate unique card number');
}

// Do NOT return pin_hash
router.get('/', async (_req, res) => {
  try {
    const [rows] = await db.execute(
      `SELECT id, card_number, customer_id, status, created_at FROM cards ORDER BY id DESC`
    );
    res.json(rows);
  } catch (err) {
    console.error('cards GET all:', err);
    res.status(500).json({ error: 'Database error' });
  }
});

router.get('/:id', async (req, res) => {
  const id = Number(req.params.id);
  if (!Number.isInteger(id) || id <= 0) return res.status(400).json({ error: 'Invalid id' });

  try {
    const [rows] = await db.execute(
      `SELECT id, card_number, customer_id, status, created_at FROM cards WHERE id = ?`,
      [id]
    );
    if (rows.length === 0) return res.status(404).json({ error: 'Not found' });
    res.json(rows[0]);
  } catch (err) {
    console.error('cards GET by id:', err);
    res.status(500).json({ error: 'Database error' });
  }
});

// POST /crud/cards
// body: { customer_id: 1, pin: "1234", status?: "active"|"locked" }
router.post('/', async (req, res) => {
  const customerId = Number(req.body.customer_id);
  const pin = String(req.body.pin ?? '');
  const status = String(req.body.status ?? 'active');

  if (!Number.isInteger(customerId) || customerId <= 0) return res.status(400).json({ error: 'Invalid customer_id' });
  if (pin.length < 3 || pin.length > 12) return res.status(400).json({ error: 'Invalid pin' });
  if (!['active', 'locked'].includes(status)) return res.status(400).json({ error: 'Invalid status' });

  try {
    // Validate customer exists (clear error instead of FK constraint failure)
    const [custRows] = await db.execute(`SELECT id FROM customers WHERE id = ?`, [customerId]);
    if (custRows.length === 0) return res.status(404).json({ error: 'Customer not found' });

    const cardNumber = await generateUniqueCardNumber();
    const pinHash = await bcrypt.hash(pin, 10);
    const [result] = await db.execute(
      `INSERT INTO cards (card_number, customer_id, pin_hash, status) VALUES (?, ?, ?, ?)`,
      [cardNumber, customerId, pinHash, status]
    );
    res.status(201).json({
      id: result.insertId,
      card_number: cardNumber,
      customer_id: customerId,
      status
    });
  } catch (err) {
    console.error('cards POST:', err);
    res.status(500).json({ error: 'Database error' });
  }
});

// PUT /crud/cards/:id
// body: { customer_id: 1, status: "active"|"locked", pin?: "newpin" }
router.put('/:id', async (req, res) => {
  const id = Number(req.params.id);
  const customerId = Number(req.body.customer_id);
  const status = String(req.body.status ?? 'active');
  const pin = req.body.pin !== undefined ? String(req.body.pin) : null;

  if (!Number.isInteger(id) || id <= 0) return res.status(400).json({ error: 'Invalid id' });
  if (!Number.isInteger(customerId) || customerId <= 0) return res.status(400).json({ error: 'Invalid customer_id' });
  if (!['active', 'locked'].includes(status)) return res.status(400).json({ error: 'Invalid status' });
  if (pin !== null && (pin.length < 3 || pin.length > 12)) return res.status(400).json({ error: 'Invalid pin' });

  try {
    if (pin === null) {
      const [result] = await db.execute(
        `UPDATE cards SET customer_id = ?, status = ? WHERE id = ?`,
        [customerId, status, id]
      );
      if (result.affectedRows === 0) return res.status(404).json({ error: 'Not found' });
      return res.json({ id, customer_id: customerId, status });
    }

    const pinHash = await bcrypt.hash(pin, 10);
    const [result] = await db.execute(
      `UPDATE cards SET customer_id = ?, pin_hash = ?, status = ? WHERE id = ?`,
      [customerId, pinHash, status, id]
    );
    if (result.affectedRows === 0) return res.status(404).json({ error: 'Not found' });
    res.json({ id, customer_id: customerId, status });
  } catch (err) {
    console.error('cards PUT:', err);
    res.status(500).json({ error: 'Database error' });
  }
});

router.delete('/:id', async (req, res) => {
  const id = Number(req.params.id);
  if (!Number.isInteger(id) || id <= 0) return res.status(400).json({ error: 'Invalid id' });

  try {
    const [result] = await db.execute(`DELETE FROM cards WHERE id = ?`, [id]);
    if (result.affectedRows === 0) return res.status(404).json({ error: 'Not found' });
    res.status(204).send();
  } catch (err) {
    console.error('cards DELETE:', err);
    res.status(500).json({ error: 'Database error' });
  }
});

module.exports = router;