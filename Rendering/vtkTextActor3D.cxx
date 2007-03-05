/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTextActor3D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkTextActor3D.h"

#include "vtkObjectFactory.h"
#include "vtkCamera.h"
#include "vtkImageActor.h"
#include "vtkImageData.h"
#include "vtkTransform.h"
#include "vtkTextProperty.h"
#include "vtkRenderer.h"
#include "vtkWindow.h"
#include "vtkFreeTypeUtilities.h"
#include "vtkMatrix4x4.h"
#include "vtkMath.h"

vtkCxxRevisionMacro(vtkTextActor3D, "1.6");
vtkStandardNewMacro(vtkTextActor3D);

vtkCxxSetObjectMacro(vtkTextActor3D, TextProperty, vtkTextProperty);

// ----------------------------------------------------------------------------
vtkTextActor3D::vtkTextActor3D()
{
  this->Input        = NULL;
  this->ImageActor   = vtkImageActor::New();
  this->ImageData    = NULL;
  this->TextProperty = NULL;

  this->BuildTime.Modified();

  this->SetTextProperty(vtkTextProperty::New());
  this->TextProperty->Delete();

  this->ImageActor->InterpolateOn();
}

// --------------------------------------------------------------------------
vtkTextActor3D::~vtkTextActor3D()
{
  this->SetTextProperty(NULL);
  this->SetInput(NULL);

  if (this->ImageActor)
    {
    this->ImageActor->Delete();
    this->ImageActor = NULL;
    }

  if (this->ImageData)
    {
    this->ImageData->Delete();
    this->ImageData = NULL;
    }
}

// --------------------------------------------------------------------------
void vtkTextActor3D::ShallowCopy(vtkProp *prop)
{
  vtkTextActor3D *a = vtkTextActor3D::SafeDownCast(prop);
  if (a != NULL)
    {
    this->SetInput(a->GetInput());
    this->SetTextProperty(a->GetTextProperty());
    }

  this->Superclass::ShallowCopy(prop);
}

// --------------------------------------------------------------------------

double* vtkTextActor3D::GetBounds()
{
  if (this->ImageActor)
    {
    return this->ImageActor->GetBounds();
    }

  return NULL;
}

// --------------------------------------------------------------------------
void vtkTextActor3D::ReleaseGraphicsResources(vtkWindow *win)
{
  if (this->ImageActor)
    {
    this->ImageActor->ReleaseGraphicsResources(win); 
    }

  this->Superclass::ReleaseGraphicsResources(win);
}

// --------------------------------------------------------------------------
int vtkTextActor3D::RenderOverlay(vtkViewport *viewport)
{
  int rendered_something = 0;

  if (this->UpdateImageActor() && this->ImageActor)
    {
    rendered_something += this->ImageActor->RenderOverlay(viewport);
    }

  return rendered_something;
}

// ----------------------------------------------------------------------------
int vtkTextActor3D::RenderTranslucentPolygonalGeometry(vtkViewport *viewport)
{
  int rendered_something = 0;

  if (this->UpdateImageActor() && this->ImageActor)
    {
    rendered_something += 
      this->ImageActor->RenderTranslucentPolygonalGeometry(viewport);
    }

  return rendered_something;
}

//-----------------------------------------------------------------------------
// Description:
// Does this prop have some translucent polygonal geometry?
int vtkTextActor3D::HasTranslucentPolygonalGeometry()
{
  int result = 0;

  if (this->UpdateImageActor() && this->ImageActor)
    {
    result=this->ImageActor->HasTranslucentPolygonalGeometry();
    }

  return result;
}

// --------------------------------------------------------------------------
int vtkTextActor3D::RenderOpaqueGeometry(vtkViewport *viewport)
{
  int rendered_something = 0;

  if (this->UpdateImageActor() && this->ImageActor)
    {
    rendered_something += 
      this->ImageActor->RenderOpaqueGeometry(viewport);
    }

  return rendered_something;
}

// --------------------------------------------------------------------------
int vtkTextActor3D::UpdateImageActor()
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
      this->ImageActor->SetInput(NULL);
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
    //cout << "Need to re-render " << this->GetMTime() << ", " <<  this->TextProperty->GetMTime() << ", " << this->BuildTime << endl;

    this->BuildTime.Modified();

    // Get the bounding box of the text to render
    // If the bounding box is invalid, then there is nothing to render
    // assign the image actor a nada input

    vtkFreeTypeUtilities *fu = vtkFreeTypeUtilities::GetInstance();
    if (!fu)
      {
      vtkErrorMacro(<<"Failed getting the FreeType utilities instance");
      return 0;
      }

    int text_bbox[4];
    fu->GetBoundingBox(this->TextProperty, this->Input, text_bbox);
    if (!fu->IsBoundingBoxValid(text_bbox))
      {
      if (this->ImageActor)
        {
        this->ImageActor->SetInput(NULL);
        }
      return 1;
      }

    // The bounding box was the area that is going to be filled with pixels
    // given a text origin of (0, 0). Now get the real size we need, i.e.
    // the full extent from the origin to the bounding box.

    int text_size[2];
    text_size[0] = (text_bbox[1] - text_bbox[0] + 1) + abs(text_bbox[0]);
    text_size[1] = (text_bbox[3] - text_bbox[2] + 1) + abs(text_bbox[2]);

    // If the RGBA image data is too small, resize it to the next power of 2
    // WARNING: at this point, since this image is going to be a texture
    // we should limit its size or query the hardware

    if (!this->ImageData)
      {
      this->ImageData = vtkImageData::New();
      this->ImageData->SetScalarTypeToUnsignedChar();
      this->ImageData->SetNumberOfScalarComponents(4);
      this->ImageData->SetSpacing(1.0, 1.0, 1.0);
      }

    // If the current image data is too small to render the text,
    // or more than twice as big (too hungry), then resize

    int img_dims[3], new_img_dims[3];
    this->ImageData->GetDimensions(img_dims);

    if (img_dims[0] < text_size[0] || img_dims[1] < text_size[1] ||
        text_size[0] * 2 < img_dims[0] || text_size[1] * 2 < img_dims[0])
      {
      new_img_dims[0] = 1 << (int)ceil(log((double)text_size[0]) / log(2.0));
      new_img_dims[1] = 1 << (int)ceil(log((double)text_size[1]) / log(2.0));
      new_img_dims[2] = 1;
      if (new_img_dims[0] != img_dims[0] || 
          new_img_dims[1] != img_dims[1] ||
          new_img_dims[2] != img_dims[2])
        {
        //cout << "I need: " << text_size[0] << "x" << text_size[1] << ", I resize to: " << new_img_dims[0] << "x" << new_img_dims[1] << endl;
        this->ImageData->SetDimensions(new_img_dims);
        this->ImageData->AllocateScalars();
        this->ImageData->UpdateInformation();
        this->ImageData->SetUpdateExtent(this->ImageData->GetWholeExtent());
        this->ImageData->PropagateUpdateExtent();
        if (this->ImageActor)
          {
          this->ImageActor->SetDisplayExtent(
            this->ImageData->GetWholeExtent());
          }
        }
      }

    // Render inside the image data
    
    int x = (text_bbox[0] < 0 ? -text_bbox[0] : 0);
    int y = (text_bbox[2] < 0 ? -text_bbox[2] : 0);

#if 0
    char *ptr = (char*)this->ImageData->GetScalarPointer();
    int nb_comp = this->ImageData->GetNumberOfScalarComponents();
    char *ptr_end = ptr + this->ImageData->GetNumberOfPoints() * nb_comp;
    memset(ptr, 0, ptr_end - ptr);
    ptr += nb_comp - 1;
    while (ptr < ptr_end)
      {
      *ptr = 255.0;
      ptr += nb_comp;
      }
#else
    memset(this->ImageData->GetScalarPointer(), 
           0, 
           (this->ImageData->GetNumberOfPoints() *
            this->ImageData->GetNumberOfScalarComponents()));
#endif

    if (!fu->RenderString(
          this->TextProperty, this->Input, x, y, this->ImageData))
      {
      vtkErrorMacro(<<"Failed rendering text to buffer");
      return 0;
      }

    // Set the image data origin so that oriented text is placed properly
    // Also take shadow into account, we want the text to be on the baseline

    if (this->TextProperty->GetShadow())
      {
      // this is wrong
      //x += this->TextProperty->GetShadowOffset()[0];
      //y += this->TextProperty->GetShadowOffset()[1];
      }
    this->ImageData->SetOrigin(-x, -y, 0);

    // Associate the image data (should be up to date now) to the image actor

    if (this->ImageActor)
      {
      this->ImageActor->SetInput(this->ImageData);
      }
    }

  // Position the actor

  if (this->ImageActor)
    {
    // Which one is faster ?
#if 1
    vtkMatrix4x4 *matrix = vtkMatrix4x4::New();
    this->GetMatrix(matrix);
    this->ImageActor->SetUserMatrix(matrix);
    matrix->Delete();
#else
    this->ImageActor->SetPosition(this->GetPosition());
    this->ImageActor->SetScale(this->GetScale());
    this->ImageActor->SetOrientation(this->GetOrientation());
#endif
    }

  return 1;
}

// --------------------------------------------------------------------------
void vtkTextActor3D::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Input: " << (this->Input ? this->Input : "(none)") << "\n";

  if (this->TextProperty)
    {
    os << indent << "Text Property:\n";
    this->TextProperty->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << indent << "Text Property: (none)\n";
    }
}
