/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageReslice.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to David G. Gobbi who developed this class.

Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
// .NAME vtkImageReslice - Reslices a volume along a new set of axes.
// .SECTION Description
// vtkImageReslice is the swiss-army-knife of image geometry filters:  
// It can permute, rotate, flip, scale, resample, deform, and pad image
// data in any combination with reasonably high efficiency.  Simple
// operations such as permutation, resampling and padding are done
// with similar efficiently to the specialized vtkImagePermute,
// vtkImageResample, and vtkImagePad filters.  There are a number of
// tasks that vtkImageReslice is well suited for:
// <p>1) Application of simple rotations, scales, and translations to
// an image. It is often a good idea to use vtkImageChangeInformation
// to center the image first, so that scales and rotations occur around
// the center rather than around the lower-left corner of the image.
// <p>2) Resampling of one data set to match the voxel sampling of 
// a second data set via the SetInformationInput() method, e.g. for
// the purpose of comparing two images or combining two images.
// A transformation, either linear or nonlinear, can be applied 
// at the same time via the SetResliceTransform method if the two
// images are not in the same coordinate space.
// <p>3) Extraction of slices from an image volume.  The most convenient
// way to do this is to use SetResliceAxesDirectionCosines() to
// specify the orientation of the slice.  The direction cosines give
// the x, y, and z axes for the output volume.  The method 
// SetOutputDimensionality(2) is used to specify that want to output a
// slice rather than a volume.  The SetResliceAxesOrigin() command is
// used to provide an (x,y,z) point that the slice will pass through.
// You can use both the ResliceAxes and the ResliceTransform at the
// same time, in order to extract slices from a volume that you have
// applied a transformation to.
// .SECTION Caveats
// This filter is very inefficient if the output X dimension is 1.
// .SECTION see also
// vtkAbstractTransform vtkMatrix4x4


#ifndef __vtkImageReslice_h
#define __vtkImageReslice_h


#include "vtkImageToImageFilter.h"
#include "vtkAbstractTransform.h"
#include "vtkMatrix4x4.h"

// interpolation mode constants
#define VTK_RESLICE_NEAREST 0
#define VTK_RESLICE_LINEAR 1
#define VTK_RESLICE_CUBIC 3

class VTK_EXPORT vtkImageReslice : public vtkImageToImageFilter
{
public:
  static vtkImageReslice *New();
  vtkTypeMacro(vtkImageReslice, vtkImageToImageFilter);

  virtual void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // This method is used to set up the axes for the output voxels.
  // The output Spacing, Origin, and Extent specify the locations
  // of the voxels within the coordinate system defined by the axes.
  // The ResliceAxes are used most often to permute the data, e.g.
  // to extract ZY or XZ slices of a volume as 2D XY images.
  // <p>The first column of the matrix specifies the x-axis 
  // vector (the fourth element must be set to zero), the second
  // column specifies the y-axis, and the third column the
  // z-axis.  The fourth column is the origin of the
  // axes (the fourth element must be set to one).  
  // <p>An alternative to SetResliceAxes() is to use 
  // SetResliceAxesDirectionCosines() to set the directions of the
  // axes and SetResliceAxesOrigin() to set the origin of the axes.
  vtkSetObjectMacro(ResliceAxes, vtkMatrix4x4);
  vtkGetObjectMacro(ResliceAxes, vtkMatrix4x4);

  // Description:
  // Specify the direction cosines for the ResliceAxes (i.e. the
  // first three elements of each of the first three columns of 
  // the ResliceAxes matrix).  This will modify the current
  // ResliceAxes matrix, or create a new matrix if none exists.
  void SetResliceAxesDirectionCosines(double x0, double x1, double x2,
				      double y0, double y1, double y2,
				      double z0, double z1, double z2);
  void SetResliceAxesDirectionCosines(const double x[3],
				      const double y[3],
				      const double z[3]) {
    this->SetResliceAxesDirectionCosines(x[0], x[1], x[2],
					 y[0], y[1], y[2],
					 z[0], z[1], z[2]); };
  void SetResliceAxesDirectionCosines(const double xyz[9]) {
    this->SetResliceAxesDirectionCosines(xyz[0], xyz[1], xyz[2],
					 xyz[3], xyz[4], xyz[5],
					 xyz[6], xyz[7], xyz[8]); };
  void GetResliceAxesDirectionCosines(double x[3], double y[3], double z[3]);
  void GetResliceAxesDirectionCosines(double xyz[9]) {
    this->GetResliceAxesDirectionCosines(&xyz[0], &xyz[3], &xyz[6]); };
  double *GetResliceAxesDirectionCosines() {
    this->GetResliceAxesDirectionCosines(this->ResliceAxesDirectionCosines);
    return this->ResliceAxesDirectionCosines; };

  // Description:
  // Specify the origin for the ResliceAxes (i.e. the first three
  // elements of the final column of the ResliceAxes matrix).
  // This will modify the current ResliceAxes matrix, or create
  // new matrix if none exists.
  void SetResliceAxesOrigin(double x, double y, double z);
  void SetResliceAxesOrigin(const double xyz[3]) {
    this->SetResliceAxesOrigin(xyz[0], xyz[1], xyz[2]); };
  void GetResliceAxesOrigin(double xyz[3]);
  double *GetResliceAxesOrigin() {
    this->GetResliceAxesOrigin(this->ResliceAxesOrigin);
    return this->ResliceAxesOrigin; };

  // Description:
  // Set a transform to be applied to the resampling grid that has
  // been defined via the ResliceAxes and the output Origin, Spacing
  // and Extent.  Note that applying a transform to the resampling
  // grid (which lies in the output coordinate system) is
  // equivalent to applying the inverse of that transform to
  // the input volume.  Nonlinear transforms such as vtkGridTransform
  // and vtkThinPlateSplineTransform can be used here.
  vtkSetObjectMacro(ResliceTransform, vtkAbstractTransform);
  vtkGetObjectMacro(ResliceTransform, vtkAbstractTransform);

  // Description:
  // Set a vtkImageData from which the default Spacing, Origin,
  // and WholeExtent of the output will be copied.  The spacing,
  // origin, and extent will be permuted according to the 
  // ResliceAxes.  Any values set via SetOutputSpacing, 
  // SetOutputOrigin, and SetOutputExtent will override these
  // values.  By default, the Spacing, Origin, and WholeExtent
  // of the Input are used.
  vtkSetObjectMacro(InformationInput, vtkImageData);
  vtkGetObjectMacro(InformationInput, vtkImageData);

  // Description:
  // Specify whether to transform the spacing, origin and extent
  // of the Input (or the InformationInput) according to the
  // direction cosines and origin of the ResliceAxes before applying
  // them as the default output spacing, origin and extent. 
  // Default: On.
  vtkSetMacro(TransformInputSampling, int);
  vtkBooleanMacro(TransformInputSampling, int);
  vtkGetMacro(TransformInputSampling, int);

  // Description:
  // Turn this on if you want to guarantee that the extent of the
  // output will be large enough to ensure that none of the 
  // data will be cropped.
  vtkSetMacro(AutoCropOutput, int);
  vtkBooleanMacro(AutoCropOutput, int);
  vtkGetMacro(AutoCropOutput, int);

  // Description:
  // Turn on wrap-pad feature (default: off). 
  vtkSetMacro(Wrap, int);
  vtkGetMacro(Wrap, int);
  vtkBooleanMacro(Wrap, int);

  // Description:
  // Turn on mirror-pad feature (default: off). 
  // This will override the wrap-pad.
  vtkSetMacro(Mirror, int);
  vtkGetMacro(Mirror, int);
  vtkBooleanMacro(Mirror, int);

  // Description:
  // Set interpolation mode (default: nearest neighbor). 
  vtkSetMacro(InterpolationMode, int);
  vtkGetMacro(InterpolationMode, int);
  void SetInterpolationModeToNearestNeighbor() {
    this->SetInterpolationMode(VTK_RESLICE_NEAREST); };
  void SetInterpolationModeToLinear() {
    this->SetInterpolationMode(VTK_RESLICE_LINEAR); };
  void SetInterpolationModeToCubic() {
    this->SetInterpolationMode(VTK_RESLICE_CUBIC); };
  const char *GetInterpolationModeAsString();

  // Description:
  // Turn on and off optimizations (default on, they should only be
  // turned off for testing purposes). 
  vtkSetMacro(Optimization, int);
  vtkGetMacro(Optimization, int);
  vtkBooleanMacro(Optimization, int);

  // Description:
  // Set the background color (for multi-component images).
  vtkSetVector4Macro(BackgroundColor, float);
  vtkGetVector4Macro(BackgroundColor, float);

  // Description:
  // Set background grey level (for single-component images).
  void SetBackgroundLevel(float v) { this->SetBackgroundColor(v,v,v,v); };
  float GetBackgroundLevel() { return this->GetBackgroundColor()[0]; };

  // Description:
  // Set the voxel spacing for the output data.  The default output
  // spacing is the input spacing permuted through the ResliceAxes.
  vtkSetVector3Macro(OutputSpacing, float);
  vtkGetVector3Macro(OutputSpacing, float);
  void SetOutputSpacingToDefault() {
    this->SetOutputSpacing(VTK_FLOAT_MAX, VTK_FLOAT_MAX, VTK_FLOAT_MAX); };

  // Description:
  // Set the origin for the output data.  The default output origin
  // is the input origin permuted through the ResliceAxes.
  vtkSetVector3Macro(OutputOrigin, float);
  vtkGetVector3Macro(OutputOrigin, float);
  void SetOutputOriginToDefault() {
    this->SetOutputOrigin(VTK_FLOAT_MAX, VTK_FLOAT_MAX, VTK_FLOAT_MAX); };

  // Description:
  // Set the extent for the output data.  The default output extent
  // is the input extent permuted through the ResliceAxes.
  vtkSetVector6Macro(OutputExtent, int);
  vtkGetVector6Macro(OutputExtent, int);
  void SetOutputExtentToDefault() {
    this->SetOutputExtent(VTK_INT_MIN, VTK_INT_MAX,
			  VTK_INT_MIN, VTK_INT_MAX,
			  VTK_INT_MIN, VTK_INT_MAX); };

  // Description:
  // Force the dimensionality of the output to either 1, 2,
  // 3 or 0 (default: 3).  If the dimensionality is 2D, then
  // the Z extent of the output is forced to (0,0) and the Z
  // origin of the output is forced to 0.0 (i.e. the output
  // extent is confined to the xy plane).  If the dimensionality
  // is 1D, the output extent is confined to the x axis.  
  // For 0D, the output extent consists of a single voxel at 
  // (0,0,0).  
  vtkSetMacro(OutputDimensionality, int);
  vtkGetMacro(OutputDimensionality, int);

  // Description:
  // When determining the modified time of the filter, 
  // this check the modified time of the transform and matrix.
  unsigned long int GetMTime();

  // Description:
  // Convenient methods for switching between nearest-neighbor and linear
  // interpolation.  
  // InterpolateOn() is equivalent to SetInterpolationModeToLinear() and
  // InterpolateOff() is equivalent to SetInterpolationModeToNearestNeighbor().
  // You should not use these methods if you use the SetInterpolationMode
  // methods.
  void SetInterpolate(int t) {
    if (t && !this->GetInterpolate()) {
      this->SetInterpolationModeToLinear(); }
    else if (!t && this->GetInterpolate()) {
      this->SetInterpolationModeToNearestNeighbor(); } };
  void InterpolateOn() {
    this->SetInterpolate(1); };
  void InterpolateOff() {
    this->SetInterpolate(0); };
  int GetInterpolate() {
    return (this->GetInterpolationMode() != VTK_RESLICE_NEAREST); };

//BTX
  // Description:
  // Helper functions not meant to be used outside this class. 
  vtkMatrix4x4 *GetIndexMatrix();
  int FindExtent(int& r1, int& r2, float *point, float *xAxis,
		 int *inMin, int *inMax, int *outExt);
//ETX

protected:
  vtkImageReslice();
  ~vtkImageReslice();
  vtkImageReslice(const vtkImageReslice&) {};
  void operator=(const vtkImageReslice&) {};

  vtkMatrix4x4 *ResliceAxes;
  double ResliceAxesDirectionCosines[9];
  double ResliceAxesOrigin[3];
  vtkAbstractTransform *ResliceTransform;
  vtkImageData *InformationInput;
  vtkMatrix4x4 *IndexMatrix;
  int Wrap;
  int Mirror;
  int InterpolationMode;
  int Optimization;
  float BackgroundColor[4];
  float OutputOrigin[3];
  float OutputSpacing[3];
  int OutputExtent[6];
  int OutputDimensionality;
  int TransformInputSampling;
  int AutoCropOutput;

  void GetAutoCroppedOutputBounds(vtkImageData *input, float bounds[6]);
  void ExecuteInformation(vtkImageData *input, vtkImageData *output);
  void ExecuteInformation() {
    this->vtkImageToImageFilter::ExecuteInformation(); };
  void ComputeInputUpdateExtent(int inExt[6], int outExt[6]);
  void ThreadedExecute(vtkImageData *inData, vtkImageData *outData, 
		       int ext[6], int id);
  void OptimizedComputeInputUpdateExtent(int inExt[6], int outExt[6]);
  void OptimizedThreadedExecute(vtkImageData *inData, vtkImageData *outData, 
				int ext[6], int id);  
};

//----------------------------------------------------------------------------
inline const char *vtkImageReslice::GetInterpolationModeAsString()
{
  switch (this->InterpolationMode)
    {
    case VTK_RESLICE_NEAREST:
      return "NearestNeighbor";
    case VTK_RESLICE_LINEAR:
      return "Linear";
    case VTK_RESLICE_CUBIC:
      return "Cubic";
    default:
      return "";
    }
}  

#endif





