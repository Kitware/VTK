/*=========================================================================*/
// .NAME vtkNewVolumeRenderer - Renders all volumetric data
// .SECTION Description

// .SECTION see also
// vtkRenderer vtkNewVolume vtkRLGridVolume

#ifndef __vtkNewVolumeRenderer_h
#define __vtkNewVolumeRenderer_h

#include "vtkRenderer.h"
// #include "vtkNewVolumeCollection.h"

class vtkNewVolumeRenderer : public vtkObject
{
public:
  vtkNewVolumeRenderer();
  ~vtkNewVolumeRenderer();
  char *GetClassName() {return "vtkNewVolumeRenderer";};
  void PrintSelf(ostream& os, vtkIndent indent);

//  void AddVolume(vtkNewVolume *);
//  void RemoveVolume(vtkNewVolume *);
//  vtkNewVolumeCollection *GetVolumes();

  // Description:
  // Render its volumes to create a composite image.
  virtual int Render(vtkRenderer *);

  virtual float *GetCurrentColorBuffer();
  virtual float *GetCurrentZBuffer();
protected:

//  vtkNewVolumeCollection Volumes;

  unsigned char *Image;
};

// Description:
// Get the list of volumes for this renderer.
// inline vtkNewVolumeCollection *vtkNewVolumeRenderer::GetVolumes() 
//  {return &(this->Volumes);};

#endif








