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

#include "vtkImageMapper3D.h"

class vtkRenderer;
class vtkCamera;
class vtkLookupTable;
class vtkImageSlice;
class vtkImageData;
class vtkImageResliceToColors;
class vtkMatrix4x4;

class VTK_RENDERING_EXPORT vtkImageResliceMapper : public vtkImageMapper3D
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
  // Compute the coords and texcoords for the image poly.
  void MakeTextureCutGeometry(
    vtkImageData *input, const int extent[6], int border,
    double coords[18], double tcoords[12], int &ncoords);

  // Description:
  // Update the slice-to-world matrix from the camera.
  void UpdateSliceToWorldMatrix(vtkCamera *camera);

  // Description:
  // Check if the vtkProp3D matrix has changed, and if so, set
  // the WorldToDataMatrix to its inverse.
  void UpdateWorldToDataMatrix(vtkImageSlice *prop);

  // Description:
  // Set all of the reslicing parameters.
  void UpdateResliceInformation(vtkRenderer *ren);

  // Description:
  // Set the interpolation.
  void UpdateResliceInterpolation(vtkImageProperty *property);

  // Description:
  // Garbage collection for reference loops.
  void ReportReferences(vtkGarbageCollector*);

  vtkImageResliceToColors *ImageReslice; // For software interpolation
  vtkMatrix4x4 *ResliceMatrix; // Cached reslice matrix
  vtkMatrix4x4 *WorldToDataMatrix; // World to Data transform matrix
  vtkMatrix4x4 *SliceToWorldMatrix; // Slice to World transform matrix

private:
  vtkImageResliceMapper(const vtkImageResliceMapper&);  // Not implemented.
  void operator=(const vtkImageResliceMapper&);  // Not implemented.
};

#endif
