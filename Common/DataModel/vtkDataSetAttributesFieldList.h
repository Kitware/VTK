/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataSetAttributesFieldList.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkDataSetAttributesFieldList
 * @brief helps manage arrays from multiple vtkDataSetAttributes.
 *
 * vtkDataSetAttributesFieldList, also called vtkDataSetAttributes::FieldList,
 * is used to help with filters when dealing with arrays from multiple
 * vtkDataSetAttributes instances, potentially from multiple inputs.
 *
 * Consider a filter that appends multiple inputs, e.g. vtkAppendPolyData.
 * Besides appending mesh elements, such a filter also needs to combine field
 * arrays (point, and cell data) from inputs to pass on to the output.
 * Now if all the inputs had exactly the same set of arrays, we're all set.
 * However, more often than not, the inputs will have different sets of arrays.
 * The filter will need to match up from various inputs to combine together,
 * potentially dropping arrays not in all inputs. Furthermore, it needs to
 * ensure arrays in the output are flagged as attributes consistently. All of
 * this can be done using vtkDataSetAttributesFieldList.
 *
 * @section Usage Usage
 *
 * Typical usage is as follows:
 * 1. call `IntersectFieldList` or `UnionFieldList` for all input vtkDataSetAttributes
 *   instances,
 * 2. allocate arrays for the output vtkDataSetAttributes by using `CopyAllocate`,
 * 3. call `CopyData` per input (preserving the input order used in step 1) to
 *    copy tuple(s) from input to the output.
 *
 * `vtkDataSetAttributes::InitializeFieldList` is provided for API compatibility
 * with previous implementation of this class and it not required to be called.
 * Simply calling `UnionFieldList` or `IntersectFieldList` for the first
 * vtkDataSetAttributes instance is sufficient.
 *
 * `CopyAllocate, `CopyData`, and `InterpolatePoint` methods on this class
 * are called by similarly named variants on vtkDataSetAttributes that take in a
 * FieldList instance as an argument. Hence, either forms may be used.
 *
 * Calls to `UnionFieldList` and `IntersectFieldList` cannot be mixed. Use
 * `Reset` or `InitializeFieldList` to change mode and start reinitialization.
 */

#ifndef vtkDataSetAttributesFieldList_h
#define vtkDataSetAttributesFieldList_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkSetGet.h"                // for VTK_LEGACY
#include "vtkSmartPointer.h"          // for vtkSmartPointer
#include "vtkSystemIncludes.h"

#include <functional>                 // for std::function
#include <memory>                     // for unique_ptr

class vtkAbstractArray;
class vtkDataSetAttributes;
class vtkIdList;

class VTKCOMMONDATAMODEL_EXPORT vtkDataSetAttributesFieldList
{
public:
  /**
   * `number_of_inputs` parameter is not required and only provided for
   * backwards compatibility.
   */
  vtkDataSetAttributesFieldList(int number_of_inputs = 0);
  virtual ~vtkDataSetAttributesFieldList();
  void PrintSelf(ostream& os, vtkIndent indent);

  /**
   * Initializes the field list to empty.
   */
  void Reset();

  /**
   * Initialize the field list. This also adds the first input.
   * Calling this method is optional. The first call to `IntersectFieldList` or
   * `UnionFieldList` on a new instance or after calling `Reset()` will have the
   * same effect.
   */
  void InitializeFieldList(vtkDataSetAttributes* dsa);

  /**
   * Update the field list for an intersection of arrays registered so far and
   * those in `dsa`.
   */
  void IntersectFieldList(vtkDataSetAttributes* dsa);

  /**
   * Update the field list for an union of arrays registered so far and
   * those in `dsa`.
   */
  void UnionFieldList(vtkDataSetAttributes* dsa);

  //@{
  /**
   * These methods can called to generate and update the output
   * vtkDataSetAttributes. These match corresponding API on vtkDataSetAttributes
   * and can be called via the output vtkDataSetAttributes instance
   * instead as well.
   */
  void CopyAllocate(vtkDataSetAttributes* output, int ctype, vtkIdType sz, vtkIdType ext) const;
  void CopyData(int inputIndex, vtkDataSetAttributes* input, vtkIdType fromId,
    vtkDataSetAttributes* output, vtkIdType toId) const;
  void CopyData(int inputIdx, vtkDataSetAttributes* input, vtkIdType inputStart,
    vtkIdType numValues, vtkDataSetAttributes* output, vtkIdType outStart) const;
  void InterpolatePoint(int inputIdx, vtkDataSetAttributes* input, vtkIdList* inputIds,
    double* weights, vtkDataSetAttributes* output, vtkIdType toId) const;
  //@}

  /**
   * Use this method to provide a custom callback function to invoke for each
   * array in the input and corresponding array in the output.
   */
  void TransformData(int inputIndex, vtkDataSetAttributes* input, vtkDataSetAttributes* output,
    std::function<void(vtkAbstractArray*, vtkAbstractArray*)> op) const;

  //@{
  /**
   * vtkDataSetAttributes::FieldList used a different internal data structure in
   * older versions of VTK. This exposes that API for legacy applications.
   *
   * Using these methods should be avoided in new code and should be replaced in
   * old code as these methods can be slow.
   *
   * @deprecated VTK 8.2
   */
  VTK_LEGACY(int IsAttributePresent(int attrType) const);
  VTK_LEGACY(int GetNumberOfFields() const);
  VTK_LEGACY(int GetFieldIndex(int i) const);
  VTK_LEGACY(const char* GetFieldName(int i) const);
  VTK_LEGACY(int GetFieldComponents(int i) const);
  VTK_LEGACY(int GetDSAIndex(int index, int i) const);
  //@}

protected:
  /**
   * Called to create an output array for the given type.
   * Default implementation calls `vtkAbstractArray::CreateArray()`.
   */
  virtual vtkSmartPointer<vtkAbstractArray> CreateArray(int type) const;

private:
  class vtkInternals;
  std::unique_ptr<vtkInternals> Internals;

  vtkDataSetAttributesFieldList(const vtkDataSetAttributesFieldList&) = delete;
  void operator=(vtkDataSetAttributesFieldList&) = delete;
};

#endif
// VTK-HeaderTest-Exclude: vtkDataSetAttributesFieldList.h
