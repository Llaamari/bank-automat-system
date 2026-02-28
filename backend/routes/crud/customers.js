const express = require('express');
const router = express.Router();
const db = require('../../db');

// GET /crud/customers
router.get('/', async (req, res) => {
  try {
    const [rows] = await db.query(
      'SELECT id, first_name, last_name, address, image_filename FROM customers'
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
    const [rows] = await db.query(
      'SELECT id, first_name, last_name, address, image_filename FROM customers WHERE id = ?',
      [req.params.id]
    );

    if (rows.length === 0) {
      return res.status(404).json({ error: 'Customer not found' });
    }

    res.json(rows[0]);
  } catch (err) {
    console.error('customers GET by id:', err);
    res.status(500).json({ error: 'Database error' });
  }
});

// POST /crud/customers
router.post('/', async (req, res) => {
  try {
    const { first_name, last_name, address, image_filename } = req.body;

    const imageValue = image_filename && image_filename.trim() !== ''
      ? image_filename
      : null;

    const [result] = await db.query(
      'INSERT INTO customers (first_name, last_name, address, image_filename) VALUES (?, ?, ?, ?)',
      [first_name, last_name, address, imageValue]
    );

    res.status(201).json({
      id: result.insertId,
      first_name,
      last_name,
      address,
      image_filename: imageValue
    });
  } catch (err) {
    console.error('customers POST:', err);
    res.status(500).json({ error: 'Database error' });
  }
});

// PUT /crud/customers/:id
router.put('/:id', async (req, res) => {
  try {
    const { first_name, last_name, address, image_filename } = req.body;

    const imageValue = image_filename && image_filename.trim() !== ''
      ? image_filename
      : null;

    const [result] = await db.query(
      `UPDATE customers 
       SET first_name = ?, last_name = ?, address = ?, image_filename = ?
       WHERE id = ?`,
      [first_name, last_name, address, imageValue, req.params.id]
    );

    if (result.affectedRows === 0) {
      return res.status(404).json({ error: 'Customer not found' });
    }

    res.json({
      id: req.params.id,
      first_name,
      last_name,
      address,
      image_filename: imageValue
    });
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
    const [result] = await db.query(
      'DELETE FROM customers WHERE id = ?',
      [req.params.id]
    );

    if (result.affectedRows === 0) {
      return res.status(404).json({ error: 'Customer not found' });
    }

    res.json({ message: 'Customer deleted' });
  } catch (err) {
    console.error('customers DELETE:', err);
    res.status(500).json({ error: 'Database error' });
  }
});

module.exports = router;