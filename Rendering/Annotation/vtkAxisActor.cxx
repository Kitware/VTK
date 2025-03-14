// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkAxisActor.h"

#include "vtkAxisFollower.h"
#include "vtkCamera.h"
#include "vtkCellArray.h"
#include "vtkCoordinate.h"
#include "vtkFollower.h"
#include "vtkMath.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProp3DAxisFollower.h"
#include "vtkProperty.h"
#include "vtkProperty2D.h"
#include "vtkRenderer.h"
#include "vtkStringArray.h"
#include "vtkTextActor.h"
#include "vtkTextActor3D.h"
#include "vtkTextProperty.h"
#include "vtkTextRenderer.h"
#include "vtkTransform.h"
#include "vtkVectorText.h"
#include "vtkViewport.h"
#include "vtkWindow.h"

#define VTK_MAX_TICKS 1000

#include <sstream>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkAxisActor);
vtkCxxSetSmartPointerMacro(vtkAxisActor, Camera, vtkCamera);
vtkCxxSetSmartPointerMacro(vtkAxisActor, LabelTextProperty, vtkTextProperty);
vtkCxxSetSmartPointerMacro(vtkAxisActor, TitleTextProperty, vtkTextProperty);

//------------------------------------------------------------------------------
// Instantiate this object.
//------------------------------------------------------------------------------

vtkAxisActor::vtkAxisActor()
{
  this->Point1Coordinate->SetCoordinateSystemToWorld();
  this->Point1Coordinate->SetValue(0.0, 0.0, 0.0);

  this->Point2Coordinate->SetCoordinateSystemToWorld();
  this->Point2Coordinate->SetValue(0.75, 0.0, 0.0);

  this->Bounds[0] = this->Bounds[2] = this->Bounds[4] = -1;
  this->Bounds[1] = this->Bounds[3] = this->Bounds[5] = 1;

  this->LabelFormat = new char[8];
  snprintf(this->LabelFormat, 8, "%s", "%-#6.3g");

  this->TitleTextProperty = vtkSmartPointer<vtkTextProperty>::New();
  this->TitleTextProperty->SetColor(0., 0., 0.);
  this->TitleTextProperty->SetFontFamilyToArial();
  this->TitleTextProperty->SetFontSize(18.);
  this->TitleTextProperty->SetVerticalJustificationToCentered();
  this->TitleTextProperty->SetJustificationToCentered();

  this->TitleMapper->SetInputConnection(this->TitleVector->GetOutputPort());
  this->TitleActor->SetAxis(this);
  this->TitleActor->SetMapper(this->TitleMapper);
  this->TitleActor->SetEnableDistanceLOD(0);

  this->TitleProp3D->SetAxis(this);
  this->TitleProp3D->SetEnableDistanceLOD(0);
  this->TitleProp3D->SetProp3D(this->TitleActor3D);

  this->LabelTextProperty = vtkSmartPointer<vtkTextProperty>::New();
  this->LabelTextProperty->SetColor(0., 0., 0.);
  this->LabelTextProperty->SetFontFamilyToArial();
  this->LabelTextProperty->SetFontSize(14.);
  this->LabelTextProperty->SetVerticalJustificationToBottom();
  this->LabelTextProperty->SetJustificationToLeft();

  this->ExponentMapper->SetInputConnection(this->ExponentVector->GetOutputPort());
  this->ExponentActor->SetAxis(this);
  this->ExponentActor->SetMapper(this->ExponentMapper);
  this->ExponentActor->SetEnableDistanceLOD(0);

  this->ExponentProp3D->SetAxis(this);
  this->ExponentProp3D->SetEnableDistanceLOD(0);
  this->ExponentProp3D->SetProp3D(this->ExponentActor3D);

  // Main line of axis
  this->AxisLinesMapper->SetInputData(this->AxisLines);
  this->AxisLinesActor->SetMapper(this->AxisLinesMapper);

  // Major ticks
  this->AxisMajorTicksMapper->SetInputData(this->AxisMajorTicks);
  this->AxisMajorTicksActor->SetMapper(this->AxisMajorTicksMapper);

  // Minor ticks
  this->AxisMinorTicksMapper->SetInputData(this->AxisMinorTicks);
  this->AxisMinorTicksActor->SetMapper(this->AxisMinorTicksMapper);

  this->GridlinesMapper->SetInputData(this->Gridlines);
  this->GridlinesActor->SetMapper(this->GridlinesMapper);
  this->InnerGridlinesMapper->SetInputData(this->InnerGridlines);
  this->InnerGridlinesActor->SetMapper(this->InnerGridlinesMapper);
  this->GridpolysMapper->SetInputData(this->Gridpolys);
  this->GridpolysActor->SetMapper(this->GridpolysMapper);
}

//------------------------------------------------------------------------------
vtkAxisActor::~vtkAxisActor()
{
  this->SetCamera(nullptr);

  delete[] this->LabelFormat;
  this->LabelFormat = nullptr;

  delete[] this->Title;
  this->Title = nullptr;

  delete[] this->Exponent;
  this->Exponent = nullptr;

  if (this->LabelMappers != nullptr)
  {
    for (int i = 0; i < this->NumberOfLabelsBuilt; i++)
    {
      this->LabelActors[i]->Delete();
      this->LabelProps3D[i]->Delete();
    }
    this->NumberOfLabelsBuilt = 0;
    delete[] this->LabelVectors;
    delete[] this->LabelMappers;
    delete[] this->LabelActors;
    delete[] this->LabelProps3D;
    delete[] this->LabelActors3D;
    delete[] this->LabelActors2D;
    this->LabelVectors = nullptr;
    this->LabelMappers = nullptr;
    this->LabelActors = nullptr;
    this->LabelProps3D = nullptr;
    this->LabelActors3D = nullptr;
    this->LabelActors2D = nullptr;
  }
}

//------------------------------------------------------------------------------
void vtkAxisActor::ReleaseGraphicsResources(vtkWindow* win)
{
  this->TitleActor->ReleaseGraphicsResources(win);
  this->TitleProp3D->ReleaseGraphicsResources(win);
  this->TitleActor3D->ReleaseGraphicsResources(win);
  this->TitleActor2D->ReleaseGraphicsResources(win);
  this->ExponentActor->ReleaseGraphicsResources(win);
  this->ExponentProp3D->ReleaseGraphicsResources(win);
  this->ExponentActor3D->ReleaseGraphicsResources(win);
  this->ExponentActor2D->ReleaseGraphicsResources(win);

  for (int i = 0; i < this->NumberOfLabelsBuilt; i++)
  {
    this->LabelActors[i]->ReleaseGraphicsResources(win);
    this->LabelProps3D[i]->ReleaseGraphicsResources(win);
    this->LabelActors3D[i]->ReleaseGraphicsResources(win);
    this->LabelActors2D[i]->ReleaseGraphicsResources(win);
  }
  this->AxisLinesActor->ReleaseGraphicsResources(win);
  this->AxisMajorTicksActor->ReleaseGraphicsResources(win);
  this->AxisMinorTicksActor->ReleaseGraphicsResources(win);

  this->GridlinesActor->ReleaseGraphicsResources(win);
  this->InnerGridlinesActor->ReleaseGraphicsResources(win);
  this->GridpolysActor->ReleaseGraphicsResources(win);
}

//------------------------------------------------------------------------------
int vtkAxisActor::RenderOpaqueGeometry(vtkViewport* viewport)
{
  int renderedSomething = 0;

  this->BuildAxis(viewport, false);

  // Everything is built, just have to render

  // pass keys to sub props
  vtkInformation* propKeys = this->GetPropertyKeys();

  if (!this->AxisHasZeroLength)
  {
    if (this->DrawGridlinesOnly && this->DrawGridlines)
    {
      // Exit !!!!
      this->GridlinesActor->SetPropertyKeys(propKeys);
      return this->GridlinesActor->RenderOpaqueGeometry(viewport);
    }
    if (this->Title != nullptr && this->Title[0] != 0 && this->TitleVisibility)
    {
      if (this->Use2DMode)
      {
        this->TitleActor2D->SetPropertyKeys(propKeys);
        renderedSomething += this->TitleActor2D->RenderOpaqueGeometry(viewport);
      }
      else if (this->UseTextActor3D)
      {
        this->TitleProp3D->SetPropertyKeys(propKeys);
        renderedSomething += this->TitleProp3D->RenderOpaqueGeometry(viewport);
      }
      else
      {
        this->TitleActor->SetPropertyKeys(propKeys);
        renderedSomething += this->TitleActor->RenderOpaqueGeometry(viewport);
      }
    }
    if (this->AxisVisibility)
    {
      this->AxisLinesActor->SetPropertyKeys(propKeys);
      renderedSomething += this->AxisLinesActor->RenderOpaqueGeometry(viewport);
      if (this->TickVisibility)
      {
        this->AxisMajorTicksActor->SetPropertyKeys(propKeys);
        renderedSomething += this->AxisMajorTicksActor->RenderOpaqueGeometry(viewport);
        this->AxisMinorTicksActor->SetPropertyKeys(propKeys);
        renderedSomething += this->AxisMinorTicksActor->RenderOpaqueGeometry(viewport);
      }
    }
    if (this->DrawGridlines)
    {
      this->GridlinesActor->SetPropertyKeys(propKeys);
      renderedSomething += this->GridlinesActor->RenderOpaqueGeometry(viewport);
    }
    if (this->DrawInnerGridlines)
    {
      this->InnerGridlinesActor->SetPropertyKeys(propKeys);
      renderedSomething += this->InnerGridlinesActor->RenderOpaqueGeometry(viewport);
    }
    if (this->LabelVisibility)
    {
      for (int i = 0; i < this->NumberOfLabelsBuilt; i++)
      {
        if (this->Use2DMode)
        {
          this->LabelActors2D[i]->SetPropertyKeys(propKeys);
          renderedSomething += this->LabelActors2D[i]->RenderOpaqueGeometry(viewport);
        }
        else if (this->UseTextActor3D)
        {
          this->LabelActors3D[i]->SetPropertyKeys(propKeys);
          renderedSomething += this->LabelActors3D[i]->RenderOpaqueGeometry(viewport);
        }
        else
        {
          this->LabelActors[i]->SetPropertyKeys(propKeys);
          renderedSomething += this->LabelActors[i]->RenderOpaqueGeometry(viewport);
        }
      }

      if (this->ExponentVisibility && this->Exponent != nullptr && this->Exponent[0] != 0)
      {
        if (this->Use2DMode)
        {
          this->ExponentActor2D->SetPropertyKeys(propKeys);
          renderedSomething += this->ExponentActor2D->RenderOpaqueGeometry(viewport);
        }
        else if (this->UseTextActor3D)
        {
          this->ExponentProp3D->SetPropertyKeys(propKeys);
          renderedSomething += this->ExponentProp3D->RenderOpaqueGeometry(viewport);
        }
        else
        {
          this->ExponentActor->SetPropertyKeys(propKeys);
          renderedSomething += this->ExponentActor->RenderOpaqueGeometry(viewport);
        }
      }
    }
  }

  return renderedSomething;
}

//------------------------------------------------------------------------------
// Build the translucent poly actors and render.
//------------------------------------------------------------------------------
int vtkAxisActor::RenderTranslucentGeometry(vtkViewport* viewport)
{
  return this->RenderTranslucentPolygonalGeometry(viewport);
}

//------------------------------------------------------------------------------
// Build the translucent poly actors and render.
//------------------------------------------------------------------------------
int vtkAxisActor::RenderTranslucentPolygonalGeometry(vtkViewport* viewport)
{

  int renderedSomething = 0;

  this->BuildAxis(viewport, false);

  // Everything is built, just have to render

  // pass keys to sub props
  vtkInformation* propKeys = this->GetPropertyKeys();

  if (!this->AxisHasZeroLength && !this->DrawGridlinesOnly)
  {
    if (this->DrawGridpolys)
    {
      this->GridpolysActor->SetPropertyKeys(propKeys);
      renderedSomething += this->GridpolysActor->RenderTranslucentPolygonalGeometry(viewport);
    }
    if (this->Title != nullptr && this->Title[0] != 0 && this->TitleVisibility)
    {
      if (this->Use2DMode)
      {
        this->TitleActor2D->SetPropertyKeys(propKeys);
        renderedSomething += this->TitleActor2D->RenderTranslucentPolygonalGeometry(viewport);
      }
      else if (this->UseTextActor3D)
      {
        this->TitleProp3D->SetPropertyKeys(propKeys);
        renderedSomething += this->TitleProp3D->RenderTranslucentPolygonalGeometry(viewport);
      }
      else
      {
        this->TitleActor->SetPropertyKeys(propKeys);
        renderedSomething += this->TitleActor->RenderTranslucentPolygonalGeometry(viewport);
      }
    }
    if (this->LabelVisibility)
    {
      for (int i = 0; i < this->NumberOfLabelsBuilt; i++)
      {
        if (this->Use2DMode)
        {
          this->LabelActors2D[i]->SetPropertyKeys(propKeys);
          renderedSomething += this->LabelActors2D[i]->RenderTranslucentPolygonalGeometry(viewport);
        }
        else if (this->UseTextActor3D)
        {
          this->LabelProps3D[i]->SetPropertyKeys(propKeys);
          renderedSomething += this->LabelProps3D[i]->RenderTranslucentPolygonalGeometry(viewport);
        }
        else
        {
          this->LabelActors[i]->SetPropertyKeys(propKeys);
          renderedSomething += this->LabelActors[i]->RenderTranslucentPolygonalGeometry(viewport);
        }
      }
      if (this->ExponentVisibility)
      {
        if (this->Use2DMode)
        {
          this->ExponentActor2D->SetPropertyKeys(propKeys);
          renderedSomething += this->ExponentActor2D->RenderTranslucentPolygonalGeometry(viewport);
        }
        else if (this->UseTextActor3D)
        {
          this->ExponentProp3D->SetPropertyKeys(propKeys);
          renderedSomething += this->ExponentProp3D->RenderTranslucentPolygonalGeometry(viewport);
        }
        else
        {
          this->ExponentActor->SetPropertyKeys(propKeys);
          renderedSomething += this->ExponentActor->RenderTranslucentPolygonalGeometry(viewport);
        }
      }
    }
  }
  return renderedSomething;
}

//------------------------------------------------------------------------------
// Render the 2d annotations.
//------------------------------------------------------------------------------
int vtkAxisActor::RenderOverlay(vtkViewport* viewport)
{
  int renderedSomething = 0;

  // Everything is built, just have to render
  if (!this->AxisHasZeroLength && !this->DrawGridlinesOnly)
  {
    if (this->TitleVisibility)
    {
      if (this->Use2DMode)
      {
        renderedSomething += this->TitleActor2D->RenderOverlay(viewport);
      }
      else if (this->UseTextActor3D)
      {
        renderedSomething += this->TitleProp3D->RenderOverlay(viewport);
      }
      else
      {
        renderedSomething += this->TitleActor->RenderOverlay(viewport);
      }
    }
    if (this->LabelVisibility)
    {
      for (int i = 0; i < this->NumberOfLabelsBuilt; i++)
      {
        if (this->Use2DMode)
        {
          renderedSomething += this->LabelActors2D[i]->RenderOverlay(viewport);
        }
        else if (this->UseTextActor3D)
        {
          renderedSomething += this->LabelProps3D[i]->RenderOverlay(viewport);
        }
        else
        {
          renderedSomething += this->LabelActors[i]->RenderOverlay(viewport);
        }
      }
      if (this->ExponentVisibility)
      {
        if (this->Use2DMode)
        {
          renderedSomething += this->ExponentActor2D->RenderOverlay(viewport);
        }
        else if (this->UseTextActor3D)
        {
          renderedSomething += this->ExponentProp3D->RenderOverlay(viewport);
        }
        else
        {
          renderedSomething += this->ExponentActor->RenderOverlay(viewport);
        }
      }
    }
  }
  return renderedSomething;
}

//------------------------------------------------------------------------------
vtkTypeBool vtkAxisActor::HasTranslucentPolygonalGeometry()
{
  if (this->Visibility && !this->AxisHasZeroLength)
  {
    if (this->TitleVisibility)
    {
      if (this->Use2DMode)
      {
        if (this->TitleActor2D->HasTranslucentPolygonalGeometry())
        {
          return 1;
        }
      }
      else if (this->UseTextActor3D)
      {
        if (this->TitleProp3D->HasTranslucentPolygonalGeometry())
        {
          return 1;
        }
      }
      else
      {
        if (this->TitleActor->HasTranslucentPolygonalGeometry())
        {
          return 1;
        }
      }
    }

    if (this->LabelVisibility)
    {
      if (this->Use2DMode)
      {
        for (int i = 0; i < this->NumberOfLabelsBuilt; ++i)
        {
          if (this->LabelActors2D[i]->HasTranslucentPolygonalGeometry())
          {
            return 1;
          } // end if
        }   // end for
      }     // end 2D
      else if (this->UseTextActor3D)
      {
        for (int i = 0; i < this->NumberOfLabelsBuilt; ++i)
        {
          // if (this->LabelActors3D[i]->HasTranslucentPolygonalGeometry())
          if (this->LabelProps3D[i]->HasTranslucentPolygonalGeometry())
          {
            return 1;
          } // end if
        }   // end for
      }     // end 3D
      else
      {
        for (int i = 0; i < this->NumberOfLabelsBuilt; ++i)
        {
          if (this->LabelActors[i]->HasTranslucentPolygonalGeometry())
          {
            return 1;
          } // end if
        }   // end for
      }     // end 3D
      if (this->ExponentVisibility)
      {
        if (this->Use2DMode)
        {
          if (this->ExponentActor2D->HasTranslucentPolygonalGeometry())
          {
            return 1;
          }
        }
        else if (this->UseTextActor3D)
        {
          if (this->ExponentProp3D->HasTranslucentPolygonalGeometry())
          {
            return 1;
          }
        }
        else
        {
          if (this->ExponentActor->HasTranslucentPolygonalGeometry())
          {
            return 1;
          }
        }
      }
    } // end label vis

    if (this->AxisLinesActor->HasTranslucentPolygonalGeometry())
    {
      return 1;
    }

    if (this->TickVisibility && this->AxisMajorTicksActor->HasTranslucentPolygonalGeometry())
    {
      return 1;
    }
    if (this->TickVisibility && this->AxisMinorTicksActor->HasTranslucentPolygonalGeometry())
    {
      return 1;
    }

    if (this->DrawGridlines && this->GridlinesActor->HasTranslucentPolygonalGeometry())
    {
      return 1;
    }

    if (this->DrawInnerGridlines && this->InnerGridlinesActor->HasTranslucentPolygonalGeometry())
    {
      return 1;
    }

    if (this->DrawGridpolys && this->GridpolysActor->HasTranslucentPolygonalGeometry())
    {
      return 1;
    }

    return this->Superclass::HasTranslucentPolygonalGeometry();
  } // end this vis
  return 0;
}

//-----------------------------------------------------------------------------*
// Perform some initialization, determine which Axis type we are
//-----------------------------------------------------------------------------*
void vtkAxisActor::BuildAxis(vtkViewport* viewport, bool force)
{
  // We'll do our computation in world coordinates. First determine the
  // location of the endpoints.
  double *x, p1[3], p2[3];
  x = this->Point1Coordinate->GetValue();
  p1[0] = x[0];
  p1[1] = x[1];
  p1[2] = x[2];
  x = this->Point2Coordinate->GetValue();
  p2[0] = x[0];
  p2[1] = x[1];
  p2[2] = x[2];

  //
  //  Test for axis of zero length.
  //
  if (p1[0] == p2[0] && p1[1] == p2[1] && p1[2] == p2[2])
  {
    vtkDebugMacro(<< "Axis has zero length, not building.");
    this->AxisHasZeroLength = true;
    return;
  }
  this->AxisHasZeroLength = false;

  if (!force && this->GetMTime() < this->BuildTime.GetMTime() &&
    viewport->GetMTime() < this->BuildTime.GetMTime())
  {
    return; // already built
  }

  if (this->Log)
  {
    if (this->Range[0] <= 0.0)
    {
      vtkWarningMacro(<< "Range value undefined for log scale enabled. "
                      << "Current Range: (" << this->Range[0] << ", " << this->Range[1] << ")"
                      << "Range[0] must be > 0.0. "
                      << ".");
      return;
    }
    if (this->MinorRangeStart <= 0.0 || this->MajorRangeStart <= 0.0)
    {
      vtkWarningMacro(
        << "MinorRangeStart value or MajorRangeStart value undefined for log scale enabled"
        << "MinorRangeStart: " << this->MinorRangeStart
        << ", MajorRangeStart: " << this->MajorRangeStart << ". "
        << "MinorRangeStart and MajorRangeStart must be > 0.0. "
        << ".");
      return;
    }
  }

  vtkDebugMacro(<< "Rebuilding axis");

  if (force || this->GetProperty()->GetMTime() > this->BuildTime.GetMTime())
  {
    // this->AxisLinesActor->SetProperty(this->GetProperty());
    this->TitleActor->SetProperty(this->GetProperty());
    this->TitleActor->GetProperty()->SetColor(this->TitleTextProperty->GetColor());
    this->TitleActor->GetProperty()->SetOpacity(this->TitleTextProperty->GetOpacity());
    if (this->UseTextActor3D)
    {
      this->TitleActor3D->GetTextProperty()->ShallowCopy(this->TitleTextProperty);
    }
  }

  //
  // Generate the axis and tick marks.
  //
  bool ticksRebuilt;
  ticksRebuilt = this->BuildTickPoints(p1, p2, force);

  bool tickVisChanged = this->TickVisibilityChanged();

  if (force || ticksRebuilt || tickVisChanged ||
    this->LastDrawGridlinesLocation != this->DrawGridlinesLocation)
  {
    this->LastDrawGridlinesLocation = this->DrawGridlinesLocation;
    this->SetAxisPointsAndLines();
  }

  // If the ticks have been rebuilt it is more than likely
  // that the labels should follow...
  this->BuildLabels(viewport, force || ticksRebuilt);
  if (this->Use2DMode == 1)
  {
    this->BuildLabels2D(viewport, force || ticksRebuilt);
  }

  if (this->Title != nullptr && this->Title[0] != 0)
  {
    this->InitTitle();
  }

  if (this->ExponentVisibility && this->Exponent != nullptr && this->Exponent[0] != 0)
  {
    this->InitExponent();
  }

  if (this->Title != nullptr && this->Title[0] != 0)
  {
    this->BuildTitle(force || ticksRebuilt);
    if (this->Use2DMode == 1)
    {
      this->BuildTitle2D(viewport, force || ticksRebuilt);
    }
  }

  if (this->ExponentVisibility && this->Exponent != nullptr && this->Exponent[0] != 0)
  {
    // build exponent
    this->BuildExponent(force);
    if (this->Use2DMode == 1)
    {
      this->BuildExponent2D(viewport, force);
    }
  }

  this->LastAxisPosition = this->AxisPosition;
  this->LastRange[0] = this->Range[0];
  this->LastRange[1] = this->Range[1];
  this->BuildTime.Modified();
}

//------------------------------------------------------------------------------
//  Set label values and properties.
//------------------------------------------------------------------------------
void vtkAxisActor::BuildLabels(vtkViewport* viewport, bool force)
{
  if (!force && !this->LabelVisibility)
  {
    return;
  }

  double maxLabelScale = 0.0;
  for (int i = 0; i < this->NumberOfLabelsBuilt; i++)
  {
    this->LabelActors[i]->SetCamera(this->Camera);
    this->LabelProps3D[i]->SetCamera(this->Camera);
    this->LabelActors[i]->GetProperty()->SetColor(this->LabelTextProperty->GetColor());
    this->LabelActors[i]->GetProperty()->SetOpacity(this->LabelTextProperty->GetOpacity());
    this->LabelActors[i]->SetOrientation(0., 0., this->LabelTextProperty->GetOrientation());
    this->LabelProps3D[i]->SetOrientation(0., 0., this->LabelTextProperty->GetOrientation());

    if (this->UseTextActor3D)
    {
      this->LabelActors3D[i]->GetTextProperty()->ShallowCopy(this->LabelTextProperty);

      double labelActorsBounds[6];
      this->LabelActors[i]->GetMapper()->GetBounds(labelActorsBounds);
      const double labelActorsWidth = (labelActorsBounds[1] - labelActorsBounds[0]);

      int labelActors3DBounds[4];
      this->LabelActors3D[i]->GetBoundingBox(labelActors3DBounds);
      const double labelActors3DWidth =
        static_cast<double>(labelActors3DBounds[1] - labelActors3DBounds[0]);

      if (labelActorsWidth / labelActors3DWidth > maxLabelScale)
      {
        maxLabelScale = labelActorsWidth / labelActors3DWidth;
      }
    }

    this->LabelActors[i]->SetAutoCenter(1);
    this->LabelProps3D[i]->SetAutoCenter(1);
  }

  for (int i = 0; i < this->NumberOfLabelsBuilt; i++)
  {
    this->LabelActors3D[i]->SetScale(maxLabelScale);
  }

  if (force || this->BuildTime.GetMTime() < this->BoundsTime.GetMTime() ||
    this->AxisPosition != this->LastAxisPosition || this->LastRange[0] != this->Range[0] ||
    this->LastRange[1] != this->Range[1])
  {
    this->SetLabelPositions(viewport, force);
  }
}

static const int vtkAxisActorMultiplierTable1[4] = { -1, -1, 1, 1 };
static const int vtkAxisActorMultiplierTable2[4] = { -1, 1, 1, -1 };

//------------------------------------------------------------------------------
// Determine and set scale factor and position for labels.
//------------------------------------------------------------------------------
void vtkAxisActor::SetLabelPositions(vtkViewport* viewport, bool force)
{
  if (!force && (!this->LabelVisibility || this->NumberOfLabelsBuilt == 0))
  {
    return;
  }

  //
  // xadjust & yadjust are used for positioning the label correctly
  // depending upon the 'orientation' of the axis as determined
  // by its position in view space (via transformed bounds).
  //
  double displayBounds[6] = { 0., 0., 0., 0., 0., 0. };
  this->TransformBounds(viewport, displayBounds);

  double bounds[6], tickBottom[3], tickTop[3], pos[3];
  double labelAngle = vtkMath::RadiansFromDegrees(this->LabelTextProperty->GetOrientation());
  double labelCos = fabs(cos(labelAngle));
  double labelSin = fabs(sin(labelAngle));
  vtkAxisFollower* pAxisFollower = nullptr;

  for (int i = 0, ptIdx = 0;
       i < this->NumberOfLabelsBuilt && ((ptIdx + 1) < this->MajorTickPts->GetNumberOfPoints());
       i++, ptIdx += 4)
  {
    this->MajorTickPts->GetPoint(ptIdx, tickTop);
    this->MajorTickPts->GetPoint(ptIdx + 1, tickBottom);

    pAxisFollower = this->LabelActors[i];

    // get Label actor Transform matrix
    vtkRenderer* ren = vtkRenderer::SafeDownCast(viewport);
    if (ren)
    {
      pAxisFollower->ComputeTransformMatrix(ren);
    }

    // WARNING: calling GetBounds() before ComputeTransformMatrix(), prevent this->Transform to be
    // updated

    // previous version: this->LabelActors[i]->GetMapper()->GetBounds(bounds); didn't include scale
    // labels
    // vtkProp3D::GetBounds() include previous transform (scale, orientation, translation)
    pAxisFollower->GetBounds(bounds);
    double labelWidth = (bounds[1] - bounds[0]);
    double labelHeight = (bounds[3] - bounds[2]);
    double labelMagnitude = sqrt(labelWidth * labelWidth + labelHeight * labelHeight);

    if (this->CalculateLabelOffset)
    {
      vtkWarningMacro("CalculateLabelOffset flag is now deprecated and has no effect");
    }

    if (this->TickVisibility)
    {
      pos[0] = tickBottom[0];
      pos[1] = tickBottom[1];
      pos[2] = tickBottom[2];
    }
    else
    {
      pos[0] = (tickTop[0] + tickBottom[0]) / 2;
      pos[1] = (tickTop[1] + tickBottom[1]) / 2;
      pos[2] = (tickTop[2] + tickBottom[2]) / 2;
    }

    double deltaPixels = 0.5 * (labelWidth * labelSin + labelHeight * labelCos) / labelMagnitude;
    pAxisFollower->SetScreenOffset(this->LabelOffset + deltaPixels * this->ScreenSize);
    this->LabelProps3D[i]->SetScreenOffset(this->LabelOffset + deltaPixels * this->ScreenSize);

    pAxisFollower->SetPosition(pos[0], pos[1], pos[2]);
    this->LabelProps3D[i]->SetPosition(pos[0], pos[1], pos[2]);
  }
}

//------------------------------------------------------------------------------
//  Set 2D label values and properties.
//------------------------------------------------------------------------------
void vtkAxisActor::BuildLabels2D(vtkViewport* viewport, bool force)
{
  if (!force && (!this->LabelVisibility || this->NumberOfLabelsBuilt == 0))
  {
    return;
  }

  for (int i = 0; i < this->NumberOfLabelsBuilt; i++)
  {
    this->LabelActors2D[i]->GetProperty()->SetColor(this->LabelTextProperty->GetColor());
    this->LabelActors2D[i]->GetProperty()->SetOpacity(this->LabelTextProperty->GetOpacity());
    this->LabelActors2D[i]->GetTextProperty()->ShallowCopy(this->LabelTextProperty);
  }

  this->NeedBuild2D = this->BoundsDisplayCoordinateChanged(viewport);
  if (force || this->NeedBuild2D)
  {
    this->SetLabelPositions2D(viewport, force);
  }
}

//------------------------------------------------------------------------------
// Determine and set scale factor and position for 2D labels.
//------------------------------------------------------------------------------
void vtkAxisActor::SetLabelPositions2D(vtkViewport* viewport, bool force)
{
  if (!force && (!this->LabelVisibility || this->NumberOfLabelsBuilt == 0))
  {
    return;
  }

  int xmult = 0;
  int ymult = 0;
  double xcoeff = 0.;
  double ycoeff = 0.;

  // we are in 2D mode, so no Z axis
  switch (this->AxisType)
  {
    case VTK_AXIS_TYPE_X:
      xmult = 0;
      ymult = vtkAxisActorMultiplierTable1[this->AxisPosition];
      xcoeff = 0.5;
      ycoeff = 1.0;
      break;
    case VTK_AXIS_TYPE_Y:
      xmult = vtkAxisActorMultiplierTable1[this->AxisPosition];
      ymult = 0;
      xcoeff = 1.0;
      ycoeff = 0.5;
      break;
    default:
      // shouldn't get here
      break;
  }

  //
  // xadjust & yadjust are used for positioning the label correctly
  // depending upon the 'orientation' of the axis as determined
  // by its position in view space (via transformed bounds).
  //
  double displayBounds[6] = { 0., 0., 0., 0., 0., 0. };
  this->TransformBounds(viewport, displayBounds);
  double xadjust = (displayBounds[0] > displayBounds[1] ? -1 : 1);
  double yadjust = (displayBounds[2] > displayBounds[3] ? -1 : 1);
  double transpos[3] = { 0., 0., 0. };
  double center[3], tick[3], pos[2];

  vtkTextRenderer* tren = vtkTextRenderer::GetInstance();
  if (!tren)
  {
    vtkErrorMacro(<< "Unable to obtain the vtkTextRenderer instance!");
    return;
  }

  vtkWindow* win = viewport->GetVTKWindow();
  if (!win)
  {
    vtkErrorMacro(<< "No render window available: cannot determine DPI.");
    return;
  }

  for (int i = 0, ptIdx = 1;
       i < this->NumberOfLabelsBuilt && ((ptIdx + 1) < this->MajorTickPts->GetNumberOfPoints());
       i++, ptIdx += 4)
  {
    this->MajorTickPts->GetPoint(ptIdx, tick);

    center[0] = tick[0] + xmult * this->MinorTickSize;
    center[1] = tick[1] + ymult * this->MinorTickSize;
    center[2] = tick[2];

    viewport->SetWorldPoint(center[0], center[1], center[2], 1.0);
    viewport->WorldToDisplay();
    viewport->GetDisplayPoint(transpos);

    int bbox[4];
    if (!tren->GetBoundingBox(this->LabelActors2D[i]->GetTextProperty(),
          this->LabelActors2D[i]->GetInput(), bbox, win->GetDPI()))
    {
      vtkErrorMacro(<< "Unable to calculate bounding box for label "
                    << this->LabelActors2D[i]->GetInput());
      continue;
    }

    double width = (bbox[1] - bbox[0]);
    double height = (bbox[3] - bbox[2]);

    pos[0] = (transpos[0] - xadjust * width * xcoeff);
    pos[1] = (transpos[1] - yadjust * height * ycoeff);

    this->LabelActors2D[i]->SetPosition(pos[0], pos[1]);
  }
}

//------------------------------------------------------------------------------
void vtkAxisActor::InitTitle()
{
  // ---------- Title ----------
  // Classic
  // Source => Mapper => Actor
  // TitleVector => TitleMapper => TitleActor

  // Text 3D
  // vtkTextActor3D::TitleActor3D _ vtkProp3DAxisFollower::TitleProp3D
  // relation: TitleProp3D->SetProp3D(this->TitleActor3D)
  this->TitleVector->SetText(this->Title);
  this->TitleActor3D->SetInput(this->Title);

  this->TitleActor->SetProperty(this->GetProperty());
  this->TitleActor->GetProperty()->SetColor(this->TitleTextProperty->GetColor());
  this->TitleActor->GetProperty()->SetOpacity(this->TitleTextProperty->GetOpacity());

  this->TitleActor3D->SetTextProperty(this->TitleTextProperty);

  this->TitleActor->SetCamera(this->Camera);
  this->TitleProp3D->SetCamera(this->Camera);

  // axis follower origin is on top-left corner, auto-center put it on the center of the label
  this->TitleActor->SetAutoCenter(1);
  this->TitleProp3D->SetAutoCenter(1);
}

//------------------------------------------------------------------------------
void vtkAxisActor::InitExponent()
{
  std::stringstream expStr;
  expStr << "e" << this->Exponent;
  this->ExponentVector->SetText(expStr.str().c_str());
  this->ExponentActor3D->SetInput(expStr.str().c_str());
  this->ExponentActor->SetProperty(this->GetProperty());
  this->ExponentActor3D->SetTextProperty(this->TitleTextProperty);
  this->ExponentActor->SetCamera(this->Camera);
  this->ExponentProp3D->SetCamera(this->Camera);
  this->ExponentActor->SetAutoCenter(1);
  this->ExponentProp3D->SetAutoCenter(1);
}

//------------------------------------------------------------------------------
//  Determines scale and position for the Title.  Currently,
//  title can only be centered with respect to its axis.
//------------------------------------------------------------------------------
void vtkAxisActor::BuildTitle(bool force)
{
  this->NeedBuild2D = false;

  if (!force && !this->TitleVisibility)
  {
    return;
  }

  if (!force && this->TitleTextTime.GetMTime() < this->BuildTime.GetMTime() &&
    this->BoundsTime.GetMTime() < this->BuildTime.GetMTime() &&
    this->LabelBuildTime.GetMTime() < this->BuildTime.GetMTime())
  {
    return;
  }

  // Text property
  this->TitleActor->GetProperty()->SetColor(this->TitleTextProperty->GetColor());
  this->TitleActor->GetProperty()->SetOpacity(this->TitleTextProperty->GetOpacity());

  // ---------- labels size ----------
  // local orientation (in the plane of the label)
  double labelAngle = vtkMath::RadiansFromDegrees(this->LabelTextProperty->GetOrientation());
  double labelCos = fabs(cos(labelAngle)), labelSin = fabs(sin(labelAngle));
  double labBounds[6];
  double offset[2] = { 0, this->TitleOffset[1] };

  // Side offset when not centered
  if (this->TitleAlignLocation == VTK_ALIGN_POINT1 || this->TitleAlignLocation == VTK_ALIGN_POINT2)
  {
    offset[0] += this->TitleOffset[0];
  }

  // find max height label (with the label text property considered)
  // only when title is on bottom
  if (this->LabelVisibility && this->TitleAlignLocation != VTK_ALIGN_TOP)
  {
    double labelMaxHeight, labHeight;
    labelMaxHeight = labHeight = 0;
    for (int i = 0; i < this->NumberOfLabelsBuilt; i++)
    {
      this->LabelActors[i]->GetMapper()->GetBounds(labBounds);

      // labels actor aren't oriented yet, width and height are considered in
      // their local coordinate system
      // however, LabelTextProperty can orient it, but locally (in its plane)
      labHeight =
        (labBounds[1] - labBounds[0]) * labelSin + (labBounds[3] - labBounds[2]) * labelCos;
      labelMaxHeight = (labHeight > labelMaxHeight ? labHeight : labelMaxHeight);
    }
    offset[1] += this->LabelOffset + this->ScreenSize * labelMaxHeight;
  }

  // ---------- title size ----------
  double titleBounds[6];
  this->TitleActor->GetMapper()->GetBounds(titleBounds);
  double halfTitleHeight = (titleBounds[3] - titleBounds[2]) * 0.5;
  double halfTitleWidth = (titleBounds[1] - titleBounds[0]) * 0.5;

  double* p1 = this->Point1Coordinate->GetValue();
  double* p2 = this->Point2Coordinate->GetValue();
  double pos[3];
  if (this->CalculateTitleOffset)
  {
    vtkWarningMacro("CalculateTitleOffset flag is now deprecated and has no effect");
  }
  int vertOffsetSign = 1;
  switch (this->TitleAlignLocation)
  {
    case (VTK_ALIGN_TOP):
      vertOffsetSign = -1;
      VTK_FALLTHROUGH;
    // NO BREAK
    case (VTK_ALIGN_BOTTOM):
      // Position to center of axis
      for (int i = 0; i < 3; i++)
      {
        pos[i] = p1[i] + (p2[i] - p1[i]) / 2.0;
      }
      offset[1] += this->ScreenSize * halfTitleHeight;
      break;
    case (VTK_ALIGN_POINT1):
      // Position to p1
      for (int i = 0; i < 3; i++)
      {
        pos[i] = p1[i];
      }
      offset[0] += this->ScreenSize * halfTitleWidth;
      offset[1] += this->ScreenSize * halfTitleHeight;
      break;
    case (VTK_ALIGN_POINT2):
      // Position to p2
      for (int i = 0; i < 3; i++)
      {
        pos[i] = p2[i];
      }
      offset[0] += this->ScreenSize * halfTitleWidth;
      break;
    default:
      // shouldn't get there
      break;
  }

  if (this->TickVisibility &&
    (this->TickLocation == VTK_TICKS_BOTH ||
      (this->TickLocation == VTK_TICKS_INSIDE && this->TitleAlignLocation == VTK_ALIGN_TOP) ||
      (this->TickLocation == VTK_TICKS_OUTSIDE && this->TitleAlignLocation != VTK_ALIGN_TOP)))
  {
    for (int i = 0; i < 3; i++)
    {
      pos[i] += vertOffsetSign * this->TickVector[i];
    }
  }

  offset[1] *= vertOffsetSign;
  this->TitleActor->SetScreenOffsetVector(offset);
  this->TitleProp3D->SetScreenOffsetVector(offset);

  if (this->UseTextActor3D)
  {
    int titleActor3DBounds[4];
    this->TitleActor3D->GetBoundingBox(titleActor3DBounds);
    const double titleActor3DWidth =
      static_cast<double>(titleActor3DBounds[1] - titleActor3DBounds[0]);

    // Convert from font coordinate system to world coordinate system:
    this->TitleActor3D->SetScale((titleBounds[1] - titleBounds[0]) / titleActor3DWidth);
  }
  this->TitleActor->SetPosition(pos);
  this->TitleProp3D->SetPosition(pos);
}

//------------------------------------------------------------------------------
void vtkAxisActor::BuildExponent(bool force)
{
  if (!force && (!this->ExponentVisibility || !this->Exponent))
  {
    return;
  }

  if (!force && this->ExponentTextTime.GetMTime() < this->BuildTime.GetMTime() &&
    this->BoundsTime.GetMTime() < this->BuildTime.GetMTime() &&
    this->LabelBuildTime.GetMTime() < this->BuildTime.GetMTime())
  {
    return;
  }

  // Text property
  this->ExponentActor->GetProperty()->SetColor(this->TitleTextProperty->GetColor());
  this->ExponentActor->GetProperty()->SetOpacity(this->TitleTextProperty->GetOpacity());

  // ---------- labels size ----------
  // local orientation (in the plane of the label)
  double labelAngle = vtkMath::RadiansFromDegrees(this->LabelTextProperty->GetOrientation());
  double labelCos = fabs(cos(labelAngle)), labelSin = fabs(sin(labelAngle));
  double labBounds[6];
  double offset[2] = { 0, this->ExponentOffset };

  // find max height label (with the label text property considered)
  // only when title is on bottom
  if (this->LabelVisibility && this->ExponentLocation != VTK_ALIGN_TOP)
  {
    double labelMaxHeight, labHeight;
    labelMaxHeight = labHeight = 0;
    for (int i = 0; i < this->NumberOfLabelsBuilt; i++)
    {
      this->LabelActors[i]->GetMapper()->GetBounds(labBounds);

      // labels actor aren't oriented yet, width and height are considered in
      // their local coordinate system
      // however, LabelTextProperty can orient it, but locally (in its plane)
      labHeight =
        (labBounds[1] - labBounds[0]) * labelSin + (labBounds[3] - labBounds[2]) * labelCos;
      labelMaxHeight = (labHeight > labelMaxHeight ? labHeight : labelMaxHeight);
    }
    offset[1] += this->LabelOffset + this->ScreenSize * labelMaxHeight;
  }

  // ---------- title size ----------
  double titleBounds[6];
  this->TitleActor->GetMapper()->GetBounds(titleBounds);
  if (this->TitleVisibility && this->TitleAlignLocation == this->ExponentLocation)
  {
    offset[1] += this->TitleOffset[1] + this->ScreenSize * titleBounds[3] - titleBounds[2];
  }

  // ---------- exponent size ----------
  double exponentBounds[6];
  this->ExponentActor->GetMapper()->GetBounds(exponentBounds);
  double halfExponentHeight = (exponentBounds[3] - exponentBounds[2]) * 0.5;
  double halfExponentWidth = (exponentBounds[1] - exponentBounds[0]) * 0.5;

  double* p1 = this->Point1Coordinate->GetValue();
  double* p2 = this->Point2Coordinate->GetValue();
  double pos[3];

  int offsetSign = 1;
  switch (this->ExponentLocation)
  {
    case (VTK_ALIGN_TOP):
      offsetSign = -1;
      VTK_FALLTHROUGH;
    // NO BREAK
    case (VTK_ALIGN_BOTTOM):
      // Position to center of axis
      for (int i = 0; i < 3; i++)
      {
        pos[i] = p1[i] + (p2[i] - p1[i]) / 2.0;
      }
      offset[1] += this->ScreenSize * halfExponentHeight;
      break;
    case (VTK_ALIGN_POINT1):
      // Position to p1
      for (int i = 0; i < 3; i++)
      {
        pos[i] = p1[i];
      }
      offset[0] += this->ScreenSize * halfExponentWidth;
      offset[1] += this->ScreenSize * halfExponentHeight;
      break;
    case (VTK_ALIGN_POINT2):
      // Position to p2
      for (int i = 0; i < 3; i++)
      {
        pos[i] = p2[i];
      }
      offset[0] += this->ScreenSize * halfExponentWidth;
      break;
    default:
      // shouldn't get there
      break;
  }

  if (this->TickVisibility &&
    (this->TickLocation == VTK_TICKS_BOTH ||
      (this->TickLocation == VTK_TICKS_INSIDE && this->ExponentLocation == VTK_ALIGN_TOP) ||
      (this->TickLocation == VTK_TICKS_OUTSIDE && this->ExponentLocation != VTK_ALIGN_TOP)))
  {
    for (int i = 0; i < 3; i++)
    {
      pos[i] += offsetSign * this->TickVector[i];
    }
  }

  // Offset is: ExponentOffset + TitleOffset is visible + LabelOffset if visible
  // + ScreenSize of all
  offset[1] *= offsetSign;
  this->ExponentActor->SetScreenOffsetVector(offset);
  this->ExponentProp3D->SetScreenOffsetVector(offset);

  if (this->UseTextActor3D)
  {
    int exponentActor3DBounds[4];
    this->ExponentActor3D->GetBoundingBox(exponentActor3DBounds);
    const double exponentActor3DWidth =
      static_cast<double>(exponentActor3DBounds[1] - exponentActor3DBounds[0]);

    // Convert from font coordinate system to world coordinate system:
    this->ExponentActor3D->SetScale((exponentBounds[1] - exponentBounds[0]) / exponentActor3DWidth);
  }

  this->ExponentActor->SetPosition(pos);
  this->ExponentProp3D->SetPosition(pos);
}

//------------------------------------------------------------------------------
//  Determines scale and position for the 2D Title.  Currently,
//  title can only be centered with respect to its axis.
//------------------------------------------------------------------------------
void vtkAxisActor::BuildTitle2D(vtkViewport* viewport, bool force)
{
  if (!this->NeedBuild2D && !force && !this->TitleVisibility)
  {
    return;
  }

  // for textactor instead of follower
  this->TitleActor2D->SetInput(this->TitleVector->GetText());
  this->TitleActor2D->GetProperty()->SetColor(this->TitleTextProperty->GetColor());
  this->TitleActor2D->GetProperty()->SetOpacity(this->TitleTextProperty->GetOpacity());
  this->TitleActor2D->GetTextProperty()->ShallowCopy(this->TitleTextProperty);

  if (this->AxisType == VTK_AXIS_TYPE_Y)
  {
    if (strlen(this->TitleActor2D->GetInput()) > 2)
    {
      // warning : orientation have to be set on vtkTextActor and not on the vtkTextActor's
      // vtkTextProperty
      // otherwise there is a strange effect (first letter is not align with the others)
      this->TitleActor2D->SetOrientation(90);
    }
    else
    {
      // if in the previous rendering, the orientation was set.
      this->TitleActor2D->SetOrientation(0);
    }
  }

  // stuff for 2D axis with TextActor
  double transpos[3];
  double* pos = this->TitleActor->GetPosition();
  viewport->SetWorldPoint(pos[0], pos[1], pos[2], 1.0);
  viewport->WorldToDisplay();
  viewport->GetDisplayPoint(transpos);

  int offsetSign = 1;
  if (this->TitleAlignLocation == VTK_ALIGN_TOP)
  {
    offsetSign = -1;
  }

  if (this->AxisType == VTK_AXIS_TYPE_X)
  {
    transpos[1] += offsetSign * this->VerticalOffsetXTitle2D;
  }
  else if (this->AxisType == VTK_AXIS_TYPE_Y)
  {
    transpos[0] += offsetSign * this->HorizontalOffsetYTitle2D;
  }
  if (transpos[1] < 10.)
  {
    transpos[1] = 10.;
  }
  if (transpos[0] < 10.)
  {
    transpos[0] = 10.;
  }

  if (this->SaveTitlePosition == 0)
  {
    this->TitleActor2D->SetPosition(transpos[0], transpos[1]);
  }
  else
  {
    if (this->SaveTitlePosition == 1)
    {
      this->TitleConstantPosition[0] = transpos[0];
      this->TitleConstantPosition[1] = transpos[1];
      this->SaveTitlePosition = 2;
    }
    this->TitleActor2D->SetPosition(this->TitleConstantPosition[0], this->TitleConstantPosition[1]);
  }
  this->RotateActor2DFromAxisProjection(this->TitleActor2D);
}

//------------------------------------------------------------------------------
void vtkAxisActor::BuildExponent2D(vtkViewport* viewport, bool force)
{
  if (!this->NeedBuild2D && !force && !this->LabelVisibility)
  {
    return;
  }

  // for textactor instead of follower
  this->ExponentActor2D->SetInput(this->ExponentVector->GetText());
  this->ExponentActor2D->GetProperty()->SetColor(this->TitleTextProperty->GetColor());
  this->ExponentActor2D->GetProperty()->SetOpacity(this->TitleTextProperty->GetOpacity());
  this->ExponentActor2D->GetTextProperty()->ShallowCopy(this->TitleTextProperty);

  if (this->AxisType == VTK_AXIS_TYPE_Y)
  {
    if (strlen(this->ExponentActor2D->GetInput()) > 2)
    {
      // warning : orientation have to be set on vtkTextActor and not on the vtkTextActor's
      // vtkTextProperty
      // otherwise there is a strange effect (first letter is not align with the others)
      this->ExponentActor2D->SetOrientation(90);
    }
    else
    {
      // if in the previous rendering, the orientation was set.
      this->ExponentActor2D->SetOrientation(0);
    }
  }

  // stuff for 2D axis with TextActor
  double transpos[3];
  double* pos = this->ExponentActor->GetPosition();
  viewport->SetWorldPoint(pos[0], pos[1], pos[2], 1.0);
  viewport->WorldToDisplay();
  viewport->GetDisplayPoint(transpos);

  int offsetSign = 1;
  if (this->ExponentLocation == VTK_ALIGN_TOP)
  {
    offsetSign = -1;
  }

  int titleMult = 1;
  if (this->TitleVisibility && this->TitleAlignLocation == this->ExponentLocation)
  {
    titleMult = 2;
  }

  if (this->AxisType == VTK_AXIS_TYPE_X)
  {
    transpos[1] += offsetSign * titleMult * this->VerticalOffsetXTitle2D;
  }
  else if (this->AxisType == VTK_AXIS_TYPE_Y)
  {
    transpos[0] += offsetSign * titleMult * this->HorizontalOffsetYTitle2D;
  }
  if (transpos[1] < 10.)
  {
    transpos[1] = 10.;
  }
  if (transpos[0] < 10.)
  {
    transpos[0] = 10.;
  }

  this->ExponentActor2D->SetPosition(transpos[0], transpos[1]);

  this->RotateActor2DFromAxisProjection(this->ExponentActor2D);
}

//------------------------------------------------------------------------------
//  Transform the bounding box to display coordinates.  Used
//  in determining orientation of the axis.
//------------------------------------------------------------------------------
void vtkAxisActor::TransformBounds(vtkViewport* viewport, double bnds[6])
{
  double minPt[3], maxPt[3], transMinPt[3], transMaxPt[3];
  minPt[0] = this->Bounds[0];
  minPt[1] = this->Bounds[2];
  minPt[2] = this->Bounds[4];
  maxPt[0] = this->Bounds[1];
  maxPt[1] = this->Bounds[3];
  maxPt[2] = this->Bounds[5];

  viewport->SetWorldPoint(minPt[0], minPt[1], minPt[2], 1.0);
  viewport->WorldToDisplay();
  viewport->GetDisplayPoint(transMinPt);
  viewport->SetWorldPoint(maxPt[0], maxPt[1], maxPt[2], 1.0);
  viewport->WorldToDisplay();
  viewport->GetDisplayPoint(transMaxPt);

  bnds[0] = transMinPt[0];
  bnds[2] = transMinPt[1];
  bnds[4] = transMinPt[2];
  bnds[1] = transMaxPt[0];
  bnds[3] = transMaxPt[1];
  bnds[5] = transMaxPt[2];
}

//------------------------------------------------------------------------------
void vtkAxisActor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Title: " << (this->Title ? this->Title : "(none)") << "\n";
  os << indent << "Number Of Labels Built: " << this->NumberOfLabelsBuilt << "\n";
  os << indent << "Range: (" << this->Range[0] << ", " << this->Range[1] << ")\n";

  os << indent << "UseTextActor3D: " << this->UseTextActor3D << "\n";
  os << indent << "Label Format: " << this->LabelFormat << "\n";

  os << indent << "Axis Visibility: " << (this->AxisVisibility ? "On\n" : "Off\n");

  os << indent << "Tick Visibility: " << (this->TickVisibility ? "On\n" : "Off\n");

  os << indent << "Label Visibility: " << (this->LabelVisibility ? "On\n" : "Off\n");

  os << indent << "Title Visibility: " << (this->TitleVisibility ? "On\n" : "Off\n");

  os << indent << "Point1 Coordinate: " << this->Point1Coordinate << "\n";
  this->Point1Coordinate->PrintSelf(os, indent.GetNextIndent());

  os << indent << "Point2 Coordinate: " << this->Point2Coordinate << "\n";
  this->Point2Coordinate->PrintSelf(os, indent.GetNextIndent());

  os << indent << "Title offset: " << this->TitleOffset[0] << ", " << this->TitleOffset[1] << "\n";
  os << indent << "Label Y-offset: " << this->LabelOffset << "\n";
  os << indent << "Exponent Y-offset: " << this->ExponentOffset << "\n";

  os << indent << "AxisType: ";
  switch (this->AxisType)
  {
    case VTK_AXIS_TYPE_X:
      os << "X Axis" << endl;
      break;
    case VTK_AXIS_TYPE_Y:
      os << "Y Axis" << endl;
      break;
    case VTK_AXIS_TYPE_Z:
      os << "Z Axis" << endl;
      break;
    default:
      // shouldn't get here
      break;
  }

  os << indent << "DeltaMajor: " << this->DeltaMajor[0] << "," << this->DeltaMajor[1] << ","
     << this->DeltaMajor[2] << endl;
  os << indent << "DeltaMinor: " << this->DeltaMinor << endl;
  os << indent << "DeltaRangeMajor: " << this->DeltaRangeMajor << endl;
  os << indent << "DeltaRangeMinor: " << this->DeltaRangeMinor << endl;
  os << indent << "MajorRangeStart: " << this->MajorRangeStart << endl;
  os << indent << "MinorRangeStart: " << this->MinorRangeStart << endl;

  os << indent << "MinorTicksVisible: " << this->MinorTicksVisible << endl;

  os << indent << "TitleActor: ";
  if (this->TitleActor)
  {
    os << indent << "TitleActor: (" << this->TitleActor << ")\n";
  }
  else
  {
    os << "(none)" << endl;
  }

  os << indent << "Camera: ";
  if (this->Camera)
  {
    this->Camera->PrintSelf(os, indent);
  }
  else
  {
    os << "(none)" << endl;
  }

  os << indent << "MajorTickSize: " << this->MajorTickSize << endl;
  os << indent << "MinorTickSize: " << this->MinorTickSize << endl;

  os << indent << "DrawGridlines: " << this->DrawGridlines << endl;

  os << indent << "MajorStart: " << this->MajorStart[0] << "," << this->MajorStart[1] << ","
     << this->MajorStart[2] << endl;

  os << indent << "AxisPosition: " << this->AxisPosition << endl;

  os << indent << "GridlineXLength: " << this->GridlineXLength << endl;
  os << indent << "GridlineYLength: " << this->GridlineYLength << endl;
  os << indent << "GridlineZLength: " << this->GridlineZLength << endl;

  os << indent << "DrawInnerGridpolys: " << this->DrawGridpolys << endl;
  os << indent << "DrawInnerGridlines: " << this->DrawInnerGridlines << endl;

  os << indent << "TickLocation: " << this->TickLocation << endl;

  os << indent << "CalculateLabelOffset: " << this->CalculateLabelOffset << std::endl;
  os << indent << "CalculateTitleOffset: " << this->CalculateTitleOffset << std::endl;

  os << indent << "LabelTextProperty: " << this->LabelTextProperty << endl;
  os << indent << "TitleTextProperty: " << this->TitleTextProperty << endl;

  os << indent << "Use2DMode: " << this->Use2DMode << endl;
  os << indent << "SaveTitlePosition: " << this->SaveTitlePosition << endl;
  os << indent << "VerticalOffsetXTitle2D" << this->VerticalOffsetXTitle2D << endl;
  os << indent << "HorizontalOffsetYTitle2D" << this->HorizontalOffsetYTitle2D << endl;
  os << indent << "LastMinDisplayCoordinates: (" << this->LastMinDisplayCoordinate[0] << ", "
     << this->LastMinDisplayCoordinate[1] << ", " << this->LastMinDisplayCoordinate[2] << ")"
     << endl;
  os << indent << "LastMaxDisplayCoordinates: (" << this->LastMaxDisplayCoordinate[0] << ", "
     << this->LastMaxDisplayCoordinate[1] << ", " << this->LastMaxDisplayCoordinate[2] << ")"
     << endl;
}

//------------------------------------------------------------------------------
// Sets text string for label vectors.  Allocates memory if necessary.
//------------------------------------------------------------------------------
void vtkAxisActor::SetLabels(vtkStringArray* labels)
{
  //
  // If the number of labels has changed, re-allocate the correct
  // amount of memory.
  //
  int numLabels = labels->GetNumberOfValues();
  if (numLabels < 0)
  {
    vtkErrorMacro(<< "Number of labels " << numLabels << " is invalid");
    return;
  }
  if (this->NumberOfLabelsBuilt != numLabels)
  {
    if (this->LabelMappers != nullptr)
    {
      for (int i = 0; i < this->NumberOfLabelsBuilt; i++)
      {
        this->LabelActors[i]->Delete();
        this->LabelProps3D[i]->Delete();
      }
      delete[] this->LabelVectors;
      delete[] this->LabelMappers;
      delete[] this->LabelActors;
      delete[] this->LabelProps3D;
      delete[] this->LabelActors3D;
      delete[] this->LabelActors2D;
    }

    this->LabelVectors = new vtkSmartPointer<vtkVectorText>[numLabels];
    this->LabelMappers = new vtkSmartPointer<vtkPolyDataMapper>[numLabels];
    this->LabelActors = new vtkAxisFollower*[numLabels];
    this->LabelProps3D = new vtkProp3DAxisFollower*[numLabels];
    this->LabelActors3D = new vtkSmartPointer<vtkTextActor3D>[numLabels];
    this->LabelActors2D = new vtkSmartPointer<vtkTextActor>[numLabels];

    for (int i = 0; i < numLabels; i++)
    {
      this->LabelVectors[i] = vtkSmartPointer<vtkVectorText>::New();
      this->LabelMappers[i] = vtkSmartPointer<vtkPolyDataMapper>::New();
      this->LabelMappers[i]->SetInputConnection(this->LabelVectors[i]->GetOutputPort());
      this->LabelActors[i] = vtkAxisFollower::New();
      this->LabelActors[i]->SetAxis(this);
      this->LabelActors[i]->SetMapper(this->LabelMappers[i]);
      this->LabelActors[i]->SetEnableDistanceLOD(0);
      this->LabelActors[i]->GetProperty()->SetAmbient(1.);
      this->LabelActors[i]->GetProperty()->SetDiffuse(0.);
      this->LabelActors[i]->GetProperty()->SetColor(this->LabelTextProperty->GetColor());
      this->LabelActors[i]->GetProperty()->SetOpacity(this->LabelTextProperty->GetOpacity());
      this->LabelProps3D[i] = vtkProp3DAxisFollower::New();
      this->LabelProps3D[i]->SetAxis(this);
      this->LabelProps3D[i]->SetEnableDistanceLOD(0);
      this->LabelActors3D[i] = vtkSmartPointer<vtkTextActor3D>::New();
      this->LabelProps3D[i]->SetProp3D(this->LabelActors3D[i]);
      this->LabelActors2D[i] = vtkSmartPointer<vtkTextActor>::New();
    }
  }

  //
  // Set the label vector text.
  //
  for (int i = 0; i < numLabels; i++)
  {
    this->LabelVectors[i]->SetText(labels->GetValue(i).c_str());
    this->LabelActors3D[i]->SetInput(this->LabelVectors[i]->GetText());
    this->LabelActors2D[i]->SetInput(this->LabelVectors[i]->GetText());
  }
  this->NumberOfLabelsBuilt = numLabels;
  this->LabelBuildTime.Modified();
}

//-----------------------------------------------------------------------------*
// Creates Poly data (lines) from tickmarks (minor/major), gridlines, and axis.
//-----------------------------------------------------------------------------*
void vtkAxisActor::SetAxisPointsAndLines()
{
  vtkPoints* mainLinePts = vtkPoints::New();
  vtkPoints* axisMajorTicksPts = vtkPoints::New();
  vtkPoints* axisMinorTicksPts = vtkPoints::New();

  vtkCellArray* mainLine = vtkCellArray::New();
  vtkCellArray* axisMajorTicksLines = vtkCellArray::New();
  vtkCellArray* axisMinorTicksLines = vtkCellArray::New();

  vtkCellArray* gridlines = vtkCellArray::New();
  vtkCellArray* innerGridlines = vtkCellArray::New();
  vtkCellArray* polys = vtkCellArray::New();

  this->AxisLines->SetPoints(mainLinePts);
  this->AxisLines->SetLines(mainLine);

  this->AxisMajorTicks->SetPoints(axisMajorTicksPts);
  this->AxisMajorTicks->SetLines(axisMajorTicksLines);

  this->AxisMinorTicks->SetPoints(axisMinorTicksPts);
  this->AxisMinorTicks->SetLines(axisMinorTicksLines);

  this->Gridlines->SetPoints(this->GridlinePts);
  this->Gridlines->SetLines(gridlines);
  this->InnerGridlines->SetPoints(this->InnerGridlinePts);
  this->InnerGridlines->SetLines(innerGridlines);
  this->Gridpolys->SetPoints(this->GridpolyPts);
  this->Gridpolys->SetPolys(polys);

  mainLinePts->Delete();
  axisMajorTicksPts->Delete();
  axisMinorTicksPts->Delete();

  mainLine->Delete();
  axisMajorTicksLines->Delete();
  axisMinorTicksLines->Delete();

  gridlines->Delete();
  innerGridlines->Delete();
  polys->Delete();
  int numMinorTickPts, numGridlines, numInnerGridlines, numMajorTickPts, numGridpolys, numLines;
  vtkIdType ptIds[2];
  vtkIdType polyPtIds[4];

  if (this->TickVisibility)
  {
    if (this->MinorTicksVisible)
    {
      // In 2D mode, the minorTickPts for yz portion or xz portion have been removed.
      numMinorTickPts = this->MinorTickPts->GetNumberOfPoints();
      for (int i = 0; i < numMinorTickPts; i++)
      {
        axisMinorTicksPts->InsertNextPoint(this->MinorTickPts->GetPoint(i));
      }
    }
    numMajorTickPts = this->MajorTickPts->GetNumberOfPoints();
    if (this->Use2DMode == 0)
    {
      for (int i = 0; i < numMajorTickPts; i++)
      {
        axisMajorTicksPts->InsertNextPoint(this->MajorTickPts->GetPoint(i));
      }
    }
    else
    {
      // In 2D mode, we don't need the pts for the xz portion or yz portion of the major tickmarks
      // majorTickPts not modified because all points are used for labels' positions.
      for (int i = 0; i < numMajorTickPts; i += 4)
      {
        axisMajorTicksPts->InsertNextPoint(this->MajorTickPts->GetPoint(i));
        axisMajorTicksPts->InsertNextPoint(this->MajorTickPts->GetPoint(i + 1));
      }
    }
  }

  // create major ticks lines
  numLines = axisMajorTicksPts->GetNumberOfPoints() / 2;
  for (int i = 0; i < numLines; i++)
  {
    ptIds[0] = 2 * i;
    ptIds[1] = 2 * i + 1;
    axisMajorTicksLines->InsertNextCell(2, ptIds);
  }

  // create major ticks lines
  numLines = axisMinorTicksPts->GetNumberOfPoints() / 2;
  for (int i = 0; i < numLines; i++)
  {
    ptIds[0] = 2 * i;
    ptIds[1] = 2 * i + 1;
    axisMinorTicksLines->InsertNextCell(2, ptIds);
  }

  if (this->AxisVisibility)
  {
    // first axis point
    ptIds[0] = mainLinePts->InsertNextPoint(this->Point1Coordinate->GetValue());
    // last axis point
    ptIds[1] = mainLinePts->InsertNextPoint(this->Point2Coordinate->GetValue());
    mainLine->InsertNextCell(2, ptIds);
  }
  // create grid lines
  if (this->DrawGridlines && this->AxisOnOrigin == 0)
  {
    numGridlines = this->GridlinePts->GetNumberOfPoints() / 2;
    int start = (this->DrawGridlinesLocation == 0 || this->DrawGridlinesLocation == 1) ? 0 : 1;
    int increment = (this->DrawGridlinesLocation == 0) ? 1 : 2;
    for (int i = start; i < numGridlines; i += increment)
    {
      ptIds[0] = 2 * i;
      ptIds[1] = 2 * i + 1;
      gridlines->InsertNextCell(2, ptIds);
    }
  }

  // create inner grid lines
  if (this->DrawInnerGridlines && this->AxisOnOrigin == 0)
  {
    numInnerGridlines = this->InnerGridlinePts->GetNumberOfPoints() / 2;
    for (int i = 0; i < numInnerGridlines; i++)
    {
      ptIds[0] = 2 * i;
      ptIds[1] = 2 * i + 1;
      innerGridlines->InsertNextCell(2, ptIds);
    }
  }

  // create polys (grid polys)
  if (this->DrawGridpolys && this->AxisOnOrigin == 0)
  {
    numGridpolys = this->GridpolyPts->GetNumberOfPoints() / 4;
    for (int i = 0; i < numGridpolys; i++)
    {
      polyPtIds[0] = 4 * i;
      polyPtIds[1] = 4 * i + 1;
      polyPtIds[2] = 4 * i + 2;
      polyPtIds[3] = 4 * i + 3;
      polys->InsertNextCell(4, polyPtIds);
    }
  }
}

//-----------------------------------------------------------------------------**
// Returns true if any tick vis attribute has changed since last check.
//-----------------------------------------------------------------------------**
bool vtkAxisActor::TickVisibilityChanged()
{
  bool retVal = (this->TickVisibility != this->LastTickVisibility) ||
    (this->DrawGridlines != this->LastDrawGridlines) ||
    (this->MinorTicksVisible != this->LastMinorTicksVisible);

  this->LastTickVisibility = this->TickVisibility;
  this->LastDrawGridlines = this->DrawGridlines;
  this->LastMinorTicksVisible = this->MinorTicksVisible;

  return retVal;
}

//-----------------------------------------------------------------------------**
// Set the bounds for this actor to use.  Sets timestamp BoundsModified.
//-----------------------------------------------------------------------------**
void vtkAxisActor::SetBounds(const double b[6])
{
  if ((this->Bounds[0] != b[0]) || (this->Bounds[1] != b[1]) || (this->Bounds[2] != b[2]) ||
    (this->Bounds[3] != b[3]) || (this->Bounds[4] != b[4]) || (this->Bounds[5] != b[5]))
  {
    for (int i = 0; i < 6; i++)
    {
      this->Bounds[i] = b[i];
    }
    this->BoundsTime.Modified();
  }
}

//-----------------------------------------------------------------------------**
// Retrieves the bounds of this actor.
//-----------------------------------------------------------------------------**
void vtkAxisActor::SetBounds(
  double xmin, double xmax, double ymin, double ymax, double zmin, double zmax)
{
  if ((this->Bounds[0] != xmin) || (this->Bounds[1] != xmax) || (this->Bounds[2] != ymin) ||
    (this->Bounds[3] != ymax) || (this->Bounds[4] != zmin) || (this->Bounds[5] != zmax))
  {
    this->Bounds[0] = xmin;
    this->Bounds[1] = xmax;
    this->Bounds[2] = ymin;
    this->Bounds[3] = ymax;
    this->Bounds[4] = zmin;
    this->Bounds[5] = zmax;

    this->BoundsTime.Modified();
  }
}

//-----------------------------------------------------------------------------**
// Retrieves the bounds of this actor.
//-----------------------------------------------------------------------------**
double* vtkAxisActor::GetBounds()
{
  return this->Bounds;
}

//-----------------------------------------------------------------------------**
// Retrieves the bounds of this actor.
//-----------------------------------------------------------------------------**

void vtkAxisActor::GetBounds(double b[6])
{
  for (int i = 0; i < 6; i++)
  {
    b[i] = this->Bounds[i];
  }
}

//-----------------------------------------------------------------------------**
// Method:  vtkAxisActor::ComputeMaxLabelLength
//-----------------------------------------------------------------------------**
double vtkAxisActor::ComputeMaxLabelLength(const double vtkNotUsed(center)[3])
{
  double bounds[6];
  double xsize, ysize;
  vtkProperty* newProp = this->NewLabelProperty();
  double maxXSize = 0;
  double maxYSize = 0;
  for (int i = 0; i < this->NumberOfLabelsBuilt; i++)
  {
    if (this->UseTextActor3D)
    {
      this->LabelProps3D[i]->SetCamera(this->Camera);
      this->LabelActors3D[i]->GetBounds(bounds);
    }
    else
    {
      this->LabelActors[i]->SetCamera(this->Camera);
      this->LabelActors[i]->SetProperty(newProp);
      this->LabelActors[i]->GetMapper()->GetBounds(bounds);
    }
    xsize = bounds[1] - bounds[0];
    ysize = bounds[3] - bounds[2];
    maxXSize = (xsize > maxXSize ? xsize : maxXSize);
    maxYSize = (ysize > maxYSize ? ysize : maxYSize);
  }
  newProp->Delete();
  return sqrt(maxXSize * maxXSize + maxYSize * maxYSize);
}

//-----------------------------------------------------------------------------**
// Method:  vtkAxisActor::ComputeTitleLength
//-----------------------------------------------------------------------------**
double vtkAxisActor::ComputeTitleLength(const double vtkNotUsed(center)[3])
{
  double bounds[6];
  double xsize, ysize;
  double length;

  if (this->UseTextActor3D)
  {
    this->TitleActor3D->SetInput(this->Title);
    this->TitleProp3D->SetCamera(this->Camera);
    this->TitleActor3D->GetBounds(bounds);
  }
  else
  {
    this->TitleVector->SetText(this->Title);
    this->TitleActor->SetCamera(this->Camera);
    vtkProperty* newProp = this->NewTitleProperty();
    this->TitleActor->SetProperty(newProp);
    newProp->Delete();
    this->TitleActor->GetMapper()->GetBounds(bounds);
  }
  xsize = bounds[1] - bounds[0];
  ysize = bounds[3] - bounds[2];
  length = sqrt(xsize * xsize + ysize * ysize);

  return length;
}

//-----------------------------------------------------------------------------**
void vtkAxisActor::SetLabelScale(double s)
{
  for (int i = 0; i < this->NumberOfLabelsBuilt; i++)
  {
    this->SetLabelScale(i, s);
  }
}

//-----------------------------------------------------------------------------**
void vtkAxisActor::SetLabelScale(int label, double s)
{
  this->LabelActors[label]->SetScale(s);
  this->LabelProps3D[label]->SetScale(s);
}

//-----------------------------------------------------------------------------**
void vtkAxisActor::SetTitleScale(double s)
{
  this->TitleActor->SetScale(s);
  this->TitleProp3D->SetScale(s);
  this->ExponentActor->SetScale(s);
  this->ExponentProp3D->SetScale(s);
}

//-----------------------------------------------------------------------------**
void vtkAxisActor::SetTitle(const char* t)
{
  if (this->Title == nullptr && t == nullptr)
  {
    return;
  }
  if (this->Title && t && (!strcmp(this->Title, t)))
  {
    return;
  }
  delete[] this->Title;
  if (t)
  {
    this->Title = new char[strlen(t) + 1];
    strcpy(this->Title, t);
  }
  else
  {
    this->Title = nullptr;
  }
  this->TitleTextTime.Modified();
  this->Modified();
}

//-----------------------------------------------------------------------------**
void vtkAxisActor::SetTitleAlignLocation(int location)
{
  if (location != this->TitleAlignLocation)
  {
    switch (location)
    {
      case VTK_ALIGN_TOP:
      case VTK_ALIGN_BOTTOM:
      case VTK_ALIGN_POINT1:
      case VTK_ALIGN_POINT2:
      {
        this->TitleAlignLocation = location;
        this->TitleTextTime.Modified();
        this->Modified();
        break;
      }
      default:
      {
        break;
      }
    }
  }
}

//-----------------------------------------------------------------------------**
void vtkAxisActor::SetExponent(const char* t)
{
  if (this->Exponent == nullptr && t == nullptr)
  {
    return;
  }
  if (this->Exponent && t && (!strcmp(this->Exponent, t)))
  {
    return;
  }
  delete[] this->Exponent;
  if (t)
  {
    this->Exponent = new char[strlen(t) + 1];
    strcpy(this->Exponent, t);
  }
  else
  {
    this->Exponent = nullptr;
  }
  this->ExponentTextTime.Modified();
  this->Modified();
}

//-----------------------------------------------------------------------------**
void vtkAxisActor::SetExponentLocation(int location)
{
  if (location != this->ExponentLocation)
  {
    switch (location)
    {
      case VTK_ALIGN_TOP:
      case VTK_ALIGN_BOTTOM:
      case VTK_ALIGN_POINT1:
      case VTK_ALIGN_POINT2:
      {
        this->ExponentLocation = location;
        this->ExponentTextTime.Modified();
        this->Modified();
        break;
      }
      default:
      {
        break;
      }
    }
  }
}

//------------------------------------------------------------------------------
void vtkAxisActor::SetAxisLinesProperty(vtkProperty* prop)
{
  this->SetAxisMainLineProperty(prop);
  this->SetAxisMajorTicksProperty(prop);
  this->SetAxisMinorTicksProperty(prop);
  this->Modified();
}

//------------------------------------------------------------------------------
vtkProperty* vtkAxisActor::GetAxisLinesProperty()
{
  return this->AxisLinesActor->GetProperty();
}

//------------------------------------------------------------------------------
void vtkAxisActor::SetAxisMainLineProperty(vtkProperty* prop)
{
  this->AxisLinesActor->SetProperty(prop);
  this->Modified();
}

vtkProperty* vtkAxisActor::GetAxisMainLineProperty()
{
  return this->GetAxisLinesProperty();
}

//------------------------------------------------------------------------------
void vtkAxisActor::SetAxisMajorTicksProperty(vtkProperty* prop)
{
  this->AxisMajorTicksActor->SetProperty(prop);
  this->Modified();
}

//------------------------------------------------------------------------------
vtkProperty* vtkAxisActor::GetAxisMajorTicksProperty()
{
  return this->AxisMajorTicksActor->GetProperty();
}

//------------------------------------------------------------------------------
void vtkAxisActor::SetAxisMinorTicksProperty(vtkProperty* prop)
{
  this->AxisMinorTicksActor->SetProperty(prop);
  this->Modified();
}

//------------------------------------------------------------------------------
vtkProperty* vtkAxisActor::GetAxisMinorTicksProperty()
{
  return this->AxisMinorTicksActor->GetProperty();
}

//------------------------------------------------------------------------------
void vtkAxisActor::SetGridlinesProperty(vtkProperty* prop)
{
  this->GridlinesActor->SetProperty(prop);
  this->Modified();
}

//------------------------------------------------------------------------------
vtkProperty* vtkAxisActor::GetGridlinesProperty()
{
  return this->GridlinesActor->GetProperty();
}

//------------------------------------------------------------------------------
void vtkAxisActor::SetInnerGridlinesProperty(vtkProperty* prop)
{
  this->InnerGridlinesActor->SetProperty(prop);
  this->Modified();
}

//------------------------------------------------------------------------------
vtkProperty* vtkAxisActor::GetInnerGridlinesProperty()
{
  return this->InnerGridlinesActor->GetProperty();
}

//------------------------------------------------------------------------------
void vtkAxisActor::SetGridpolysProperty(vtkProperty* prop)
{
  this->GridpolysActor->SetProperty(prop);
  this->Modified();
}

//------------------------------------------------------------------------------
vtkProperty* vtkAxisActor::GetGridpolysProperty()
{
  return this->GridpolysActor->GetProperty();
}

//------------------------------------------------------------------------------
vtkTextProperty* vtkAxisActor::GetTitleTextProperty()
{
  return this->TitleTextProperty.Get();
}

//------------------------------------------------------------------------------
vtkTextProperty* vtkAxisActor::GetLabelTextProperty()
{
  return this->LabelTextProperty.Get();
}

//------------------------------------------------------------------------------
vtkProperty* vtkAxisActor::NewTitleProperty()
{
  vtkProperty* newProp = vtkProperty::New();
  newProp->DeepCopy(this->GetProperty());
  newProp->SetColor(this->TitleTextProperty->GetColor());
  return newProp;
}

//------------------------------------------------------------------------------
vtkProperty* vtkAxisActor::NewLabelProperty()
{
  vtkProperty* newProp = vtkProperty::New();
  newProp->DeepCopy(this->GetProperty());
  newProp->SetColor(this->LabelTextProperty->GetColor());
  return newProp;
}

//------------------------------------------------------------------------------
vtkCamera* vtkAxisActor::GetCamera()
{
  return this->Camera.Get();
}

//------------------------------------------------------------------------------
double vtkAxisActor::GetDeltaMajor(int axis)
{
  return (axis >= 0 && axis <= 2) ? this->DeltaMajor[axis] : 0;
}

//------------------------------------------------------------------------------
void vtkAxisActor::SetDeltaMajor(int axis, double value)
{
  if (axis >= 0 && axis <= 2)
  {
    this->DeltaMajor[axis] = value;
  }
}

//------------------------------------------------------------------------------
double vtkAxisActor::GetMajorStart(int axis)
{
  return (axis >= 0 && axis <= 2) ? this->MajorStart[axis] : 0;
}

//------------------------------------------------------------------------------
void vtkAxisActor::SetMajorStart(int axis, double value)
{
  if (axis >= 0 && axis <= 2)
  {
    this->MajorStart[axis] = value;
  }
}

//------------------------------------------------------------------------------
bool vtkAxisActor::BoundsDisplayCoordinateChanged(vtkViewport* viewport)
{
  double transMinPt[3], transMaxPt[3];
  viewport->SetWorldPoint(this->Bounds[0], this->Bounds[2], this->Bounds[4], 1.0);
  viewport->WorldToDisplay();
  viewport->GetDisplayPoint(transMinPt);
  viewport->SetWorldPoint(this->Bounds[1], this->Bounds[3], this->Bounds[5], 1.0);
  viewport->WorldToDisplay();
  viewport->GetDisplayPoint(transMaxPt);

  if (this->LastMinDisplayCoordinate[0] != transMinPt[0] ||
    this->LastMinDisplayCoordinate[1] != transMinPt[1] ||
    this->LastMinDisplayCoordinate[2] != transMinPt[2] ||
    this->LastMaxDisplayCoordinate[0] != transMaxPt[0] ||
    this->LastMaxDisplayCoordinate[1] != transMaxPt[1] ||
    this->LastMaxDisplayCoordinate[2] != transMaxPt[2])
  {
    for (int i = 0; i < 3; ++i)
    {
      this->LastMinDisplayCoordinate[i] = transMinPt[i];
      this->LastMaxDisplayCoordinate[i] = transMaxPt[i];
    }
    return true;
  }

  return false;
}
//------------------------------------------------------------------------------
// endpoint-related methods
//------------------------------------------------------------------------------
vtkCoordinate* vtkAxisActor::GetPoint1Coordinate()
{
  vtkDebugMacro(<< this->GetClassName() << " (" << this << "): returning Point1 Coordinate address "
                << this->Point1Coordinate);
  return this->Point1Coordinate;
}

//------------------------------------------------------------------------------
vtkCoordinate* vtkAxisActor::GetPoint2Coordinate()
{
  vtkDebugMacro(<< this->GetClassName() << " (" << this << "): returning Point2 Coordinate address "
                << this->Point2Coordinate);
  return this->Point2Coordinate;
}

//------------------------------------------------------------------------------
void vtkAxisActor::SetPoint1(double x, double y, double z)
{
  this->Point1Coordinate->SetValue(x, y, z);
}

//------------------------------------------------------------------------------
void vtkAxisActor::SetPoint2(double x, double y, double z)
{
  this->Point2Coordinate->SetValue(x, y, z);
}

//------------------------------------------------------------------------------
double* vtkAxisActor::GetPoint1()
{
  return this->Point1Coordinate->GetValue();
}

//------------------------------------------------------------------------------
double* vtkAxisActor::GetPoint2()
{
  return this->Point2Coordinate->GetValue();
}

//------------------------------------------------------------------------------
// Creates points for ticks (minor, major, gridlines) in correct position
// for a generic axis.
//------------------------------------------------------------------------------
bool vtkAxisActor::BuildTickPoints(double p1[3], double p2[3], bool force)
{
  // Prevent any unwanted computation
  if (!force && (this->AxisPosition == this->LastAxisPosition) &&
    (this->TickLocation == this->LastTickLocation) &&
    (this->BoundsTime.GetMTime() < this->BuildTime.GetMTime()) &&
    (this->Point1Coordinate->GetMTime() < this->BuildTickPointsTime.GetMTime()) &&
    (this->Point2Coordinate->GetMTime() < this->BuildTickPointsTime.GetMTime()) &&
    (this->Range[0] == this->LastRange[0]) && (this->Range[1] == this->LastRange[1]))
  {
    return false;
  }

  // Reset previous objects
  this->MinorTickPts->Reset();
  this->MajorTickPts->Reset();
  this->GridlinePts->Reset();
  this->InnerGridlinePts->Reset();
  this->GridpolyPts->Reset();

  // As we assume that the Axis is not necessary aligned to the absolute X/Y/Z
  // axis, we will convert the absolute XYZ information to relative one
  // using a base composed as follow (axis, u, v)

  double coordSystem[3][3]; // axisVector, uVector, vVector

  switch (this->AxisType)
  {
    case VTK_AXIS_TYPE_X:
      memcpy(&coordSystem[0], this->AxisBaseForX, 3 * sizeof(double));
      memcpy(&coordSystem[1], this->AxisBaseForY, 3 * sizeof(double));
      memcpy(&coordSystem[2], this->AxisBaseForZ, 3 * sizeof(double));
      break;

    case VTK_AXIS_TYPE_Y:
      memcpy(&coordSystem[0], this->AxisBaseForY, 3 * sizeof(double));
      memcpy(&coordSystem[1], this->AxisBaseForX, 3 * sizeof(double));
      memcpy(&coordSystem[2], this->AxisBaseForZ, 3 * sizeof(double));
      break;

    case VTK_AXIS_TYPE_Z:
      memcpy(&coordSystem[0], this->AxisBaseForZ, 3 * sizeof(double));
      memcpy(&coordSystem[1], this->AxisBaseForX, 3 * sizeof(double));
      memcpy(&coordSystem[2], this->AxisBaseForY, 3 * sizeof(double));
      break;

    default:
      // shouldn't get here
      break;
  }

  //-----------------------------------------------------------------------------*
  // Build Minor Ticks
  //-----------------------------------------------------------------------------*
  if (this->Log)
  {
    this->BuildMinorTicksLog(p1, p2, coordSystem);
  }
  else
  {
    this->BuildMinorTicks(p1, p2, coordSystem);
  }

  //-----------------------------------------------------------------------------*
  // Build Gridline + GridPoly points + InnerGrid (Only for Orthonormal base)
  //-----------------------------------------------------------------------------*
  if (!this->Log)
  {
    BuildAxisGridLines(p1, p2, coordSystem);
  }

  //-----------------------------------------------------------------------------*
  // Build Major ticks
  //-----------------------------------------------------------------------------*
  if (this->Log)
  {
    BuildMajorTicksLog(p1, p2, coordSystem);
  }
  else
  {
    BuildMajorTicks(p1, p2, coordSystem);
  }

  this->BuildTickPointsTime.Modified();
  this->LastTickLocation = this->TickLocation;
  return true;
}

//------------------------------------------------------------------------------
void vtkAxisActor::BuildAxisGridLines(double p1[3], double p2[3], double localCoordSys[3][3])
{
  int uIndex = 0, vIndex = 0;
  double uGridLength = 0.0, vGridLength = 0.0;
  double gridPointClosest[3], gridPointFarest[3], gridPointU[3], gridPointV[3];
  double innerGridPointClosestU[3], innerGridPointClosestV[3];
  double innerGridPointFarestU[3], innerGridPointFarestV[3];
  double deltaVector[3];

  int uMult = vtkAxisActorMultiplierTable1[this->AxisPosition];
  int vMult = vtkAxisActorMultiplierTable2[this->AxisPosition];

  double* axisVector = localCoordSys[0];
  double* uVector = localCoordSys[1];
  double* vVector = localCoordSys[2];

  switch (this->AxisType)
  {
    case VTK_AXIS_TYPE_X:
      uGridLength = this->GridlineYLength;
      vGridLength = this->GridlineZLength;
      uIndex = 1;
      vIndex = 2;
      break;
    case VTK_AXIS_TYPE_Y:
      uGridLength = this->GridlineXLength;
      vGridLength = this->GridlineZLength;
      uIndex = 0;
      vIndex = 2;
      break;
    case VTK_AXIS_TYPE_Z:
      uGridLength = this->GridlineXLength;
      vGridLength = this->GridlineYLength;
      uIndex = 0;
      vIndex = 1;
      break;
    default:
      // shouldn't get here
      break;
  }

  bool hasOrthogonalVectorBase = this->AxisBaseForX[0] == 1 && this->AxisBaseForX[1] == 0 &&
    this->AxisBaseForX[2] == 0 && this->AxisBaseForY[0] == 0 && this->AxisBaseForY[1] == 1 &&
    this->AxisBaseForY[2] == 0 && this->AxisBaseForZ[0] == 0 && this->AxisBaseForZ[1] == 0 &&
    this->AxisBaseForZ[2] == 1;

  // - Initialize all points to be on the axis
  for (int i = 0; i < 3; i++)
  {
    gridPointClosest[i] = gridPointFarest[i] = gridPointU[i] = gridPointV[i] = p1[i];
    deltaVector[i] = (p2[i] - p1[i]);
  }

  double axisLength = vtkMath::Norm(deltaVector);
  double rangeScale = axisLength / (this->Range[1] - this->Range[0]);

  // - Test that the delta is numerically different from zero
  if (fabs(this->DeltaMajor[this->AxisType]) <= FLT_EPSILON)
  {
    return;
  }

  // - Reduce the deltaVector to correspond to a major tick step
  vtkMath::Normalize(deltaVector);
  for (int i = 0; i < 3; i++)
  {
    deltaVector[i] *= this->DeltaMajor[this->AxisType];
  }

  // - Move base points
  for (int i = 0; i < 3; i++)
  {
    gridPointU[i] -= uVector[i] * uMult * uGridLength;
    gridPointV[i] -= vVector[i] * vMult * vGridLength;
    gridPointFarest[i] -= uVector[i] * uMult * uGridLength + vVector[i] * vMult * vGridLength;
  }

  // - Add the initial shift if any
  double axisShift = (this->MajorRangeStart - this->Range[0]) * rangeScale;
  for (int i = 0; i < 3; i++)
  {
    gridPointU[i] += axisVector[i] * axisShift;
    gridPointV[i] += axisVector[i] * axisShift;
    gridPointFarest[i] += axisVector[i] * axisShift;
    gridPointClosest[i] += axisVector[i] * axisShift;
  }

  // - Insert Gridlines points along the axis using the DeltaMajor vector
  double nbIterationAsDouble = (axisLength - axisShift) / vtkMath::Norm(deltaVector);

  // - Test number of points for numerical difficulties
  if (!std::isfinite(nbIterationAsDouble) || (nbIterationAsDouble <= 0) ||
    (nbIterationAsDouble > VTK_MAX_TICKS))
  {
    return;
  }

  int nbIteration = vtkMath::Floor(nbIterationAsDouble + 2 * FLT_EPSILON) + 1;
  nbIteration = (nbIteration < VTK_MAX_TICKS) ? nbIteration : VTK_MAX_TICKS;
  for (int nbTicks = 0; nbTicks < nbIteration; nbTicks++)
  {
    // Closest U
    this->GridlinePts->InsertNextPoint(gridPointClosest);
    this->GridlinePts->InsertNextPoint(gridPointU);

    // Farthest U
    this->GridlinePts->InsertNextPoint(gridPointFarest);
    this->GridlinePts->InsertNextPoint(gridPointU);

    // Closest V
    this->GridlinePts->InsertNextPoint(gridPointClosest);
    this->GridlinePts->InsertNextPoint(gridPointV);

    // Farthest V
    this->GridlinePts->InsertNextPoint(gridPointFarest);
    this->GridlinePts->InsertNextPoint(gridPointV);

    // PolyPoints
    this->GridpolyPts->InsertNextPoint(gridPointClosest);
    this->GridpolyPts->InsertNextPoint(gridPointU);
    this->GridpolyPts->InsertNextPoint(gridPointFarest);
    this->GridpolyPts->InsertNextPoint(gridPointV);

    // Move forward along the axis
    for (int i = 0; i < 3; i++)
    {
      gridPointClosest[i] += deltaVector[i];
      gridPointU[i] += deltaVector[i];
      gridPointFarest[i] += deltaVector[i];
      gridPointV[i] += deltaVector[i];
    }
  }

  // - Insert InnerGridLines points

  // We can only handle inner grid line with orthonormal base, otherwise
  // we would need to change the API of AxisActor which we don't want for
  // backward compatibility.
  if (hasOrthogonalVectorBase)
  {
    double axis, u, v;
    axis = this->MajorStart[this->AxisType];
    innerGridPointClosestU[vIndex] = this->GetBounds()[vIndex * 2];
    innerGridPointFarestU[vIndex] = this->GetBounds()[vIndex * 2 + 1];
    innerGridPointClosestV[uIndex] = this->GetBounds()[uIndex * 2];
    innerGridPointFarestV[uIndex] = this->GetBounds()[uIndex * 2 + 1];
    while (axis <= p2[this->AxisType])
    {
      innerGridPointClosestU[this->AxisType] = innerGridPointClosestV[this->AxisType] =
        innerGridPointFarestU[this->AxisType] = innerGridPointFarestV[this->AxisType] = axis;

      // u lines
      u = this->MajorStart[uIndex];
      while (u <= p2[uIndex] && this->DeltaMajor[uIndex] > FLT_EPSILON)
      {
        innerGridPointClosestU[uIndex] = innerGridPointFarestU[uIndex] = u;
        this->InnerGridlinePts->InsertNextPoint(innerGridPointClosestU);
        this->InnerGridlinePts->InsertNextPoint(innerGridPointFarestU);
        u += this->DeltaMajor[uIndex];
      }

      // v lines
      v = this->MajorStart[vIndex];
      while (v <= p2[vIndex] && this->DeltaMajor[vIndex] > FLT_EPSILON)
      {
        innerGridPointClosestV[vIndex] = innerGridPointFarestV[vIndex] = v;
        this->InnerGridlinePts->InsertNextPoint(innerGridPointClosestV);
        this->InnerGridlinePts->InsertNextPoint(innerGridPointFarestV);
        v += this->DeltaMajor[vIndex];
      }

      axis += this->DeltaMajor[this->AxisType];
    }
  }
}

//------------------------------------------------------------------------------
void vtkAxisActor::BuildMinorTicks(double p1[3], double p2[3], double localCoordSys[3][3])
{
  // (p2 - p1) vector
  double deltaVector[3];

  // inside point: point shifted toward x,y,z direction
  // outside points:  point shifted toward -x,-y,-z direction
  double vPointInside[3], vPointOutside[3], uPointInside[3], uPointOutside[3];

  int uMult = vtkAxisActorMultiplierTable1[this->AxisPosition];
  int vMult = vtkAxisActorMultiplierTable2[this->AxisPosition];

  double* axisVector = localCoordSys[0];
  double* uVector = localCoordSys[1];
  double* vVector = localCoordSys[2];

  // - Initialize all points to be on the axis
  for (int i = 0; i < 3; i++)
  {
    uPointInside[i] = vPointInside[i] = uPointOutside[i] = vPointOutside[i] = p1[i];
    deltaVector[i] = (p2[i] - p1[i]);
  }

  double axisLength = vtkMath::Norm(deltaVector);
  double rangeScale = axisLength / (this->Range[1] - this->Range[0]);

  // - Move outside points if needed (Axis -> Outside)
  if (this->TickLocation == VTK_TICKS_OUTSIDE || this->TickLocation == VTK_TICKS_BOTH)
  {
    for (int i = 0; i < 3; i++)
    {
      uPointOutside[i] += uVector[i] * uMult * this->MinorTickSize;
      vPointOutside[i] += vVector[i] * vMult * this->MinorTickSize;
    }
  }

  // - Move inside points if needed (Axis -> Inside)
  if (this->TickLocation == VTK_TICKS_INSIDE || this->TickLocation == VTK_TICKS_BOTH)
  {
    for (int i = 0; i < 3; i++)
    {
      uPointInside[i] -= uVector[i] * uMult * this->MinorTickSize;
      vPointInside[i] -= vVector[i] * vMult * this->MinorTickSize;
    }
  }

  // - Add the initial shift if any
  double axisShift = (this->MinorRangeStart - this->Range[0]) * rangeScale;
  axisLength -= axisShift;
  for (int i = 0; i < 3; i++)
  {
    uPointInside[i] += axisVector[i] * axisShift;
    vPointInside[i] += axisVector[i] * axisShift;
    uPointOutside[i] += axisVector[i] * axisShift;
    vPointOutside[i] += axisVector[i] * axisShift;
  }

  // - Reduce the deltaVector to correspond to a tick step
  vtkMath::Normalize(deltaVector);
  double deltaMinor = this->DeltaRangeMinor * rangeScale;

  if (deltaMinor <= 0.0)
  {
    return;
  }

  // - Estimate number of steps to catch numerical difficulties
  double nTicks = axisLength / deltaMinor;
  if (!std::isfinite(nTicks) || (nTicks <= 0) || (nTicks > VTK_MAX_TICKS))
  {
    return;
  }

  // - Insert tick points along the axis using the deltaVector

  // step is a multiple of deltaMajor value
  // currentStep as well, except for the last value, it doesn't exceed axisLength

  double step = 0.0, currentStep = 0.0;
  double tickPoint[3];
  while (currentStep < axisLength)
  {
    currentStep = (step > axisLength) ? axisLength : step;

    // axis/u side
    for (int i = 0; i < 3; i++)
    {
      tickPoint[i] = deltaVector[i] * currentStep + uPointInside[i];
    }
    this->MinorTickPts->InsertNextPoint(tickPoint);

    // axis/u side
    for (int i = 0; i < 3; i++)
    {
      tickPoint[i] = deltaVector[i] * currentStep + uPointOutside[i];
    }
    this->MinorTickPts->InsertNextPoint(tickPoint);

    // axis/v side
    for (int i = 0; i < 3; i++)
    {
      tickPoint[i] = deltaVector[i] * currentStep + vPointInside[i];
    }
    this->MinorTickPts->InsertNextPoint(tickPoint);

    // axis/v side
    for (int i = 0; i < 3; i++)
      tickPoint[i] = deltaVector[i] * currentStep + vPointOutside[i];
    this->MinorTickPts->InsertNextPoint(tickPoint);

    step += deltaMinor;
  }
}

//------------------------------------------------------------------------------
void vtkAxisActor::BuildMajorTicks(double p1[3], double p2[3], double localCoordSys[3][3])
{
  double deltaVector[3];
  double* axisVector = localCoordSys[0];
  double* uVector = localCoordSys[1];
  double* vVector = localCoordSys[2];

  // inside point: point shifted toward x,y,z direction
  // outside points:  point shifted toward -x,-y,-z direction
  double vPointInside[3], vPointOutside[3], uPointInside[3], uPointOutside[3];

  int uMult = vtkAxisActorMultiplierTable1[this->AxisPosition];
  int vMult = vtkAxisActorMultiplierTable2[this->AxisPosition];

  // init deltaVector
  for (int i = 0; i < 3; i++)
  {
    deltaVector[i] = (p2[i] - p1[i]);
  }

  double axisLength = vtkMath::Norm(deltaVector);

  // factor of conversion world coord <-> range
  double rangeScale = axisLength / (this->Range[1] - this->Range[0]);

  // Delta vector is already initialized with the Major tick scale
  // - Initialize all points to be on the axis
  for (int i = 0; i < 3; i++)
  {
    uPointInside[i] = vPointInside[i] = uPointOutside[i] = vPointOutside[i] = p1[i];
    this->TickVector[i] = uVector[i] * uMult * this->MajorTickSize;
  }

  // - Move outside points if needed (Axis -> Outside)
  if (this->TickLocation == VTK_TICKS_OUTSIDE || this->TickLocation == VTK_TICKS_BOTH)
  {
    for (int i = 0; i < 3; i++)
    {
      uPointOutside[i] += this->TickVector[i];
      vPointOutside[i] += vVector[i] * vMult * this->MajorTickSize;
    }
  }

  // - Move inside points if needed (Axis -> Inside)
  if (this->TickLocation == VTK_TICKS_INSIDE || this->TickLocation == VTK_TICKS_BOTH)
  {
    for (int i = 0; i < 3; i++)
    {
      uPointInside[i] -= this->TickVector[i];
      vPointInside[i] -= vVector[i] * vMult * this->MajorTickSize;
    }
  }

  // - Add the initial shift if any
  double axisShift = (this->MajorRangeStart - this->Range[0]) * rangeScale;
  axisLength -= axisShift;
  for (int i = 0; i < 3; i++)
  {
    uPointInside[i] += axisVector[i] * axisShift;
    vPointInside[i] += axisVector[i] * axisShift;
    uPointOutside[i] += axisVector[i] * axisShift;
    vPointOutside[i] += axisVector[i] * axisShift;
  }

  // - Reduce the deltaVector to correspond to a major tick step
  vtkMath::Normalize(deltaVector);
  double deltaMajor = this->DeltaRangeMajor * rangeScale;

  if (deltaMajor <= 0.0)
  {
    return;
  }

  // - Estimate number of steps to catch numerical difficulties
  double nTicks = axisLength / deltaMajor;
  if (!std::isfinite(nTicks) || (nTicks <= 0) || (nTicks > VTK_MAX_TICKS))
  {
    return;
  }

  // - Insert tick points along the axis using the deltaVector

  // step is a multiple of deltaMajor value
  // currentStep as well, except for the last value, it doesn't exceed axisLength

  double step = 0.0, currentStep = 0.0;
  double tickPoint[3];
  while (currentStep < axisLength)
  {
    currentStep =
      (step + (this->LastMajorTickPointCorrection * this->DeltaRangeMajor / 2) > axisLength)
      ? axisLength
      : step;

    // axis/u side
    for (int i = 0; i < 3; i++)
      tickPoint[i] = deltaVector[i] * currentStep + uPointInside[i];
    this->MajorTickPts->InsertNextPoint(tickPoint);

    // axis/u side
    for (int i = 0; i < 3; i++)
      tickPoint[i] = deltaVector[i] * currentStep + uPointOutside[i];
    this->MajorTickPts->InsertNextPoint(tickPoint);

    // axis/v side
    for (int i = 0; i < 3; i++)
      tickPoint[i] = deltaVector[i] * currentStep + vPointInside[i];
    this->MajorTickPts->InsertNextPoint(tickPoint);

    // axis/v side
    for (int i = 0; i < 3; i++)
      tickPoint[i] = deltaVector[i] * currentStep + vPointOutside[i];
    this->MajorTickPts->InsertNextPoint(tickPoint);

    step += deltaMajor;
  }
}

//------------------------------------------------------------------------------
void vtkAxisActor::BuildMinorTicksLog(double p1[3], double p2[3], double localCoordSys[3][3])
{
  double deltaVector[3];

  double* axisVector = localCoordSys[0];
  double* uVector = localCoordSys[1];
  double* vVector = localCoordSys[2];

  // inside point: point shifted toward x,y,z direction
  // outside points:  point shifted toward -x,-y,-z direction
  double uPointInside[3], vPointInside[3], uPointOutside[3], vPointOutside[3];

  for (int i = 0; i < 3; i++)
  {
    uPointInside[i] = vPointInside[i] = uPointOutside[i] = vPointOutside[i] = p1[i];
    deltaVector[i] = (p2[i] - p1[i]);
  }

  double axisLength = vtkMath::Norm(deltaVector);

  // factor of conversion world coord <-> range
  double rangeScale = axisLength / log10(this->Range[1] / this->Range[0]);

  vtkMath::Normalize(deltaVector);

  int uMult = vtkAxisActorMultiplierTable1[this->AxisPosition];
  int vMult = vtkAxisActorMultiplierTable2[this->AxisPosition];
  // - Move outside points if needed (Axis -> Outside)
  if (this->TickLocation == VTK_TICKS_OUTSIDE || this->TickLocation == VTK_TICKS_BOTH)
  {
    for (int i = 0; i < 3; i++)
    {
      uPointOutside[i] += uVector[i] * uMult * this->MinorTickSize;
      vPointOutside[i] += vVector[i] * vMult * this->MinorTickSize;
    }
  }

  // - Move inside points if needed (Axis -> Inside)
  if (this->TickLocation == VTK_TICKS_INSIDE || this->TickLocation == VTK_TICKS_BOTH)
  {
    for (int i = 0; i < 3; i++)
    {
      uPointInside[i] -= uVector[i] * uMult * this->MinorTickSize;
      vPointInside[i] -= vVector[i] * vMult * this->MinorTickSize;
    }
  }

  // - Add the initial shift if any
  double axisShift = log10(this->MinorRangeStart / this->Range[0]) * rangeScale;
  axisLength -= axisShift;
  for (int i = 0; i < 3; i++)
  {
    uPointInside[i] += axisVector[i] * axisShift;
    vPointInside[i] += axisVector[i] * axisShift;
    uPointOutside[i] += axisVector[i] * axisShift;
    vPointOutside[i] += axisVector[i] * axisShift;
  }

  double base = 10.0, index;
  double step, tickRangeVal, tickVal;

  // pre set values
  double log10Range0 = log10(this->Range[0]), log10Range1 = log10(this->Range[1]);
  double lowBound = pow(base, floor(log10Range0)), upBound = pow(base, ceil(log10Range1));

  // log scale can't work with lowBound <= 0
  if (!std::isfinite(lowBound) || (lowBound <= 0))
  {
    return;
  }

  double minorTickPoint[3], minorTickOnAxis[3];

  // step match the minor tick log step, varying between each major tick.
  // for log10: minor step is 0.1 between 0.1 and 1.0, then 1.0 between 1.0 and 10.0, and so on
  for (step = lowBound; step < upBound; step *= base)
  {
    // number of minor tick between two major tick.  for log10, index goes to 2.0 to 9.0
    for (index = 2.0; index < base; index += 1.0)
    {
      tickRangeVal = index * step;

      // particular cases:
      if (tickRangeVal <= Range[0])
      {
        continue;
      }

      // over upper bound
      if (tickRangeVal >= Range[1])
      {
        break;
      }

      tickVal = (log10(tickRangeVal) - log10Range0) * rangeScale;

      // set tick point on axis (not inserted)
      for (int i = 0; i < 3; i++)
      {
        minorTickOnAxis[i] = deltaVector[i] * (tickVal);
      }
      // vtkMath::Add(minorTickOnAxis, p1, minorTickOnAxis); // handled later

      // u inside point
      vtkMath::Add(minorTickOnAxis, uPointInside, minorTickPoint);
      this->MinorTickPts->InsertNextPoint(minorTickPoint);

      // u outside point
      vtkMath::Add(minorTickOnAxis, uPointOutside, minorTickPoint);
      this->MinorTickPts->InsertNextPoint(minorTickPoint);

      if (this->Use2DMode == 0)
      {
        // v inside point
        vtkMath::Add(minorTickOnAxis, vPointInside, minorTickPoint);
        this->MinorTickPts->InsertNextPoint(minorTickPoint);

        // v outside point
        vtkMath::Add(minorTickOnAxis, vPointOutside, minorTickPoint);
        this->MinorTickPts->InsertNextPoint(minorTickPoint);
      }
    }
  }
}

//------------------------------------------------------------------------------
void vtkAxisActor::BuildMajorTicksLog(double p1[3], double p2[3], double localCoordSys[3][3])
{
  double deltaVector[3];

  double* axisVector = localCoordSys[0];
  double* uVector = localCoordSys[1];
  double* vVector = localCoordSys[2];

  // inside point: point shifted toward x,y,z direction
  // outside points:  point shifted toward -x,-y,-z direction
  double vPointInside[3], vPointOutside[3], uPointInside[3], uPointOutside[3];

  for (int i = 0; i < 3; i++)
  {
    uPointInside[i] = vPointInside[i] = uPointOutside[i] = vPointOutside[i] = p1[i];
    deltaVector[i] = (p2[i] - p1[i]);
  }

  // length of axis (wold coord system)
  double axisLength = vtkMath::Norm(deltaVector);

  // factor of conversion world coord <-> range
  double rangeScale = axisLength / log10(this->Range[1] / this->Range[0]);

  vtkMath::Normalize(deltaVector);

  int uMult = vtkAxisActorMultiplierTable1[this->AxisPosition];
  int vMult = vtkAxisActorMultiplierTable2[this->AxisPosition];

  for (int i = 0; i < 3; i++)
  {
    this->TickVector[i] = uVector[i] * uMult * this->MajorTickSize;
  }

  // - Move outside points if needed (Axis -> Outside)
  if (this->TickLocation == VTK_TICKS_OUTSIDE || this->TickLocation == VTK_TICKS_BOTH)
  {
    for (int i = 0; i < 3; i++)
    {
      uPointOutside[i] += this->TickVector[i];
      vPointOutside[i] += vVector[i] * vMult * this->MajorTickSize;
    }
  }

  // - Move inside points if needed (Axis -> Inside)
  if (this->TickLocation == VTK_TICKS_INSIDE || this->TickLocation == VTK_TICKS_BOTH)
  {
    for (int i = 0; i < 3; i++)
    {
      uPointInside[i] -= this->TickVector[i];
      vPointInside[i] -= vVector[i] * vMult * this->MajorTickSize;
    }
  }

  // - Add the initial shift if any
  double axisShift = log10(this->MajorRangeStart / this->Range[0]) * rangeScale;
  axisLength -= axisShift;
  for (int i = 0; i < 3; i++)
  {
    uPointInside[i] += axisVector[i] * axisShift;
    vPointInside[i] += axisVector[i] * axisShift;
    uPointOutside[i] += axisVector[i] * axisShift;
    vPointOutside[i] += axisVector[i] * axisShift;
  }

  double base = 10.0;
  double indexTickRangeValue;
  double tickVal, tickRangeVal;
  double log10Range0 = log10(this->Range[0]), log10Range1 = log10(this->Range[1]);
  double lowBound = pow(base, (int)floor(log10Range0)), upBound = pow(base, (int)ceil(log10Range1));

  // log scale can't work with lowBound <= 0
  if (!std::isfinite(lowBound) || (lowBound <= 0))
  {
    return;
  }

  double majorTickOnAxis[3], majorTickPoint[3];
  for (indexTickRangeValue = lowBound; indexTickRangeValue <= upBound; indexTickRangeValue *= base)
  {
    tickRangeVal = indexTickRangeValue;

    if (indexTickRangeValue < this->Range[0])
    {
      tickRangeVal = this->Range[0];
    }
    else if (indexTickRangeValue > this->Range[1])
    {
      tickRangeVal = this->Range[1];
    }

    tickVal = (log10(tickRangeVal) - log10Range0) * rangeScale;

    for (int i = 0; i < 3; i++)
    {
      majorTickOnAxis[i] = deltaVector[i] * tickVal;
    }

    // u inside point
    vtkMath::Add(majorTickOnAxis, uPointInside, majorTickPoint);
    this->MajorTickPts->InsertNextPoint(majorTickPoint);

    // u outside point
    vtkMath::Add(majorTickOnAxis, uPointOutside, majorTickPoint);
    this->MajorTickPts->InsertNextPoint(majorTickPoint);

    // v inside point
    vtkMath::Add(majorTickOnAxis, vPointInside, majorTickPoint);
    this->MajorTickPts->InsertNextPoint(majorTickPoint);

    // v outside point
    vtkMath::Add(majorTickOnAxis, vPointOutside, majorTickPoint);
    this->MajorTickPts->InsertNextPoint(majorTickPoint);
  }
}

//------------------------------------------------------------------------------
void vtkAxisActor::RotateActor2DFromAxisProjection(vtkTextActor* pActor2D)
{
  double* p1 = this->Point1Coordinate->GetValue();
  double* p2 = this->Point2Coordinate->GetValue();

  vtkMatrix4x4* matModelView = this->Camera->GetModelViewTransformMatrix();
  double nearPlane = this->Camera->GetClippingRange()[0];

  // Need view coordinate points.
  double viewPt1[4] = { p1[0], p1[1], p1[2], 1.0 };
  double viewPt2[4] = { p2[0], p2[1], p2[2], 1.0 };

  matModelView->MultiplyPoint(viewPt1, viewPt1);
  matModelView->MultiplyPoint(viewPt2, viewPt2);

  if (viewPt1[2] == 0.0 || viewPt2[2] == 0.0)
  {
    return;
  }

  double p1Pjt[3] = { -nearPlane * viewPt1[0] / viewPt1[2], -nearPlane * viewPt1[1] / viewPt1[2],
    -nearPlane };
  double p2Pjt[3] = { -nearPlane * viewPt2[0] / viewPt2[2], -nearPlane * viewPt2[1] / viewPt2[2],
    -nearPlane };

  double axisOnScreen[2] = { p2Pjt[0] - p1Pjt[0], p2Pjt[1] - p1Pjt[1] };
  double x[2] = { 1.0, 0.0 }, y[2] = { 0.0, 1.0 };

  double dotProd = vtkMath::Dot2D(x, axisOnScreen);

  double orient = 0.0;
  if (vtkMath::Norm2D(axisOnScreen) == 0.0)
  {
    pActor2D->SetOrientation(0.0);
    return;
  }
  else
  {
    orient = acos(dotProd / vtkMath::Norm2D(axisOnScreen));
    orient = vtkMath::DegreesFromRadians(orient);
  }

  // adjust angle
  if (vtkMath::Dot2D(y, axisOnScreen) < 0.0)
  {
    orient *= -1.0;
  }

  if (vtkMath::Dot2D(x, axisOnScreen) < 0.0)
  {
    orient += 180.0;
  }

  pActor2D->SetOrientation(orient);
}
VTK_ABI_NAMESPACE_END
