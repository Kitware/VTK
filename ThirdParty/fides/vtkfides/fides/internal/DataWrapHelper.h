//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//============================================================================

#ifndef DataWrapHelper_H_
#define DataWrapHelper_H_

#include <fides/DataContainer.h>
#include <fides/FidesTypes.h>

#include <memory>
#include <utility>

#if FIDES_USE_VISKORES
#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/ArrayHandleUniformPointCoordinates.h>
#include <viskores/cont/DataSet.h>
#endif

#if FIDES_USE_VTK
#include "vtkPartitionedDataSet.h"
#include "vtkSmartPointer.h"
#endif

namespace fides
{

namespace internal
{

#if FIDES_USE_VISKORES
using UniformCoordType = viskores::cont::ArrayHandleUniformPointCoordinates;

/// For the Viskores data type backed, we chose to wrap the vector of DataSets
/// due to its mutability, so that PostRead() or application code can modify it.
/// Also it's trivial and free to create a viskores::cont::PartitionedDataSet
/// from the vector)
inline std::unique_ptr<DataContainer> Wrap(std::vector<viskores::cont::DataSet>&& vec)
{
  return std::make_unique<ConcreteDataWrapper<std::vector<viskores::cont::DataSet>>>(
    std::move(vec));
}

/// Converts a fides::FieldAssociation to a viskores::cont::Field::Association.
viskores::cont::Field::Association FIDES_EXPORT
ConvertToViskoresFieldAssociation(fides::FieldAssociation assoc);

/// Converts a viskores::cont::Field::Association to a fides::FieldAssociation.
fides::FieldAssociation FIDES_EXPORT
ConvertFromViskoresFieldAssociation(viskores::cont::Field::Association assoc);

/// Converts a fides::CellShape to a Viskores cell shape type (UInt8).
viskores::UInt8 FIDES_EXPORT ConvertCellShapeToViskores(fides::CellShape shape);

/// Converts a Viskores cell shape type to a fides::CellShape.
fides::CellShape FIDES_EXPORT ConvertCellShapeFromViskores(viskores::UInt8 cellShapeType);

/// Association for fields, based on Viskores's association enum, but
/// also includes a value for representing field data.

/// Converts a Viskores cell shape type to the fides string.
/// Throws a runtime error for unsupported cell types.
std::string FIDES_EXPORT ConvertViskoresCellTypeToFides(viskores::UInt8 cellShapeType);

/// Converts a fides cell name to Viskores cell shape type.
/// Throws a runtime error for unsupported cell types.
viskores::UInt8 FIDES_EXPORT ConvertFidesCellTypeToViskores(const std::string& cellShapeName);

// Converts a Fides RawArray into a Viskores UnknownArrayHandle.
// Safely handles both single-component and multi-component (RuntimeVec) data.
// Memory ownership defaults to CopyFlag::Off (Zero-Copy) assuming Fides
// outlives the Viskores pipeline execution.
viskores::cont::UnknownArrayHandle RawArrayToUnknownArrayHandle(
  const fides::RawArray& raw,
  viskores::CopyFlag copy = viskores::CopyFlag::Off);
#endif

#if FIDES_USE_VTK
/// For the VTK data type backend, since vtkPartitionedDataSet allows mutable access,
/// we can wrap the vtkSmartPointer directly
inline std::unique_ptr<DataContainer> Wrap(vtkSmartPointer<vtkPartitionedDataSet> ds)
{
  return std::make_unique<ConcreteDataWrapper<fides::VTKCollection>>(ds);
}
#endif

#if FIDES_USE_CONDUIT
/// For the Conduit data type backend, take a Conduit node and yield exclusive
/// ownership to the reader pipeline
inline std::unique_ptr<DataContainer> Wrap(std::shared_ptr<fides::ConduitNode> node)
{
  return std::make_unique<ConcreteDataWrapper<std::shared_ptr<conduit::Node>>>(std::move(node));
}
#endif

}
}

#endif
