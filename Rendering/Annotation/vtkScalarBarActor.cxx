/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkScalarBarActor.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkScalarBarActor.h"
#include "vtkScalarBarActorInternal.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkColor.h"
#include "vtkColorTransferFunction.h"
#include "vtkCoordinate.h"
#include "vtkFloatArray.h"
#include "vtkImageData.h"
#include "vtkLookupTable.h"
#include "vtkMath.h"
#include "vtkMathTextUtilities.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper2D.h"
#include "vtkProperty2D.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkScalarsToColors.h"
#include "vtkSmartPointer.h"
#include "vtkTextActor.h"
#include "vtkTextProperty.h"
#include "vtkTexture.h"
#include "vtkTexturedActor2D.h"
#include "vtkViewport.h"
#include "vtkWindow.h"

#include <vector>
#include <set>
#include <map>

#include <cstdio> // for snprintf

#undef VTK_DBG_LAYOUT

vtkStandardNewMacro(vtkScalarBarActor);

vtkCxxSetObjectMacro(vtkScalarBarActor, LookupTable, vtkScalarsToColors);
vtkCxxSetObjectMacro(vtkScalarBarActor, AnnotationTextProperty, vtkTextProperty);
vtkCxxSetObjectMacro(vtkScalarBarActor, LabelTextProperty, vtkTextProperty);
vtkCxxSetObjectMacro(vtkScalarBarActor, TitleTextProperty, vtkTextProperty);
vtkCxxSetObjectMacro(vtkScalarBarActor, BackgroundProperty, vtkProperty2D);
vtkCxxSetObjectMacro(vtkScalarBarActor, FrameProperty, vtkProperty2D);

//----------------------------------------------------------------------------
// Instantiate object with 64 maximum colors; 5 labels; %%-#6.3g label
// format, no title, and vertical orientation. The initial scalar bar
// size is (0.05 x 0.8) of the viewport size.
vtkScalarBarActor::vtkScalarBarActor()
{
  this->P = new vtkScalarBarActorInternal;
  this->LookupTable = NULL;
  this->Position2Coordinate->SetValue(0.17, 0.8);

  this->PositionCoordinate->SetCoordinateSystemToNormalizedViewport();
  this->PositionCoordinate->SetValue(0.82, 0.1);

  this->TextPad = 1;
  this->TitleRatio = 0.5;
  this->BarRatio = 0.375;
  this->MaximumNumberOfColors = 64;
  this->NumberOfLabels = 5;
  this->NumberOfLabelsBuilt = 0;
  this->Orientation = VTK_ORIENT_VERTICAL;
  this->Title = NULL;
  this->ComponentTitle = NULL;
  this->VerticalTitleSeparation = 0;

  this->LabelTextProperty = vtkTextProperty::New();
  this->LabelTextProperty->SetFontSize(12);
  this->LabelTextProperty->SetBold(1);
  this->LabelTextProperty->SetItalic(1);
  this->LabelTextProperty->SetShadow(1);
  this->LabelTextProperty->SetFontFamilyToArial();

  this->AnnotationTextProperty = vtkTextProperty::New();
  this->AnnotationTextProperty->SetFontSize(12);
  this->AnnotationTextProperty->SetBold(1);
  this->AnnotationTextProperty->SetItalic(1);
  this->AnnotationTextProperty->SetShadow(1);
  this->AnnotationTextProperty->SetFontFamilyToArial();

  this->TitleTextProperty = vtkTextProperty::New();
  this->TitleTextProperty->ShallowCopy(this->LabelTextProperty);

  this->LabelFormat = new char[8];
  sprintf(this->LabelFormat, "%s", "%-#6.3g");

  this->TitleActor = vtkTextActor::New();
  this->TitleActor->GetPositionCoordinate()->
    SetReferenceCoordinate(this->PositionCoordinate);

  this->ScalarBar = vtkPolyData::New();
  this->ScalarBarMapper = vtkPolyDataMapper2D::New();
  this->ScalarBarMapper->SetInputData(this->ScalarBar);
  this->ScalarBarActor = vtkActor2D::New();
  this->ScalarBarActor->SetMapper(this->ScalarBarMapper);
  this->ScalarBarActor->GetPositionCoordinate()->
    SetReferenceCoordinate(this->PositionCoordinate);
  this->LastOrigin[0] = 0;
  this->LastOrigin[1] = 0;
  this->LastSize[0] = 0;
  this->LastSize[1] = 0;

  this->DrawAnnotations = 1;
  this->DrawNanAnnotation = 0;
  this->AnnotationTextScaling = 0;
  this->FixedAnnotationLeaderLineColor = 0;
  this->NanAnnotation = 0;
  this->SetNanAnnotation("NaN");
  this->P->NanSwatch = vtkPolyData::New();
  this->P->NanSwatchMapper = vtkPolyDataMapper2D::New();
  this->P->NanSwatchActor = vtkActor2D::New();
  this->P->NanSwatchMapper->SetInputData(this->P->NanSwatch);
  this->P->NanSwatchActor->SetMapper(this->P->NanSwatchMapper);
  this->P->NanSwatchActor->GetPositionCoordinate()->
    SetReferenceCoordinate(this->PositionCoordinate);

  this->DrawBelowRangeSwatch = false;
  this->BelowRangeAnnotation = 0;
  this->SetBelowRangeAnnotation("Below");
  this->P->BelowRangeSwatch = vtkPolyData::New();
  this->P->BelowRangeSwatchMapper = vtkPolyDataMapper2D::New();
  this->P->BelowRangeSwatchActor = vtkActor2D::New();
  this->P->BelowRangeSwatchMapper->SetInputData(this->P->BelowRangeSwatch);
  this->P->BelowRangeSwatchActor->SetMapper(this->P->BelowRangeSwatchMapper);
  this->P->BelowRangeSwatchActor->GetPositionCoordinate()->
    SetReferenceCoordinate(this->PositionCoordinate);

  this->DrawAboveRangeSwatch = false;
  this->AboveRangeAnnotation = 0;
  this->SetAboveRangeAnnotation("Above");
  this->P->AboveRangeSwatch = vtkPolyData::New();
  this->P->AboveRangeSwatchMapper = vtkPolyDataMapper2D::New();
  this->P->AboveRangeSwatchActor = vtkActor2D::New();
  this->P->AboveRangeSwatchMapper->SetInputData(this->P->AboveRangeSwatch);
  this->P->AboveRangeSwatchActor->SetMapper(this->P->AboveRangeSwatchMapper);
  this->P->AboveRangeSwatchActor->GetPositionCoordinate()->
    SetReferenceCoordinate(this->PositionCoordinate);

  this->AnnotationLeaderPadding = 8.;
  this->P->AnnotationBoxes = vtkPolyData::New();
  this->P->AnnotationBoxesMapper = vtkPolyDataMapper2D::New();
  this->P->AnnotationBoxesActor = vtkActor2D::New();
  this->P->AnnotationBoxesMapper->SetInputData(this->P->AnnotationBoxes);
  this->P->AnnotationBoxesActor->SetMapper(this->P->AnnotationBoxesMapper);
  this->P->AnnotationBoxesActor->GetPositionCoordinate()->
    SetReferenceCoordinate(this->PositionCoordinate);
  this->P->AnnotationLeaders = vtkPolyData::New();
  this->P->AnnotationLeadersMapper = vtkPolyDataMapper2D::New();
  this->P->AnnotationLeadersActor = vtkActor2D::New();
  this->P->AnnotationLeadersMapper->SetInputData(this->P->AnnotationLeaders);
  this->P->AnnotationLeadersActor->SetMapper(this->P->AnnotationLeadersMapper);
  this->P->AnnotationLeadersActor->GetPositionCoordinate()->
    SetReferenceCoordinate(this->PositionCoordinate);

  this->P->TitleBox.Posn[0] = 0;
  this->P->TitleBox.Posn[1] = 0;
  this->P->TitleBox.Size[0] = 0;
  this->P->TitleBox.Size[1] = 0;

  // If opacity is on, a jail like texture is displayed behind it..

  this->UseOpacity       = 0;
  this->TextureGridWidth = 10.0;

  this->TexturePolyData = vtkPolyData::New();
  vtkPolyDataMapper2D* textureMapper = vtkPolyDataMapper2D::New();
  textureMapper->SetInputData(this->TexturePolyData);
  this->TextureActor = vtkTexturedActor2D::New();
  this->TextureActor->SetMapper(textureMapper);
  textureMapper->Delete();
  this->TextureActor->GetPositionCoordinate()->
    SetReferenceCoordinate(this->PositionCoordinate);
  vtkFloatArray* tc = vtkFloatArray::New();
  tc->SetNumberOfComponents(2);
  tc->SetNumberOfTuples(4);
  tc->InsertComponent(0, 0, 0.0);
  tc->InsertComponent(0, 1, 0.0);
  tc->InsertComponent(1, 1, 0.0);
  tc->InsertComponent(3, 0, 0.0);
  this->TexturePolyData->GetPointData()->SetTCoords(tc);
  tc->Delete();

  vtkCellArray* polys2 = vtkCellArray::New();
  polys2->InsertNextCell(4);
  polys2->InsertCellPoint(0);
  polys2->InsertCellPoint(1);
  polys2->InsertCellPoint(2);
  polys2->InsertCellPoint(3);
  polys2->Delete();

  vtkProperty2D* imageProperty = vtkProperty2D::New();
  imageProperty->SetOpacity(0.08);
  this->TextureActor->SetProperty(imageProperty);
  imageProperty->Delete();

  // Create the default texture. Just a "Jail" like grid

  const unsigned int dim = 128;
  vtkImageData* image = vtkImageData::New();
  image->SetDimensions(dim, dim, 1);
  image->AllocateScalars(VTK_UNSIGNED_CHAR, 1);

  for (unsigned int y = 0; y < dim; y++)
  {
    unsigned char* ptr =
      static_cast<unsigned char*>(image->GetScalarPointer(0, y, 0));
    for (unsigned int x = 0; x < dim; x++)
    {
      *ptr = ((x == y) || (x == (dim - y - 1))) ? 255 : 0;
      ++ptr;
    }
  }

  this->Texture = vtkTexture::New();
  this->Texture->SetInputData(image);
  this->Texture->RepeatOn();
  this->TextureActor->SetTexture(this->Texture);
  image->Delete();

  // Default text position : Above scalar bar if orientation is horizontal
  //                         Right of scalar bar if orientation is vertical
  this->TextPosition = SucceedScalarBar;

  this->MaximumWidthInPixels = VTK_INT_MAX;
  this->MaximumHeightInPixels = VTK_INT_MAX;

  this->BackgroundProperty = vtkProperty2D::New();
  this->FrameProperty = vtkProperty2D::New();

  this->DrawBackground = 0;
  this->Background = vtkPolyData::New();
  this->BackgroundMapper = vtkPolyDataMapper2D::New();
  this->BackgroundMapper->SetInputData(this->Background);
  this->BackgroundActor = vtkActor2D::New();
  this->BackgroundActor->SetMapper(this->BackgroundMapper);
  this->BackgroundActor->GetPositionCoordinate()
    ->SetReferenceCoordinate(this->PositionCoordinate);

#ifdef VTK_DBG_LAYOUT
  this->DrawFrame = 1;
#else // VTK_DBG_LAYOUT
  this->DrawFrame = 0;
#endif // VTK_DBG_LAYOUT
  this->Frame = vtkPolyData::New();
  this->FrameMapper = vtkPolyDataMapper2D::New();
  this->FrameMapper->SetInputData(this->Frame);
  this->FrameActor = vtkActor2D::New();
  this->FrameActor->SetMapper(this->FrameMapper);
  this->FrameActor->GetPositionCoordinate()
    ->SetReferenceCoordinate(this->PositionCoordinate);

  this->DrawColorBar = 1;
  this->DrawTickLabels = 1;
  this->UnconstrainedFontSize = false;
}

//----------------------------------------------------------------------------
// Release any graphics resources that are being consumed by this actor.
// The parameter window could be used to determine which graphic
// resources to release.
void vtkScalarBarActor::ReleaseGraphicsResources(vtkWindow* win)
{
  this->TitleActor->ReleaseGraphicsResources(win);
  if (!this->P->TextActors.empty())
  {
    vtkScalarBarActorInternal::ActorVector::iterator it;
    for (
         it = this->P->TextActors.begin();
         it != this->P->TextActors.end();
         ++it)
    {
      (*it)->ReleaseGraphicsResources(win);
    }
  }
  for (size_t i = 0; i < this->P->AnnotationLabels.size(); ++i)
  {
    this->P->AnnotationLabels[i]->ReleaseGraphicsResources(win);
  }
  this->ScalarBarActor->ReleaseGraphicsResources(win);
  this->P->NanSwatchActor->ReleaseGraphicsResources(win);
  this->P->AboveRangeSwatchActor->ReleaseGraphicsResources(win);
  this->P->BelowRangeSwatchActor->ReleaseGraphicsResources(win);
  this->P->AnnotationBoxesActor->ReleaseGraphicsResources(win);
  this->P->AnnotationLeadersActor->ReleaseGraphicsResources(win);
  this->BackgroundActor->ReleaseGraphicsResources(win);
  this->FrameActor->ReleaseGraphicsResources(win);
  this->Texture->ReleaseGraphicsResources(win);
}

//----------------------------------------------------------------------------
void vtkScalarBarActor::GetScalarBarRect(int rect[4], vtkViewport* viewport)
{
  vtkCoordinate *origin = this->ScalarBarActor->GetPositionCoordinate();
  int * vpPos = origin->GetComputedViewportValue(viewport);
  rect[0] = vpPos[0];
  rect[1] = vpPos[1];

  double *bounds = this->ScalarBar->GetBounds();
  rect[0] += static_cast<int>(bounds[0] + 0.5);
  rect[1] += static_cast<int>(bounds[2] + 0.5);
  rect[2] = static_cast<int>(bounds[1] - bounds[0] + 0.5);
  rect[3] = static_cast<int>(bounds[3] - bounds[2] + 0.5);
}

//----------------------------------------------------------------------------
vtkScalarBarActor::~vtkScalarBarActor()
{
  delete [] this->LabelFormat;
  this->LabelFormat = NULL;

  this->TitleActor->Delete();
  this->SetNanAnnotation(NULL);
  this->SetBelowRangeAnnotation(NULL);
  this->SetAboveRangeAnnotation(NULL);

  this->ScalarBar->Delete();
  this->ScalarBarMapper->Delete();
  this->ScalarBarActor->Delete();

  this->P->NanSwatch->Delete();
  this->P->BelowRangeSwatch->Delete();
  this->P->AboveRangeSwatch->Delete();
  this->P->NanSwatchMapper->Delete();
  this->P->AboveRangeSwatchMapper->Delete();
  this->P->BelowRangeSwatchMapper->Delete();
  this->P->NanSwatchActor->Delete();
  this->P->AboveRangeSwatchActor->Delete();
  this->P->BelowRangeSwatchActor->Delete();

  this->P->AnnotationBoxes->Delete();
  this->P->AnnotationBoxesMapper->Delete();
  this->P->AnnotationBoxesActor->Delete();

  this->P->AnnotationLeaders->Delete();
  this->P->AnnotationLeadersMapper->Delete();
  this->P->AnnotationLeadersActor->Delete();

  delete [] this->Title;
  this->Title = NULL;

  delete [] this->ComponentTitle;
  this->ComponentTitle = NULL;

  this->SetLookupTable(NULL);
  this->SetAnnotationTextProperty(NULL);
  this->SetLabelTextProperty(NULL);
  this->SetTitleTextProperty(NULL);
  this->Texture->Delete();
  this->TextureActor->Delete();
  this->TexturePolyData->Delete();
  this->Background->Delete();
  this->BackgroundMapper->Delete();
  this->BackgroundActor->Delete();
  this->Frame->Delete();
  this->FrameMapper->Delete();
  this->FrameActor->Delete();
  this->SetBackgroundProperty(NULL);
  this->SetFrameProperty(NULL);
  delete this->P;
}

//----------------------------------------------------------------------------
int vtkScalarBarActor::RenderOverlay(vtkViewport* viewport)
{
  if (!this->RebuildLayoutIfNeeded(viewport))
  {
    return 0;
  }

  int renderedSomething = 0;

  // Is the viewport's RenderWindow capturing GL2PS-special props? We'll need
  // to handle this specially to get the texture to show up right.
  if (vtkRenderer *renderer = vtkRenderer::SafeDownCast(viewport))
  {
    if (vtkRenderWindow *renderWindow = renderer->GetRenderWindow())
    {
      if (renderWindow->GetCapturingGL2PSSpecialProps())
      {
        renderer->CaptureGL2PSSpecialProp(this);
      }
    }
  }

  // Everything is built, just have to render
  if (this->DrawBackground)
  {
    renderedSomething += this->BackgroundActor->RenderOverlay(viewport);
  }

  if (this->UseOpacity && this->DrawColorBar)
  {
    renderedSomething += this->TextureActor->RenderOverlay(viewport);
  }

  // Draw either the scalar bar (non-indexed mode) or
  // the annotated value boxes (indexed mode).
  if (!this->LookupTable->GetIndexedLookup())
  {
    if (this->DrawColorBar)
    {
      renderedSomething += this->ScalarBarActor->RenderOverlay(viewport);
    }

    if (this->DrawTickLabels)
    {
      vtkScalarBarActorInternal::ActorVector::iterator it;
      for (
           it = this->P->TextActors.begin();
           it != this->P->TextActors.end();
           ++it)
      {
        renderedSomething += (*it)->RenderOverlay(viewport);
      }
    }
  }
  else if (this->DrawColorBar)
  {
    renderedSomething +=
      this->P->AnnotationBoxesActor->RenderOverlay(viewport);
  }

  if (this->DrawNanAnnotation)
  {
    renderedSomething +=
      this->P->NanSwatchActor->RenderOverlay(viewport);
  }

  if (this->DrawBelowRangeSwatch)
  {
    renderedSomething +=
      this->P->BelowRangeSwatchActor->RenderOverlay(viewport);
  }

  if (this->DrawAboveRangeSwatch)
  {
    renderedSomething +=
      this->P->AboveRangeSwatchActor->RenderOverlay(viewport);
  }

  if (this->DrawFrame)
  {
    renderedSomething += this->FrameActor->RenderOverlay(viewport);
  }

  if (this->Title != NULL)
  {
    renderedSomething += this->TitleActor->RenderOverlay(viewport);
  }

  if (this->DrawAnnotations)
  {
    if (!this->P->AnnotationLabels.empty())
    {
      renderedSomething +=
        this->P->AnnotationLeadersActor->RenderOverlay(viewport);
      for (size_t i = 0; i < this->P->AnnotationLabels.size(); ++i)
      {
        renderedSomething +=
          this->P->AnnotationLabels[i]->RenderOverlay(viewport);
      }
    }
  }

  renderedSomething = (renderedSomething > 0)?(1):(0);
  return renderedSomething;
}

int vtkScalarBarActor::RebuildLayoutIfNeeded(vtkViewport* viewport)
{
  if (!this->LookupTable)
  {
    vtkWarningMacro(<< "Need a mapper to render a scalar bar");
    return 0;
  }

  if (!this->TitleTextProperty)
  {
    vtkErrorMacro(<< "Need title text property to render a scalar bar");
    return 0;
  }

  if (!this->LabelTextProperty)
  {
    vtkErrorMacro(<< "Need label text property to render a scalar bar");
    return 0;
  }

  if (!this->AnnotationTextProperty)
  {
    vtkErrorMacro(<< "Need annotation text property to render a scalar bar");
    return 0;
  }

  // Check to see whether we have to rebuild everything
  int positionsHaveChanged = 0;
  if (viewport->GetMTime() > this->BuildTime ||
      (viewport->GetVTKWindow() &&
        viewport->GetVTKWindow()->GetMTime() > this->BuildTime))
  {
    // if the viewport has changed we may - or may not need
    // to rebuild, it depends on if the projected coords chage
    int size[2];
    int* barOrigin;
    barOrigin = this->PositionCoordinate->GetComputedViewportValue(viewport);
    size[0] =
      this->Position2Coordinate->GetComputedViewportValue(viewport)[0] -
      barOrigin[0];
    size[1] =
      this->Position2Coordinate->GetComputedViewportValue(viewport)[1] -
      barOrigin[1];

    // Check if we have bounds on the maximum size
    size[0] = size[0] > this->MaximumWidthInPixels
      ? this->MaximumWidthInPixels : size[0];
    size[1] = size[1] > this->MaximumHeightInPixels
      ? this->MaximumHeightInPixels : size[1];

    if (this->LastSize[0] != size[0] ||
        this->LastSize[1] != size[1] ||
        this->LastOrigin[0] != barOrigin[0] ||
        this->LastOrigin[1] != barOrigin[1])
    {
      positionsHaveChanged = 1;
    }
  }

  // Check to see whether we have to rebuild everything
  if (positionsHaveChanged ||
      this->GetMTime() > this->BuildTime ||
      this->LookupTable->GetMTime() > this->BuildTime ||
      this->AnnotationTextProperty->GetMTime() > this->BuildTime ||
      this->LabelTextProperty->GetMTime() > this->BuildTime ||
      this->TitleTextProperty->GetMTime() > this->BuildTime ||
      this->BackgroundProperty->GetMTime() > this->BuildTime ||
      this->FrameProperty->GetMTime() > this->BuildTime)

  {
    this->RebuildLayout(viewport);
  }
  return 1;
}

//----------------------------------------------------------------------------
int vtkScalarBarActor::RenderOpaqueGeometry(vtkViewport* viewport)
{
  if (!this->RebuildLayoutIfNeeded(viewport))
  {
    return 0;
  }

  int renderedSomething = 0;

  // Everything is built, just have to render
  if (this->Title != NULL)
  {
    renderedSomething +=
      this->TitleActor->RenderOpaqueGeometry(viewport);
  }
  // Draw either the scalar bar (non-indexed mode) or
  // the annotated value boxes (indexed mode).
  if (!this->LookupTable->GetIndexedLookup())
  {
    if (this->DrawColorBar)
    {
      renderedSomething +=
        this->ScalarBarActor->RenderOpaqueGeometry(viewport);
    }
    vtkScalarBarActorInternal::ActorVector::iterator ait;
    for (
         ait = this->P->TextActors.begin();
         ait != this->P->TextActors.end();
         ++ait)
    {
      renderedSomething += (*ait)->RenderOpaqueGeometry(viewport);
    }
  }
  else
  {
    if (this->DrawColorBar)
    {
      renderedSomething +=
        this->P->AnnotationBoxesActor->RenderOpaqueGeometry(viewport);
    }
  }

  if (this->DrawNanAnnotation)
  {
    renderedSomething +=
      this->P->NanSwatchActor->RenderOpaqueGeometry(viewport);
  }

  if (this->DrawBelowRangeSwatch)
  {
    renderedSomething +=
      this->P->BelowRangeSwatchActor->RenderOpaqueGeometry(viewport);
  }

  if (this->DrawAboveRangeSwatch)
  {
    renderedSomething +=
      this->P->AboveRangeSwatchActor->RenderOpaqueGeometry(viewport);
  }

  // Draw the annotation leaders and labels
  if (this->DrawAnnotations)
  {
    if (!this->P->AnnotationLabels.empty())
    {
      renderedSomething +=
        this->P->AnnotationLeadersActor->RenderOpaqueGeometry(viewport);
      for (size_t i = 0; i < this->P->AnnotationLabels.size(); ++i)
      {
        renderedSomething +=
          this->P->AnnotationLabels[i]->RenderOpaqueGeometry(viewport);
      }
    }
  }

  renderedSomething = (renderedSomething > 0)?(1):(0);

  return renderedSomething;
}

//-----------------------------------------------------------------------------
// Description:
// Does this prop have some translucent polygonal geometry?
int vtkScalarBarActor::HasTranslucentPolygonalGeometry()
{ // TODO: Handle case when IndexedLookup is true and any colors in the palette
  // have an alpha value, as the color swatches drawn by
  // this->P->AnnotationBoxesActor have 1 translucent triangle for each
  // alpha-swatch.
  return 0;
}

//----------------------------------------------------------------------------
void vtkScalarBarActor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  if (this->LookupTable)
  {
    os << indent << "Lookup Table:\n";
    this->LookupTable->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << indent << "Lookup Table: (none)\n";
  }

  if (this->TitleTextProperty)
  {
    os << indent << "Title Text Property:\n";
    this->TitleTextProperty->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << indent << "Title Text Property: (none)\n";
  }

  if (this->LabelTextProperty)
  {
    os << indent << "Label Text Property:\n";
    this->LabelTextProperty->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << indent << "Label Text Property: (none)\n";
  }

  if (this->AnnotationTextProperty)
  {
    os << indent << "Annotation Text Property:\n";
    this->AnnotationTextProperty->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << indent << "Annotation Text Property: (none)\n";
  }

  os << indent << "Title: " << (this->Title ? this->Title : "(none)") << "\n";
  os << indent << "ComponentTitle: "
    << (this->ComponentTitle ? this->ComponentTitle : "(none)") << "\n";
  os << indent << "Maximum Number Of Colors: "
    << this->MaximumNumberOfColors << "\n";
  os << indent << "Number Of Labels: " << this->NumberOfLabels << "\n";
  os << indent << "Number Of Labels Built: " << this->NumberOfLabelsBuilt << "\n";

  os << indent << "Orientation: ";
  if (this->Orientation == VTK_ORIENT_HORIZONTAL)
  {
    os << "Horizontal\n";
  }
  else
  {
    os << "Vertical\n";
  }

  os << indent << "Label Format: " << this->LabelFormat << "\n";
  os << indent << "UseOpacity: " << this->UseOpacity << "\n";
  if (this->UseOpacity)
  {
    os << indent << "TextureGridWidth: " << this->TextureGridWidth << "\n";
    os << indent << "TextureActor:\n";
    this->TextureActor->PrintSelf(os, indent.GetNextIndent());
  }
  if (this->TextPosition == vtkScalarBarActor::PrecedeScalarBar)
  {
    os << indent << "TextPosition: PrecedeScalarBar\n";
  }
  else
  {
    os << indent << "TextPosition: SucceedScalarBar\n";
  }

  os << indent << "MaximumWidthInPixels: "
    << this->MaximumWidthInPixels << endl;
  os << indent << "MaximumHeightInPixels: "
    << this->MaximumHeightInPixels << endl;

  os << indent << "DrawAnnotations: "
    << this->DrawAnnotations << endl;
  os << indent << "DrawNanAnnotation: "
    << this->DrawNanAnnotation << endl;
  os << indent << "NanAnnotation: "
    << (this->NanAnnotation ? this->NanAnnotation : "(none)") << endl;
  os << indent << "AnnotationLeaderPadding: "
    << this->AnnotationLeaderPadding << endl;
  os << indent << "AnnotationTextScaling: "
    << this->AnnotationTextScaling << endl;
  os << indent << "VerticalTitleSeparation: "
    << VerticalTitleSeparation << endl;

  os << indent << "DrawBelowRangeSwatch: "
    << this->DrawBelowRangeSwatch << endl;
  os << indent << "BelowRangeAnnotation: "
    << (this->BelowRangeAnnotation ? this->BelowRangeAnnotation : "(none)") << endl;

  os << indent << "DrawAboveRangeSwatch: "
    << this->DrawAboveRangeSwatch << endl;
  os << indent << "AboveRangeAnnotation: "
    << (this->AboveRangeAnnotation ? this->AboveRangeAnnotation : "(none)") << endl;

  os << indent << "DrawBackground: " << this->DrawBackground << "\n";
  os << indent << "Background Property:\n";
  this->BackgroundProperty->PrintSelf(os, indent.GetNextIndent());
  os << indent << "DrawFrame: " << this->DrawFrame << "\n";
  os << indent << "Frame Property:\n";
  this->FrameProperty->PrintSelf(os, indent.GetNextIndent());
}

//----------------------------------------------------------------------------
void vtkScalarBarActor::ShallowCopy(vtkProp* prop)
{
  vtkScalarBarActor* a = vtkScalarBarActor::SafeDownCast(prop);
  if (a != NULL)
  {
    this->SetPosition2(a->GetPosition2());
    this->SetLookupTable(a->GetLookupTable());
    this->SetMaximumNumberOfColors(a->GetMaximumNumberOfColors());
    this->SetOrientation(a->GetOrientation());
    this->SetAnnotationTextProperty(a->GetAnnotationTextProperty());
    this->SetLabelTextProperty(a->GetLabelTextProperty());
    this->SetTitleTextProperty(a->GetTitleTextProperty());
    this->SetLabelFormat(a->GetLabelFormat());
    this->SetTitle(a->GetTitle());
    this->GetPositionCoordinate()->SetCoordinateSystem(
      a->GetPositionCoordinate()->GetCoordinateSystem());
    this->GetPositionCoordinate()->SetValue(
      a->GetPositionCoordinate()->GetValue());
    this->GetPosition2Coordinate()->SetCoordinateSystem(
      a->GetPosition2Coordinate()->GetCoordinateSystem());
    this->GetPosition2Coordinate()->SetValue(
      a->GetPosition2Coordinate()->GetValue());
    this->SetDrawBackground(a->GetDrawBackground());
    this->SetBackgroundProperty(a->GetBackgroundProperty());
    this->SetDrawFrame(a->GetDrawFrame());
    this->SetFrameProperty(a->GetFrameProperty());
  }

  // Now do superclass
  this->vtkActor2D::ShallowCopy(prop);
}

//----------------------------------------------------------------------------
void vtkScalarBarActor::RebuildLayout(vtkViewport* viewport)
{
  vtkDebugMacro(<< "Rebuilding subobjects");

  this->P->Viewport = viewport;
  this->FreeLayoutStorage();

  // Permute indices used to measure width and height
  // so that thickness and length according to the orientation
  // of the scalar bar instead.
  if (this->Orientation == VTK_ORIENT_VERTICAL)
  {
    this->P->TL[0] = 0;
    this->P->TL[1] = 1;
  }
  else
  {
    this->P->TL[0] = 1;
    this->P->TL[1] = 0;
  }
  this->P->NumNotes = this->LookupTable->GetNumberOfAnnotatedValues();

  // Warning: The order of these calls is extremely important
  // as each updates members of this->P used by later methods!
  this->ComputeFrame();
  this->ComputeScalarBarThickness();
  this->ComputeSwatchPad();
  this->LayoutNanSwatch();
  this->LayoutBelowRangeSwatch();
  this->LayoutAboveRangeSwatch();

  this->PrepareTitleText();
  this->LayoutTitle();
  this->ComputeScalarBarLength();

  this->LayoutAboveRangeSwatchPosn();

  this->LayoutTicks();

  this->LayoutAnnotations();

  if (this->UnconstrainedFontSize)
  {
    this->LayoutForUnconstrainedFont();
  }

  // Now generate/configure the VTK datasets and actors that
  // illustrate the scalar bar when rendered using the
  // layout computed above.
  this->ConfigureAnnotations();
  this->ConfigureFrame();
  this->ConfigureScalarBar();
  this->ConfigureTitle();
  this->ConfigureTicks();
  this->ConfigureNanSwatch();
  this->ConfigureAboveBelowRangeSwatch(false);
  this->ConfigureAboveBelowRangeSwatch(true);
#ifdef VTK_DBG_LAYOUT
  this->DrawBoxes();
#endif // VTK_DBG_LAYOUT
  this->BuildTime.Modified();
}

namespace
{
void AddBox(vtkPoints* pts, vtkCellArray* lines, vtkScalarBarBox& box, int tl[2])
{
  vtkIdType pid[5];
  pid[0] = pts->InsertNextPoint(box.Posn[0], box.Posn[1], 0.);
  pid[1] = pts->InsertNextPoint(box.Posn[0] + box.Size[tl[0]], box.Posn[1], 0.);
  pid[2] = pts->InsertNextPoint(box.Posn[0] + box.Size[tl[0]], box.Posn[1] +
    box.Size[tl[1]], 0.);
  pid[3] = pts->InsertNextPoint(box.Posn[0], box.Posn[1] + box.Size[tl[1]], 0.);

  pid[4] = pid[0];
  for (int i = 0; i < 4; ++i)
  {
    lines->InsertNextCell(2, pid + i);
  }
}
}

void vtkScalarBarActor::DrawBoxes()
{
  vtkPoints* pts = this->Frame->GetPoints();
  vtkCellArray* lines = this->Frame->GetLines();

  AddBox(pts, lines, this->P->ScalarBarBox, this->P->TL);
  AddBox(pts, lines, this->P->NanBox, this->P->TL);
  AddBox(pts, lines, this->P->TitleBox, this->P->TL);
  if (this->NumberOfLabels > 0)
  {
    AddBox(pts, lines, this->P->TickBox, this->P->TL);
  }
}

//----------------------------------------------------------------------------
void vtkScalarBarActor::ComputeFrame()
{
  // get the viewport size in display coordinates
  int* p0;
  int* p1;
  int size[2];
  p0 = this->PositionCoordinate->GetComputedViewportValue(this->P->Viewport);
  p1 = this->Position2Coordinate->GetComputedViewportValue(this->P->Viewport);
  for (int i = 0; i < 2; ++i)
  {
    //this->P->Frame.Posn[i] = p0[i];
    this->P->Frame.Posn[i] = 0; // Translate the frame's coordinate system to p0
    size[i] = p1[i] - p0[i];
  }

  // Check if we have bounds on the maximum size
  size[0] = size[0] > this->MaximumWidthInPixels
    ? this->MaximumWidthInPixels : size[0];
  size[1] = size[1] > this->MaximumHeightInPixels
    ? this->MaximumHeightInPixels : size[1];

  for (int i = 0; i < 2; ++i)
  {
    this->P->Frame.Size[i] = size[this->P->TL[i]];
  }

  this->LastOrigin[0] = p0[0];
  this->LastOrigin[1] = p0[1];
  this->LastSize[0] = size[0];
  this->LastSize[1] = size[1];
}

//----------------------------------------------------------------------------
void vtkScalarBarActor::ComputeScalarBarThickness()
{
  // We do not set Size[1] (length), since the title bounds may encroach
  // on it in the vertical orientation.
  this->P->ScalarBarBox.Size[0] =
    static_cast<int>(ceil(this->P->Frame.Size[0] * this->BarRatio));

  // The lower-left corner of the scalar bar may be estimated here
  // as identical to the Frame position in one or both coordinates,
  // depending on whether tick marks should precede the scalar bar or
  // not and on the orientation.
  //
  // It will be altered later in this->LayoutTicks to account
  // for the half-height/width of tick labels.
  this->P->ScalarBarBox.Posn = this->P->Frame.Posn;
  if (this->TextPosition == PrecedeScalarBar)
  {
    this->P->ScalarBarBox.Posn[this->P->TL[0]] +=
      this->P->Frame.Size[0] - this->P->ScalarBarBox.Size[0];
  }

  // Now knock the thickness down and nudge the bar so the bar doesn't hug the frame.
  double nudge = this->P->ScalarBarBox.Size[0] / 8.;
  if (nudge > this->TextPad)
  {
    nudge = this->TextPad;
  }
  this->P->ScalarBarBox.Size[0] =
    static_cast<int>(this->P->ScalarBarBox.Size[0] - nudge);
  this->P->ScalarBarBox.Posn[this->P->TL[0]] =
    static_cast<int>(this->P->ScalarBarBox.Posn[this->P->TL[0]] +
      (nudge * (this->TextPosition == PrecedeScalarBar ? -1 : 1)));
}

//----------------------------------------------------------------------------
void vtkScalarBarActor::ComputeSwatchPad()
{
  if (this->P->NumNotes)
  {
    this->P->SwatchPad = this->P->Frame.Size[1] / this->P->NumNotes > 16. ?
      4. : (this->P->Frame.Size[1] / this->P->NumNotes / 4.);
  }
  else
  {
    this->P->SwatchPad = 4.;
  }
}

//----------------------------------------------------------------------------
void vtkScalarBarActor::LayoutNanSwatch()
{
  // We don't have ScalarBarBox.Size[1] set yet; use the frame width instead.
  this->P->NanSwatchSize = static_cast<double>(
    this->P->ScalarBarBox.Size[0] > this->P->Frame.Size[1] / 4 ?
    this->P->Frame.Size[1] / 4 : this->P->ScalarBarBox.Size[0]);
  if (this->P->NanSwatchSize < 4 && this->P->Frame.Size[1] > 16)
  {
    this->P->NanSwatchSize = 4;
  }
  if (!this->DrawNanAnnotation)
  {
    this->P->NanSwatchSize = 0;
  }

  if (this->Orientation == VTK_ORIENT_VERTICAL)
  {
    this->P->NanBox.Posn[0] = this->P->ScalarBarBox.Posn[0];
    this->P->NanBox.Posn[1] = this->P->Frame.Posn[1] + this->TextPad;
    this->P->ScalarBarBox.Posn[1] = static_cast<int>(this->P->ScalarBarBox.Posn[1] +
      (this->P->NanSwatchSize + this->P->SwatchPad));
  }
  else // HORIZONTAL
  {
    this->P->NanBox.Posn = this->P->ScalarBarBox.Posn;
    this->P->NanBox.Posn[this->P->TL[1]] =
      static_cast<int>(this->P->NanBox.Posn[this->P->TL[1]] +
        (this->P->Frame.Size[1] - this->P->NanSwatchSize));
  }
  this->P->NanBox.Size[0] = this->P->ScalarBarBox.Size[0];
  this->P->NanBox.Size[1] = static_cast<int>(this->P->NanSwatchSize);
  if (this->P->NanBox.Size[1] > 2 * this->TextPad)
  {
    this->P->NanBox.Size[1] -= this->TextPad;
  }
}

//----------------------------------------------------------------------------
void vtkScalarBarActor::LayoutBelowRangeSwatch()
{
  // We don't have ScalarBarBox.Size[1] set yet; use the frame width instead.
  this->P->BelowRangeSwatchSize = static_cast<double>(
    this->P->ScalarBarBox.Size[0] > this->P->Frame.Size[1] / 4 ?
    this->P->Frame.Size[1] / 4 : this->P->ScalarBarBox.Size[0]);
  if (this->P->BelowRangeSwatchSize < 4 && this->P->Frame.Size[1] > 16)
  {
    this->P->BelowRangeSwatchSize = 4;
  }
  if (!this->DrawBelowRangeSwatch)
  {
    this->P->BelowRangeSwatchSize = 0;
  }

  if (this->Orientation == VTK_ORIENT_VERTICAL)
  {
    this->P->BelowRangeSwatchBox.Posn[0] = this->P->ScalarBarBox.Posn[0];
    this->P->BelowRangeSwatchBox.Posn[1] = this->P->Frame.Posn[1] + this->TextPad;

    // move away because of nan annotation
    if (this->DrawNanAnnotation)
    {
      this->P->BelowRangeSwatchBox.Posn[1] =
        static_cast<int>(this->P->BelowRangeSwatchBox.Posn[1] +
          (this->P->NanBox.Size[1] + this->P->SwatchPad));
    }
    this->P->ScalarBarBox.Posn[1] = static_cast<int>(this->P->ScalarBarBox.Posn[1] +
      this->P->BelowRangeSwatchSize);
  }
  else // HORIZONTAL
  {
    this->P->BelowRangeSwatchBox.Posn = this->P->ScalarBarBox.Posn;
  }

  this->P->BelowRangeSwatchBox.Size[0] = this->P->ScalarBarBox.Size[0];
  this->P->BelowRangeSwatchBox.Size[1] =
    static_cast<int>(this->P->BelowRangeSwatchSize);
  if (this->P->BelowRangeSwatchBox.Size[1] > 2 * this->TextPad)
  {
    this->P->BelowRangeSwatchBox.Size[1] -= this->TextPad;
  }
}

//----------------------------------------------------------------------------
void vtkScalarBarActor::LayoutAboveRangeSwatch()
{
  // We don't have ScalarBarBox.Size[1] set yet; use the frame width instead.
  this->P->AboveRangeSwatchSize = static_cast<double>(
    this->P->ScalarBarBox.Size[0] > this->P->Frame.Size[1] / 4 ?
    this->P->Frame.Size[1] / 4 : this->P->ScalarBarBox.Size[0]);
  if (this->P->AboveRangeSwatchSize < 4 && this->P->Frame.Size[1] > 16)
  {
    this->P->AboveRangeSwatchSize = 4;
  }
  if (!this->DrawAboveRangeSwatch)
  {
    this->P->AboveRangeSwatchSize = 0;
  }
}

//----------------------------------------------------------------------------
void vtkScalarBarActor::LayoutAboveRangeSwatchPosn()
{
  if (this->Orientation == VTK_ORIENT_VERTICAL)
  {
    this->P->AboveRangeSwatchBox.Posn[0] = this->P->ScalarBarBox.Posn[0];
    this->P->AboveRangeSwatchBox.Posn[1] =
      this->P->Frame.Posn[1] + this->TextPad + this->P->ScalarBarBox.Size[1] +
      this->P->SwatchPad;

    if (this->DrawNanAnnotation)
    {
      this->P->AboveRangeSwatchBox.Posn[1] =
        static_cast<int>(this->P->AboveRangeSwatchBox.Posn[1] +
                         this->P->SwatchPad +
                         this->P->NanBox.Size[1]);
    }

    if (this->DrawBelowRangeSwatch)
    {
      this->P->AboveRangeSwatchBox.Posn[1] =
        static_cast<int>(this->P->AboveRangeSwatchBox.Posn[1] +
                         this->P->SwatchPad +
                         this->P->BelowRangeSwatchBox.Size[1]);
    }
  }
  else // HORIZONTAL
  {
    this->P->AboveRangeSwatchBox.Posn = this->P->ScalarBarBox.Posn;
    this->P->AboveRangeSwatchBox.Posn[this->P->TL[1]] =
      static_cast<int>(this->P->AboveRangeSwatchBox.Posn[this->P->TL[1]] +
        (this->P->Frame.Size[1] - this->P->AboveRangeSwatchSize));

    if (this->DrawNanAnnotation)
    {
      this->P->AboveRangeSwatchBox.Posn[this->P->TL[1]] =
        static_cast<int>(this->P->AboveRangeSwatchBox.Posn[this->P->TL[1]] -
          (this->P->NanBox.Size[this->P->TL[1]] + this->P->SwatchPad));
    }
  }
  this->P->AboveRangeSwatchBox.Size[0] = this->P->ScalarBarBox.Size[0];
  this->P->AboveRangeSwatchBox.Size[1] =
    static_cast<int>(this->P->AboveRangeSwatchSize);
  if (this->P->AboveRangeSwatchBox.Size[1] > 2 * this->TextPad)
  {
    this->P->AboveRangeSwatchBox.Size[1] -= this->TextPad;
  }
}

//----------------------------------------------------------------------------
void vtkScalarBarActor::PrepareTitleText()
{
  // Update actor with the latest title/subtitle
  if (this->ComponentTitle && strlen(this->ComponentTitle) > 0)
  {
    //need to account for a space between title & component and null term
    char* combinedTitle =
      new char[strlen(this->Title) + strlen(this->ComponentTitle) + 2];
    strcpy(combinedTitle, this->Title);
    strcat(combinedTitle, " ");
    strcat(combinedTitle, this->ComponentTitle);
    this->TitleActor->SetInput(combinedTitle);
    delete [] combinedTitle;
  }
  else
  {
    this->TitleActor->SetInput(this->Title);
  }

  if (this->TitleTextProperty->GetMTime() > this->BuildTime)
  {
    // Shallow copy here so that the size of the title prop is not affected
    // by the automatic adjustment of its text mapper's size (i.e. its
    // mapper's text property is identical except for the font size
    // which will be modified later). This allows text actors to
    // share the same text property, and in that case specifically allows
    // the title and label text prop to be the same.
    this->TitleActor->GetTextProperty()->ShallowCopy(this->TitleTextProperty);
    this->TitleActor->GetTextProperty()->SetJustificationToCentered();
    this->TitleActor->GetTextProperty()->SetVerticalJustification(
      this->TextPosition == PrecedeScalarBar ? VTK_TEXT_BOTTOM : VTK_TEXT_TOP);
  }
}

//----------------------------------------------------------------------------
void vtkScalarBarActor::LayoutTitle()
{
  if (this->Title == NULL || !strlen(this->Title))
  {
    this->P->TitleBox.Posn = vtkTuple<int, 2>(0.);
    this->P->TitleBox.Size = vtkTuple<int, 2>(0.);
    return;
  }

  int targetWidth, targetHeight;
  // Title spans entire width of frame at top, regardless of orientation.
  targetWidth = static_cast<int>(this->P->Frame.Size[this->P->TL[0]]) - 2 *
    this->TextPad;
  // Height is either: at most half the frame height or
  // a fixed portion of the frame remaining after subtracting the
  // scalar bar's thickness.
  //
  // When laid out horizontally, ticks share vertical space with title.
  // We want the title to be larger (18pt vs 14pt).
  targetHeight = static_cast<int>((
    this->Orientation == VTK_ORIENT_VERTICAL ||
    this->LookupTable->GetIndexedLookup()) ?
      ceil(this->P->Frame.Size[this->P->TL[1]] / 2. - this->TextPad) :
      (this->P->Frame.Size[0] - this->P->ScalarBarBox.Size[0] -
       (this->TextPosition == SucceedScalarBar ?
         this->P->ScalarBarBox.Posn[this->P->TL[0]] : 0) - this->TextPad) *
      this->TitleRatio);

  if (this->UnconstrainedFontSize)
  {
    this->TitleActor->GetTextProperty()->SetFontSize(
      this->TitleTextProperty->GetFontSize());
  }
  else
  {
    this->TitleActor->SetConstrainedFontSize(
      this->P->Viewport, targetWidth, targetHeight);
  }

  // Now fetch the actual size from the actor and use it to
  // update the box size and position.
  double titleSize[2] = {0, 0};
  this->TitleActor->GetSize(this->P->Viewport, titleSize);
  this->TitleActor->GetTextProperty()->SetVerticalJustificationToTop();
  for (int i = 0; i < 2; ++i)
  {
    this->P->TitleBox.Size[this->P->TL[i]] =
      static_cast<int>(ceil(titleSize[i]));
  }

  this->P->TitleBox.Posn[0] =
    this->P->Frame.Posn[0] +
    (this->P->Frame.Size[this->P->TL[0]] - titleSize[0]) / 2;
  this->P->TitleBox.Posn[1] =
    static_cast<int>(this->P->Frame.Posn[1] + this->P->Frame.Size[this->P->TL[1]]);
  if (
      this->Orientation == VTK_ORIENT_VERTICAL ||
      this->TextPosition == vtkScalarBarActor::SucceedScalarBar)
  {
    this->P->TitleBox.Posn[1] -= this->P->TitleBox.Size[this->P->TL[1]] +
      this->TextPad + static_cast<int>(this->FrameProperty->GetLineWidth());
  }
  else
  {
    this->P->TitleBox.Posn[1] = this->P->Frame.Posn[1] + this->TextPad -
      static_cast<int>(this->FrameProperty->GetLineWidth());
  }
}

//-----------------------------------------------------------------------------
void vtkScalarBarActor::ComputeScalarBarLength()
{
  this->P->ScalarBarBox.Size[1] =
    this->Orientation == VTK_ORIENT_VERTICAL ?
    this->P->Frame.Size[1] - this->P->TitleBox.Size[1] -
    this->VerticalTitleSeparation :
    this->P->Frame.Size[1];

  // The scalar bar does not include the Nan Swatch, the Below Range Swatch and
  // the Above Range Swatch.
  this->P->ScalarBarBox.Size[1] = static_cast<int>(this->P->ScalarBarBox.Size[1] -
    (this->P->NanSwatchSize + this->P->SwatchPad));

  // Correct Swatch behevior while keeping compat with images from tests
  if (this->P->BelowRangeSwatchSize > 0)
  {
    this->P->ScalarBarBox.Size[1] -=
      this->P->BelowRangeSwatchSize + this->P->SwatchPad;
  }

  if (this->P->AboveRangeSwatchSize > 0)
  {
    this->P->ScalarBarBox.Size[1] -= this->P->AboveRangeSwatchSize;
    if (this->P->NanSwatchSize > 0)
    {
      this->P->ScalarBarBox.Size[1] -= this->P->SwatchPad;
    }
  }
}

//-----------------------------------------------------------------------------
void vtkScalarBarActor::LayoutTicks()
{
  if (this->LookupTable->GetIndexedLookup())
  { // no tick marks in indexed lookup mode.
    this->NumberOfLabelsBuilt = 0;
    return;
  }

  // find the best size for the ticks
  const double* range = this->LookupTable->GetRange();
  char string[512];
  double val;
  int i;

  // TODO: this should be optimized, maybe by keeping a list of
  // allocated mappers, in order to avoid creation/destruction of
  // their underlying text properties (i.e. each time a mapper is
  // created, text properties are created and shallow-assigned a font size
  // which value might be "far" from the target font size).
  this->P->TextActors.resize(this->NumberOfLabels);

  // Does this map have its scale set to log?
  int isLogTable = this->LookupTable->UsingLogScale();

  for (i = 0; i < this->NumberOfLabels; i++)
  {
    this->P->TextActors[i].TakeReference(vtkTextActor::New());

    if (isLogTable)
    {
      double lval;
      if (this->NumberOfLabels > 1)
      {
        lval = log10(range[0]) +
          static_cast<double>(i) / (this->NumberOfLabels - 1) *
          (log10(range[1]) - log10(range[0]));
      }
      else
      {
        lval = log10(range[0]) + 0.5 * (log10(range[1]) - log10(range[0]));
      }
      val = pow(10.0, lval);
    }
    else
    {
      if (this->NumberOfLabels > 1)
      {
        val = range[0] +
          static_cast<double>(i) / (this->NumberOfLabels - 1)
          * (range[1] - range[0]);
      }
      else
      {
        val = range[0] + 0.5 * (range[1] - range[0]);
      }
    }

    snprintf(string, 511, this->LabelFormat, val);
    this->P->TextActors[i]->SetInput(string);

    // Shallow copy here so that the size of the label prop is not affected
    // by the automatic adjustment of its text mapper's size (i.e. its
    // mapper's text property is identical except for the font size
    // which will be modified later). This allows text actors to
    // share the same text property, and in that case specifically allows
    // the title and label text prop to be the same.
    this->P->TextActors[i]->GetTextProperty()->ShallowCopy(
      this->LabelTextProperty);

    this->P->TextActors[i]->SetProperty(this->GetProperty());
    this->P->TextActors[i]->GetPositionCoordinate()->
      SetReferenceCoordinate(this->PositionCoordinate);
  }

  if (this->NumberOfLabels)
  {
    int labelSize[2];
    labelSize[0] = labelSize[1] = 0;
    int targetWidth, targetHeight;

    this->P->TickBox.Posn = this->P->ScalarBarBox.Posn;
    if (this->Orientation == VTK_ORIENT_VERTICAL)
    { // NB. Size[0] = width, Size[1] = height
      // Ticks share the width with the scalar bar
      this->P->TickBox.Size[0] =
        this->P->Frame.Size[0] - this->P->ScalarBarBox.Size[0] -
        this->TextPad * 3;
      // Tick height could be adjusted if title text is
      // lowered by box constraints, but we won't bother:
      this->P->TickBox.Size[1] = this->P->Frame.Size[1] -
        this->P->TitleBox.Size[1] - 3 * this->TextPad -
        this->VerticalTitleSeparation;
      // Tick box height also reduced by NaN swatch size, if present:
      if (this->DrawNanAnnotation)
      {
        this->P->TickBox.Size[1] = static_cast<int>(this->P->TickBox.Size[1] -
          (this->P->NanBox.Size[1] + this->P->SwatchPad));
      }
      if (this->DrawBelowRangeSwatch)
      {
        this->P->TickBox.Size[1] = static_cast<int>(this->P->TickBox.Size[1] -
          (this->P->BelowRangeSwatchBox.Size[1] + this->P->SwatchPad));
      }
      if (this->DrawAboveRangeSwatch)
      {
        this->P->TickBox.Size[1] = static_cast<int>(this->P->TickBox.Size[1] -
          (this->P->AboveRangeSwatchBox.Size[1] + this->P->SwatchPad));
      }

      if (this->TextPosition == vtkScalarBarActor::PrecedeScalarBar)
      {
        this->P->TickBox.Posn[0] = this->TextPad;
      }
      else
      {
        this->P->TickBox.Posn[0] += this->P->ScalarBarBox.Size[0] + 2 *
          this->TextPad;
      }

      targetWidth = this->P->TickBox.Size[0];
      targetHeight = static_cast<int>((this->P->TickBox.Size[1] -
        this->TextPad * (this->NumberOfLabels - 1)) /
                                      this->NumberOfLabels);
    }
    else
    { // NB. Size[1] = width, Size[0] = height
      // Ticks span the entire width of the frame
      this->P->TickBox.Size[1] = this->P->ScalarBarBox.Size[1];
      // Ticks share vertical space with title and scalar bar.
      this->P->TickBox.Size[0] =
        this->P->Frame.Size[0] - this->P->ScalarBarBox.Size[0] -
        4 * this->TextPad - this->P->TitleBox.Size[0];

      if (this->TextPosition == vtkScalarBarActor::PrecedeScalarBar)
      {
        this->P->TickBox.Posn[1] =
          this->P->TitleBox.Size[0] + 2 * this->TextPad + this->P->TitleBox.Posn[1];
      }
      else
      {
        this->P->TickBox.Posn[1] += this->P->ScalarBarBox.Size[0];
      }
      targetWidth = static_cast<int>((this->P->TickBox.Size[1] -
        this->TextPad * (this->NumberOfLabels - 1)) /
          this->NumberOfLabels);
      targetHeight = this->P->TickBox.Size[0];
    }

    if (!this->UnconstrainedFontSize)
    {
      vtkTextActor::SetMultipleConstrainedFontSize(
        this->P->Viewport, targetWidth, targetHeight,
        this->P->TextActors.PointerArray(), this->NumberOfLabels,
        labelSize);
    }

    // Now adjust scalar bar size by the half-size of the first and last ticks
    this->P->ScalarBarBox.Posn[this->P->TL[1]]
      += labelSize[this->P->TL[1]] / 2.;
    this->P->ScalarBarBox.Size[1] -= labelSize[this->P->TL[1]];
    this->P->TickBox.Posn[this->P->TL[1]] =
      static_cast<int>(this->P->TickBox.Posn[this->P->TL[1]] +
                       (labelSize[this->P->TL[1]] / 2.));
    this->P->TickBox.Size[1] -= labelSize[this->P->TL[1]];

    if (this->Orientation == VTK_ORIENT_HORIZONTAL)
    {
      this->P->ScalarBarBox.Posn[0] += this->P->BelowRangeSwatchSize;

      this->P->TickBox.Posn[0] += this->P->BelowRangeSwatchSize;

      this->P->TickBox.Posn[1] += this->TextPad *
        (this->TextPosition == PrecedeScalarBar ? -1 : 1);
      this->P->TickBox.Size[1] -= this->TextPad;
    }
  }
  this->NumberOfLabelsBuilt = this->NumberOfLabels;
}

//-----------------------------------------------------------------------------
void vtkScalarBarActor::LayoutForUnconstrainedFont()
{
  if (this->UnconstrainedFontSize)
  {
    // recompute tickbox layout max sizes
    int labelWidth = 0;
    int labelHeight = 0;
    double fontSize[2] = {0, 0};
    for (size_t i = 0; i < this->P->TextActors.size(); ++i)
    {
      this->P->TextActors.at(i)->GetSize(this->P->Viewport, fontSize);
      if (fontSize[0] > labelWidth)
      {
        labelWidth = fontSize[0];
      }

      if (fontSize[1] > labelHeight)
      {
        labelHeight = fontSize[1];
      }
    }

    if (this->Orientation == VTK_ORIENT_VERTICAL)
    {
      this->P->TitleBox.Posn[1] += labelHeight * 0.75;
      this->P->TickBox.Size[0] = labelWidth;
      if (this->TextPosition == vtkScalarBarActor::PrecedeScalarBar)
      {
        this->P->TickBox.Posn[0] =
          -labelWidth + this->P->Frame.Size[0] - this->P->ScalarBarBox.Size[0] -
          (labelWidth * 0.05);
      }
    }
    else
    {
      if (this->TextPosition == vtkScalarBarActor::PrecedeScalarBar)
      {
        this->P->TitleBox.Posn[1] =
          this->P->Frame.Posn[1] + this->P->ScalarBarBox.Posn[1] -
          this->P->TitleBox.Size[this->P->TL[1]] - labelHeight;
      }
      else
      {
        this->P->TitleBox.Posn[1] =
          this->P->Frame.Posn[1] +
          this->P->ScalarBarBox.Size[this->P->TL[1]] + labelHeight;
      }
    }
  }
}

//----------------------------------------------------------------------------
void vtkScalarBarActor::LayoutAnnotations()
{
  if (this->DrawAnnotations)
  {
    const double* range = this->LookupTable->GetRange();

    this->MapAnnotationLabels(
      this->LookupTable, this->P->ScalarBarBox.Posn[this->P->TL[1]],
      this->P->ScalarBarBox.Size[1], range);
  }
}

//----------------------------------------------------------------------------
void vtkScalarBarActor::ConfigureFrame()
{
  // set frame structure
  vtkPoints* frPts = vtkPoints::New();
  frPts->SetNumberOfPoints(5);
  vtkCellArray* frLines = vtkCellArray::New();
  frLines->Allocate(frLines->EstimateSize(1, 5));

  this->FrameActor->SetProperty(this->FrameProperty);
  this->Frame->Initialize();
  this->Frame->SetPoints(frPts);
  this->Frame->SetLines(frLines);
  frPts->Delete();
  frLines->Delete();

  // set background structure
  vtkPoints* bgPts = vtkPoints::New();
  bgPts->SetNumberOfPoints(4);
  vtkCellArray* bgPolys = vtkCellArray::New();
  bgPolys->Allocate(bgPolys->EstimateSize(1, 4));

  this->BackgroundActor->SetProperty(this->BackgroundProperty);
  this->Background->Initialize();
  this->Background->SetPoints(bgPts);
  this->Background->SetPolys(bgPolys);
  bgPts->Delete();
  bgPolys->Delete();

  double x[3];
  x[2] = 0.;

  // generate background and frame points and cell
  x[0] = 0;
  x[1] = 0;
  bgPts->SetPoint(0, x);
  frPts->SetPoint(0, x);
  frPts->SetPoint(4, x);

  x[0] = 0;
  x[1] = this->P->Frame.Size[this->P->TL[1]] - 0.5;
  bgPts->SetPoint(1, x);
  frPts->SetPoint(1, x);

  x[0] = this->P->Frame.Size[this->P->TL[0]] - 0.5;
  x[1] = this->P->Frame.Size[this->P->TL[1]] - 0.5;
  bgPts->SetPoint(2, x);
  frPts->SetPoint(2, x);

  x[0] = this->P->Frame.Size[this->P->TL[0]] - 0.5;
  x[1] = 0;
  bgPts->SetPoint(3, x);
  frPts->SetPoint(3, x);

  vtkIdType bgIds[5] = {0, 1, 2, 3, 4};
  bgPolys->InsertNextCell(4, bgIds);
  frLines->InsertNextCell(5, bgIds);
}

//----------------------------------------------------------------------------
void vtkScalarBarActor::ConfigureScalarBar()
{
  vtkScalarsToColors* lut = this->LookupTable;
  const double* range = lut->GetRange();
  this->P->NumColors = lut->GetIndexedLookup() ?
    lut->GetNumberOfAnnotatedValues() : this->MaximumNumberOfColors;
  this->P->NumSwatches = this->P->NumColors + (this->DrawNanAnnotation ? 1 : 0);
  int numPts = 2 * (this->P->NumColors + 1) + (this->DrawNanAnnotation ? 4 : 0);

  this->P->NumSwatches = this->P->NumColors + (this->DrawBelowRangeSwatch ? 1 : 0);
  numPts += (this->DrawBelowRangeSwatch ? 4 : 0);

  this->P->SwatchPts = vtkPoints::New();
  this->P->SwatchPts->SetNumberOfPoints(numPts);
  this->P->Polys = vtkCellArray::New();
  this->P->Polys->Allocate(
    this->P->Polys->EstimateSize(this->P->NumSwatches, 4));
  this->P->SwatchColors = vtkUnsignedCharArray::New();

  unsigned int nComponents = ((this->UseOpacity) ? 4 : 3);
  this->P->SwatchColors->SetNumberOfComponents(nComponents);
  this->P->SwatchColors->SetNumberOfTuples(this->P->NumSwatches);

  this->ScalarBarActor->SetProperty(this->GetProperty());
  this->ScalarBar->Initialize();
  this->ScalarBar->SetPoints(this->P->SwatchPts);
  this->ScalarBar->SetPolys(this->P->Polys);
  this->ScalarBar->GetCellData()->SetScalars(this->P->SwatchColors);
  this->P->SwatchPts->Delete();
  this->P->Polys->Delete();
  this->P->SwatchColors->Delete();

  double delta =
    static_cast<double>(this->P->ScalarBarBox.Size[1]) / this->P->NumColors;
  double x[3];
  x[2] = 0.;
  for (int i = 0; i < numPts / 2 - (this->DrawNanAnnotation ? 2 : 0) -
       (this->DrawBelowRangeSwatch ? 2 : 0) ; ++i)
  {
    x[this->P->TL[0]] = this->P->ScalarBarBox.Posn[this->P->TL[0]];
    x[this->P->TL[1]] = this->P->ScalarBarBox.Posn[this->P->TL[1]] + i * delta;
    this->P->SwatchPts->SetPoint(2 * i, x);

    x[this->P->TL[0]] = this->P->ScalarBarBox.Posn[this->P->TL[0]] +
      this->P->ScalarBarBox.Size[0];
    this->P->SwatchPts->SetPoint(2 * i + 1, x);
  }

  //polygons & cell colors
  unsigned char* rgb;
  double rgba[4];
  vtkIdType ptIds[4];
  for (int i = 0; i < this->P->NumColors; ++i)
  {
    ptIds[0] = 2 * i;
    ptIds[1] = ptIds[0] + 1;
    ptIds[2] = ptIds[1] + 2;
    ptIds[3] = ptIds[0] + 2;
    this->P->Polys->InsertNextCell(4, ptIds);
    double rgbval;
    if (this->LookupTable->UsingLogScale())
    {
      rgbval = log10(range[0]) +
        i * (log10(range[1]) - log10(range[0])) / this->P->NumColors;
      rgbval = pow(10.0, rgbval);
    }
    else
    {
      rgbval = range[0] + (range[1] - range[0]) *
        (i / static_cast<double>(this->P->NumColors));
    }
    lut->GetColor(rgbval, rgba);
    rgba[3] = lut->GetOpacity(rgbval);
    //write into array directly
    rgb = this->P->SwatchColors->GetPointer(nComponents * i);
    rgb[0] = static_cast<unsigned char>(rgba[0] * 255.);
    rgb[1] = static_cast<unsigned char>(rgba[1] * 255.);
    rgb[2] = static_cast<unsigned char>(rgba[2] * 255.);
    if (this->P->SwatchColors->GetNumberOfComponents() > 3)
    {
      rgb[3] = static_cast<unsigned char>(this->UseOpacity ? rgba[3] * 255. : 255.);
    }
  }

  // Set up a texture actor as an alternative to the 1-quad-per-color
  // scalar bar polydata.
  vtkPoints* texturePoints = vtkPoints::New();
  texturePoints->SetNumberOfPoints(4);
  this->TexturePolyData->SetPoints(texturePoints);
  texturePoints->SetPoint(0, 0.0, 0.0, 0.0);

  double p1[2], p2[2];
  p1[0] = this->P->ScalarBarBox.Posn[0];
  p1[1] = this->P->ScalarBarBox.Posn[1];
  p2[0] = p1[0] + this->P->ScalarBarBox.Size[this->P->TL[0]];
  p2[1] = p1[1] + this->P->ScalarBarBox.Size[this->P->TL[1]];

  texturePoints->SetPoint(0, p1[0], p1[1], 0.0);
  texturePoints->SetPoint(1, p2[0], p1[1], 0.0);
  texturePoints->SetPoint(2, p2[0], p2[1], 0.0);
  texturePoints->SetPoint(3, p1[0], p2[1], 0.0);
  texturePoints->Delete();

  double barWidth = this->P->ScalarBarBox.Size[this->P->TL[0]];
  double barHeight = this->P->ScalarBarBox.Size[this->P->TL[1]];
  vtkDataArray* tc = this->TexturePolyData->GetPointData()->GetTCoords();
  tc->SetTuple2(1, barWidth / this->TextureGridWidth, 0.0);
  tc->SetTuple2(2, barWidth / this->TextureGridWidth,
                barHeight / this->TextureGridWidth);
  tc->SetTuple2(3, 0.0, barHeight / this->TextureGridWidth);
}

//----------------------------------------------------------------------------
void vtkScalarBarActor::ConfigureTitle()
{
  this->TitleActor->SetPosition(
    this->P->TitleBox.Posn[0] + this->P->TitleBox.Size[this->P->TL[0]] / 2,
    this->TitleActor->GetTextProperty()->GetVerticalJustification() ==
    VTK_TEXT_BOTTOM ?
    this->P->TitleBox.Posn[1]:
    this->P->TitleBox.Posn[1] + this->P->TitleBox.Size[this->P->TL[1]]);
}

//-----------------------------------------------------------------------------
void vtkScalarBarActor::ConfigureTicks()
{
  double val;
  double sizeTextData[2];
  for (int i = 0; i < this->NumberOfLabelsBuilt; ++i)
  {
    val = (this->NumberOfLabelsBuilt > 1 ?
      static_cast<double>(i) / (this->NumberOfLabelsBuilt - 1) : 0.5) *
      this->P->TickBox.Size[1] + this->P->TickBox.Posn[this->P->TL[1]];
    this->P->TextActors[i]->GetSize(this->P->Viewport, sizeTextData);
    if (this->Orientation == VTK_ORIENT_VERTICAL)
    { // VERTICAL
      this->P->TextActors[i]->GetTextProperty()->SetJustification(
        this->TextPosition == PrecedeScalarBar ? VTK_TEXT_RIGHT : VTK_TEXT_LEFT);
      this->P->TextActors[i]->GetTextProperty()
        ->SetVerticalJustificationToBottom();
      this->P->TextActors[i]->SetPosition(
        this->TextPosition == vtkScalarBarActor::PrecedeScalarBar ?
        this->P->TickBox.Posn[0] + this->P->TickBox.Size[0] :
        this->P->TickBox.Posn[0],
        //this->P->TickBox.Posn[0],
        val - 0.5 * sizeTextData[1]);
    }
    else
    { // HORIZONTAL
      this->P->TextActors[i]->GetTextProperty()->SetJustificationToCentered();
      this->P->TextActors[i]->GetTextProperty()->SetVerticalJustification(
        this->TextPosition == PrecedeScalarBar ? VTK_TEXT_TOP : VTK_TEXT_BOTTOM);
      this->P->TextActors[i]->SetPosition(val,
        this->TextPosition == PrecedeScalarBar ?
        this->P->TickBox.Posn[1] + this->P->TickBox.Size[0] :
        this->P->TickBox.Posn[1]);
    }
  }
}

//-----------------------------------------------------------------------------
void vtkScalarBarActor::ConfigureNanSwatch()
{
  if (!this->DrawNanAnnotation)
  {
    return;
  }

  int numPts = 4;
  vtkPoints* pts = vtkPoints::New();
  pts->SetNumberOfPoints(numPts);
  vtkCellArray* polys = vtkCellArray::New();
  polys->Allocate(polys->EstimateSize(1, 4));
  vtkUnsignedCharArray* colors = vtkUnsignedCharArray::New();
  colors->SetNumberOfComponents(4); // RGBA
  colors->SetNumberOfTuples(1);

  this->P->NanSwatch->Initialize();
  this->P->NanSwatch->SetPoints(pts);
  this->P->NanSwatch->SetPolys(polys);
  this->P->NanSwatch->GetCellData()->SetScalars(colors);
  pts->Delete();
  polys->Delete();
  colors->Delete();

  double x[3];
  x[2] = 0.;

  for (int j = 0; j < 2; ++j)
  {
    x[j] = this->P->NanBox.Posn[j];
  }
  int i = 0;
  pts->SetPoint(i++, x);
  x[0] += this->P->NanBox.Size[this->P->TL[0]];
  pts->SetPoint(i++, x);
  x[1] += this->P->NanBox.Size[this->P->TL[1]];
  pts->SetPoint(i++, x);
  x[0] -= this->P->NanBox.Size[this->P->TL[0]];
  pts->SetPoint(i++, x);

  // Add the swatch to the polydata and color it:
  unsigned char* rgb;
  double rgba[4];
  vtkIdType ptIds[4];
  ptIds[0] = 0;
  ptIds[1] = 1;
  ptIds[2] = 2;
  ptIds[3] = 3;
  polys->InsertNextCell(4, ptIds);
  this->LookupTable->GetIndexedColor(-1, rgba);
  rgb = colors->GetPointer(0); //write into array directly
  rgb[0] = static_cast<unsigned char>(rgba[0] * 255.);
  rgb[1] = static_cast<unsigned char>(rgba[1] * 255.);
  rgb[2] = static_cast<unsigned char>(rgba[2] * 255.);
  rgb[3] = static_cast<unsigned char>(this->UseOpacity ? rgba[3] * 255. : 255.);
}

//----------------------------------------------------------------------------
void vtkScalarBarActor::ConfigureAboveBelowRangeSwatch(bool above)
{
  // Check above/below
  vtkPolyData* swatch;
  vtkScalarBarBox* box;
  if (above)
  {
    if (!this->DrawAboveRangeSwatch)
    {
      return;
    }
    swatch = this->P->AboveRangeSwatch;
    box = &this->P->AboveRangeSwatchBox;
  }
  else
  {
    if (!this->DrawBelowRangeSwatch)
    {
      return;
    }
    swatch = this->P->BelowRangeSwatch;
    box = &this->P->BelowRangeSwatchBox;
  }

  int numPts = 4;
  vtkNew<vtkPoints> pts;
  pts->SetNumberOfPoints(numPts);
  vtkNew<vtkCellArray> polys;
  polys->Allocate(polys->EstimateSize(1, 4));
  vtkNew<vtkUnsignedCharArray> colors;
  colors->SetNumberOfComponents(4); // RGBA
  colors->SetNumberOfTuples(1);

  swatch->Initialize();
  swatch->SetPoints(pts.Get());
  swatch->SetPolys(polys.Get());
  swatch->GetCellData()->SetScalars(colors.Get());
  double x[3];
  x[2] = 0.;

  for (int j = 0; j < 2; ++j)
  {
    x[j] = box->Posn[j];
  }
  int i = 0;
  pts->SetPoint(i++, x);
  x[0] += box->Size[this->P->TL[0]];
  pts->SetPoint(i++, x);
  x[1] += box->Size[this->P->TL[1]];
  pts->SetPoint(i++, x);
  x[0] -= box->Size[this->P->TL[0]];
  pts->SetPoint(i++, x);

  // Add the swatch to the polydata and color it:
  double rgba[4] = { 1.0, 1.0, 1.0, 1.0 };
  vtkIdType ptIds[4] = { 0, 1, 2, 3};
  polys->InsertNextCell(4, ptIds);
  this->LookupTable->GetIndexedColor(-1, rgba);

  vtkLookupTable* lt = vtkLookupTable::SafeDownCast(this->LookupTable);
  vtkColorTransferFunction* ctf =
    vtkColorTransferFunction::SafeDownCast(this->LookupTable);

  if (lt)
  {
    if (above)
    {
      lt->GetAboveRangeColor(rgba);
    }
    else
    {
      lt->GetBelowRangeColor(rgba);
    }
  }
  else if (ctf)
  {
    if (above)
    {
      ctf->GetAboveRangeColor(rgba);
    }
    else
    {
      ctf->GetBelowRangeColor(rgba);
    }
  }

  unsigned char* rgb = colors->GetPointer(0); //write into array directly
  rgb[0] = static_cast<unsigned char>(rgba[0] * 255.);
  rgb[1] = static_cast<unsigned char>(rgba[1] * 255.);
  rgb[2] = static_cast<unsigned char>(rgba[2] * 255.);
  rgb[3] = static_cast<unsigned char>(this->UseOpacity ? rgba[3] * 255. : 255.);

}

//----------------------------------------------------------------------------
void vtkScalarBarActor::ConfigureAnnotations()
{
  // I. Create an actor for each valid label.
  int numNotes = static_cast<int>(this->P->Labels.size());
  if (!numNotes)
  {
    return;
  }

  this->P->AnnotationLabels.resize(numNotes);
  this->P->AnnotationAnchors.resize(numNotes);
  this->P->AnnotationColors.resize(numNotes);
  int i = 0;
  std::map<double, vtkStdString>::iterator it;
  for (it = this->P->Labels.begin(); it != this->P->Labels.end(); ++it, ++i)
  {
    this->P->AnnotationAnchors[i] = it->first;
    this->P->AnnotationColors[i] = this->P->LabelColors[it->first];
    this->P->AnnotationLabels[i] = vtkSmartPointer<vtkTextActor>::New();
    if (this->P->Viewport && this->AnnotationTextScaling)
    {
      this->P->AnnotationLabels[i]->SetTextScaleModeToViewport();
      this->P->AnnotationLabels[i]->ComputeScaledFont(this->P->Viewport);
    }
    this->P->AnnotationLabels[i]->GetTextProperty()->ShallowCopy(
      this->AnnotationTextProperty);

    this->P->AnnotationLabels[i]->SetProperty(this->GetProperty());
    // NB: If passed an empty string, pass a single space to the renderer;
    // empty strings get rendered as blobs which is Highly Undesirable.
    // Also render an empty string if the annotation can't be placed on the bar.
    this->P->AnnotationLabels[i]->SetInput(it->second.c_str());
    this->P->AnnotationLabels[i]->GetPositionCoordinate()->
      SetReferenceCoordinate(this->PositionCoordinate);
  }

  // Position each label and, in indexed mode, create the color swatches.
  if (this->LookupTable->GetIndexedLookup())
  {
    int indexedDenom = this->P->NumNotes;
    // Must reset the color on the leader lines since vtkTextProperty
    // doesn't inherit vtkProperty.
    // FIXME: Only set leader color when CellData scalars aren't used.
    this->P->AnnotationLeadersActor->GetProperty()->SetColor(
      this->AnnotationTextProperty->GetColor());
    this->P->AnnotationLeadersActor->GetProperty()->SetOpacity(
      this->AnnotationTextProperty->GetOpacity());
    // this->ScalarBar will not be drawn; instead, draw padded boxes
    // and leaders to labels for each annotated value.
    // Since labels are user-provided, we render with vtkTextActor
    // to allow fancy-ness.
    // 2 triangles per annotation: half-opaque, half-translucent:
    int numPts = 4 * indexedDenom;
    vtkPoints* pts = vtkPoints::New();
    pts->SetNumberOfPoints(numPts);
    vtkCellArray* polys = vtkCellArray::New();
    polys->Allocate(polys->EstimateSize(2 * indexedDenom, 3));
    vtkUnsignedCharArray* colors = vtkUnsignedCharArray::New();
    colors->SetNumberOfComponents(4); // RGBA
    colors->SetNumberOfTuples(2 * indexedDenom);

    this->P->AnnotationBoxes->Initialize();
    this->P->AnnotationBoxes->SetPoints(pts);
    this->P->AnnotationBoxes->SetPolys(polys);
    this->P->AnnotationBoxes->GetCellData()->SetScalars(colors);
    //this->P->AnnotationBoxes->SetProperty( this->GetProperty() );
    pts->Delete();
    polys->Delete();
    colors->Delete();

    // Use the nicely-provided scalar bar position to place
    // the annotated value swatches.
    double swatchC0, swatchC1; // fixed swatch coordinates
    double delta =
      static_cast<double>(this->P->ScalarBarBox.Size[1]) / indexedDenom;
    double x[3];
    x[2] = 0.;
    unsigned char* rgb;
    vtkIdType ptIds[4];
    swatchC0 = this->P->ScalarBarBox.Posn[this->P->TL[0]];
    swatchC1 = swatchC0 + this->P->ScalarBarBox.Size[0];
    if (this->Orientation == VTK_ORIENT_VERTICAL)
    {
      this->PlaceAnnotationsVertically(
        this->TextPosition == vtkScalarBarActor::SucceedScalarBar ?
        swatchC0 : swatchC1,
        this->P->ScalarBarBox.Posn[1],
        this->P->ScalarBarBox.Size[this->P->TL[0]],
        this->P->ScalarBarBox.Size[this->P->TL[1]],
        delta, this->P->SwatchPad);
      double top =
        this->P->ScalarBarBox.Posn[1] +
        this->P->ScalarBarBox.Size[this->P->TL[1]];
      for (i = 0; i < indexedDenom; ++i)
      {
        x[0] = swatchC0;
        x[1] = top - i * delta - this->P->SwatchPad;
        pts->SetPoint(4 * i, x);
        x[0] = swatchC1;
        pts->SetPoint(4 * i + 1, x);
        x[1] -= delta - this->P->SwatchPad * 2;
        pts->SetPoint(4 * i + 2, x);
        x[0] = swatchC0;
        pts->SetPoint(4 * i + 3, x);
      }
    }
    else
    {
      this->PlaceAnnotationsHorizontally(
        this->P->ScalarBarBox.Posn[0], swatchC1,
        this->P->ScalarBarBox.Size[1],
        this->P->ScalarBarBox.Size[0], delta, this->P->SwatchPad);
      for (i = 0; i < indexedDenom; ++i)
      {
        x[0] = this->P->ScalarBarBox.Posn[0] + i * delta + this->P->SwatchPad;
        x[1] = swatchC0;
        pts->SetPoint(4 * i, x);
        x[0] += delta - this->P->SwatchPad * 2;
        pts->SetPoint(4 * i + 1, x);
        x[1] = swatchC1;
        pts->SetPoint(4 * i + 2, x);
        x[0] -= delta - this->P->SwatchPad * 2;
        pts->SetPoint(4 * i + 3, x);
      }
    }
    for (i = 0; i < indexedDenom; ++i)
    {
      ptIds[0] = 4 * i;
      ptIds[1] = ptIds[0] + 1;
      ptIds[2] = ptIds[0] + 2;
      polys->InsertNextCell(3, ptIds);

      ptIds[1] = ptIds[2];
      ptIds[2] = ptIds[0] + 3;
      polys->InsertNextCell(3, ptIds);

      double rgbaF[4];
      this->LookupTable->GetIndexedColor(
        i == this->P->NumNotes ?
        -1 :
        i % this->LookupTable->GetNumberOfAvailableColors(),
        rgbaF);
      rgb = colors->GetPointer(
        /* numComponents */ 4 *
        /* numCells/swatch */ 2 *
        /* swatch */ i); //write into array directly
      rgb[0] = static_cast<unsigned char>(rgbaF[0] * 255.);
      rgb[1] = static_cast<unsigned char>(rgbaF[1] * 255.);
      rgb[2] = static_cast<unsigned char>(rgbaF[2] * 255.);
      rgb[3] = static_cast<unsigned char>(rgbaF[3] * 255.);
      rgb[4] = static_cast<unsigned char>(rgbaF[0] * 255.);
      rgb[5] = static_cast<unsigned char>(rgbaF[1] * 255.);
      rgb[6] = static_cast<unsigned char>(rgbaF[2] * 255.);
      rgb[7] = 255; // second triangle is always opaque
    }
  }
  else
  {
    if (this->Orientation == VTK_ORIENT_VERTICAL)
    {
      this->PlaceAnnotationsVertically(
        this->TextPosition == vtkScalarBarActor::SucceedScalarBar ?
        this->P->ScalarBarBox.Posn[this->P->TL[0]] :
        this->P->ScalarBarBox.Posn[this->P->TL[0]]
        + this->P->ScalarBarBox.Size[0],
        this->P->ScalarBarBox.Posn[1],
        this->P->ScalarBarBox.Size[0],
        this->P->ScalarBarBox.Size[1],
        0., this->P->SwatchPad);
    }
    else // HORIZONTAL
    {
      this->PlaceAnnotationsHorizontally(
        this->P->ScalarBarBox.Posn[0],
        this->P->ScalarBarBox.Posn[1] + this->P->ScalarBarBox.Size[0],
        this->P->ScalarBarBox.Size[1],
        this->P->ScalarBarBox.Size[0],
        0., this->P->SwatchPad);
    }
  }
}

//----------------------------------------------------------------------------
void vtkScalarBarActor::FreeLayoutStorage()
{
  // Delete previously constructed objects
  if (this->P->Viewport && this->P->Viewport->GetVTKWindow())
  {
    vtkWindow *win = this->P->Viewport->GetVTKWindow();
    if (!this->P->TextActors.empty())
    {
      vtkScalarBarActorInternal::ActorVector::iterator it;
      for (
           it = this->P->TextActors.begin();
           it != this->P->TextActors.end();
           ++it)
      {
        (*it)->ReleaseGraphicsResources(win);
      }
    }
    for (size_t i = 0; i < this->P->AnnotationLabels.size(); ++i)
    {
      this->P->AnnotationLabels[i]->ReleaseGraphicsResources(win);
    }
  }

  this->P->TextActors.clear();
  this->P->AnnotationLabels.clear();
  this->P->AnnotationAnchors.clear();
  this->P->AnnotationColors.clear();
}

//----------------------------------------------------------------------------
void vtkScalarBarActor::SizeTitle(double* titleSize,
                                  int* size,
                                  vtkViewport* viewport)
{
  titleSize[0] = titleSize[1] = 0;

  if (this->Title == NULL || !strlen(this->Title))
  {
    return;
  }

  int targetWidth, targetHeight;
  if (this->Orientation == VTK_ORIENT_VERTICAL)
  {
    targetWidth = static_cast<int>(0.9 * size[0]);
    targetHeight = static_cast<int>(0.1 * size[1]);
  }
  else
  {
    targetWidth = size[0];
    /*
       targetHeight = static_cast<int>(
       (this->LookupTable->GetIndexedLookup() ? 0.5 : 0.25) * size[1]);
     */
    if (this->LookupTable->GetIndexedLookup())
    {
      targetHeight = static_cast<int>(0.5 * size[1]);
    }
    else
    {
      double sizeTextData[2] = {0, 0};
      if (!this->P->TextActors.empty())
      { // Don't overlap tick-mark labels:
        this->P->TextActors[0]->GetSize(viewport, sizeTextData);
      }
      // The scalar bar takes half the height.
      // Subtract tick-label height and padding
      targetHeight =
        static_cast<int>((0.5 - 2 * 0.05) * size[1] - sizeTextData[1]);
    }
  }

  this->TitleActor->SetConstrainedFontSize(
    viewport, targetWidth, targetHeight);

  this->TitleActor->GetSize(viewport, titleSize);
}

//----------------------------------------------------------------------------
int vtkScalarBarActor::MapAnnotationLabels(
  vtkScalarsToColors* lkup, double start, double delta, const double* range)
{
  int numNotes = lkup->GetNumberOfAnnotatedValues();
  bool indexed = lkup->GetIndexedLookup() ? true : false;
  bool vertical = (this->Orientation == VTK_ORIENT_VERTICAL);
  vtkColor4d fltCol;
  double drange = range[1] - range[0];
  // I. If we are not in indexed mode, we must sort the labels that we can
  //    position by their order of appearance (since placement gives
  //    precedence to the median label). Hence, we use a map to accumulate
  //    labels.
  this->P->Labels.clear();
  this->P->LabelColors.clear();
  if (numNotes > 0)
  {
    for (int i = 0; i < numNotes; ++i)
    {
      vtkStdString label = lkup->GetAnnotation(i);
      lkup->GetAnnotationColor(lkup->GetAnnotatedValue(i), fltCol.GetData());
      double x;
      bool canPositionLabel = !label.empty();
      if (canPositionLabel)
      {
        if (indexed)
        {
          // Vertical orientation in indexed lookup mode is a special case:
          // The first swatch is placed at the top (highest y coordinate).
          // All other cases (all horizontal, interval-mode vertical) order
          // labels from lowest coordinate to highest.
          x = vertical ?
            start + (numNotes - i - 0.5) * delta / numNotes :
            start + (i + 0.5) * delta / numNotes;
        }
        else
        {
          vtkVariant pos = lkup->GetAnnotatedValue(i);
          x = pos.ToDouble(&canPositionLabel);
          if (canPositionLabel)
          { // Also do not draw if label is outside the scalar bar range.
            canPositionLabel = (x >= range[0] && x <= range[1]);
          }
          x = canPositionLabel ?
            (start + (x - range[0]) * delta / drange) :
            vtkMath::Nan();
        }
      }
      if (canPositionLabel)
      {
        this->P->Labels[x] = label;
        // Obtain a color for leader lines
        vtkColor3ub intCol;
        for (int j = 0; j < 3; ++j)
        {
          intCol.GetData()[j] =
            static_cast<unsigned char>(fltCol.GetData()[j] * 255.);
        }
        this->P->LabelColors[x] = intCol;
      }
    }
  }

  // II. Optionally add a NaN label.
  if (
      this->DrawNanAnnotation &&
      this->NanAnnotation &&
      this->NanAnnotation[0] != '\0')
  {
    // Get the NaN color
    lkup->GetIndexedColor(-1, fltCol.GetData());
    vtkColor3ub intCol;
    for (int j = 0; j < 3; ++j)
    {
      intCol.GetData()[j] =
        static_cast<unsigned char>(fltCol.GetData()[j] * 255.);
    }
    double x =
      this->P->NanBox.Posn[this->P->TL[1]] + this->P->NanBox.Size[1] / 2.;
    this->P->Labels[x] = this->NanAnnotation;
    this->P->LabelColors[x] = intCol;
  }

  // III. Optionally add a below range label.
  if (
      this->DrawBelowRangeSwatch &&
      this->BelowRangeAnnotation &&
      this->BelowRangeAnnotation[0] != '\0')
  {
    // Get the below range color
    lkup->GetIndexedColor(-1, fltCol.GetData());
    vtkColor3ub intCol;
    for (int j = 0; j < 3; ++j)
    {
      intCol.GetData()[j] =
        static_cast<unsigned char>(fltCol.GetData()[j] * 255.);
    }
    double x =
      this->P->BelowRangeSwatchBox.Posn[this->P->TL[1]] +
      this->P->BelowRangeSwatchBox.Size[1] / 2.;
    this->P->Labels[x] = this->BelowRangeAnnotation;
    this->P->LabelColors[x] = intCol;
  }

  // IV. Optionally add an above range label.
  if (
      this->DrawAboveRangeSwatch &&
      this->AboveRangeAnnotation &&
      this->AboveRangeAnnotation[0] != '\0')
  {
    // Get the above range color
    lkup->GetIndexedColor(-1, fltCol.GetData());
    vtkColor3ub intCol;
    for (int j = 0; j < 3; ++j)
    {
      intCol.GetData()[j] =
        static_cast<unsigned char>(fltCol.GetData()[j] * 255.);
    }
    double x =
      this->P->AboveRangeSwatchBox.Posn[this->P->TL[1]] +
      this->P->AboveRangeSwatchBox.Size[1] / 2.;
    this->P->Labels[x] = this->AboveRangeAnnotation;
    this->P->LabelColors[x] = intCol;
  }

  // V. Give subclasses a chance to edit the label map.
  this->EditAnnotations();
  return static_cast<int>(this->P->Labels.size());
}

//----------------------------------------------------------------------------
int vtkScalarBarActor::PlaceAnnotationsVertically(
  double barX, double barY,
  double vtkNotUsed(barWidth), double barHeight,
  double vtkNotUsed(delta), double pad)
{
  if (!this->LookupTable)
  {
    return 0;
  }

#define VTK_ANN_VLAYOUT(j, dir, delt) \
  ctr = this->P->AnnotationAnchors[j]; \
  ll[0] = lpts->InsertNextPoint(xl0, ctr, 0.); \
  this->P->AnnotationLabels[j]->GetSize(this->P->Viewport, tsz); \
  hh = (tsz[1] + pad) / 2.; /* label half-height, incl. padding */ \
  if (((dir) < 0 && ctr + hh > dnCum) || ((dir) > 0 && ctr - hh < upCum)) \
  ctr = (delt) + (dir) * hh; \
  this->P->AnnotationLabels[j]->GetTextProperty()->SetJustification(\
    this->TextPosition == PrecedeScalarBar ? VTK_TEXT_LEFT : VTK_TEXT_RIGHT);\
  this->P->AnnotationLabels[j]->GetTextProperty()\
  ->SetVerticalJustificationToCentered(); \
  this->P->AnnotationLabels[j]->SetPosition(\
    barX + (this->TextPosition == PrecedeScalarBar ? 1 : -1) * \
      (pad + this->AnnotationLeaderPadding), \
                                            ctr); \
  ll[1] = lpts->InsertNextPoint(xl1, ctr, 0.); \
  llines->InsertNextCell(2, ll); \
  llcolors->InsertNextTypedTuple(this->P->AnnotationColors[j].GetData()); \
  if (upCum < ctr + hh) upCum = ctr + hh; \
  if (dnCum > ctr - hh) dnCum = ctr - hh;

  int numNotes = static_cast<int>(this->P->AnnotationLabels.size());
  vtkPoints* lpts = vtkPoints::New();
  vtkCellArray* llines = vtkCellArray::New();
  vtkUnsignedCharArray* llcolors = vtkUnsignedCharArray::New();
  llcolors->SetName("Leader Line Colors");
  llcolors->SetNumberOfComponents(3);
  llcolors->Allocate(numNotes);
  lpts->Allocate(2 * numNotes);
  llines->Allocate(llines->EstimateSize(numNotes, 2));
  this->P->AnnotationLeaders->Initialize();
  this->P->AnnotationLeaders->SetPoints(lpts);
  this->P->AnnotationLeaders->SetLines(llines);
  if (this->FixedAnnotationLeaderLineColor)
  {
    this->P->AnnotationLeaders->GetCellData()->SetScalars(NULL);
  }
  else
  {
    this->P->AnnotationLeaders->GetCellData()->SetScalars(llcolors);
  }

  // Start at the center and move outward (both up and down),
  // accumulating label heights as we go.
  int ic = numNotes / 2;
  int dn, up;
  double dnCum, upCum, ctr, hh;
  double tsz[2];
  // leader-line endpoint x-coordinates:
  double xl0 =
    barX + (this->TextPosition == PrecedeScalarBar ? 1 : -1) * pad / 2.;
  double xl1 = barX +
    (this->TextPosition == PrecedeScalarBar ? 1 : -1) *
    (pad / 2. + this->AnnotationLeaderPadding);
  vtkIdType ll[2]; // leader-line endpoint IDs
  if (2 * ic == numNotes)
  {
    dn = ic - 1;
    up = ic;
    dnCum = barY + barHeight;
    upCum = barY;
  }
  else
  {
    dn = ic - 1;
    up = ic + 1;
    dnCum = barY + barHeight;
    upCum = barY;
    VTK_ANN_VLAYOUT(ic, 0, dnCum);
    /*
       cout
       << "ic: " << ic << " ctr: " << ctr << " hh: " << hh
       << " uc: " << upCum << " dc: " << dnCum
       << " t:" << this->P->AnnotationLabels[ic]->GetInput() << endl;
     */
  }
  for (; dn >= 0; --dn, ++up)
  {
    VTK_ANN_VLAYOUT(dn, -1, dnCum);
    /*
       cout
       << "dn: " << dn << " ctr: " << ctr << " hh: " << hh
       << " uc: " << upCum << " dc: " << dnCum
       << " t:" << this->P->AnnotationLabels[dn]->GetInput() << endl;
     */
    VTK_ANN_VLAYOUT(up, 1, upCum);
    /*
       cout
       << "up: " << up << " ctr: " << ctr << " hh: " << hh
       << " uc: " << upCum << " dc: " << dnCum
       << " t:" << this->P->AnnotationLabels[up]->GetInput() << endl;
     */
  }

  lpts->Delete();
  llines->Delete();
  llcolors->Delete();
  return numNotes;
}

//-----------------------------------------------------------------------------
struct vtkScalarBarHLabelInfo
{
  double X[2]; // padded left-right label bounds
  double Y[2]; // padded top-bottom label bounds
  int Justification;
  double Anchor[2]; // x-y coordinates of anchor point
};

//-----------------------------------------------------------------------------
// A non-overlapping label placer for a horizontal array of annotated swatches.
// When space is tight, It displaces labels vertically and uses broken leader
// lines to relate labels back to swatches.
struct vtkScalarBarHLabelPlacer
{
  std::vector<vtkScalarBarHLabelInfo> Places;
  unsigned Ctr;
  double Y0;
  double XBounds[2];
  int NumPlaced;
  double Pad;
  double LeaderPad;
  double Dir; // displacement direction (either +1 or -1)
  bool HaveCtr; // Is a label at the dead center? (i.e., is Places.size() odd?)

  vtkScalarBarHLabelPlacer(
    unsigned n, double y0, double dir, double xmin, double xmax,
    double pad, double leaderPad)
    : Places(n), Ctr(n % 2 ? n / 2 : n / 2 - 1), Y0(y0), NumPlaced(0),
    Pad(pad), LeaderPad(leaderPad),
    Dir(dir < 0 ? -1. : 1.), HaveCtr(n % 2 ? true : false)
  {
      this->XBounds[0] = xmin;
      this->XBounds[1] = xmax;
  }

  void Place(unsigned i, double xBest, double wd, double ht)
  {
    vtkScalarBarHLabelInfo& placement(this->Places[i]);
    unsigned farLo, farHi;
    int medNeighbor;
    int posRelToCenter =
      (i == this->Ctr && this->HaveCtr) ? 0 : (i > this->Ctr ? 1 : -1);

    if (posRelToCenter == 0 || this->NumPlaced == 0)
    { // center label
      placement.Y[0] = this->Y0 + this->Dir * (this->LeaderPad + this->Pad);
      // Note Y[1] has un-padded bounds on distal y axis! Required below.
      placement.Y[1] = placement.Y[0] + this->Dir * ht;
      placement.X[0] = xBest - wd / 2. - this->Pad;
      placement.X[1] = xBest + wd / 2. + this->Pad;
      placement.Justification = VTK_TEXT_CENTERED;
      placement.Anchor[0] = xBest;
      // Vertical justification changes, but Y[0] is always anchor:
      placement.Anchor[1] = placement.Y[0];
    }
    else // placing *a lateral* (as opposed to *the medial*) label.
    {
      // First: Horizontal placement.
      // Check immediate medial neighbor to see if placement can occur
      // without more displacement.
      bool needToDisplace = false;
      if (posRelToCenter == 1)
      {
        // label is right-justified;
        // placement.X[1] bounded from above by XBounds[1]

        // Furthest label we have placed so far.
        farLo = 2 * this->Ctr + (this->HaveCtr ? 0 : 1) - i;
        farHi = this->Ctr; // The closest label we might overlap is dead center.
        medNeighbor = i - 1;
        placement.Justification = VTK_TEXT_RIGHT;
        placement.X[1] = xBest;
        placement.X[0] = placement.X[1] - wd - 2 * this->Pad;
        placement.Anchor[0] = placement.X[1]; // - this->Pad;
        if (xBest - wd < this->Places[medNeighbor].X[1])
        { // We must displace
          needToDisplace = true;
        }
      }
      else // posRelToCenter == -1
      {
        // label is left-justified;
        // placement.X[0] bounded from below by XBounds[0] or left neighbor

        // The center label is the closest label we might overlap:
        farLo = this->Ctr + (this->HaveCtr ? 0 : 1);
        // The furthest label to the right we have placed so far.
        farHi = 2 * this->Ctr - i - (this->HaveCtr ? 1 : 0);
        medNeighbor = i + 1;
        if (!this->HaveCtr && medNeighbor >= static_cast<int>(farHi))
        {
          medNeighbor = -1;
        }
        placement.Justification = VTK_TEXT_LEFT;
        placement.X[0] = xBest;
        placement.X[1] = placement.X[0] + wd + 2 * this->Pad;
        placement.Anchor[0] = placement.X[0];
        if (medNeighbor >= 0 && xBest + wd > this->Places[medNeighbor].X[0])
        {
          // we must displace; put the label where it makes sense:
          // bounded on left by swatch edge.
          needToDisplace = true;
        }
      }
      // Second: Vertical placement. Displace label to avoid overlap if need be.
      if (!needToDisplace)
      {
        placement.Y[0] = (medNeighbor >= 0 ?
                          this->Places[medNeighbor].Y[0] :
                          this->Y0 + this->Dir * (this->LeaderPad + this->Pad));
        placement.Y[1] = placement.Y[0] + this->Dir * ht;
        //placement.X[1] = placement.X[0] + wd;
        placement.Anchor[1] = placement.Y[0];
      }
      else
      { // must displace... find out by how much
        // I. At least as much as immediate medial neighbor
        placement.Y[0] = this->Places[medNeighbor].Y[1] + this->Dir * this->Pad;
        for (unsigned j = farLo; j <= farHi; ++j)
        {
          // II. Check whether label has any y overlap && any x overlap.
          // There are 2 cases: one for labels above swatches, the other
          // for labels below swatches.
          if (
              (this->Dir < 0 && placement.Y[0] > this->Places[j].Y[1] &&
               (i > j ?
                placement.X[0] <= this->Places[j].X[1] :
                placement.X[1] >= this->Places[j].X[0])) ||
              (this->Dir > 0 && placement.Y[0] < this->Places[j].Y[1] &&
               (i > j ?
                placement.X[0] <= this->Places[j].X[1] :
                placement.X[1] >= this->Places[j].X[0])))
          {
            placement.Y[0] = this->Places[j].Y[1] + this->Dir * this->Pad;
          }
        }
        placement.Y[1] = placement.Y[0] + this->Dir * ht;
        // Vertical justification changes, but Y[0] is always anchor
        placement.Anchor[1] = placement.Y[0];
      }
    }
    ++this->NumPlaced;
  }

  void BreakLeader(
    vtkScalarBarHLabelInfo& label, double& curY, int j,
    vtkPoints* pts, vtkCellArray* lines, vtkUnsignedCharArray* colors,
    const vtkColor3ub& color
    )
  {
    vtkScalarBarHLabelInfo& other(this->Places[j]);
    vtkIdType pt;
    if (
        label.Anchor[0] > other.X[0] && label.Anchor[0] < other.X[1] &&
        ((this->Dir > 0 && label.Anchor[1] >= other.Y[0]) ||
         (this->Dir < 0 && label.Anchor[1] <= other.Y[0])
        )
       )
    {
      pt = pts->InsertNextPoint(label.Anchor[0], other.Y[0], 0.);
      lines->InsertCellPoint(pt);
      lines->InsertNextCell(2);
      colors->InsertNextTypedTuple(color.GetData());
      curY = other.Y[1];
      pt = pts->InsertNextPoint(label.Anchor[0], curY, 0.);
      lines->InsertCellPoint(pt);
    }
  }

  // Only called after all labels are placed
  void AddBrokenLeader(
    int lidx, vtkPoints* pts, vtkCellArray* lines,
    vtkUnsignedCharArray* colors, const vtkColor3ub& color)
  {
    vtkIdType pt;
    vtkScalarBarHLabelInfo& label(this->Places[lidx]);

    // I. Insert first vertex near swatch:
    lines->InsertNextCell(2);
    colors->InsertNextTypedTuple(color.GetData());
    double curY = this->Y0 + this->Dir * this->Pad / 2.;
    pt = pts->InsertNextPoint(label.Anchor[0], curY, 0.);
    lines->InsertCellPoint(pt);

    // II. Loop over all labels checking for interference.
    // Where found, close current line and start new one on the other side.
    int ic = static_cast<int>(this->Places.size()) / 2;
    int lf, rt;
    bool done = false;
    if (!this->HaveCtr)
    {
      lf = ic - 1;
      rt = ic;
    }
    else
    {
      lf = ic - 1;
      rt = ic + 1;
      if (lidx == ic)
      {
        done = true;
      }
      else
      {
        //cout << "Break " << lidx << " with " << ic << "\n";
        this->BreakLeader(label, curY, ic, pts, lines, colors, color);
      }
    }
    if (!done)
    {
      for (; lf >= 0; --lf, ++rt)
      {
        if (lf == lidx) break;
        //cout << "Break " << lidx << " with " << lf << "\n";
        this->BreakLeader(label, curY, lf, pts, lines, colors, color);
        if (rt == lidx) break;
        //cout << "Break " << lidx << " with " << rt << "\n";
        this->BreakLeader(label, curY, rt, pts, lines, colors, color);
      }
    }

    // III. Finally, close the open line segment with the label anchor point.
    pt = pts->InsertNextPoint(
      label.Anchor[0], label.Anchor[1] - this->Dir * this->Pad / 2., 0.);
    lines->InsertCellPoint(pt);
  }
};

//----------------------------------------------------------------------------
/**\brief Non-overlapping label placer for a horizontal array of swatches.
 *
 * A set of rules are enforced during layout:
 * <ul>
 * <li> Any label may be wider than the entire legend.
 * <li> The center label should be centered on the center swatch
 * <li> No other label should extend beyond the legend's matching lateral
 *   extent (i.e., a label to the left of center
 *   should never extend beyond the left bounds of its swatch).
 * <li> To enforce this, labels may be displaced vertically (distally) away
 *   from the legend.
 * <li> Broken leaders should be drawn connecting each displaced label to
 *   its swatch, with breaks where long labels from the centerline or beyond
 *   obstruct it.
 * </ul>
 *
 * The algorithm for performing the layout enforces these rules as follows:
 * Labels are placed starting with the central (medial) label
 * and moving outwards;
 * this provides a consistent placement as the actor is resized.
 * First the horizontal label position is determined by examining the
 * width of the label and the extents of its medial neighbor (which will
 * have been placed already).
 * The vertical displacement is then computed by either copying the medial
 * neighbor's displacement (if no interference with the neighbor was required)
 * or incrementing the displacement beyond its immediate
 * neighbor and checking all other relevant labels for intereference.
 */
int vtkScalarBarActor::PlaceAnnotationsHorizontally(
  double barX, double barY,
  double barWidth, double barHeight,
  double vtkNotUsed(delta), double pad)
{
  if (!this->LookupTable)
  {
    return 0;
  }

#define VTK_ANN_HLAYOUT(j, placer) \
  this->P->AnnotationLabels[j]->GetTextProperty()\
  ->SetJustification(placer.Places[j].Justification); \
  this->P->AnnotationLabels[j]->GetTextProperty()\
  ->SetVerticalJustification(placer.Dir > 0 ? \
    VTK_TEXT_BOTTOM : VTK_TEXT_TOP); \
  this->P->AnnotationLabels[j]->SetPosition(placer.Places[j].Anchor); \
  placer.AddBrokenLeader(j, lpts, llines, llcolors, \
                         this->P->AnnotationColors[j]);

  int numNotes = static_cast<int>(this->P->AnnotationLabels.size());
  bool precede = this->TextPosition == vtkScalarBarActor::PrecedeScalarBar;
  vtkScalarBarHLabelPlacer placer(
    numNotes, precede ? barY : barY - barHeight, precede ? 1 : -1,
    barX, barX + barWidth, pad, this->AnnotationLeaderPadding);

  vtkPoints* lpts = vtkPoints::New();
  vtkCellArray* llines = vtkCellArray::New();
  vtkUnsignedCharArray* llcolors = vtkUnsignedCharArray::New();
  llcolors->SetName("Leader Line Color");
  llcolors->SetNumberOfComponents(3);
  llcolors->Allocate(numNotes * numNotes);
  // TODO: Improve estimates, but we don't know how many breaks there will be:
  lpts->Allocate(numNotes * numNotes);
  llines->Allocate(llines->EstimateSize(numNotes * numNotes, 2));
  this->P->AnnotationLeaders->Initialize();
  this->P->AnnotationLeaders->SetPoints(lpts);
  this->P->AnnotationLeaders->SetLines(llines);
  if (this->FixedAnnotationLeaderLineColor)
  {
    this->P->AnnotationLeaders->GetCellData()->SetScalars(NULL);
  }
  else
  {
    this->P->AnnotationLeaders->GetCellData()->SetScalars(llcolors);
  }

  // Start at the center and move outward (both up and down),
  // accumulating label displacement as we go.
  int ic = numNotes / 2;
  int lf, rt;
  double tsz[2];
  vtkColor3ub leaderColor;
  if (2 * ic == numNotes)
  {
    lf = ic - 1;
    rt = ic;
  }
  else
  {
    lf = ic - 1;
    rt = ic + 1;
    this->P->AnnotationLabels[ic]->GetSize(this->P->Viewport, tsz);
    placer.Place(
      ic, this->P->AnnotationAnchors[ic], tsz[0], tsz[1]);
    VTK_ANN_HLAYOUT(ic, placer);
  }
  for (; lf >= 0; --lf, ++rt)
  {
    this->P->AnnotationLabels[lf]->GetSize(this->P->Viewport, tsz);
    placer.Place(
      lf, this->P->AnnotationAnchors[lf], tsz[0], tsz[1]);
    VTK_ANN_HLAYOUT(lf, placer);
    this->P->AnnotationLabels[rt]->GetSize(this->P->Viewport, tsz);
    placer.Place(
      rt, this->P->AnnotationAnchors[rt], tsz[0], tsz[1]);
    VTK_ANN_HLAYOUT(rt, placer);
  }

  lpts->Delete();
  llines->Delete();
  llcolors->Delete();
  return numNotes;
}
