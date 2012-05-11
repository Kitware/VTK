/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCubeAxesActor2D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCubeAxesActor2D.h"

#include "vtkAxisActor2D.h"
#include "vtkCamera.h"
#include "vtkDataSet.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkTextProperty.h"
#include "vtkTrivialProducer.h"
#include "vtkViewport.h"

vtkStandardNewMacro(vtkCubeAxesActor2D);

vtkCxxSetObjectMacro(vtkCubeAxesActor2D,Camera,vtkCamera);
vtkCxxSetObjectMacro(vtkCubeAxesActor2D,ViewProp,vtkProp);
vtkCxxSetObjectMacro(vtkCubeAxesActor2D,AxisLabelTextProperty,vtkTextProperty);
vtkCxxSetObjectMacro(vtkCubeAxesActor2D,AxisTitleTextProperty,vtkTextProperty);

class vtkCubeAxesActor2DConnection : public vtkAlgorithm
{
public:
  static vtkCubeAxesActor2DConnection *New();
  vtkTypeMacro(vtkCubeAxesActor2DConnection,vtkAlgorithm);

  vtkCubeAxesActor2DConnection()
    {
      this->SetNumberOfInputPorts(1);
    }
};

vtkStandardNewMacro(vtkCubeAxesActor2DConnection);

//----------------------------------------------------------------------------
// Instantiate this object.
vtkCubeAxesActor2D::vtkCubeAxesActor2D()
{
  this->ConnectionHolder = vtkCubeAxesActor2DConnection::New();

  this->ViewProp = NULL;
  this->Bounds[0] = -1.0; this->Bounds[1] = 1.0;
  this->Bounds[2] = -1.0; this->Bounds[3] = 1.0;
  this->Bounds[4] = -1.0; this->Bounds[5] = 1.0;

  this->UseRanges = 0;
  this->Ranges[0] = 0; this->Ranges[1] = 0;
  this->Ranges[2] = 0; this->Ranges[3] = 0;
  this->Ranges[4] = 0; this->Ranges[5] = 0;

  this->Camera = NULL;
  this->FlyMode = VTK_FLY_CLOSEST_TRIAD;
  this->Scaling = 1;

  this->XAxis = vtkAxisActor2D::New();
  this->XAxis->GetPositionCoordinate()->SetCoordinateSystemToDisplay();
  this->XAxis->GetPosition2Coordinate()->SetCoordinateSystemToDisplay();
  this->XAxis->AdjustLabelsOff();

  this->YAxis = vtkAxisActor2D::New();
  this->YAxis->GetPositionCoordinate()->SetCoordinateSystemToDisplay();
  this->YAxis->GetPosition2Coordinate()->SetCoordinateSystemToDisplay();
  this->YAxis->AdjustLabelsOff();

  this->ZAxis = vtkAxisActor2D::New();
  this->ZAxis->GetPositionCoordinate()->SetCoordinateSystemToDisplay();
  this->ZAxis->GetPosition2Coordinate()->SetCoordinateSystemToDisplay();
  this->ZAxis->AdjustLabelsOff();

  this->NumberOfLabels = 3;

  this->AxisLabelTextProperty = vtkTextProperty::New();
  this->AxisLabelTextProperty->SetBold(1);
  this->AxisLabelTextProperty->SetItalic(1);
  this->AxisLabelTextProperty->SetShadow(1);
  this->AxisLabelTextProperty->SetFontFamilyToArial();

  this->AxisTitleTextProperty = vtkTextProperty::New();
  this->AxisTitleTextProperty->ShallowCopy(this->AxisLabelTextProperty);

  this->LabelFormat = new char[8];
  sprintf(this->LabelFormat,"%s","%-#6.3g");
  this->FontFactor = 1.0;
  this->CornerOffset = 0.05;
  this->Inertia = 1;
  this->ShowActualBounds = 1;
  this->RenderCount = 0;

  this->XAxisVisibility = 1;
  this->YAxisVisibility = 1;
  this->ZAxisVisibility = 1;

  this->XLabel = new char[2];
  sprintf(this->XLabel,"%s","X");
  this->YLabel = new char[2];
  sprintf(this->YLabel,"%s","Y");
  this->ZLabel = new char[2];
  sprintf(this->ZLabel,"%s","Z");

  // Allow the user to specify an origin for the axes. The axes will then run
  // from this origin to the bounds and will cross over at this origin.
  this->XOrigin = VTK_DOUBLE_MAX;
  this->YOrigin = VTK_DOUBLE_MAX;
  this->ZOrigin = VTK_DOUBLE_MAX;
}

//----------------------------------------------------------------------------
// Shallow copy of an actor.
void vtkCubeAxesActor2D::ShallowCopy(vtkCubeAxesActor2D *actor)
{
  this->vtkActor2D::ShallowCopy(actor);
  this->SetAxisLabelTextProperty(actor->GetAxisLabelTextProperty());
  this->SetAxisTitleTextProperty(actor->GetAxisTitleTextProperty());
  this->SetLabelFormat(actor->GetLabelFormat());
  this->SetFontFactor(actor->GetFontFactor());
  this->SetCornerOffset(actor->GetCornerOffset());
  this->SetInertia(static_cast<int>(actor->GetInertia()));
  this->SetXLabel(actor->GetXLabel());
  this->SetYLabel(actor->GetYLabel());
  this->SetZLabel(actor->GetZLabel());
  this->SetFlyMode(actor->GetFlyMode());
  this->SetInputConnection(
    actor->ConnectionHolder->GetInputConnection(0, 0));
  this->SetViewProp(actor->GetViewProp());
  this->SetCamera(actor->GetCamera());
}

//----------------------------------------------------------------------------
vtkCubeAxesActor2D::~vtkCubeAxesActor2D()
{
  this->ConnectionHolder->Delete();

  if ( this->ViewProp )
    {
    this->ViewProp->Delete();
    }

  if ( this->Camera )
    {
    this->Camera->UnRegister(this);
    }

  this->XAxis->Delete();
  this->YAxis->Delete();
  this->ZAxis->Delete();

  if (this->LabelFormat)
    {
    delete [] this->LabelFormat;
    this->LabelFormat = NULL;
    }

  if ( this->XLabel )
    {
    delete [] this->XLabel;
    }
  if ( this->YLabel )
    {
    delete [] this->YLabel;
    }
  if ( this->ZLabel )
    {
    delete [] this->ZLabel;
    }

  this->SetAxisLabelTextProperty(NULL);
  this->SetAxisTitleTextProperty(NULL);
}

//----------------------------------------------------------------------------
void vtkCubeAxesActor2D::SetInputConnection(vtkAlgorithmOutput* ao)
{
  this->ConnectionHolder->SetInputConnection(ao);
}

//----------------------------------------------------------------------------
void vtkCubeAxesActor2D::SetInputData(vtkDataSet* ds)
{
  vtkTrivialProducer* tp = vtkTrivialProducer::New();
  tp->SetOutput(ds);
  this->SetInputConnection(tp->GetOutputPort());
  tp->Delete();
}

//----------------------------------------------------------------------------
vtkDataSet* vtkCubeAxesActor2D::GetInput()
{
  return vtkDataSet::SafeDownCast(
    this->ConnectionHolder->GetInputDataObject(0, 0));
}

//----------------------------------------------------------------------------
// Static variable describes connections in cube.
static int Conn[8][3] = {{1,2,4}, {0,3,5}, {3,0,6}, {2,1,7},
                         {5,6,0}, {4,7,1}, {7,4,2}, {6,5,3}};

//----------------------------------------------------------------------------
// Project the bounding box and compute edges on the border of the bounding
// cube. Determine which parts of the edges are visible via intersection
// with the boundary of the viewport (minus borders).
int vtkCubeAxesActor2D::RenderOverlay(vtkViewport *viewport)
{
  int renderedSomething=0;

  // Initialization
  if ( ! this->RenderSomething )
    {
    return 0;
    }

  //Render the axes
  if ( this->XAxisVisibility )
    {
    renderedSomething += this->XAxis->RenderOverlay(viewport);
    }
  if ( this->YAxisVisibility )
    {
    renderedSomething += this->YAxis->RenderOverlay(viewport);
    }
  if ( this->ZAxisVisibility )
    {
    renderedSomething += this->ZAxis->RenderOverlay(viewport);
    }

  return renderedSomething;
}

//----------------------------------------------------------------------------
// Project the bounding box and compute edges on the border of the bounding
// cube. Determine which parts of the edges are visible via intersection
// with the boundary of the viewport (minus borders).
int vtkCubeAxesActor2D::RenderOpaqueGeometry(vtkViewport *viewport)
{
  double bounds[6];
  double slope = 0.0, minSlope, num, den;
  double pts[8][3], d2, d2Min, min;
  int i, idx = 0;
  int xIdx = 0, yIdx = 0, zIdx = 0, zIdx2 = 0, renderedSomething=0;
  int xAxes = 0, yAxes = 0, zAxes = 0;

  // Initialization
  if ( !this->Camera )
    {
    vtkErrorMacro(<<"No camera!");
    this->RenderSomething = 0;
    return 0;
    }

  this->RenderSomething = 1;

  // determine the bounds to use
  this->GetBounds(bounds);

  // Check for user specified origins. By default, these are placed at a corner of the
  // bounding box of the dataset (corner is based on the fly mode)
  if (this->XOrigin != VTK_DOUBLE_MAX)
    {
    bounds[0] = this->XOrigin;
    }
  if (this->YOrigin != VTK_DOUBLE_MAX)
    {
    bounds[2] = this->YOrigin;
    }
  if (this->ZOrigin != VTK_DOUBLE_MAX)
    {
    bounds[4] = this->ZOrigin;
    }

  // Build the axes (almost always needed so we don't check mtime)
  // Transform all points into display coordinates
  this->TransformBounds(viewport, bounds, pts);

  // Find the portion of the bounding box that fits within the viewport,
  if ( !this->ShowActualBounds && (this->ClipBounds(viewport, pts, bounds) == 0) )
    {
    this->RenderSomething = 0;
    return 0;
    }

  // Take into account the inertia. Process only so often.
  if ( this->RenderCount++ == 0 || !(this->RenderCount % this->Inertia) )
    {
    // Okay, we have a bounding box, maybe clipped and scaled, that is visible.
    // We setup the axes depending on the fly mode.
    if (this->FlyMode == VTK_FLY_NONE)
      {
      idx = 2; // Just use the default axis orientation.
      xAxes = 0;
      xIdx = Conn[idx][0];
      yAxes = 1;
      yIdx = Conn[idx][1];
      zAxes = 2;
      zIdx = idx;
      zIdx2 = Conn[idx][2];
      }
    else if ( this->FlyMode == VTK_FLY_CLOSEST_TRIAD )
      {
      // Loop over points and find the closest point to the camera
      min = VTK_DOUBLE_MAX;
      for (i=0; i < 8; i++)
        {
        if ( pts[i][2] < min )
          {
          idx = i;
          min = pts[i][2];
          }
        }

      // Setup the three axes to be drawn
      xAxes = 0;
      xIdx = Conn[idx][0];
      yAxes = 1;
      yIdx = Conn[idx][1];
      zAxes = 2;
      zIdx = idx;
      zIdx2 = Conn[idx][2];
      }
    else if (this->FlyMode == VTK_FLY_OUTER_EDGES)
      {
      double e1[2], e2[2], e3[2];

      // Find distance to origin
      d2Min = VTK_DOUBLE_MAX;
      for (i=0; i < 8; i++)
        {
        d2 = pts[i][0]*pts[i][0] + pts[i][1]*pts[i][1];
        if ( d2 < d2Min )
          {
          d2Min = d2;
          idx = i;
          }
        }

      // find minimum slope point connected to closest point and on
      // right side (in projected coordinates). This is the first edge.
      minSlope = VTK_DOUBLE_MAX;
      for (xIdx=0, i=0; i<3; i++)
        {
        num = (pts[Conn[idx][i]][1] - pts[idx][1]);
        den = (pts[Conn[idx][i]][0] - pts[idx][0]);
        if ( den != 0.0 )
          {
          slope = num / den;
          }
        if ( slope < minSlope && den > 0 )
          {
          xIdx = Conn[idx][i];
          yIdx = Conn[idx][(i+1)%3];
          zIdx = Conn[idx][(i+2)%3];
          xAxes = i;
          minSlope = slope;
          }
        }

      // find edge (connected to closest point) on opposite side
      for ( i=0; i<2; i++)
        {
        e1[i] = (pts[xIdx][i] - pts[idx][i]);
        e2[i] = (pts[yIdx][i] - pts[idx][i]);
        e3[i] = (pts[zIdx][i] - pts[idx][i]);
        }
      vtkMath::Normalize2D(e1);
      vtkMath::Normalize2D(e2);
      vtkMath::Normalize2D(e3);

      if ( vtkMath::Dot2D(e1,e2) < vtkMath::Dot2D(e1,e3) )
        {
        yAxes = (xAxes + 1) % 3;
        }
      else
        {
        yIdx = zIdx;
        yAxes = (xAxes + 2) % 3;
        }

      // Find the final point by determining which global x-y-z axes have not
      // been represented, and then determine the point closest to the viewer.
      zAxes = (xAxes != 0 && yAxes != 0 ? 0 :
              (xAxes != 1 && yAxes != 1 ? 1 : 2));
      if ( pts[Conn[xIdx][zAxes]][2] < pts[Conn[yIdx][zAxes]][2] )
        {
        zIdx = xIdx;
        zIdx2 = Conn[xIdx][zAxes];
        }
      else
        {
        zIdx = yIdx;
        zIdx2 = Conn[yIdx][zAxes];
        }
      }//else boundary edges fly mode
    this->InertiaAxes[0] = idx;
    this->InertiaAxes[1] = xIdx;
    this->InertiaAxes[2] = yIdx;
    this->InertiaAxes[3] = zIdx;
    this->InertiaAxes[4] = zIdx2;
    this->InertiaAxes[5] = xAxes;
    this->InertiaAxes[6] = yAxes;
    this->InertiaAxes[7] = zAxes;
    } //inertia
  else
    {
    idx = this->InertiaAxes[0];
    xIdx = this->InertiaAxes[1];
    yIdx = this->InertiaAxes[2];
    zIdx = this->InertiaAxes[3];
    zIdx2 = this->InertiaAxes[4];
    xAxes = this->InertiaAxes[5];
    yAxes = this->InertiaAxes[6];
    zAxes = this->InertiaAxes[7];
    }

  // Setup the axes for plotting
  double xCoords[4], yCoords[4], zCoords[4], xRange[2], yRange[2], zRange[2];
  this->AdjustAxes(pts, bounds, idx, xIdx, yIdx, zIdx, zIdx2,
                   xAxes, yAxes, zAxes,
                   xCoords, yCoords, zCoords, xRange, yRange, zRange);

  // Sorry for ugly hack. I find the fonts slightly too large on the axis. BNW
  double AxisFontFactor = this->FontFactor*.75;


  // Upate axes
  this->Labels[0] = this->XLabel;
  this->Labels[1] = this->YLabel;
  this->Labels[2] = this->ZLabel;

  this->XAxis->GetPositionCoordinate()->SetValue(xCoords[0], xCoords[1]);
  this->XAxis->GetPosition2Coordinate()->SetValue(xCoords[2], xCoords[3]);
  this->XAxis->SetRange(xRange[0], xRange[1]);
  this->XAxis->SetTitle(this->Labels[xAxes]);
  this->XAxis->SetNumberOfLabels(this->NumberOfLabels);
  this->XAxis->SetLabelFormat(this->LabelFormat);
  this->XAxis->SetFontFactor(AxisFontFactor);
  this->XAxis->SetProperty(this->GetProperty());

  this->YAxis->GetPositionCoordinate()->SetValue(yCoords[2], yCoords[3]);
  this->YAxis->GetPosition2Coordinate()->SetValue(yCoords[0], yCoords[1]);
  this->YAxis->SetRange(yRange[1], yRange[0]);
  this->YAxis->SetTitle(this->Labels[yAxes]);
  this->YAxis->SetNumberOfLabels(this->NumberOfLabels);
  this->YAxis->SetLabelFormat(this->LabelFormat);
  this->YAxis->SetFontFactor(AxisFontFactor);
  this->YAxis->SetProperty(this->GetProperty());

  this->ZAxis->GetPositionCoordinate()->SetValue(zCoords[0], zCoords[1]);
  this->ZAxis->GetPosition2Coordinate()->SetValue(zCoords[2], zCoords[3]);
  this->ZAxis->SetRange(zRange[0], zRange[1]);
  this->ZAxis->SetTitle(this->Labels[zAxes]);
  this->ZAxis->SetNumberOfLabels(this->NumberOfLabels);
  this->ZAxis->SetLabelFormat(this->LabelFormat);
  this->ZAxis->SetFontFactor(AxisFontFactor);
  this->ZAxis->SetProperty(this->GetProperty());

  // Rebuid text props
  // Perform shallow copy here since each individual axis can be
  // accessed through the class API (i.e. each individual axis text prop
  // can be changed). Therefore, we can not just assign pointers otherwise
  // each individual axis text prop would point to the same text prop.

  if (this->AxisLabelTextProperty &&
      this->AxisLabelTextProperty->GetMTime() > this->BuildTime)
    {
    if (this->XAxis->GetLabelTextProperty())
      {
      this->XAxis->GetLabelTextProperty()->ShallowCopy(
        this->AxisLabelTextProperty);
      }
    if (this->YAxis->GetLabelTextProperty())
      {
      this->YAxis->GetLabelTextProperty()->ShallowCopy(
        this->AxisLabelTextProperty);
      }
    if (this->ZAxis->GetLabelTextProperty())
      {
      this->ZAxis->GetLabelTextProperty()->ShallowCopy(
        this->AxisLabelTextProperty);
      }
    }

  if (this->AxisTitleTextProperty &&
      this->AxisTitleTextProperty->GetMTime() > this->BuildTime)
    {
    if (this->XAxis->GetLabelTextProperty())
      {
      this->XAxis->GetTitleTextProperty()->ShallowCopy(
        this->AxisTitleTextProperty);
      }
    if (this->YAxis->GetLabelTextProperty())
      {
      this->YAxis->GetTitleTextProperty()->ShallowCopy(
        this->AxisTitleTextProperty);
      }
    if (this->ZAxis->GetLabelTextProperty())
      {
      this->ZAxis->GetTitleTextProperty()->ShallowCopy(
        this->AxisTitleTextProperty);
      }
    }

  this->BuildTime.Modified();

  //Render the axes
  if ( this->XAxisVisibility )
    {
    renderedSomething += this->XAxis->RenderOpaqueGeometry(viewport);
    }
  if ( this->YAxisVisibility )
    {
    renderedSomething += this->YAxis->RenderOpaqueGeometry(viewport);
    }
  if ( this->ZAxisVisibility )
    {
    renderedSomething += this->ZAxis->RenderOpaqueGeometry(viewport);
    }

  return renderedSomething;
}

//-----------------------------------------------------------------------------
// Description:
// Does this prop have some translucent polygonal geometry?
int vtkCubeAxesActor2D::HasTranslucentPolygonalGeometry()
{
  return 0;
}

//----------------------------------------------------------------------------
// Do final adjustment of axes to control offset, etc.
void vtkCubeAxesActor2D::AdjustAxes(double pts[8][3], double bounds[6],
                      int idx, int xIdx, int yIdx, int zIdx, int zIdx2,
                      int xAxes, int yAxes, int zAxes,
                      double xCoords[4], double yCoords[4], double zCoords[4],
                      double xRange[2], double yRange[2], double zRange[2])
{
  double *internal_bounds;
  if ( this->UseRanges )
  {
    internal_bounds = this->Ranges;
  }
  else
  {
    internal_bounds = bounds;
  }

  // The x-axis
  xCoords[0] = pts[idx][0];
  xCoords[1] = pts[idx][1];
  xCoords[2] = pts[xIdx][0];
  xCoords[3] = pts[xIdx][1];
  if ( idx < xIdx )
    {
      xRange[0] = internal_bounds[2*xAxes];
      xRange[1] = internal_bounds[2*xAxes+1];
    }
  else
    {
      xRange[0] = internal_bounds[2*xAxes+1];
      xRange[1] = internal_bounds[2*xAxes];
    }

  // The y-axis
  yCoords[0] = pts[idx][0];
  yCoords[1] = pts[idx][1];
  yCoords[2] = pts[yIdx][0];
  yCoords[3] = pts[yIdx][1];
  if ( idx < yIdx )
    {
      yRange[0] = internal_bounds[2*yAxes];
      yRange[1] = internal_bounds[2*yAxes+1];
    }
  else
    {
      yRange[0] = internal_bounds[2*yAxes+1];
      yRange[1] = internal_bounds[2*yAxes];
    }

  // The z-axis
  if ( zIdx != xIdx && zIdx != idx ) //rearrange for labels
    {
    zIdx = zIdx2;
    zIdx2 = yIdx;
    }

  zCoords[0] = pts[zIdx][0];
  zCoords[1] = pts[zIdx][1];
  zCoords[2] = pts[zIdx2][0];
  zCoords[3] = pts[zIdx2][1];
  if ( zIdx < zIdx2 )
    {
      zRange[0] = internal_bounds[2*zAxes];
      zRange[1] = internal_bounds[2*zAxes+1];
    }
  else
    {
      zRange[0] = internal_bounds[2*zAxes+1];
      zRange[1] = internal_bounds[2*zAxes];
    }

  // Pull back the corners if specified
  if ( this->CornerOffset > 0.0 )
    {
    double ave;

    // x-axis
    ave = (xCoords[0] + xCoords[2]) / 2.0;
    xCoords[0] = xCoords[0] - this->CornerOffset * (xCoords[0] - ave);
    xCoords[2] = xCoords[2] - this->CornerOffset * (xCoords[2] - ave);

    ave = (xCoords[1] + xCoords[3]) / 2.0;
    xCoords[1] = xCoords[1] - this->CornerOffset * (xCoords[1] - ave);
    xCoords[3] = xCoords[3] - this->CornerOffset * (xCoords[3] - ave);

    ave = (xRange[1] + xRange[0]) / 2.0;
    if (!this->ShowActualBounds)
      {
      xRange[0] = xRange[0] - this->CornerOffset * (xRange[0] - ave);
      xRange[1] = xRange[1] - this->CornerOffset * (xRange[1] - ave);
      }

    // y-axis
    ave = (yCoords[0] + yCoords[2]) / 2.0;
    yCoords[0] = yCoords[0] - this->CornerOffset * (yCoords[0] - ave);
    yCoords[2] = yCoords[2] - this->CornerOffset * (yCoords[2] - ave);

    ave = (yCoords[1] + yCoords[3]) / 2.0;
    yCoords[1] = yCoords[1] - this->CornerOffset * (yCoords[1] - ave);
    yCoords[3] = yCoords[3] - this->CornerOffset * (yCoords[3] - ave);

    ave = (yRange[1] + yRange[0]) / 2.0;
    if (!this->ShowActualBounds)
      {
      yRange[0] = yRange[0] - this->CornerOffset * (yRange[0] - ave);
      yRange[1] = yRange[1] - this->CornerOffset * (yRange[1] - ave);
      }

    // z-axis
    ave = (zCoords[0] + zCoords[2]) / 2.0;
    zCoords[0] = zCoords[0] - this->CornerOffset * (zCoords[0] - ave);
    zCoords[2] = zCoords[2] - this->CornerOffset * (zCoords[2] - ave);

    ave = (zCoords[1] + zCoords[3]) / 2.0;
    zCoords[1] = zCoords[1] - this->CornerOffset * (zCoords[1] - ave);
    zCoords[3] = zCoords[3] - this->CornerOffset * (zCoords[3] - ave);

    ave = (zRange[1] + zRange[0]) / 2.0;
    if (!this->ShowActualBounds)
      {
      zRange[0] = zRange[0] - this->CornerOffset * (zRange[0] - ave);
      zRange[1] = zRange[1] - this->CornerOffset * (zRange[1] - ave);
      }
    }
}

//----------------------------------------------------------------------------
// Release any graphics resources that are being consumed by this actor.
// The parameter window could be used to determine which graphic
// resources to release.
void vtkCubeAxesActor2D::ReleaseGraphicsResources(vtkWindow *win)
{
  this->XAxis->ReleaseGraphicsResources(win);
  this->YAxis->ReleaseGraphicsResources(win);
  this->ZAxis->ReleaseGraphicsResources(win);
}

//----------------------------------------------------------------------------
// Return the ranges
void vtkCubeAxesActor2D::GetRanges(double ranges[6])
{
  int i;
  for ( i=0; i<6; i++ )
    {
    ranges[i] = this->Ranges[i];
    }
}

//----------------------------------------------------------------------------
// Compute the ranges
void vtkCubeAxesActor2D::GetRanges(double& xmin, double& xmax,
                                   double& ymin, double& ymax,
                                   double& zmin, double& zmax)
{
  double ranges[6];
  this->GetRanges(ranges);
  xmin = ranges[0];
  xmax = ranges[1];
  ymin = ranges[2];
  ymax = ranges[3];
  zmin = ranges[4];
  zmax = ranges[5];
}

//----------------------------------------------------------------------------
// Compute the bounds
double *vtkCubeAxesActor2D::GetRanges()
{
  double ranges[6];
  this->GetRanges(ranges);
  return this->Ranges;
}

//----------------------------------------------------------------------------
// Compute the bounds
void vtkCubeAxesActor2D::GetBounds(double bounds[6])
{
  double *propBounds;
  int i;

  if ( this->GetInput() )
    {
    this->ConnectionHolder->GetInputAlgorithm()->Update();
    this->GetInput()->GetBounds(bounds);
    for (i=0; i< 6; i++)
      {
      this->Bounds[i] = bounds[i];
      }
    }
  else if ( this->ViewProp &&
            ((propBounds = this->ViewProp->GetBounds()) && propBounds != NULL) )
    {
    for (i=0; i< 6; i++)
      {
      bounds[i] = this->Bounds[i] = propBounds[i];
      }
    }
  else
    {
    for (i=0; i< 6; i++)
      {
      bounds[i] = this->Bounds[i];
      }
    }
}

//----------------------------------------------------------------------------
// Compute the bounds
void vtkCubeAxesActor2D::GetBounds(double& xmin, double& xmax,
                                   double& ymin, double& ymax,
                                   double& zmin, double& zmax)
{
  double bounds[6];
  this->GetBounds(bounds);
  xmin = bounds[0];
  xmax = bounds[1];
  ymin = bounds[2];
  ymax = bounds[3];
  zmin = bounds[4];
  zmax = bounds[5];
}

//----------------------------------------------------------------------------
// Compute the bounds
double *vtkCubeAxesActor2D::GetBounds()
{
  this->GetBounds(this->Bounds);
  return this->Bounds;
}

//----------------------------------------------------------------------------
void vtkCubeAxesActor2D::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  if ( this->GetInput() )
    {
    os << indent << "Input: (" << (void *)this->GetInput() << ")\n";
    }
  else
    {
    os << indent << "Input: (none)\n";
    }

  if ( this->ViewProp )
    {
    os << indent << "ViewProp: (" << (void *)this->ViewProp << ")\n";
    }
  else
    {
    os << indent << "ViewProp: (none)\n";
    }

  os << indent << "Bounds: \n";
  os << indent << "  Xmin,Xmax: (" << this->Bounds[0] << ", "
     << this->Bounds[1] << ")\n";
  os << indent << "  Ymin,Ymax: (" << this->Bounds[2] << ", "
     << this->Bounds[3] << ")\n";
  os << indent << "  Zmin,Zmax: (" << this->Bounds[4] << ", "
     << this->Bounds[5] << ")\n";

  if ( this->Camera )
    {
    os << indent << "Camera:\n";
    this->Camera->PrintSelf(os,indent.GetNextIndent());
    }
  else
    {
    os << indent << "Camera: (none)\n";
    }

  if (this->AxisTitleTextProperty)
    {
    os << indent << "Axis Title Text Property:\n";
    this->AxisTitleTextProperty->PrintSelf(os,indent.GetNextIndent());
    }
  else
    {
    os << indent << "Axis Title Text Property: (none)\n";
    }

  if (this->AxisLabelTextProperty)
    {
    os << indent << "Axis Label Text Property:\n";
    this->AxisLabelTextProperty->PrintSelf(os,indent.GetNextIndent());
    }
  else
    {
    os << indent << "Axis Label Text Property: (none)\n";
    }

  if ( this->FlyMode == VTK_FLY_CLOSEST_TRIAD )
    {
    os << indent << "Fly Mode: CLOSEST_TRIAD\n";
    }
  else if (this->FlyMode == VTK_FLY_OUTER_EDGES)
    {
    os << indent << "Fly Mode: OUTER_EDGES\n";
    }
  else if (this->FlyMode == VTK_FLY_NONE)
    {
    os << indent << "Fly Mode: Disabled\n";
    }

  os << indent << "Scaling: " << (this->Scaling ? "On\n" : "Off\n");
  os << indent << "UseRanges: " << (this->UseRanges ? "On\n" : "Off\n");
  os << indent << "Ranges: \n";
  os << indent << "  Xmin,Xmax: (" << this->Ranges[0] << ", "
     << this->Ranges[1] << ")\n";
  os << indent << "  Ymin,Ymax: (" << this->Ranges[2] << ", "
     << this->Ranges[3] << ")\n";
  os << indent << "  Zmin,Zmax: (" << this->Ranges[4] << ", "
     << this->Ranges[5] << ")\n";

  os << indent << "Number Of Labels: " << this->NumberOfLabels << "\n";
  os << indent << "X Label: " << this->XLabel << "\n";
  os << indent << "Y Label: " << this->YLabel << "\n";
  os << indent << "Z Label: " << this->ZLabel << "\n";

  os << indent << "X Axis Visibility: " << (this->XAxisVisibility ? "On\n" : "Off\n");
  os << indent << "Y Axis Visibility: " << (this->YAxisVisibility ? "On\n" : "Off\n");
  os << indent << "Z Axis Visibility: " << (this->ZAxisVisibility ? "On\n" : "Off\n");

  os << indent << "Label Format: " << this->LabelFormat << "\n";
  os << indent << "Font Factor: " << this->FontFactor << "\n";
  os << indent << "Inertia: " << this->Inertia << "\n";
  os << indent << "Corner Offset: " << this->CornerOffset << "\n";
  os << indent << "UseRanges: " << (this->UseRanges ? "On" : "Off") << "\n";
  os << indent << "Ranges: "
     << this->Ranges[0] << ", " << this->Ranges[1] << ", "
     << this->Ranges[2] << ", " << this->Ranges[3] << ", "
     << this->Ranges[4] << ", " << this->Ranges[5] << "\n";
  os << indent << "Show Actual Bounds: " << (this->ShowActualBounds ? "On\n" : "Off\n");
  if (this->XOrigin != VTK_DOUBLE_MAX)
    {
    os << indent << "User specified X Origin: " << this->XOrigin << endl;
    }
  if (this->YOrigin != VTK_DOUBLE_MAX)
    {
    os << indent << "User specified Y Origin: " << this->YOrigin << endl;
    }
  if (this->ZOrigin != VTK_DOUBLE_MAX)
    {
    os << indent << "User specified Z Origin: " << this->ZOrigin << endl;
    }
}

//----------------------------------------------------------------------------
static int IsInBounds(double x[3], double bounds[6]);

// Clip the axes to fit into the viewport. Do this clipping each of the three
// axes to determine which part of the cube is in view. Returns 0 if
// nothing should be drawn.
#define VTK_DIVS 10
int vtkCubeAxesActor2D::ClipBounds(vtkViewport *viewport, double pts[8][3],
                                   double bounds[6])
{
  int i, j, k, numIters;
  double planes[24];
  double x[3];
  double val, maxVal=0, anchor[3], scale;
  double delX, delY, delZ;
  double bounds2[6];
  double scale2, newScale, origin[3];
  double aspect[2];

  // Only do this mojo if scaling is required
  if ( ! this->Scaling )
    {
    return 1;
    }

  // Get the 6 planes defining the view frustrum
  viewport->GetAspect( aspect );
  this->Camera->GetFrustumPlanes((aspect[0] / aspect[1]), planes);

  // Hunt for the point in the bounds furthest inside the frustum
  // Iteratively loop over points in bounding box and evaluate the
  // maximum minimum distance. Find the point furthest inside of the
  // bounding box. Use this as an anchor point to scale to. Repeat
  // the process to hone in on the best point.
  delX = (bounds[1]-bounds[0]) / (VTK_DIVS-1);
  delY = (bounds[3]-bounds[2]) / (VTK_DIVS-1);
  delZ = (bounds[5]-bounds[4]) / (VTK_DIVS-1);
  anchor[0] = (bounds[1]+bounds[0])/2.0;
  anchor[1] = (bounds[3]+bounds[2])/2.0;
  anchor[2] = (bounds[5]+bounds[4])/2.0;

  for ( numIters=0; numIters < 8; numIters++)
    {
    origin[0] = anchor[0] - delX*(VTK_DIVS-1)/2.0;
    origin[1] = anchor[1] - delY*(VTK_DIVS-1)/2.0;
    origin[2] = anchor[2] - delZ*(VTK_DIVS-1)/2.0;

    for (maxVal=0.0, k=0; k<VTK_DIVS; k++)
      {
      x[2] = origin[2] + k * delZ;
      for (j=0; j<VTK_DIVS; j++)
        {
        x[1] = origin[1] + j * delY;
        for (i=0; i<VTK_DIVS; i++)
          {
          x[0] = origin[0] + i * delX;
          if ( IsInBounds(x,bounds) )
            {
            val = this->EvaluatePoint(planes, x);
            if ( val > maxVal )
              {
              anchor[0] = x[0];
              anchor[1] = x[1];
              anchor[2] = x[2];
              maxVal = val;
              }
            }//if in bounding box
          }//i
        }//j
      }//k

    delX /= (VTK_DIVS-1) * 1.414;
    delY /= (VTK_DIVS-1) * 1.414;
    delZ /= (VTK_DIVS-1) * 1.414;
    }//Iteratively find anchor point

  if ( maxVal <= 0.0 )
    {
    return 0; //couldn't find a point inside
    }

  // Now  iteratively scale the bounding box until all points are inside
  // the frustrum. Use bisection method.
  scale = 1.0;
  scale2 = 0.00001;
  val = this->EvaluateBounds(planes, bounds);

  // Get other end point for bisection technique
  for (i=0; i<3; i++)
    {
    bounds2[2*i] = (bounds[2*i]-anchor[i])*scale2 + anchor[i];
    bounds2[2*i+1] = (bounds[2*i+1]-anchor[i])*scale2 + anchor[i];
    }
  val = this->EvaluateBounds(planes, bounds2);
  if ( val <= 0.0 )
    {
    return 0; //not worth drawing - too small
    }

  for ( numIters=0; numIters < 10; numIters++)
    {
    newScale = (scale + scale2) / 2.0;
    for (i=0; i<3; i++)
      {
      bounds2[2*i] = (bounds[2*i]-anchor[i])*newScale + anchor[i];
      bounds2[2*i+1] = (bounds[2*i+1]-anchor[i])*newScale + anchor[i];
      }
    val = this->EvaluateBounds(planes, bounds2);

    if ( val > 0.0 )
      {
      scale2 = newScale;
      }
    else
      {
      scale = newScale;
      }
    }//for converged

  for (i=0; i<6; i++) //copy the result
    {
    bounds[i] = bounds2[i];
    }

  this->TransformBounds(viewport, bounds, pts);

  return 1;
}
#undef VTK_DIVS

//----------------------------------------------------------------------------
void vtkCubeAxesActor2D::TransformBounds(vtkViewport *viewport,
                                         double bounds[6], double pts[8][3])
{
  int i, j, k, idx;
  double x[3];

  //loop over verts of bounding box
  for (k=0; k<2; k++)
    {
    x[2] = bounds[4+k];
    for (j=0; j<2; j++)
      {
      x[1] = bounds[2+j];
      for (i=0; i<2; i++)
        {
        idx = i + 2*j + 4*k;
        x[0] = bounds[i];
        viewport->SetWorldPoint(x[0],x[1],x[2],1.0);
        viewport->WorldToDisplay();
        viewport->GetDisplayPoint(pts[idx]);
        }
      }
    }
}

//----------------------------------------------------------------------------
// Return smallest value of point evaluated against frustum planes. Also
// sets the closest point coordinates in xyz.
double vtkCubeAxesActor2D::EvaluatePoint(double planes[24], double x[3])
{
  int kk;
  double *plane;
  double val, minPlanesValue;

  for (minPlanesValue=VTK_DOUBLE_MAX, kk=0; kk<6 ; kk++)
    {
    plane = planes + kk*4;
    val = plane[0]*x[0] + plane[1]*x[1] + plane[2]*x[2] + plane[3];

    if ( val < minPlanesValue )
      {
      minPlanesValue = val;
      }
    }//for all planes

  return minPlanesValue;
}

//----------------------------------------------------------------------------
// Return the smallest point of the bounding box evaluated against the
// frustum planes.
double vtkCubeAxesActor2D::EvaluateBounds(double planes[24], double bounds[6])
{
  double val, minVal, x[3];
  int i, j, k;

  for (minVal=VTK_DOUBLE_MAX, k=0; k<2; k++)
    {
    x[2] = bounds[4+k];
    for (j=0; j<2; j++)
      {
      x[1] = bounds[2+j];
      for (i=0; i<2; i++)
        {
        x[0] = bounds[i];
        val = this->EvaluatePoint(planes, x);
        if ( val < minVal )
          {
          minVal = val;
          }
        }
      }
    }//loop over verts of bounding box

  return minVal;
}

//----------------------------------------------------------------------------
static int IsInBounds(double x[3], double bounds[6])
{
  if ( x[0] < bounds[0] || x[0] > bounds[1] ||
       x[1] < bounds[2] || x[1] > bounds[3] ||
       x[2] < bounds[4] || x[2] > bounds[5])
    {
    return 0;
    }
  else
    {
    return 1;
    }
}
