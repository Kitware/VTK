/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMathTextActor.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkMathTextActor.h"

#include "vtkCellArray.h"
#include "vtkFloatArray.h"
#include "vtkImageData.h"
#include "vtkMath.h"
#include "vtkMathTextUtilities.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper2D.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkTextProperty.h"
#include "vtkTextRenderer.h"
#include "vtkTexture.h"
#include "vtkViewport.h"

// ----------------------------------------------------------------------------
vtkStandardNewMacro(vtkMathTextActor);

// ----------------------------------------------------------------------------
bool vtkMathTextActor::IsSupported()
{
  return vtkMathTextUtilities::GetInstance() != NULL;
}

// ----------------------------------------------------------------------------
vtkMathTextActor::vtkMathTextActor()
{
  this->FallbackText = NULL;
}

// ----------------------------------------------------------------------------
vtkMathTextActor::~vtkMathTextActor()
{
  this->SetFallbackText(NULL);
}

// ----------------------------------------------------------------------------
void vtkMathTextActor::ShallowCopy(vtkProp *prop)
{
  if (vtkMathTextActor *actor = vtkMathTextActor::SafeDownCast(prop))
    {
    this->SetFallbackText(actor->GetFallbackText());
    }
  this->Superclass::ShallowCopy(prop);
}

// ----------------------------------------------------------------------------
bool vtkMathTextActor::RenderImage(vtkTextProperty *tprop,
                                   vtkViewport *viewport)
{
  vtkMathTextUtilities* util = vtkMathTextUtilities::GetInstance();
  if (!util)
    { // Fall back to freetype rendering
    if (!this->TextRenderer->RenderString(
          tprop, this->FallbackText ? this->FallbackText : this->Input,
          this->ImageData, NULL, 120, vtkTextRenderer::MathText))
      {
      vtkErrorMacro(<<"Failed rendering fallback text to buffer");
      return false;
      }
    }
  else
    {
    unsigned int dpi = 120;
    vtkRenderer *ren = vtkRenderer::SafeDownCast(viewport);
    if (ren)
      {
      if (ren->GetRenderWindow())
        {
        dpi = static_cast<unsigned int>(ren->GetRenderWindow()->GetDPI());
        }
      }

    if (!util->RenderString(this->Input, this->ImageData, tprop, dpi))
      {
      vtkErrorMacro(<<"Failed rendering text to buffer");
      return false;
      }
    }

  return true;
}

// ----------------------------------------------------------------------------
bool vtkMathTextActor::GetImageBoundingBox(vtkTextProperty *tprop,
                                      vtkViewport *viewport, int bbox[4])
{
  vtkMathTextUtilities* util = vtkMathTextUtilities::GetInstance();
  if (!util)
    { // Fall back to freetype rendering
    if (!this->TextRenderer->GetBoundingBox(
          tprop, this->FallbackText ? this->FallbackText : this->Input,
          bbox, vtkTextRenderer::MathText))
      {
      vtkErrorMacro(<<"Failed rendering fallback text to buffer");
      return false;
      }
    }
  else
    {
    unsigned int dpi = 120;
    vtkRenderer *ren = vtkRenderer::SafeDownCast(viewport);
    if (ren)
      {
      if (ren->GetRenderWindow())
        {
        dpi = static_cast<unsigned int>(ren->GetRenderWindow()->GetDPI());
        }
      }

    if (!util->GetBoundingBox(tprop ,this->Input, dpi, bbox))
      {
      vtkErrorMacro(<<"Failed rendering text to buffer");
      return false;
      }
    }
  return true;
}

// ----------------------------------------------------------------------------
void vtkMathTextActor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  if (this->FallbackText)
    {
    os << indent << "FallbackText: " << this->FallbackText << endl;
    }
  else
    {
    os << indent << "FallbackText: (none)\n";
    }
}
