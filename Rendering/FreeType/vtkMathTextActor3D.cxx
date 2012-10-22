/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMathTextActor3D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkMathTextActor3D.h"

#include "vtkObjectFactory.h"
#include "vtkCamera.h"
#include "vtkImageActor.h"
#include "vtkImageData.h"
#include "vtkFreeTypeUtilities.h"
#include "vtkTransform.h"
#include "vtkTextProperty.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkWindow.h"
#include "vtkMathTextUtilities.h"
#include "vtkMath.h"

vtkStandardNewMacro(vtkMathTextActor3D)

// ----------------------------------------------------------------------------
vtkMathTextActor3D::vtkMathTextActor3D()
{
  this->FallbackText = NULL;
}

// --------------------------------------------------------------------------
vtkMathTextActor3D::~vtkMathTextActor3D()
{
  this->SetFallbackText(NULL);
}

// --------------------------------------------------------------------------
bool vtkMathTextActor3D::IsSupported()
{
  return vtkMathTextUtilities::GetInstance() != NULL;
}

// --------------------------------------------------------------------------
int vtkMathTextActor3D::GetBoundingBox(int bbox[])
{
  if (!this->TextProperty)
    {
    vtkErrorMacro(<<"Need valid vtkTextProperty.");
    return 0;
    }

  if (!bbox)
    {
    vtkErrorMacro(<<"Need 4-element int array for bounding box.");
    }

  vtkMathTextUtilities *mtu = vtkMathTextUtilities::GetInstance();
  if (!mtu)
    { // Use freetype fallback
    vtkFreeTypeUtilities *fu = vtkFreeTypeUtilities::GetInstance();
    if (!fu)
      {
      vtkErrorMacro(<<"Failed getting the FreeType utilities instance");
      return 0;
      }

    fu->GetBoundingBox(this->TextProperty, this->FallbackText ?
                         this->FallbackText : this->Input, bbox);
    if (!fu->IsBoundingBoxValid(bbox))
      {
      vtkErrorMacro(<<"Cannot determine bounding box of fallback text.");
      return 0;
      }
    }
  else
    {
    // Assume a 120 DPI output device
    if (mtu->GetBoundingBox(this->TextProperty, this->Input, 120, bbox) == 0)
      {
      vtkErrorMacro(<<"Cannot determine bounding box of input.");
      return 0;
      }
    }
  return 1;
}

// --------------------------------------------------------------------------
void vtkMathTextActor3D::ShallowCopy(vtkProp *prop)
{
  vtkMathTextActor3D *a = vtkMathTextActor3D::SafeDownCast(prop);
  if (a != NULL)
    {
    this->SetFallbackText(a->GetFallbackText());
    }

  this->Superclass::ShallowCopy(prop);
}

// --------------------------------------------------------------------------
int vtkMathTextActor3D::UpdateImageActor()
{
  // Need text prop
  if (!this->TextProperty)
    {
    vtkErrorMacro(<<"Need a text property to render text actor");
    return 0;
    }

  // No input, the assign the image actor a zilch input
  if (!this->Input || !*this->Input)
    {
    if (this->ImageActor)
      {
      this->ImageActor->SetInputData(0);
      }
    return 1;
    }

  // Do we need to (re-)render the text ?
  // Yes if:
  //  - instance has been modified since last build
  //  - text prop has been modified since last build
  //  - ImageData ivar has not been allocated yet
  if (this->GetMTime() > this->BuildTime ||
      this->TextProperty->GetMTime() > this->BuildTime ||
      !this->ImageData)
    {

    this->BuildTime.Modified();

    // Create the image data
    if (!this->ImageData)
      {
      this->ImageData = vtkImageData::New();
      this->ImageData->SetSpacing(1.0, 1.0, 1.0);
      }

    vtkMathTextUtilities *mtu = vtkMathTextUtilities::GetInstance();
    if (!mtu)
      { // Render fallback text
      vtkFreeTypeUtilities *fu = vtkFreeTypeUtilities::GetInstance();
      if (!fu)
        {
        vtkErrorMacro(<<"Failed getting the FreeType utilities instance");
        return 0;
        }

      if (!fu->RenderString(this->TextProperty, this->FallbackText ?
                            this->FallbackText : this->Input, this->ImageData))
        {
        vtkErrorMacro(<<"Failed rendering fallback text to buffer");
        return 0;
        }
      }
    else
      {
      if (!mtu->RenderString(this->Input, this->ImageData, this->TextProperty,
                             120))
        {
        vtkErrorMacro(<<"Failed rendering text to buffer");
        return 0;
        }
      }

    // Associate the image data (should be up to date now) to the image actor
    if (this->ImageActor)
      {
      this->ImageActor->SetInputData(this->ImageData);
      this->ImageActor->SetDisplayExtent(this->ImageData->GetExtent());
      }

    } // if (this->GetMTime() ...

  // Position the actor
  if (this->ImageActor)
    {
    vtkMatrix4x4 *matrix = this->ImageActor->GetUserMatrix();
    if (!matrix)
      {
      matrix = vtkMatrix4x4::New();
      this->ImageActor->SetUserMatrix(matrix);
      matrix->Delete();
      }
    this->GetMatrix(matrix);
    }

  return 1;
}

// --------------------------------------------------------------------------
void vtkMathTextActor3D::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  if (this->FallbackText)
    {
    os << indent << "FallbackText: " << this->FallbackText << "\n";
    }
  else
    {
    os << indent << "FallbackText: (none)\n";
    }
}
