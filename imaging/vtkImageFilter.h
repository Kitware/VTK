/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageFilter.h
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
// .NAME vtkImageFilter - Generic filter that has one input..
// .SECTION Description
// vtkImageFilter is a filter class that can hide som of the pipeline 
// complexity.  This super class will loop over extra dimensions so the
// subclass can deal with simple low dimensional regions.
// "ComputeRequiredInputExtent(vtkImageRegion *out, *in)" must set the
// extent of in required to compute out. 
// The advantage of using the execute method is that this super class
// will automatically break the execution into pieces if the 
// InputMemroyLimit is violated or the input request fails.
// This creates streaming where the pipeline processes images
// in dynamically sized pieces.



#ifndef __vtkImageFilter_h
#define __vtkImageFilter_h


#include "vtkImageCachedSource.h"
#include "vtkStructuredPointsToImage.h"
class vtkImageRegion;
class vtkImageCache;

class VTK_EXPORT vtkImageFilter : public vtkImageCachedSource
{
public:
  vtkImageFilter();
  static vtkImageFilter *New() {return new vtkImageFilter;};
  char *GetClassName() {return "vtkImageFilter";};
  void PrintSelf(ostream& os, vtkIndent indent);

  virtual void SetInput(vtkImageCache *input);
  void SetInput(vtkStructuredPoints *spts)
    {this->SetInput(spts->GetStructuredPointsToImage()->GetOutput());}
  
  void Update(vtkImageRegion *outRegion);
  void UpdateImageInformation(vtkImageRegion *region);
  unsigned long int GetPipelineMTime();
  
  // Description:
  // Get input to this filter.
  vtkGetObjectMacro(Input,vtkImageCache);

  // Description:
  // Set/Get input memory limit.  Make this smaller to stream.
  vtkSetMacro(InputMemoryLimit,long);
  vtkGetMacro(InputMemoryLimit,long);

  // Description:
  // Set/Get the order to split region requests if we need to stream
  void SetSplitOrder(int num, int *axes);
  vtkImageSetMacro(SplitOrder,int);
  void GetSplitOrder(int num, int *axes);
  vtkImageGetMacro(SplitOrder,int);
  int *GetSplitOrder() {return this->SplitOrder;};

protected:
  vtkImageCache *Input;     
  
  int Dimensionality;
  // This is set in the subclass constructor and (probably) should not be 
  // changed.  It indicates the dimensionality of the regions the
  // execute/update methods expect.  It may be larger than dimensionality
  // to make the filter faster (this supper class is not efficient at
  // looping over extra dimensions.
  int ExecuteDimensionality;
  
  int SplitOrder[VTK_IMAGE_DIMENSIONS];
  int NumberOfSplitAxes;
  long InputMemoryLimit;
  
  virtual void ComputeOutputImageInformation(vtkImageRegion *inRegion,
					     vtkImageRegion *outRegion);
  virtual void ComputeRequiredInputRegionExtent(vtkImageRegion *outRegion,
						vtkImageRegion *inRegion);
  void RecursiveStreamUpdate(vtkImageRegion *inRegion, 
			     vtkImageRegion *outRegion);
  virtual void RecursiveLoopExecute(int dim, vtkImageRegion *inRegion, 
				    vtkImageRegion *outRegion);
  virtual void Execute(vtkImageRegion *inRegion, vtkImageRegion *outRegion);
};

#endif







