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
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//=========================================================================
#include <viskores/cont/Algorithm.h>
#include <viskores/cont/ArrayCopy.h>
#include <viskores/cont/ErrorFilterExecution.h>
#include <viskores/filter/mesh_info/MeshQuality.h>
#include <viskores/filter/mesh_info/MeshQualityArea.h>
#include <viskores/filter/mesh_info/MeshQualityAspectGamma.h>
#include <viskores/filter/mesh_info/MeshQualityAspectRatio.h>
#include <viskores/filter/mesh_info/MeshQualityCondition.h>
#include <viskores/filter/mesh_info/MeshQualityDiagonalRatio.h>
#include <viskores/filter/mesh_info/MeshQualityDimension.h>
#include <viskores/filter/mesh_info/MeshQualityJacobian.h>
#include <viskores/filter/mesh_info/MeshQualityMaxAngle.h>
#include <viskores/filter/mesh_info/MeshQualityMaxDiagonal.h>
#include <viskores/filter/mesh_info/MeshQualityMinAngle.h>
#include <viskores/filter/mesh_info/MeshQualityMinDiagonal.h>
#include <viskores/filter/mesh_info/MeshQualityOddy.h>
#include <viskores/filter/mesh_info/MeshQualityRelativeSizeSquared.h>
#include <viskores/filter/mesh_info/MeshQualityScaledJacobian.h>
#include <viskores/filter/mesh_info/MeshQualityShape.h>
#include <viskores/filter/mesh_info/MeshQualityShapeAndSize.h>
#include <viskores/filter/mesh_info/MeshQualityShear.h>
#include <viskores/filter/mesh_info/MeshQualitySkew.h>
#include <viskores/filter/mesh_info/MeshQualityStretch.h>
#include <viskores/filter/mesh_info/MeshQualityTaper.h>
#include <viskores/filter/mesh_info/MeshQualityVolume.h>
#include <viskores/filter/mesh_info/MeshQualityWarpage.h>

namespace viskores
{
namespace filter
{
namespace mesh_info
{
namespace
{
//Names of the available cell metrics, for use in
//the output dataset fields
const std::map<CellMetric, std::string> MetricNames = {
  { CellMetric::None, "-empty-metric-" },
  { CellMetric::Area, "area" },
  { CellMetric::AspectGamma, "aspectGamma" },
  { CellMetric::AspectRatio, "aspectRatio" },
  { CellMetric::Condition, "condition" },
  { CellMetric::DiagonalRatio, "diagonalRatio" },
  { CellMetric::Dimension, "dimension" },
  { CellMetric::Jacobian, "jacobian" },
  { CellMetric::MaxAngle, "maxAngle" },
  { CellMetric::MaxDiagonal, "maxDiagonal" },
  { CellMetric::MinAngle, "minAngle" },
  { CellMetric::MinDiagonal, "minDiagonal" },
  { CellMetric::Oddy, "oddy" },
  { CellMetric::RelativeSizeSquared, "relativeSizeSquared" },
  { CellMetric::ScaledJacobian, "scaledJacobian" },
  { CellMetric::Shape, "shape" },
  { CellMetric::ShapeAndSize, "shapeAndSize" },
  { CellMetric::Shear, "shear" },
  { CellMetric::Skew, "skew" },
  { CellMetric::Stretch, "stretch" },
  { CellMetric::Taper, "taper" },
  { CellMetric::Volume, "volume" },
  { CellMetric::Warpage, "warpage" }
};
} // anonymous namespace

VISKORES_CONT MeshQuality::MeshQuality()
{
  this->SetUseCoordinateSystemAsField(true);
  this->SetOutputFieldName(MetricNames.at(this->MyMetric));
}

VISKORES_CONT MeshQuality::MeshQuality(CellMetric metric)
  : MyMetric(metric)
{
  this->SetUseCoordinateSystemAsField(true);
  this->SetOutputFieldName(MetricNames.at(this->MyMetric));
}

VISKORES_CONT void MeshQuality::SetMetric(CellMetric metric)
{
  this->MyMetric = metric;
  this->SetOutputFieldName(this->GetMetricName());
}

VISKORES_CONT std::string MeshQuality::GetMetricName() const
{
  return MetricNames.at(this->MyMetric);
}

VISKORES_CONT viskores::cont::DataSet MeshQuality::DoExecute(const viskores::cont::DataSet& input)
{
  std::unique_ptr<viskores::filter::Filter> implementation;
  switch (this->MyMetric)
  {
    case viskores::filter::mesh_info::CellMetric::Area:
      implementation.reset(new viskores::filter::mesh_info::MeshQualityArea);
      break;
    case viskores::filter::mesh_info::CellMetric::AspectGamma:
      implementation.reset(new viskores::filter::mesh_info::MeshQualityAspectGamma);
      break;
    case viskores::filter::mesh_info::CellMetric::AspectRatio:
      implementation.reset(new viskores::filter::mesh_info::MeshQualityAspectRatio);
      break;
    case viskores::filter::mesh_info::CellMetric::Condition:
      implementation.reset(new viskores::filter::mesh_info::MeshQualityCondition);
      break;
    case viskores::filter::mesh_info::CellMetric::DiagonalRatio:
      implementation.reset(new viskores::filter::mesh_info::MeshQualityDiagonalRatio);
      break;
    case viskores::filter::mesh_info::CellMetric::Dimension:
      implementation.reset(new viskores::filter::mesh_info::MeshQualityDimension);
      break;
    case viskores::filter::mesh_info::CellMetric::Jacobian:
      implementation.reset(new viskores::filter::mesh_info::MeshQualityJacobian);
      break;
    case viskores::filter::mesh_info::CellMetric::MaxAngle:
      implementation.reset(new viskores::filter::mesh_info::MeshQualityMaxAngle);
      break;
    case viskores::filter::mesh_info::CellMetric::MaxDiagonal:
      implementation.reset(new viskores::filter::mesh_info::MeshQualityMaxDiagonal);
      break;
    case viskores::filter::mesh_info::CellMetric::MinAngle:
      implementation.reset(new viskores::filter::mesh_info::MeshQualityMinAngle);
      break;
    case viskores::filter::mesh_info::CellMetric::MinDiagonal:
      implementation.reset(new viskores::filter::mesh_info::MeshQualityMinDiagonal);
      break;
    case viskores::filter::mesh_info::CellMetric::Oddy:
      implementation.reset(new viskores::filter::mesh_info::MeshQualityOddy);
      break;
    case viskores::filter::mesh_info::CellMetric::RelativeSizeSquared:
      implementation.reset(new viskores::filter::mesh_info::MeshQualityRelativeSizeSquared);
      break;
    case viskores::filter::mesh_info::CellMetric::ScaledJacobian:
      implementation.reset(new viskores::filter::mesh_info::MeshQualityScaledJacobian);
      break;
    case viskores::filter::mesh_info::CellMetric::Shape:
      implementation.reset(new viskores::filter::mesh_info::MeshQualityShape);
      break;
    case viskores::filter::mesh_info::CellMetric::ShapeAndSize:
      implementation.reset(new viskores::filter::mesh_info::MeshQualityShapeAndSize);
      break;
    case viskores::filter::mesh_info::CellMetric::Shear:
      implementation.reset(new viskores::filter::mesh_info::MeshQualityShear);
      break;
    case viskores::filter::mesh_info::CellMetric::Skew:
      implementation.reset(new viskores::filter::mesh_info::MeshQualitySkew);
      break;
    case viskores::filter::mesh_info::CellMetric::Stretch:
      implementation.reset(new viskores::filter::mesh_info::MeshQualityStretch);
      break;
    case viskores::filter::mesh_info::CellMetric::Taper:
      implementation.reset(new viskores::filter::mesh_info::MeshQualityTaper);
      break;
    case viskores::filter::mesh_info::CellMetric::Volume:
      implementation.reset(new viskores::filter::mesh_info::MeshQualityVolume);
      break;
    case viskores::filter::mesh_info::CellMetric::Warpage:
      implementation.reset(new viskores::filter::mesh_info::MeshQualityWarpage);
      break;
    case viskores::filter::mesh_info::CellMetric::None:
      // Nothing to do
      return input;
  }

  VISKORES_ASSERT(implementation);

  implementation->SetOutputFieldName(this->GetOutputFieldName());
  implementation->SetActiveCoordinateSystem(this->GetActiveCoordinateSystemIndex());
  return implementation->Execute(input);
}
} // namespace mesh_info
} // namespace filter
} // namespace viskores
