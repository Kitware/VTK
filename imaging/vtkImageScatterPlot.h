/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageScatterPlot.h
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
// .NAME vtkImageScatterPlot - Produces a scatter plot from 1 axis.
// .SECTION Description
// vtkImageScatterPlot Was written to test the T2Median filter.
// It converts one axis into a space, all other axis are ignored.
// For example, it will convert an image with 2 spectral channels (components)
// into a 2d scatter plot. All pixels become dots in the plot.  The
// output of this filter is an image of unsigned bytes whose values
// are 0 or 255. InRegion specifies the region to use from the input
// that will create the plot.  OutRegion Specifies the dimensions of the
// scatter plot.  AspectRatio specifies how the components are converted
// into the OutRegion.  This filter will only work on 4d data 
// (3d + components).


#ifndef __vtkImageScatterPlot_h
#define __vtkImageScatterPlot_h


#include "vtkImageFilter.h"

class vtkImageScatterPlot : public vtkImageFilter
{
public:
  vtkImageScatterPlot();
  char *GetClassName() {return "vtkImageScatterPlot";};
  
  // Description:
  // You can modify the extent of InRegion and OutRegions, 
  // but you nust get them first.
  vtkImageRegion *GetInRegion(){return &(this->InRegion);};
  vtkImageRegion *GetImageRegion(){return &(this->ImageRegion);};
  
  void SetInput(vtkImageSource *input);
  void SetAxes(int dim, int *axes);
  
  // Description:
  // Set/Get The aspect ratio (same for all axes)
  vtkSetMacro(AspectRatio,float);
  vtkGetMacro(AspectRatio,float);
  
protected:
  float AspectRatio;
  vtkImageRegion InRegion;   // filter is performed over this region.
  vtkImageRegion ImageRegion;  // Just a way to provide ImageExtent.
  
  void ComputeOutputImageInformation(vtkImageRegion *inRegion,
				     vtkImageRegion *outRegion);
  void ComputeRequiredInputRegionExtent(vtkImageRegion *outRegion, 
					vtkImageRegion *inRegion);

  void UpdateRegion(vtkImageRegion *outRegion);
};

#endif



