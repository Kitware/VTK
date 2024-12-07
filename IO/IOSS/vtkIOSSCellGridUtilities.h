// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkIOSSCellGridUtilities_h
#define vtkIOSSCellGridUtilities_h

/**
 * @namespace vtkIOSSCellGridUtilities
 * @brief internal utilities for vtkIOSSCellGridReader
 *
 * vtkIOSSCellGridReader provides some helper functions to go between vtkCellGrid
 * and Ioss. Not intended for public consumption. API likely to change without notice.
 *
 * We limit this namespace for utility functions that go between Ioss and vtkCellGrid
 * or vice-versa. Thus, methods that are not straddling that fence should be not be
 * added here.
 */

#include "vtkCellAttribute.h" // For CellTypeInfo internal class.
#include "vtkDataArraySelection.h"
#include "vtkDoubleArray.h"
#include "vtkIOSSReader.h"
#include "vtkIOSSUtilities.h"
#include "vtkLogger.h"
#include "vtkObject.h"
#include "vtkSmartPointer.h"
#include "vtkStringToken.h"
#include "vtkTypeInt32Array.h"
#include "vtkTypeInt64Array.h"
#include "vtkTypeList.h" // Needed for ArrayList definition

// Ioss includes
#include <vtk_ioss.h>
// clang-format off
#include VTK_IOSS(Ioss_Region.h)
#include VTK_IOSS(Ioss_Transform.h)
#include VTK_IOSS(Ioss_StructuredBlock.h)
#include VTK_IOSS(Ioss_SideSet.h)
#include VTK_IOSS(Ioss_SideBlock.h)
// clang-format on

#include <cassert>
#include <set>

VTK_ABI_NAMESPACE_BEGIN
class vtkCellGrid;
class vtkCellArray;
class vtkCellMetadata;
class vtkDGCell;
class vtkDataSet;
VTK_ABI_NAMESPACE_END

namespace vtkIOSSCellGridUtilities
{
VTK_ABI_NAMESPACE_BEGIN

/**
 * Returns the an instance of a vtkCellMetadata subclass that can hold
 * data corresponding to the input Ioss \a topology element.
 *
 * This may return a null pointer for unsupported types.
 * If the returned pointer is non-null, \a ioss_cell_points will
 * be set to a positive integer specifying the number of connectivity
 * entries per cell and \a ioss_cell_order will return the (presumably
 * uniform) polynomial degree of the cell along each parameter-space axis.
 * (The cell order is the value returned by Ioss_ElementTopology::order()
 * which does not appear to be the total order of the shape function.)
 */
vtkSmartPointer<vtkCellMetadata> GetCellMetadata(const Ioss::ElementTopology* topology,
  int& ioss_cell_points, int& ioss_cell_order, vtkCellGrid* cellGrid = nullptr);

/**
 * Return (if possible) an Ioss::ElementTopology object that corresponds
 * to the input cell metadata.
 *
 * This may return a null pointer.
 */
const Ioss::ElementTopology* GetElementTopology(vtkCellMetadata* cellType);

/**
 * Read connectivity and possibly ghost-node markings into \a meta.
 *
 * This also adds the arrays to \a grid in instances of
 * vtkDataSetAttributes corresponding to the cell- or side-specification
 * which the arrays are referenced.
 */
bool GetConnectivity(const Ioss::GroupingEntity* group_entity, vtkCellGrid* grid, vtkDGCell* meta,
  int ioss_cell_points, int spec_index = -1, const std::string& group_name = std::string(),
  vtkIOSSUtilities::Cache* cache = nullptr);

/**
 * Read connectivity information from the group_entity with offsetting
 * but without permutation.
 *
 * Unlike GetConnectivity(), which may permute the connectivity entries
 * from the IOSS/Exodus standard into the expected VTK node ordering, this
 * method does no processing and returns a raw array of node IDs.
 * This is used to construct vtkCellGrid objects that use different basis
 * functions (and thus different node orderings).
 *
 * However, all connectivity arrays returned by this method adjust node IDs
 * to be zero-based (instead of 1-based).
 *
 * Returns an integer array and the element type for elements in this
 * group_entity. Note that the returned array may be vtkTypeInt32Array,
 * vtkTypeInt64Array, or vtkIdTypeArray; no guarantees are made for which
 * type is returned.
 *
 * NOTE: this does not support entity groups with mixed topological elements.
 *
 * Throws `std::runtime_error` on error.
 */
vtkSmartPointer<vtkCellMetadata> GetCellMetadata(const Ioss::GroupingEntity* group_entity,
  int& ioss_cell_points, int& ioss_cell_order, vtkCellGrid* cell_grid = nullptr,
  vtkIOSSUtilities::Cache* cache = nullptr);

/**
 * Fetch the nodal coordinates for \a group_entity and create the
 * \a cell_grid's shape attribute.
 *
 * The \a ioss_cell_order refers to the polynomial order of the
 * interpolating function along each axis of the cell.
 * This is used to choose the cell-attribute's type (in turn
 * used to choose basis functions for evaluating/rendering cells).
 */
bool GetShape(Ioss::Region* region, const Ioss::GroupingEntity* group_entity,
  vtkCellAttribute::CellTypeInfo& cellShapeInfo, int timestep, vtkDGCell* meta,
  vtkCellGrid* grid = nullptr, vtkIOSSUtilities::Cache* cache = nullptr);

VTK_ABI_NAMESPACE_END
}

#endif
// VTK-HeaderTest-Exclude: vtkIOSSCellGridUtilities.h
