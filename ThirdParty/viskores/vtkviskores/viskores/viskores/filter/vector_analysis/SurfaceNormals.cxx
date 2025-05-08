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
#include <viskores/filter/vector_analysis/SurfaceNormals.h>
#include <viskores/filter/vector_analysis/worklet/OrientNormals.h>
#include <viskores/filter/vector_analysis/worklet/SurfaceNormals.h>
#include <viskores/worklet/TriangleWinding.h>

namespace viskores
{
namespace filter
{

namespace vector_analysis
{
namespace
{
inline std::string ComputePointNormalsName(const SurfaceNormals* filter)
{
  if (!filter->GetPointNormalsName().empty())
  {
    return filter->GetPointNormalsName();
  }
  else if (!filter->GetOutputFieldName().empty())
  {
    return filter->GetOutputFieldName();
  }
  else
  {
    return "Normals";
  }
}

inline std::string ComputeCellNormalsName(const SurfaceNormals* filter)
{
  if (!filter->GetCellNormalsName().empty())
  {
    return filter->GetCellNormalsName();
  }
  else if (!filter->GetGeneratePointNormals() && !filter->GetOutputFieldName().empty())
  {
    return filter->GetOutputFieldName();
  }
  else
  {
    return "Normals";
  }
}

} // internal

SurfaceNormals::SurfaceNormals()
{
  this->SetUseCoordinateSystemAsField(true);
}

viskores::cont::DataSet SurfaceNormals::DoExecute(const viskores::cont::DataSet& inputDataSet)
{
  if (!this->GetUseCoordinateSystemAsField())
  {
    VISKORES_LOG_S(viskores::cont::LogLevel::Warn,
                   "Active scalars to SurfaceNormals filter must be a coordinate system. "
                   "Ignoring false UseCoordinateSystemAsField flag.");
  }

  if (!this->GenerateCellNormals && !this->GeneratePointNormals)
  {
    throw viskores::cont::ErrorFilterExecution("No normals selected.");
  }

  const auto& inputCellSet = inputDataSet.GetCellSet();
  const auto& coords =
    inputDataSet.GetCoordinateSystem(this->GetActiveCoordinateSystemIndex()).GetDataAsMultiplexer();

  viskores::cont::ArrayHandle<viskores::Vec3f> faceNormals;
  auto resolveType = [&](const auto& concrete)
  {
    viskores::worklet::FacetedSurfaceNormals faceted;
    faceted.SetNormalize(this->NormalizeCellNormals);
    faceted.Run(inputCellSet, concrete, faceNormals);
  };
  this->CastAndCallVecField<3>(coords, resolveType);

  viskores::cont::DataSet outputDataSet;
  viskores::cont::ArrayHandle<viskores::Vec3f> pointNormals;
  if (this->GeneratePointNormals)
  {
    viskores::worklet::SmoothSurfaceNormals smooth;
    smooth.Run(inputCellSet, faceNormals, pointNormals);
    outputDataSet =
      this->CreateResultFieldPoint(inputDataSet, ComputePointNormalsName(this), pointNormals);
    if (this->GenerateCellNormals)
    {
      outputDataSet.AddField(
        viskores::cont::make_FieldCell(ComputeCellNormalsName(this), faceNormals));
    }
  }
  else
  {
    outputDataSet =
      this->CreateResultFieldCell(inputDataSet, ComputeCellNormalsName(this), faceNormals);
  }

  if (this->AutoOrientNormals)
  {
    using Orient = viskores::worklet::OrientNormals;

    if (this->GenerateCellNormals && this->GeneratePointNormals)
    {
      Orient::RunPointAndCellNormals(inputCellSet, coords, pointNormals, faceNormals);
    }
    else if (this->GenerateCellNormals)
    {
      Orient::RunCellNormals(inputCellSet, coords, faceNormals);
    }
    else if (this->GeneratePointNormals)
    {
      Orient::RunPointNormals(inputCellSet, coords, pointNormals);
    }

    if (this->FlipNormals)
    {
      if (this->GenerateCellNormals)
      {
        Orient::RunFlipNormals(faceNormals);
      }
      if (this->GeneratePointNormals)
      {
        Orient::RunFlipNormals(pointNormals);
      }
    }
  }

  if (this->Consistency && this->GenerateCellNormals)
  {
    auto newCells = viskores::worklet::TriangleWinding::Run(inputCellSet, coords, faceNormals);
    outputDataSet.SetCellSet(newCells); // Overwrite the inputCellSet in the outputDataSet
  }

  return outputDataSet;
}
} // namespace vector_analysis
} // namespace filter
} // namespace viskores
