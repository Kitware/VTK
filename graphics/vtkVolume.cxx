#include <stdlib.h>
#include <math.h>

#include "vtkVolume.h"

// Description:
// Creates a Volume with the following defaults: origin(0,0,0) 
// position=(0,0,0) scale=1 visibility=1 pickable=1 dragable=1
// orientation=(0,0,0).
vtkVolume::vtkVolume()
{
  this->Scale		= 1.0;
  this->VolumeMapper	= NULL;

  this->VolumeProperty	= NULL;
  this->SelfCreatedProperty = 0;
}

// Description:
// Destruct a volume
vtkVolume::~vtkVolume()
{
  if( this->SelfCreatedProperty && this->VolumeProperty )
    this->VolumeProperty->Delete();
}

// Description:
// Shallow copy of an volume.
vtkVolume& vtkVolume::operator=(const vtkVolume& volume)
{

  this->VolumeMapper = volume.VolumeMapper;

  *((vtkProp *)this) = volume;

  this->Scale = volume.Scale;
  
  this->VolumeProperty = volume.VolumeProperty;

  return *this;
}

// Description:
// Copy the volume's composite 4x4 matrix into the matrix provided.
void vtkVolume::GetMatrix(vtkMatrix4x4& result)
{
  this->GetOrientation();
  this->Transform.Push();  
  this->Transform.Identity();  
  this->Transform.PreMultiply();  

  // first translate
  this->Transform.Translate(this->Position[0],
			    this->Position[1],
			    this->Position[2]);
   
  // shift to origin
  this->Transform.Translate(this->Origin[0],
			    this->Origin[1],
			    this->Origin[2]);
   
  // rotate
  this->Transform.RotateZ(this->Orientation[2]);
  this->Transform.RotateX(this->Orientation[0]);
  this->Transform.RotateY(this->Orientation[1]);

  // scale
  this->Transform.Scale(this->Scale,
			this->Scale,
			this->Scale);

  // shift back from origin
  this->Transform.Translate(-this->Origin[0],
			    -this->Origin[1],
			    -this->Origin[2]);

  result = this->Transform.GetMatrix();
  this->Transform.Pop();  
} 

// Description:
// Get the bounds for this Volume as (Xmin,Xmax,Ymin,Ymax,Zmin,Zmax).
float *vtkVolume::GetBounds()
{
  int i,n;
  float *bounds, bbox[24], *fptr;
  float *result;
  vtkMatrix4x4 matrix;
  
  // get the bounds of the Mapper if we have one
  if (!this->VolumeMapper)
    {
    return this->Bounds;
    }

  bounds = this->VolumeMapper->GetBounds();

  // fill out vertices of a bounding box
  bbox[ 0] = bounds[1]; bbox[ 1] = bounds[3]; bbox[ 2] = bounds[5];
  bbox[ 3] = bounds[1]; bbox[ 4] = bounds[2]; bbox[ 5] = bounds[5];
  bbox[ 6] = bounds[0]; bbox[ 7] = bounds[2]; bbox[ 8] = bounds[5];
  bbox[ 9] = bounds[0]; bbox[10] = bounds[3]; bbox[11] = bounds[5];
  bbox[12] = bounds[1]; bbox[13] = bounds[3]; bbox[14] = bounds[4];
  bbox[15] = bounds[1]; bbox[16] = bounds[2]; bbox[17] = bounds[4];
  bbox[18] = bounds[0]; bbox[19] = bounds[2]; bbox[20] = bounds[4];
  bbox[21] = bounds[0]; bbox[22] = bounds[3]; bbox[23] = bounds[4];
  
  // save the old transform
  this->GetMatrix(matrix);
  this->Transform.Push(); 
  this->Transform.Identity();
  this->Transform.Concatenate(matrix);

  // and transform into actors coordinates
  fptr = bbox;
  for (n = 0; n < 8; n++) 
    {
    this->Transform.SetPoint(fptr[0],fptr[1],fptr[2],1.0);
  
    // now store the result
    result = this->Transform.GetPoint();
    fptr[0] = result[0] / result[3];
    fptr[1] = result[1] / result[3];
    fptr[2] = result[2] / result[3];
    fptr += 3;
    }
  
  this->Transform.Pop();  
  
  // now calc the new bounds
  this->Bounds[0] = this->Bounds[2] = this->Bounds[4] = VTK_LARGE_FLOAT;
  this->Bounds[1] = this->Bounds[3] = this->Bounds[5] = -VTK_LARGE_FLOAT;
  for (i = 0; i < 8; i++)
    {
    for (n = 0; n < 3; n++)
      {
      if (bbox[i*3+n] < this->Bounds[n*2]) this->Bounds[n*2] = bbox[i*3+n];
      if (bbox[i*3+n] > this->Bounds[n*2+1]) this->Bounds[n*2+1] = bbox[i*3+n];
      }
    }

  return this->Bounds;
}

// Description:
// Get the minimum X bound
float vtkVolume::GetMinXBound( )
{
  this->GetBounds();
  return this->Bounds[0];
}

// Description:
// Get the maximum X bound
float vtkVolume::GetMaxXBound( )
{
  this->GetBounds();
  return this->Bounds[1];
}

// Description:
// Get the minimum Y bound
float vtkVolume::GetMinYBound( )
{
  this->GetBounds();
  return this->Bounds[2];
}

// Description:
// Get the maximum Y bound
float vtkVolume::GetMaxYBound( )
{
  this->GetBounds();
  return this->Bounds[3];
}

// Description:
// Get the minimum Z bound
float vtkVolume::GetMinZBound( )
{
  this->GetBounds();
  return this->Bounds[4];
}

// Description:
// Get the maximum Z bound
float vtkVolume::GetMaxZBound( )
{
  this->GetBounds();
  return this->Bounds[5];
}

void vtkVolume::Render( vtkRenderer *ren )
{
  if ( !this->VolumeMapper ) return;

  if( !this->VolumeProperty )
    {
    // Force the creation of a property
    this->GetVolumeProperty();
    }

  this->VolumeMapper->Render( ren, this );
}

void vtkVolume::Update()
{
  if ( this->VolumeMapper )
    this->VolumeMapper->Update();
}

void vtkVolume::SetVolumeProperty(vtkVolumeProperty *property)
{
  if( this->VolumeProperty != property )
    {
    if( this->SelfCreatedProperty )
      this->VolumeProperty->Delete();

    this->SelfCreatedProperty = 0;
    this->VolumeProperty = property;
    this->Modified();
    }
}

vtkVolumeProperty *vtkVolume::GetVolumeProperty()
{
  if( this->VolumeProperty == NULL )
    {
    this->VolumeProperty = vtkVolumeProperty::New();
    this->SelfCreatedProperty = 1;
    }
  return this->VolumeProperty;
}

unsigned long int vtkVolume::GetMTime()
{
  unsigned long mTime=this->vtkObject::GetMTime();
  unsigned long time;

  if ( this->VolumeProperty != NULL )
    {
    time = this->VolumeProperty->GetMTime();
    mTime = ( time > mTime ? time : mTime );
    }

  return mTime;
}

void vtkVolume::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkProp::PrintSelf(os,indent);

  if( this->VolumeProperty )
    {
    os << indent << "Volume Property:\n";
    this->VolumeProperty->PrintSelf(os,indent.GetNextIndent());
    }
  else
    {
    os << indent << "Volume Property: (not defined)\n";
    }

  // make sure our bounds are up to date
  if ( this->VolumeMapper )
    {
      this->GetBounds();
      os << indent << "Bounds: (" << this->Bounds[0] << ", " 
	 << this->Bounds[1] << ") (" << this->Bounds[2] << ") ("
	 << this->Bounds[3] << ") (" << this->Bounds[4] << ") ("
	 << this->Bounds[5] << ")\n";
    }
  else
    {
    os << indent << "Bounds: (not defined)\n";
    }
  os << indent << "Scale: (" << this->Scale << ")\n";
}

