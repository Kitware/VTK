#include "vtkSystemIncludes.h"

const char* f()
{
  return 0;
}

const char* g()
{
  return "Hello, world!";
}

int main()
{
  if(const char* buf = f())
    {
    cout << "f(): " << buf << "\n";
    }
  else if(const char* buf = g())
    {
    cout << "g(): " << buf << "\n";
    }
  
  return 0;
}
