/*
Copyright (c) 2016 Walter William Karas

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#if defined(TESTLOOP_H_20160430)

#error only include once

#else

#define TESTLOOP_H_20160430

#endif

#include <cstdlib>
#include <iostream>
#include <vector>

using namespace std;

struct Test_base;

vector<Test_base *> test_list;

// Function to set breakpoints on for debugging
void pre_break() { }

class Test_base
  {
  public:

    Test_base() { test_list.push_back(this); }

    bool operator () ()
      {
        pre_break();
        return(test());
      }

  private:

    virtual bool test() { return(false); }
  };

void one_test(unsigned tno)
  {
    if (!((*(test_list[tno]))()))
      cout << "Test " << tno << " failed\n";
  }

int main(int n_arg, const char * const * arg)
  {
    if (n_arg < 3)
      {
        if (n_arg == 2)
          {
            unsigned tno = static_cast<unsigned>(atoi(arg[1]));

            if (tno < test_list.size())
              one_test(static_cast<unsigned>(tno));
            else
              cout << "test number must be less than " << test_list.size()
                   << '\n';
          }
        else
          for (unsigned tno = 0; tno < test_list.size(); ++tno)
            one_test(tno);
      }

    return(0);
  }
