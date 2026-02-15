const COLORS = {
  red: '#dc2626',
  blue: '#2563eb',
  green: '#16a34a',
  yellow: '#d97706',
  pink: '#db2777',
  teal: '#0d9488',
  orange: '#ea580c',
  purple: '#7e22ce',
  wild: '#111827',
};

let suppressSync = false;

function withNoSync(fn) {
  suppressSync = true;
  try { fn(); } finally { suppressSync = false; }
}

// Tabs
for (const btn of document.querySelectorAll('.tab-btn')) {
  btn.addEventListener('click', () => {
    document.querySelectorAll('.tab-btn').forEach((b) => b.classList.remove('active'));
    document.querySelectorAll('.tab').forEach((tab) => tab.classList.remove('active'));
    btn.classList.add('active');
    document.getElementById(btn.dataset.tab).classList.add('active');
  });
}

// --- PeerJS P2P (room code 4 char, max 4 players) ---
const p2pStatus = document.getElementById('p2pStatus');
const createRoomBtn = document.getElementById('createRoomBtn');
const joinRoomBtn = document.getElementById('joinRoomBtn');
const leaveRoomBtn = document.getElementById('leaveRoomBtn');
const joinCodeInput = document.getElementById('joinCodeInput');
const roomCodeBadge = document.getElementById('roomCodeBadge');

const p2p = {
  peer: null,
  isHost: false,
  hostConn: null,
  guestConns: [],
  roomCode: null,
};

function randomRoomCode() {
  return Math.random().toString(36).slice(2, 6).toUpperCase();
}

function makePeer(id) {
  const cfg = { host: '0.peerjs.com', port: 443, secure: true, path: '/' };
  return id ? new window.Peer(id, cfg) : new window.Peer(cfg);
}

function updateP2PStatus(text) {
  p2pStatus.textContent = text;
}

function updateRoomBadge(code = '-') {
  roomCodeBadge.textContent = `Room: ${code}`;
}

function cleanupP2P() {
  p2p.hostConn?.close();
  p2p.guestConns.forEach((c) => c.close());
  p2p.peer?.destroy();
  p2p.peer = null;
  p2p.isHost = false;
  p2p.hostConn = null;
  p2p.guestConns = [];
  p2p.roomCode = null;
  updateP2PStatus('Offline');
  updateRoomBadge('-');
}

function emitP2P(action, payload) {
  if (suppressSync) return;
  const msg = { action, payload };
  if (!p2p.peer) return;
  if (p2p.isHost) {
    p2p.guestConns.forEach((c) => c.open && c.send(msg));
  } else if (p2p.hostConn?.open) {
    p2p.hostConn.send(msg);
  }
}

function handleP2PMessage(msg, sourceConn = null) {
  if (!msg || !msg.action) return;
  withNoSync(() => {
    if (msg.action === 'uno:start') {
      startUno(msg.payload.mode);
    } else if (msg.action === 'uno:play') {
      playerPlay(msg.payload.index, true);
    } else if (msg.action === 'uno:draw') {
      handleUnoDraw(true);
    } else if (msg.action === 'chess:move') {
      applyRemoteChessMove(msg.payload.fr, msg.payload.fc, msg.payload.tr, msg.payload.tc);
    } else if (msg.action === 'chess:reset') {
      initChess();
    } else if (msg.action === 'mono:addPlayer') {
      addMonopolyPlayer(msg.payload.name, msg.payload.balance, true);
    } else if (msg.action === 'mono:transfer') {
      monopolyTransfer(msg.payload.from, msg.payload.to, msg.payload.amount, true);
    } else if (msg.action === 'mono:bankTxn') {
      monopolyBankTxn(msg.payload.idx, msg.payload.amount, true);
    }
  });

  if (p2p.isHost && sourceConn) {
    p2p.guestConns.forEach((c) => {
      if (c !== sourceConn && c.open) c.send(msg);
    });
  }
}

createRoomBtn.addEventListener('click', () => {
  if (!window.Peer) {
    updateP2PStatus('PeerJS tidak tersedia.');
    return;
  }
  cleanupP2P();
  const roomCode = randomRoomCode();
  const hostId = `gamehub-${roomCode}`;
  const peer = makePeer(hostId);
  p2p.peer = peer;
  p2p.isHost = true;
  p2p.roomCode = roomCode;
  updateP2PStatus(`Membuat room ${roomCode}...`);
  updateRoomBadge(roomCode);

  peer.on('open', () => { updateP2PStatus(`Host online. Kode room: ${roomCode}`); updateRoomBadge(roomCode); });
  peer.on('connection', (conn) => {
    if (p2p.guestConns.length >= 3) {
      conn.on('open', () => conn.send({ action: 'system', payload: 'Room penuh (max 4).' }));
      setTimeout(() => conn.close(), 500);
      return;
    }
    p2p.guestConns.push(conn);
    conn.on('data', (msg) => handleP2PMessage(msg, conn));
    conn.on('close', () => {
      p2p.guestConns = p2p.guestConns.filter((c) => c !== conn);
      updateP2PStatus(`Host room ${roomCode} online | client: ${p2p.guestConns.length}`);
    });
    updateP2PStatus(`Host room ${roomCode} online | client: ${p2p.guestConns.length}`);
  });
  peer.on('error', (e) => updateP2PStatus(`Error host: ${e.type} (cek internet/CDN)`));
});

joinRoomBtn.addEventListener('click', () => {
  if (!window.Peer) {
    updateP2PStatus('PeerJS tidak tersedia.');
    return;
  }
  cleanupP2P();
  const code = joinCodeInput.value.trim().toUpperCase();
  if (code.length !== 4) {
    updateP2PStatus('Kode room harus 4 karakter.');
    return;
  }
  const peer = makePeer();
  p2p.peer = peer;
  p2p.roomCode = code;
  updateRoomBadge(code);
  peer.on('open', () => {
    const conn = peer.connect(`gamehub-${code}`);
    p2p.hostConn = conn;
    conn.on('open', () => updateP2PStatus(`Terhubung ke room ${code}`));
    conn.on('data', (msg) => {
      if (msg.action === 'system') {
        updateP2PStatus(String(msg.payload));
        return;
      }
      handleP2PMessage(msg);
    });
    conn.on('close', () => updateP2PStatus('Koneksi host terputus'));
  });
  peer.on('error', (e) => updateP2PStatus(`Error join: ${e.type} (kode room / internet)`));
});

leaveRoomBtn.addEventListener('click', cleanupP2P);
updateRoomBadge('-');

// --- UNO HD ---
const unoMode = document.getElementById('unoMode');
const unoStatus = document.getElementById('unoStatus');
const unoTopCard = document.getElementById('unoTopCard');
const unoBotCount = document.getElementById('unoBotCount');
const unoHand = document.getElementById('unoHand');
const unoSideBadge = document.getElementById('unoSideBadge');
const unoDrawPile = document.getElementById('unoDrawPile');
let unoState;

function randomUnoColor() {
  const cs = ['red', 'blue', 'green', 'yellow'];
  return cs[Math.floor(Math.random() * cs.length)];
}

function makeFlipDual(valueFront, colorFront) {
  const darkColors = ['pink', 'teal', 'orange', 'purple'];
  const backColor = darkColors[Math.floor(Math.random() * darkColors.length)];
  const backValue = valueFront === 'flip' ? String((Math.floor(Math.random() * 7) + 1)) : valueFront;
  return {
    front: { value: valueFront, color: colorFront },
    back: { value: backValue, color: backColor },
  };
}

function buildUnoDeck(mode) {
  const colors = ['red', 'blue', 'green', 'yellow'];
  const numbers = [0, 1, 2, 3, 4, 5, 6, 7, 8, 9];
  const deck = [];
  for (const c of colors) {
    for (const n of numbers) {
      if (mode === 'flip') deck.push(makeFlipDual(String(n), c));
      else deck.push({ color: c, value: String(n) });
    }
    if (mode === 'flip') {
      deck.push(makeFlipDual('skip', c), makeFlipDual('+2', c), makeFlipDual('flip', c));
    } else if (mode === 'mercy') {
      deck.push(
        { color: c, value: 'skip' },
        { color: c, value: 'reverse' },
        { color: c, value: '+2' },
        { color: c, value: '+10' },
        { color: c, value: '+8' },
      );
    } else {
      deck.push({ color: c, value: 'skip' }, { color: c, value: 'reverse' }, { color: c, value: '+2' });
    }
  }

  for (let i = 0; i < 4; i++) {
    if (mode === 'flip') deck.push(makeFlipDual('wild', 'wild'));
    else {
      deck.push({ color: 'wild', value: 'wild' });
      deck.push({ color: 'wild', value: 'wild+4' });
    }
  }
  return deck.sort(() => Math.random() - 0.5);
}

function getCardFace(card) {
  if (unoState.mode === 'flip') return card[unoState.side];
  return card;
}

function currentTopColor() {
  return unoState.chosenColor || getCardFace(unoState.top).color;
}

function canPlay(card, top) {
  const c = getCardFace(card);
  const topFace = getCardFace(top);
  const topColor = currentTopColor();
  return c.color === 'wild' || c.color === topColor || c.value === topFace.value;
}

function drawCard(player) {
  if (!unoState.deck.length) return;
  player.push(unoState.deck.pop());
}

function drawMultiple(player, count) {
  for (let i = 0; i < count; i++) drawCard(player);
}

function applySpecialRules(targetHand) {
  const mode = unoState.mode;
  if (mode === 'mercy' && targetHand.length >= 35) return true;
  if (mode === 'fkk' && targetHand.length >= 15) drawMultiple(targetHand, 2);
  return false;
}

function chooseColorByActor(actor) {
  if (actor === 'bot') return randomUnoColor();
  const input = window.prompt('Pilih warna: red / blue / green / yellow', 'red');
  if (!input) return randomUnoColor();
  const picked = input.trim().toLowerCase();
  return ['red', 'blue', 'green', 'yellow'].includes(picked) ? picked : randomUnoColor();
}

function resolveUnoCardEffect(face, actor) {
  let skipOpponent = false;
  let drawCount = 0;

  if (face.value === 'skip' || face.value === 'reverse') skipOpponent = true;
  if (face.value === '+2') drawCount = 2;
  if (face.value === 'wild+4') drawCount = 4;
  if (face.value === '+10') drawCount = 10;
  if (face.value === '+8') drawCount = 8;

  if (drawCount > 0) {
    if (actor === 'player') drawMultiple(unoState.bot, drawCount);
    else drawMultiple(unoState.player, drawCount);
    skipOpponent = true;
  }

  if (face.color === 'wild') {
    unoState.chosenColor = chooseColorByActor(actor);
  } else {
    unoState.chosenColor = null;
  }

  return { skipOpponent };
}

function startUno(forcedMode = null) {
  const mode = forcedMode || unoMode.value;
  unoMode.value = mode;
  const deck = buildUnoDeck(mode);
  const startCards = mode === 'classic' ? 7 : mode === 'flip' ? 7 : 10;
  unoState = {
    mode,
    deck,
    player: [],
    bot: [],
    top: null,
    ended: false,
    side: 'front',
    chosenColor: null,
  };
  for (let i = 0; i < startCards; i++) { drawCard(unoState.player); drawCard(unoState.bot); }
  unoState.top = unoState.deck.pop();
  const firstFace = getCardFace(unoState.top);
  if (firstFace.color === 'wild') unoState.chosenColor = randomUnoColor();
  renderUno();
  emitP2P('uno:start', { mode });
}

function botTurn(chain = 0) {
  if (unoState.ended || chain > 3) return;
  const idx = unoState.bot.findIndex((c) => canPlay(c, unoState.top));

  if (idx === -1) {
    drawCard(unoState.bot);
  } else {
    const played = unoState.bot.splice(idx, 1)[0];
    unoState.top = played;
    const face = getCardFace(played);

    if (unoState.mode === 'flip' && face.value === 'flip') {
      unoState.side = unoState.side === 'front' ? 'back' : 'front';
    }

    const effect = resolveUnoCardEffect(face, 'bot');
    if (effect.skipOpponent) {
      if (applySpecialRules(unoState.player)) {
        unoState.ended = true;
        unoStatus.textContent = 'Mercy: Kamu over-limit kartu. Bot menang!';
        return;
      }
      return botTurn(chain + 1);
    }
  }

  if (applySpecialRules(unoState.bot)) {
    unoState.ended = true;
    unoStatus.textContent = 'Mercy: bot kena over-limit kartu. Kamu menang!';
  } else if (!unoState.bot.length) {
    unoState.ended = true;
    unoStatus.textContent = 'Bot menang!';
  }
}

function playerPlay(index, fromRemote = false) {
  if (unoState.ended) return;
  const card = unoState.player[index];
  if (!card || !canPlay(card, unoState.top)) return;

  const played = unoState.player.splice(index, 1)[0];
  unoState.top = played;
  const face = getCardFace(played);

  if (unoState.mode === 'flip' && face.value === 'flip') {
    unoState.side = unoState.side === 'front' ? 'back' : 'front';
  }

  const effect = resolveUnoCardEffect(face, 'player');

  if (!unoState.player.length) {
    unoState.ended = true;
    unoStatus.textContent = 'Kamu menang!';
  } else if (!effect.skipOpponent) {
    botTurn();
  }

  if (applySpecialRules(unoState.bot)) {
    unoState.ended = true;
    unoStatus.textContent = 'Mercy: bot kena over-limit kartu. Kamu menang!';
  }

  renderUno();
  if (!fromRemote) emitP2P('uno:play', { index });
}

function handleUnoDraw(fromRemote = false) {
  if (unoState.ended) return;
  drawCard(unoState.player);
  botTurn();
  renderUno();
  if (!fromRemote) emitP2P('uno:draw', {});
}

function renderUno() {
  const topFace = getCardFace(unoState.top);
  const shownColor = currentTopColor();
  unoTopCard.textContent = `${shownColor} ${topFace.value}`;
  unoTopCard.style.background = COLORS[shownColor] || '#111827';
  unoBotCount.textContent = `${unoState.bot.length} kartu`;
  unoDrawPile.textContent = `${unoState.deck.length} kartu`;
  unoSideBadge.textContent = unoState.mode === 'flip' ? unoState.side.toUpperCase() : (unoState.chosenColor || 'N/A').toUpperCase();
  unoHand.innerHTML = '';

  unoState.player.forEach((card, i) => {
    const f = getCardFace(card);
    const btn = document.createElement('button');
    btn.className = 'play-card';
    btn.style.background = COLORS[f.color] || '#111827';
    if (unoState.mode === 'flip') btn.textContent = `${card.front.value}/${card.back.value} (${f.color})`;
    else btn.textContent = `${f.color} ${f.value}`;
    btn.onclick = () => playerPlay(i);
    unoHand.appendChild(btn);
  });

  if (!unoState.ended) {
    unoStatus.textContent = `Mode ${unoState.mode.toUpperCase()} | Giliran kamu`;
  }
}

document.getElementById('startUno').addEventListener('click', () => startUno());
document.getElementById('unoDraw').addEventListener('click', () => handleUnoDraw());
startUno();

// --- Chess ---
const PIECES = {
  r: '♜', n: '♞', b: '♝', q: '♛', k: '♚', p: '♟',
  R: '♖', N: '♘', B: '♗', Q: '♕', K: '♔', P: '♙',
};
let chessState;
const chessBoardEl = document.getElementById('chessBoard');
const chessStatus = document.getElementById('chessStatus');
const chessCapturedWhite = document.getElementById('chessCapturedWhite');
const chessCapturedBlack = document.getElementById('chessCapturedBlack');

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
    capturedByWhite: [],
    capturedByBlack: [],
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

function moveChessPiece(fr, fc, tr, tc) {
  if (!validMove(fr, fc, tr, tc)) return false;
  const captured = chessState.board[tr][tc];
  chessState.board[tr][tc] = chessState.board[fr][fc];
  chessState.board[fr][fc] = '.';
  if (captured !== '.') {
    if (chessState.turn === 'white') chessState.capturedByWhite.push(captured);
    else chessState.capturedByBlack.push(captured);
  }
  if (captured !== '.' && ['k', 'q'].includes(captured.toLowerCase())) {
    chessState.ended = true;
    chessState.winner = chessState.turn;
  } else {
    chessState.turn = chessState.turn === 'white' ? 'black' : 'white';
  }
  return true;
}

function applyRemoteChessMove(fr, fc, tr, tc) {
  if (chessState.ended) return;
  moveChessPiece(fr, fc, tr, tc);
  chessState.selected = null;
  renderChess();
}

function handleSquareClick(r, c) {
  if (chessState.ended) return;
  const sel = chessState.selected;
  const currentPiece = chessState.board[r][c];

  if (!sel) {
    if (currentPiece === '.') return;
    if ((chessState.turn === 'white' && !isWhite(currentPiece)) || (chessState.turn === 'black' && isWhite(currentPiece))) return;
    chessState.selected = [r, c];
    renderChess();
    return;
  }

  const [fr, fc] = sel;
  const moved = moveChessPiece(fr, fc, r, c);
  chessState.selected = null;
  renderChess();
  if (moved) emitP2P('chess:move', { fr, fc, tr: r, tc: c });
}

function renderChess() {
  chessBoardEl.innerHTML = '';
  for (let r = 0; r < 8; r++) {
    for (let c = 0; c < 8; c++) {
      const sq = document.createElement('button');
      sq.className = `square ${(r + c) % 2 === 0 ? 'light' : 'dark'}`;
      if (chessState.selected && chessState.selected[0] === r && chessState.selected[1] === c) sq.classList.add('selected');
      const piece = chessState.board[r][c];
      sq.textContent = piece === '.' ? '' : PIECES[piece];
      sq.addEventListener('click', () => handleSquareClick(r, c));
      chessBoardEl.appendChild(sq);
    }
  }
  chessStatus.textContent = chessState.ended ? `Game tamat: ${chessState.winner} menang.` : `Giliran: ${chessState.turn}`;
  chessCapturedWhite.textContent = chessState.capturedByWhite.map((p) => PIECES[p]).join(' ');
  chessCapturedBlack.textContent = chessState.capturedByBlack.map((p) => PIECES[p]).join(' ');
}

document.getElementById('resetChess').addEventListener('click', () => {
  initChess();
  emitP2P('chess:reset', {});
});
initChess();

// --- Monopoly ---
const players = [];
const ledger = document.getElementById('bankLedger');
const bankHistory = document.getElementById('bankHistory');
const playerName = document.getElementById('playerName');
const startBalance = document.getElementById('startBalance');
const fromPlayer = document.getElementById('fromPlayer');
const toPlayer = document.getElementById('toPlayer');
const bankPlayer = document.getElementById('bankPlayer');

function renderMonopolyBoard() {
  const tiles = ['GO', 'Mediterania', 'Dana Umum', 'Baltic', 'Pajak', 'Reading RR', 'Oriental', 'Chance', 'Vermont', 'Jail', 'St.Charles', 'Electric', 'States', 'Virginia', 'Penn RR', 'Free Parking'];
  const board = document.getElementById('monopolyBoard');
  board.innerHTML = '';
  tiles.forEach((t) => {
    const div = document.createElement('div');
    div.className = 'mono-tile';
    div.textContent = t;
    board.appendChild(div);
  });
}

function addHistory(text) {
  const li = document.createElement('li');
  li.textContent = text;
  bankHistory.prepend(li);
}

function checkBankrupt(player) {
  if (player.balance <= 0) addHistory(`⚠️ ${player.name} bangkrut (saldo $${player.balance.toFixed(2)})`);
}

function refreshSelects() {
  [fromPlayer, toPlayer, bankPlayer].forEach((sel) => {
    sel.innerHTML = players.map((p, i) => `<option value="${i}">${p.name}</option>`).join('');
  });
  ledger.innerHTML = players.map((p) => `<li>${p.name}: $${p.balance.toFixed(2)}</li>`).join('');
}

function addMonopolyPlayer(name, bal, fromRemote = false) {
  if (!name || Number.isNaN(Number(bal))) return;
  players.push({ name, balance: Number(bal) });
  addHistory(`➕ ${name} masuk game dengan saldo $${Number(bal).toFixed(2)}`);
  refreshSelects();
  if (!fromRemote) emitP2P('mono:addPlayer', { name, balance: Number(bal) });
}

function monopolyTransfer(from, to, amount, fromRemote = false) {
  if ([from, to, amount].some((x) => Number.isNaN(Number(x)))) return;
  const f = Number(from), t = Number(to), a = Number(amount);
  if (!players[f] || !players[t] || f === t || a <= 0 || players[f].balance < a) return;
  players[f].balance -= a;
  players[t].balance += a;
  addHistory(`💸 ${players[f].name} transfer $${a.toFixed(2)} ke ${players[t].name}`);
  checkBankrupt(players[f]);
  refreshSelects();
  if (!fromRemote) emitP2P('mono:transfer', { from: f, to: t, amount: a });
}

function monopolyBankTxn(idx, amount, fromRemote = false) {
  const i = Number(idx), a = Number(amount);
  if (!players[i] || Number.isNaN(a)) return;
  players[i].balance += a;
  addHistory(`🏦 Bank ${a >= 0 ? 'kredit' : 'debit'} $${Math.abs(a).toFixed(2)} untuk ${players[i].name}`);
  checkBankrupt(players[i]);
  refreshSelects();
  if (!fromRemote) emitP2P('mono:bankTxn', { idx: i, amount: a });
}

document.getElementById('addPlayer').addEventListener('click', () => {
  addMonopolyPlayer(playerName.value.trim(), Number(startBalance.value));
  playerName.value = '';
});

document.getElementById('transferBtn').addEventListener('click', () => {
  monopolyTransfer(fromPlayer.value, toPlayer.value, Number(document.getElementById('transferAmount').value));
});

document.getElementById('bankTxnBtn').addEventListener('click', () => {
  monopolyBankTxn(bankPlayer.value, Number(document.getElementById('bankAmount').value));
});

renderMonopolyBoard();

// --- Hand AI ---
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

const AI_STORAGE_KEY = 'gesture-ai-learning-v1';
let gestureDataset = JSON.parse(localStorage.getItem(AI_STORAGE_KEY) || '{}');

function featureFromLandmarks(landmarks) {
  const wrist = landmarks[0];
  return [4, 8, 12, 16, 20].map((idx) => {
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
      const dist = Math.sqrt(sample.reduce((sum, val, i) => sum + (val - feature[i]) ** 2, 0));
      if (dist < bestDistance) { bestDistance = dist; bestLabel = label; }
    });
  });
  return (!Number.isFinite(bestDistance) || bestDistance > 0.25) ? 'unknown' : bestLabel;
}

function renderGestureSamples() {
  gestureSamplesList.innerHTML = Object.entries(gestureDataset)
    .map(([label, samples]) => `<li><b>${label}</b>: ${samples.length} sample</li>`)
    .join('') || '<li>Belum ada data AI gesture.</li>';
}

function animateAvatarByGesture(gesture) {
  avatarMouth.className = 'mouth neutral';
  if (gesture.includes('open') || gesture.includes('five') || gesture.includes('stop')) avatarMouth.className = 'mouth open';
  else if (gesture.includes('smile') || gesture.includes('ok') || gesture.includes('love')) avatarMouth.className = 'mouth smile';
}

function drawResults(landmarks) {
  handOverlay.width = handVideo.videoWidth || 640;
  handOverlay.height = handVideo.videoHeight || 480;
  handCtx.clearRect(0, 0, handOverlay.width, handOverlay.height);
  handCtx.drawImage(handVideo, 0, 0, handOverlay.width, handOverlay.height);

  if (!landmarks?.length) {
    latestFeature = null;
    gestureLabel.textContent = 'Gesture terdeteksi: unknown';
    animateAvatarByGesture('unknown');
    return;
  }
  const hand = landmarks[0];
  window.drawConnectors(handCtx, hand, window.HAND_CONNECTIONS, { color: '#22c55e', lineWidth: 3 });
  window.drawLandmarks(handCtx, hand, { color: '#ef4444', lineWidth: 2 });
  latestFeature = featureFromLandmarks(hand);
  const label = classifyGesture(latestFeature);
  gestureLabel.textContent = `Gesture terdeteksi: ${label}`;
  animateAvatarByGesture(label);
}

async function startHandTracking() {
  if (trackingActive) return;
  if (!window.Hands) {
    handStatus.textContent = 'Status: MediaPipe gagal dimuat.';
    return;
  }
  try {
    handStatus.textContent = 'Status: meminta akses kamera...';
    cameraStream = await navigator.mediaDevices.getUserMedia({ video: true });
    handVideo.srcObject = cameraStream;
    await handVideo.play();
    mpHands = new window.Hands({ locateFile: (f) => `https://cdn.jsdelivr.net/npm/@mediapipe/hands/${f}` });
    mpHands.setOptions({ maxNumHands: 1, modelComplexity: 1, minDetectionConfidence: 0.6, minTrackingConfidence: 0.5 });
    mpHands.onResults((res) => drawResults(res.multiHandLandmarks));
    trackingActive = true;
    handStatus.textContent = 'Status: hand tracking aktif';

    const loop = async () => {
      if (!trackingActive) return;
      await mpHands.send({ image: handVideo });
      requestAnimationFrame(loop);
    };
    loop();
  } catch (err) {
    handStatus.textContent = `Status: gagal (${err.message})`;
  }
}

function stopHandTracking() {
  trackingActive = false;
  cameraStream?.getTracks().forEach((t) => t.stop());
  cameraStream = null;
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
