/*=========================================================================*/
#include <stdlib.h>
#include <string.h>

#include "vtkNewVolumeRenderer.hh"
#include "vtkRenderWindow.hh"
#include "vtkMath.hh"
#include "vtkVoxel.hh"

static vtkMath math;

vtkNewVolumeRenderer::vtkNewVolumeRenderer()
{
  this->Image = NULL;
}

vtkNewVolumeRenderer::~vtkNewVolumeRenderer()
{
  this->Image = NULL;
}

// Description:
// Main routine to do the volume rendering.
void vtkNewVolumeRenderer::Render(vtkRenderer *ren)
{

}


void vtkNewVolumeRenderer::PrintSelf(ostream& os, vtkIndent indent)
{
  this->vtkObject::PrintSelf(os,indent);

  os << indent << "Volumes:\n";
//  this->Volumes.PrintSelf(os,indent.GetNextIndent());
}

