/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestCxxFeatures.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
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

#if defined(_MSC_VER)
# define VTK_CXX_MSVC
#endif

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

#if defined(__GNUC__) && (__GNUC__ < 3)
# if (__GNUC__ < 3)
#  define VTK_CXX_GCC_2
# elif (__GNUC__ == 3)
#  define VTK_CXX_GCC_3
# endif
#endif

//----------------------------------------------------------------------------

/* Check for known compiler limitations.  */

// Check for IRIX64-6.5-CC-o32 (old SGI compiler).
#if defined(VTK_CXX_SGI_6)
# define VTK_TYPENAME /* empty */
# define VTK_CLASS_TEMPLATE_SPECIALIZATION /* empty */
#endif

// Check for MSVC.
#if defined(VTK_CXX_MSVC)
# define VTK_TYPENAME /* empty */
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

/* Test inclusion of sstream header.  */
//#if !(defined(VTK_CXX_GCC_2) || defined(VTK_CXX_ACC) || defined(VTK_CXX_SGI_6))
#if defined(VTK_CXX_GCC_3)
# include <sstream>
#endif

//----------------------------------------------------------------------------

/* Test inclusion of some stl headers.  */
#ifdef _MSC_VER
#pragma warning (push, 2)
#endif

#include <vector>

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#ifndef VTK_NO_STD_NAMESPACE
# define vtkstd std
#else
# define vtkstd
#endif

#if !defined(VTK_CXX_SGI_6)
// Fails on kulu.crd IRIX64-6.5-CC-o32 (old SGI compiler).
void UsingStdVector()
{
  using vtkstd::vector;
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
  ofstream fout("TestCxxFeatures_TestBinaryWriting", ios::out );
#else  
  ofstream fout("TestCxxFeatures_TestBinaryWriting", ios::out | ios::binary);
#endif
  if(!fout)
    {
    cerr << "Error opening TestCxxFeatures_TestBinaryWriting for binary writing.\n";
    result = 0;
    }
  return result;
}

//----------------------------------------------------------------------------

#define DO_TEST(x) \
  if(x()) { cout << "Passed: " #x "\n"; } \
  else { cout << "Failed: " #x "\n"; result = 1; }

int main()
{
  int result = 0;
  DO_TEST(TestFullySpecializedFunction);
#if !defined(VTK_CXX_SGI_6)
  DO_TEST(TestBool);
#endif
  DO_TEST(TestFullySpecializedClass);
  DO_TEST(TestIfScope);
  DO_TEST(TestNonTypeTemplate);
  DO_TEST(TestBinaryWriting);
  return result;
}
