#include <iostream>
#include <string>
#include <cstring>
#include <stdio.h>
// #include "http_client.cpp"
#include "utils.hpp"

void test1() {
  char* test = "ht tp: / / www.goog\0le.com\n";
  string output = clean_string(test);
  if (output == "http://www.google.com\n") {
    cout << "----------------------Test1 pass---------------------" << endl; 
  } else {
    cout << "Expected: http://www.google.com\n, got " + output << endl;
  }
}

void TestIsValidProtocol() {
  string input1 = "http://www.google.com";
  if (IsValidProtocol(&input1)) {
    cout << input1 + " has valid protocol" << endl;
  } else {
    cout << "----------------------------TestIsValidProtocol Failed------------------------------" << endl;
  }
  if (IsValidProtocol(&input1)) {
    cout << input1 + " has valid protocol" << endl;
  } else {
    cout << "----------------------------TestIsValidProtocol Failed------------------------------" << endl;
  }
  
  string input2 = "htpt://-www.google.com";
  if (IsValidProtocol(&input2)) {
    cout << "----------------------------TestIsValidProtocol Failed------------------------------" << endl;
  } else {
    cout << input2 + " has invalid protocol" << endl;
  }
}

int main() {
  // test1();
  TestIsValidProtocol();
  return 0;
}