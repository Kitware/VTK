// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkAffineArray.h"
#include "vtkArrayComponents.h"
#include "vtkCharArray.h"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkIntArray.h"
#include "vtkNew.h"
#include "vtkTypeUInt64Array.h"

#include "vtkTestErrorObserver.h"

#include <array>

#include <iostream>

namespace
{

template <vtkArrayComponents NormType, typename ValueType>
bool TestNormValueType(vtkDataArray* baseArray, const std::array<ValueType, 4>& expected)
{
  auto normArray = vtk::ComponentOrNormAsDataArray(baseArray, NormType);

  std::cout << "  Testing " << vtk::to_string(NormType) << "\n";
  bool ok = true;
  for (vtkIdType ii = 0; ii < 4; ++ii)
  {
    std::cout << "    tuple " << ii << " norm " << normArray->GetTuple1(ii) << "\n";
    if (std::fabs(normArray->GetTuple1(ii) - expected[ii]) > 1e-14)
    {
      ok = false;
      std::cerr << "  ERROR: Expected " << expected[ii] << " got " << normArray->GetTuple1(ii)
                << "\n";
    }
  }

  return ok;
}

template <typename ValueType>
bool TestComponentArray(vtkDataArray* baseArray, int component)
{
  bool ok = true;
  auto compArray = vtk::ComponentOrNormAsDataArray(baseArray, component);
  if (!compArray)
  {
    if (component >= 0 && component < baseArray->GetNumberOfComponents())
    {
      ok = false;
      vtkErrorWithObjectMacro(baseArray, "Failed to extract a valid component " << component);
    }
    return ok;
  };

  std::cout << "  Testing component " << component << "\n";
  bool componentIsValid = (component >= 0 && component < baseArray->GetNumberOfComponents());
  for (vtkIdType ii = 0; ii < 4; ++ii)
  {
    std::cout << "    tuple " << ii << " value " << compArray->GetTuple1(ii) << "\n";
    if (componentIsValid && compArray->GetTuple1(ii) != baseArray->GetComponent(ii, component))
    {
      ok = false;
      std::cerr << "  ERROR: Expected " << baseArray->GetComponent(ii, component) << " got "
                << compArray->GetTuple1(ii) << "\n";
    }
    else if (!componentIsValid && compArray->GetTuple1(ii) != baseArray->GetComponent(ii, 0))
    {
      ok = false;
      std::cerr << "  ERROR: Expected " << baseArray->GetComponent(ii, 0) << " got "
                << compArray->GetTuple1(ii) << "\n";
    }
  }

  return ok;
}

template <typename VArrayType, typename ValueType>
bool TestValueType(const std::array<double, 4>& l1Norms, const std::array<double, 4>& l2Norms,
  const std::array<ValueType, 4>& lInfNorms)
{
  constexpr bool Signed = std::is_signed<ValueType>::value;
  // First, create a base array whose norms we'll compute.
  vtkNew<VArrayType> baseArray;
  // clang-format off
  std::array<std::array<ValueType, 2>, 4> data{{
    {                 3 ,                 4 },
    {                 5 , Signed ? -12 : 12 },
    { Signed ? -10 : 10 , Signed ? -10 : 10 },
    {                 0 ,                 1 }
  }};
  // clang-format on
  baseArray->SetNumberOfComponents(2);
  baseArray->SetNumberOfTuples(static_cast<vtkIdType>(data.size()));
  vtkIdType ii = 0;
  for (const auto& tuple : data)
  {
    baseArray->SetTypedTuple(ii++, tuple.data());
  }
  std::cout << "Testing with " << baseArray->GetClassName() << "\n";

  bool ok = true;
  // Test valid component numbers map to the proper entry of each tuple.
  ok &= TestComponentArray<ValueType>(baseArray, 0);
  ok &= TestComponentArray<ValueType>(baseArray, 1);
  // Test that invalid component numbers return a null array.
  // This will normally print an error, so we catch it.
  vtkNew<vtkTest::ErrorObserver> errorObserver;
  baseArray->AddObserver(vtkCommand::ErrorEvent, errorObserver);
  ok &= TestComponentArray<ValueType>(baseArray, 2);
  baseArray->RemoveObserver(errorObserver);
  if (errorObserver->CheckErrorMessage("Invalid component 2 requested."))
  {
    vtkErrorWithObjectMacro(baseArray, "Missing error for invalid component.");
    ok = false;
  }

  // Test each type of norm (L₁, L₂, L∞) on the array:
  ok &= TestNormValueType<vtkArrayComponents::L1Norm>(baseArray, l1Norms);
  ok &= TestNormValueType<vtkArrayComponents::L2Norm>(baseArray, l2Norms);
  ok &= TestNormValueType<vtkArrayComponents::LInfNorm>(baseArray, lInfNorms);

  // Test that vtk::ComponentOrNormAsDataArray() can accept an implicit array and
  // not just "traditional" vtkGenericDataArray<vtkAOSDataArrayTemplate<Type>,Type>
  // arrays.
  vtkNew<vtkImplicitArray<vtkAffineImplicitBackend<ValueType>>> affine;
  affine->SetBackend(std::make_shared<vtkAffineImplicitBackend<ValueType>>(1, 1));
  affine->SetNumberOfComponents(3);
  affine->SetNumberOfTuples(4);
  std::cout << "  Testing vtk::ComponentOrNormAsArray on affine backend\n";
  auto affineNorm = vtk::ComponentOrNormAsDataArray(affine, vtkArrayComponents::L2Norm);
  auto affineZ = vtk::ComponentOrNormAsDataArray(affine, 2);
  if (affineNorm && affineZ)
  {
    std::array<double, 3> tuple;
    double norm;
    double zz;
    for (ii = 0; ii < 4; ++ii)
    {
      affineNorm->GetTuple(ii, &norm);
      affineZ->GetTuple(ii, &zz);
      affine->GetTuple(ii, tuple.data());
      double expectedNorm =
        std::sqrt(tuple[0] * tuple[0] + tuple[1] * tuple[1] + tuple[2] * tuple[2]);
      std::cout << "    tuple " << ii << " (" << tuple[0] << "," << tuple[1] << "," << tuple[2]
                << ")"
                << " norm " << norm << " z " << zz << "\n";
      if (std::abs(norm - expectedNorm) > 1e-14)
      {
        ok = false;
        std::cerr << "      ERROR! Norm differs by " << (expectedNorm - norm)
                  << " from expected.\n";
      }
      if (std::abs(tuple[2] - zz) > 1e-14)
      {
        ok = false;
        std::cerr << "      ERROR! Z differs by " << (tuple[2] - zz) << " from expected.\n";
      }
    }
  }
  else
  {
    std::cerr << "  ERROR! Null norm or component array returned!\n";
    ok = false;
  }
  return ok;
}

} // anonymous namespace

int TestArrayComponents(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  std::array<double, 4> dbl_l2Norms{ 5, 13, 14.142135623730951, 1 };
  std::array<double, 4> dbl_l1Norms{ 7, 17, 20, 1 };
  std::array<double, 4> dbl_lInfNorms{ 4, 12, 10, 1 };
  bool ok = TestValueType<vtkDoubleArray>(dbl_l1Norms, dbl_l2Norms, dbl_lInfNorms);

  std::array<float, 4> flt_lInfNorms{ 4, 12, 10, 1 };
  ok &= TestValueType<vtkFloatArray>(dbl_l1Norms, dbl_l2Norms, flt_lInfNorms);

  std::array<vtkTypeUInt64, 4> ull_lInfNorms{ 4, 12, 10, 1 };
  ok &= TestValueType<vtkTypeUInt64Array>(dbl_l1Norms, dbl_l2Norms, ull_lInfNorms);

  std::array<char, 4> sc_lInfNorms{ 4, 12, 10, 1 };
  ok &= TestValueType<vtkCharArray>(dbl_l1Norms, dbl_l2Norms, sc_lInfNorms);

  if (!ok)
  {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
