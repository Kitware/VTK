/*=========================================================================*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "vtkNewVolumeRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkVoxel.h"


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
int vtkNewVolumeRenderer::Render(vtkRenderer *ren)
{
  if( 0 )
    ren->GetClassName();

  return( 0 );
}


void vtkNewVolumeRenderer::PrintSelf(ostream& os, vtkIndent indent)
{
  this->vtkObject::PrintSelf(os,indent);

  os << indent << "Volumes:\n";
//  this->Volumes.PrintSelf(os,indent.GetNextIndent());
}

