/*=========================================================================*/
// .NAME vtkVolume - represents a volume (data & properties) in a rendered scene
//
// .SECTION Description
// vtkVolume is used to represent a volumetric entity in a rendering scene.
// It inherits functions related to the volume's position, orientation and
// origin from vtkProp. The volume also maintains a reference to the
// volumetric data (i.e., the volume mapper), which itself contains all
// rendering parameters.

// .SECTION see also
// vtkVolumeMapper vtkProp

#ifndef __vtkVolume_h
#define __vtkVolume_h

#include "vtkProp.h"
#include "vtkTransform.h"
#include "vtkVolumeMapper.h"

class vtkRenderer;
class vtkVolumeMapper;

class vtkVolume : public vtkProp
{
 public:
  vtkVolume();
  ~vtkVolume();
  char *GetClassName() {return "vtkVolume";};
  void PrintSelf(ostream& os, vtkIndent indent);

  vtkVolume &operator=(const vtkVolume& volume);

  virtual void Render(vtkRenderer *ren);
  virtual void Update();

  // Description:
  // Set/Get the scale of the volume. Scaling in performed isotropically in
  // X,Y and Z. Any scale values that are zero will be automatically
  // converted to one. Non-isotropic scaling must be done in the 
  // scalar data provided to vtkVolumeMapper.
  vtkGetMacro(Scale,float);
  vtkSetMacro(Scale,float);

  // Description:
  // Get the matrix from the position, origin, scale and orientation
  // This matrix is cached, so multiple GetMatrix() calls will be
  // efficient.
  void GetMatrix(vtkMatrix4x4& m);

  // Description:
  // Get the bounds. GetBounds(),
  // GetXRange(), GetYRange(), and GetZRange return world coordinates.
  float *GetBounds();
  float GetMinXBound();
  float GetMaxXBound();
  float GetMinYBound();
  float GetMaxYBound();
  float GetMinZBound();
  float GetMaxZBound();


  vtkSetObjectMacro(VolumeMapper,vtkVolumeMapper);
  vtkGetObjectMacro(VolumeMapper,vtkVolumeMapper);

protected:

  float             Scale;
  vtkMatrix4x4      Matrix;
  vtkTimeStamp      MatrixMTime;

  vtkVolumeMapper   *VolumeMapper;
};

#endif

