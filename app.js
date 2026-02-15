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
const unoDifficulty = document.getElementById('unoDifficulty');
const unoOpponentMode = document.getElementById('unoOpponentMode');
const unoStatus = document.getElementById('unoStatus');
const unoTopCard = document.getElementById('unoTopCard');
const unoBotCount = document.getElementById('unoBotCount');
const unoHand = document.getElementById('unoHand');
const unoHandP2 = document.getElementById('unoHandP2');
const unoP2Title = document.getElementById('unoP2Title');
const unoOpponentLabel = document.getElementById('unoOpponentLabel');
const unoSideBadge = document.getElementById('unoSideBadge');
const unoDrawPile = document.getElementById('unoDrawPile');
const unoColorPicker = document.getElementById('unoColorPicker');
const unoCallBtn = document.getElementById('unoCall');
const unoHistory = document.getElementById('unoHistory');
const unoRound = document.getElementById('unoRound');
const unoScorePlayer = document.getElementById('unoScorePlayer');
const unoScoreBot = document.getElementById('unoScoreBot');
const unoTimer = document.getElementById('unoTimer');
const globalFxOverlay = document.getElementById('globalFxOverlay');

const unoMeta = { scorePlayer: 0, scoreBot: 0, round: 1 };
let unoTimerHandle = null;
let unoState;

function randomUnoColor() {
  const cs = ['red', 'blue', 'green', 'yellow'];
  return cs[Math.floor(Math.random() * cs.length)];
}

function makeFlipDual(valueFront, colorFront) {
  const darkColors = ['pink', 'teal', 'orange', 'purple'];
  return {
    front: { value: valueFront, color: colorFront },
    back: { value: valueFront === 'flip' ? String((Math.floor(Math.random() * 7) + 1)) : valueFront, color: darkColors[Math.floor(Math.random() * darkColors.length)] },
  };
}

function buildUnoDeck(mode) {
  const colors = ['red', 'blue', 'green', 'yellow'];
  const numbers = [0, 1, 2, 3, 4, 5, 6, 7, 8, 9];
  const deck = [];
  for (const c of colors) {
    for (const n of numbers) deck.push(mode === 'flip' ? makeFlipDual(String(n), c) : { color: c, value: String(n) });
    if (mode === 'flip') deck.push(makeFlipDual('skip', c), makeFlipDual('+2', c), makeFlipDual('flip', c));
    else if (mode === 'mercy') deck.push({ color: c, value: 'skip' }, { color: c, value: 'reverse' }, { color: c, value: '+2' }, { color: c, value: '+10' }, { color: c, value: '+8' });
    else deck.push({ color: c, value: 'skip' }, { color: c, value: 'reverse' }, { color: c, value: '+2' });
  }
  for (let i = 0; i < 4; i++) {
    if (mode === 'flip') deck.push(makeFlipDual('wild', 'wild'));
    else deck.push({ color: 'wild', value: 'wild' }, { color: 'wild', value: 'wild+4' });
  }
  return deck.sort(() => Math.random() - 0.5);
}

function getCardFace(card) { return unoState.mode === 'flip' ? card[unoState.side] : card; }
function currentTopColor() { return unoState.chosenColor || getCardFace(unoState.top).color; }

function canPlay(card, top) {
  const c = getCardFace(card);
  const topFace = getCardFace(top);
  if (unoState.pendingDraw > 0) {
    return ['+2', 'wild+4', '+8', '+10'].includes(c.value) && c.value === unoState.pendingType;
  }
  return c.color === 'wild' || c.color === currentTopColor() || c.value === topFace.value;
}

function logUno(text) {
  const li = document.createElement('li');
  li.textContent = text;
  unoHistory.prepend(li);
  while (unoHistory.children.length > 15) unoHistory.removeChild(unoHistory.lastChild);
}

function recycleDeckIfNeeded() {
  if (unoState.deck.length > 0) return;
  const top = unoState.top;
  unoState.deck = unoState.discard.sort(() => Math.random() - 0.5);
  unoState.discard = [top];
  logUno('Deck di-recycle dari discard.');
}

function drawCard(player) {
  recycleDeckIfNeeded();
  if (!unoState.deck.length) return;
  player.push(unoState.deck.pop());
}
function drawMultiple(player, count) { for (let i = 0; i < count; i++) drawCard(player); }

function applySpecialRules(targetHand, actor) {
  if (unoState.mode === 'mercy' && targetHand.length >= 35) {
    unoState.ended = true;
    unoStatus.textContent = actor === 'player' ? 'Mercy: kamu over-limit. Bot menang!' : 'Mercy: bot over-limit. Kamu menang!';
    return true;
  }
  if (unoState.mode === 'fkk' && targetHand.length >= 15) drawMultiple(targetHand, 2);
  return false;
}

function beginTurnTimer() {
  clearInterval(unoTimerHandle);
  unoState.turnTimer = 20;
  unoTimer.textContent = `Timer: ${unoState.turnTimer}`;
  unoTimerHandle = setInterval(() => {
    if (unoState.ended) return clearInterval(unoTimerHandle);
    unoState.turnTimer -= 1;
    unoTimer.textContent = `Timer: ${unoState.turnTimer}`;
    if (unoState.turnTimer <= 0) {
      clearInterval(unoTimerHandle);
      logUno('Timer habis: auto draw 1 kartu.');
      handleUnoDraw();
    }
  }, 1000);
}

function showWildPicker(cb) {
  unoColorPicker.classList.remove('hidden');
  const buttons = [...unoColorPicker.querySelectorAll('.color-btn')];
  buttons.forEach((b) => {
    b.onclick = () => {
      unoColorPicker.classList.add('hidden');
      cb(b.dataset.color);
    };
  });
}

function chooseColorByActor(actor, done) {
  if (actor === 'bot') return done(randomUnoColor());
  showWildPicker(done);
}

function resolveUnoCardEffect(face, actor, done) {
  let skipOpponent = false;
  let drawCount = 0;

  if (face.value === 'skip') skipOpponent = true;
  if (face.value === 'reverse') {
    skipOpponent = true;
    if (unoState.mode === 'mercy') drawCount += 5; // reverse khusus mercy
  }
  if (face.value === '+2') drawCount = 2;
  if (face.value === 'wild+4') drawCount = 4;
  if (face.value === '+10') drawCount = 10;
  if (face.value === '+8') drawCount = 8;

  if (drawCount > 0) {
    if (actor === 'player') drawMultiple(unoState.bot, drawCount);
    else drawMultiple(unoState.player, drawCount);
    skipOpponent = true;
    unoState.pendingDraw = drawCount;
    unoState.pendingType = face.value;
  } else {
    unoState.pendingDraw = 0;
    unoState.pendingType = null;
  }

  if (face.color === 'wild') {
    chooseColorByActor(actor, (color) => {
      unoState.chosenColor = color;
      done({ skipOpponent });
    });
  } else {
    unoState.chosenColor = null;
    done({ skipOpponent });
  }
}

function updateUnoMetaUI() {
  unoRound.textContent = `Round: ${unoMeta.round}`;
  unoScorePlayer.textContent = `P1: ${unoMeta.scorePlayer}`;
  unoScoreBot.textContent = `${unoState?.opponentMode === 'local' ? 'P2' : 'Bot'}: ${unoMeta.scoreBot}`;
}

function currentActor() {
  return unoState.turn === 'p1' ? 'player' : (unoState.opponentMode === 'local' ? 'p2' : 'bot');
}

function handByTurn() {
  return unoState.turn === 'p1' ? unoState.player : unoState.opponent;
}

function spawnGlobalFx(count) {
  if (!globalFxOverlay) return;
  const palette = ['#f8fafc', '#22d3ee', '#a78bfa', '#f472b6', '#f59e0b'];
  for (let i = 0; i < Math.min(count, 10); i++) {
    const e = document.createElement('span');
    e.className = 'global-fx-item';
    e.textContent = '';
    e.style.background = palette[(i + count) % palette.length];
    e.style.left = `${5 + Math.random() * 90}%`;
    e.style.top = `${40 + Math.random() * 45}%`;
    globalFxOverlay.appendChild(e);
    setTimeout(() => e.remove(), 1500);
  }
}

function startUno(forcedMode = null) {
  const mode = forcedMode || unoMode.value;
  unoMode.value = mode;
  const deck = buildUnoDeck(mode);
  const startCards = mode === 'classic' ? 7 : mode === 'flip' ? 7 : 10;
  unoState = {
    mode,
    deck,
    discard: [],
    player: [],
    opponent: [],
    top: null,
    ended: false,
    side: 'front',
    chosenColor: null,
    pendingDraw: 0,
    pendingType: null,
    unoCalled: false,
    turnTimer: 20,
    turn: 'p1',
    opponentMode: unoOpponentMode.value,
  };
  for (let i = 0; i < startCards; i++) { drawCard(unoState.player); drawCard(unoState.opponent); }
  unoState.top = unoState.deck.pop();
  unoState.discard.push(unoState.top);
  const firstFace = getCardFace(unoState.top);
  if (firstFace.color === 'wild') unoState.chosenColor = randomUnoColor();
  unoOpponentLabel.innerHTML = `${unoState.opponentMode === 'local' ? '🧑 Player 2' : '🤖 Bot'} <span id="unoBotCount">0 kartu</span>`;
  logUno(`Round ${unoMeta.round} dimulai (${mode}) - ${unoState.opponentMode === 'local' ? '2 Player Offline' : 'Bot'}.`);
  renderUno();
  beginTurnTimer();
  emitP2P('uno:start', { mode });
}

function maybeAutoOpponentTurn() {
  if (unoState.opponentMode === 'bot' && unoState.turn === 'p2' && !unoState.ended) {
    setTimeout(() => botTurn(), 400);
  }
}

function switchTurn(skip = false) {
  if (skip) return;
  unoState.turn = unoState.turn === 'p1' ? 'p2' : 'p1';
}

function botTurn(chain = 0) {
  if (unoState.ended || chain > 4) return;
  unoState.turn = 'p2';
  const idxs = unoState.opponent.map((c, i) => canPlay(c, unoState.top) ? i : -1).filter((x) => x >= 0);
  if (!idxs.length) {
    const drawN = unoState.pendingDraw > 0 ? unoState.pendingDraw : 1;
    drawMultiple(unoState.opponent, drawN);
    unoState.pendingDraw = 0;
    unoState.pendingType = null;
    logUno(`Bot draw ${drawN}`);
    unoState.turn = 'p1';
    renderUno();
    beginTurnTimer();
    return;
  }

  let idx = idxs[0];
  if (unoDifficulty.value === 'hard') idx = idxs.find((i) => ['wild+4', '+10', '+8', '+2'].includes(getCardFace(unoState.opponent[i]).value)) ?? idxs[0];
  else if (unoDifficulty.value === 'normal') idx = idxs[Math.floor(Math.random() * idxs.length)];

  const played = unoState.opponent.splice(idx, 1)[0];
  unoState.top = played;
  unoState.discard.push(played);
  const face = getCardFace(played);
  if (unoState.mode === 'flip' && face.value === 'flip') unoState.side = unoState.side === 'front' ? 'back' : 'front';

  resolveUnoCardEffect(face, 'bot', (effect) => {
    logUno(`Bot main ${face.value}`);
    spawnGlobalFx(3);
    if (applySpecialRules(unoState.player, 'player') || applySpecialRules(unoState.opponent, 'bot')) return renderUno();
    if (!unoState.opponent.length) {
      unoState.ended = true;
      unoMeta.scoreBot += 1;
      unoStatus.textContent = 'Bot menang!';
      updateUnoMetaUI();
      return renderUno();
    }
    if (effect.skipOpponent) return botTurn(chain + 1);
    unoState.turn = 'p1';
    renderUno();
    beginTurnTimer();
  });
}

function maybeUnoPenalty() {
  const hand = handByTurn();
  if (hand.length === 1 && !unoState.unoCalled) {
    drawMultiple(hand, 2);
    logUno('Penalty: lupa tekan UNO! +2 kartu');
  }
  unoState.unoCalled = false;
}

function playerPlay(index, fromRemote = false) {
  if (unoState.ended) return;
  const actor = currentActor();
  if (actor === 'bot') return;
  const hand = handByTurn();
  const card = hand[index];
  if (!card || !canPlay(card, unoState.top)) return;
  clearInterval(unoTimerHandle);

  const played = hand.splice(index, 1)[0];
  unoState.top = played;
  unoState.discard.push(played);
  const face = getCardFace(played);
  if (unoState.mode === 'flip' && face.value === 'flip') unoState.side = unoState.side === 'front' ? 'back' : 'front';

  resolveUnoCardEffect(face, actor === 'player' ? 'player' : 'bot', (effect) => {
    logUno(`${actor === 'player' ? 'P1' : 'P2'} main ${face.value}`);
    spawnGlobalFx(2);
    if (!hand.length) {
      unoState.ended = true;
      if (actor === 'player') unoMeta.scorePlayer += 1;
      else unoMeta.scoreBot += 1;
      unoStatus.textContent = `${actor === 'player' ? 'P1' : 'P2'} menang!`;
      updateUnoMetaUI();
      return renderUno();
    }

    maybeUnoPenalty();
    if (applySpecialRules(unoState.player, 'player') || applySpecialRules(unoState.opponent, 'bot')) {
      updateUnoMetaUI();
      return renderUno();
    }

    switchTurn(effect.skipOpponent);
    renderUno();
    beginTurnTimer();
    maybeAutoOpponentTurn();
  });

  if (!fromRemote) emitP2P('uno:play', { index });
}

function handleUnoDraw(fromRemote = false) {
  if (unoState.ended) return;
  clearInterval(unoTimerHandle);
  const hand = handByTurn();
  const drawN = unoState.pendingDraw > 0 ? unoState.pendingDraw : 1;
  drawMultiple(hand, drawN);
  unoState.pendingDraw = 0;
  unoState.pendingType = null;
  logUno(`${unoState.turn.toUpperCase()} draw ${drawN}`);
  switchTurn(false);
  renderUno();
  beginTurnTimer();
  maybeAutoOpponentTurn();
  if (!fromRemote) emitP2P('uno:draw', {});
}

function renderUno() {
  const topFace = getCardFace(unoState.top);
  const shownColor = currentTopColor();
  unoTopCard.textContent = `${shownColor} ${topFace.value}`;
  unoTopCard.style.background = COLORS[shownColor] || '#111827';
  document.getElementById('unoBotCount').textContent = `${unoState.opponent.length} kartu`;
  unoDrawPile.textContent = `${unoState.deck.length} kartu`;
  unoSideBadge.textContent = unoState.mode === 'flip' ? unoState.side.toUpperCase() : (unoState.chosenColor || 'N/A').toUpperCase();
  unoP2Title.classList.toggle('hidden', unoState.opponentMode !== 'local');
  unoHandP2.classList.toggle('hidden', unoState.opponentMode !== 'local');

  const renderHand = (el, cards, isActive) => {
    el.innerHTML = '';
    cards.forEach((card, i) => {
      const f = getCardFace(card);
      const playable = isActive && canPlay(card, unoState.top);
      const btn = document.createElement('button');
      btn.className = `play-card ${playable ? 'playable' : ''}`;
      btn.style.background = COLORS[f.color] || '#111827';
      btn.textContent = unoState.mode === 'flip' ? `${card.front.value}/${card.back.value} (${f.color})` : `${f.color} ${f.value}`;
      btn.onclick = () => playerPlay(i);
      el.appendChild(btn);
    });
  };

  renderHand(unoHand, unoState.player, unoState.turn === 'p1');
  if (unoState.opponentMode === 'local') renderHand(unoHandP2, unoState.opponent, unoState.turn === 'p2');

  if (!unoState.ended) {
    const turnName = unoState.turn === 'p1' ? 'P1' : (unoState.opponentMode === 'local' ? 'P2' : 'Bot');
    unoStatus.textContent = `Mode ${unoState.mode.toUpperCase()} | Turn: ${turnName}${unoState.pendingDraw ? ` | Stack: ${unoState.pendingType} (${unoState.pendingDraw})` : ''}`;
  }
}

document.getElementById('startUno').addEventListener('click', () => {
  unoMeta.round += 1;
  updateUnoMetaUI();
  startUno();
});
document.getElementById('unoDraw').addEventListener('click', () => handleUnoDraw());
unoDrawPile.addEventListener('click', () => handleUnoDraw());
unoCallBtn.addEventListener('click', () => {
  if (handByTurn().length === 1) {
    unoState.unoCalled = true;
    logUno(`${unoState.turn.toUpperCase()} teriak UNO!`);
  }
});
window.addEventListener('keydown', (e) => {
  if (e.key.toLowerCase() === 'd') handleUnoDraw();
});
updateUnoMetaUI();
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
const handViewMode = document.getElementById('handViewMode');
const horrorModeToggle = document.getElementById('horrorModeToggle');
const handAiSection = document.getElementById('hand-ai');
const gestureLabel = document.getElementById('gestureLabel');
const fingerCountLabel = document.getElementById('fingerCountLabel');
const handFxStage = document.getElementById('handFxStage');
const threeFxCanvas = document.getElementById('threeFxCanvas');
const aiModelStatus = document.getElementById('aiModelStatus');
const gestureNameInput = document.getElementById('gestureName');
const gestureSamplesList = document.getElementById('gestureSamples');
const avatarMouth = document.getElementById('avatarMouth');
const avatarFace = document.getElementById('avatarFace');

const handCtx = handOverlay.getContext('2d');
let mpHands;
let cameraStream;
let trackingActive = false;
let latestFeature = null;
let fxCooldown = 0;
let horrorEnabled = true;
let threeScene; let threeCamera; let threeRenderer; let threeMesh; let threeParticles = [];
let transformerLoaded = false;




function initThreeFx() {
  if (!window.THREE || threeRenderer) return;
  const THREE = window.THREE;
  threeScene = new THREE.Scene();
  threeCamera = new THREE.PerspectiveCamera(50, 2, 0.1, 100);
  threeCamera.position.z = 4;
  threeRenderer = new THREE.WebGLRenderer({ canvas: threeFxCanvas, antialias: true, alpha: true });
  threeRenderer.setSize(threeFxCanvas.clientWidth, threeFxCanvas.clientHeight, false);

  const geo = new THREE.TorusKnotGeometry(0.9, 0.25, 100, 16);
  const mat = new THREE.MeshStandardMaterial({ color: 0x22d3ee, emissive: 0x0ea5e9, metalness: 0.4, roughness: 0.2 });
  threeMesh = new THREE.Mesh(geo, mat);
  const light = new THREE.PointLight(0xffffff, 1.2);
  light.position.set(2, 3, 4);
  const ambient = new THREE.AmbientLight(0x334155, 0.8);
  threeScene.add(light, ambient, threeMesh);

  const tick = () => {
    if (!threeRenderer) return;
    threeMesh.rotation.x += 0.01;
    threeMesh.rotation.y += 0.013;
    threeParticles.forEach((p) => {
      p.position.y += 0.02;
      p.material.opacity -= 0.01;
    });
    threeParticles = threeParticles.filter((p) => p.material.opacity > 0);
    threeRenderer.render(threeScene, threeCamera);
    requestAnimationFrame(tick);
  };
  tick();
}

function burstThree(color = 0xff00ff) {
  if (!window.THREE || !threeScene) return;
  const THREE = window.THREE;
  for (let i = 0; i < 10; i++) {
    const g = new THREE.SphereGeometry(0.03 + Math.random() * 0.06, 10, 10);
    const m = new THREE.MeshBasicMaterial({ color, transparent: true, opacity: 0.9 });
    const s = new THREE.Mesh(g, m);
    s.position.set((Math.random() - 0.5) * 2, -0.8 + Math.random() * 0.5, (Math.random() - 0.5));
    threeScene.add(s);
    threeParticles.push(s);
  }
}

function runTenAnimations(count) {
  // 10 animation presets
  if (count === 1) { avatarFace.classList.add('pulse'); burstThree(0x22c55e); }
  if (count === 2) { spawnSticker(3); burstThree(0xf59e0b); }
  if (count === 3) { handFxStage.classList.add('horror-fog'); burstThree(0x0ea5e9); }
  if (count === 4) { avatarFace.classList.add('horror'); burstThree(0xffffff); }
  if (count === 5) { avatarFace.classList.add('pulse'); spawnSticker(6); burstThree(0xf43f5e); }
  if (count === 6) { spawnSticker(4); burstThree(0xa855f7); }
  if (count === 7) { handFxStage.classList.add('horror-fog'); burstThree(0x10b981); }
  if (count === 8) { avatarFace.classList.add('horror'); burstThree(0x60a5fa); }
  if (count === 9) { avatarFace.classList.add('pulse'); spawnSticker(7); burstThree(0xfb7185); }
  if (count >= 10) { handFxStage.classList.add('horror-fog'); avatarFace.classList.add('horror'); spawnSticker(8); burstThree(0xf8fafc); }
}

async function initTransformerLite() {
  if (transformerLoaded) return;
  try {
    if (!window.transformersBridge?.pipeline) {
      aiModelStatus.textContent = 'Transformer.js: bridge belum siap';
      return;
    }
    // lightweight warmup call
    const classifier = await window.transformersBridge.pipeline('sentiment-analysis', 'Xenova/distilbert-base-uncased-finetuned-sst-2-english');
    const out = await classifier('hand animation ready');
    aiModelStatus.textContent = `Transformer.js: ready (${out[0].label})`;
    transformerLoaded = true;
  } catch (e) {
    aiModelStatus.textContent = 'Transformer.js: gagal load model';
  }
}

function applyHandViewMode() {
  if (!handAiSection) return;
  handAiSection.classList.toggle('hand-mode-stage', handViewMode.value === 'stage');
}

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

function isFingerOpen(landmarks, tip, pip) {
  return landmarks[tip].y < landmarks[pip].y;
}

function countOpenFingers(landmarks) {
  if (!landmarks) return 0;
  let count = 0;
  if (Math.abs(landmarks[4].x - landmarks[3].x) > 0.025) count += 1; // thumb approx
  if (isFingerOpen(landmarks, 8, 6)) count += 1;
  if (isFingerOpen(landmarks, 12, 10)) count += 1;
  if (isFingerOpen(landmarks, 16, 14)) count += 1;
  if (isFingerOpen(landmarks, 20, 18)) count += 1;
  return count;
}

function clearFx() {
  handFxStage.innerHTML = '';
  handFxStage.classList.remove('horror-fog');
  avatarFace.classList.remove('horror');
  if (globalFxOverlay) globalFxOverlay.innerHTML = '';
}

function spawnSticker(count) {
  const palette = ['#f8fafc', '#22d3ee', '#a78bfa', '#f472b6', '#f59e0b', '#34d399'];
  for (let i = 0; i < Math.min(12, count + 4); i++) {
    const el = document.createElement('span');
    el.className = 'fx-sticker';
    el.textContent = '';
    el.style.left = `${10 + Math.random() * 80}%`;
    el.style.bottom = `${Math.random() * 30}px`;
    el.style.background = palette[(i + count) % palette.length];
    handFxStage.appendChild(el);

    if (globalFxOverlay) {
      const ge = document.createElement('span');
      ge.className = 'global-fx-item';
      ge.textContent = '';
      ge.style.left = `${5 + Math.random() * 90}%`;
      ge.style.top = `${20 + Math.random() * 70}%`;
      ge.style.background = palette[(i + count + 1) % palette.length];
      globalFxOverlay.appendChild(ge);
      setTimeout(() => ge.remove(), 1500);
    }
  }
}

function applyHandFxByCount(count) {
  if (fxCooldown > 0) {
    fxCooldown -= 1;
    return;
  }

  clearFx();
  avatarFace.classList.remove('pulse');
  initThreeFx();
  runTenAnimations(count);

  if (count === 2) {
    spawnSticker(2);
    fxCooldown = 10;
  } else if (count === 3) {
    const h = document.createElement('div');
    h.className = 'fx-blackhole';
    handFxStage.appendChild(h);
    if (horrorEnabled) { handFxStage.classList.add('horror-fog'); avatarFace.classList.add('horror'); }
    if (globalFxOverlay) {
      const gh = document.createElement('div');
      gh.className = 'fx-blackhole';
      gh.style.left = 'calc(50% - 120px)';
      gh.style.top = 'calc(50% - 120px)';
      gh.style.width = '240px';
      gh.style.height = '240px';
      globalFxOverlay.appendChild(gh);
      setTimeout(() => gh.remove(), 1200);
    }
    fxCooldown = 12;
  } else if (count === 4) {
    const h = document.createElement('div');
    h.className = 'fx-whitehole';
    handFxStage.appendChild(h);
    if (horrorEnabled) { handFxStage.classList.add('horror-fog'); }
    if (globalFxOverlay) {
      const gw = document.createElement('div');
      gw.className = 'fx-whitehole';
      gw.style.left = 'calc(50% - 120px)';
      gw.style.top = 'calc(50% - 120px)';
      gw.style.width = '240px';
      gw.style.height = '240px';
      globalFxOverlay.appendChild(gw);
      setTimeout(() => gw.remove(), 1200);
    }
    fxCooldown = 12;
  } else if (count === 5) {
    avatarFace.classList.add('pulse');
    spawnSticker(5);
    fxCooldown = 8;
  } else if (count >= 6) {
    const b = document.createElement('div');
    b.className = 'fx-blackhole';
    const w = document.createElement('div');
    w.className = 'fx-whitehole';
    w.style.transform = 'scale(0.6)';
    handFxStage.appendChild(b);
    handFxStage.appendChild(w);
    spawnSticker(count);
    avatarFace.classList.add('pulse');
    if (horrorEnabled) { handFxStage.classList.add('horror-fog'); avatarFace.classList.add('horror'); }
    fxCooldown = 12;
  }
}

function drawResults(landmarksList) {
  handOverlay.width = handVideo.videoWidth || 640;
  handOverlay.height = handVideo.videoHeight || 480;
  handCtx.clearRect(0, 0, handOverlay.width, handOverlay.height);
  handCtx.drawImage(handVideo, 0, 0, handOverlay.width, handOverlay.height);

  if (!landmarksList?.length) {
    latestFeature = null;
    gestureLabel.textContent = 'Gesture terdeteksi: unknown';
    fingerCountLabel.textContent = 'Jumlah jari: 0';
    animateAvatarByGesture('unknown');
    return;
  }

  const hand = landmarksList[0];
  window.drawConnectors(handCtx, hand, window.HAND_CONNECTIONS, { color: '#22c55e', lineWidth: 3 });
  window.drawLandmarks(handCtx, hand, { color: '#ef4444', lineWidth: 2 });
  latestFeature = featureFromLandmarks(hand);
  const label = classifyGesture(latestFeature);
  const fingerCount = countOpenFingers(hand);
  gestureLabel.textContent = `Gesture terdeteksi: ${label}`;
  fingerCountLabel.textContent = `Jumlah jari: ${fingerCount}`;
  animateAvatarByGesture(label);
  applyHandFxByCount(fingerCount);
  if (!horrorEnabled) {
    handFxStage.classList.remove('horror-fog');
    avatarFace.classList.remove('horror');
  }
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
    mpHands.setOptions({ maxNumHands: 2, modelComplexity: 1, minDetectionConfidence: 0.6, minTrackingConfidence: 0.5 });
    mpHands.onResults((res) => drawResults(res.multiHandLandmarks));
    trackingActive = true;
    handStatus.textContent = 'Status: hand tracking aktif';
    initThreeFx();
    initTransformerLite();

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
  clearFx();
  handStatus.textContent = 'Status: idle';
}

document.getElementById('startHandTracking').addEventListener('click', startHandTracking);
document.getElementById('stopHandTracking').addEventListener('click', stopHandTracking);
handViewMode.addEventListener('change', applyHandViewMode);
horrorModeToggle.addEventListener('change', () => { horrorEnabled = horrorModeToggle.checked; if (!horrorEnabled) { handFxStage.classList.remove('horror-fog'); avatarFace.classList.remove('horror'); } });
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
applyHandViewMode();
renderGestureSamples();
