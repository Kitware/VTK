/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageInPlaceFilter.h
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
// .NAME vtkImageInPlaceFilter - Filter that operates in place.
// .SECTION Description
// vtkImageInPlaceFilter is a filter super class that 
// operates directly on the input region.  The data is copied
// if the requested region has different extent than the input region
// or some other object is referencing the input region.



#ifndef __vtkImageInPlaceFilter_h
#define __vtkImageInPlaceFilter_h


#include "vtkImageCachedSource.h"

class VTK_EXPORT vtkImageInPlaceFilter : public vtkImageCachedSource
{
public:
  vtkImageInPlaceFilter();
  char *GetClassName() {return "vtkImageInPlaceFilter";};
  void PrintSelf(ostream& os, vtkIndent indent);

  virtual void SetInput(vtkImageSource *input);
  void SetInput(vtkStructuredPoints *spts)
    {this->SetInput(spts->GetStructuredPointsToImage()->GetOutput());}

  void UpdatePointData(int dim, vtkImageRegion *outRegion);
  void UpdateImageInformation(vtkImageRegion *region);
  unsigned long int GetPipelineMTime();
  
  // Description:
  // Get input to this InPlaceFilter.
  vtkGetObjectMacro(Input,vtkImageSource);
  
  // Description:
  // The basic operation is on regions with this dimensionality.  
  // Filters that operate on pixels would have dimensionality 0.
  // 2D Image filters would have dimensionality 2.
  vtkGetMacro(Dimensionality, int);
    
protected:
  vtkImageSource *Input;     

  int Dimensionality;
  // This is set in the subclass constructor and (probably) should not be 
  // changed.  It indicates the dimensionality of the regions the
  // execute/update methods expect.  It may be larger than dimensionality
  // to make the filter faster (this supper class is not efficient at
  // looping over extra dimensions.
  int ExecuteDimensionality;
  
  // Description:
  // These are conveniance functions for writing InPlaceFilters that have their
  // own UpdateRegion methods.  They create the region object as well as 
  // getting the input source to fill it with data.  The extent
  // of the unspecified dimensions default to [0, 0];
  // Used in vtkImageScatterPlot.
  vtkImageRegion *GetInputRegion(int dim, int *extent);    
  
  virtual void ComputeOutputImageInformation(vtkImageRegion *inRegion,
					     vtkImageRegion *outRegion);
  virtual void ComputeRequiredInputRegionExtent(vtkImageRegion *outRegion,
						vtkImageRegion *inRegion);
  virtual void Execute(int dim, vtkImageRegion *inRegion, 
		       vtkImageRegion *outRegion);
  virtual void Execute(vtkImageRegion *inRegion, vtkImageRegion *outRegion);
};

#endif







