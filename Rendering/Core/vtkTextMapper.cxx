/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTextMapper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkTextMapper.h"

#include "vtkActor2D.h"
#include "vtkCellArray.h"
#include "vtkFloatArray.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper2D.h"
#include "vtkRenderer.h"
#include "vtkStdString.h"
#include "vtkTextProperty.h"
#include "vtkTextRenderer.h"
#include "vtkTexture.h"

#include <algorithm>

//----------------------------------------------------------------------------
vtkObjectFactoryNewMacro(vtkTextMapper)
//----------------------------------------------------------------------------
vtkCxxSetObjectMacro(vtkTextMapper,TextProperty,vtkTextProperty);

//----------------------------------------------------------------------------
// Creates a new text mapper
vtkTextMapper::vtkTextMapper()
{
  this->Input = NULL;
  this->TextProperty = NULL;

  vtkNew<vtkTextProperty> tprop;
  this->SetTextProperty(tprop.GetPointer());

  this->Points->SetNumberOfPoints(4);
  this->Points->SetPoint(0, 0., 0., 0.);
  this->Points->SetPoint(1, 0., 0., 0.);
  this->Points->SetPoint(2, 0., 0., 0.);
  this->Points->SetPoint(3, 0., 0., 0.);
  this->PolyData->SetPoints(this->Points.GetPointer());

  vtkNew<vtkCellArray> quad;
  quad->InsertNextCell(4);
  quad->InsertCellPoint(0);
  quad->InsertCellPoint(1);
  quad->InsertCellPoint(2);
  quad->InsertCellPoint(3);
  this->PolyData->SetPolys(quad.GetPointer());

  vtkNew<vtkFloatArray> tcoords;
  tcoords->SetNumberOfComponents(2);
  tcoords->SetNumberOfTuples(4);
  tcoords->SetTuple2(0, 0., 0.);
  tcoords->SetTuple2(1, 0., 0.);
  tcoords->SetTuple2(2, 0., 0.);
  tcoords->SetTuple2(3, 0., 0.);
  this->PolyData->GetPointData()->SetTCoords(tcoords.GetPointer());
  this->Mapper->SetInputData(this->PolyData.GetPointer());

  this->Texture->SetInputData(this->Image.GetPointer());
  this->TextDims[0] = this->TextDims[1] = 0;
}

//----------------------------------------------------------------------------
// Shallow copy of an actor.
void vtkTextMapper::ShallowCopy(vtkTextMapper *tm)
{
  this->SetInput(tm->GetInput());
  this->SetTextProperty(tm->GetTextProperty());

  this->SetClippingPlanes(tm->GetClippingPlanes());
}

//----------------------------------------------------------------------------
vtkTextMapper::~vtkTextMapper()
{
  delete [] this->Input;
  this->SetTextProperty(NULL);
}

//----------------------------------------------------------------------------
void vtkTextMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  if (this->TextProperty)
    {
    os << indent << "Text Property:\n";
    this->TextProperty->PrintSelf(os,indent.GetNextIndent());
    }
  else
    {
    os << indent << "Text Property: (none)\n";
    }

  os << indent << "Input: " << (this->Input ? this->Input : "(none)") << "\n";

  os << indent << "TextDims: "
     << this->TextDims[0] << ", " << this->TextDims[1] << "\n";

  os << indent << "CoordsTime: " << this->CoordsTime.GetMTime() << "\n";
  os << indent << "TCoordsTime: " << this->TCoordsTime.GetMTime() << "\n";
  os << indent << "Image:\n";
  this->Image->PrintSelf(os, indent.GetNextIndent());
  os << indent << "Points:\n";
  this->Points->PrintSelf(os, indent.GetNextIndent());
  os << indent << "PolyData:\n";
  this->PolyData->PrintSelf(os, indent.GetNextIndent());
  os << indent << "Mapper:\n";
  this->Mapper->PrintSelf(os, indent.GetNextIndent());
  os << indent << "Texture:\n";
  this->Texture->PrintSelf(os, indent.GetNextIndent());
}

//----------------------------------------------------------------------------
void vtkTextMapper::GetSize(vtkViewport *, int size[])
{
  UpdateImage();
  size[0] = this->TextDims[0];
  size[1] = this->TextDims[1];
}

//----------------------------------------------------------------------------
int vtkTextMapper::GetWidth(vtkViewport* viewport)
{
  int size[2];
  this->GetSize(viewport, size);
  return size[0];
}

//----------------------------------------------------------------------------
int vtkTextMapper::GetHeight(vtkViewport* viewport)
{
  int size[2];
  this->GetSize(viewport, size);
  return size[1];
}

//----------------------------------------------------------------------------
int vtkTextMapper::SetConstrainedFontSize(vtkViewport *viewport,
                                          int targetWidth,
                                          int targetHeight)
{
  return this->SetConstrainedFontSize(this, viewport, targetWidth, targetHeight);
}


//----------------------------------------------------------------------------
int vtkTextMapper::SetConstrainedFontSize(vtkTextMapper *tmapper, vtkViewport *viewport,
                                          int targetWidth, int targetHeight)
{
  // If target "empty" just return
  if (targetWidth == 0 && targetHeight == 0)
    {
    return 0;
    }

  vtkTextProperty *tprop = tmapper->GetTextProperty();
  if (!tprop)
    {
    vtkGenericWarningMacro(<<"Need text property to apply constraint");
    return 0;
    }
  int fontSize = tprop->GetFontSize();

  // Use the last size as a first guess
  int tempi[2];
  tmapper->GetSize(viewport, tempi);

  // Now get an estimate of the target font size using bissection
  // Based on experimentation with big and small font size increments,
  // ceil() gives the best result.
  // big:   floor: 10749, ceil: 10106, cast: 10749, vtkMath::Round: 10311
  // small: floor: 12122, ceil: 11770, cast: 12122, vtkMath::Round: 11768
  // I guess the best optim would be to have a look at the shape of the
  // font size growth curve (probably not that linear)
  if (tempi[0] && tempi[1])
    {
    float fx = targetWidth / static_cast<float>(tempi[0]);
    float fy = targetHeight / static_cast<float>(tempi[1]);
    fontSize = static_cast<int>(ceil(fontSize * ((fx <= fy) ? fx : fy)));
    tprop->SetFontSize(fontSize);
    tmapper->GetSize(viewport, tempi);
    }

  // While the size is too small increase it
  while (tempi[1] <= targetHeight &&
         tempi[0] <= targetWidth &&
         fontSize < 100)
    {
    fontSize++;
    tprop->SetFontSize(fontSize);
    tmapper->GetSize(viewport, tempi);
    }

  // While the size is too large decrease it
  while ((tempi[1] > targetHeight || tempi[0] > targetWidth)
         && fontSize > 0)
    {
    fontSize--;
    tprop->SetFontSize(fontSize);
    tmapper->GetSize(viewport, tempi);
    }

  return fontSize;
}

//----------------------------------------------------------------------------
int vtkTextMapper::SetMultipleConstrainedFontSize(vtkViewport *viewport,
                                                  int targetWidth,
                                                  int targetHeight,
                                                  vtkTextMapper **mappers,
                                                  int nbOfMappers,
                                                  int *maxResultingSize)
{
  maxResultingSize[0] = maxResultingSize[1] = 0;

  if (nbOfMappers == 0)
    {
    return 0;
    }

  int fontSize, aSize;

  // First try to find the constrained font size of the first mapper: it
  // will be used minimize the search for the remaining mappers, given the
  // fact that all mappers are likely to have the same constrained font size.
  int i, first;
  for (first = 0; first < nbOfMappers && !mappers[first]; first++) {}

  if (first >= nbOfMappers)
    {
    return 0;
    }

  fontSize = mappers[first]->SetConstrainedFontSize(
    viewport, targetWidth, targetHeight);

  // Find the constrained font size for the remaining mappers and
  // pick the smallest
  for (i = first + 1; i < nbOfMappers; i++)
    {
    if (mappers[i])
      {
      mappers[i]->GetTextProperty()->SetFontSize(fontSize);
      aSize = mappers[i]->SetConstrainedFontSize(
        viewport, targetWidth, targetHeight);
      if (aSize < fontSize)
        {
        fontSize = aSize;
        }
      }
    }

  // Assign the smallest size to all text mappers and find the largest area
  int tempi[2];
  for (i = first; i < nbOfMappers; i++)
    {
    if (mappers[i])
      {
      mappers[i]->GetTextProperty()->SetFontSize(fontSize);
      mappers[i]->GetSize(viewport, tempi);
      if (tempi[0] > maxResultingSize[0])
        {
        maxResultingSize[0] = tempi[0];
        }
      if (tempi[1] > maxResultingSize[1])
        {
        maxResultingSize[1] = tempi[1];
        }
      }
    }

  // The above code could be optimized further since the mappers
  // labels are likely to have the same height: in that case, we could
  // have searched for the largest label, find the constrained size
  // for this one, then applied this size to all others.  But who
  // knows, maybe one day the text property will support a text
  // orientation/rotation, and in that case the height will vary.
  return fontSize;
}


//----------------------------------------------------------------------------
int vtkTextMapper::SetRelativeFontSize(vtkTextMapper *tmapper,
                                       vtkViewport *viewport,  int *targetSize,
                                       int *stringSize, float sizeFactor)
{
  sizeFactor = (sizeFactor <= 0.0f ? 0.015f : sizeFactor);

  int fontSize, targetWidth, targetHeight;
  // Find the best size for the font
  targetWidth = targetSize[0] > targetSize[1] ? targetSize[0] : targetSize[1];
  targetHeight = static_cast<int>(sizeFactor * targetSize[0]
                                  + sizeFactor * targetSize[1]);

  fontSize = tmapper->SetConstrainedFontSize(tmapper, viewport, targetWidth, targetHeight);
  tmapper->GetSize(viewport, stringSize);

  return fontSize;
}

//----------------------------------------------------------------------------
int vtkTextMapper::SetMultipleRelativeFontSize(vtkViewport *viewport,
                                               vtkTextMapper **textMappers,
                                               int nbOfMappers, int *targetSize,
                                               int *stringSize, float sizeFactor)
{
  int fontSize, targetWidth, targetHeight;

  // Find the best size for the font
  // WARNING: check that the below values are in sync with the above
  // similar function.

  targetWidth = targetSize [0] > targetSize[1] ? targetSize[0] : targetSize[1];

  targetHeight = static_cast<int>(sizeFactor * targetSize[0]
                                  + sizeFactor * targetSize[1]);

  fontSize =
    vtkTextMapper::SetMultipleConstrainedFontSize(viewport,
                                                  targetWidth, targetHeight,
                                                  textMappers,
                                                  nbOfMappers,
                                                  stringSize);

  return fontSize;
}

//----------------------------------------------------------------------------
void vtkTextMapper::RenderOverlay(vtkViewport *viewport, vtkActor2D *actor)
{
  vtkDebugMacro(<<"RenderOverlay called");

  vtkRenderer *ren = NULL;
  if (this->Input && this->Input[0])
    {
    this->UpdateImage();
    this->UpdateQuad(actor);
    ren = vtkRenderer::SafeDownCast(viewport);
    if (ren)
      {
      vtkDebugMacro(<<"Texture::Render called");
      this->Texture->Render(ren);
      vtkInformation *info = actor->GetPropertyKeys();
      if (!info)
        {
        info = vtkInformation::New();
        actor->SetPropertyKeys(info);
        info->Delete();
        }
      info->Set(vtkProp::GeneralTextureUnit(),
        this->Texture->GetTextureUnit());
      }
    }

  vtkDebugMacro(<<"PolyData::RenderOverlay called");
  this->Mapper->RenderOverlay(viewport, actor);

  // clean up
  if (ren)
    {
    this->Texture->PostRender(ren);
    }

  vtkDebugMacro(<<"Superclass::RenderOverlay called");
  this->Superclass::RenderOverlay(viewport, actor);
}

//----------------------------------------------------------------------------
void vtkTextMapper::ReleaseGraphicsResources(vtkWindow *win)
{
  this->Superclass::ReleaseGraphicsResources(win);
  this->Texture->ReleaseGraphicsResources(win);
}

//----------------------------------------------------------------------------
unsigned long vtkTextMapper::GetMTime()
{
  unsigned long result = this->Superclass::GetMTime();
  result = std::max(result, this->CoordsTime.GetMTime());
  result = std::max(result, this->Image->GetMTime());
  result = std::max(result, this->Points->GetMTime());
  result = std::max(result, this->PolyData->GetMTime());
  result = std::max(result, this->Mapper->GetMTime());
  result = std::max(result, this->Texture->GetMTime());
  return result;
}

//----------------------------------------------------------------------------
// Determine the number of lines in the Input string (delimited by "\n").
#ifndef VTK_LEGACY_REMOVE
int vtkTextMapper::GetNumberOfLines(const char *input)
{
  VTK_LEGACY_BODY(vtkTextMapper::GetNumberOfLines, "VTK 6.0")
  if ( input == NULL || input[0] == '\0')
    {
    return 0;
    }

  int numLines=1;
  const char *ptr = input;

  while ( ptr != NULL )
    {
    if ( (ptr=strstr(ptr,"\n")) != NULL )
      {
      numLines++;
      ptr++; //skip over \n
      }
    }

  return numLines;
}
#endif // VTK_LEGACY_REMOVE

//------------------------------------------------------------------------------
namespace {
// Given an Actor2D position coordinate (viewport, bottom left corner of actor)
// and image dimensions, adjust the position to reflect the supplied alignment.
void AdjustOrigin(int hAlign, int vAlign, int origin[2], const int dims[2])
{
  switch (hAlign)
    {
    default:
    case VTK_TEXT_LEFT:
      break;
    case VTK_TEXT_CENTERED:
      origin[0] -= dims[0] / 2;
      break;
    case VTK_TEXT_RIGHT:
      origin[0] -= dims[0];
      break;
    }

  switch (vAlign)
    {
    default:
    case VTK_TEXT_TOP:
      origin[1] -= dims[1];
      break;
    case VTK_TEXT_CENTERED:
      origin[1] -= dims[1] / 2;
      break;
    case VTK_TEXT_BOTTOM:
      break;
    }
}
}

//----------------------------------------------------------------------------
void vtkTextMapper::UpdateQuad(vtkActor2D *actor)
{
  vtkDebugMacro(<<"UpdateQuad called");

  // Ensure that the image is up to date.
  UpdateImage();

  // Update texture coordinates:
  if (this->Image->GetMTime() > this->TCoordsTime)
    {
    int dims[3];
    this->Image->GetDimensions(dims);

    // Add a fudge factor to the texture coordinates to prevent the top
    // row of pixels from being truncated on some systems. The coordinates
    // are calculated to be centered on a texel and trim the padding from the
    // image. (padding is often added to create textures that have power-of-two
    // dimensions)
    float tw = static_cast<float>(this->TextDims[0]);
    float th = static_cast<float>(this->TextDims[1]);
    float iw = static_cast<float>(dims[0]);
    float ih = static_cast<float>(dims[1]);
    float tcXMin = 1.f / (2.f * iw);
    float tcYMin = 1.f / (2.f * ih);
    float tcXMax = std::min(1.0f,
                            (((2.f * tw - 1.f) / (2.f)) + 0.000001f) / iw);
    float tcYMax = std::min(1.0f,
                            (((2.f * th - 1.f) / (2.f)) + 0.000001f) / ih);
    if (vtkFloatArray *tc =
        vtkFloatArray::SafeDownCast(
          this->PolyData->GetPointData()->GetTCoords()))
      {
      vtkDebugMacro(<<"Setting tcoords: xmin, xmax, ymin, ymax: "
                    << tcXMin << ", " << tcXMax << ", "
                    << tcYMin << ", " << tcYMax);
      tc->Reset();
      tc->InsertNextValue(tcXMin);
      tc->InsertNextValue(tcYMin);

      tc->InsertNextValue(tcXMin);
      tc->InsertNextValue(tcYMax);

      tc->InsertNextValue(tcXMax);
      tc->InsertNextValue(tcYMax);

      tc->InsertNextValue(tcXMax);
      tc->InsertNextValue(tcYMin);

      this->TCoordsTime.Modified();
      }
    else
      {
      vtkErrorMacro(<<"Invalid texture coordinate array type.");
      }
    }

  if (this->CoordsTime < actor->GetMTime() ||
      this->CoordsTime < this->TextProperty->GetMTime())
    {
    int pos[2] = { 0, 0 };
    AdjustOrigin(this->TextProperty->GetJustification(),
                 this->TextProperty->GetVerticalJustification(),
                 pos, this->TextDims);

    double x = static_cast<double>(pos[0]);
    double y = static_cast<double>(pos[1]);
    double w = static_cast<double>(this->TextDims[0]);
    double h = static_cast<double>(this->TextDims[1]);

    this->Points->Reset();
    this->Points->InsertNextPoint(x, y, 0.);
    this->Points->InsertNextPoint(x, y + h, 0.);
    this->Points->InsertNextPoint(x + w, y + h, 0.);
    this->Points->InsertNextPoint(x + w, y, 0.);
    this->CoordsTime.Modified();
    }
}

//----------------------------------------------------------------------------
void vtkTextMapper::UpdateImage()
{
  vtkDebugMacro(<<"UpdateImage called");
  if (this->MTime > this->Image->GetMTime() ||
      this->TextProperty->GetMTime() > this->Image->GetMTime())
    {
    vtkTextRenderer *tren = vtkTextRenderer::GetInstance();
    if (tren)
      {
      if (!tren->RenderString(this->TextProperty, this->Input,
                              this->Image.GetPointer(), this->TextDims))
        {
        vtkErrorMacro(<<"Texture generation failed.");
        }
      vtkDebugMacro(<< "Text rendered to " << this->TextDims[0] << ", "
                    << this->TextDims[1] << " buffer.");
      }
    else
      {
      vtkErrorMacro(<<"Could not locate vtkTextRenderer object.");
      }
    }
}

#ifndef VTK_LEGACY_REMOVE
int vtkTextMapper::GetNumberOfLines()
{
  VTK_LEGACY_BODY(vtkTextMapper::GetNumberOfLines, "VTK 6.0")
  const char *input = this->Input;
  if ( input == NULL || input[0] == '\0')
    {
    return 0;
    }

  int numLines=1;
  const char *ptr = input;

  while ( ptr != NULL )
    {
    if ( (ptr=strstr(ptr,"\n")) != NULL )
      {
      numLines++;
      ptr++; //skip over \n
      }
    }

  return numLines;
}
#endif // VTK_LEGACY_REMOVE
