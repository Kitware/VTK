/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestCxxFeatures.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME TestCxxFeatures
// .SECTION Description
// Provides a reference for the set of C++ features that can be used
// by VTK.

#include "vtkConfigure.h"

//----------------------------------------------------------------------------

/* Check for known compilers.  */

#if defined(__sgi) && !defined(__GNUC__)
# define VTK_CXX_SGI
# if !defined(_COMPILER_VERSION)
#  define VTK_CXX_SGI_6
# endif
#endif

#if defined(__HP_aCC)
# define VTK_CXX_ACC
#endif

#if defined(__SUNPRO_CC)
# define VTK_CXX_SUNPRO
#endif

//----------------------------------------------------------------------------

/* Check for known compiler limitations.  */

// Check for IRIX64-6.5-CC-o32 (old SGI compiler).
#if defined(VTK_CXX_SGI_6)
# define VTK_TYPENAME /* empty */
# define VTK_CLASS_TEMPLATE_SPECIALIZATION /* empty */
#endif

// Assume standard behavior if symbol is not already defined.
#if !defined(VTK_TYPENAME)
# define VTK_TYPENAME typename
#endif

// Assume standard behavior if symbol is not already defined.
#if !defined(VTK_CLASS_TEMPLATE_SPECIALIZATION)
# define VTK_CLASS_TEMPLATE_SPECIALIZATION template <>
#endif

//----------------------------------------------------------------------------

#include "vtkSystemIncludes.h"

//----------------------------------------------------------------------------

/* Test inclusion of typeinfo header.  */

#include <typeinfo>

//----------------------------------------------------------------------------

/* Test use of namespaces.  */

#if !defined(VTK_CXX_SGI_6)
// Fails on kulu.crd IRIX64-6.5-CC-o32 (old SGI compiler).
namespace NamespaceTest {}
namespace {}
void NamespaceTestFunc() {}
namespace NamespaceTest
{
  using ::NamespaceTestFunc;
}
using namespace NamespaceTest;
#endif

//----------------------------------------------------------------------------

/* Test nested classes defined outside.  */

class NestedTestOuter
{
public:
  NestedTestOuter();
  ~NestedTestOuter();
private:
  class NestedTestInner;
  NestedTestInner* Inner;
};

class NestedTestOuter::NestedTestInner
{
public:
  NestedTestInner() {}
  ~NestedTestInner() {}
};

NestedTestOuter::NestedTestOuter()
{
  this->Inner = new NestedTestInner;
}

NestedTestOuter::~NestedTestOuter()
{
  delete this->Inner;
}

//----------------------------------------------------------------------------

/* Test inclusion of some stl headers.  */
#ifdef _MSC_VER
#pragma warning (push, 2)
#endif

#include <vector>

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#if !defined(VTK_CXX_SGI_6)
// Fails on kulu.crd IRIX64-6.5-CC-o32 (old SGI compiler).
void UsingStdVector()
{
  using std::vector;
  vector<int>();
}
#endif

//----------------------------------------------------------------------------

/* Test full template specialization of functions.  */
template <class T>
int FullySpecializedFunction(T*)
{
  return 0;
}

#if !defined(VTK_CXX_SGI)
// Fails on kulu.crd IRIX64-6.5-CC-o32 (old SGI compiler).
// Fails on manifold.crd IRIX64-6.5-CC-n32 (new SGI compiler).
template <>
int FullySpecializedFunction<int>(int*)
{
  return 1;
}
#else
// Let overload resolution pick this one instead.
int FullySpecializedFunction(int*)
{
  return 1;
}
#endif

int TestFullySpecializedFunction()
{
  int result = 1;
  int should_be_0 = FullySpecializedFunction(static_cast<float*>(0));
  if(should_be_0 != 0)
    {
    cerr << "FullySpecializedFunction<float*>() returned "
         << should_be_0 << ", not 0.\n";
    result = 0;
    }
  int should_be_1 = FullySpecializedFunction(static_cast<int*>(0));
  if(should_be_1 != 1)
    {
    cerr << "FullySpecializedFunction(int*) returned "
         << should_be_1 << ", not 1.\n";
    result = 0;
    }
  return result;
}

//----------------------------------------------------------------------------

/* Test member template of non-template.  */

class NonTemplate
{
  void* Pointer;
public:
  template <class T> void Set(T* t) { this->Pointer = t; }
  template <class T> void Get(T*& t) { t = static_cast<T*>(this->Pointer); }
};

int TestNonTemplateMemberTemplate()
{
  int x = 123;
  int* px = 0;
  NonTemplate nt;
  nt.Set(&x);
  nt.Get(px);
  return (*px == 123);
}

//----------------------------------------------------------------------------

/* Test member template of template.  */

template <class T>
class OuterTemplate
{
  T* Pointer;
public:
  template <class U> void Set(U* u) { this->Pointer = u; }
  template <class U> void Get(U*& u) { u = static_cast<U*>(this->Pointer); }
};

int TestTemplateMemberTemplate()
{
  int x = 123;
  int* px = 0;
  OuterTemplate<void> nt;
  nt.Set(&x);
  nt.Get(px);
  return (*px == 123);
}

//----------------------------------------------------------------------------

/* Test use of standard "bool" type and values.  */

#if !defined(VTK_CXX_SGI_6)
bool GetFalse()
{
  return false;
}

bool GetTrue()
{
  return true;
}

int TestBool()
{
  int result = 1;
  bool should_be_false = GetFalse();
  bool should_be_true = GetTrue();
  if(should_be_false)
    {
    cerr << "GetFalse() returned " << should_be_false << ", not false.\n";
    result = 0;
    }
  if(!should_be_true)
    {
    cerr << "GetTrue() returned " << should_be_true << ", not true.\n";
    result = 0;
    }
  return result;
}
#endif
//----------------------------------------------------------------------------

/* Test full template specialization of classes.  */

template <class T>
struct FullySpecializedClass
{
  static int Method() { return 0; }
  typedef T Type;
};

VTK_CLASS_TEMPLATE_SPECIALIZATION
struct FullySpecializedClass<float>
{
  static int Method() { return 1; }
  typedef int Type;
};

template <class T>
int TestFullySpecializedClassTrait(T*)
{
  typedef VTK_TYPENAME FullySpecializedClass<T>::Type Type;
  if(static_cast<Type>(3.1) == 3.1)
    {
    return 0;
    }
  return 1;
}

int TestFullySpecializedClass()
{
  int result = 1;
  int should_be_0 = FullySpecializedClass<int>::Method();
  if(should_be_0 != 0)
    {
    cerr << "FullySpecializedClass<int>::Method() returned "
         << should_be_0 << ", not 0.\n";
    result = 0;
    }
  int should_be_1 = FullySpecializedClass<float>::Method();
  if(should_be_1 != 1)
    {
    cerr << "FullySpecializedClass<float>::Method() returned "
         << should_be_1 << ", not 1.\n";
    result = 0;
    }
  if(!TestFullySpecializedClassTrait(static_cast<float*>(0)))
    {
    cerr << "Trait lookup of float didn't produce int.";
    result = 0;
    }
  return result;
}

//----------------------------------------------------------------------------

/* Test if(int x = f()) style scoping.  */

int TestIfScopeHelper(int i)
{
  int result = 1;
  if(int x = i)
    {
    if(x != i)
      {
      cerr << "TestIfScope: x != " << i << "\n";
      result = 0;
      }
    }
  else
    {
    if(x != i)
      {
      cerr << "TestIfScope: x != " << i << "\n";
      result = 0;
      }
    }
  int x = result;
  return x;
}

int TestIfScope()
{
  int result = 1;
  if(!TestIfScopeHelper(1))
    {
    result = 0;
    }
  if(!TestIfScopeHelper(0))
    {
    result = 0;
    }
  return result;
}

//----------------------------------------------------------------------------

/* Test non-type template parameter.  */

template <int I>
struct NonTypeTemplate
{
  static int GetValue() { return I; }
};

int TestNonTypeTemplate()
{
  int result = 1;
  if(NonTypeTemplate<0>::GetValue() != 0)
    {
    cerr << "NonTypeTemplate<0>::GetValue() != 0\n";
    result = 0;
    }
  if(NonTypeTemplate<1>::GetValue() != 1)
    {
    cerr << "NonTypeTemplate<1>::GetValue() != 1\n";
    result = 0;
    }
  if(NonTypeTemplate<2>::GetValue() != 2)
    {
    cerr << "NonTypeTemplate<2>::GetValue() != 2\n";
    result = 0;
    }
  return result;
}

//----------------------------------------------------------------------------

/* Test mixed type and non-type template arguments in a non-trival way.  */

#if !defined(__BORLANDC__)
// Borland does not support this fancy array template.
template <class T, int N>
int TestMixedTypeTemplateFunction(T (*)[N])
{
  return N;
}
int TestMixedTypeTemplate()
{
  int x2[2];
  float x3[3];
  int result = 1;
  if(TestMixedTypeTemplateFunction(&x2) != 2)
    {
    cerr << "TestMixedTypeTemplateFunction(&x2) != 2\n";
    result = 0;
    }
  if(TestMixedTypeTemplateFunction(&x3) != 3)
    {
    cerr << "TestMixedTypeTemplateFunction(&x3) != 3\n";
    result = 0;
    }
  return result;
}
#endif

//----------------------------------------------------------------------------

int TestBinaryWriting()
{
  int result = 1;
  // ios::binary does not exist on SGI and OSF cxx (DEC)
  // it failed to compile on these machines:
  // ct02_oc.crd IRIX64-6.5-CC-64
  // manifold IRIX64-6.5-CC-n32
  // kulu.crd IRIX64-6.5-CC-o32
  // a62.iue.tuwien.ac.at OSF1-V5.1-cxx
#if defined(VTK_CXX_SGI) || defined( __DECCXX_VER)
  ofstream fout_with_warning_C4701("TestCxxFeatures_TestBinaryWriting", ios::out );
#else
  ofstream fout_with_warning_C4701("TestCxxFeatures_TestBinaryWriting", ios::out | ios::binary);
#endif
  if(!fout_with_warning_C4701)
    {
    cerr << "Error opening TestCxxFeatures_TestBinaryWriting for binary writing.\n";
    result = 0;
    }
  return result;
}

//----------------------------------------------------------------------------

class SafeBoolIdiomClass
{
private:
  struct SafeBoolDummy { void Dummy() {} };
  typedef void (SafeBoolDummy::* SafeBool)();
public:
  SafeBoolIdiomClass(int x): Value(x) {}
  operator SafeBool()
    {
    return this->Value? &SafeBoolDummy::Dummy : 0;
    }
  SafeBool operator !()
    {
    return this->Value? 0 : &SafeBoolDummy::Dummy;
    }
protected:
  int Value;
};

int TestSafeBoolIdiom()
{
  int result = 1;
  SafeBoolIdiomClass cTrue(1);
  SafeBoolIdiomClass cFalse(0);
  if(cTrue) {}
  else
    {
    cerr << "if(cTrue) evaluates to false.\n";
    result = 0;
    }
  if(!cTrue)
    {
    cerr << "if(!cTrue) evaluates to true.\n";
    result = 0;
    }
  if(cFalse)
    {
    cerr << "if(cFalse) evaluates to true.\n";
    result = 0;
    }
  if(!cFalse) {}
  else
    {
    cerr << "if(!cFalse) evaluates to false.\n";
    result = 0;
    }
  return result;
}

//----------------------------------------------------------------------------

/* Test use of exceptions.  */

#if defined(_MSC_VER)
# pragma warning (push)
# pragma warning (disable: 4702) /* Unreachable code. */
#endif

class TestExceptionUnwind
{
  int* pvalue;
public:
  TestExceptionUnwind(int* p): pvalue(p) {}
  ~TestExceptionUnwind() { *pvalue = 1; }
  void Use() {}
};

class ExceptionClass {};

void TestThrowException(int* p)
{
  TestExceptionUnwind unwind(p);
  unwind.Use();
  throw ExceptionClass();
}

int TestException()
{
  int value = 0;
  try
    {
    TestThrowException(&value);
    }
  catch(ExceptionClass&)
    {
    if(value)
      {
      return 1;
      }
    else
      {
      cerr << "TestExceptionUnwind object not destroyed!" << endl;
      return 0;
      }
    }
  catch(...)
    {
    cerr << "ExceptionClass not caught!" << endl;
    return 0;
    }
  cerr << "No exception caught!" << endl;
  return 0;
}

#if defined(_MSC_VER)
# pragma warning (pop)
#endif

//----------------------------------------------------------------------------

/* Test void return type syntax.  */

void TestVoidReturnInner() {}
void TestVoidReturnOuter()
{
  // MIPSpro 7.3 does not support void returns.
#if !(defined(_COMPILER_VERSION) && (_COMPILER_VERSION < 740))
  return TestVoidReturnInner();
#endif
}

// MIPSpro warns about type qualifiers on return types.
#if defined(_COMPILER_VERSION)
# pragma set woff 3303 // type qualifier on return is meaningless
#endif
// Intel C++ warns about type qualifiers on return types.
#if defined(__INTEL_COMPILER)
# pragma warning (push)
# pragma warning (disable:858) // type qualifier on return is meaningless
#endif

// clang warns about type qualifiers on return types.
#if defined(__clang__)
# pragma clang diagnostic push
# pragma clang diagnostic ignored "-Wignored-qualifiers"
#endif

// gcc>=4.3 says type qualifiers ignored on function return type
#if defined(__GNUC__) && ((__GNUC__ > 4) || ((__GNUC__ == 4) && (__GNUC_MINOR__ >= 6)))
# pragma GCC diagnostic push
#endif
#if defined(__GNUC__) && ((__GNUC__ > 4) || ((__GNUC__ == 4) && (__GNUC_MINOR__ >= 3)))
# pragma GCC diagnostic ignored "-Wignored-qualifiers"
#endif

// aCC warns "type qualifier on return type is meaningless" - just omit the
// function on aCC builds since there is no way to suppress the warning via
// pragmas...
#if !defined(__HP_aCC)
void const TestVoidConstReturn() {}
#endif

#if defined(__GNUC__) && ((__GNUC__ > 4) || ((__GNUC__ == 4) && (__GNUC_MINOR__ >= 6)))
# pragma GCC diagnostic pop
#endif

#if defined(__clang__)
# pragma clang diagnostic pop
#endif

#if defined(__INTEL_COMPILER)
# pragma warning (pop)
#endif

#if defined(_COMPILER_VERSION)
# pragma reset woff 3303 // type qualifier on return is meaningless
#endif


//-------------------------------------------------------------------
// See if the following code works on all platforms
#if defined(_MSC_VER) && defined(_DEBUG)
/* MSVC debug hook to prevent dialogs when running from DART.  */
# include <crtdbg.h>
static int TestDriverDebugReport(int type, char* message, int* retVal)
{
  (void)type; (void)retVal;
  fprintf(stderr, message);
  exit(1);
}
#endif


//----------------------------------------------------------------------------

/* Test setlocale  */
#include <locale.h>
int TestSetLocale()
{
  char *oldLocale = strdup(setlocale(LC_NUMERIC,NULL));
  setlocale(LC_NUMERIC,"English");

  // restore the local
  if (oldLocale)
    {
    setlocale(LC_NUMERIC,oldLocale);
    free(oldLocale);
    return 1;
    }
  return 0;
}

//----------------------------------------------------------------------------

#define DO_TEST(x) \
  if(x()) { cout << "Passed: " #x "\n"; } \
  else { cout << "Failed: " #x "\n"; result = 1; }

int main()
{
  int result = 0;
  DO_TEST(TestFullySpecializedFunction);
  DO_TEST(TestNonTemplateMemberTemplate);
  DO_TEST(TestTemplateMemberTemplate);
#if !defined(VTK_CXX_SGI_6)
  DO_TEST(TestBool);
#endif
  DO_TEST(TestFullySpecializedClass);
  DO_TEST(TestIfScope);
  DO_TEST(TestNonTypeTemplate);
#if !defined(__BORLANDC__)
  DO_TEST(TestMixedTypeTemplate);
#endif
  DO_TEST(TestBinaryWriting);
  DO_TEST(TestSafeBoolIdiom);
  DO_TEST(TestException);
  DO_TEST(TestSetLocale);

#if defined(_MSC_VER) && defined(_DEBUG)
  // just call the code to shut up a linker warning
  int retVal = 0;
  if (result)
    {
    // really shouldn't be called unless something else failed
    // just want to make the compiler think it might get called
    // all this will be yanked once I see the results of this test
    TestDriverDebugReport(0, "a temp test", &retVal);
    }
#endif
  return result;
}
