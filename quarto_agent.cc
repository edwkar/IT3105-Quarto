#include <algorithm>
#include <bitset>
#include <cassert>
#include <cstdio>
#include <ctime>
#include <iostream>
#include <queue>
#include <set>
#include <stdexcept>
#include <vector>


using namespace std;




struct quarto_state;
typedef vector<quarto_state> vqs;



const int A_LOT = 1 << 29;
const int ROW_COL[16][2] = {
  { 0, 0 }, { 0, 1 }, { 0, 2 }, { 0, 3 }, 
  { 1, 0 }, { 1, 1 }, { 1, 2 }, { 1, 3 }, 
  { 2, 0 }, { 2, 1 }, { 2, 2 }, { 2, 3 }, 
  { 3, 0 }, { 3, 1 }, { 3, 2 }, { 3, 3 }
};
const int QUARTO_POSITIONS[10][4][2] = {
  {{ 0, 0 }, { 0, 1 }, { 0, 2 }, { 0, 3 }},
  {{ 1, 0 }, { 1, 1 }, { 1, 2 }, { 1, 3 }},
  {{ 2, 0 }, { 2, 1 }, { 2, 2 }, { 2, 3 }},
  {{ 3, 0 }, { 3, 1 }, { 3, 2 }, { 3, 3 }},

  {{ 0, 0 }, { 1, 0 }, { 2, 0 }, { 3, 0 }},
  {{ 0, 1 }, { 1, 1 }, { 2, 1 }, { 3, 1 }},
  {{ 0, 2 }, { 1, 2 }, { 2, 2 }, { 3, 2 }},
  {{ 0, 3 }, { 1, 3 }, { 2, 3 }, { 3, 3 }},

  {{ 0, 0 }, { 1, 1 }, { 2, 2 }, { 3, 3 }},
  {{ 0, 3 }, { 1, 2 }, { 2, 1 }, { 3, 0 }}
};
const int NO_PIECE_TO_PLACE = -1;




/* Quarto state representation, with methods for successors generation and
 * state evaluation.
 */
struct quarto_state {
  bitset<16*4> data;
  bitset<16> taken;
  int _currentPiece;
  vqs _successors;
  bool hasSuccessorCached;


  quarto_state() : _currentPiece(NO_PIECE_TO_PLACE), hasSuccessorCached(false) {}


  quarto_state(const quarto_state& xs) :
    data(xs.data),
    taken(xs.taken),
    _currentPiece(xs._currentPiece),
    _successors(xs._successors),
    hasSuccessorCached(xs.hasSuccessorCached) {}


  vqs successors() {
    if (!hasSuccessorCached) {
      _successors = computeSuccessors();
      hasSuccessorCached = true;
    }
    return _successors;
  }


  vqs computeSuccessors() const;
  void setCurrentPiece(int x) { _currentPiece = x; }
  vector<int> availablePieces() const;


  typedef bitset<16*4>::reference ref;
  bool sizeAt (int row, int col) const { return data[row*4*4+col*4+0]; }
  ref  sizeAt (int row, int col)       { return data[row*4*4+col*4+0]; }
  bool shapeAt(int row, int col) const { return data[row*4*4+col*4+1]; }
  ref  shapeAt(int row, int col)       { return data[row*4*4+col*4+1]; }
  bool colorAt(int row, int col) const { return data[row*4*4+col*4+2]; }
  ref  colorAt(int row, int col)       { return data[row*4*4+col*4+2]; }
  bool holeAt (int row, int col) const { return data[row*4*4+col*4+3]; }
  ref  holeAt (int row, int col)       { return data[row*4*4+col*4+3]; }
  bool takenAt(int row, int col) const { return taken[row*4+col];      }
  bitset<16>::reference  takenAt(int row, int col) { return taken[row*4+col]; }


  int at(int row, int col) const { 
    return (sizeAt(row, col)  << 0) | (shapeAt(row, col) << 1) |
           (colorAt(row, col) << 2) | (holeAt(row, col)  << 3);
  }


  void insertAt(int row, int col, int newPiece) { 
    sizeAt(row, col)  = (newPiece&1) ? 1 : 0;
    shapeAt(row, col) = (newPiece&2) ? 1 : 0;
    colorAt(row, col) = (newPiece&4) ? 1 : 0;
    holeAt(row, col)  = (newPiece&8) ? 1 : 0;
    takenAt(row, col) = true;
  }


  bool isTerminal() const {
    if (score() == 10)
      return true;
    for (int i = 0; i < 16; ++i) 
      if (!taken[i])
        return false;
    return true;
  }


  int score() const;
  int scoreAfterPlacement() const;
};


vector<int> quarto_state::availablePieces() const {
  vector<bool> notAvail(16, false);
  for (int row = 0; row < 4; ++row) for (int col = 0; col < 4; ++col)
    if (takenAt(row, col)) 
      notAvail[ at(row, col) ] = true;

  vector<int> res; res.reserve(16);
  for (int x = 0; x < 16; ++x)
    if (!notAvail[x])
      res.push_back(x);
  return res;
}


vqs quarto_state::computeSuccessors() const {
  vector<int> avail = availablePieces();

  bool hasFirstMove = avail.size() == 16 && 
                        _currentPiece == NO_PIECE_TO_PLACE;
  if (hasFirstMove) {
    vqs res(1, *this);
    res[0].setCurrentPiece(0);
    return res;
  } 

  avail.erase(find(avail.begin(), avail.end(), _currentPiece));
  int numAvailPieces = avail.size();  

  vqs res; res.reserve(256);
  for (int row = 3; row >= 0; --row)
    for (int col = 0; col < 4; ++col) 
      if ( ! takenAt(row, col) ) {
        quarto_state succ(*this);
        succ.insertAt(row, col, _currentPiece);
        if (numAvailPieces == 0) {
          succ._currentPiece = NO_PIECE_TO_PLACE; // Last move.
          res.push_back(succ);
        } else 
          for (auto it = avail.begin(); it != avail.end(); ++it) {
            quarto_state ssucc(succ);
            ssucc.setCurrentPiece(*it);
            res.push_back(ssucc);
          }
      }

  return res;
}


int quarto_state::score() const {
  for (int seqi = 0; seqi < 10; ++seqi) {
    int numEmpty = 0, positives = _currentPiece,
                      negatives = _currentPiece;
    for (int i = 0; i < 4; ++i) {
      int row = QUARTO_POSITIONS[seqi][i][0], 
          col = QUARTO_POSITIONS[seqi][i][1];
      if (!takenAt(row, col)) {
        if (++numEmpty > 1)
          goto NEXT;
      } else {
        positives &= at(row, col);
        negatives |= at(row, col);
      }
    }

    if (numEmpty == 1 && (positives != 0 || negatives != 15))
      return 10;

    NEXT: ;
  }

  auto ap = availablePieces();
  for (int seqi = 0; seqi < 10; ++seqi) {
    int numEmpty = 0, positives = (1<<4)-1,
                      negatives = 0;
    for (int i = 0; i < 4; ++i) {
      int row = QUARTO_POSITIONS[seqi][i][0], 
          col = QUARTO_POSITIONS[seqi][i][1];
      if (!takenAt(row, col))
        ++numEmpty;
      else {
        positives &= at(row, col);
        negatives |= at(row, col);
      }
    }

    if (numEmpty >= 1 && numEmpty < 4) { 
      for (int prop = 0; prop < 4; ++prop) {
        if ((positives&(1<<prop))) {
          int n = 0;
          for (auto it = ap.begin(); it != ap.end(); ++it) 
            if ((1<<prop)&(*it))
              n++;
          if (numEmpty == n && ((1<<prop)&_currentPiece)) 
            return 5;
        }

        if ((negatives&(1<<prop)) == 0) {
          int n = 0;
          for (auto it = ap.begin(); it != ap.end(); ++it) 
            if (((1<<prop)&(*it)) == 0)
              n++;
          if (numEmpty == n && ((1<<prop)&_currentPiece)==0) 
            return 5;
        }
      }
    }
  }
  return 0;
}


int quarto_state::scoreAfterPlacement() const {
  for (int seqi = 0; seqi < 10; ++seqi) {
    int positives = (1<<4)-1, 
                    negatives = 0;
    for (int i = 0; i < 4; ++i) {
      int row = QUARTO_POSITIONS[seqi][i][0], 
          col = QUARTO_POSITIONS[seqi][i][1];
      if (!takenAt(row, col)) 
        goto NEXT;
      positives &= at(row, col);
      negatives |= at(row, col);
    }

    if (positives != 0 || negatives != 15)
      return 10;
    NEXT: ;
  }

  return 0;
}





/* Agent logic.
 */

struct agent_move {
  int row, col, currentPiece;
  agent_move() {}
  agent_move(int row, int col, int currentPiece) :
    row(row), col(col), currentPiece(currentPiece) {}
};
agent_move difference(const quarto_state& a, const quarto_state& b) {
  for (int row = 0; row < 4; ++row) for (int col = 0; col < 4; ++col)
    if ( b.takenAt(row, col) && !a.takenAt(row, col) ) 
      return agent_move(row, col, b._currentPiece);
  throw logic_error("should never happen");
}


int minMaxDepth;
quarto_state minimaxBestSucc;

int abMinValue(quarto_state node, int depth, 
              int alpha, int beta);

int abMaxValue(quarto_state node, int depth, 
              int alpha, int beta) {
  auto ap = node.availablePieces();

  if (depth == minMaxDepth || node.isTerminal()) 
    return node.score();

  int v = -A_LOT;
  auto succs = node.successors();  
  for (auto it = succs.begin(); it != succs.end(); ++it) {
    int score = abMinValue(*it, depth+1, alpha, beta);
    if (score > v) {
      v = score;
      if (depth == 0) 
        minimaxBestSucc = *it;
    }
    if (v >= beta)
      return v;
    alpha = max(alpha, v);
  }

  return v;
}

int abMinValue(quarto_state node, int depth, 
              int alpha, int beta) {
  if (depth == minMaxDepth || node.isTerminal()) 
    return -node.score();

  int v = A_LOT;
  auto succs = node.successors();  
  for (auto it = succs.begin(); it != succs.end(); ++it) {
    v = min(v, abMaxValue(*it, depth+1, alpha, beta));
    if (v <= alpha)
      return v;
    beta = min(beta, v);
  }

  return v;
}



agent_move nextNovice(quarto_state& s);
agent_move nextRandom(quarto_state& s);

/* Minimax agent. Plays the novice strategy when there are more than 10 pieces
 * left on the board, otherwise plays a best strategy available at search depth
 * [minMaxDepth].
 */
agent_move nextMinimax(quarto_state& s) {
  auto ap = s.availablePieces();
  if (ap.size() > 10)
    return nextNovice(s);

  if (s.isTerminal()) {
    auto succs = s.successors();
    for (auto it = succs.begin(); it != succs.end(); ++it) 
      if (it->scoreAfterPlacement() == 10) {
        minimaxBestSucc = *it;
        break;
      }
  } else 
    abMaxValue(s, 0, -1000, +1000);
  
  return difference(s, minimaxBestSucc);
}


/* Novice agent. Choose a winning move, if there exists one, otherwise
 * seemingly worst move with respect to opponent.
 */
agent_move nextNovice(quarto_state& s) {
  auto succs = s.successors();
  if (s.score() == 10) 
    for (auto it = succs.begin(); it != succs.end(); ++it) 
      if (it->scoreAfterPlacement() == 10)
        return difference(s, *it);

  random_shuffle(succs.begin(), succs.end());
  for (auto it = succs.begin(); it != succs.end(); ++it) 
    if (it->score() <= 5)
      return difference(s, *it);
  return difference(s, succs.at(0));
}

/* Random agent. Chooses a random move among all successors.
 */
agent_move nextRandom(quarto_state& s) {
  auto ap = s.availablePieces();
  auto succs = s.successors();
  random_shuffle(succs.begin(), succs.end());
  return difference(s, succs.at(0));
}


int I(){ 
  int n; 
  // A bit hackish. Since the game runner does not signal when we're done, we
  // just exit on the first EOF-producing scanf...
  if ((scanf("%d", &n)) == EOF) 
    exit(0);
  return n; 
}
quarto_state readCurrentState() {
  quarto_state s;
  for (int row = 0; row < 4; ++row) for (int col = 0, p; col < 4; ++col) 
    if ((p = I()) != NO_PIECE_TO_PLACE) {
      assert(p >= 0 && p < 16);
      s.insertAt(row, col, p);
    }
  int p;
  if ((p = I()) != NO_PIECE_TO_PLACE)
  s.setCurrentPiece(p);
  return s;
}


int main(int argc, char *argv[]) {
  srand(42); 

  string strategy(argv[1]);
  if (argc == 3 && strategy == "-minimax") 
    minMaxDepth = atoi(argv[2]);
  else if (argc == 2) {
    if (strategy != "-novice" && strategy != "-random") {
      cout << "Unknown strategy!\n";
      exit(-1);
    }
    minMaxDepth = -1;
  } else {
    printf("Usage: %s (-minimax [depth]|-novice|-random)\n", argv[0]);
    exit(-1);
  }

  for (;;) {
    quarto_state s = readCurrentState();
    auto ap = s.availablePieces();
    bool isFirstRoundState = 
            s._currentPiece == NO_PIECE_TO_PLACE && ap.size() == 16;

    agent_move firstRoundChoice(-1, -1, 0);
    agent_move c = isFirstRoundState     ? firstRoundChoice :
                   strategy == "-minimax" ? nextMinimax(s) :
                   strategy == "-novice"  ? nextNovice(s)  :
                                            nextRandom(s);
    printf("%d %d %d\n", c.row, c.col, c.currentPiece);
    fflush(stdout);
  }

  return 0;
}
