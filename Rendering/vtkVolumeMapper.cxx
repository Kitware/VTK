/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVolumeMapper.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkVolumeMapper.h"

#include "vtkImageClip.h"
#include "vtkImageData.h"

vtkCxxRevisionMacro(vtkVolumeMapper, "1.45");

// Construct a vtkVolumeMapper with empty scalar input and clipping off.
vtkVolumeMapper::vtkVolumeMapper()
{
  int i;

  this->Cropping = 0;
  for ( i = 0; i < 3; i++ )
    {
    this->CroppingRegionPlanes[2*i    ]      = 0;
    this->CroppingRegionPlanes[2*i + 1]      = 1;
    this->VoxelCroppingRegionPlanes[2*i]     = 0;
    this->VoxelCroppingRegionPlanes[2*i + 1] = 1;    
    }
  this->Bounds[0] = this->Bounds[2] = this->Bounds[4] = -1.0;
  this->Bounds[1] = this->Bounds[3] = this->Bounds[5] = 1.0;
  this->Center[0] = this->Center[1] = this->Center[2] = 0.0;
  this->CroppingRegionFlags = VTK_CROP_SUBVOLUME;

  this->UseImageClipper = 1;
  this->ImageClipper = vtkImageClip::New();
  this->ImageClipper->ClipDataOn();
  
  this->IndependentComponents = 1;
  this->ComponentBlendMode = vtkVolumeMapper::ComponentBlendAdd;
}

vtkVolumeMapper::~vtkVolumeMapper()
{
  
  this->ImageClipper->Delete();
}

void vtkVolumeMapper::Update()
{
  if ( this->GetInput() )
    {
    this->GetInput()->UpdateInformation();
    this->GetInput()->SetUpdateExtentToWholeExtent();
    this->GetInput()->Update();
    }
}

void vtkVolumeMapper::ConvertCroppingRegionPlanesToVoxels()
{
  float *spacing    = this->GetInput()->GetSpacing();
  float *origin     = this->GetInput()->GetOrigin();
  int   *dimensions = this->GetInput()->GetDimensions();
  
  for ( int i = 0; i < 6; i++ )
    {
    this->VoxelCroppingRegionPlanes[i] =
      (this->CroppingRegionPlanes[i] - origin[i/2]) / spacing[i/2];
    
    this->VoxelCroppingRegionPlanes[i] = 
      ( this->VoxelCroppingRegionPlanes[i] < 0 ) ?
      ( 0 ) : ( this->VoxelCroppingRegionPlanes[i] );

    this->VoxelCroppingRegionPlanes[i] = 
      ( this->VoxelCroppingRegionPlanes[i] > dimensions[i/2]-1 ) ?
      ( dimensions[i/2]-1 ) : ( this->VoxelCroppingRegionPlanes[i] );
    }
}

// Get the bounds for the input of this mapper as 
// (Xmin,Xmax,Ymin,Ymax,Zmin,Zmax).
float *vtkVolumeMapper::GetBounds()
{
  static float bounds[] = {-1.0,1.0, -1.0,1.0, -1.0,1.0};

  if ( ! this->GetInput() ) 
    {
    return bounds;
    }
  else
    {
    this->GetInput()->UpdateInformation();
    this->GetInput()->SetUpdateExtentToWholeExtent();
    this->GetInput()->Update();
    this->GetInput()->GetBounds(this->Bounds);
    return this->Bounds;
    }
}

void vtkVolumeMapper::SetUseImageClipper(int arg)
{
  if (this->UseImageClipper == arg)
    {
    return;
    }

  this->UseImageClipper = arg;
  this->Modified();

  // Force a change of the input to reconnect the pipeline correctly

  vtkImageData *input = this->GetInput();
  this->SetInput(NULL);
  this->SetInput(input);
}

void vtkVolumeMapper::SetInput( vtkImageData *input )
{
  if (this->UseImageClipper)
    {
    this->ImageClipper->SetInput(input);
    /*
      if (input)
      {
      this->ImageClipper->ResetOutputWholeExtent();
      }
    */
    this->vtkProcessObject::SetNthInput(0, this->ImageClipper->GetOutput());
    }
  else
    {
    this->vtkProcessObject::SetNthInput(0, input);
    }
}

vtkImageData *vtkVolumeMapper::GetInput()
{
  if (this->NumberOfInputs < 1)
    {
    return NULL;
    }
  return (vtkImageData *)this->Inputs[0];
}


// Print the vtkVolumeMapper
void vtkVolumeMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Independent Components: " << 
    (this->IndependentComponents ? "On\n" : "Off\n");
  
  os << indent << "Component Blend Mode: ";
  switch ( this->ComponentBlendMode )
    {
    case vtkVolumeMapper::ComponentBlendAdd: 
      os << "Add\n"; 
      break;
    case vtkVolumeMapper::ComponentBlendMaxOpacity:
      os << "Maximum Opacity\n";
      break;
    default:
      os << "Unknown\n";
      break;
    }
  
  os << indent << "Cropping: " << (this->Cropping ? "On\n" : "Off\n");

  os << indent << "Cropping Region Planes: " << endl 
     << indent << "  In X: " << this->CroppingRegionPlanes[0] 
     << " to " << this->CroppingRegionPlanes[1] << endl 
     << indent << "  In Y: " << this->CroppingRegionPlanes[2] 
     << " to " << this->CroppingRegionPlanes[3] << endl 
     << indent << "  In Z: " << this->CroppingRegionPlanes[4] 
     << " to " << this->CroppingRegionPlanes[5] << endl;
 
  os << indent << "Cropping Region Flags: " 
     << this->CroppingRegionFlags << endl;

  // Don't print this->VoxelCroppingRegionPlanes
  
  os << indent << "Build Time: " <<this->BuildTime.GetMTime() << "\n";

  os << indent << "UseImageClipper: " 
     << (this->UseImageClipper ? "On\n" : "Off\n");
}

