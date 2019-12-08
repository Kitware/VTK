/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestSelectionExpression.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkNew.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkSignedCharArray.h"

#include <algorithm>
#include <map>
#include <random>
#include <string>

vtkSmartPointer<vtkSignedCharArray> NewArray(vtkIdType numVals)
{
  std::random_device rd;  // Will be used to obtain a seed for the random number engine
  std::mt19937 gen(rd()); // Standard mersenne_twister_engine seeded with rd()
  std::uniform_int_distribution<> dis(0, 1);

  auto array = vtkSmartPointer<vtkSignedCharArray>::New();
  array->SetNumberOfComponents(1);
  array->SetNumberOfTuples(numVals);
  std::generate_n(array->GetPointer(0), numVals, [&]() { return dis(gen); });
  return array;
}

static inline bool get(vtkSignedCharArray* array, vtkIdType cc)
{
  return array->GetTypedComponent(cc, 0) != 0;
}

template <typename Functor>
void ValidateIternal(vtkSignedCharArray* result, const Functor& f)
{
  for (vtkIdType cc = 0; cc < 1024; ++cc)
  {
    bool val = f(cc);
    bool rval = get(result, cc);
    if (val != rval)
    {
      cerr << "ERROR: failed at index '" << cc << "'." << endl;
      throw std::runtime_error("value mismatch");
    }
  }
}

template <typename Functor>
void Validate(vtkSelection* expr, const char* exprstr,
  const std::vector<vtkSignedCharArray*>& arrays, const Functor& f)
{
  expr->SetExpression(exprstr);
  auto result = expr->Evaluate(&arrays[0], static_cast<unsigned int>(arrays.size()));
  if (!result)
  {
    throw std::runtime_error("null result");
  }
  ValidateIternal(result, f);
}

template <typename Functor, typename MapType>
void Validate(vtkSelection* expr, const char* exprstr, const MapType& arrays, const Functor& f)
{
  expr->SetExpression(exprstr);
  auto result = expr->Evaluate(arrays);
  if (!result)
  {
    throw std::runtime_error("null result");
  }
  ValidateIternal(result, f);
}

int TestSelectionExpression(int, char*[])
{
  vtkNew<vtkSelection> expr;
  vtkNew<vtkSelectionNode> aItem;
  vtkNew<vtkSelectionNode> bItem;
  vtkNew<vtkSelectionNode> cItem;
  vtkNew<vtkSelectionNode> dItem;
  vtkNew<vtkSelectionNode> eItem;
  vtkNew<vtkSelectionNode> fItem;
  vtkNew<vtkSelectionNode> gItem;
  expr->SetNode("A", aItem);
  expr->SetNode("B", bItem);
  expr->SetNode("C", cItem);
  expr->SetNode("D", dItem);
  expr->SetNode("D", dItem);
  expr->SetNode("E", eItem);
  expr->SetNode("F", fItem);
  expr->SetNode("G", gItem);

  std::map<std::string, vtkSmartPointer<vtkSignedCharArray> > arrays;
  std::vector<vtkSignedCharArray*> arrays_ptrs(expr->GetNumberOfNodes(), nullptr);
  for (int cc = 0, max = expr->GetNumberOfNodes(); cc < max; ++cc)
  {
    const std::string varname = expr->GetNodeNameAtIndex(cc);
    auto newarray = NewArray(1024);
    arrays[varname] = newarray;
    arrays_ptrs[cc] = newarray;
  }

  Validate(expr, "A & (B | (C & D))", arrays_ptrs, [&](vtkIdType cc) {
    auto a = get(arrays["A"], cc);
    auto b = get(arrays["B"], cc);
    auto c = get(arrays["C"], cc);
    auto d = get(arrays["D"], cc);
    return a && (b || (c && d));
  });

  Validate(expr, "A & B | (C & D)", arrays_ptrs, [&](vtkIdType cc) {
    auto a = get(arrays["A"], cc);
    auto b = get(arrays["B"], cc);
    auto c = get(arrays["C"], cc);
    auto d = get(arrays["D"], cc);
    return (a && b) || (c && d);
  });

  Validate(expr, "(A & B) | (C | (D & E) ) | (C & D)", arrays_ptrs, [&](vtkIdType cc) {
    auto a = get(arrays["A"], cc);
    auto b = get(arrays["B"], cc);
    auto c = get(arrays["C"], cc);
    auto d = get(arrays["D"], cc);
    auto e = get(arrays["E"], cc);
    return (a && b) || (c || (d && e)) || (c && d);
  });

  // empty expression is treated as "|"
  Validate(expr, "", arrays, [&](vtkIdType cc) {
    auto a = get(arrays["A"], cc);
    auto b = get(arrays["B"], cc);
    auto c = get(arrays["C"], cc);
    auto d = get(arrays["D"], cc);
    auto e = get(arrays["E"], cc);
    auto f = get(arrays["F"], cc);
    auto g = get(arrays["G"], cc);
    return (a || b || c || d || e || f || g);
  });

  return EXIT_SUCCESS;
}
