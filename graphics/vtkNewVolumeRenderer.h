/*=========================================================================*/
// .NAME vtkNewVolumeRenderer - Renders all volumetric data
// .SECTION Description

// .SECTION see also
// vtkRenderer vtkNewVolume vtkRLGridVolume

#ifndef __vtkNewVolumeRenderer_hh
#define __vtkNewVolumeRenderer_hh

#include "vtkRenderer.hh"
// #include "vtkNewVolumeCollection.hh"

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

protected:

//  vtkNewVolumeCollection Volumes;

  unsigned char *Image;
};

// Description:
// Get the list of volumes for this renderer.
// inline vtkNewVolumeCollection *vtkNewVolumeRenderer::GetVolumes() 
//  {return &(this->Volumes);};

#endif








