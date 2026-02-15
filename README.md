# Game Hub HD

Web app mini game dengan tampilan HD dan update UNO lebih mendekati game asli.

## Fokus update terbaru (UNO)

Perbaikan utama:
- Mode Mercy sekarang punya **reverse khusus** (reverse memberi efek skip + draw tambahan pada lawan).
- Kartu action yang sebelumnya bermasalah sekarang aktif: `reverse`, `skip`, `+2`, `wild`, `wild+4`, `+10`, `+8`.

### 15 fitur baru UNO
1. Reverse khusus mode Mercy.
2. Wild card pilih warna (player via color picker).
3. Wild+4 aktif penuh.
4. Stack draw card (`+2/+4/+8/+10`) dengan tipe sama.
5. Pending draw indicator di status.
6. Draw pile bisa diklik untuk draw.
7. Shortcut keyboard `D` untuk draw.
8. UNO button (`UNO!`).
9. Penalti lupa tekan UNO (+2).
10. Recycle deck otomatis dari discard saat deck habis.
11. Bot difficulty selector (easy/normal/hard).
12. Riwayat aksi UNO (maks 15 log).
13. Highlight kartu legal playable.
14. Timer giliran 20 detik + auto draw saat habis.
15. Scoreboard + round counter.

## Fitur lain
- Catur HD: langkah dasar + game tamat saat raja/ratu tertangkap + panel bidak tertangkap.
- Monopoly Board HD: board visual, saldo pemain, transfer, transaksi bank, riwayat.
- Hand Avatar AI: MediaPipe hand tracking + avatar animasi + AI learning gesture.
- PeerJS P2P: room 4 karakter, hingga 4 pemain total.

## Menjalankan

```bash
python3 -m http.server 8000
```

Lalu buka `http://localhost:8000`.


## Update Hand AI (efek gesture jari)
- Deteksi jumlah jari realtime ditampilkan di UI.
- Gesture jari 2: stiker terbang.
- Gesture jari 3: efek blackhole animasi.
- Gesture jari 4: efek whitehole animasi.
- Gesture jari 5: avatar pulse + stiker.
- Gesture jari 6 ke atas: combo blackhole + whitehole + stiker.


## Update terbaru tambahan
- UNO sekarang bisa pilih **lawan Bot** atau **2 Player Offline**.
- Layout UNO HD dibuat lebih jelas, draw/discard tampil seperti tumpukan kartu nyata.
- Efek Hand AI sekarang tampil juga di **seluruh layar** (fullscreen overlay) selain panel FX lokal.
