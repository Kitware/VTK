/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageIterateFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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

class VTKIMAGINGCORE_EXPORT vtkImageIterateFilter : public vtkThreadedImageAlgorithm
{
public:
  vtkTypeMacro(vtkImageIterateFilter,vtkThreadedImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * Get which iteration is current being performed. Normally the
   * user will not access this method.
   */
  vtkGetMacro(Iteration,int);
  vtkGetMacro(NumberOfIterations,int);
  //@}

protected:
  vtkImageIterateFilter();
  ~vtkImageIterateFilter();

  // Implement standard requests by calling iterative versions the
  // specified number of times.
  virtual int RequestUpdateExtent(vtkInformation*,
                                  vtkInformationVector**,
                                  vtkInformationVector*) VTK_OVERRIDE;
  virtual int RequestInformation (vtkInformation*,
                                  vtkInformationVector**,
                                  vtkInformationVector*) VTK_OVERRIDE;
  virtual int RequestData(vtkInformation* request,
                          vtkInformationVector** inputVector,
                          vtkInformationVector* outputVector) VTK_OVERRIDE;

  // Iterative versions of standard requests.  These are given the
  // pipeline information object for the in/out pair at each
  // iteration.
  virtual int IterativeRequestInformation(vtkInformation* in,
                                          vtkInformation* out);
  virtual int IterativeRequestUpdateExtent(vtkInformation* in,
                                           vtkInformation* out);
  virtual int IterativeRequestData(vtkInformation*,
                                   vtkInformationVector**,
                                   vtkInformationVector*);

  virtual void SetNumberOfIterations(int num);

  // for filters that execute multiple times.
  int NumberOfIterations;
  int Iteration;
  // A list of intermediate caches that is created when
  // is called SetNumberOfIterations()
  vtkAlgorithm **IterationData;

  vtkInformationVector* InputVector;
  vtkInformationVector* OutputVector;
private:
  vtkImageIterateFilter(const vtkImageIterateFilter&) VTK_DELETE_FUNCTION;
  void operator=(const vtkImageIterateFilter&) VTK_DELETE_FUNCTION;
};

#endif







