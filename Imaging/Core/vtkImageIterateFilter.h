// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkImageIterateFilter
 * @brief   Multiple executes per update.
 *
 * vtkImageIterateFilter is a filter superclass that supports calling execute
 * multiple times per update.  The largest hack/open issue is that the input
 * and output caches are temporarily changed to "fool" the subclasses.  I
 * believe the correct solution is to pass the in and out cache to the
 * subclasses methods as arguments.  Now the data is passes.  Can the caches
 * be passed, and data retrieved from the cache?
 */

#ifndef vtkImageIterateFilter_h
#define vtkImageIterateFilter_h

#include "vtkImagingCoreModule.h" // For export macro
#include "vtkThreadedImageAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKIMAGINGCORE_EXPORT vtkImageIterateFilter : public vtkThreadedImageAlgorithm
{
public:
  vtkTypeMacro(vtkImageIterateFilter, vtkThreadedImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Get which iteration is current being performed. Normally the
   * user will not access this method.
   */
  vtkGetMacro(Iteration, int);
  vtkGetMacro(NumberOfIterations, int);
  ///@}

protected:
  vtkImageIterateFilter();
  ~vtkImageIterateFilter() override;

  // Implement standard requests by calling iterative versions the
  // specified number of times.
  int RequestUpdateExtent(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  // Iterative versions of standard requests.  These are given the
  // pipeline information object for the in/out pair at each
  // iteration.
  virtual int IterativeRequestInformation(vtkInformation* in, vtkInformation* out);
  virtual int IterativeRequestUpdateExtent(vtkInformation* in, vtkInformation* out);
  virtual int IterativeRequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*);

  virtual void SetNumberOfIterations(int num);

  // for filters that execute multiple times.
  int NumberOfIterations;
  int Iteration;
  // A list of intermediate caches that is created when
  // is called SetNumberOfIterations()
  vtkAlgorithm** IterationData;

  vtkInformationVector* InputVector;
  vtkInformationVector* OutputVector;

private:
  vtkImageIterateFilter(const vtkImageIterateFilter&) = delete;
  void operator=(const vtkImageIterateFilter&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
