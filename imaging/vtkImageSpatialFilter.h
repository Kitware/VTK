/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageSpatialFilter.h
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
// .NAME vtkImageSpatialFilter - Filters that operate on pixel neighborhoods.
// .SECTION Description
// vtkImageSpatialFilter is a super class for filters that operate on
// an input neighborhood for each output pixel.  It is setup to for
// any dimensionality.  It handels even sized neighborhoods, but their can be
// a half pixel shift associated with processing.  This superclass has some
// logic for handling boundaries.  It can split regions into boundary and 
// non-boundary pieces and call different execute methods.


#ifndef __vtkImageSpatialFilter_h
#define __vtkImageSpatialFilter_h


#include "vtkImageFilter.h"
#include "vtkImageRegion.h"

// Different execute behavior
#define VTK_IMAGE_SPATIAL_CENTER 0
#define VTK_IMAGE_SPATIAL_PIXEL 1
#define VTK_IMAGE_SPATIAL_SUBCLASS 1

// A macro to get the name of a type
#define vtkImageSpatialExecuteTypeMacro(type) \
(((type) == VTK_IMAGE_SPATIAL_SUBCLASS) ? "subclass" : \
(((type) == VTK_IMAGE_SPATIAL_CENTER) ? "center" : \
(((type) == VTK_IMAGE_SPATIAL_PIXEL) ? "pixel" : \
"Undefined")))

class VTK_EXPORT vtkImageSpatialFilter : public vtkImageFilter
{
public:
  vtkImageSpatialFilter();
  static vtkImageSpatialFilter *New() {return new vtkImageSpatialFilter;};
  const char *GetClassName() {return "vtkImageSpatialFilter";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // users shouldn't access these directly but templated functions need to
  int   KernelSize[4];
  int   KernelMiddle[4];      // Index of kernel origin
  int   Strides[4];      // Shrink factor
  int   HandleBoundaries;     // Output shrinks if boundaries aren't handled
  int   ExecuteType;   // Subclasses can use special execute methods.

protected:
  // Description:
  // There are three types of execute methods that can be used by the subclass.
  // "Subclass" is the usual execute method for non spatial filters,
  // "Center" breaks the regions into two types: those that need boundary
  // handling, and those that do not.  Different execute methods are called
  // for the two types of regions. "Pixel" is an execute method called
  // for each pixel of the output.
  vtkSetMacro(ExecuteType, int);
  vtkGetMacro(ExecuteType, int);
  void SetExecuteTypeToSubclass()
    {this->SetExecuteType(VTK_IMAGE_SPATIAL_SUBCLASS);}
  void SetExecuteTypeToCenter()
    {this->SetExecuteType(VTK_IMAGE_SPATIAL_CENTER);}
  void SetExecuteTypeToPixel()
    {this->SetExecuteType(VTK_IMAGE_SPATIAL_PIXEL);}
  
  void ExecuteImageInformation(vtkImageCache *in, vtkImageCache *out);
  void ComputeOutputWholeExtent(int *extent, int handleBoundaries);
  void ComputeRequiredInputUpdateExtent(vtkImageCache *out, vtkImageCache *in);
  void ComputeRequiredInputRegionExtent(vtkImageRegion *out, 
					vtkImageRegion *in);
  void ComputeRequiredInputExtent(int *extent, int *wholeExtent);
  
  void RecursiveLoopExecute(int dim, vtkImageRegion *inRegion, 
			    vtkImageRegion *outRegion);
  // For breaking up into center and boundary ...
  void ExecuteCenter(int dim, vtkImageRegion *inRegion,
		     vtkImageRegion *outRegion);  
  virtual void ExecuteCenter(vtkImageRegion *inRegion, 
			     vtkImageRegion *outRegion);  
  // For processing pixel by pixel.
  void ExecutePixel(int dim, vtkImageRegion *inRegion,
		    vtkImageRegion *outRegion);  
  virtual void ExecutePixel(vtkImageRegion *inRegion, 
			    vtkImageRegion *outRegion);  
  

};

#endif










