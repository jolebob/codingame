#include <math.h>
#include <algorithm>
#include <chrono>
#include <climits>
#include <cmath>
#include <ctime>
#include <iostream>
#include <random>
#include <string>
#include <utility>
#include <vector>

#pragma GCC target("avx")
#pragma GCC optimize("O3")
#pragma GCC optimize("omit-frame-pointer")
#pragma GCC optimize("unroll-all-loops")
#pragma GCC optimize("inline")

#define MAX_VOLUME (100)
#define NB_TRUCK (100)
#define MAX_T (100)
#define SCORE_MULTIPLIER (20000)
#define MAX_TIME (49.5)
#define LOOP_TIME (16)

using namespace std;
using namespace std::chrono;

static double maxT_s            = MAX_T;
static double scoreMultiplier_s = SCORE_MULTIPLIER;
static double loopTime_s        = LOOP_TIME;

// To find a state with lower energy according to the given condition
template <typename state,
          typename count,
          typename energy_function,
          typename temperature_function,
          typename next_function,
          typename previous_function,
          typename counter_function>
state simulated_annealing(state                 &state_old,
                          count                 &c,
                          energy_function      &&energyFunction,
                          temperature_function &&temperatureFunction,
                          next_function        &&nextFunction,
                          previous_function    &&previousFunction,
                          counter_function     &&updateCounter) {
  random_device rd;
  mt19937_64    g(rd());

  auto energy_old = energyFunction(state_old);

  state state_best  = state_old;
  auto  energy_best = energy_old;

  uniform_real_distribution<decltype(energy_old)> rf(0, 1);

  for (; c > 0; updateCounter(state_old, c)) {
    nextFunction(state_old);
    auto energy_new = energyFunction(state_old);

    if (energy_new < energy_best) {
      state_best  = state_old;
      energy_best = energy_new;
      energy_old  = move(energy_new);
      continue;
    }

    auto t       = temperatureFunction(c);
    auto delta_e = energy_new - energy_old;

    if (delta_e > 10.0 * t) {
      previousFunction(state_old);
      continue;  // as std::exp(-10.0) is a very small number
    }

    if (delta_e < 0.0 || exp(-delta_e / t) > rf(g)) {
      energy_old = move(energy_new);
    } else {
      previousFunction(state_old);
    }
  }
  return (state_best);
}

auto tf = [](long double k) { return k * maxT_s; };

struct Box {
  int    index;
  double volume;
  double weight;
  Box(int i) : index(i), volume(0), weight(0) {}
  Box(int i, double w, double v) : index(i), volume(v), weight(w) {}
  bool operator<(const Box &a) const { return (weight < a.weight) || (weight == a.weight && volume > a.volume); }
};

struct Truck {
  int         index;
  double      volume;
  double      weight;
  vector<int> boxes;
  Truck(int i) : index(i), volume(0), weight(0) {}
  bool operator<(const Box &a) const { return (weight < a.weight) || (weight == a.weight && volume > a.volume); }
  void                      reset() {
    volume = 0;
    weight = 0;
    boxes.clear();
  }

  void insertBox(Box &box) {
    volume += box.volume;
    weight += box.weight;
    boxes.push_back(box.index);
  }
  void removeBox(Box &box) {
    vector<int>::iterator position = find(boxes.begin(), boxes.end(), box.index);
    if (position != boxes.end()) {
      boxes.erase(position);
      volume -= box.volume;
      weight -= box.weight;
    }
  }
  bool checkLimits(int avg, int sd) { return (volume <= MAX_VOLUME) && (weight <= avg + sd); }
  bool checkNewBox(Box &box) { return (volume + box.volume <= MAX_VOLUME); }
};

struct State {
  vector<Box>                       boxes;
  int                               nbBox;
  int                               nbTruck;
  double                            targetWeight;
  time_point<high_resolution_clock> start;
  vector<Truck>                     trucks;
  vector<int>                       boxPosition;
  int                               minIndex, maxIndex;
  vector<pair<int, int> > lastMoves;  // box index + origin truck index
  State(int b, int t, time_point<high_resolution_clock> s)
      : nbBox(b), nbTruck(t), targetWeight(0), start(s), minIndex(-1), maxIndex(-1) {}
  void addBox(Box &b) { boxes.push_back(b); }
  void insertBox(int boxIndex, int truckDest) {
    Box &box = boxes[boxIndex];
    trucks[truckDest].insertBox(box);
    boxPosition[boxIndex] = truckDest;
  }
  void moveBox(int boxIndex, int truckDest) {
    Box &box = boxes[boxIndex];
    trucks[boxPosition[boxIndex]].removeBox(box);
    trucks[truckDest].insertBox(box);
    boxPosition[boxIndex] = truckDest;
  }
  void reset() {
    for (auto &truck : trucks) {
      truck.reset();
    }
    for (auto &pos : boxPosition) {
      pos = -1;
    }
    minIndex = -1;
    maxIndex = -1;
    lastMoves.clear();
    start = high_resolution_clock::now();
  }
};

struct Action {
  vector<pair<int, int> > swapList;
};

void printState(State &train, int i);

double evaluate(State &train) {
  double minWeight = INT_MAX;
  double maxWeight = INT_MIN;
  double score     = 0;
  for (auto &truck : train.trucks) {
    score += fabs(truck.weight - train.targetWeight);
    if (truck.weight < minWeight) {
      minWeight      = truck.weight;
      train.minIndex = truck.index;
    }
    if (truck.weight > maxWeight) {
      maxWeight      = truck.weight;
      train.maxIndex = truck.index;
    }
  }
  score /= NB_TRUCK;
  return scoreMultiplier_s * (maxWeight - minWeight + score);
}

bool swapBoxes(State &train, int t1Index, int t2Index) {
  Truck &t1              = train.trucks[t1Index];
  Truck &t2              = train.trucks[t2Index];
  int    best_t1BoxIndex = -1;
  int    best_t2BoxIndex = -1;
  // double best_score      = fabs(t1.weight - train.targetWeight) + fabs(t2.weight - train.targetWeight);
  double best_score = t1.weight + t2.weight;

  if (t1.weight >= t2.weight) {
    for (auto &t1BoxIndex : t1.boxes) {
      if (t2.volume + train.boxes[t1BoxIndex].volume > MAX_VOLUME) continue;
      double score = fabs(t1.weight - train.boxes[t1BoxIndex].weight - train.targetWeight) +
                     fabs(t2.weight + train.boxes[t1BoxIndex].weight - train.targetWeight);
      if (score < best_score) {
        best_score      = score;
        best_t1BoxIndex = t1BoxIndex;
      }
    }
  } else {
    for (auto &t2BoxIndex : t2.boxes) {
      if (t1.volume + train.boxes[t2BoxIndex].volume > MAX_VOLUME) continue;
      double score = fabs(t1.weight + train.boxes[t2BoxIndex].weight - train.targetWeight) +
                     fabs(t2.weight - train.boxes[t2BoxIndex].weight - train.targetWeight);
      if (score < best_score) {
        best_score      = score;
        best_t2BoxIndex = t2BoxIndex;
      }
    }
  }
  for (auto &t1BoxIndex : t1.boxes) {
    for (auto &t2BoxIndex : t2.boxes) {
      if (t1.volume - train.boxes[t1BoxIndex].volume + train.boxes[t2BoxIndex].volume > MAX_VOLUME ||
          t2.volume - train.boxes[t2BoxIndex].volume + train.boxes[t1BoxIndex].volume > MAX_VOLUME)
        continue;
      double score =
          fabs(t1.weight - train.boxes[t1BoxIndex].weight + train.boxes[t2BoxIndex].weight - train.targetWeight) +
          fabs(t2.weight - train.boxes[t2BoxIndex].weight + train.boxes[t1BoxIndex].weight - train.targetWeight);
      if (score < best_score) {
        best_score      = score;
        best_t1BoxIndex = t1BoxIndex;
        best_t2BoxIndex = t2BoxIndex;
      }
    }
  }
  if (best_t1BoxIndex == -1 && best_t2BoxIndex == -1) return false;  // cannot do better...
  if (best_t1BoxIndex != -1) {
    train.moveBox(best_t1BoxIndex, t2Index);
    train.lastMoves.push_back(pair<int, int>(best_t1BoxIndex, t1Index));
  }
  if (best_t2BoxIndex != -1) {
    train.moveBox(best_t2BoxIndex, t1Index);
    train.lastMoves.push_back(pair<int, int>(best_t2BoxIndex, t2Index));
  }
  return true;
}

bool swapRandomBoxes(State &train, int t1Index, int t2Index) {
  Truck &t1 = train.trucks[t1Index];
  Truck &t2 = train.trucks[t2Index];
  vector<vector<pair<int, int> > > swapList;

  if (t1.weight >= t2.weight) {
    for (auto &t1BoxIndex : t1.boxes) {
      if (t2.volume + train.boxes[t1BoxIndex].volume > MAX_VOLUME) continue;
      vector<pair<int, int> > v;
      v.push_back(pair<int, int>(t1BoxIndex, t2Index));
      swapList.push_back(v);
    }
  } else {
    for (auto &t2BoxIndex : t2.boxes) {
      if (t1.volume + train.boxes[t2BoxIndex].volume > MAX_VOLUME) continue;
      vector<pair<int, int> > v;
      v.push_back(pair<int, int>(t2BoxIndex, t1Index));
      swapList.push_back(v);
    }
  }
  for (auto &t1BoxIndex : t1.boxes) {
    for (auto &t2BoxIndex : t2.boxes) {
      if (t1.volume - train.boxes[t1BoxIndex].volume + train.boxes[t2BoxIndex].volume > MAX_VOLUME ||
          t2.volume - train.boxes[t2BoxIndex].volume + train.boxes[t1BoxIndex].volume > MAX_VOLUME)
        continue;
      vector<pair<int, int> > v;
      v.push_back(pair<int, int>(t1BoxIndex, t2Index));
      v.push_back(pair<int, int>(t2BoxIndex, t1Index));
      swapList.push_back(v);
    }
  }
  if (swapList.size() == 0) return false;  // cannot do better...
  int i = rand() % swapList.size();
  for (auto &swap : swapList[i]) {
    train.moveBox(swap.first, swap.second);
    train.lastMoves.push_back(swap);
  }
  return true;
}

bool undoSwaps(State &train) {
  for (unsigned i = train.lastMoves.size(); i-- > 0;) {
    auto &swap = train.lastMoves[i];
    train.moveBox(swap.first, swap.second);
  }
}

void nextState(State &train) {
  static random_device              rd;
  static mt19937_64                 g(rd());
  static uniform_int_distribution<> d(0, NB_TRUCK - 1);
  // try and swap from two random trucks
  train.lastMoves.clear();
  bool swapResult = false;
  while (!swapResult) {
    int t1Index = d(g);
    int t2Index = d(g);
    swapResult  = swapBoxes(train, t1Index, t2Index);
  }
}

void updateCounter(State &train, long double &c) {
  time_point<high_resolution_clock> end     = high_resolution_clock::now();
  duration<double>                  elapsed = end - train.start;
  c                                         = (1.0 - elapsed.count() / loopTime_s);
}

void basicInit(State &train) {
  int truckIndex = 0;
  for (auto &box : train.boxes) {
    if (!train.trucks[truckIndex].checkNewBox(box)) {
      truckIndex++;
    }
    train.insertBox(box.index, truckIndex);
  }
}

void randInit(State &train) {
  int truckIndex;
  for (auto &box : train.boxes) {
    truckIndex = rand() % (NB_TRUCK);
    while (!train.trucks[truckIndex].checkNewBox(box)) {
      truckIndex = rand() % (NB_TRUCK);
    }
    train.insertBox(box.index, truckIndex);
  }
}

void printState(State &train, int i) {
  cerr << i << "," << train.minIndex << "," << train.trucks[train.minIndex].weight << "," << train.maxIndex << ","
       << train.trucks[train.maxIndex].weight << ","
       << (train.trucks[train.maxIndex].weight - train.trucks[train.minIndex].weight) << endl;
}

void readInputs(State &train) {
  double targetWeight = 0;
  double avgVolume    = 0;
  for (int i = 0; i < NB_TRUCK; ++i) {
    train.trucks.push_back(Truck(i));
  }
  for (int i = 0; i < train.nbBox; i++) {
    double weight;
    double volume;
    cin >> weight >> volume;
    cin.ignore();
    Box b(i, weight, volume);
    train.addBox(b);
    train.boxPosition.push_back(-1);
    targetWeight += weight;
    avgVolume += volume;
  }
  targetWeight /= NB_TRUCK;
  train.targetWeight = targetWeight;
  cerr << "STATS: target weight=" << targetWeight << ", average volume=" << avgVolume / NB_TRUCK << endl << endl;
}

void swapMinMax(State &train) {
  bool   swapResult = true;
  int    i          = 0;
  double score      = evaluate(train);
  for (; i < 200 && swapResult; i++) {
    swapResult = swapBoxes(train, train.minIndex, train.maxIndex);
    score      = evaluate(train);
    if (!swapResult) break;
  }
}

int main(int argc, char const *argv[]) {
  time_point<high_resolution_clock> start, end;
  start = high_resolution_clock::now();

  if (argc >= 3) {
    maxT_s            = atof(argv[1]);
    scoreMultiplier_s = atof(argv[2]);
  }
  if (argc == 4) {
    loopTime_s = atof(argv[3]);
  }

  cerr << "working with Tmax=" << maxT_s << ", ScoreX=" << scoreMultiplier_s << ", loopTime=" << loopTime_s << "s"
       << endl;

  int boxCount;
  cin >> boxCount;
  cin.ignore();

  State train(boxCount, NB_TRUCK, start);
  readInputs(train);

  /**************
   * BASIC MODE *
   **************/
  basicInit(train);
  swapMinMax(train);
  // Simulated annealing
  long double count      = 1.0;
  State       best_train = simulated_annealing(train, count, evaluate, tf, nextState, undoSwaps, updateCounter);
  double      best_score = evaluate(best_train);
  printState(best_train, count);
  end                         = high_resolution_clock::now();
  duration<double> elapsed_ms = end - start;

  while (MAX_TIME - elapsed_ms.count() > loopTime_s) {
    train.reset();
    randInit(train);
    swapMinMax(train);
    // Simulated annealing
    long double count     = 1.0;
    State       new_train = simulated_annealing(train, count, evaluate, tf, nextState, undoSwaps, updateCounter);
    double      new_score = evaluate(new_train);
    printState(new_train, count);

    if (new_score < best_score) {
      best_score = new_score;
      best_train = move(new_train);
    }

    end        = high_resolution_clock::now();
    elapsed_ms = end - start;
  }

  for (auto &pos : best_train.boxPosition) {
    cout << pos << " ";
  }
  cout << endl;

  printState(best_train, count);
  cerr << "SCORE=" << (best_train.trucks[best_train.maxIndex].weight - best_train.trucks[best_train.minIndex].weight)
       << endl;
  end        = high_resolution_clock::now();
  elapsed_ms = end - start;
  cerr << "duration: " << elapsed_ms.count() << " s" << endl;
}
