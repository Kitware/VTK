/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVolumeMapper.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
#include "vtkVolumeMapper.h"

// Construct a vtkVolumeMapper with empty scalar input and clipping off.
vtkVolumeMapper::vtkVolumeMapper()
{
  this->RGBTextureInput = NULL;
  this->Clipping = 0;
  this->Bounds[0] = this->Bounds[2] = this->Bounds[4] = -1.0;
  this->Bounds[1] = this->Bounds[3] = this->Bounds[5] = 1.0;
  this->Center[0] = this->Center[1] = this->Center[2] = 0.0;
}

vtkVolumeMapper::~vtkVolumeMapper()
{
  this->SetInput((vtkStructuredPoints *)NULL);
  if (this->RGBTextureInput)
    {
    this->RGBTextureInput->Delete ();
    this->RGBTextureInput = NULL;
    }
}

void vtkVolumeMapper::Update()
{
  if ( this->Input )
    {
    this->Input->Update();
    }

  if ( this->RGBTextureInput )
    {
    this->RGBTextureInput->Update();
    }
}

void vtkVolumeMapper::SetInput( vtkStructuredPoints *input )
{
  if ( (vtkDataSet *)input != this->Input )
    {
    // If we have data already, unregister it
    if ( this->Input )
      {
      this->Input->UnRegister(this);
      }
    // Set the input data
    this->Input = (vtkDataSet *)input;
    // If this is not NULL, register it
    if ( this->Input )
      {
      this->Input->Register(this);
      }
    // We've been modified!
    this->Modified();    
    }
}

void vtkVolumeMapper::SetRGBTextureInput( vtkStructuredPoints *rgbTexture )
{
  vtkPointData    *pd;
  vtkScalars      *scalars;

  if ( rgbTexture != this->RGBTextureInput )
    {
    // If we are actually setting a texture (not NULL) then
    // do some error checking to make sure it is of the right
    // type with the right number of components
    if ( rgbTexture )
      {
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
    // If we have a texture already, unregister it
    if ( this->RGBTextureInput )
      {
      this->RGBTextureInput->UnRegister(this);
      }
    // Set the texture
    this->RGBTextureInput = rgbTexture;
    // If this is not NULL, register it
    if ( this->RGBTextureInput )
      {
      this->RGBTextureInput->Register(this);
      }
    // We've been modified!
    this->Modified();
    }
}

// Print the vtkVolumeMapper
void vtkVolumeMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkAbstractMapper::PrintSelf(os,indent);

  if ( this->RGBTextureInput )
    {
    os << indent << "RGBTextureInput: (" << this->RGBTextureInput << ")\n";
    }
  else
    {
    os << indent << "RGBTextureInput: (none)\n";
    }

  os << indent << "Clipping: " << (this->Clipping ? "On\n" : "Off\n");
  os << indent << "Build Time: " <<this->BuildTime.GetMTime() << "\n";
}

