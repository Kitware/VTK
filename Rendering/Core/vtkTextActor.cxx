/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTextActor.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkTextActor.h"

#include "vtkCellArray.h"
#include "vtkFloatArray.h"
#include "vtkImageData.h"
#include "vtkMath.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper2D.h"
#include "vtkProperty2D.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkTextProperty.h"
#include "vtkTextRenderer.h"
#include "vtkTexture.h"
#include "vtkTransform.h"
#include "vtkViewport.h"
#include "vtkWindow.h"

#include <algorithm>

vtkObjectFactoryNewMacro(vtkTextActor)

// ----------------------------------------------------------------------------
vtkTextActor::vtkTextActor()
{
  // To remain compatible with code using vtkActor2D, we must set
  // position coord to Viewport, not Normalized Viewport
  // so...compute equivalent coords for initial position
  this->PositionCoordinate->SetCoordinateSystemToViewport();

  // This initializes the rectangle structure.
  // It will be used to display the text image as a texture map.
  this->Rectangle = vtkPolyData::New();
  this->RectanglePoints = vtkPoints::New();
  // The actual corner points of the rectangle will be computed later.
  this->Rectangle->SetPoints(this->RectanglePoints);
  vtkCellArray* polys = vtkCellArray::New();
  polys->InsertNextCell(4);
  polys->InsertCellPoint(0);
  polys->InsertCellPoint(1);
  polys->InsertCellPoint(2);
  polys->InsertCellPoint(3);
  this->Rectangle->SetPolys(polys);
  polys->Delete();
  vtkFloatArray* tc = vtkFloatArray::New();
  tc->SetNumberOfComponents(2);
  tc->SetNumberOfTuples(4);
  tc->InsertComponent(0,0, 0.0);  tc->InsertComponent(0,1, 0.0);
  tc->InsertComponent(1,0, 0.0);  tc->InsertComponent(1,1, 1.0);
  tc->InsertComponent(2,0, 1.0);  tc->InsertComponent(2,1, 1.0);
  tc->InsertComponent(3,0, 1.0);  tc->InsertComponent(3,1, 0.0);
  this->Rectangle->GetPointData()->SetTCoords(tc);
  tc->Delete();

  this->ImageData = vtkImageData::New();
  vtkTexture* texture = vtkTexture::New();
  texture->SetInputData(this->ImageData);
  this->SetTexture(texture);
  texture->Delete();

  vtkPolyDataMapper2D *mapper = vtkPolyDataMapper2D::New();
  this->SetMapper(mapper);
  mapper->SetInputData(this->Rectangle);
  mapper->Delete();

  this->TextProperty = vtkTextProperty::New();
  this->ScaledTextProperty = vtkTextProperty::New();
  this->Transform = vtkTransform::New();

  this->LastOrigin[0]     = 0;
  this->LastOrigin[1]     = 0;

  this->LastSize[0]       = 0;
  this->LastSize[1]       = 0;

  this->MinimumSize[0]    = 10;
  this->MinimumSize[1]    = 10;

  this->MaximumLineHeight = 1.0;
  this->TextScaleMode     = TEXT_SCALE_MODE_NONE;
  this->Orientation       = 0.0;
  this->UseBorderAlign    = 0;

  this->FontScaleExponent = 1;

  this->Input = nullptr;
  this->InputRendered = false;

  this->FormerOrientation = 0.0;
  this->RenderedDPI = 0;

  this->TextRenderer = vtkTextRenderer::GetInstance();
  if (!this->TextRenderer)
  {
    vtkErrorMacro(<<"Failed getting the TextRenderer instance!");
  }
}

// ----------------------------------------------------------------------------
vtkTextActor::~vtkTextActor()
{
  this->ImageData->Delete();
  this->Transform->Delete();
  this->SetTextProperty(nullptr);
  this->ScaledTextProperty->Delete();
  this->ScaledTextProperty = nullptr;
  delete [] this->Input;
  this->Rectangle->Delete();
  this->Rectangle = nullptr;
  this->RectanglePoints->Delete();
  this->RectanglePoints = nullptr;
  this->SetTexture(nullptr);
}

// ----------------------------------------------------------------------------
void vtkTextActor::GetBoundingBox(
  vtkViewport* vport, double bbox[4])
{
  if (this->UpdateRectangle(vport) && this->RectanglePoints &&
    this->RectanglePoints->GetNumberOfPoints() >= 4)
  {
    double x[3];
    this->RectanglePoints->GetPoint( 0, x );
    bbox[0] = bbox[1] = x[0];
    bbox[2] = bbox[3] = x[1];
    for (int i = 1; i < this->RectanglePoints->GetNumberOfPoints(); ++i)
    {
      this->RectanglePoints->GetPoint( i, x );

      if ( bbox[0] > x[0] )
        bbox[0] = x[0];
      else if ( bbox[1] < x[0] )
        bbox[1] = x[0];

      if ( bbox[2] > x[1] )
        bbox[2] = x[1];
      else if ( bbox[3] < x[1] )
        bbox[3] = x[1];
    }
    // Use pixel centers rather than pixel corners for the coordinates.
    --bbox[1];
    --bbox[3];
  }
  else
  {
    vtkErrorMacro("UpdateRectangle failed to do so");
  }
}

//----------------------------------------------------------------------------
void vtkTextActor::GetSize(vtkViewport* vport, double size[2])
{
  double bds[4];
  // If we have a viewport, use it. Otherwise, GetBoundingBox() calls
  // UpdateRectange(nullptr) which builds a (probably-too-low-resolution) image
  // to determine its size.
  this->UpdateRectangle(vport);
  this->GetBoundingBox(vport, bds);
  size[0] = bds[1] - bds[0];
  size[1] = bds[3] - bds[2];
}

//----------------------------------------------------------------------------
int vtkTextActor::SetConstrainedFontSize(vtkViewport* viewport,
                                          int targetWidth,
                                          int targetHeight)
{
  return this->SetConstrainedFontSize(this, viewport, targetWidth,
                                      targetHeight);
}


//----------------------------------------------------------------------------
int vtkTextActor::SetConstrainedFontSize(
  vtkTextActor* tactor, vtkViewport* viewport,
  int targetWidth, int targetHeight)
{
  // If target "empty" just return
  if (targetWidth == 0 && targetHeight == 0)
  {
    return 0;
  }

  vtkTextProperty* tprop = tactor->GetTextProperty();
  if (!tprop)
  {
    vtkGenericWarningMacro(<<"Need text property to apply constraint");
    return 0;
  }
  int fontSize = tprop->GetFontSize();

  // Use the last size as a first guess
  double tempi[2];
  tactor->GetSize(viewport, tempi);

  // Now get an estimate of the target font size using bissection
  // Based on experimentation with big and small font size increments,
  // ceil() gives the best result.
  // big:   floor: 10749, ceil: 10106, cast: 10749, vtkMath::Round: 10311
  // small: floor: 12122, ceil: 11770, cast: 12122, vtkMath::Round: 11768
  // I guess the best optim would be to have a look at the shape of the
  // font size growth curve (probably not that linear)
  if (tempi[0] > 0.5 && tempi[1] > 0.5)
  {
    float fx = targetWidth / static_cast<float>(tempi[0]);
    float fy = targetHeight / static_cast<float>(tempi[1]);
    fontSize = static_cast<int>(ceil(fontSize * ((fx <= fy) ? fx : fy)));
    fontSize = fontSize > 2 ? fontSize : 2;
    tprop->SetFontSize(fontSize);
    tactor->GetSize(viewport, tempi);
  }

  // While the size is too small increase it
  while (tempi[1] <= targetHeight &&
         tempi[0] <= targetWidth &&
         fontSize < 100)
  {
    fontSize++;
    tprop->SetFontSize(fontSize);
    tactor->GetSize(viewport, tempi);
  }

  // While the size is too large decrease it..  but never below 2 pt.
  // (The MathText rendering uses matplotlib which behaves poorly
  // for very small fonts.)
  while ((tempi[1] > targetHeight || tempi[0] > targetWidth)
         && fontSize > 3)
  {
    fontSize--;
    tprop->SetFontSize(fontSize);
    tactor->GetSize(viewport, tempi);
  }

  return fontSize;
}

//----------------------------------------------------------------------------
int vtkTextActor::SetMultipleConstrainedFontSize(
  vtkViewport* viewport, int targetWidth, int targetHeight,
  vtkTextActor** actors, int nbOfActors, int* maxResultingSize)
{
  maxResultingSize[0] = maxResultingSize[1] = 0;

  if (nbOfActors == 0)
  {
    return 0;
  }

  int fontSize, aSize;

  // First try to find the constrained font size of the first mapper: it
  // will be used minimize the search for the remaining actors, given the
  // fact that all actors are likely to have the same constrained font size.
  int i, first;
  for (first = 0; first < nbOfActors && !actors[first]; first++) {}

  if (first >= nbOfActors)
  {
    return 0;
  }

  fontSize = actors[first]->SetConstrainedFontSize(
    viewport, targetWidth, targetHeight);

  // Find the constrained font size for the remaining actors and
  // pick the smallest
  for (i = first + 1; i < nbOfActors; i++)
  {
    if (actors[i])
    {
      actors[i]->GetTextProperty()->SetFontSize(fontSize);
      aSize = actors[i]->SetConstrainedFontSize(
        viewport, targetWidth, targetHeight);
      if (aSize < fontSize)
      {
        fontSize = aSize;
      }
    }
  }

  // Assign the smallest size to all text actors and find the largest area
  double tempi[2];
  for (i = first; i < nbOfActors; i++)
  {
    if (actors[i])
    {
      actors[i]->GetTextProperty()->SetFontSize(fontSize);
      actors[i]->GetSize(viewport, tempi);
      if (tempi[0] > maxResultingSize[0])
      {
        maxResultingSize[0] = static_cast<int>(tempi[0]);
      }
      if (tempi[1] > maxResultingSize[1])
      {
        maxResultingSize[1] = static_cast<int>(tempi[1]);
      }
    }
  }

  // The above code could be optimized further since the actor
  // labels are likely to have the same height: in that case, we could
  // have searched for the largest label, found the constrained size
  // for this one, then applied this size to all others.  But who
  // knows, maybe one day the text property will support a text
  // orientation/rotation, and in that case the height will vary.
  return fontSize;
}


// ----------------------------------------------------------------------------
void vtkTextActor::SetNonLinearFontScale(double exp, int tgt)
{
  if (   (this->FontScaleExponent == exp)
      && (this->TextProperty->GetFontSize() == tgt) )
  {
    return;
  }
  this->FontScaleExponent = exp;
  this->TextProperty->SetFontSize(tgt);
  this->Modified();
}

// ----------------------------------------------------------------------------
bool vtkTextActor::RenderImage(vtkTextProperty *tprop, vtkViewport *vp)
{
  vtkStdString text;
  if (this->Input && this->Input[0])
  {
    text = this->Input;
  }

  vtkWindow *win = vp->GetVTKWindow();
  if (!win)
  {
    vtkErrorMacro(<<"No render window available: cannot determine DPI.");
    return false;
  }

  return this->TextRenderer->RenderString(tprop, text, this->ImageData,
                                          nullptr, win->GetDPI());
}

// ----------------------------------------------------------------------------
bool vtkTextActor::GetImageBoundingBox(vtkTextProperty *tprop, vtkViewport *vp,
                                       int bbox[4])
{
  vtkStdString text;
  if (this->Input && this->Input[0])
  {
    text = this->Input;
  }

  vtkWindow *win = vp->GetVTKWindow();
  if (!win)
  {
    vtkErrorMacro(<<"No render window available: cannot determine DPI.");
    return false;
  }
  return this->TextRenderer->GetBoundingBox(tprop, text, bbox, win->GetDPI());
}

// ----------------------------------------------------------------------------
void vtkTextActor::SetInput(const char* str)
{
  if(!str)
  {
    str = "";
  }
  if(this->Input)
  {
    if(strcmp(this->Input, str) == 0)
    {
      return;
    }
    delete [] this->Input;
  }
  this->Input = new char[strlen(str)+1];
  strcpy(this->Input, str);
  this->InputRendered = false;
  this->Modified();
}

// ----------------------------------------------------------------------------
char* vtkTextActor::GetInput()
{
  return this->Input;
}

// ----------------------------------------------------------------------------
void vtkTextActor::SetTextProperty(vtkTextProperty *p)
{
  if ( this->TextProperty == p )
  {
    return;
  }
  if ( this->TextProperty )
  {
    this->TextProperty->UnRegister( this );
    this->TextProperty = nullptr;
  }
  this->TextProperty = p;
  if (this->TextProperty)
  {
    this->TextProperty->Register(this);
    this->ScaledTextProperty->ShallowCopy(this->TextProperty);
  }
  this->Modified();
}

// ----------------------------------------------------------------------------
void vtkTextActor::ShallowCopy(vtkProp *prop)
{
  vtkTextActor *a = vtkTextActor::SafeDownCast(prop);
  if ( a != nullptr )
  {
    this->SetPosition2(a->GetPosition2());
    this->SetMinimumSize(a->GetMinimumSize());
    this->SetMaximumLineHeight(a->GetMaximumLineHeight());
    this->SetTextScaleMode(a->GetTextScaleMode());
    this->SetTextProperty(a->GetTextProperty());
    this->SetInput(a->GetInput());
  }
  // Now do superclass (mapper is handled by it as well).
  this->Superclass::ShallowCopy(prop);
}

// ----------------------------------------------------------------------------
// Release any graphics resources that are being consumed by this actor.
// The parameter window could be used to determine which graphic
// resources to release.
void vtkTextActor::ReleaseGraphicsResources(vtkWindow *win)
{
  this->Superclass::ReleaseGraphicsResources(win);
}

// ----------------------------------------------------------------------------
int vtkTextActor::RenderOverlay(vtkViewport *viewport)
{
  if (!this->Visibility || !this->Input || !this->Input[0])
  {
    return 0;
  }

  // Everything is built in RenderOpaqueGeometry, just have to render
  int renderedSomething = this->Superclass::RenderOverlay(viewport);
  return renderedSomething;
}

// ----------------------------------------------------------------------------
int vtkTextActor::RenderOpaqueGeometry(vtkViewport *viewport)
{
  if (!this->Visibility)
  {
    return 0;
  }

  //Make sure we have a string to render
  if(!this->Input || this->Input[0] == '\0')
  {
    return 0;
  }

  int *vSize = viewport->GetSize();
  //vSize == (0,0) means that we're not ready to render yet
  if(vSize[0] == 0 && vSize[1] == 0)
  {
    return 0;
  }
  //not sure what vSize == 1 means, but it can cause divide-by-zero errors
  //in some of the coordinate conversion methods used below
  if(vSize[0] == 1 || vSize[1] == 1)
  {
    return 0;
  }

  if ( ! this->UpdateRectangle(viewport) )
  {
    return 0;
  }

  // Everything is built, just have to render
  // but we do not render opaque geometry so return 0
  return 0; // this->Superclass::RenderOpaqueGeometry(viewport);
}

//-----------------------------------------------------------------------------
// Description:
// Does this prop have some translucent polygonal geometry?
int vtkTextActor::HasTranslucentPolygonalGeometry()
{
  return 0;
}

// ----------------------------------------------------------------------------
void vtkTextActor::SetOrientation(float orientation)
{
  if (this->Orientation == orientation)
  {
    return;
  }
  this->Modified();
  this->Orientation = orientation;
}


// ----------------------------------------------------------------------------
int vtkTextActor::GetAlignmentPoint()
{
  int alignmentCode = 0;

  if ( ! this->TextProperty)
  {
    return 0;
  }
  switch (this->TextProperty->GetJustification())
  {
    case VTK_TEXT_LEFT:
      alignmentCode = 0;
      break;
    case VTK_TEXT_CENTERED:
      alignmentCode = 1;
      break;
    case VTK_TEXT_RIGHT:
      alignmentCode = 2;
      break;
    default:
      vtkErrorMacro(<<"Unknown justifaction code.");
  }
   switch (this->TextProperty->GetVerticalJustification())
   {
    case VTK_TEXT_BOTTOM:
      alignmentCode += 0;
      break;
    case VTK_TEXT_CENTERED:
      alignmentCode += 3;
      break;
    case VTK_TEXT_TOP:
      alignmentCode += 6;
      break;
    default:
      vtkErrorMacro(<<"Unknown justifaction code.");
   }
  return alignmentCode;
}

// ----------------------------------------------------------------------------
void vtkTextActor::SetAlignmentPoint(int val)
{
  vtkWarningMacro(<< "Alignment point is being deprecated.  You should use "
                  << "SetJustification and SetVerticalJustification in the text property.");

  switch (val)
  {
    case 0:
      this->TextProperty->SetJustificationToLeft();
      this->TextProperty->SetVerticalJustificationToBottom();
      break;
    case 1:
      this->TextProperty->SetJustificationToCentered();
      this->TextProperty->SetVerticalJustificationToBottom();
      break;
    case 2:
      this->TextProperty->SetJustificationToRight();
      this->TextProperty->SetVerticalJustificationToBottom();
      break;
    case 3:
      this->TextProperty->SetJustificationToLeft();
      this->TextProperty->SetVerticalJustificationToCentered();
      break;
    case 4:
      this->TextProperty->SetJustificationToCentered();
      this->TextProperty->SetVerticalJustificationToCentered();
      break;
    case 5:
      this->TextProperty->SetJustificationToRight();
      this->TextProperty->SetVerticalJustificationToCentered();
      break;
    case 6:
      this->TextProperty->SetJustificationToLeft();
      this->TextProperty->SetVerticalJustificationToTop();
      break;
    case 7:
      this->TextProperty->SetJustificationToCentered();
      this->TextProperty->SetVerticalJustificationToTop();
      break;
    case 8:
      this->TextProperty->SetJustificationToRight();
      this->TextProperty->SetVerticalJustificationToTop();
      break;
  }
}

//-----------------------------------------------------------------------------
float vtkTextActor::GetFontScale(vtkViewport *viewport)
{
  int *viewportSize = viewport->GetSize();

  // Pretend the long dimension is the "width"
  int viewportWidth
    = (viewportSize[0] > viewportSize[1]) ? viewportSize[0] : viewportSize[1];

  // Scale based on the assumtion of a 6 inch wide image at 72 DPI.
  return static_cast<double>(viewportWidth)/(6*72);
}

//-----------------------------------------------------------------------------
void vtkTextActor::ComputeScaledFont(vtkViewport *viewport)
{
  if (this->ScaledTextProperty->GetMTime() < this->TextProperty->GetMTime())
  {
    this->ScaledTextProperty->ShallowCopy(this->TextProperty);
  }

  // Combine this actor's orientation with the set text property's rotation
  double rotAngle = this->TextProperty->GetOrientation() + this->Orientation;
  this->ScaledTextProperty->SetOrientation(rotAngle);

  if (this->TextScaleMode == TEXT_SCALE_MODE_NONE)
  {
    if (this->TextProperty)
    {
      this->ScaledTextProperty->SetFontSize(this->TextProperty->GetFontSize());
    }
    return;
  }

  if (this->TextScaleMode == TEXT_SCALE_MODE_VIEWPORT)
  {
    if (   (viewport->GetMTime() > this->BuildTime)
        || (   viewport->GetVTKWindow()
            && (viewport->GetVTKWindow()->GetMTime() > this->BuildTime) )
        || (   this->TextProperty
            && (this->TextProperty->GetMTime() > this->BuildTime) ) )
    {
      double requestedSize
        = static_cast<double>(this->TextProperty->GetFontSize());
      double scale = static_cast<double>(vtkTextActor::GetFontScale(viewport));
      double targetSize = scale*requestedSize;
      // Apply non-linear scaling
      int fsize
        = static_cast<int>(  pow(targetSize, this->FontScaleExponent)
                           * pow(requestedSize, 1.0-this->FontScaleExponent) );
      this->ScaledTextProperty->SetFontSize(fsize);
    }
    return;
  }

  //Scaled text case.  We need to be sure that our text will fit
  //inside the specified boundaries
  if(this->TextScaleMode == TEXT_SCALE_MODE_PROP)
  {
    int size[2], *point1, *point2;
    point1 = this->PositionCoordinate->GetComputedViewportValue(viewport);
    point2 = this->Position2Coordinate->GetComputedViewportValue(viewport);
    size[0] = point2[0] - point1[0];
    size[1] = point2[1] - point1[1];

    // Check to see whether we have to rebuild everything
    int positionsHaveChanged = 0;
    int orientationHasChanged = 0;
    //First check the obvious, am I changed (e.g., text changed)
    if ( this->GetMTime() > this->BuildTime )
    {
      positionsHaveChanged = 1; // short circuit the work of checking positions, etc.
    }
    else
    {
      if (   viewport->GetMTime() > this->BuildTime
          || (   viewport->GetVTKWindow()
              && viewport->GetVTKWindow()->GetMTime() > this->BuildTime ) )
      {
        // if the viewport has changed we may - or may not need
        // to rebuild, it depends on if the projected coords change
        if (   (this->LastSize[0]   != size[0])
            || (this->LastSize[1]   != size[1])
            || (this->LastOrigin[0] != point1[0])
            || (this->LastOrigin[1] != point1[1]) )
        {
          positionsHaveChanged = 1;
        }
      }

      // If the orientation has changed then we'll probably need to change our
      // constrained font size as well
      if(this->FormerOrientation != rotAngle)
      {
        this->FormerOrientation = rotAngle;
        orientationHasChanged = 1;
      }
    }

    // Check to see whether we have to rebuild everything
    if (   positionsHaveChanged || orientationHasChanged
        || (this->Mapper && this->Mapper->GetMTime() > this->BuildTime)
        || (   this->TextProperty
            && (this->TextProperty->GetMTime() > this->BuildTime) ) )
    {
      vtkDebugMacro(<<"Rebuilding text");

      this->LastOrigin[0] = point1[0];
      this->LastOrigin[1] = point1[1];

      //  Lets try to minimize the number of times we change the font size.
      //  If the width of the font box has not changed by more than a pixel
      // (numerical issues) do not recompute font size. Also, if the text
      // string has changed then we have to change font size.
      if ( (this->Mapper && this->Mapper->GetMTime() > this->BuildTime) ||
           (this->Mapper && this->GetMTime() > this->Mapper->GetMTime()) ||
           (this->TextProperty
            && (this->TextProperty->GetMTime() > this->BuildTime) ) ||
           (this->LastSize[0] < size[0]-1) || (this->LastSize[1] < size[1]-1) ||
           (this->LastSize[0] > size[0]+1) || (this->LastSize[1] > size[1]+1) ||
           orientationHasChanged)
      {
        this->LastSize[0] = size[0];
        this->LastSize[1] = size[1];

        // limit by minimum size
        if (this->MinimumSize[0] > size[0])
        {
          size[0] = this->MinimumSize[0];
        }
        if (this->MinimumSize[1] > size[1])
        {
          size[1] = this->MinimumSize[1];
        }
        int max_height = static_cast<int>(this->MaximumLineHeight * size[1]);

        vtkWindow *win = viewport->GetVTKWindow();
        if (!win)
        {
          vtkErrorMacro(<<"No render window available: cannot determine DPI.");
          return;
        }

        int fsize = this->TextRenderer->GetConstrainedFontSize(
          this->Input, this->ScaledTextProperty, size[0],
          (size[1] < max_height ? size[1] : max_height), win->GetDPI());

        if (fsize == -1)
        {
          vtkWarningMacro(<<"Could not determine constrained font size for "
                          "string:\n\t'" << this->Input << "'\n. Resetting to "
                          "20pt.");
          fsize = 20;
        }

        // apply non-linear scaling
        fsize = static_cast<int>(
                     pow(static_cast<double>(fsize), this->FontScaleExponent)
                   * pow(static_cast<double>(this->TextProperty->GetFontSize()),
                         1.0 - this->FontScaleExponent));
        // and set the new font size
        this->ScaledTextProperty->SetFontSize(fsize);
      }
    }
    return;
  }

  vtkWarningMacro(<< "Unknown text scaling mode: " << this->TextScaleMode);
}

// ----------------------------------------------------------------------------
void vtkTextActor::ComputeRectangle(vtkViewport *viewport)
{
  int dims[2] = {0, 0};
  int anchorOffset[2] = {0, 0};

  this->RectanglePoints->Reset();
  if ( this->ImageData )
  {
    int p2dims[3];
    this->ImageData->GetDimensions(p2dims);
    int text_bbox[4];
    if (!this->GetImageBoundingBox(this->ScaledTextProperty, viewport, text_bbox))
    {
      vtkErrorMacro("Cannot compute bounding box.")
      return;
    }
    dims[0] = ( text_bbox[1] - text_bbox[0] + 1 );
    dims[1] = ( text_bbox[3] - text_bbox[2] + 1 );
    anchorOffset[0] = text_bbox[0];
    anchorOffset[1] = text_bbox[2];

    // compute TCoords.
    vtkFloatArray* tc = vtkArrayDownCast<vtkFloatArray>
      ( this->Rectangle->GetPointData()->GetTCoords() );
    float tcXMax, tcYMax;
    // Add a fudge factor to the texture coordinates to prevent the top
    // row of pixels from being truncated on some systems.
    tcXMax = std::min(1.0f, (dims[0] + 0.001f) / static_cast<float>(p2dims[0]));
    tcYMax = std::min(1.0f, (dims[1] + 0.001f) / static_cast<float>(p2dims[1]));
    tc->InsertComponent(0, 0, 0.0);
    tc->InsertComponent(0, 1, 0.0);

    tc->InsertComponent(1, 0, 0.0);
    tc->InsertComponent(1, 1, tcYMax);

    tc->InsertComponent(2, 0, tcXMax);
    tc->InsertComponent(2, 1, tcYMax);

    tc->InsertComponent(3, 0, tcXMax);
    tc->InsertComponent(3, 1, 0.0);
    tc->Modified();
  }

  double xo = 0.0, yo = 0.0;

  // When TextScaleMode is PROP, we justify text based on the rectangle
  // formed by Position & Position2 coordinates
  if( ( this->TextScaleMode == TEXT_SCALE_MODE_PROP ) || this->UseBorderAlign )
  {
    double position1[3], position2[3];
    this->PositionCoordinate->GetValue( position1 );
    this->Position2Coordinate->GetValue( position2 );
    this->SpecifiedToDisplay( position1, viewport,
                              this->PositionCoordinate->GetCoordinateSystem() );
    this->SpecifiedToDisplay( position2, viewport,
                              this->Position2Coordinate->GetCoordinateSystem() );
    double maxWidth = position2[0] - position1[0];
    double maxHeight = position2[1] - position1[1];
    // I could get rid of "GetAlignmentPoint" and use justification directly.
    switch( this->GetAlignmentPoint() )
    {
      case 0:
        break;
      case 1:
        xo = ( maxWidth - dims[0] ) * 0.5;
        break;
      case 2:
        xo = ( maxWidth - dims[0] );
        break;
      case 3:
        yo = ( maxHeight - dims[1] ) * 0.5;
        break;
      case 4:
        xo = ( maxWidth - dims[0] ) * 0.5;
        yo = ( maxHeight - dims[1] ) * 0.5;
        break;
      case 5:
        xo = ( maxWidth - dims[0] );
        yo = ( maxHeight - dims[1] ) * 0.5;
        break;
      case 6:
        yo = ( maxHeight - dims[1] );
        break;
      case 7:
        xo = ( maxWidth - dims[0] ) * 0.5;
        yo = ( maxHeight - dims[1] );
        break;
      case 8:
        xo = ( maxWidth - dims[0] );
        yo = ( maxHeight - dims[1] );
        break;
      default:
        vtkErrorMacro( << "Bad alignment point value." );
    }
  }
  else
  {
    xo = anchorOffset[0];
    yo = anchorOffset[1];
  } //end unscaled text case

  this->RectanglePoints->SetNumberOfPoints(4);
  this->RectanglePoints->SetPoint(0, xo,           yo,           0.0);
  this->RectanglePoints->SetPoint(1, xo,           yo + dims[1], 0.0);
  this->RectanglePoints->SetPoint(2, xo + dims[0], yo + dims[1], 0.0);
  this->RectanglePoints->SetPoint(3, xo + dims[0], yo,           0.0);
}

// ----------------------------------------------------------------------------
int vtkTextActor::UpdateRectangle(vtkViewport* viewport)
{
  if (this->TextProperty->GetMTime() > this->ScaledTextProperty->GetMTime() ||
      this->GetMTime() > this->BuildTime)
  {
    this->ComputeScaledFont(viewport);
  }

  vtkWindow *win = viewport->GetVTKWindow();
  if (!win)
  {
    vtkErrorMacro(<<"No render window available: cannot determine DPI.");
    return 0;
  }

  //check if we need to render the string
  if(this->ScaledTextProperty->GetMTime() > this->BuildTime ||
    !this->InputRendered || this->GetMTime() > this->BuildTime ||
     this->RenderedDPI != win->GetDPI())
  {
    if(!this->RenderImage(this->ScaledTextProperty, viewport))
    {
      vtkErrorMacro(<<"Failed rendering text to buffer");
      return 0;
    }

    // Check if we need to create a new rectangle.
    // Need to check if angle has changed.
    //justification and line offset are handled in ComputeRectangle
    this->ComputeRectangle(viewport);

    this->ImageData->Modified();
    this->Texture->SetInputData(this->ImageData);
    this->Texture->Modified();
    this->InputRendered = true;
    this->RenderedDPI = win->GetDPI();
    this->BuildTime.Modified();
  }
  return 1;
}

// ----------------------------------------------------------------------------
void vtkTextActor::SpecifiedToDisplay(double *pos, vtkViewport *vport,
                                      int specified)
{
  if ( ! vport )
  {
    return;
  }

  switch(specified)
  {
  case VTK_WORLD:
    vport->WorldToView(pos[0], pos[1], pos[2]);
    VTK_FALLTHROUGH;
  case VTK_VIEW:
    vport->ViewToNormalizedViewport(pos[0], pos[1], pos[2]);
    VTK_FALLTHROUGH;
  case VTK_NORMALIZED_VIEWPORT:
    vport->NormalizedViewportToViewport(pos[0], pos[1]);
    VTK_FALLTHROUGH;
  case VTK_VIEWPORT:
    vport->ViewportToNormalizedDisplay(pos[0], pos[1]);
    VTK_FALLTHROUGH;
  case VTK_NORMALIZED_DISPLAY:
    vport->NormalizedDisplayToDisplay(pos[0], pos[1]);
    VTK_FALLTHROUGH;
  case VTK_DISPLAY:
    break;
  }
}

// ----------------------------------------------------------------------------
void vtkTextActor::DisplayToSpecified(double *pos, vtkViewport *vport,
                                      int specified)
{
  switch(specified)
  {
    case VTK_WORLD:
      vport->DisplayToNormalizedDisplay(pos[0], pos[1]);
      vport->NormalizedDisplayToViewport(pos[0], pos[1]);
      vport->ViewportToNormalizedViewport(pos[0], pos[1]);
      vport->NormalizedViewportToView(pos[0], pos[1], pos[2]);
      vport->ViewToWorld(pos[0], pos[1], pos[2]);
      break;
    case VTK_VIEW:
      vport->DisplayToNormalizedDisplay(pos[0], pos[1]);
      vport->NormalizedDisplayToViewport(pos[0], pos[1]);
      vport->ViewportToNormalizedViewport(pos[0], pos[1]);
      vport->NormalizedViewportToView(pos[0], pos[1], pos[2]);
      break;
    case VTK_NORMALIZED_VIEWPORT:
      vport->DisplayToNormalizedDisplay(pos[0], pos[1]);
      vport->NormalizedDisplayToViewport(pos[0], pos[1]);
      vport->ViewportToNormalizedViewport(pos[0], pos[1]);
      break;
    case VTK_VIEWPORT:
      vport->DisplayToNormalizedDisplay(pos[0], pos[1]);
      vport->NormalizedDisplayToViewport(pos[0], pos[1]);
      break;
    case VTK_NORMALIZED_DISPLAY:
      vport->DisplayToNormalizedDisplay(pos[0], pos[1]);
      break;
    case VTK_DISPLAY:
      break;
  }
}

// ----------------------------------------------------------------------------
void vtkTextActor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  if (this->Input)
  {
    os << indent << "Input: " << this->Input << endl;
  }
  else
  {
    os << indent << "Input: (none)\n";
  }

  if (this->TextProperty)
  {
    os << indent << "Text Property:\n";
    this->TextProperty->PrintSelf(os,indent.GetNextIndent());
  }
  else
  {
    os << indent << "Text Property: (none)\n";
  }

  os << indent << "Scaled Text Property:\n";
  this->ScaledTextProperty->PrintSelf(os, indent.GetNextIndent());

  os << indent << "MaximumLineHeight: " << this->MaximumLineHeight << endl;
  os << indent << "MinimumSize: " << this->MinimumSize[0] << " " << this->MinimumSize[1] << endl;
  os << indent << "TextScaleMode: " << this->TextScaleMode << endl;
  os << indent << "Orientation: " << this->Orientation << endl;
  os << indent << "FontScaleExponent: " << this->FontScaleExponent << endl;
  os << indent << "UseBorderAlign: " << this->UseBorderAlign << "\n";
}
