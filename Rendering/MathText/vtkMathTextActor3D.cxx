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
  this->Input = NULL;
  this->ImageActor->InterpolateOn();
  this->ImageActor->SetInputData(this->ImageData.GetPointer());

  vtkNew<vtkTextProperty> tprop;
  this->SetTextProperty(tprop.GetPointer());
}

// --------------------------------------------------------------------------
vtkMathTextActor3D::~vtkMathTextActor3D()
{
  this->SetInput(NULL);
}

// --------------------------------------------------------------------------
bool vtkMathTextActor3D::IsSupported()
{
  return vtkMathTextUtilities::GetInstance() != NULL;
}

// --------------------------------------------------------------------------
void vtkMathTextActor3D::ShallowCopy(vtkProp *prop)
{
  vtkMathTextActor3D *a = vtkMathTextActor3D::SafeDownCast(prop);
  if (a != NULL)
    {
    this->SetInput(a->GetInput());
    this->SetTextProperty(a->GetTextProperty());
    }

  this->Superclass::ShallowCopy(prop);
}

// --------------------------------------------------------------------------
void vtkMathTextActor3D::SetTextProperty(vtkTextProperty *p)
{
  this->TextProperty = p;
}

// --------------------------------------------------------------------------
vtkTextProperty *vtkMathTextActor3D::GetTextProperty()
{
  return this->TextProperty.GetPointer();
}

// --------------------------------------------------------------------------
double* vtkMathTextActor3D::GetBounds()
{
  // the culler could be asking our bounds, in which case it's possible
  // that we haven't rendered yet, so we have to make sure our bounds
  // are up to date so that we don't get culled.
  this->UpdateImageActor();
  return this->ImageActor->GetBounds();
}

// --------------------------------------------------------------------------
int *vtkMathTextActor3D::GetImageDimensions()
{
  this->UpdateImageActor();
  return this->ImageData->GetDimensions();
}

// --------------------------------------------------------------------------
void vtkMathTextActor3D::GetImageDimensions(int dims[])
{
  this->UpdateImageActor();
  this->ImageData->GetDimensions(dims);
}

// --------------------------------------------------------------------------
void vtkMathTextActor3D::ReleaseGraphicsResources(vtkWindow *win)
{
  this->ImageActor->ReleaseGraphicsResources(win);
  this->Superclass::ReleaseGraphicsResources(win);
}

// --------------------------------------------------------------------------
int vtkMathTextActor3D::RenderOverlay(vtkViewport *viewport)
{
  int renderedSomething = 0;

  if (this->UpdateImageActor())
    {
    renderedSomething += this->ImageActor->RenderOverlay(viewport);
    }

  return renderedSomething;
}

// ----------------------------------------------------------------------------
int vtkMathTextActor3D::RenderTranslucentPolygonalGeometry(vtkViewport *viewport)
{
  if (!this->Visibility)
    {
    return 0;
    }

  int renderedSomething = 0;

  if (this->UpdateImageActor())
    {
    renderedSomething +=
        this->ImageActor->RenderTranslucentPolygonalGeometry(viewport);
    }

  return renderedSomething;
}

//-----------------------------------------------------------------------------
int vtkMathTextActor3D::HasTranslucentPolygonalGeometry()
{
  int result = 0;

  if (this->UpdateImageActor())
    {
    result=this->ImageActor->HasTranslucentPolygonalGeometry();
    }

  return result;
}

// --------------------------------------------------------------------------
int vtkMathTextActor3D::RenderOpaqueGeometry(vtkViewport *viewport)
{
  if (!this->Visibility)
  {
    return 0;
  }

  // Is the viewport's RenderWindow capturing GL2PS-special props?
  if (vtkRenderer *ren = vtkRenderer::SafeDownCast(viewport))
    {
    if (vtkRenderWindow *renderWindow = ren->GetRenderWindow())
      {
      if (renderWindow->GetCapturingGL2PSSpecialProps())
        {
        renderWindow->CaptureGL2PSSpecialProp(this);
        }
      }
    }

  int renderedSomething = 0;

  if (this->UpdateImageActor())
    {
    renderedSomething +=
      this->ImageActor->RenderOpaqueGeometry(viewport);
    }

  return renderedSomething;
}

// --------------------------------------------------------------------------
int vtkMathTextActor3D::UpdateImageActor()
{
  if (this->TextProperty.GetPointer() == NULL)
    {
    vtkErrorMacro(<<"Need a text property to render text actor");
    return 0;
    }

  // Do we need to (re-)render the text ?
  // Yes if:
  //  - instance has been modified since last build
  //  - text prop has been modified since last build
  if (this->GetMTime() > this->ImageData->GetMTime() ||
      this->TextProperty->GetMTime() > this->ImageData->GetMTime())
    {

    vtkMathTextUtilities *mtu = vtkMathTextUtilities::GetInstance();
    if (!mtu)
      {
      vtkErrorMacro(<<"Failed getting a MathText utilities instance");
      return 0;
      }

    if (!mtu->RenderString(this->Input, this->ImageData.GetPointer(),
                           this->TextProperty.GetPointer(), 120))
      {
      vtkErrorMacro(<<"Failed rendering text to buffer");
      return 0;
      }

    this->ImageActor->SetDisplayExtent(this->ImageData->GetExtent());
    this->ImageActor->SetPosition(this->GetPosition());
    this->ImageActor->SetOrientation(this->GetOrientation());

    } // if (this->GetMTime() ...

  return 1;
}

// --------------------------------------------------------------------------
void vtkMathTextActor3D::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  if (this->TextProperty.GetPointer())
    {
    os << indent << "Text Property:\n";
    this->TextProperty->PrintSelf(os,indent.GetNextIndent());
    }
  else
    {
    os << indent << "Text Property: (none)\n";
    }

  os << indent << "Image Actor:\n";
  this->ImageActor->PrintSelf(os,indent.GetNextIndent());
  os << indent << "Image Data:\n";
  this->ImageData->PrintSelf(os,indent.GetNextIndent());

  if (this->Input)
    {
    os << indent << "Input:\n" << indent.GetNextIndent() << this->Input << "\n";
    }
  else
    {
    os << indent << "Input: (none)\n";
  }
}
