/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageReslice.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkImageResliceBase - Base class for reslicing a volume along a new set of axes.
// .SECTION Description
// Base class for vtkImageReslice. Concrete implementations are
// vtkImageReslice and vtkImageSlabReslice. This class simply provides
// the interface. Actual implementations of reslicing are done in the
// concrete subclasses.
// .SECTION Thanks
// A significant portion of this code originates from vtkImageReslice, which
// was developed by David Gobbi.

#ifndef __vtkImageResliceBase_h
#define __vtkImageResliceBase_h

#include "vtkThreadedImageAlgorithm.h"

// interpolation mode constants
#define VTK_RESLICE_NEAREST 0
#define VTK_RESLICE_LINEAR 1
#define VTK_RESLICE_RESERVED_2 2
#define VTK_RESLICE_CUBIC 3
#define VTK_RESLICE_LANCZOS 4
#define VTK_RESLICE_KAISER 5

class vtkImageData;
class vtkAbstractTransform;
class vtkMatrix4x4;
class vtkImageStencilData;
class vtkScalarsToColors;

class VTK_IMAGING_EXPORT vtkImageResliceBase : public vtkThreadedImageAlgorithm
{
public:
  static vtkImageResliceBase *New();
  vtkTypeMacro(vtkImageResliceBase, vtkThreadedImageAlgorithm);

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
  virtual void SetResliceAxes(vtkMatrix4x4*);
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
  virtual void SetResliceTransform(vtkAbstractTransform*);
  vtkGetObjectMacro(ResliceTransform, vtkAbstractTransform);

  // Description:
  // Specify whether to transform the spacing, origin and extent
  // of the Input (or the InformationInput) according to the
  // direction cosines and origin of the ResliceAxes before applying
  // them as the default output spacing, origin and extent
  // (default: On).
  vtkSetMacro(TransformInputSampling, int);
  vtkBooleanMacro(TransformInputSampling, int);
  vtkGetMacro(TransformInputSampling, int);

  // Description:
  // Turn this on if you want to guarantee that the extent of the
  // output will be large enough to ensure that none of the
  // data will be cropped (default: Off).
  vtkSetMacro(AutoCropOutput, int);
  vtkBooleanMacro(AutoCropOutput, int);
  vtkGetMacro(AutoCropOutput, int);

  // Description:
  // Turn on wrap-pad feature (default: Off).
  vtkSetMacro(Wrap, int);
  vtkGetMacro(Wrap, int);
  vtkBooleanMacro(Wrap, int);

  // Description:
  // Turn on mirror-pad feature (default: Off).
  // This will override the wrap-pad.
  vtkSetMacro(Mirror, int);
  vtkGetMacro(Mirror, int);
  vtkBooleanMacro(Mirror, int);

  // Description:
  // Extend the apparent input border by a half voxel (default: On).
  // This changes how interpolation is handled at the borders of the
  // input image: if the center of an output voxel is beyond the edge
  // of the input image, but is within a half voxel width of the edge
  // (using the input voxel width), then the value of the output voxel
  // is calculated as if the input's edge voxels were duplicated past
  // the edges of the input.
  // This has no effect if Mirror or Wrap are on.
  vtkSetMacro(Border, int);
  vtkGetMacro(Border, int);
  vtkBooleanMacro(Border, int);

  // Description:
  // Set interpolation mode (default: nearest neighbor).  Also
  // see SetInterpolationSizeParameter, which is valid for the
  // Lanczos and Kaiser windowed sinc interpolation interpolation
  // methods.
  vtkSetClampMacro(InterpolationMode, int,
                   VTK_RESLICE_NEAREST, VTK_RESLICE_KAISER);
  vtkGetMacro(InterpolationMode, int);
  void SetInterpolationModeToNearestNeighbor() {
    this->SetInterpolationMode(VTK_RESLICE_NEAREST); };
  void SetInterpolationModeToLinear() {
    this->SetInterpolationMode(VTK_RESLICE_LINEAR); };
  void SetInterpolationModeToCubic() {
    this->SetInterpolationMode(VTK_RESLICE_CUBIC); };
  void SetInterpolationModeToLanczos() {
    this->SetInterpolationMode(VTK_RESLICE_LANCZOS); };
  void SetInterpolationModeToKaiser() {
    this->SetInterpolationMode(VTK_RESLICE_KAISER); };
  virtual const char *GetInterpolationModeAsString();

  // Description:
  // Set the size parameter for any interpolation kernel
  // that takes such a parameter.  For windowed sinc methods
  // such as Lanczos and Kaiser, this is the half-width of
  // the kernel.  This parameter must be an integer between
  // 1 and 7, and it has a default value of 3.  Note that
  // the alpha parameter for Kaiser is automatically forced
  // to three times this value, and cannot be modified
  // independently.
  vtkSetClampMacro(InterpolationSizeParameter, int, 1, 7);
  vtkGetMacro(InterpolationSizeParameter, int);

  // Description:
  // Set the background color (for multi-component images).
  vtkSetVector4Macro(BackgroundColor, double);
  vtkGetVector4Macro(BackgroundColor, double);

  // Description:
  // Set background grey level (for single-component images).
  void SetBackgroundLevel(double v) { this->SetBackgroundColor(v,v,v,v); };
  double GetBackgroundLevel() { return this->GetBackgroundColor()[0]; };

  // Description:
  // Set the voxel spacing for the output data.  The default output
  // spacing is the input spacing permuted through the ResliceAxes.
  virtual void SetOutputSpacing(double x, double y, double z);
  virtual void SetOutputSpacing(const double a[3]) {
    this->SetOutputSpacing(a[0], a[1], a[2]); };
  vtkGetVector3Macro(OutputSpacing, double);
  void SetOutputSpacingToDefault();

  // Description:
  // Set the origin for the output data.  The default output origin
  // is the input origin permuted through the ResliceAxes.
  virtual void SetOutputOrigin(double x, double y, double z);
  virtual void SetOutputOrigin(const double a[3]) {
    this->SetOutputOrigin(a[0], a[1], a[2]); };
  vtkGetVector3Macro(OutputOrigin, double);
  void SetOutputOriginToDefault();

  // Description:
  // Set the extent for the output data.  The default output extent
  // is the input extent permuted through the ResliceAxes.
  virtual void SetOutputExtent(int a, int b, int c, int d, int e, int f);
  virtual void SetOutputExtent(const int a[6]) {
    this->SetOutputExtent(a[0], a[1], a[2], a[3], a[4], a[5]); };
  vtkGetVector6Macro(OutputExtent, int);
  void SetOutputExtentToDefault();

  // Description:
  // When determining the modified time of the filter,
  // this check the modified time of the transform and matrix.
  unsigned long int GetMTime();

  // Description:
  // Convenient methods for switching between nearest-neighbor and linear
  // interpolation.
  // InterpolateOn() is equivalent to SetInterpolationModeToLinear() and
  // InterpolateOff() is equivalent to SetInterpolationModeToNearestNeighbor()
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

protected:
  vtkImageResliceBase();
  ~vtkImageResliceBase();

  vtkMatrix4x4 *ResliceAxes;
  double ResliceAxesDirectionCosines[9];
  double ResliceAxesOrigin[3];
  vtkAbstractTransform *ResliceTransform;
  int Wrap;
  int Mirror;
  int Border;
  int InterpolationMode;
  int InterpolationSizeParameter;
  double BackgroundColor[4];
  double OutputOrigin[3];
  double OutputSpacing[3];
  int OutputExtent[6];
  int TransformInputSampling;
  int AutoCropOutput;
  int HitInputExtent;
  int ComputeOutputSpacing;
  int ComputeOutputOrigin;
  int ComputeOutputExtent;
  int OutputDimensionality;

  vtkMatrix4x4 *IndexMatrix;
  vtkAbstractTransform *OptimizedTransform;

  // Description:
  // This should be set to 1 by derived classes that override the
  // ConvertScalars method.
  int HasConvertScalars;

  // Description:
  // This should be overridden by derived classes that operate on
  // the interpolated data before it is placed in the output.
  virtual int ConvertScalarInfo(int &scalarType, int &numComponents);

  // Description:
  // This should be overridden by derived classes that operate on
  // the interpolated data before it is placed in the output.
  // The input data will usually be double or float (since the
  // interpolation routines use floating-point) but it could be
  // of any type.  This method will be called from multiple threads,
  // so it must be thread-safe in derived classes.
  virtual void ConvertScalars(void *inPtr, void *outPtr,
                              int inputType, int inputNumComponents,
                              int count, int idX, int idY, int idZ,
                              int threadId);

  void ConvertScalarsBase(void *inPtr, void *outPtr,
                          int inputType, int inputNumComponents,
                          int count, int idX, int idY, int idZ, int threadId) {
    this->ConvertScalars(inPtr, outPtr, inputType, inputNumComponents,
                         count, idX, idY, idZ, threadId); }

  void GetAutoCroppedOutputBounds(vtkInformation *inInfo, double bounds[6]);
  virtual int RequestInformation(vtkInformation *, vtkInformationVector **,
                                 vtkInformationVector *);
  virtual int InternalRequestInformation(vtkInformation *,
                                 vtkInformationVector **,
                                 vtkInformationVector *,
                                 int inWholeExt[6], double inSpacing[3],
                                 double inOrigin[3]);
  virtual int RequestUpdateExtent(vtkInformation *, vtkInformationVector **,
                                  vtkInformationVector *);

  vtkMatrix4x4 *GetIndexMatrix(vtkInformation *inInfo,
                               vtkInformation *outInfo);

  vtkAbstractTransform *GetOptimizedTransform()
    {
    return this->OptimizedTransform;
    }

  // This must be overridden in the subclass regardless. This is because the
  // table itself is a static float array of the form static float [] in
  // vtkImageResliceDetail. Hence the value must be populated in every
  // translation unit.
  virtual void BuildInterpolationTables();

  virtual void ThreadedRequestData(vtkInformation *request,
                                   vtkInformationVector **inputVector,
                                   vtkInformationVector *outputVector,
                                   vtkImageData ***inData,
                                   vtkImageData **outData, int ext[6], int id);

  // Description:
  // Subclasses override this method to do the real work.
  virtual void InternalThreadedRequestData(vtkInformation *request,
                                   vtkInformationVector **inputVector,
                                   vtkInformationVector *outputVector,
                                   vtkImageData ***inData,
                                   vtkImageData **outData,
                                   int ext[6], int id);


private:
  vtkImageResliceBase(const vtkImageResliceBase&);  // Not implemented.
  void operator=(const vtkImageResliceBase&);  // Not implemented.
};

#endif
