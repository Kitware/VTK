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
#ifndef viskores_cont_SplineEvaluateRectilinearGrid_h
#define viskores_cont_SplineEvaluateRectilinearGrid_h

#include <viskores/cont/DataSet.h>
#include <viskores/exec/SplineEvaluateRectilinearGrid.h>

namespace viskores
{
namespace cont
{

/// @brief Grid evaluation for rectilinear grids using spline interpolation.
///
/// This evaluator supports evaluating fields on uniform rectilinear grids using
/// Catmull-Rom cubic spline interpolation. This can provide higher-order interpolation
/// compared to linear interpolation.
///
/// This evaluator requires the input data set to have a uniform grid structure. This
/// means the data set must use `viskores::cont::ArrayHandleCartesianProduct` for its
/// coordinate system. This evaluator will throw an error if the data set does not meet
/// these requirements.
///
/// Right now, this evaluator only supports scalar fields on 3D uniform grids. The field to be
/// evaluated is specified by name.

class VISKORES_CONT_EXPORT SplineEvaluateRectilinearGrid
  : public viskores::cont::ExecutionObjectBase
{
public:
  VISKORES_CONT SplineEvaluateRectilinearGrid(const viskores::cont::DataSet& dataSet,
                                              const std::string& fieldName)
    : DataSet(dataSet)
    , FieldName(fieldName)
  {
  }

  viskores::exec::SplineEvaluateRectilinearGrid PrepareForExecution(
    viskores::cont::DeviceAdapterId device,
    viskores::cont::Token& token) const;

private:
  viskores::cont::DataSet DataSet;
  std::string FieldName;
};

}
} // viskores::cont

#endif //viskores_cont_SplineEvaluateRectilinearGrid_h
