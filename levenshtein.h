#pragma once

#include <string>
#include <vector>

namespace util {

struct LevenshteinHelper {
  LevenshteinHelper(std::string a, std::string b) : strA(a), strB(b)
  {
    int cols = strA.length();
    int rows = strB.length();
    M.resize((rows+1)*(cols+1));
    for (int i=0; i<rows+1; ++i) {
      this->at(i,0) = i;
    }
    for (int j=1; j<cols+1; ++j) {
      this->at(0,j) = j;
    }
    for (int i=1; i<rows+1; ++i) {
      for (int j=1; j<cols+1; ++j) {
        this->at(i,j) = Lev(i,j);
      }
    }
  }

  int Lev(int i, int j) {
    if (std::min(i,j)==0) return 0;
    int a = at(i-1,j)+1;
    int b = at(i,j-1)+1;
    int c = at(i-1,j-1) + (strA[i-1] != strB[j-1] ? 1 : 0);
    return std::min(std::min(a,b),c);
  }

  int &at(int i, int j)
  { return M[i*strA.size()+j]; }

  int &operator()(int i, int j)
  { return M[i*strA.size()+j]; }

  std::vector<int> M;
  std::string strA, strB;
};

int levenshteinDistance(std::string a, std::string b) {

  LevenshteinHelper helper(a,b);
  return helper(b.length(),a.length());
}

} // namespace util
