// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkForEach_h
#define vtkForEach_h

#include "vtkCommonExecutionModelModule.h" // for export macro
#include "vtkDataObjectAlgorithm.h"

#include <memory> // for std::unique_ptr

/**
 * @class vtkForEach
 * @brief Algorithm allowing to implement a for loop using the VTK pipeline and a sister filter
 * vtkEndFor
 *
 * This filter begins a for loop that can execute a portion of a pipeline (sub-pipeline) a certain
 * number of times. To be used in conjunction with the `vtkEndFor` filter that should end the loop.
 *
 * > Largely inspired by the ttkForEach/ttkEndFor in the TTK project
 * > (https://github.com/topology-tool-kit/ttk/tree/dev)
 *
 * @sa vtkEndFor, vtkExecutionRange
 */

VTK_ABI_NAMESPACE_BEGIN

class vtkEndFor;
class vtkExecutionRange;
class vtkInformationObjectBaseKey;
class VTKCOMMONEXECUTIONMODEL_EXPORT vtkForEach : public vtkDataObjectAlgorithm
{
public:
  static vtkForEach* New();
  vtkTypeMacro(vtkForEach, vtkDataObjectAlgorithm);
  void PrintSelf(std::ostream& os, vtkIndent indent) override;

  /**
   * Information key used to pass this filter into the pipeline
   */
  static vtkInformationObjectBaseKey* FOR_EACH_FILTER();

  /**
   * Range object to use to control execution loop
   */
  virtual void SetRange(vtkExecutionRange*);

  /**
   * Method indicating whether the filter is currently iterating
   */
  virtual bool IsIterating();

  /**
   * Go to next iteration
   */
  void Iter();

  /**
   * Method for registering the end of the loop vtkEndFor filter
   */
  virtual void RegisterEndFor(vtkEndFor*);

protected:
  vtkForEach();
  ~vtkForEach() override;

  int RequestDataObject(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestUpdateExtent(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkForEach(const vtkForEach&) = delete;
  void operator=(const vtkForEach&) = delete;

  struct Internals;
  std::unique_ptr<Internals> Internal;
};

VTK_ABI_NAMESPACE_END

#endif // vtkForEach_h
