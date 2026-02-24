const express = require('express');
const router = express.Router();
const db = require('../../db');

// GET /crud/customers
router.get('/', async (req, res) => {
  try {
    const [rows] = await db.execute(
      `SELECT id, first_name, last_name, address, created_at FROM customers ORDER BY id DESC`
    );
    res.json(rows);
  } catch (err) {
    console.error('customers GET all:', err);
    res.status(500).json({ error: 'Database error' });
  }
});

// GET /crud/customers/:id
router.get('/:id', async (req, res) => {
  const id = Number(req.params.id);
  if (!Number.isInteger(id) || id <= 0) return res.status(400).json({ error: 'Invalid id' });

  try {
    const [rows] = await db.execute(
      `SELECT id, first_name, last_name, address, created_at FROM customers WHERE id = ?`,
      [id]
    );
    if (rows.length === 0) return res.status(404).json({ error: 'Not found' });
    res.json(rows[0]);
  } catch (err) {
    console.error('customers GET by id:', err);
    res.status(500).json({ error: 'Database error' });
  }
});

// POST /crud/customers
router.post('/', async (req, res) => {
  const firstName = String(req.body.first_name ?? '').trim();
  const lastName = String(req.body.last_name ?? '').trim();
  const address = String(req.body.address ?? '').trim();

  if (!firstName || !lastName || !address) {
    return res.status(400).json({ error: 'first_name, last_name, address required' });
  }

  try {
    const [result] = await db.execute(
      `INSERT INTO customers (first_name, last_name, address) VALUES (?, ?, ?)`,
      [firstName, lastName, address]
    );
    res.status(201).json({ id: result.insertId, first_name: firstName, last_name: lastName, address });
  } catch (err) {
    console.error('customers POST:', err);
    res.status(500).json({ error: 'Database error' });
  }
});

// PUT /crud/customers/:id
router.put('/:id', async (req, res) => {
  const id = Number(req.params.id);
  const firstName = String(req.body.first_name ?? '').trim();
  const lastName = String(req.body.last_name ?? '').trim();
  const address = String(req.body.address ?? '').trim();

  if (!Number.isInteger(id) || id <= 0) return res.status(400).json({ error: 'Invalid id' });
  if (!firstName || !lastName || !address) {
    return res.status(400).json({ error: 'first_name, last_name, address required' });
  }

  try {
    const [result] = await db.execute(
      `UPDATE customers SET first_name = ?, last_name = ?, address = ? WHERE id = ?`,
      [firstName, lastName, address, id]
    );
    if (result.affectedRows === 0) return res.status(404).json({ error: 'Not found' });
    res.json({ id, first_name: firstName, last_name: lastName, address });
  } catch (err) {
    console.error('customers PUT:', err);
    res.status(500).json({ error: 'Database error' });
  }
});

// DELETE /crud/customers/:id
router.delete('/:id', async (req, res) => {
  const id = Number(req.params.id);
  if (!Number.isInteger(id) || id <= 0) return res.status(400).json({ error: 'Invalid id' });

  try {
    const [result] = await db.execute(`DELETE FROM customers WHERE id = ?`, [id]);
    if (result.affectedRows === 0) return res.status(404).json({ error: 'Not found' });
    res.status(204).send();
  } catch (err) {
    console.error('customers DELETE:', err);
    res.status(500).json({ error: 'Database error' });
  }
});

module.exports = router;