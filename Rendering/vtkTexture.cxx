/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTexture.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkTexture.h"

#include "vtkDataSetAttributes.h"
#include "vtkExecutive.h"
#include "vtkGraphicsFactory.h"
#include "vtkImageData.h"
#include "vtkLookupTable.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkTransform.h"

vtkCxxSetObjectMacro(vtkTexture, LookupTable, vtkScalarsToColors);
//----------------------------------------------------------------------------
// Needed when we don't use the vtkStandardNewMacro.
vtkInstantiatorNewMacro(vtkTexture);
//----------------------------------------------------------------------------

// Construct object and initialize.
vtkTexture::vtkTexture()
{
  this->Repeat = 1;
  this->Interpolate = 0;
  this->EdgeClamp = 0;
  this->Quality = VTK_TEXTURE_QUALITY_DEFAULT;
  this->PremultipliedAlpha = false;

  this->LookupTable = NULL;
  this->MappedScalars = NULL;
  this->MapColorScalarsThroughLookupTable = 0;
  this->Transform = NULL;

  this->SelfAdjustingTableRange = 0;

  this->SetNumberOfOutputPorts(0);

  this->BlendingMode = VTK_TEXTURE_BLENDING_MODE_NONE;

  this->RestrictPowerOf2ImageSmaller = 0;

  // By default select active point scalars.
  this->SetInputArrayToProcess(0,0,0,
    vtkDataObject::FIELD_ASSOCIATION_POINTS_THEN_CELLS,
    vtkDataSetAttributes::SCALARS);
}

//----------------------------------------------------------------------------
vtkTexture::~vtkTexture()
{
  if (this->MappedScalars)
    {
    this->MappedScalars->Delete();
    }

  if (this->LookupTable != NULL) 
    {
    this->LookupTable->UnRegister(this);
    }

  if(this->Transform != NULL)
    {
    this->Transform->UnRegister(this);
    }
}

//----------------------------------------------------------------------------
// return the correct type of Texture 
vtkTexture *vtkTexture::New()
{  
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkGraphicsFactory::CreateInstance("vtkTexture");
  return static_cast<vtkTexture *>(ret);
}

//----------------------------------------------------------------------------
vtkImageData *vtkTexture::GetInput()
{
  if (this->GetNumberOfInputConnections(0) < 1)
    {
    return 0;
    }
  return vtkImageData::SafeDownCast(this->GetExecutive()->GetInputData(0, 0)); 
}

//----------------------------------------------------------------------------
void vtkTexture::SetTransform(vtkTransform *transform)
{
  if (transform == this->Transform)
    {
    return;
    }

  if (this->Transform)
    {
    this->Transform->Delete();
    this->Transform = NULL;
    }

  if (transform)
    {
    this->Transform = transform;
    this->Transform->Register(this);
    }
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkTexture::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Interpolate: " << (this->Interpolate ? "On\n" : "Off\n");
  os << indent << "Repeat:      " << (this->Repeat ? "On\n" : "Off\n");
  os << indent << "EdgeClamp:   " << (this->EdgeClamp ? "On\n" : "Off\n");
  os << indent << "Quality:     ";
  switch (this->Quality)
    {
    case VTK_TEXTURE_QUALITY_DEFAULT:
      os << "Default\n";
      break;
    case VTK_TEXTURE_QUALITY_16BIT:
      os << "16Bit\n";
      break;
    case VTK_TEXTURE_QUALITY_32BIT:
      os << "32Bit\n";
      break;
    }
  os << indent << "MapColorScalarsThroughLookupTable: " << 
    (this->MapColorScalarsThroughLookupTable  ? "On\n" : "Off\n");
  os << indent << "PremultipliedAlpha: " << (this->PremultipliedAlpha ? "On\n" : "Off\n");

  if ( this->GetInput() )
    {
    os << indent << "Input: (" << static_cast<void *>(this->GetInput()) << ")\n";
    }
  else
    {
    os << indent << "Input: (none)\n";
    }
  if ( this->LookupTable )
    {
    os << indent << "LookupTable:\n";
    this->LookupTable->PrintSelf (os, indent.GetNextIndent ());
    }
  else
    {
    os << indent << "LookupTable: (none)\n";
    }

  if ( this->MappedScalars )
    {
    os << indent << "Mapped Scalars: " << this->MappedScalars << "\n";
    }
  else
    {
    os << indent << "Mapped Scalars: (none)\n";
    }

  if ( this->Transform )
    {
    os << indent << "Transform: " << this->Transform << "\n";
    }
  else
    {
    os << indent << "Transform: (none)\n";
    }
  os << indent << "MultiTexture Blending Mode:     ";
  switch (this->BlendingMode)
    {
    case VTK_TEXTURE_BLENDING_MODE_NONE:
      os << "None\n";
      break;
    case VTK_TEXTURE_BLENDING_MODE_REPLACE:
      os << "Replace\n";
      break;
    case VTK_TEXTURE_BLENDING_MODE_MODULATE:
      os << "Modulate\n";
      break;
    case VTK_TEXTURE_BLENDING_MODE_ADD:
      os << "Add\n";
      break;
    case VTK_TEXTURE_BLENDING_MODE_ADD_SIGNED:
      os << "Add Signed\n";
      break;
    case VTK_TEXTURE_BLENDING_MODE_INTERPOLATE:
      os << "Interpolate\n";
      break;
    case VTK_TEXTURE_BLENDING_MODE_SUBTRACT:
      os << "Subtract\n";
      break;
    }
  os << indent << "RestrictPowerOf2ImageSmaller:   " << (this->RestrictPowerOf2ImageSmaller ? "On\n" : "Off\n");
}

//----------------------------------------------------------------------------
unsigned char *vtkTexture::MapScalarsToColors (vtkDataArray *scalars)
{
  // if there is no lookup table, create one
  if (this->LookupTable == NULL)
    {
    this->LookupTable = vtkLookupTable::New();
    this->LookupTable->Register(this);
    this->LookupTable->Delete();
    this->LookupTable->Build ();
    this->SelfAdjustingTableRange = 1;
    }
  else
    {
    this->SelfAdjustingTableRange = 0;
    }
  // Delete old colors
  if (this->MappedScalars)
    {
    this->MappedScalars->Delete();
    this->MappedScalars = 0;
    }      
  
  // if the texture created its own lookup table, set the Table Range
  // to the range of the scalar data.
  if (this->SelfAdjustingTableRange)
    {
    this->LookupTable->SetRange(scalars->GetRange(0));
    }
  
  // map the scalars to colors
  this->MappedScalars = this->LookupTable->MapScalars(scalars,
    this->MapColorScalarsThroughLookupTable?
    VTK_COLOR_MODE_MAP_SCALARS : VTK_COLOR_MODE_DEFAULT, -1);
  
  return this->MappedScalars? reinterpret_cast<unsigned char*>(
    this->MappedScalars->GetVoidPointer(0)): NULL;
}

//----------------------------------------------------------------------------
void vtkTexture::Render(vtkRenderer *ren)
{
  vtkImageData *input = this->GetInput();
  
  if (input) //load texture map
    {
    // We do not want more than requested.
    input->RequestExactExtentOn();
    
    // Updating the whole extent may not be necessary.
    input->UpdateInformation();
    input->SetUpdateExtentToWholeExtent();
    input->Update();
    this->Load(ren);
    }
}







