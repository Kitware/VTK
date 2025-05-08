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
#include <viskores/cont/CellSetSingleType.h>
#include <viskores/cont/CellSetStructured.h>
#include <viskores/cont/UnknownCellSet.h>

#include <viskores/filter/contour/Contour.h>
#include <viskores/filter/contour/ContourFlyingEdges.h>
#include <viskores/filter/contour/ContourMarchingCells.h>

namespace viskores
{
namespace filter
{

using SupportedTypes =
  viskores::List<viskores::UInt8, viskores::Int8, viskores::Float32, viskores::Float64>;

namespace contour
{
//-----------------------------------------------------------------------------
viskores::cont::DataSet Contour::DoExecute(const viskores::cont::DataSet& inDataSet)
{
  // Switch between Marching Cubes and Flying Edges implementation of contour,
  // depending on the type of CellSet we are processing

  viskores::cont::UnknownCellSet inCellSet = inDataSet.GetCellSet();
  auto inCoords = inDataSet.GetCoordinateSystem(this->GetActiveCoordinateSystemIndex()).GetData();
  std::unique_ptr<viskores::filter::contour::AbstractContour> implementation;

  // Flying Edges is only used for 3D Structured CellSets
  if (inCellSet.template IsType<viskores::cont::CellSetStructured<3>>())
  {
    VISKORES_LOG_S(viskores::cont::LogLevel::Info, "Using flying edges");
    implementation.reset(new viskores::filter::contour::ContourFlyingEdges);
    implementation->SetComputeFastNormals(this->GetComputeFastNormals());
  }
  else
  {
    VISKORES_LOG_S(viskores::cont::LogLevel::Info, "Using marching cells");
    implementation.reset(new viskores::filter::contour::ContourMarchingCells);
    implementation->SetComputeFastNormals(this->GetComputeFastNormals());
  }

  implementation->SetMergeDuplicatePoints(this->GetMergeDuplicatePoints());
  implementation->SetGenerateNormals(this->GetGenerateNormals());
  implementation->SetAddInterpolationEdgeIds(this->GetAddInterpolationEdgeIds());
  implementation->SetNormalArrayName(this->GetNormalArrayName());
  implementation->SetInputCellDimension(this->GetInputCellDimension());
  implementation->SetActiveField(this->GetActiveFieldName());
  implementation->SetFieldsToPass(this->GetFieldsToPass());
  implementation->SetNumberOfIsoValues(this->GetNumberOfIsoValues());
  for (int i = 0; i < this->GetNumberOfIsoValues(); i++)
  {
    implementation->SetIsoValue(i, this->GetIsoValue(i));
  }

  return implementation->Execute(inDataSet);
}
} // namespace contour
} // namespace filter
} // namespace viskores
