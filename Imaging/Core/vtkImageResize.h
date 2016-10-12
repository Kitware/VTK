/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageResize.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkImageResize
 * @brief   High-quality image resizing filter
 *
 * vtkImageResize will magnify or shrink an image with interpolation and
 * antialiasing.  The resizing is done with a 5-lobe Lanczos-windowed sinc
 * filter that is bandlimited to the output sampling frequency in order to
 * avoid aliasing when the image size is reduced.  This filter utilizes a
 * O(n) algorithm to provide good effiency even though the filtering kernel
 * is large.  The sinc interpolator can be turned off if nearest-neighbor
 * interpolation is required, or it can be replaced with a different
 * vtkImageInterpolator object.
 * @par Thanks:
 * Thanks to David Gobbi for contributing this class to VTK.
*/

#ifndef vtkImageResize_h
#define vtkImageResize_h

#include "vtkImagingCoreModule.h" // For export macro
#include "vtkThreadedImageAlgorithm.h"

class vtkAbstractImageInterpolator;

class VTKIMAGINGCORE_EXPORT vtkImageResize : public vtkThreadedImageAlgorithm
{
public:
  static vtkImageResize *New();
  vtkTypeMacro(vtkImageResize, vtkThreadedImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  enum
  {
    OUTPUT_DIMENSIONS,
    OUTPUT_SPACING,
    MAGNIFICATION_FACTORS
  };

  //@{
  /**
   * The resizing method to use.  The default is to set the output image
   * dimensions, and allow the filter to resize the image to these new
   * dimensions.  It is also possible to resize the image by setting the
   * output image spacing or by setting a magnification factor.
   */
  vtkSetClampMacro(ResizeMethod, int, OUTPUT_DIMENSIONS, MAGNIFICATION_FACTORS);
  vtkGetMacro(ResizeMethod, int);
  void SetResizeMethodToOutputDimensions() {
    this->SetResizeMethod(OUTPUT_DIMENSIONS); }
  void SetResizeMethodToOutputSpacing() {
    this->SetResizeMethod(OUTPUT_SPACING); }
  void SetResizeMethodToMagnificationFactors() {
    this->SetResizeMethod(MAGNIFICATION_FACTORS); }
  virtual const char *GetResizeMethodAsString();
  //@}

  //@{
  /**
   * The desired output dimensions.  This is only used if the ResizeMethod is
   * set to OutputDimensions.  If you want to keep one of the image dimensions
   * the same as the input, then set that dimension to -1.
   */
  vtkSetVector3Macro(OutputDimensions, int);
  vtkGetVector3Macro(OutputDimensions, int);
  //@}

  //@{
  /**
   * The desired output spacing.  This is only used if the ResizeMethod is
   * set to OutputSpacing.  If you want to keep one of the original spacing
   * values, then set that spacing value to zero.
   */
  vtkSetVector3Macro(OutputSpacing, double);
  vtkGetVector3Macro(OutputSpacing, double);
  //@}

  //@{
  /**
   * The desired magnification factor, meaning that the sample spacing will
   * be reduced by this factor.  This setting is only used if the ResizeMethod
   * is set to MagnificationFactors.
   */
  vtkSetVector3Macro(MagnificationFactors, double);
  vtkGetVector3Macro(MagnificationFactors, double);
  //@}

  //@{
  /**
   * If Border is Off (the default), then the centers of each of the corner
   * voxels will be considered to form the rectangular bounds of the image.
   * This is the way that VTK normally computes image bounds.  If Border is On,
   * then the image bounds will be defined by the outer corners of the voxels.
   * This setting impacts how the resizing is done.  For example, if a
   * MagnificationFactor of two is applied to a 256x256 image, the output
   * image will be 512x512 if Border is On, or 511x511 if Border is Off.
   */
  vtkSetMacro(Border, int);
  vtkBooleanMacro(Border, int);
  vtkGetMacro(Border, int);
  //@}

  //@{
  /**
   * Whether to crop the input image before resizing (Off by default).  If this
   * is On, then the CroppingRegion must be set.
   */
  vtkSetMacro(Cropping, int);
  vtkBooleanMacro(Cropping, int);
  vtkGetMacro(Cropping, int);
  //@}

  //@{
  /**
   * If Cropping is On, then the CroppingRegion will be used to crop the image
   * before it is resized.  The region must be specified in data coordinates,
   * rather than voxel indices.
   */
  vtkSetVector6Macro(CroppingRegion, double);
  vtkGetVector6Macro(CroppingRegion, double);
  //@}

  //@{
  /**
   * Turn interpolation on or off (by default, interpolation is on).
   */
  vtkSetMacro(Interpolate, int);
  vtkBooleanMacro(Interpolate, int);
  vtkGetMacro(Interpolate, int);
  //@}

  //@{
  /**
   * Set the interpolator for resampling the data.
   */
  virtual void SetInterpolator(vtkAbstractImageInterpolator *sampler);
  virtual vtkAbstractImageInterpolator *GetInterpolator();
  //@}

  /**
   * Get the modified time of the filter.
   */
  vtkMTimeType GetMTime();

protected:
  vtkImageResize();
  ~vtkImageResize();

  virtual vtkAbstractImageInterpolator *GetInternalInterpolator();

  virtual int RequestInformation(vtkInformation *, vtkInformationVector **,
                                 vtkInformationVector *);
  virtual int RequestUpdateExtent(vtkInformation *, vtkInformationVector **,
                                  vtkInformationVector *);
  virtual int RequestData(vtkInformation *, vtkInformationVector **,
                          vtkInformationVector *);
  virtual void ThreadedRequestData(vtkInformation *request,
                                   vtkInformationVector **inputVector,
                                   vtkInformationVector *outputVector,
                                   vtkImageData ***inData,
                                   vtkImageData **outData, int ext[6], int id);

  int ResizeMethod;
  int OutputDimensions[3];
  double OutputSpacing[3];
  double MagnificationFactors[3];
  int Border;
  int Cropping;
  double CroppingRegion[6];

  double IndexStretch[3];
  double IndexTranslate[3];

  vtkAbstractImageInterpolator *Interpolator;
  vtkAbstractImageInterpolator *NNInterpolator;
  int Interpolate;

private:
  vtkImageResize(const vtkImageResize&) VTK_DELETE_FUNCTION;
  void operator=(const vtkImageResize&) VTK_DELETE_FUNCTION;
};

#endif
