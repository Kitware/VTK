/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageIterateFilter.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkImageIterateFilter - Multiple executes per update.
// .SECTION Description
// vtkImageIterateFilter is a filter superclass that supports calling execute
// multiple times per update.  The largest hack/open issue is that the input
// and output caches are temporarily changed to "fool" the subclasses.  I
// believe the correct solution is to pass the in and out cache to the
// subclasses methods as arguments.  Now the data is passes.  Can the caches
// be passed, and data retrieved from the cache? 

#ifndef __vtkImageIterateFilter_h
#define __vtkImageIterateFilter_h


#include "vtkImageToImageFilter.h"

class VTK_IMAGING_EXPORT vtkImageIterateFilter : public vtkImageToImageFilter
{
public:
  vtkTypeRevisionMacro(vtkImageIterateFilter,vtkImageToImageFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get which iteration is current being performed. Normally the
  // user will not access this method.
  vtkGetMacro(Iteration,int);
  vtkGetMacro(NumberOfIterations,int);  

  void ComputeInputUpdateExtents( vtkDataObject *output );
  //BTX
  void ComputeInputUpdateExtent( int [6], int [6] )
    { vtkErrorMacro( << "ComputeInputUpdateExtent should be implemented in subclass" );};
  //ETX

protected:
  vtkImageIterateFilter();
  ~vtkImageIterateFilter();

  // Superclass API. Sets defaults, then calls 
  // ExecuteInformation(vtkImageData *inData, vtkImageData *outData)
  // for each iteration
  void ExecuteInformation();
  // Called for each iteration (differs from superclass in arguments).
  // You should override this method if needed.
  virtual void ExecuteInformation(vtkImageData *inData, vtkImageData *outData);
  
  // Superclass API: Calls Execute(vtkImageData *inData, vtkImageData *outData)
  // for each iteration.
  void ExecuteData(vtkDataObject *output);
  virtual void IterativeExecuteData(vtkImageData *in, vtkImageData *out) = 0;
  
  // Replaces "EnlargeOutputUpdateExtent"
  virtual void AllocateOutputScalars(vtkImageData *outData);
  
  // Allows subclass to specify the number of iterations  
  virtual void SetNumberOfIterations(int num);
  
  // for filters that execute multiple times.
  int NumberOfIterations;
  int Iteration;
  // A list of intermediate caches that is created when 
  // is called SetNumberOfIterations()
  vtkImageData **IterationData;
  
  // returns correct vtkImageDatas based on current iteration.
  vtkImageData *GetIterationInput();
  vtkImageData *GetIterationOutput();
private:
  vtkImageIterateFilter(const vtkImageIterateFilter&);  // Not implemented.
  void operator=(const vtkImageIterateFilter&);  // Not implemented.
};

#endif







