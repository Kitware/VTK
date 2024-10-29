// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @brief Reflection methods for Unstructured Grid
 *
 * This file defines a namespace providing functions used by vtkReflectionFilter and
 * vtkAxisAlignedReflectionFilter to process Unstructured Grids.
 */

#ifndef vtkReflectionUtilities_h
#define vtkReflectionUtilities_h

#include "vtkAlgorithm.h"        // for vtkAlgorithm class
#include "vtkUnstructuredGrid.h" // for vtkUnstructuredGrid class

VTK_ABI_NAMESPACE_BEGIN

namespace vtkReflectionUtilities
{

/**
 * method to determine which arrays from a field data can be reflected and store them in the
 * provided reflectableArrays. Only 3/6/9 component signed data array are considered reflectable.
 */
void FindReflectableArrays(
  vtkFieldData* fd, std::vector<std::pair<vtkIdType, int>>& reflectableArrays);

/**
 * If reflectAllInputArrays is true, calls FindReflectableArrays.
 * Otherwise, this function fills reflectableArrays with vectors, normals, and tensors.
 */
void FindAllReflectableArrays(std::vector<std::pair<vtkIdType, int>>& reflectableArrays,
  vtkDataSetAttributes* inData, bool reflectAllInputArrays);

/**
 * Reflect each component of the provided tuple according to the provided mirrorDir using the
 * provided nComp component.
 */
void ReflectTuple(double* tuple, int* mirrorDir, int nComp);

/**
 * Reflect the i-th tuple of each array in reflectableArrays.
 *
 * @param reflectableArrays Vector of all the arrays that need to be reflected and their
 * corresponding number of components.
 * @param inData DataSetAttribute in which to get the actual arrays from their ids.
 * @param outData DataSetAttribute in which to set the reflected arrays.
 * @param i Index of the tuple that needs to be reflected in the input array.
 * @param mirrorDir Mirror direction for 3-components arrays.
 * @param mirrorSymmetricTensorDir Mirror direction for 6-components arrays.
 * @param mirrorTensorDir Mirror direction for 9-components arrays.
 * @param id Index of the reflected tuple in the output array.
 */
void ReflectReflectableArrays(std::vector<std::pair<vtkIdType, int>>& reflectableArrays,
  vtkDataSetAttributes* inData, vtkDataSetAttributes* outData, vtkIdType i, int mirrorDir[3],
  int mirrorSymmetricTensorDir[6], int mirrorTensorDir[9], vtkIdType id);

/**
 * Generate new, non-3D cell and return the generated cell id.
 *
 * @param input Input dataset to be reflected.
 * @param output Output unstructured grid that is the reflection of the input and the input itself
 * if copyInput is on.
 * @param cellId Id of the cell to be reflected.
 * @param numInputPoints Number of points in the input dataset.
 * @param copyInput @see vtkAxisAlignedReflectionFilter::CopyInput.
 */
vtkIdType ReflectNon3DCellInternal(vtkDataSet* input, vtkUnstructuredGrid* output, vtkIdType cellId,
  vtkIdType numInputPoints, bool copyInput);

/**
 * Generate the reflection of the input dataset as an unstructured grid.
 *
 * @param input Input dataset to be reflected.
 * @param output Output unstructured grid that is the reflection of the input and the input itself
 * if copyInput is on.
 * @param constant Offset depending on the chosen reflection plane.
 * @param mirrorDir Mirror direction for 3-components arrays.
 * @param mirrorSymmetricTensorDir Mirror direction for 6-components arrays.
 * @param mirrorTensorDir Mirror direction for 9-components arrays.
 * @param copyInput @see vtkAxisAlignedReflectionFilter::CopyInput.
 * @param reflectAllInputArrays @see vtkAxisAlignedReflectionFilter::ReflectAllInputArrays.
 * @param algorithm Algorithm object to CheckAbort during points and cells iterations.
 */
void ProcessUnstructuredGrid(vtkDataSet* input, vtkUnstructuredGrid* output, double constant[3],
  int mirrorDir[3], int mirrorSymmetricTensorDir[6], int mirrorTensorDir[9], bool copyInput,
  bool reflectAllInputArrays, vtkAlgorithm* algorithm);
}

VTK_ABI_NAMESPACE_END

#endif
