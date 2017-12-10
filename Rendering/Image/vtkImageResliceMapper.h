/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageResliceMapper.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkImageResliceMapper
 * @brief   map a slice of a vtkImageData to the screen
 *
 * vtkImageResliceMapper will cut a 3D image with an abitrary slice plane
 * and draw the results on the screen.  The slice can be set to automatically
 * follow the camera, so that the camera controls the slicing.
 * @par Thanks:
 * Thanks to David Gobbi at the Seaman Family MR Centre and Dept. of Clinical
 * Neurosciences, Foothills Medical Centre, Calgary, for providing this class.
 * @sa
 * vtkImageSlice vtkImageProperty vtkImageSliceMapper
*/

#ifndef vtkImageResliceMapper_h
#define vtkImageResliceMapper_h

#include "vtkRenderingImageModule.h" // For export macro
#include "vtkImageMapper3D.h"

class vtkImageSliceMapper;
class vtkRenderer;
class vtkRenderWindow;
class vtkCamera;
class vtkLookupTable;
class vtkImageSlice;
class vtkImageData;
class vtkImageResliceToColors;
class vtkMatrix4x4;
class vtkAbstractImageInterpolator;

class VTKRENDERINGIMAGE_EXPORT vtkImageResliceMapper : public vtkImageMapper3D
{
public:
  static vtkImageResliceMapper *New();
  vtkTypeMacro(vtkImageResliceMapper,vtkImageMapper3D);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Set the slice that will be used to cut through the image.
   * This slice should be in world coordinates, rather than
   * data coordinates.  Use SliceFacesCamera and SliceAtFocalPoint
   * if you want the slice to automatically follow the camera.
   */
  virtual void SetSlicePlane(vtkPlane *plane);

  //@{
  /**
   * When using SliceAtFocalPoint, this causes the slicing to occur at
   * the closest slice to the focal point, instead of the default behavior
   * where a new slice is interpolated between the original slices.  This
   * flag is ignored if the slicing is oblique to the original slices.
   */
  vtkSetMacro(JumpToNearestSlice, vtkTypeBool);
  vtkBooleanMacro(JumpToNearestSlice, vtkTypeBool);
  vtkGetMacro(JumpToNearestSlice, vtkTypeBool);
  //@}

  //@{
  /**
   * The slab thickness, for thick slicing (default: zero)
   */
  vtkSetMacro(SlabThickness, double);
  vtkGetMacro(SlabThickness, double);
  //@}

  //@{
  /**
   * The slab type, for thick slicing (default: Mean).
   * The resulting view is a parallel projection through the volume.  This
   * method can be used to generate a facsimile of a digitally-reconstructed
   * radiograph or a minimum-intensity projection as long as perspective
   * geometry is not required.  Note that the Sum mode provides an output
   * with units of intensity times distance, while all other modes provide
   * an output with units of intensity.
   */
  vtkSetClampMacro(SlabType, int, VTK_IMAGE_SLAB_MIN, VTK_IMAGE_SLAB_SUM);
  vtkGetMacro(SlabType, int);
  void SetSlabTypeToMin() {
    this->SetSlabType(VTK_IMAGE_SLAB_MIN); };
  void SetSlabTypeToMax() {
    this->SetSlabType(VTK_IMAGE_SLAB_MAX); };
  void SetSlabTypeToMean() {
    this->SetSlabType(VTK_IMAGE_SLAB_MEAN); };
  void SetSlabTypeToSum() {
    this->SetSlabType(VTK_IMAGE_SLAB_SUM); };
  virtual const char *GetSlabTypeAsString();
  //@}

  //@{
  /**
   * Set the number of slab samples to use as a factor of the number
   * of input slices within the slab thickness.  The default value
   * is 2, but 1 will increase speed with very little loss of quality.
   */
  vtkSetClampMacro(SlabSampleFactor, int, 1, 2);
  vtkGetMacro(SlabSampleFactor, int);
  //@}

  //@{
  /**
   * Set the reslice sample frequency as in relation to the input image
   * sample frequency.  The default value is 1, but higher values can be
   * used to improve the results.  This is cheaper than turning on
   * ResampleToScreenPixels.
   */
  vtkSetClampMacro(ImageSampleFactor, int, 1, 16);
  vtkGetMacro(ImageSampleFactor, int);
  //@}

  //@{
  /**
   * Automatically reduce the rendering quality for greater speed
   * when doing an interactive render.  This is on by default.
   */
  vtkSetMacro(AutoAdjustImageQuality, vtkTypeBool);
  vtkBooleanMacro(AutoAdjustImageQuality, vtkTypeBool);
  vtkGetMacro(AutoAdjustImageQuality, vtkTypeBool);
  //@}

  //@{
  /**
   * Resample the image directly to the screen pixels, instead of
   * using a texture to scale the image after resampling.  This is
   * slower and uses more memory, but provides high-quality results.
   * It is On by default.
   */
  vtkSetMacro(ResampleToScreenPixels, vtkTypeBool);
  vtkBooleanMacro(ResampleToScreenPixels, vtkTypeBool);
  vtkGetMacro(ResampleToScreenPixels, vtkTypeBool);
  //@}

  //@{
  /**
   * Keep the color mapping stage distinct from the reslicing stage.
   * This will improve the quality and possibly the speed of interactive
   * window/level operations, but it uses more memory and might slow down
   * interactive slicing operations.  On by default.
   */
  vtkSetMacro(SeparateWindowLevelOperation, vtkTypeBool);
  vtkBooleanMacro(SeparateWindowLevelOperation, vtkTypeBool);
  vtkGetMacro(SeparateWindowLevelOperation, vtkTypeBool);
  //@}

  //@{
  /**
   * Set a custom interpolator.  This will only be used if the
   * ResampleToScreenPixels option is on.
   */
  virtual void SetInterpolator(vtkAbstractImageInterpolator *sampler);
  virtual vtkAbstractImageInterpolator *GetInterpolator();
  //@}

  /**
   * This should only be called by the renderer.
   */
  void Render(vtkRenderer *renderer, vtkImageSlice *prop) override;

  /**
   * Release any graphics resources that are being consumed by
   * this mapper.  The parameter window is used to determine
   * which graphic resources to release.
   */
  void ReleaseGraphicsResources(vtkWindow *) override;

  /**
   * Get the mtime for the mapper.
   */
  vtkMTimeType GetMTime() override;

  //@{
  /**
   * The bounding box (array of six doubles) of the data expressed as
   * (xmin,xmax, ymin,ymax, zmin,zmax).
   */
  double *GetBounds() override;
  void GetBounds(double bounds[6]) override
    { this->vtkAbstractMapper3D::GetBounds(bounds); };
  //@}

  /**
   * Handle requests from the pipeline executive.
   */
  int ProcessRequest(vtkInformation* request,
                     vtkInformationVector** inInfo,
                     vtkInformationVector* outInfo) override;

protected:
  vtkImageResliceMapper();
  ~vtkImageResliceMapper() override;

  /**
   * Do a checkerboard pattern to the alpha of an RGBA image
   */
  void CheckerboardImage(
    vtkImageData *input, vtkCamera *camera, vtkImageProperty *property);

  /**
   * Update the slice-to-world matrix from the camera.
   */
  void UpdateSliceToWorldMatrix(vtkCamera *camera);

  /**
   * Check if the vtkProp3D matrix has changed, and if so, set
   * the WorldToDataMatrix to its inverse.
   */
  void UpdateWorldToDataMatrix(vtkImageSlice *prop);

  /**
   * Update the reslice matrix, which is the slice-to-data matrix.
   */
  void UpdateResliceMatrix(vtkRenderer *ren, vtkImageSlice *prop);

  /**
   * Set all of the reslicing parameters.  This requires that
   * the SliceToWorld and WorldToData matrices are up-to-date.
   */
  void UpdateResliceInformation(vtkRenderer *ren);

  /**
   * Set the interpolation.
   */
  void UpdateResliceInterpolation(vtkImageProperty *property);

  /**
   * Update anything related to the image coloring.
   */
  void UpdateColorInformation(vtkImageProperty *property);

  /**
   * Make a polygon by cutting the data bounds with a plane.
   */
  void UpdatePolygonCoords(vtkRenderer *ren);

  //@{
  /**
   * Override Update to handle some tricky details.
   */
  void Update(int port) override;
  void Update() override;
  int Update(int port, vtkInformationVector* requests) override;
  int Update(vtkInformation* requests) override;
  //@}

  /**
   * Garbage collection for reference loops.
   */
  void ReportReferences(vtkGarbageCollector*) override;

  vtkImageSliceMapper *SliceMapper; // Does the OpenGL rendering

  vtkTypeBool JumpToNearestSlice; // Adjust SliceAtFocalPoint
  vtkTypeBool AutoAdjustImageQuality; // LOD-style behavior
  vtkTypeBool SeparateWindowLevelOperation; // Do window/level as a separate step
  double SlabThickness; // Current slab thickness
  int SlabType; // Current slab mode
  int SlabSampleFactor; // Sampling factor for slab mode
  int ImageSampleFactor; // Sampling factor for image pixels
  vtkTypeBool ResampleToScreenPixels; // Use software interpolation only
  int InternalResampleToScreenPixels; // Use software interpolation only
  int ResliceNeedUpdate; // Execute reslice on next render
  vtkImageResliceToColors *ImageReslice; // For software interpolation
  vtkMatrix4x4 *ResliceMatrix; // Cached reslice matrix
  vtkMatrix4x4 *WorldToDataMatrix; // World to Data transform matrix
  vtkMatrix4x4 *SliceToWorldMatrix; // Slice to World transform matrix
  vtkTimeStamp UpdateTime;

private:
  vtkImageResliceMapper(const vtkImageResliceMapper&) = delete;
  void operator=(const vtkImageResliceMapper&) = delete;
};

#endif
