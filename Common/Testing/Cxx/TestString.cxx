// This test should only really bump up the coverage.
// If it fails, something is really wrong.

#include "vtkString.h"

int main(int, char** argv)
{
  int res = 0;
  int len = vtkString::Length(argv[0]);
  if ( len <= 3 )
    {
    cout << "Lenght of the program name should be longer than that" << endl;
    res = 1;
    }
  char *dup = vtkString::Duplicate(argv[0]);
  if ( vtkString::Compare(dup, argv[0]) != 0 )
    {
    cout << "Compare does not work" << endl;
    res = 1;
    }
  if ( !vtkString::Equals(dup, argv[0] ) )
    {
    cout << "Equals does not work" << endl;
    res = 1;
    }
  delete [] dup;
  const char* str = "TestString";
  if ( !vtkString::StartsWith(str, "Test") )
    {
    cout << "StartsWith does not work" << endl;
    res =1;
    }
  if ( !vtkString::EndsWith(str, "String") )
    {
    cout << "EndsWith does not work" << endl;
    res =1;
    }
  dup = new char[ vtkString::Length(argv[0]) + 1 ];
  vtkString::Copy(dup, argv[0]);
   if ( vtkString::Compare(dup, argv[0]) != 0 )
    {
    cout << "Compare does not work" << endl;
    res = 1;
    }
  if ( !vtkString::Equals(dup, argv[0] ) )
    {
    cout << "Equals does not work" << endl;
    res = 1;
    }
  delete [] dup;
  if ( vtkString::Compare("a", "b") != -1 )
    {
    cout << "Compare does not work" << endl;
    res = 1;
    }
  if ( vtkString::Compare("b", "a") != 1 )
    {
    cout << "Compare does not work" << endl;
    res = 1;
    }
  char *hello_world = vtkString::Append("Hello", "World");
  if ( !vtkString::Equals(hello_world, "HelloWorld") )
    {
    cout << "Append does not work" << endl;
    res = 1;
    }
  return res;
}
