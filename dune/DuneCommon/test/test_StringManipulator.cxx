// test_StringManipulator.cxx

// David Adams
// April 2018
//
// This is a test and demonstration for StringManipulator.

#undef NDEBUG

#include "../StringManipulator.h"
#include <string>
#include <iostream>
#include <iomanip>
#include <vector>
#include <cassert>

using std::string;
using std::cout;
using std::endl;
using std::setw;
using std::vector;

using Index = unsigned int;
using StringVector = StringManipulator::StringVector;
using StringVV = std::vector<StringVector>;

//**********************************************************************

bool areEqual(const StringVector& lhs, const StringVector& rhs) {
  string myname = "areEqual: ";
  if ( lhs == rhs ) return true; 
  Index nlhs = lhs.size();
  Index nrhs = rhs.size();
  Index nstr = nlhs > nrhs ? nlhs : nrhs;
  cout << myname << "Unequal string vectors: " << endl;
  cout << myname << "  Vector lengths: " << nlhs << ", " << nrhs << endl;
  for ( Index istr=0; istr<nstr; ++istr ) {
    string slhs = istr < nlhs ? lhs[istr] : "";
    string srhs = istr < nrhs ? rhs[istr] : "";
    cout << myname << "    " << istr << ": " << slhs << ", " << srhs << endl;
  }
  return false;
}

//**********************************************************************

int test_StringManipulator(Index logLevel) {
  const string myname = "test_StringManipulator: ";
  cout << myname << "Starting test" << endl;
#ifdef NDEBUG
  cout << myname << "NDEBUG must be off." << endl;
  abort();
#endif
  string line = "-----------------------------";
  string scfg;

  char cfill = 'x';
  cout << myname << "Checking fill char." << endl;
  int vint = 123;
  cfill = StringManipulator::getFill<int>(vint);
  cout << myname << "  int " << vint << ": " << cfill << endl;
  assert( cfill == '0' );
  vint = -123;
  cfill = StringManipulator::getFill<int>(vint);
  cout << myname << "  int " << vint << ": " << cfill << endl;
  assert( cfill == '-' );
  float vflo = 1.23;
  cfill = StringManipulator::getFill<float>(vflo);
  cout << myname << "  float " << vflo << ": " << cfill << endl;
  assert( cfill == '_' );

  // Build raw and expected strings.
  vector<string> raws = {
    "abc*SUB*ghi", 
    "abc*POS*ghi", 
    "abc*NEG*ghi",
    "abc*SUB*ghi_*NEG*_*POS*.txt"
  };
  vector<string> exps = {
    "abcdefghi",
    "abc123ghi",
    "abc-123ghi",
    "abcdefghi_-123_123.txt"
  };
  vector<string> expws = {
    "abc__defghi",
    "abc00123ghi",
    "abc--123ghi",
    "abc__defghi_--123_00123.txt"
  };

  cout << myname << "Testing no width with " << raws.size() << " strings." << endl;
  Index w = 12;
  for ( Index istr=0; istr<raws.size(); ++istr ) {
    string raw = raws[istr];
    string exp = exps[istr];
    string val = raw;
    StringManipulator sman(val);
    assert( sman.logLevel() == 0 );
    sman.setLogLevel(logLevel);
    assert( sman.logLevel() == logLevel );
    sman.replace("*SUB*", "def");
    sman.replace("*POS*", 123);
    sman.replace("*NEG*", -123);
    cout << myname << "  "
         << setw(w) << raw << ": "
         << setw(w) << val << " ?= "
         << setw(w) << exp << endl;
    assert( raw != exp );
    assert( val == exp );
  }

  cout << myname << "Testing width 5 with " << raws.size() << " strings." << endl;
  for ( Index istr=0; istr<raws.size(); ++istr ) {
    string raw = raws[istr];
    string exp = expws[istr];
    string val = raw;
    StringManipulator sman(val);
    sman.setLogLevel(logLevel);
    sman.replaceFixedWidth("*SUB*", "def", 5);
    sman.replaceFixedWidth("*POS*", 123, 5);
    sman.replaceFixedWidth("*NEG*", -123, 5);
    cout << myname << "  "
         << setw(w) << raw << ": "
         << setw(w) << val << " ?= "
         << setw(w) << exp << endl;
    assert( raw != exp );
    assert( val == exp );
  }

  cout << myname << "Test split." << endl;
  StringVector strs;
  StringVector sepss(100, ",");
  StringVV splits;
  strs.push_back("a,bb,ccc");
  splits.push_back({"a", "bb", "ccc"});
  strs.push_back(",a,bb,ccc,");
  splits.push_back({"a", "bb", "ccc"});
  strs.push_back("a,,bb,ccc");
  splits.push_back({"a", "bb", "ccc"});
  for ( Index itst=0; itst<strs.size(); ++itst ) {
    string str = strs[itst];
    string seps = sepss[itst];
    StringManipulator sman(str);
    sman.setLogLevel(logLevel);
    cout << myname << "  " << str << endl;
    assert( areEqual(sman.split(seps), splits[itst]) );
  }

  cout << myname << "Test pattern split." << endl;
  strs.clear();
  StringVector spats(100, "{,}");
  splits.clear();
  strs.push_back("where did {bob,sally,kim} go?");
  splits.push_back({"where did bob go?", "where did sally go?", "where did kim go?"});
  strs.push_back("where did {bob,sal,kim} {go,stay}?");
  splits.push_back({"where did bob go?", "where did bob stay?",
                    "where did sal go?", "where did sal stay?",
                    "where did kim go?", "where did kim stay?"});
  for ( Index itst=0; itst<strs.size(); ++itst ) {
    string str = strs[itst];
    string spat = spats[itst];
    StringManipulator sman(str);
    cout << myname << "  " << str << ", " << spat << endl;
    assert( areEqual(sman.patternSplit(spat), splits[itst]) );
  }

  cout << myname << "Done." << endl;
  return 0;
}

//**********************************************************************

int main() {
  return test_StringManipulator(1);
}

//**********************************************************************
