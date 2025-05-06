//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <viskores/cont/ErrorFilterExecution.h>
#include <viskores/cont/UnknownCellSet.h>
#include <viskores/filter/vector_analysis/Gradient.h>
#include <viskores/filter/vector_analysis/worklet/Gradient.h>

namespace
{
//-----------------------------------------------------------------------------
template <typename T, typename S>
inline void transpose_3x3(
  viskores::cont::ArrayHandle<viskores::Vec<viskores::Vec<T, 3>, 3>, S>& field)
{
  viskores::worklet::gradient::Transpose3x3<T> transpose;
  transpose.Run(field);
}

//-----------------------------------------------------------------------------
template <typename T, typename S>
inline void transpose_3x3(viskores::cont::ArrayHandle<viskores::Vec<T, 3>, S>&)
{ //This is not a 3x3 matrix so no transpose needed
}

} //namespace

namespace viskores
{
namespace filter
{
namespace vector_analysis
{
//-----------------------------------------------------------------------------
viskores::cont::DataSet Gradient::DoExecute(const viskores::cont::DataSet& inputDataSet)
{
  const auto& field = this->GetFieldFromDataSet(inputDataSet);
  if (!field.IsPointField())
  {
    throw viskores::cont::ErrorFilterExecution("Point field expected.");
  }

  const bool isVector = field.GetData().GetNumberOfComponents() == 3;
  if (GetComputeQCriterion() && !isVector)
  {
    throw viskores::cont::ErrorFilterExecution("scalar gradients can't generate qcriterion");
  }
  if (GetComputeVorticity() && !isVector)
  {
    throw viskores::cont::ErrorFilterExecution("scalar gradients can't generate vorticity");
  }

  const viskores::cont::UnknownCellSet& inputCellSet = inputDataSet.GetCellSet();
  const viskores::cont::CoordinateSystem& coords =
    inputDataSet.GetCoordinateSystem(this->GetActiveCoordinateSystemIndex());

  viskores::cont::UnknownArrayHandle gradientArray;
  viskores::cont::UnknownArrayHandle divergenceArray;
  viskores::cont::UnknownArrayHandle vorticityArray;
  viskores::cont::UnknownArrayHandle qcriterionArray;

  // TODO: there are a humungous number of (weak) symbols in the .o file. Investigate if
  //  they are all legit.

  auto resolveType = [&](const auto& concrete)
  {
    // use std::decay to remove const ref from the decltype of concrete.
    using T = typename std::decay_t<decltype(concrete)>::ValueType;
    viskores::worklet::GradientOutputFields<T> gradientfields(this->GetComputeGradient(),
                                                              this->GetComputeDivergence(),
                                                              this->GetComputeVorticity(),
                                                              this->GetComputeQCriterion());

    viskores::cont::ArrayHandle<viskores::Vec<T, 3>> result;
    if (this->ComputePointGradient)
    {
      viskores::worklet::PointGradient gradient;
      result = gradient.Run(inputCellSet, coords, concrete, gradientfields);
    }
    else
    {
      viskores::worklet::CellGradient gradient;
      result = gradient.Run(inputCellSet, coords, concrete, gradientfields);
    }
    if (!this->RowOrdering)
    {
      transpose_3x3(result);
    }

    gradientArray = result;
    divergenceArray = gradientfields.Divergence;
    vorticityArray = gradientfields.Vorticity;
    qcriterionArray = gradientfields.QCriterion;
  };

  using SupportedTypes =
    viskores::List<viskores::Float32, viskores::Float64, viskores::Vec3f_32, viskores::Vec3f_64>;
  field.GetData()
    .CastAndCallForTypesWithFloatFallback<SupportedTypes, VISKORES_DEFAULT_STORAGE_LIST>(
      resolveType);

  // This copies the CellSet and Fields to be passed from inputDataSet to outputDataSet
  viskores::cont::DataSet outputDataSet = this->CreateResult(inputDataSet);

  std::string outputName = this->GetOutputFieldName();
  if (outputName.empty())
  {
    outputName = this->GradientsName;
  }

  viskores::cont::Field::Association fieldAssociation(
    this->ComputePointGradient ? viskores::cont::Field::Association::Points
                               : viskores::cont::Field::Association::Cells);

  outputDataSet.AddField(viskores::cont::Field{ outputName, fieldAssociation, gradientArray });

  if (this->GetComputeDivergence() && isVector)
  {
    outputDataSet.AddField(
      viskores::cont::Field{ this->GetDivergenceName(), fieldAssociation, divergenceArray });
  }
  if (this->GetComputeVorticity() && isVector)
  {
    outputDataSet.AddField(
      viskores::cont::Field{ this->GetVorticityName(), fieldAssociation, vorticityArray });
  }
  if (this->GetComputeQCriterion() && isVector)
  {
    outputDataSet.AddField(
      viskores::cont::Field{ this->GetQCriterionName(), fieldAssociation, qcriterionArray });
  }
  return outputDataSet;
}
} // namespace vector_analysis
} // namespace filter
} // namespace viskores
