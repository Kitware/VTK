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
// vtkImageSpatialFilter is meant to replace (or be a super class) of the
// 1d, 2d and 3d spatial filters.  It was written for the 2d Gradient
// filters.  It has an ivar describing the neighborhood, but does not supp;y
// methods to allow the user to modify the neighborhood.  The main
// functionalit of this class is to break the images into central
// and boundary pieces.  Different execute methods are called for these
// two classes of regions.


#ifndef __vtkImageSpatialFilter_h
#define __vtkImageSpatialFilter_h


#include "vtkImageFilter.h"
#include "vtkImageRegion.h"

class vtkImageSpatialFilter : public vtkImageFilter
{
public:
  vtkImageSpatialFilter();
  char *GetClassName() {return "vtkImageSpatialFilter";};
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Get the Spatial kernel size and middle.
  vtkGetVector3Macro(KernelSize,int);
  vtkGetVector3Macro(KernelMiddle,int);
  // Description:
  // Set/Get whether use boundary execute method or not (shrink image).
  vtkSetMacro(HandleBoundaries,int);
  vtkGetMacro(HandleBoundaries,int);
  vtkBooleanMacro(HandleBoundaries,int);
  
  // Description:
  // Set/Get whether a special method exists for non boundary condition.
  vtkSetMacro(UseExecuteCenter,int);
  vtkGetMacro(UseExecuteCenter,int);
  vtkBooleanMacro(UseExecuteCenter,int);
  
  
protected:
  int   KernelSize[4];
  int   KernelMiddle[4];      // Index of kernel origin
  int   HandleBoundaries;     // Shrink kernel at boundaries?
  int   UseExecuteCenter;     // Will the subclass have special execute method.

  void ComputeOutputImageInformation(vtkImageRegion *inRegion,
				     vtkImageRegion *outRegion);
  void ComputeRequiredInputRegionExtent(vtkImageRegion *outRegion, 
					vtkImageRegion *inRegion);
  
  virtual void Execute4d(vtkImageRegion *inRegion, vtkImageRegion *outRegion);
  virtual void ExecuteCenter4d(vtkImageRegion *inRegion, 
			       vtkImageRegion *outRegion);
  virtual void ExecuteCenter3d(vtkImageRegion *inRegion, 
			       vtkImageRegion *outRegion);
  virtual void ExecuteCenter2d(vtkImageRegion *inRegion, 
			       vtkImageRegion *outRegion);
  virtual void ExecuteCenter1d(vtkImageRegion *inRegion, 
			       vtkImageRegion *outRegion);
  
};

#endif










