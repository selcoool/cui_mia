import express from 'express';
import multer from 'multer';
import fs from 'fs';
import path from 'path';

// Tạo folder 'images' nếu chưa có
const imagesFolder = './images';
if (!fs.existsSync(imagesFolder)) {
  fs.mkdirSync(imagesFolder);
}

const app = express();
const port = 80;

// Cấu hình multer: lưu tệp vào thư mục 'images' và thêm timestamp vào tên tệp
const storage = multer.diskStorage({
  destination: function (req, file, cb) {
    cb(null, imagesFolder);
  },
  filename: function (req, file, cb) {
    cb(null, Date.now() + '_' + file.originalname); // Đặt tên tệp với timestamp
  }
});

const upload = multer({
  storage: storage,
  limits: { fileSize: 10000000 }, // Giới hạn kích thước tệp (10MB)
  fileFilter: function (req, file, cb) {
    if (!/^image/.test(file.mimetype)) {
      return cb(new Error('Invalid file type. Only image files are allowed.'));
    }
    cb(null, true);
  }
});

// Định tuyến GET cho trang chủ
app.get('/', (req, res) => {
  console.log('Received GET request to /');
  res.send('Hello World!');
});

// Định tuyến POST cho việc tải lên ảnh
app.post('/upload', upload.single('image'), (req, res) => {
  console.log('Received POST request to /upload');

  // Kiểm tra nếu không có tệp nào được tải lên
  if (!req.file) {
    console.log('No file uploaded');
    return res.status(400).send('No file uploaded');
  }

  console.log('Image uploaded successfully:', req.file.filename);

  // Trả về phản hồi thành công
  res.sendStatus(200);
});

// Lắng nghe trên cổng 80
app.listen(port, () => {
  console.log(`Server listening on port ${port}`);
});
