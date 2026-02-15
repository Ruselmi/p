# Game Hub HD

Web app mini game dengan tampilan HD dan perbaikan bug:

- **UNO HD**: Classic, Flip (1 kartu 2 data front/back), Mercy (+10/+8 card effect), FKK.
- **Catur HD**: langkah dasar + game tamat saat raja/ratu tertangkap.
- **Monopoly Bank + Board HD**: board visual, saldo pemain, transfer, transaksi bank, riwayat.
- **Hand Avatar AI**: MediaPipe hand tracking + avatar animasi + AI learning gesture.
- **PeerJS P2P**: opsi main online antar device/internet dengan kode room 4 karakter (maks 4 pemain: host + 3 client).

## Perbaikan bug utama

- Room code sekarang tampil jelas di badge `Room: XXXX`.
- Flow create/join room pakai konfigurasi PeerJS cloud (`0.peerjs.com:443`).
- Sinkronisasi catur P2P diperbaiki agar kirim data langkah lengkap (from/to), bukan click mentah.
- UNO mode Mercy diperbaiki dengan efek kartu +10/+8 ke lawan.

## Cara Jalan

```bash
python3 -m http.server 8000
```

Lalu buka `http://localhost:8000`.

## Catatan

- Fitur PeerJS dan MediaPipe memakai CDN internet.
- Untuk mode online, pastikan semua device punya koneksi internet stabil.
