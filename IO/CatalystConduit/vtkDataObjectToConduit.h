// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkDataObjectToConduit
 * @brief Convert VTK Data Object to Conduit Node
 */

#ifndef vtkDataObjectToConduit_h
#define vtkDataObjectToConduit_h

#include "vtkIOCatalystConduitModule.h" // For windows import/export of shared libraries
#include "vtkObject.h"

namespace conduit_cpp
{
class Node;
}

VTK_ABI_NAMESPACE_BEGIN
class vtkDataObject;
VTK_ABI_NAMESPACE_END

namespace vtkDataObjectToConduit
{
VTK_ABI_NAMESPACE_BEGIN
/**
 * Fill the given conduit node with the data from the data object.
 * The final structure is a valid blueprint mesh.
 *
 * In the specific case of mixed shape unstructured datasets,
 * a new field "vtkCellSizes" is added to the input data object.
 * At the moment, only vtkDataSet are supported.
 */
VTKIOCATALYSTCONDUIT_EXPORT bool FillConduitNode(
  vtkDataObject* data_object, conduit_cpp::Node& conduit_node);
VTK_ABI_NAMESPACE_END
}

#endif
// VTK-HeaderTest-Exclude: vtkDataObjectToConduit.h
