/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageSliceMapper.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkImageSliceMapper - map a slice of a vtkImageData to the screen
// .SECTION Description
// vtkImageSliceMapper is a mapper that will draw a 2D image, or a slice
// of a 3D image.  For 3D images, the slice may be oriented in the X, Y,
// or Z direction.  This mapper works via 2D textures with accelerated
// zoom and pan operations.
// .SECTION Thanks
// Thanks to David Gobbi at the Seaman Family MR Centre and Dept. of Clinical
// Neurosciences, Foothills Medical Centre, Calgary, for providing this class.
// .SECTION See also
// vtkImageSlice vtkImageProperty vtkImageResliceMapper

#ifndef vtkImageSliceMapper_h
#define vtkImageSliceMapper_h

#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkImageMapper3D.h"

class vtkCamera;
class vtkPoints;

class VTKRENDERINGCORE_EXPORT vtkImageSliceMapper : public vtkImageMapper3D
{
public:
  static vtkImageSliceMapper *New();
  vtkTypeMacro(vtkImageSliceMapper,vtkImageMapper3D);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // The slice to display, if there are multiple slices.
  virtual void SetSliceNumber(int slice);
  virtual int GetSliceNumber();

  // Description
  // Use GetSliceNumberMinValue() and GetSliceNumberMaxValue()
  // to get the range of allowed slices.  These methods call
  // UpdateInformation as a side-effect.
  virtual int GetSliceNumberMinValue();
  virtual int GetSliceNumberMaxValue();

  // Description:
  // Set the orientation of the slices to display.  The default
  // orientation is 2, which is Z.
  vtkSetClampMacro(Orientation, int, 0, 2);
  vtkGetMacro(Orientation, int);
  void SetOrientationToX() { this->SetOrientation(0); }
  void SetOrientationToY() { this->SetOrientation(1); }
  void SetOrientationToZ() { this->SetOrientation(2); }

  // Description:
  // Use the specified CroppingRegion.  The default is to display
  // the full slice.
  vtkSetMacro(Cropping, int);
  vtkBooleanMacro(Cropping, int);
  vtkGetMacro(Cropping, int);

  // Description:
  // Set the display extent.  This is ignored unless Cropping
  // is set.
  vtkSetVector6Macro(CroppingRegion, int);
  vtkGetVector6Macro(CroppingRegion, int);

  // Description:
  // Override Update to handle some tricky details.
  virtual void Update();
  virtual void Update(int port);

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
  // The bounding box (array of six doubles) of data expressed as
  // (xmin,xmax, ymin,ymax, zmin,zmax).
  double *GetBounds();
  void GetBounds(double bounds[6]) {
    this->vtkAbstractMapper3D::GetBounds(bounds); };

  // Description:
  // Get the plane as a homogeneous 4-vector that gives the plane
  // equation coefficients.  It is computed from the Orientation
  // and SliceNumber, the propMatrix is unused and can be zero.
  virtual void GetSlicePlaneInDataCoords(vtkMatrix4x4 *propMatrix,
                                         double plane[4]);

  // Description:
  // Handle requests from the pipeline executive.
  virtual int ProcessRequest(vtkInformation* request,
                             vtkInformationVector** inInfo,
                             vtkInformationVector* outInfo);

protected:
  vtkImageSliceMapper();
  ~vtkImageSliceMapper();

  // Description:
  // Set points that describe a polygon on which the slice will
  // be rendered.
  void SetPoints(vtkPoints *points);
  vtkPoints *GetPoints() { return this->Points; }

  // Description:
  // Force linear interpolation.  Internal method, for when this
  // mapper is used as a helper class.
  void SetExactPixelMatch(int v) {
    this->ExactPixelMatch = (v != 0); }

  // Description:
  // Pass color data.  Internal method, for when this mapper is
  // used as a helper class.
  void SetPassColorData(int v) {
    this->PassColorData = (v != 0); }

  // Description:
  // Set the display extent.  Internal method, for when this mapper
  // is used as a helper class.
  void SetDisplayExtent(int extent[6]) {
    this->DisplayExtent[0] = extent[0];
    this->DisplayExtent[1] = extent[1];
    this->DisplayExtent[2] = extent[2];
    this->DisplayExtent[3] = extent[3];
    this->DisplayExtent[4] = extent[4];
    this->DisplayExtent[5] = extent[5]; }

  // Description:
  // Get the camera orientation as a simple integer [0,1,2,3,4,5]
  // that indicates one of the six major directions.  The integers
  // 0,1,2 are x,y,z and 3,4,5 are -x,-y,-z.
  int GetOrientationFromCamera(vtkMatrix4x4 *propMatrix, vtkCamera *camera);

  // Description:
  // Get the current slice as the one closest to the focal point.
  int GetSliceFromCamera(vtkMatrix4x4 *propMatrix, vtkCamera *camera);

  // Description:
  // Get the dimension indices according to the orientation.
  static void GetDimensionIndices(int orientation, int &xdim, int &ydim);

  int SliceNumber;
  int SliceNumberMinValue;
  int SliceNumberMaxValue;
  int Orientation;
  int Cropping;
  int CroppingRegion[6];
  int DisplayExtent[6];
  int ExactPixelMatch;
  int PassColorData;
  vtkPoints *Points;
  vtkTimeStamp UpdateTime;

private:
  vtkImageSliceMapper(const vtkImageSliceMapper&);  // Not implemented.
  void operator=(const vtkImageSliceMapper&);  // Not implemented.

  friend class vtkImageResliceMapper;
};

#endif
