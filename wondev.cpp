#include <algorithm>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#pragma GCC target("avx")
#pragma GCC optimize("O3")
#pragma GCC optimize("omit-frame-pointer")
#pragma GCC optimize("unroll-all-loops")
#pragma GCC optimize("inline")

using namespace std;

#define DEPTH (3)

static bool input_debug = true;
static bool trace_debug = false;
static int  size;
static int  unitsPerPlayer;

enum actionType { MOVE_BUILD, PUSH_BUILD, ACTION_TYPE_MAX };
static const vector<string> action2str = {"MOVE&BUILD", "PUSH&BUILD"};
static const map<string, int> str2action = {{"MOVE&BUILD", MOVE_BUILD}, {"PUSH&BUILD", PUSH_BUILD}};
enum dir { N, NE, E, SE, S, SW, W, NW, DIR_MAX };
static const vector<string> dir2str = {"N", "NE", "E", "SE", "S", "SW", "W", "NW"};
static const map<string, int> str2dir = {
    {"N", N}, {"NE", NE}, {"E", E}, {"SE", SE}, {"S", S}, {"SW", SW}, {"W", W}, {"NW", NW}};

int convertStringIntoActionType(string s) { return str2action.at(s); }
int convertStringIntoDirection(string d) { return str2dir.at(d); }

void getNewPosition(int &x, int &y, int dir) {
  switch (dir) {
    case N:
      y--;
      break;
    case NE:
      x++;
      y--;
      break;
    case E:
      x++;
      break;
    case SE:
      x++;
      y++;
      break;
    case S:
      y++;
      break;
    case SW:
      x--;
      y++;
      break;
    case W:
      x--;
      break;
    case NW:
      x--;
      y--;
      break;
    default:
      break;
  }
  if (x < 0 || x >= size || y < 0 || y >= size) {
    x = -1;
    y = -1;
  }
}

void getPreviousPosition(int &x, int &y, int dir) {
  switch (dir) {
    case N:
      y++;
      break;
    case NE:
      x--;
      y++;
      break;
    case E:
      x--;
      break;
    case SE:
      x--;
      y--;
      break;
    case S:
      y--;
      break;
    case SW:
      x++;
      y--;
      break;
    case W:
      x++;
      break;
    case NW:
      x++;
      y++;
      break;
    default:
      break;
  }
  if (x < 0 || x >= size || y < 0 || y >= size) {
    x = -1;
    y = -1;
  }
}

struct Cell {
  int height;
  int unitIdx;

  Cell() : height(-1), unitIdx(-1) {}

  bool isPlayable(void) { return height != -1; }
  bool isMoveable(int unitHeight) { return unitHeight >= height - 1 && height < 4 && unitIdx == -1; }
  bool isPushable(int unitHeight) { return unitHeight >= height - 1 && height < 4 && unitIdx != -1; }
  bool                isBuildable(void) { return height < 4                &&unitIdx == -1; }

  void build(void) { height++; }
  void unbuild(void) { height--; }
  void assignUnit(int index) { unitIdx = index; }

  void display(void) {
    if (trace_debug)
      cerr << "height=" << height << ", unitIdx=" << unitIdx << ", isPlayable=" << (height != -1 ? "yes" : "no")
           << endl;
  }
};

struct Action {
  int score;
  int atype;
  int index;
  int dir1;
  int dir2;

  Action() : score(0), atype(0), index(0), dir1(0), dir2(0) {}
  Action(int a, int i, int d1, int d2) : score(0), atype(a), index(i), dir1(d1), dir2(d2) {}
  bool operator<(const Action &a) const { return score < a.score; }
  bool operator<=(const Action &a) const { return score <= a.score; }
  string                        toString(void) {
    stringstream s;
    s << action2str[atype] << " " << index << " " << dir2str[dir1] << " " << dir2str[dir2] << " => " << score;
    return s.str();
  }
  void print() { cout << this->toString() << endl; }
};

struct State {
  int          actionScore;
  int          myScore;
  int          otherScore;
  vector<Cell> board;
  vector<pair<int, int> > players;

  State()
      : actionScore(0),
        myScore(0),
        otherScore(0),
        board(vector<Cell>(size * size, Cell())),
        players(vector<pair<int, int> >(2 * unitsPerPlayer, pair<int, int>(0, 0))) {}

  bool operator<(const State &a) const { return actionScore < a.actionScore; }

  void resetScore() {
    actionScore = 0;
    myScore     = 0;
    otherScore  = 0;
  }

  void computePossibleActions(vector<Action> &actions, bool myTurn) {
    int firstUnit = (myTurn ? 0 : unitsPerPlayer);
    actions.clear();
    for (int unitId = firstUnit; unitId < firstUnit + unitsPerPlayer; unitId++) {
      int x = players[unitId].first;
      int y = players[unitId].second;
      if (x == -1 || y == -1) {
        continue;
      }
      int height = board[x + size * y].height;
      //      if (trace_debug) {
      //        cerr << "TREATING " << x << " " << y << " ";
      //        state.board[x + size * y].display();
      //      }
      // Generate all possible actions for this unit
      // look at neighbors
      for (int dir1 = 0; dir1 < DIR_MAX; dir1++) {
        int x2 = x, y2 = y;
        getNewPosition(x2, y2, dir1);
        if (x2 == -1) {
          // invalid position
          continue;
        }
        Cell &nextCell = board[x2 + size * y2];
        if (!nextCell.isPlayable()) continue;
        if (nextCell.isMoveable(height)) {
          // Ignore cell if it has 0 accessible neighbors
          vector<int> accessibleNeighbors;
          for (int dir2 = 0; dir2 < DIR_MAX; dir2++) {
            int x3 = x2, y3 = y2;
            getNewPosition(x3, y3, dir2);
            if (x3 == -1) {
              // invalid position
              continue;
            }
            Cell &nextNextCell = board[x3 + size * y3];
            if (!nextNextCell.isPlayable()) continue;
            if (nextNextCell.isMoveable(nextCell.height)) accessibleNeighbors.push_back(dir2);
          }
          if (accessibleNeighbors.size() == 0) continue;
          // try and build from there
          for (auto dir2 : accessibleNeighbors) {
            int x3 = x2, y3 = y2;
            getNewPosition(x3, y3, dir2);
            Cell &nextNextCell = board[x3 + size * y3];
            if (nextNextCell.isBuildable() || (x3 == x && y3 == y)) {
              // We have a valide action here, add it to the list
              Action action = Action(MOVE_BUILD, unitId, dir1, dir2);
              actions.push_back(action);
              if (trace_debug) {
                //                cerr << x2 << " " << y2 << " ";
                //                nextCell.display();
                //                cerr << x3 << " " << y3 << " ";
                //                nextNextCell.display();
                // cerr << action.toString() << endl;
              }
            }
          }
        }
        if (nextCell.isPushable(height)) {
          // try and push on adjacent cell
          for (int dir2 = dir1 - 1; dir2 < dir1 + 2; dir2++) {
            int x3 = x2, y3 = y2;
            int realDir2 = (dir2 == -1 ? 7 : (dir2 == 8 ? 0 : dir2));
            getNewPosition(x3, y3, realDir2);
            if (x3 == -1) {
              // invalid position
              continue;
            }
            // cerr << "x3,y3=" << x3 << "," << y3 << endl;
            Cell &nextNextCell = board[x3 + size * y3];
            if (!nextNextCell.isPlayable()) continue;
            if (nextNextCell.isMoveable(nextCell.height)) {
              // We have a valide action here, add it to the list
              Action action = Action(PUSH_BUILD, unitId, dir1, realDir2);
              actions.push_back(action);
              if (trace_debug) {
                //                cerr << x2 << " " << y2 << " ";
                //                nextCell.display();
                //                cerr << x3 << " " << y3 << " ";
                //                nextNextCell.display();
                // cerr << action.toString() << endl;
              }
            }
          }
        }
      }  // end for dir1
    }    // end for unitId
  }

  void applyAction(Action &action) {
    // if (trace_debug) cerr << action.toString() << endl;
    switch (action.atype) {
      case MOVE_BUILD: {
        int x = players[action.index].first;
        int y = players[action.index].second;
        // move player
        // if (trace_debug) cerr << "move player at " << x << "," << y << " to " << dir2str[action.dir1] << endl;
        board[x + size * y].assignUnit(-1);
        getNewPosition(x, y, action.dir1);
        board[x + size * y].assignUnit(action.index);
        players[action.index].first  = x;
        players[action.index].second = y;
        if (board[x + size * y].height == 3) {
          if (action.index < unitsPerPlayer) {
            myScore++;
          } else {
            otherScore++;
          }
        }
        // if (trace_debug) cerr << "player now at " << x << "," << y << endl;
        // build
        getNewPosition(x, y, action.dir2);
        board[x + size * y].build();
        break;
      }
      case PUSH_BUILD: {
        int x = players[action.index].first;
        int y = players[action.index].second;
        // move other player
        // if (trace_debug) cerr << "move player at " << x << "," << y << " to " << dir2str[action.dir1] << endl;
        getNewPosition(x, y, action.dir1);
        int otherPlayerIndex = board[x + size * y].unitIdx;
        board[x + size * y].assignUnit(-1);
        board[x + size * y].build();

        getNewPosition(x, y, action.dir2);
        board[x + size * y].assignUnit(otherPlayerIndex);
        players[otherPlayerIndex].first  = x;
        players[otherPlayerIndex].second = y;

        break;
      }
      default:
        break;
    }
  }

  void undoAction(Action &action) {
    switch (action.atype) {
      case MOVE_BUILD: {
        int x = players[action.index].first;
        int y = players[action.index].second;
        // remove point
        if (board[x + size * y].height == 3) {
          if (action.index < unitsPerPlayer) {
            myScore--;
          } else {
            otherScore--;
          }
        }
        // unbuild
        int xb = x, yb = y;
        getNewPosition(xb, yb, action.dir2);
        board[xb + size * yb].unbuild();
        // move back player
        board[x + size * y].assignUnit(-1);
        getPreviousPosition(x, y, action.dir1);
        board[x + size * y].assignUnit(action.index);
        players[action.index].first  = x;
        players[action.index].second = y;
        break;
      }
      case PUSH_BUILD: {
        int x = players[action.index].first;
        int y = players[action.index].second;
        // unbuild
        getNewPosition(x, y, action.dir1);
        board[x + size * y].unbuild();
        // move back other player
        int xo = x, yo = y;
        getNewPosition(xo, yo, action.dir2);
        int otherPlayerIndex = board[xo + size * yo].unitIdx;
        board[xo + size * yo].assignUnit(-1);
        board[x + size * y].assignUnit(otherPlayerIndex);
        players[otherPlayerIndex].first  = x;
        players[otherPlayerIndex].second = y;
        break;
      }
      default:
        break;
    }
  }

  void evaluateState(void) {
    actionScore = 1000 * (myScore - otherScore);
    for (int i = 0; i < unitsPerPlayer; i++) {
      int   x          = players[i].first;
      int   y          = players[i].second;
      int   nbMoveable = 0;
      int   unitScore  = 0;
      Cell &cell       = board[x + size       *y];

      // look at neighbors
      for (int dir1 = 0; dir1 < DIR_MAX; dir1++) {
        int x2 = x, y2 = y;
        getNewPosition(x2, y2, dir1);
        if (x2 == -1) {
          // invalid position
          continue;
        }
        Cell &nextCell = board[x2 + size * y2];
        // cerr << "next cell at " << x2 << "," << y2 << " ";
        // nextCell.display();
        if (!nextCell.isPlayable()) {
          continue;
        }
        if (nextCell.isMoveable(cell.height)) {
          nbMoveable++;
          unitScore += 10 * (nextCell.height * nextCell.height + 1);
        }
      }
      if (nbMoveable == 0) {
        continue;
      } else {
        unitScore += 10000 + 100 * cell.height;
        actionScore += unitScore;
      }
    }
  }

  void printState(void) {
    if (trace_debug) {
      for (int i = 0; i < size; i++) {
        cerr << "[ ";
        for (int j = 0; j < size; j++) {
          cerr << "(" << (board[j + size * i].height >= 0 ? " " : "") << board[j + size * i].height << ","
               << (board[j + size * i].unitIdx >= 0 ? " " : "") << board[j + size * i].unitIdx << ") ";
        }
        cerr << "]" << endl;
      }
      cerr << "State scores: myScore=" << myScore << " otherScore=" << otherScore << " actionScore=" << actionScore
           << endl;
    }
  }
};

struct Minimax {
  int            depthMax;
  vector<Action> initialActions;
  Minimax() : depthMax(0) {}
  Minimax(int d) : depthMax(d) {}
  Action *doMinimax(State &state, int depth, bool myTurn) {
    // cerr << "DEPTH=" << depth << ", myTurn=" << myTurn << endl;
    vector<Action> actions;

    if (depth > 1) {
      state.computePossibleActions(actions, myTurn);
      if (actions.size() == 0) {
        // no possible actions
        return NULL;
      }
    }

    Action *bestAction = NULL;
    for (auto &action : (depth == 1 ? initialActions : actions)) {
      State nextState       = state;
      nextState.actionScore = 0;
      nextState.applyAction(action);
      // state.printState();

      if (depth == depthMax) {
        nextState.evaluateState();
        action.score = nextState.actionScore;
      } else {
        Action *nextAction = doMinimax(nextState, depth + 1, !myTurn);
        if (nextAction == NULL) {
          state.evaluateState();
          action.score = state.actionScore;
        } else {
          action.score = nextAction->score;
        }
      }
      if (bestAction == NULL || (myTurn && bestAction->score < action.score) ||
          (!myTurn && bestAction->score > action.score)) {
        bestAction = &action;
      }
      if (trace_debug) cerr << string(3 * depth, ' ') << action.toString() << endl;
    }
    return bestAction;
  }
};

struct AlphaBeta {
  int            depthMax;
  vector<Action> initialActions;
  AlphaBeta() : depthMax(0) {}
  AlphaBeta(int d) : depthMax(d) {}
  void          doAlphaBeta(
      State &state, int depth, int alpha, int beta, bool myTurn, bool &foundBestAction, Action &bestAction) {
    // cerr << "DEPTH=" << depth << ", myTurn=" << myTurn << endl;
    vector<Action> actions;
    Action         nextAction;
    foundBestAction = false;

    if (depth > 1) {
      state.computePossibleActions(actions, myTurn);
      if (actions.size() == 0) {
        // Go and explore further below to find best action for next player
        if (depth < depthMax) {
          doAlphaBeta(state, depth + 1, alpha, beta, !myTurn, foundBestAction, bestAction);
          if (trace_debug)
            cerr << string(3 * depth, ' ') << depth << ": best next action is " << bestAction.toString() << endl;
        }
        // no possible actions
        return;
      }
    }

    for (auto &action : (depth == 1 ? initialActions : actions)) {
      if (trace_debug) cerr << string(3 * depth, ' ') << action.toString() << endl;
      // State state       = state;
      state.actionScore = 0;
      state.applyAction(action);
      // state.printState();

      if (depth == depthMax) {
        state.evaluateState();
        action.score = state.actionScore;
      } else {
        bool foundNextBestAction = false;
        doAlphaBeta(state, depth + 1, alpha, beta, !myTurn, foundNextBestAction, nextAction);
        if (trace_debug)
          cerr << string(3 * depth, ' ') << depth << ": best next action is " << nextAction.toString() << endl;
        if (!foundNextBestAction) {
          state.evaluateState();
          action.score = state.actionScore;
        } else {
          action.score = nextAction.score;
        }
      }

      if (trace_debug) state.printState();
      state.undoAction(action);
      if (trace_debug) state.printState();

      if (!foundBestAction || (myTurn && bestAction < action) || (!myTurn && action < bestAction)) {
        foundBestAction = true;
        bestAction      = action;
        if (trace_debug)
          cerr << string(3 * depth, ' ') << depth << ": new best action is " << bestAction.toString() << endl;
      }

      if (trace_debug)
        cerr << string(3 * depth, ' ') << depth << ": best action so far is " << bestAction.toString() << endl;

      if ((myTurn && beta < bestAction.score) || (!myTurn && bestAction.score < alpha)) {
        if (trace_debug)
          cerr << string(3 * depth, ' ') << depth << ": " << action.toString() << " CUT !!! Alpha=" << alpha
               << " Beta=" << beta << endl;
        break;
      }
      if (myTurn && alpha < bestAction.score) alpha = bestAction.score;
      if (!myTurn && bestAction.score < beta) beta  = bestAction.score;
      if (trace_debug)
        cerr << string(3 * depth, ' ') << depth << ": " << action.toString() << " Alpha=" << alpha << " Beta=" << beta
             << endl;
    }
    if (trace_debug)
      cerr << string(3 * depth, ' ') << depth << ": return next action is " << bestAction.toString() << endl;
  }
};

/**
 * Auto-generated code below aims at helping you parse
 * the standard input according to the problem statement.
 **/
int main() {
  int turn = 0;

  cin >> size;
  cin.ignore();
  cin >> unitsPerPlayer;
  cin.ignore();
  if (input_debug) cerr << size << " " << unitsPerPlayer << endl;
  State     state     = State();
  AlphaBeta alphaBeta = AlphaBeta(DEPTH);

  // game loop
  while (1) {
    turn++;
    // if (turn == 25) trace_debug = true;
    for (int i = 0; i < size; i++) {
      string row;
      cin >> row;
      cin.ignore();
      if (input_debug) cerr << row << endl;
      for (int j = 0; j < size; j++) {
        if (row[j] == '.') {
          state.board[i * size + j].height = -1;
        } else {
          state.board[i * size + j].height = row[j] - '0';
        }
        state.board[i * size + j].unitIdx = -1;
      }
    }
    for (int i = 0; i < 2 * unitsPerPlayer; i++) {
      cin >> state.players[i].first >> state.players[i].second;
      cin.ignore();
      if (input_debug) cerr << state.players[i].first << " " << state.players[i].second << endl;
      if (state.players[i].first != -1)
        state.board[state.players[i].first + size * state.players[i].second].unitIdx = i;
    }

    alphaBeta.initialActions.clear();
    int nbLegalActions;
    cin >> nbLegalActions;
    cin.ignore();
    if (input_debug) cerr << nbLegalActions << endl;
    for (int i = 0; i < nbLegalActions; i++) {
      string atype;
      int    index;
      string dir1;
      string dir2;
      cin >> atype >> index >> dir1 >> dir2;
      cin.ignore();
      if (input_debug) cerr << atype << " " << index << " " << dir1 << " " << dir2 << endl;
      Action action = Action(convertStringIntoActionType(atype),
                             index,
                             convertStringIntoDirection(dir1),
                             convertStringIntoDirection(dir2));
      alphaBeta.initialActions.push_back(action);
    }

    if (alphaBeta.initialActions.size() == 0) {
      cout << "ACCEPT-DEFEAT" << endl;
      return 0;
    }

    int alpha = -100000, beta = 100000;
    state.resetScore();
    Action bestAction;
    bool   foundBest = false;
    alphaBeta.doAlphaBeta(state, 1, alpha, beta, true, foundBest, bestAction);
    if (!foundBest) {
      cout << "ACCEPT-DEFEAT" << endl;
    } else {
      cerr << "SCORE=" << bestAction.score << endl;
      bestAction.print();
    }
  }
}
