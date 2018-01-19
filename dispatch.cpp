#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

using namespace std;

/**
 * Auto-generated code below aims at helping you parse
 * the standard input according to the problem statement.
 **/
int main() {
  int boxCount;
  cin >> boxCount;
  cin.ignore();
  for (int i = 0; i < boxCount; i++) {
    float weight;
    float volume;
    cin >> weight >> volume;
    cin.ignore();
  }

  // Write an action using cout. DON'T FORGET THE "<< endl"
  // To debug: cerr << "Debug messages..." << endl;

  cout << "0 0 0 0 0 ..." << endl;
}
