/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVolumeMapper.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkVolumeMapper.h"

// Construct a vtkVolumeMapper with empty scalar input and clipping off.
vtkVolumeMapper::vtkVolumeMapper()
{
  int i;

  this->Cropping = 0;
  for ( i = 0; i < 3; i++ )
    {
    this->CroppingRegionPlanes[2*i    ] = 0;
    this->CroppingRegionPlanes[2*i + 1] = 1;
    }
  this->Bounds[0] = this->Bounds[2] = this->Bounds[4] = -1.0;
  this->Bounds[1] = this->Bounds[3] = this->Bounds[5] = 1.0;
  this->Center[0] = this->Center[1] = this->Center[2] = 0.0;
  this->CroppingRegionFlags = 0x02000;
}

vtkVolumeMapper::~vtkVolumeMapper()
{
}

void vtkVolumeMapper::Update()
{
  if ( this->GetInput() )
    {
    this->GetInput()->UpdateInformation();
    this->GetInput()->SetUpdateExtentToWholeExtent();
    this->GetInput()->RequestExactExtentOn();
    this->GetInput()->Update();
    }

  if ( this->GetRGBTextureInput() )
    {
    this->GetRGBTextureInput()->UpdateInformation();
    this->GetRGBTextureInput()->SetUpdateExtentToWholeExtent();
    this->GetRGBTextureInput()->RequestExactExtentOn();
    this->GetRGBTextureInput()->Update();
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

void vtkVolumeMapper::SetInput( vtkImageData *input )
{
  this->vtkProcessObject::SetNthInput(0, input);
}

vtkImageData *vtkVolumeMapper::GetInput()
{
  if (this->NumberOfInputs < 1)
    {
    return NULL;
    }
  return (vtkImageData*)(this->Inputs[0]);
}


void vtkVolumeMapper::SetRGBTextureInput( vtkImageData *rgbTexture )
{
  vtkPointData    *pd;
  vtkDataArray    *scalars;

  if ( rgbTexture )
    {
    rgbTexture->UpdateInformation();
    rgbTexture->SetUpdateExtentToWholeExtent();
    rgbTexture->Update();
    pd = rgbTexture->GetPointData();
    if ( !pd )
      {
      vtkErrorMacro( << "No PointData in texture!" );
      return;
      }
    scalars = pd->GetScalars();
    if ( !scalars )
      {
      vtkErrorMacro( << "No scalars in texture!" );
      return;
      }
    if ( scalars->GetDataType() != VTK_UNSIGNED_CHAR )
      {
      vtkErrorMacro( << "Scalars in texture must be unsigned char!" );
      return;      
      }
    if ( scalars->GetNumberOfComponents() != 3 )
      {
      vtkErrorMacro( << "Scalars must have 3 components (r, g, and b)" );
      return;      
      }
    }
  
  this->vtkProcessObject::SetNthInput(1, rgbTexture);

}

vtkImageData *vtkVolumeMapper::GetRGBTextureInput()
{
  if (this->NumberOfInputs < 2)
    {
    return NULL;
    }
  return (vtkImageData *)(this->Inputs[1]);
}


// Print the vtkVolumeMapper
void vtkVolumeMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->vtkAbstractMapper3D::PrintSelf(os,indent);

  if ( this->GetRGBTextureInput() )
    {
    os << indent << "RGBTextureInput: (" << this->GetRGBTextureInput() 
       << ")\n";
    }
  else
    {
    os << indent << "RGBTextureInput: (none)\n";
    }

  os << indent << "Cropping: " << (this->Cropping ? "On\n" : "Off\n");

  os << indent << "Cropping Region Planes: " << endl 
     << indent << "  In X: " << this->CroppingRegionPlanes[0] 
     << " to " << this->CroppingRegionPlanes[1] << endl 
     << indent << "  In Y: " << this->CroppingRegionPlanes[2] 
     << " to " << this->CroppingRegionPlanes[3] << endl 
     << indent << "  In Z: " << this->CroppingRegionPlanes[4] 
     << " to " << this->CroppingRegionPlanes[5] << endl;
 
  os << indent << "Cropping Region Flags: " << this->CroppingRegionFlags << endl;

  os << indent << "Build Time: " <<this->BuildTime.GetMTime() << "\n";
}

