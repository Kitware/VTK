/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageIterateFilter.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to C. Charles Law who developed this class.

Copyright (c) 1993-1995 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
// .NAME vtkImageIterateFilter - Multiple executes per update.
// .SECTION Description
// vtkImageIterateFilter is a filter superclass that supports calling execute
// multiple times per update.  The largest hack/open issue is that the input
// and output caches are temporarily changed to "fool" the subclasses.  I
// believe the correct solution is to pass the in and out cache to the
// subclasses methods as arguments.  Now the data is passes.  Can the caches
// be passed, and data retieved from the cache? 

#ifndef __vtkImageIterateFilter_h
#define __vtkImageIterateFilter_h


#include "vtkImageToImageFilter.h"
#include "vtkMultiThreader.h"

class VTK_EXPORT vtkImageIterateFilter : public vtkImageToImageFilter
{
public:
  static vtkImageIterateFilter *New() {return new vtkImageIterateFilter;};
  const char *GetClassName() {return "vtkImageIterateFilter";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get which iteration is current being performed. Normally the
  // user will not access this method.
  vtkGetMacro(Iteration,int);
  vtkGetMacro(NumberOfIterations,int);  
  
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
  
  // Ends up calling ComputeRequiredInputUpdateExtent(int inExt[6],int outExt[6])
  // for each iteration.
  int ComputeDivisionExtents(vtkDataObject *output,
			     int division, int numDivisions);

  // Superclass API: Calls Execute(vtkImageData *inData, vtkImageData *outData)
  // for each iteration.
  void Execute();
  // defined in superclass, but hidden by Execute().
  void Execute(vtkImageData *inData, vtkImageData *outData);
  
  // Allows subclass to specify the number of iterations  
  virtual void SetNumberOfIterations(int num);
  
  // for filteres that execute multiple times.
  int NumberOfIterations;
  int Iteration;
  // A list of intermediate caches that is created when 
  // is called SetNumberOfIterations()
  vtkImageData **IterationData;
  
  // returns correct vtkImageDatas based on current iteration.
  vtkImageData *GetIterationInput();
  vtkImageData *GetIterationOutput();
};

#endif







