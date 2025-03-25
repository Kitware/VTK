// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
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
#include "vtkSmartPointer.h"          // for vtkSmartPointer
#include "vtkSystemIncludes.h"

#include <functional> // for std::function
#include <memory>     // for unique_ptr

VTK_ABI_NAMESPACE_BEGIN
class vtkAbstractArray;
class vtkDataSetAttributes;
class vtkFieldData;
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

  ///@{
  /**
   * These methods can called to generate and update the output
   * vtkDataSetAttributes. These match corresponding API on vtkDataSetAttributes
   * and can be called via the output vtkDataSetAttributes instance
   * instead as well.
   */
  void CopyAllocate(vtkDataSetAttributes* output, int ctype, vtkIdType sz, vtkIdType ext) const;
  void CopyData(int inputIndex, vtkDataSetAttributes* input, vtkIdType fromId,
    vtkDataSetAttributes* output, vtkIdType toId) const;
  void CopyData(int inputIndex, vtkDataSetAttributes* input, vtkIdType inputStart,
    vtkIdType numValues, vtkDataSetAttributes* output, vtkIdType outStart) const;
  void InterpolatePoint(int inputIndex, vtkDataSetAttributes* input, vtkIdList* inputIds,
    double* weights, vtkDataSetAttributes* output, vtkIdType toId) const;
  ///@}

  /**
   * Use this method to provide a custom callback function to invoke for each
   * array in the input and corresponding array in the output.
   */
  void TransformData(int inputIndex, vtkFieldData* input, vtkFieldData* output,
    std::function<void(vtkAbstractArray*, vtkAbstractArray*)> op) const;

  /**
   * This method can be used to determine the number of arrays remaining
   * after intersection or union operations. See also
   * vtkFieldData::GetNumberOfArrays().
   */
  int GetNumberOfArrays();

  /**
   * A convenience function that builds a prototype / template dataset
   * attributes for initializing the process of attribute interpolation and
   * copying. The supplied protoPD should be initialized (empty), and the
   * arrays present in this field list are instantiated and added to the
   * prototype attributes. The typical usage is to use field list
   * intersection (or union) operations to build up the field list, then
   * create the prototype. Note, to retain an order of the data arrays,
   * an optional ordering dataset attributes may be provided. (This is
   * necessary because the vtkDataSetAttributesFieldList does not necessarily
   * retain the original order of data arrays.)
   */
  void BuildPrototype(vtkDataSetAttributes* protoDSA, vtkDataSetAttributes* ordering = nullptr);

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

VTK_ABI_NAMESPACE_END
#endif
// VTK-HeaderTest-Exclude: vtkDataSetAttributesFieldList.h
