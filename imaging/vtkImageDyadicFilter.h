/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageDyadicFilter.h
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
// .NAME vtkImageDyadicFilter - Generic filter that has two inputs.
// .SECTION Description
// vtkImageDyadicFilter is a super class for filters that have two inputs.
// Dynamic splitting is not implemented for this super class (but could be).
// If a input request fails, this class just passes the failure to
// the consumer.




#ifndef __vtkImageDyadicFilter_h
#define __vtkImageDyadicFilter_h


#include "vtkImageCachedSource.h"
#include "vtkImageRegion.h"

class vtkImageDyadicFilter : public vtkImageCachedSource
{
public:
  vtkImageDyadicFilter();
  char *GetClassName() {return "vtkImageDyadicFilter";};
  void PrintSelf(ostream& os, vtkIndent indent);

  virtual void SetInput1(vtkImageSource *input);
  virtual void SetInput2(vtkImageSource *input);
  void UpdateRegion(vtkImageRegion *outRegion);
  void UpdateImageInformation(vtkImageRegion *outRegion);
  unsigned long int GetPipelineMTime();
  
  vtkSetMacro(UseExecuteMethod,int);
  vtkGetMacro(UseExecuteMethod,int);
  vtkBooleanMacro(UseExecuteMethod,int);
  
  // Description:
  // Get input to this filter.
  vtkGetObjectMacro(Input1,vtkImageSource);
  vtkGetObjectMacro(Input2,vtkImageSource);

protected:
  vtkImageSource *Input1;     // One of the inputs to the filter
  vtkImageSource *Input2;     // One of the inputs to the filter
  int UseExecuteMethod;       // Use UpdateRegion or Execute method?

  // Description:
  // These are conveniance functions for writing filters that have their
  // own UpdateRegion methods.  They create the region object as well as 
  // getting the input source to fill it with data.  The extent
  // of the unspecified dimensions default to [0, 0];
  // Used in vtkImageScatterPlot.
  vtkImageRegion *GetInput1Region(int *extent, int dim);    
  vtkImageRegion *GetInput1Region5d(int extent[10])
    {return this->GetInput1Region(extent, 5);};
  vtkImageRegion *GetInput1Region4d(int extent[8])
    {return this->GetInput1Region(extent, 4);};
  vtkImageRegion *GetInput1Region3d(int extent[6])
    {return this->GetInput1Region(extent, 3);};
  vtkImageRegion *GetInput1Region2d(int extent[4])
    {return this->GetInput1Region(extent, 2);};
  vtkImageRegion *GetInput1Region1d(int extent[2])
    {return this->GetInput1Region(extent, 1);};
  vtkImageRegion *GetInput2Region(int *extent, int dim);    
  vtkImageRegion *GetInput2Region5d(int extent[10])
    {return this->GetInput2Region(extent, 5);};
  vtkImageRegion *GetInput2Region4d(int extent[8])
    {return this->GetInput2Region(extent, 4);};
  vtkImageRegion *GetInput2Region3d(int extent[6])
    {return this->GetInput2Region(extent, 3);};
  vtkImageRegion *GetInput2Region2d(int extent[4])
    {return this->GetInput2Region(extent, 2);};
  vtkImageRegion *GetInput2Region1d(int extent[2])
    {return this->GetInput2Region(extent, 1);};
  
  void UpdateRegionTiled(vtkImageRegion *outRegion);
  virtual void ComputeOutputImageInformation(vtkImageRegion *inRegion1,
					     vtkImageRegion *inRegion2,
					     vtkImageRegion *outRegion);
  virtual void ComputeRequiredInputRegionExtent(vtkImageRegion *outRegion,
						vtkImageRegion *inRegion1,
						vtkImageRegion *inRegion2);
  virtual void Execute5d(vtkImageRegion *inRegion1, vtkImageRegion *inRegion2, 
			 vtkImageRegion *outRegion);
  virtual void Execute4d(vtkImageRegion *inRegion1, vtkImageRegion *inRegion2, 
			 vtkImageRegion *outRegion);
  virtual void Execute3d(vtkImageRegion *inRegion1, vtkImageRegion *inRegion2, 
			 vtkImageRegion *outRegion);
  virtual void Execute2d(vtkImageRegion *inRegion1, vtkImageRegion *inRegion2, 
			 vtkImageRegion *outRegion);
  virtual void Execute1d(vtkImageRegion *inRegion1, vtkImageRegion *inRegion2, 
			 vtkImageRegion *outRegion);
};

#endif







