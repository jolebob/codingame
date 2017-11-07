#include <algorithm>
#include <climits>
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

struct Action {
  int score;

  Action() : score(0) {}
  bool operator<(const Action &a) const { return score < a.score; }
  bool operator<=(const Action &a) const { return score <= a.score; }
  string                        toString(void) {
    stringstream s;
    s << score;
    return s.str();
  }
  void print() { cout << this->toString() << endl; }
};

struct State {
  int actionScore;
  int myScore;
  int otherScore;

  State() : actionScore(0), myScore(0), otherScore(0) {}

  bool operator<(const State &a) const { return actionScore < a.actionScore; }

  void resetScore() {
    actionScore = 0;
    myScore     = 0;
    otherScore  = 0;
  }

  void computePossibleActions(vector<Action> &actions, bool myTurn) {}

  void applyAction(Action &action) {}

  void undoAction(Action &action) {}

  void evaluateState(void) {}

  void printState(void) {}
};

struct AlphaBeta {
  int            depthMax;
  vector<Action> initialActions;
  AlphaBeta() : depthMax(0) {}
  AlphaBeta(int d) : depthMax(d) {}

  void doAlphaBeta(
      State &state, int depth, int alpha, int beta, bool myTurn, bool &foundBestAction, Action &bestAction) {
    vector<Action> actions;
    Action         nextAction;
    foundBestAction = false;

    // In case first actions are given in input by the referee; otherwise, we should compute them
    if (depth > 1) {
      state.computePossibleActions(actions, myTurn);
      if (actions.size() == 0) {
        // Go and explore further below to find best action for the next player
        if (depth < depthMax) {
          doAlphaBeta(state, depth + 1, alpha, beta, !myTurn, foundBestAction, bestAction);
        }
        // no possible actions
        return;
      }
    }

    for (auto &action : (depth == 1 ? initialActions : actions)) {
      state.actionScore = 0;
      state.applyAction(action);

      if (depth == depthMax) {
        // max depth => evaluate action result's score
        state.evaluateState();
        action.score = state.actionScore;
      } else {
        // get next best action (recursive call)
        bool foundNextBestAction = false;
        doAlphaBeta(state, depth + 1, alpha, beta, !myTurn, foundNextBestAction, nextAction);
        if (!foundNextBestAction) {
          // no next best action (no possible move for next player for instance)
          // => we evaluate the current state's score
          state.evaluateState();
          action.score = state.actionScore;
        } else {
          // we keep the next best action score
          action.score = nextAction.score;
        }
      }

      // this helps avoiding copying state object all the time
      state.undoAction(action);

      if (!foundBestAction || (myTurn && bestAction < action) || (!myTurn && action < bestAction)) {
        foundBestAction = true;
        bestAction      = action;
      }

      // Alpha Beta pruning is here !
      if ((myTurn && beta < bestAction.score) || (!myTurn && bestAction.score < alpha)) {
        break;
      }

      // update alpha and beta
      if (myTurn && alpha < bestAction.score) alpha = bestAction.score;
      if (!myTurn && bestAction.score < beta) beta  = bestAction.score;
    }
  }
};

/**
 * Auto-generated code below aims at helping you parse
 * the standard input according to the problem statement.
 **/
int main() {
  int turn = 0;

  State     state     = State();
  AlphaBeta alphaBeta = AlphaBeta(DEPTH);

  // game loop
  while (1) {
    turn++;

    // INITIALISE STATE

    // RETRIEVE OR COMPUTE ACTIONS

    state.resetScore();
    Action bestAction;
    bool   foundBest = false;
    alphaBeta.doAlphaBeta(state, 1, INT_MIN, INT_MAX, true, foundBest, bestAction);
    if (!foundBest) {
      // DO SOMETHING LIKE
      cout << "ACCEPT-DEFEAT" << endl;
    } else {
      cerr << "SCORE=" << bestAction.score << endl;
      bestAction.print();
    }
  }
}
