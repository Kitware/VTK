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
// .NAME vtkImageResliceMapper - map a slice of a vtkImageData to the screen
// .SECTION Description
// vtkImageResliceMapper will cut a 3D image with an abitrary slice plane
// and draw the results on the screen.  The slice can be set to automatically
// follow the camera, so that the camera controls the slicing.
// .SECTION Thanks
// Thanks to David Gobbi at the Seaman Family MR Centre and Dept. of Clinical
// Neurosciences, Foothills Medical Centre, Calgary, for providing this class.
// .SECTION See also
// vtkImageSlice vtkImageProperty vtkImageSliceMapper

#ifndef __vtkImageResliceMapper_h
#define __vtkImageResliceMapper_h

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
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the slice that will be used to cut through the image.
  // This slice should be in world coordinates, rather than
  // data coordinates.  Use SliceFacesCamera and SliceAtFocalPoint
  // if you want the slice to automatically follow the camera.
  virtual void SetSlicePlane(vtkPlane *plane);

  // Description:
  // When using SliceAtFocalPoint, this causes the slicing to occur at
  // the closest slice to the focal point, instead of the default behavior
  // where a new slice is interpolated between the original slices.  This
  // flag is ignored if the slicing is oblique to the original slices.
  vtkSetMacro(JumpToNearestSlice, int);
  vtkBooleanMacro(JumpToNearestSlice, int);
  vtkGetMacro(JumpToNearestSlice, int);

  // Description:
  // The slab thickness, for thick slicing (default: zero)
  vtkSetMacro(SlabThickness, double);
  vtkGetMacro(SlabThickness, double);

  // Description:
  // The slab type, for thick slicing (default: mean)
  vtkSetClampMacro(SlabType, int, VTK_IMAGE_SLAB_MIN, VTK_IMAGE_SLAB_MEAN);
  vtkGetMacro(SlabType, int);
  void SetSlabTypeToMin() {
    this->SetSlabType(VTK_IMAGE_SLAB_MIN); };
  void SetSlabTypeToMax() {
    this->SetSlabType(VTK_IMAGE_SLAB_MAX); };
  void SetSlabTypeToMean() {
    this->SetSlabType(VTK_IMAGE_SLAB_MEAN); };
  virtual const char *GetSlabTypeAsString();

  // Description:
  // Set the number of slab samples to use as a factor of the number
  // of input slices within the slab thickness.  The default value
  // is 2, but 1 will increase speed with very little loss of quality.
  vtkSetClampMacro(SlabSampleFactor, int, 1, 2);
  vtkGetMacro(SlabSampleFactor, int);

  // Description:
  // Set the reslice sample frequency as in relation to the input image
  // sample frequency.  The default value is 1, but higher values can be
  // used to improve the results.  This is cheaper than turning on
  // ResampleToScreenPixels.
  vtkSetClampMacro(ImageSampleFactor, int, 1, 16);
  vtkGetMacro(ImageSampleFactor, int);

  // Description:
  // Automatically reduce the rendering quality for greater speed
  // when doing an interactive render.  This is on by default.
  vtkSetMacro(AutoAdjustImageQuality, int);
  vtkBooleanMacro(AutoAdjustImageQuality, int);
  vtkGetMacro(AutoAdjustImageQuality, int);

  // Description:
  // Resample the image directly to the screen pixels, instead of
  // using a texture to scale the image after resampling.  This is
  // slower and uses more memory, but provides high-quality results.
  // It is On by default.
  vtkSetMacro(ResampleToScreenPixels, int);
  vtkBooleanMacro(ResampleToScreenPixels, int);
  vtkGetMacro(ResampleToScreenPixels, int);

  // Description:
  // Keep the color mapping stage distinct from the reslicing stage.
  // This will improve the quality and possibly the speed of interactive
  // window/level operations, but it uses more memory and might slow down
  // interactive slicing operations.  On by default.
  vtkSetMacro(SeparateWindowLevelOperation, int);
  vtkBooleanMacro(SeparateWindowLevelOperation, int);
  vtkGetMacro(SeparateWindowLevelOperation, int);

  // Description:
  // Set a custom interpolator.  This will only be used if the
  // ResampleToScreenPixels option is on.
  virtual void SetInterpolator(vtkAbstractImageInterpolator *sampler);
  virtual vtkAbstractImageInterpolator *GetInterpolator();

  // Description:
  // This should only be called by the renderer.
  virtual void Render(vtkRenderer *renderer, vtkImageSlice *prop);

  // Description:
  // Release any graphics resources that are being consumed by
  // this mapper.  The parameter window is used to determine
  // which graphic resources to release.
  virtual void ReleaseGraphicsResources(vtkWindow *);

  // Description:
  // Get the mtime for the mapper.
  unsigned long GetMTime();

  // Description:
  // The bounding box (array of six doubles) of the data expressed as
  // (xmin,xmax, ymin,ymax, zmin,zmax).
  double *GetBounds();
  void GetBounds(double bounds[6])
    { this->vtkAbstractMapper3D::GetBounds(bounds); };

  // Description:
  // Handle requests from the pipeline executive.
  int ProcessRequest(vtkInformation* request,
                     vtkInformationVector** inInfo,
                     vtkInformationVector* outInfo);

protected:
  vtkImageResliceMapper();
  ~vtkImageResliceMapper();

  // Description:
  // Do a checkerboard pattern to the alpha of an RGBA image
  void CheckerboardImage(
    vtkImageData *input, vtkCamera *camera, vtkImageProperty *property);

  // Description:
  // Update the slice-to-world matrix from the camera.
  void UpdateSliceToWorldMatrix(vtkCamera *camera);

  // Description:
  // Check if the vtkProp3D matrix has changed, and if so, set
  // the WorldToDataMatrix to its inverse.
  void UpdateWorldToDataMatrix(vtkImageSlice *prop);

  // Description:
  // Update the reslice matrix, which is the slice-to-data matrix.
  void UpdateResliceMatrix(vtkRenderer *ren, vtkImageSlice *prop);

  // Description:
  // Set all of the reslicing parameters.  This requires that
  // the SliceToWorld and WorldToData matrices are up-to-date.
  void UpdateResliceInformation(vtkRenderer *ren);

  // Description:
  // Set the interpolation.
  void UpdateResliceInterpolation(vtkImageProperty *property);

  // Description:
  // Update anything related to the image coloring.
  void UpdateColorInformation(vtkImageProperty *property);

  // Description:
  // Make a polygon by cutting the data bounds with a plane.
  void UpdatePolygonCoords(vtkRenderer *ren);

  // Description:
  // Override Update to handle some tricky details.
  void Update();
  void Update(int port);

  // Description:
  // Garbage collection for reference loops.
  void ReportReferences(vtkGarbageCollector*);

  vtkImageSliceMapper *SliceMapper; // Does the OpenGL rendering

  int JumpToNearestSlice; // Adjust SliceAtFocalPoint
  int AutoAdjustImageQuality; // LOD-style behavior
  int SeparateWindowLevelOperation; // Do window/level as a separate step
  double SlabThickness; // Current slab thickness
  int SlabType; // Current slab mode
  int SlabSampleFactor; // Sampling factor for slab mode
  int ImageSampleFactor; // Sampling factor for image pixels
  int ResampleToScreenPixels; // Use software interpolation only
  int InternalResampleToScreenPixels; // Use software interpolation only
  int ResliceNeedUpdate; // Execute reslice on next render
  vtkImageResliceToColors *ImageReslice; // For software interpolation
  vtkMatrix4x4 *ResliceMatrix; // Cached reslice matrix
  vtkMatrix4x4 *WorldToDataMatrix; // World to Data transform matrix
  vtkMatrix4x4 *SliceToWorldMatrix; // Slice to World transform matrix
  vtkTimeStamp UpdateTime;

private:
  vtkImageResliceMapper(const vtkImageResliceMapper&);  // Not implemented.
  void operator=(const vtkImageResliceMapper&);  // Not implemented.
};

#endif
