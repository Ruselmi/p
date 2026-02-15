# Game Hub

Proyek ini berisi kumpulan mini game berbasis web:

- **UNO** dengan mode **Classic**, **Flip**, **Mercy**, dan **FKK**.
- **Catur** 2 pemain lokal dengan validasi langkah dasar.
- **Monopoly (Bank Edition)** untuk mencatat saldo pemain, transfer, dan transaksi bank.
- **Hand Avatar AI** berbasis MediaPipe Hands.

## Update terbaru

- UI UNO, Catur, dan Monopoly dibuat lebih rapi (card panel, status pill, rule box).
- Ditambahkan panel **Hukum / Cara Main** di UNO, Catur, dan Monopoly.
- Catur: jika **Raja** atau **Ratu** tertangkap, game langsung **tamat**.
- Monopoly: ditambahkan **riwayat transaksi** dan penanda **bangkrut** saat saldo <= 0.

## Menjalankan

Cukup buka `index.html` di browser.

Atau jalankan server lokal sederhana:

```bash
python3 -m http.server 8000
```

Lalu akses `http://localhost:8000`.

> Catatan: fitur Hand Avatar AI butuh izin kamera dan koneksi internet untuk memuat CDN MediaPipe.
