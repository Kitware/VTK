/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageMultipleInputFilter.h
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
// .NAME vtkImageMultipleInputFilter - Generic filter that has N inputs.
// .SECTION Description
// vtkImageMultipleInputFilter is a super class for filters that 
// any number of inputs.  Steaming is not available in this class yet.

// .SECTION See Also
// vtkImageFilter vtImageInPlaceFilter vtkImageTwoInputFilter
// vtkImageTwoOutputFilter



#ifndef __vtkImageMultipleInputFilter_h
#define __vtkImageMultipleInputFilter_h


#include "vtkImageCachedSource.h"
#include "vtkStructuredPointsToImage.h"
class vtkImageRegion;
class vtkImageCache;


class VTK_EXPORT vtkImageMultipleInputFilter : public vtkImageCachedSource
{
public:
  vtkImageMultipleInputFilter();
  ~vtkImageMultipleInputFilter();
  static vtkImageMultipleInputFilter *New() {return new vtkImageMultipleInputFilter;};
  char *GetClassName() {return "vtkImageMultipleInputFilter";};
  void PrintSelf(ostream& os, vtkIndent indent);

  virtual void SetInput(int num, vtkImageCache *input);
  void SetInput(int num, vtkStructuredPoints *spts)
    {this->SetInput(num, spts->GetStructuredPointsToImage()->GetOutput());}
  
  void Update(vtkImageRegion *outRegion);
  void UpdateImageInformation(vtkImageRegion *outRegion);
  unsigned long int GetPipelineMTime();
  
  // Description:
  // Get one input to this filter.
  vtkImageCache *GetInput(int num) {return this->Inputs[num];};

  // Description:
  // Get the number of inputs to this filter
  vtkGetMacro(NumberOfInputs, int);
  
protected:
  int NumberOfInputs;
  vtkImageCache **Inputs;     // An Array of the inputs to the filter
  vtkImageRegion **Regions;   // We need an array for inputs.

  // Should be set in the constructor.
  void SetNumberOfInputs(int num);
  
  virtual void ComputeOutputImageInformation(vtkImageRegion **inRegions,
					     vtkImageRegion *outRegion);
  virtual void ComputeRequiredInputRegionExtent(vtkImageRegion *outRegion,
						vtkImageRegion **inRegions);
  virtual void RecursiveLoopExecute(int dim, vtkImageRegion **inRegions,
				    vtkImageRegion *outRegion);
  virtual void Execute(vtkImageRegion **inRegions, vtkImageRegion *outRegion);
};

#endif







