const COLORS = {
  red: '#dc2626',
  blue: '#2563eb',
  green: '#16a34a',
  yellow: '#d97706',
  wild: '#111827',
};

// Tabs
for (const btn of document.querySelectorAll('.tab-btn')) {
  btn.addEventListener('click', () => {
    document.querySelectorAll('.tab-btn').forEach((b) => b.classList.remove('active'));
    document.querySelectorAll('.tab').forEach((tab) => tab.classList.remove('active'));
    btn.classList.add('active');
    document.getElementById(btn.dataset.tab).classList.add('active');
  });
}

// UNO
const unoMode = document.getElementById('unoMode');
const unoStatus = document.getElementById('unoStatus');
const unoTopCard = document.getElementById('unoTopCard');
const unoBotCount = document.getElementById('unoBotCount');
const unoHand = document.getElementById('unoHand');
let unoState;

function buildUnoDeck(mode) {
  const colors = ['red', 'blue', 'green', 'yellow'];
  const numbers = mode === 'flip' ? [1, 2, 3, 4, 5, 6, 7, 8, 9, 10] : [0, 1, 2, 3, 4, 5, 6, 7, 8, 9];
  const deck = [];
  for (const c of colors) {
    for (const n of numbers) deck.push({ color: c, value: String(n) });
    deck.push({ color: c, value: 'skip' }, { color: c, value: '+2' });
  }
  for (let i = 0; i < 4; i++) deck.push({ color: 'wild', value: 'wild' });
  return deck.sort(() => Math.random() - 0.5);
}

function canPlay(card, top) {
  return card.color === 'wild' || card.color === top.color || card.value === top.value;
}

function drawCard(player) {
  if (unoState.deck.length === 0) return;
  player.push(unoState.deck.pop());
}

function applySpecialRules(targetHand) {
  const mode = unoState.mode;
  if (mode === 'mercy' && targetHand.length >= 25) return true;
  if (mode === 'fkk' && targetHand.length >= 15) {
    for (let i = 0; i < 2; i++) drawCard(targetHand);
  }
  return false;
}

function startUno() {
  const mode = unoMode.value;
  const deck = buildUnoDeck(mode);
  const startCards = mode === 'classic' ? 7 : mode === 'flip' ? 8 : 10;
  unoState = {
    mode,
    deck,
    player: [],
    bot: [],
    top: null,
    ended: false,
  };
  for (let i = 0; i < startCards; i++) {
    drawCard(unoState.player);
    drawCard(unoState.bot);
  }
  unoState.top = unoState.deck.pop();
  renderUno();
}

function botTurn() {
  if (unoState.ended) return;
  const idx = unoState.bot.findIndex((c) => canPlay(c, unoState.top));
  if (idx === -1) drawCard(unoState.bot);
  else unoState.top = unoState.bot.splice(idx, 1)[0];

  if (applySpecialRules(unoState.bot)) {
    unoState.ended = true;
    unoStatus.textContent = 'Mercy rule aktif: Bot kalah karena kartu terlalu banyak. Kamu menang!';
  } else if (unoState.bot.length === 0) {
    unoState.ended = true;
    unoStatus.textContent = 'Bot menang!';
  }
}

function playerPlay(index) {
  if (unoState.ended) return;
  const card = unoState.player[index];
  if (!canPlay(card, unoState.top)) return;
  unoState.top = unoState.player.splice(index, 1)[0];
  if (unoState.player.length === 0) {
    unoState.ended = true;
    unoStatus.textContent = 'Kamu menang!';
  } else {
    botTurn();
  }
  renderUno();
}

function renderUno() {
  unoTopCard.textContent = `${unoState.top.color} ${unoState.top.value}`;
  unoTopCard.style.background = COLORS[unoState.top.color] || '#111827';
  unoBotCount.textContent = `${unoState.bot.length} kartu`;
  unoHand.innerHTML = '';
  unoState.player.forEach((c, i) => {
    const btn = document.createElement('button');
    btn.className = 'play-card';
    btn.style.background = COLORS[c.color];
    btn.textContent = `${c.color} ${c.value}`;
    btn.onclick = () => playerPlay(i);
    unoHand.appendChild(btn);
  });
  if (!unoState.ended) {
    unoStatus.textContent = `Mode: ${unoState.mode.toUpperCase()} | Giliran kamu.`;
  }
}

document.getElementById('startUno').addEventListener('click', startUno);
document.getElementById('unoDraw').addEventListener('click', () => {
  if (unoState.ended) return;
  drawCard(unoState.player);
  botTurn();
  renderUno();
});
startUno();

// Chess (basic legal movement, no check logic)
const PIECES = {
  r: '♜', n: '♞', b: '♝', q: '♛', k: '♚', p: '♟',
  R: '♖', N: '♘', B: '♗', Q: '♕', K: '♔', P: '♙',
};
let chessState;
const chessBoardEl = document.getElementById('chessBoard');
const chessStatus = document.getElementById('chessStatus');

function initChess() {
  chessState = {
    board: [
      [...'rnbqkbnr'], [...'pppppppp'], [...'........'], [...'........'],
      [...'........'], [...'........'], [...'PPPPPPPP'], [...'RNBQKBNR'],
    ],
    turn: 'white',
    selected: null,
    ended: false,
    winner: null,
  };
  renderChess();
}

function isWhite(piece) { return piece >= 'A' && piece <= 'Z'; }

function validMove(fr, fc, tr, tc) {
  const piece = chessState.board[fr][fc];
  const target = chessState.board[tr][tc];
  if (piece === '.') return false;
  if (chessState.turn === 'white' && !isWhite(piece)) return false;
  if (chessState.turn === 'black' && isWhite(piece)) return false;
  if (target !== '.' && isWhite(target) === isWhite(piece)) return false;

  const dr = tr - fr;
  const dc = tc - fc;
  const adR = Math.abs(dr);
  const adC = Math.abs(dc);
  const lower = piece.toLowerCase();

  const clearPath = () => {
    const sr = Math.sign(dr), sc = Math.sign(dc);
    let r = fr + sr, c = fc + sc;
    while (r !== tr || c !== tc) {
      if (chessState.board[r][c] !== '.') return false;
      r += sr; c += sc;
    }
    return true;
  };

  if (lower === 'p') {
    const dir = isWhite(piece) ? -1 : 1;
    if (dc === 0 && target === '.' && dr === dir) return true;
    if (dc === 0 && target === '.' && dr === 2 * dir && ((fr === 6 && isWhite(piece)) || (fr === 1 && !isWhite(piece))) && chessState.board[fr + dir][fc] === '.') return true;
    if (adC === 1 && dr === dir && target !== '.') return true;
    return false;
  }
  if (lower === 'n') return (adR === 2 && adC === 1) || (adR === 1 && adC === 2);
  if (lower === 'b') return adR === adC && clearPath();
  if (lower === 'r') return (dr === 0 || dc === 0) && clearPath();
  if (lower === 'q') return ((adR === adC) || (dr === 0 || dc === 0)) && clearPath();
  if (lower === 'k') return adR <= 1 && adC <= 1;
  return false;
}

function handleSquareClick(r, c) {
  if (chessState.ended) return;
  const sel = chessState.selected;
  const currentPiece = chessState.board[r][c];

  if (!sel) {
    if (currentPiece === '.') return;
    if ((chessState.turn === 'white' && !isWhite(currentPiece)) || (chessState.turn === 'black' && isWhite(currentPiece))) {
      return;
    }
    chessState.selected = [r, c];
    renderChess();
    return;
  }

  const [fr, fc] = sel;
  if (fr === r && fc === c) {
    chessState.selected = null;
    renderChess();
    return;
  }

  if (validMove(fr, fc, r, c)) {
    const captured = chessState.board[r][c];
    chessState.board[r][c] = chessState.board[fr][fc];
    chessState.board[fr][fc] = '.';

    if (captured !== '.' && ['k', 'q'].includes(captured.toLowerCase())) {
      chessState.ended = true;
      chessState.winner = chessState.turn;
    } else {
      chessState.turn = chessState.turn === 'white' ? 'black' : 'white';
    }
  }
  chessState.selected = null;
  renderChess();
}

function renderChess() {
  chessBoardEl.innerHTML = '';
  for (let r = 0; r < 8; r++) {
    for (let c = 0; c < 8; c++) {
      const sq = document.createElement('button');
      sq.className = `square ${(r + c) % 2 === 0 ? 'light' : 'dark'}`;
      if (chessState.selected && chessState.selected[0] === r && chessState.selected[1] === c) {
        sq.classList.add('selected');
      }
      const piece = chessState.board[r][c];
      sq.textContent = piece === '.' ? '' : PIECES[piece];
      sq.addEventListener('click', () => handleSquareClick(r, c));
      chessBoardEl.appendChild(sq);
    }
  }
  if (chessState.ended) {
    chessStatus.textContent = `Game tamat: ${chessState.winner} menang (Raja/Ratu lawan tertangkap).`;
  } else {
    chessStatus.textContent = `Giliran: ${chessState.turn}`;
  }
}

document.getElementById('resetChess').addEventListener('click', initChess);
initChess();

// Monopoly bank edition (digital banker)
const players = [];
const ledger = document.getElementById('bankLedger');
const playerName = document.getElementById('playerName');
const startBalance = document.getElementById('startBalance');
const fromPlayer = document.getElementById('fromPlayer');
const toPlayer = document.getElementById('toPlayer');
const bankPlayer = document.getElementById('bankPlayer');
const bankHistory = document.getElementById('bankHistory');


function addHistory(text) {
  const item = document.createElement('li');
  item.textContent = text;
  bankHistory.prepend(item);
}

function checkBankrupt(player) {
  if (player.balance <= 0) {
    addHistory(`⚠️ ${player.name} bangkrut (saldo: $${player.balance.toFixed(2)})`);
  }
}

function refreshSelects() {
  [fromPlayer, toPlayer, bankPlayer].forEach((sel) => {
    sel.innerHTML = players.map((p, i) => `<option value="${i}">${p.name}</option>`).join('');
  });
  ledger.innerHTML = players.map((p) => `<li>${p.name}: $${p.balance.toFixed(2)}</li>`).join('');
}

document.getElementById('addPlayer').addEventListener('click', () => {
  const name = playerName.value.trim();
  const bal = Number(startBalance.value);
  if (!name || Number.isNaN(bal)) return;
  players.push({ name, balance: bal });
  addHistory(`➕ Pemain ${name} ditambahkan dengan saldo awal $${bal.toFixed(2)}`);
  playerName.value = '';
  refreshSelects();
});

document.getElementById('transferBtn').addEventListener('click', () => {
  const from = Number(fromPlayer.value);
  const to = Number(toPlayer.value);
  const amount = Number(document.getElementById('transferAmount').value);
  if ([from, to, amount].some(Number.isNaN) || amount <= 0 || from === to) return;
  if (!players[from] || !players[to] || players[from].balance < amount) return;
  players[from].balance -= amount;
  players[to].balance += amount;
  addHistory(`💸 ${players[from].name} transfer $${amount.toFixed(2)} ke ${players[to].name}`);
  checkBankrupt(players[from]);
  refreshSelects();
});

document.getElementById('bankTxnBtn').addEventListener('click', () => {
  const idx = Number(bankPlayer.value);
  const amount = Number(document.getElementById('bankAmount').value);
  if (Number.isNaN(idx) || Number.isNaN(amount) || !players[idx]) return;
  players[idx].balance += amount;
  addHistory(`🏦 Bank transaksi ${amount >= 0 ? 'kredit' : 'debit'} $${Math.abs(amount).toFixed(2)} untuk ${players[idx].name}`);
  checkBankrupt(players[idx]);
  refreshSelects();
});

// MediaPipe hand avatar + AI learning gesture
const handVideo = document.getElementById('handVideo');
const handOverlay = document.getElementById('handOverlay');
const handStatus = document.getElementById('handStatus');
const gestureLabel = document.getElementById('gestureLabel');
const gestureNameInput = document.getElementById('gestureName');
const gestureSamplesList = document.getElementById('gestureSamples');
const avatarMouth = document.getElementById('avatarMouth');

const handCtx = handOverlay.getContext('2d');
let mpHands;
let cameraStream;
let trackingActive = false;
let latestFeature = null;
let latestGesture = 'unknown';

const AI_STORAGE_KEY = 'gesture-ai-learning-v1';
let gestureDataset = JSON.parse(localStorage.getItem(AI_STORAGE_KEY) || '{}');

function featureFromLandmarks(landmarks) {
  const wrist = landmarks[0];
  const tips = [4, 8, 12, 16, 20];
  return tips.map((idx) => {
    const p = landmarks[idx];
    const dx = p.x - wrist.x;
    const dy = p.y - wrist.y;
    return Math.sqrt(dx * dx + dy * dy);
  });
}

function classifyGesture(feature) {
  let bestLabel = 'unknown';
  let bestDistance = Number.POSITIVE_INFINITY;

  Object.entries(gestureDataset).forEach(([label, samples]) => {
    samples.forEach((sample) => {
      const dist = Math.sqrt(sample.reduce((sum, val, i) => {
        const d = val - feature[i];
        return sum + d * d;
      }, 0));
      if (dist < bestDistance) {
        bestDistance = dist;
        bestLabel = label;
      }
    });
  });

  if (!Number.isFinite(bestDistance) || bestDistance > 0.25) {
    return 'unknown';
  }
  return bestLabel;
}

function renderGestureSamples() {
  const rows = Object.entries(gestureDataset).map(([label, samples]) => (
    `<li><b>${label}</b>: ${samples.length} sample</li>`
  ));
  gestureSamplesList.innerHTML = rows.join('') || '<li>Belum ada data AI gesture.</li>';
}

function animateAvatarByGesture(gesture) {
  avatarMouth.className = 'mouth neutral';
  if (gesture === 'unknown') return;
  if (gesture.includes('open') || gesture.includes('five') || gesture.includes('stop')) {
    avatarMouth.className = 'mouth open';
    return;
  }
  if (gesture.includes('smile') || gesture.includes('ok') || gesture.includes('love')) {
    avatarMouth.className = 'mouth smile';
    return;
  }
  avatarMouth.className = 'mouth neutral';
}

function drawResults(landmarks) {
  handOverlay.width = handVideo.videoWidth || 640;
  handOverlay.height = handVideo.videoHeight || 480;
  handCtx.clearRect(0, 0, handOverlay.width, handOverlay.height);
  handCtx.drawImage(handVideo, 0, 0, handOverlay.width, handOverlay.height);

  if (!landmarks || landmarks.length === 0) {
    latestFeature = null;
    latestGesture = 'unknown';
    gestureLabel.textContent = 'Gesture terdeteksi: unknown';
    animateAvatarByGesture('unknown');
    return;
  }

  const hand = landmarks[0];
  window.drawConnectors(handCtx, hand, window.HAND_CONNECTIONS, { color: '#22c55e', lineWidth: 3 });
  window.drawLandmarks(handCtx, hand, { color: '#ef4444', lineWidth: 2 });

  latestFeature = featureFromLandmarks(hand);
  latestGesture = classifyGesture(latestFeature);
  gestureLabel.textContent = `Gesture terdeteksi: ${latestGesture}`;
  animateAvatarByGesture(latestGesture);
}

async function startHandTracking() {
  if (trackingActive) return;
  if (!window.Hands) {
    handStatus.textContent = 'Status: MediaPipe gagal dimuat.';
    return;
  }

  handStatus.textContent = 'Status: meminta akses kamera...';
  try {
    cameraStream = await navigator.mediaDevices.getUserMedia({ video: true });
    handVideo.srcObject = cameraStream;
    await handVideo.play();

    mpHands = new window.Hands({
      locateFile: (file) => `https://cdn.jsdelivr.net/npm/@mediapipe/hands/${file}`,
    });
    mpHands.setOptions({
      maxNumHands: 1,
      modelComplexity: 1,
      minDetectionConfidence: 0.6,
      minTrackingConfidence: 0.5,
    });
    mpHands.onResults((results) => drawResults(results.multiHandLandmarks));

    trackingActive = true;
    handStatus.textContent = 'Status: hand tracking aktif';

    const processFrame = async () => {
      if (!trackingActive) return;
      await mpHands.send({ image: handVideo });
      requestAnimationFrame(processFrame);
    };
    processFrame();
  } catch (error) {
    handStatus.textContent = `Status: gagal (${error.message})`;
  }
}

function stopHandTracking() {
  trackingActive = false;
  if (cameraStream) {
    cameraStream.getTracks().forEach((track) => track.stop());
    cameraStream = null;
  }
  handStatus.textContent = 'Status: idle';
}

document.getElementById('startHandTracking').addEventListener('click', startHandTracking);
document.getElementById('stopHandTracking').addEventListener('click', stopHandTracking);

document.getElementById('saveGestureBtn').addEventListener('click', () => {
  const label = gestureNameInput.value.trim().toLowerCase();
  if (!label || !latestFeature) {
    handStatus.textContent = 'Status: isi nama gesture & pastikan tangan terdeteksi.';
    return;
  }
  if (!gestureDataset[label]) gestureDataset[label] = [];
  gestureDataset[label].push(latestFeature);
  localStorage.setItem(AI_STORAGE_KEY, JSON.stringify(gestureDataset));
  handStatus.textContent = `Status: sample gesture "${label}" tersimpan.`;
  renderGestureSamples();
});

document.getElementById('clearGestureBtn').addEventListener('click', () => {
  gestureDataset = {};
  localStorage.setItem(AI_STORAGE_KEY, JSON.stringify(gestureDataset));
  handStatus.textContent = 'Status: semua data AI gesture dihapus.';
  renderGestureSamples();
});

renderGestureSamples();
