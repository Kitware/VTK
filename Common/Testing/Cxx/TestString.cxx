// This test should only really bump up the coverage.
// If it fails, something is really wrong.

#include "vtkString.h"

#define TEST(test, name) \
  if ( test ) \
    {\
    cout << __LINE__ << ": " << #name << " Does not work" << endl;\
    res = 1;\
    }

int TestString(int, char* argv[])
{
  int res = 0;
  int len = vtkString::Length(argv[0]);
  if ( len <= 3 )
    {
    cout << "Lenght of the program name should be longer than that" << endl;
    res = 1;
    }
  char *dup = vtkString::Duplicate(argv[0]);
  TEST( ( vtkString::Compare(dup, argv[0]) != 0 ), Compare);
  TEST( (!vtkString::Equals(dup, argv[0] ) ), Equals);
  delete [] dup;

  const char* str = "TestString";
  TEST( (!vtkString::StartsWith(str, "Test") ), StartsWith );
  TEST( ( vtkString::StartsWith(str, 0) ), StartsWith );
  TEST( (!vtkString::EndsWith(str, "String") ), EndsWith );
  TEST( ( vtkString::EndsWith(str, 0) ), EndsWith );

  dup = new char[ vtkString::Length(argv[0]) + 1 ];
  vtkString::Copy(dup, argv[0]);
  TEST( ( vtkString::Compare(dup, argv[0])), Compare );
  TEST( (!vtkString::Equals(dup, argv[0])), Equals );
  delete [] dup;

  dup = vtkString::Duplicate(0);
  TEST( (dup), Duplicate(0) );
  delete [] dup;

  TEST( ( vtkString::Equals("FoObAr", "fOoBaR" ) ), Equals );
  TEST( (!vtkString::EqualsCase("FoObAr", "fOoBaR" ) ), EqualsCase );

  TEST( (vtkString::Compare("a", "b")!=-1 ), Compare );
  TEST( (vtkString::Compare("b", "a")!= 1 ), Compare );

  TEST( (vtkString::CompareCase("A", "b")!=-1 ), CompareCase );
  TEST( (vtkString::CompareCase("b", "A")!= 1 ), CompareCase );

  TEST( (vtkString::Compare("A", 0)!= 1 ), Compare );
  TEST( (vtkString::Compare(0, "A")!=-1 ), Compare );

  TEST( (vtkString::CompareCase("A", 0)!= 1 ), CompareCase );
  TEST( (vtkString::CompareCase(0, "A")!=-1 ), CompareCase );

  char *hello_world = vtkString::Append("Hello", "World");
  TEST( (!vtkString::Equals(hello_world, "HelloWorld") ), Append );
  delete [] hello_world;

  char *hello_ = vtkString::Append("Hello", 0);
  TEST( (!vtkString::Equals(hello_, "Hello") ), Append );
  delete [] hello_;

  char *_world = vtkString::Append(0, "World");
  TEST( (!vtkString::Equals(_world, "World") ), Append );
  delete [] _world;

  char *empty = vtkString::Append(0,0);
  TEST( ( empty ), Append );
  
  return res;
}
