// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkMultiProcessControllerHelper
 * @brief   collection of assorted helper
 * routines dealing with communication.
 *
 * vtkMultiProcessControllerHelper is collection of assorted helper
 * routines dealing with communication.
 */

#ifndef vtkMultiProcessControllerHelper_h
#define vtkMultiProcessControllerHelper_h

#include "vtkFiltersParallelModule.h" // For export macro
#include "vtkObject.h"
#include "vtkSmartPointer.h" // needed for vtkSmartPointer.

#include <vector> // needed for std::vector

VTK_ABI_NAMESPACE_BEGIN
class vtkDataObject;
class vtkMultiProcessController;
class vtkMultiProcessStream;

class VTKFILTERSPARALLEL_EXPORT vtkMultiProcessControllerHelper : public vtkObject
{
public:
  static vtkMultiProcessControllerHelper* New();
  vtkTypeMacro(vtkMultiProcessControllerHelper, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Reduce the stream to all processes calling the (*operation) for reduction.
   * The operation is assumed to be commutative.
   */
  static int ReduceToAll(vtkMultiProcessController* controller, vtkMultiProcessStream& data,
    void (*operation)(vtkMultiProcessStream& A, vtkMultiProcessStream& B), int tag);

  /**
   * Utility method to merge pieces received from several processes. It does not
   * handle all data types, and hence not meant for non-paraview specific use.
   * Returns a new instance of data object containing the merged result on
   * success, else returns nullptr. The caller is expected to release the memory
   * from the returned data-object.
   */
  static vtkDataObject* MergePieces(vtkDataObject** pieces, unsigned int num_pieces);

  /**
   * Overload where the merged pieces are combined into result.
   */
  static bool MergePieces(
    std::vector<vtkSmartPointer<vtkDataObject>>& pieces, vtkDataObject* result);

protected:
  vtkMultiProcessControllerHelper();
  ~vtkMultiProcessControllerHelper() override;

private:
  vtkMultiProcessControllerHelper(const vtkMultiProcessControllerHelper&) = delete;
  void operator=(const vtkMultiProcessControllerHelper&) = delete;
};
VTK_ABI_NAMESPACE_END
#endif
