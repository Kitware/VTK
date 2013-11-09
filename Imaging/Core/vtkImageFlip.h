/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageFlip.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkImageFlip - This flips an axis of an image. Right becomes left ...
// .SECTION Description
// vtkImageFlip will reflect the data along the filtered axis.  This filter is
// actually a thin wrapper around vtkImageReslice.

#ifndef __vtkImageFlip_h
#define __vtkImageFlip_h


#include "vtkImagingCoreModule.h" // For export macro
#include "vtkImageReslice.h"

class VTKIMAGINGCORE_EXPORT vtkImageFlip : public vtkImageReslice
{
public:
  static vtkImageFlip *New();

  vtkTypeMacro(vtkImageFlip,vtkImageReslice);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify which axis will be flipped.  This must be an integer
  // between 0 (for x) and 2 (for z). Initial value is 0.
  vtkSetMacro(FilteredAxis, int);
  vtkGetMacro(FilteredAxis, int);

  // Description:
  // By default the image will be flipped about its center, and the
  // Origin, Spacing and Extent of the output will be identical to
  // the input.  However, if you have a coordinate system associated
  // with the image and you want to use the flip to convert +ve values
  // along one axis to -ve values (and vice versa) then you actually
  // want to flip the image about coordinate (0,0,0) instead of about
  // the center of the image.  This method will adjust the Origin of
  // the output such that the flip occurs about (0,0,0).  Note that
  // this method only changes the Origin (and hence the coordinate system)
  // the output data: the actual pixel values are the same whether or not
  // this method is used.  Also note that the Origin in this method name
  // refers to (0,0,0) in the coordinate system associated with the image,
  // it does not refer to the Origin ivar that is associated with a
  // vtkImageData.
  vtkSetMacro(FlipAboutOrigin, int);
  vtkGetMacro(FlipAboutOrigin, int);
  vtkBooleanMacro(FlipAboutOrigin, int);

  // Description:
  // Keep the mis-named Axes variations around for compatibility with old
  // scripts. Axis is singular, not plural...
  void SetFilteredAxes(int axis) { this->SetFilteredAxis(axis); }
  int GetFilteredAxes() { return this->GetFilteredAxis(); }

  // Description:
  // PreserveImageExtentOff wasn't covered by test scripts and its
  // implementation was broken.  It is deprecated now and it has
  // no effect (i.e. the ImageExtent is always preserved).
  vtkSetMacro(PreserveImageExtent, int);
  vtkGetMacro(PreserveImageExtent, int);
  vtkBooleanMacro(PreserveImageExtent, int);

protected:
  vtkImageFlip();
  ~vtkImageFlip() {}

  virtual int RequestInformation(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

  int FilteredAxis;
  int FlipAboutOrigin;
  int PreserveImageExtent;

private:
  vtkImageFlip(const vtkImageFlip&);  // Not implemented.
  void operator=(const vtkImageFlip&);  // Not implemented.
};

#endif
