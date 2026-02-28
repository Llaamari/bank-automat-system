const express = require('express');
const router = express.Router();
const path = require('path');
const fs = require('fs');
const multer = require('multer');
const { randomUUID } = require('crypto');

// Where uploads are stored on disk (inside backend/public)
const UPLOAD_DIR = path.join(__dirname, '..', 'public', 'images', 'uploads');

// Ensure upload dir exists
fs.mkdirSync(UPLOAD_DIR, { recursive: true });

// Multer storage config
const storage = multer.diskStorage({
  destination: (req, file, cb) => {
    cb(null, UPLOAD_DIR);
  },
  filename: (req, file, cb) => {
    // Keep original extension, but generate unique filename
    const ext = path.extname(file.originalname).toLowerCase();
    const safeExt = ext && ext.length <= 10 ? ext : '';
    const unique = `${Date.now()}-${randomUUID()}${safeExt}`;
    cb(null, unique);
  }
});

// Only accept images
function fileFilter(req, file, cb) {
  if (file && typeof file.mimetype === 'string' && file.mimetype.startsWith('image/')) {
    return cb(null, true);
  }
  const err = new Error('Only image files are allowed');
  err.statusCode = 400;
  cb(err, false);
}

const upload = multer({
  storage,
  fileFilter,
  limits: {
    fileSize: 5 * 1024 * 1024 // 5 MB
  }
});

// POST /images/upload  (multipart/form-data field: "image")
router.post('/upload', (req, res) => {
  upload.single('image')(req, res, (err) => {
    if (err) {
      if (err.code === 'LIMIT_FILE_SIZE') {
        return res.status(400).json({ ok: false, error: 'File too large (max 5MB)' });
      }
      const status = err.statusCode || 400;
      return res.status(status).json({ ok: false, error: err.message || 'Upload failed' });
    }

    if (!req.file) {
      return res.status(400).json({ ok: false, error: 'No file uploaded. Use field name "image".' });
    }

    const filename = req.file.filename;
    const url = `/images/uploads/${filename}`;

    return res.status(201).json({ ok: true, filename, url });
  });
});

module.exports = router;
