/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageSlabReslice.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkImageSlabReslice - Thick slab reformat through data.
// .SECTION Description
// This class derives from vtkImageResliceBase. Much like vtkImageReslice, it
// reslices the data. It is multi-threaded. It takes a three dimensional image
// as input and produces a two dimensional thick MPR along some direction.
// <p> The class reslices the thick slab using a blending function. Supported
// blending functions are Minimum Intensity blend through the slab, maximum
// intensity blend and a Mean (average) intensity of values across the slab.
// <p> The user can adjust the thickness of the slab by using the method
// SetSlabThickness. The distance between sample points used for blending
// across the thickness of the slab is controlled by the method
// SetSlabResolution. These two methods determine the number of slices used
// across the slab for blending, which is computed as
// {(2 x (int)(0.5 x SlabThickness/SlabResolution)) + 1}. This value may
// be queried via GetNumBlendSamplePoints() and is always >= 1.
// <p> Much like vtkImageReslice, the reslice axes direction cosines may be
// set via the methods SetResliceAxes or SetResliceAxesDirectionCosines. The
// output spacing is controlled by SetOutputSpacing and the output origin is
// controlled by SetOutputOrigin. The default value to be set on pixels that
// lie outside the volume when reformatting is controlled by
// SetBackgroundColor or SetBackgroundLevel. The SetResliceAxesOrigin()
// method can also be used to provide an (x,y,z) point that the slice will
// pass through.
// <p> The interpolation mode may be set via SetInterpolationMode. Only
// nearest neighbor, trilinear and cubic interpolations are supported. Kaiser
// and Lanczos are not supported by the filter, although the API of
// the superclass vtkImageResliceBase, allows one to set these interpolation
// modes.
// .SECTION see also
// vtkImageReslice


#ifndef __vtkImageSlabReslice_h
#define __vtkImageSlabReslice_h

#include "vtkImageResliceBase.h"

#define VTK_IMAGESLAB_BLEND_MIN  0
#define VTK_IMAGESLAB_BLEND_MAX  1
#define VTK_IMAGESLAB_BLEND_MEAN 2

class VTK_IMAGING_EXPORT vtkImageSlabReslice : public vtkImageResliceBase
{
public:

  static vtkImageSlabReslice *New();
  vtkTypeMacro(vtkImageSlabReslice, vtkImageResliceBase);

  // Description:
  // Printself method.
  virtual void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/Get the blend mode. Default is MIP (ie Max)
  vtkSetMacro( BlendMode, int );
  vtkGetMacro( BlendMode, int );
  void SetBlendModeToMin()  { this->SetBlendMode(VTK_IMAGESLAB_BLEND_MIN ); }
  void SetBlendModeToMax()  { this->SetBlendMode(VTK_IMAGESLAB_BLEND_MAX ); }
  void SetBlendModeToMean() { this->SetBlendMode(VTK_IMAGESLAB_BLEND_MEAN); }

  // Description:
  // Number of sample points used across the slab cross-section. If equal to
  // 1, this ends up being a thin reslice through the data a.k.a.
  // vtkImageReslice
  vtkGetMacro( NumBlendSamplePoints, int );

  // Description:
  // SlabThickness of slab in world coords. SlabThickness must be non-zero and
  // positive.
  vtkSetMacro( SlabThickness, double );
  vtkGetMacro( SlabThickness, double );

  // Description:
  // Spacing between slabs in world units. (Number of Slices, ie samples to
  // blend is computed from SlabThickness and SlabResolution).
  vtkSetMacro( SlabResolution, double );
  vtkGetMacro( SlabResolution, double );

protected:
  vtkImageSlabReslice();
  ~vtkImageSlabReslice();

  // Description:
  // This method is called from ThreadedRequestData implemented in
  // vtkImageResliceBase.
  virtual void InternalThreadedRequestData(vtkInformation *request,
                                 vtkInformationVector **inputVector,
                                 vtkInformationVector *outputVector,
                                 vtkImageData ***inData,
                                 vtkImageData **outData, int ext[6], int id);


  // Description:
  // This method simply calls the superclass method. In addition, it also
  // precomputes the NumBlendSamplePoints based on the SlabThickness and
  // SlabResolution.
  virtual int RequestInformation(vtkInformation *, vtkInformationVector **,
                                 vtkInformationVector *);

  int    BlendMode; // can be MIN, MIP, MAX
  double SlabThickness;
  double SlabResolution;
  int    NumBlendSamplePoints;

private:
  vtkImageSlabReslice(const vtkImageSlabReslice&);  // Not implemented.
  void operator=(const vtkImageSlabReslice&);  // Not implemented.
};

#endif
