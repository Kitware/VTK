// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkDataObjectToConduit
 * @brief Convert VTK Data Object to Conduit Node
 */

#ifndef vtkDataObjectToConduit_h
#define vtkDataObjectToConduit_h

#include "vtkIOCatalystConduitModule.h" // For windows import/export of shared libraries

namespace conduit_cpp
{
class Node;
}

VTK_ABI_NAMESPACE_BEGIN
class vtkDataObject;
class vtkPartitionedDataSetCollection;
VTK_ABI_NAMESPACE_END

namespace vtkDataObjectToConduit
{
VTK_ABI_NAMESPACE_BEGIN
/**
 * Fill the given conduit "data" node with the data from the data object.
 * data_object *must* be either vtkDataSet or vtkPartitionedDataSetCollection.
 * The final structure is a valid blueprint mesh (for dataset) or multimesh (for pdc).
 *
 * The conduit multimesh protocol is documented in ParaView's documentation
 * https://docs.paraview.org/en/latest/Catalyst/blueprints.html#protocol-multimesh
 *
 * In the specific case of mixed shape unstructured datasets,
 * a new field "vtkCellSizes" is added to the input data object.
 * At the moment, only vtkDataSet are supported.
 */
VTKIOCATALYSTCONDUIT_EXPORT bool FillConduitNode(
  vtkDataObject* data_object, conduit_cpp::Node& conduit_node);

/**
 * Append PDC assembly node to conduit channel.
 * The node provided is usually the parent of the "data" node given to FillConduitNode.
 */
VTKIOCATALYSTCONDUIT_EXPORT void FillConduitNodeAssembly(
  vtkPartitionedDataSetCollection* pdc, conduit_cpp::Node& conduit_node);

VTK_ABI_NAMESPACE_END
}

#endif
// VTK-HeaderTest-Exclude: vtkDataObjectToConduit.h
