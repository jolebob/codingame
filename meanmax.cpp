#include <algorithm>
#include <climits>
#include <cmath>
#include <iostream>
#include <string>
#include <vector>

using namespace std;

typedef pair<int, int> Location;
typedef pair<int, int> Speed;

struct Unit {
  int      id;
  int      type;
  int      playerId;
  int      mass;
  int      radius;
  Location loc;
  Speed    speed;
  int      nbWater;
  int      waterCapacity;

  Unit()
      : id(-1),
        type(0),
        playerId(0),
        mass(0),
        radius(0),
        loc(Location(0, 0)),
        speed(Speed(0, 0)),
        nbWater(0),
        waterCapacity(0) {}
  Unit(int i, int t, int p, int m, int r, int x, int y, int vx, int vy, int e, int e2)
      : id(i),
        type(t),
        playerId(p),
        mass(m),
        radius(r),
        loc(Location(x, y)),
        speed(Speed(vx, vy)),
        nbWater(e),
        waterCapacity(e2) {}

  int distance(Unit u) { return pow(loc.first - u.loc.first, 2) + pow(loc.second - u.loc.second, 2); }
  void goTo(Unit u, bool fullSpeed = false) {
    int dist = this->distance(u);
    int acc  = 300;
    if (!fullSpeed) {
      if (dist < 600) acc = 150;
      if (dist < 400) acc = 0;
    }
    cout << u.loc.first - speed.first << " " << u.loc.second - speed.second << " " << acc << endl;
  }
  void print() {
    cerr << id << " " << type << " " << playerId << " " << mass << " " << radius << " " << loc.first << " "
         << loc.second << " " << speed.first << " " << speed.second << " " << nbWater << " " << waterCapacity << endl;
  }
};

int main() {
  // game loop
  while (1) {
    int myScore;
    cin >> myScore;
    cin.ignore();
    int enemyScore1;
    cin >> enemyScore1;
    cin.ignore();
    int enemyScore2;
    cin >> enemyScore2;
    cin.ignore();
    int myRage;
    cin >> myRage;
    cin.ignore();
    int enemyRage1;
    cin >> enemyRage1;
    cin.ignore();
    int enemyRage2;
    cin >> enemyRage2;
    cin.ignore();
    int unitCount;
    cin >> unitCount;
    cin.ignore();
    vector<Unit> allUnits;
    Unit         myReaper, myDestroyer, myDoof;
    Unit         otherReaper1, otherDestroyer1, otherDoof1;
    Unit         otherReaper2, otherDestroyer2, otherDoof2;
    vector<Unit> tankers;
    vector<Unit> wrecks;
    for (int i = 0; i < unitCount; i++) {
      int   unitId;
      int   unitType;
      int   player;
      float mass;
      int   radius;
      int   x;
      int   y;
      int   vx;
      int   vy;
      int   nbWater;
      int   waterCapacity;
      cin >> unitId >> unitType >> player >> mass >> radius >> x >> y >> vx >> vy >> nbWater >> waterCapacity;
      cin.ignore();
      allUnits.push_back(Unit(unitId, unitType, player, mass, radius, x, y, vx, vy, nbWater, waterCapacity));
      allUnits[i].print();
      if (allUnits[i].playerId == 0) {
        if (allUnits[i].type == 0) {
          myReaper = allUnits[i];
        } else if (allUnits[i].type == 1) {
          myDestroyer = allUnits[i];
        } else if (allUnits[i].type == 2) {
          myDoof = allUnits[i];
        }
      } else if (allUnits[i].playerId == 1) {
        if (allUnits[i].type == 0) {
          otherReaper1 = allUnits[i];
        } else if (allUnits[i].type == 1) {
          otherDestroyer1 = allUnits[i];
        } else if (allUnits[i].type == 2) {
          otherDoof1 = allUnits[i];
        }
      } else if (allUnits[i].playerId == 2) {
        if (allUnits[i].type == 0) {
          otherReaper2 = allUnits[i];
        } else if (allUnits[i].type == 1) {
          otherDestroyer2 = allUnits[i];
        } else if (allUnits[i].type == 2) {
          otherDoof2 = allUnits[i];
        }
      } else {
        if (allUnits[i].type == 3) {
          tankers.push_back(allUnits[i]);
        } else if (allUnits[i].type == 4) {
          wrecks.push_back(allUnits[i]);
        }
      }
    }

    Unit closestWreck;
    int  minDist1 = INT_MAX;
    for (auto &wreck : wrecks) {
      int dist = myReaper.distance(wreck);
      if (minDist1 > dist) {
        minDist1     = dist;
        closestWreck = wreck;
      }
    }
    if (closestWreck.id != -1) {
      cerr << "closest wreck is ";
      closestWreck.print();
    }

    Unit closestTanker;
    int  minDist2 = INT_MAX;
    for (auto &tanker : tankers) {
      int dist = myReaper.distance(tanker);
      if (minDist2 > dist) {
        minDist2      = dist;
        closestTanker = tanker;
      }
    }
    if (closestTanker.id != -1) {
      cerr << "closest tanker is ";
      closestTanker.print();
    }

    Unit firstReaper = (enemyScore1 > enemyScore2 ? otherReaper1 : otherReaper2);

    // REAPER
    if (closestWreck.id == -1) {
      if (closestTanker.id == -1) {
        cout << "WAIT" << endl;
      } else {
        myReaper.goTo(closestTanker);
      }
    } else {
      myReaper.goTo(closestWreck);
    }

    // DESTROYER
    if (closestTanker.id == -1) {
      cout << "WAIT" << endl;
    } else {
      if (minDist2 < 1000 && myRage > 60) {
        cout << "GRENADES " << closestTanker.loc.first << " " << closestTanker.loc.second << endl;
      } else {
        myDestroyer.goTo(closestTanker);
      }
    }

    // DOOF
    myDoof.goTo(firstReaper, true);
  }
}
