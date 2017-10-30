#include <algorithm>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

static bool input_debug = true;
static bool trace_debug = true;
static int  size;
static int  unitsPerPlayer;

enum actionType { MOVE_BUILD, PUSH_BUILD, ACTION_TYPE_MAX };
static vector<string> action2str = {"MOVE&BUILD", "PUSH&BUILD"};
static map<string, int> str2action = {{"MOVE&BUILD", MOVE_BUILD}, {"PUSH&BUILD", PUSH_BUILD}};
enum dir { N, NE, E, SE, S, SW, W, NW, DIR_MAX };
static vector<string> dir2str = {"N", "NE", "E", "SE", "S", "SW", "W", "NW"};
static map<string, int> str2dir = {
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

struct Cell {
  int height;
  int unitIdx;

  Cell() : height(-1), unitIdx(-1) {}

  bool isPlayable(void) { return height != -1; }
  bool isMoveable(int unitHeight) { return unitHeight >= height - 1 && height < 4 && unitIdx == -1; }
  bool isPushable(int unitHeight) { return unitHeight >= height - 1 && height < 4 && unitIdx != -1; }
  bool                isBuildable(void) { return height < 4                &&unitIdx == -1; }

  void build(void) { height++; }
  void assignUnit(int index) { unitIdx = index; }

  void display(void) {
    if (trace_debug)
      cerr << "height=" << height << ", unitIdx=" << unitIdx << ", isPlayable=" << (height != -1 ? "yes" : "no")
           << endl;
  }
};

struct Action {
  int atype;
  int index;
  int dir1;
  int dir2;
  int score;

  Action(int a, int i, int d1, int d2) : atype(a), index(i), dir1(d1), dir2(d2), score(0) {}
  bool operator<(const Action &a) const { return score > a.score; }
  string                       toString(void) {
    stringstream s;
    s << action2str[atype] << " " << index << " " << dir2str[dir1] << " " << dir2str[dir2];
    return s.str();
  }
};

struct GameState {
  int          myScore;
  int          otherScore;
  vector<Cell> board;
  vector<pair<int, int> > players;
  pair<int, int>          build;

  GameState()
      : myScore(0),
        otherScore(0),
        board(vector<Cell>(size * size, Cell())),
        players(vector<pair<int, int> >(2 * unitsPerPlayer, pair<int, int>(0, 0))) {}

  void applyAction(Action &action) {
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
        build = pair<int, int>(x, y);
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
        build = pair<int, int>(x, y);

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

  int evaluateState(bool myself) {
    int score = 10 * (myself ? myScore : otherScore);
    for (int i = (myself ? 0 : unitsPerPlayer); i < (myself ? unitsPerPlayer : 2 * unitsPerPlayer); i++) {
      int x                = players[i].first;
      int y                = players[i].second;
      int nbWiningCell     = 0;
      int nbAccessibleCell = 0;

      Cell &cell = board[x + size * y];
      // cell.display();

      // look at neighbors
      for (int x2 = max(0, x - 1); x2 < min(size, x + 2); x2++) {
        for (int y2 = max(0, y - 1); y2 < min(size, y + 2); y2++) {
          Cell &nextCell = board[x2 + size * y2];
          // cerr << "next cell at " << x2 << "," << y2 << " ";
          // nextCell.display();
          if (!nextCell.isPlayable()) {
            continue;
          }
          if (nextCell.isMoveable(cell.height)) {
            nbAccessibleCell++;
            if (nextCell.height == 3) {
              nbWiningCell++;
            }
          }
        }
      }

      score += 5 * nbWiningCell + nbAccessibleCell + cell.height;
    }
    return score;
  }

  int evaluateState2(void) {
    int score = 1000 * myScore;
    for (int i = 0; i < unitsPerPlayer; i++) {
      int   x          = players[i].first;
      int   y          = players[i].second;
      int   nbMoveable = 0;
      int   unitScore  = 0;
      Cell &cell       = board[x + size       *y];

      // look at neighbors
      for (int x2 = max(0, x - 1); x2 < min(size, x + 2); x2++) {
        for (int y2 = max(0, y - 1); y2 < min(size, y + 2); y2++) {
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
      }
      if (nbMoveable == 0) {
        continue;
      } else {
        unitScore += 100 * cell.height;
        if (cell.height == 3) {
          unitScore += 100;
        }
        score += unitScore;
      }
    }
    return score;
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
    }
  }
};

struct Input {
  GameState      state;
  vector<Action> actions;

  Input(GameState g) : state(g) {}

  void computePossibleActions(int firstUnit) {
    actions.clear();
    for (int unitId = firstUnit; unitId < firstUnit + unitsPerPlayer; unitId++) {
      int x = state.players[unitId].first;
      int y = state.players[unitId].second;
      if (x == -1 || y == -1) {
        continue;
      }
      int height = state.board[x + size * y].height;
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
        Cell &nextCell = state.board[x2 + size * y2];
        if (!nextCell.isPlayable()) continue;
        if (nextCell.isMoveable(height)) {
          // try and build from there
          for (int dir2 = 0; dir2 < DIR_MAX; dir2++) {
            int x3 = x2, y3 = y2;
            getNewPosition(x3, y3, dir2);
            if (x3 == -1) {
              // invalid position
              continue;
            }
            Cell &nextNextCell = state.board[x3 + size * y3];
            if (!nextNextCell.isPlayable()) continue;
            if (nextNextCell.isBuildable() || (x3 == x && y3 == y)) {
              // We have a valide action here, add it to the list
              Action action = Action(MOVE_BUILD, unitId, dir1, dir2);
              actions.push_back(action);
              if (trace_debug) {
                //                cerr << x2 << " " << y2 << " ";
                //                nextCell.display();
                //                cerr << x3 << " " << y3 << " ";
                //                nextNextCell.display();
                cerr << action.toString() << endl;
              }
            }
          }
        }
        if (nextCell.isPushable(height)) {
          // try and push on adjacent cell
          for (int dir2 = dir1 - 1; dir2 < dir1 + 2; dir2++) {
            int x3 = x2, y3 = y2;
            getNewPosition(x3, y3, (dir2 == -1 ? 7 : (dir2 == 8 ? 0 : dir2)));
            if (x3 == -1) {
              // invalid position
              continue;
            }
            Cell &nextNextCell = state.board[x3 + size * y3];
            if (!nextNextCell.isPlayable()) continue;
            if (nextNextCell.isMoveable(nextCell.height)) {
              // We have a valide action here, add it to the list
              Action action = Action(PUSH_BUILD, unitId, dir1, dir2);
              actions.push_back(action);
              if (trace_debug) {
                //                cerr << x2 << " " << y2 << " ";
                //                nextCell.display();
                //                cerr << x3 << " " << y3 << " ";
                //                nextNextCell.display();
                cerr << action.toString() << endl;
              }
            }
          }
        }
      }  // end for dir1
    }    // end for unitId
  }

  void applyAllActions(void) {
    for (auto &action : actions) {
      if (trace_debug) cerr << action.toString() << endl;
      GameState tmpState = state;
      tmpState.applyAction(action);
      // tmpState.printState();
      // action.score = tmpState.evaluateState(true);  // evaluate for myself
      // if (trace_debug) cerr << "score=" << action.score << endl;
      action.score = tmpState.evaluateState2();  // evaluate for myself
      if (trace_debug) cerr << "score2=" << action.score << endl;
    }
  }

  Action findBestAction(void) {
    sort(actions.begin(), actions.end());
    return actions[0];
  }

  Action findWorstAction(void) {
    sort(actions.end(), actions.begin());
    return actions[0];
  }
};

/**
 * Auto-generated code below aims at helping you parse
 * the standard input according to the problem statement.
 **/
int main() {
  cin >> size;
  cin.ignore();
  cin >> unitsPerPlayer;
  cin.ignore();
  if (input_debug) cerr << size << " " << unitsPerPlayer << endl;
  GameState state = GameState();

  // game loop
  while (1) {
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
      }
    }
    for (int i = 0; i < 2 * unitsPerPlayer; i++) {
      cin >> state.players[i].first >> state.players[i].second;
      cin.ignore();
      if (input_debug) cerr << state.players[i].first << " " << state.players[i].second << endl;
      if (state.players[i].first != -1)
        state.board[state.players[i].first + size * state.players[i].second].unitIdx = i;
    }
    Input input = Input(state);

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
      input.actions.push_back(action);
    }

    if (input.actions.size() == 0) {
      cout << "I AM DEFEATED!" << endl;
      return 0;
    } else {
      input.applyAllActions();
      Action bestAction = input.findBestAction();
      cout << bestAction.toString() << endl;
    }
    // input.state.printState();
    // input.computePossibleActions(0);
  }
}
