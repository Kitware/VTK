#include "vtkVolumeMapper.h"

// Description:
// Construct a vtkVolumeMapper with empty scalar input and clipping off.
vtkVolumeMapper::vtkVolumeMapper()
{
  this->ScalarInput = NULL;
  this->Clipping = 0;
}

// Description:
// Destroy the object.
vtkVolumeMapper::~vtkVolumeMapper()
{
}

void vtkVolumeMapper::operator=(const vtkVolumeMapper& m)
{
}

void vtkVolumeMapper::Update()
{
  if ( this->ScalarInput )
    {
    this->ScalarInput->Update();
    }
}

// Description:
// Get the bounds of the ScalarInput
float *vtkVolumeMapper::GetBounds()
{
  static float bounds[] = {-1.0,1.0, -1.0,1.0, -1.0,1.0};

  if ( ! this->ScalarInput ) 
    return bounds;
  else
    {
    this->ScalarInput->Update();
    return this->ScalarInput->GetBounds();
    }
}

// Description:
// Print the vtkVolumeMapper
void vtkVolumeMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkObject::PrintSelf(os,indent);

  if ( this->ScalarInput )
    {
    os << indent << "ScalarInput: (" << this->ScalarInput << ")\n";
    }
  else
    {
    os << indent << "ScalarInput: (none)\n";
    }

  os << indent << "Clipping: " << (this->Clipping ? "On\n" : "Off\n");
  os << indent << "Build Time: " <<this->BuildTime.GetMTime() << "\n";
}

