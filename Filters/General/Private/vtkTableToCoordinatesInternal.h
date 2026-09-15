// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkABINamespace.h"
#include "vtkType.h"

#include <vector>

VTK_ABI_NAMESPACE_BEGIN
class vtkDataArray;
class vtkDataSetAttributes;
class vtkDoubleArray;
class vtkFieldData;
class vtkIdTypeArray;

namespace vtkTableToCoordinatesInternal
{
/**
 * Fill output array with unique, sorted values from the given input.
 *
 * Return true on success, false on any error.
 */
bool ComputeUniqueAndSorted(vtkDataArray* input, vtkDataArray* output);

/**
 * Construct a N-dimension space from the list of points.
 *
 * @arg inputPoints: a list of points in a N-dimension space.
 * Each tuple of inputPoints is a point in N-dimension, N being the number
 * of arrays.
 * @arg space: the output N-dimension space. Each output array in space contains
 * unique and sorted values, extracted from the inputPoints array.
 * In space, each array can have its own number of tuples (i.e. each axis has its own dimension)
 * This can be seen as a N-dimension extension of a vtkRectilinearGrid.
 *
 * Non numerical input arrays are skipped.
 *
 * @see ComputeUniqueAndSorted
 */
void ConstructSpace(vtkFieldData* space, vtkDataSetAttributes* inputPoints);

/**
 * Map every point index from space to the randomly ordered and sparsed source arrays
 * from inputPoints.
 *
 * @arg space: an extension to N-dimension of a vtkRectilinearGrid.
 * Thus, arrays in space should have unique and ordered values.
 * But they can have different number of tuples.
 *
 * @arg inputPoints: a list of point in space, without any requirement on order.
 * So each arrays should have the same number of tuples.
 *
 * @arg inputToSpaceMap: the map filled by this method.
 * Some inputPoint may not exists in space. In that case, inputToSpaceMap will point to
 * invalidMapping id.
 * @arg invalidMapping: the value to use in the map for invalid input point.
 *
 * @see ConstructSpace to get such space from the inputPoint container.
 */
void MapSpaceToInput(vtkFieldData* space, vtkDataSetAttributes* inputPoints,
  vtkIdTypeArray* inputToSpaceMap, vtkIdType invalidMapping);

/**
 * Given a structured N-dimension space,
 * compute the structured coordinates from an inputPoint and return its idx in space.
 */
vtkIdType GetPointIdxInSpace(
  vtkFieldData* space, vtkDataSetAttributes* inputPoints, vtkIdType inputPointId);

/**
 * Given a random point, return its surrounding point in space with associated weight.
 * @arg space: as filled by ConstructSpace. Each array has unique, sorted values.
 * @arg point: coordinates of a random point in space.
 * @arg pointIdx: output variable with the list of surrounding point indices.
 * In some way, they represent the structured cell that includes the given point.
 * @arg weights: output variable with the weights of each surrounding point,
 * for linear interpolation purpose.
 */
void GetSurroundingPoints(vtkFieldData* space, const std::vector<double>& point,
  vtkIdTypeArray* pointIdx, vtkDoubleArray* weights);

}
VTK_ABI_NAMESPACE_END
