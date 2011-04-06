/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCubeAxesActor.cxx
  Thanks:    Kathleen Bonnell, B Division, Lawrence Livermore National Lab

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
#include "vtkCubeAxesActor.h"

#include "vtkCamera.h"
#include "vtkCoordinate.h"
#include "vtkAxisActor.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkProperty.h"
#include "vtkStringArray.h"
#include "vtkViewport.h"

// *************************************************************************
// Modifications:
//   Kathleen Bonnell, Wed Mar  6 13:48:48 PST 2002
//   Replace 'New' method with Macro to match VTK 4.0 API.
//
// *************************************************************************

vtkStandardNewMacro(vtkCubeAxesActor);
vtkCxxSetObjectMacro(vtkCubeAxesActor, Camera,vtkCamera);

// *************************************************************************
// Instantiate this object.
//
// Modifications:
//   Kathleen Bonnell, Wed Oct 31 07:57:49 PST 2001
//   Intialize new members lastPow, last*AxisDigits.
//
//   Kathleen Bonnell, Wed Nov  7 16:19:16 PST 2001
//   Intialize new members:  Last*Extent, LastFlyMode,
//   renderAxes*, numAxes*.
//
//   Hank Childs, Fri Sep 27 17:15:07 PDT 2002
//   Initialize new members for units.
//
//   Kathleen Bonnell, Fri Jul 25 14:37:32 PDT 2003
//   Remove 'Input' and 'Prop' members, initialize new members
//   valueScaleFactor, mustAdjustValue, ForceLabelReset.
//
//   Kathleen Bonnell, Wed Aug  6 13:59:15 PDT 2003
//   Remove valueScaleFactor, replace mustAdjustValue and ForceLabelReset
//   with one for each axis type.
//
//   Kathleen Bonnell, Tue Dec 16 11:27:30 PST 2003
//   Replace Last*Extent with Last*Range.  (* = X, Y, Z)
//   Add AutoLabelScaling, UserXPow, UserYPow, UserZPow.
//
//   Brad Whitlock, Fri Jul 23 18:18:41 PST 2004
//   Added ActualXLabel et al so we can keep title separate from what's
//   actually displayed so information is not lost.
//
// *************************************************************************

vtkCubeAxesActor::vtkCubeAxesActor()
{
  this->Bounds[0] = -1.0; this->Bounds[1] = 1.0;
  this->Bounds[2] = -1.0; this->Bounds[3] = 1.0;
  this->Bounds[4] = -1.0; this->Bounds[5] = 1.0;

  this->TickLocation = VTK_TICKS_INSIDE;
  this->Camera = NULL;
  this->FlyMode = VTK_FLY_CLOSEST_TRIAD;
  int i;
  for (i = 0; i < 4; i++)
    {
    this->XAxes[i] = vtkAxisActor::New();
    this->XAxes[i]->SetTickVisibility(1);
    this->XAxes[i]->SetMinorTicksVisible(1);
    this->XAxes[i]->SetLabelVisibility(1);
    this->XAxes[i]->SetTitleVisibility(1);
    this->XAxes[i]->SetAxisTypeToX();
    this->XAxes[i]->SetAxisPosition(i);

    this->YAxes[i] = vtkAxisActor::New();
    this->YAxes[i]->SetTickVisibility(1);
    this->YAxes[i]->SetMinorTicksVisible(1);
    this->YAxes[i]->SetLabelVisibility(1);
    this->YAxes[i]->SetTitleVisibility(1);
    this->YAxes[i]->SetAxisTypeToY();
    this->YAxes[i]->SetAxisPosition(i);

    this->ZAxes[i] = vtkAxisActor::New();
    this->ZAxes[i]->SetTickVisibility(1);
    this->ZAxes[i]->SetMinorTicksVisible(1);
    this->ZAxes[i]->SetLabelVisibility(1);
    this->ZAxes[i]->SetTitleVisibility(1);
    this->ZAxes[i]->SetAxisTypeToZ();
    this->ZAxes[i]->SetAxisPosition(i);
    }

  this->XLabelFormat = new char[8];
  sprintf(this->XLabelFormat, "%s", "%-#6.3g");
  this->YLabelFormat = new char[8];
  sprintf(this->YLabelFormat, "%s", "%-#6.3g");
  this->ZLabelFormat = new char[8];
  sprintf(this->ZLabelFormat, "%s", "%-#6.3g");
  this->CornerOffset = 0.0;
  this->Inertia = 1;
  this->RenderCount = 0;

  this->XAxisVisibility = 1;
  this->YAxisVisibility = 1;
  this->ZAxisVisibility = 1;

  this->XAxisTickVisibility = 1;
  this->YAxisTickVisibility = 1;
  this->ZAxisTickVisibility = 1;

  this->XAxisMinorTickVisibility = 1;
  this->YAxisMinorTickVisibility = 1;
  this->ZAxisMinorTickVisibility = 1;

  this->XAxisLabelVisibility = 1;
  this->YAxisLabelVisibility = 1;
  this->ZAxisLabelVisibility = 1;

  this->DrawXGridlines = 0;
  this->DrawYGridlines = 0;
  this->DrawZGridlines = 0;

  this->XTitle = new char[7];
  sprintf(this->XTitle, "%s", "X-Axis");
  this->XUnits = NULL;
  this->YTitle = new char[7];
  sprintf(this->YTitle, "%s", "Y-Axis");
  this->YUnits = NULL;
  this->ZTitle = new char[7];
  sprintf(this->ZTitle, "%s", "Z-Axis");
  this->ZUnits = NULL;

  this->ActualXLabel = 0;
  this->ActualYLabel = 0;
  this->ActualZLabel = 0;

  this->LastXPow = 0;
  this->LastYPow = 0;
  this->LastZPow = 0;
  this->LastXAxisDigits = 3;
  this->LastYAxisDigits = 3;
  this->LastZAxisDigits = 3;

  this->LastXRange[0] = VTK_FLOAT_MAX;
  this->LastXRange[1] = VTK_FLOAT_MAX;
  this->LastYRange[0] = VTK_FLOAT_MAX;
  this->LastYRange[1] = VTK_FLOAT_MAX;
  this->LastZRange[0] = VTK_FLOAT_MAX;
  this->LastZRange[1] = VTK_FLOAT_MAX;

  this->LastFlyMode = -1;
  for (i = 0; i < 4; i++)
    {
    this->RenderAxesX[i] = i;
    this->RenderAxesY[i] = i;
    this->RenderAxesZ[i] = i;
    }
  this->NumberOfAxesX = this->NumberOfAxesY = this->NumberOfAxesZ = 1;

  this->MustAdjustXValue = false;
  this->MustAdjustYValue = false;
  this->MustAdjustZValue = false;

  this->ForceXLabelReset = false;
  this->ForceYLabelReset = false;
  this->ForceZLabelReset = false;

  this->AutoLabelScaling = true;
  this->UserXPow = 0;
  this->UserYPow = 0;
  this->UserZPow = 0;

  this->XAxisRange[0] = VTK_DOUBLE_MAX;
  this->XAxisRange[1] = VTK_DOUBLE_MAX;
  this->YAxisRange[0] = VTK_DOUBLE_MAX;
  this->YAxisRange[1] = VTK_DOUBLE_MAX;
  this->ZAxisRange[0] = VTK_DOUBLE_MAX;
  this->ZAxisRange[1] = VTK_DOUBLE_MAX;
}

// ****************************************************************************
// Shallow copy of an actor.
//
// Modifications:
//   Kathleen Bonnell, Wed Mar  6 13:48:48 PST 2002
//   Call superclass method the new VTK 4.0 way.
//
//   Kathleen Bonnell, Fri Jul 25 14:37:32 PDT 2003
//   Remove 'Input' and 'Prop' members, added new members
//   valueScaleFactor, mustAdjustValue, ForceLabelReset.
//
//   Kathleen Bonnell, Wed Aug  6 13:59:15 PDT 2003
//   Remove valueScaleFactor, replace mustAdjustValue and ForceLabelReset
//   with one for each axis type.
//
// ****************************************************************************

void vtkCubeAxesActor::ShallowCopy(vtkCubeAxesActor *actor)
{
  this->Superclass::ShallowCopy(actor);
  this->SetXLabelFormat(actor->GetXLabelFormat());
  this->SetYLabelFormat(actor->GetYLabelFormat());
  this->SetZLabelFormat(actor->GetZLabelFormat());
  this->SetCornerOffset(actor->GetCornerOffset());
  this->SetInertia(actor->GetInertia());
  this->SetXTitle(actor->GetXTitle());
  this->SetYTitle(actor->GetYTitle());
  this->SetZTitle(actor->GetZTitle());
  this->SetFlyMode(actor->GetFlyMode());
  this->SetCamera(actor->GetCamera());
  this->SetBounds(actor->GetBounds());
  this->MustAdjustXValue = actor->MustAdjustXValue;
  this->MustAdjustYValue = actor->MustAdjustYValue;
  this->MustAdjustZValue = actor->MustAdjustZValue;
  this->ForceXLabelReset = actor->ForceXLabelReset;
  this->ForceYLabelReset = actor->ForceYLabelReset;
  this->ForceZLabelReset = actor->ForceZLabelReset;
}

// ****************************************************************************
//  Modifications:
//
//    Hank Childs, Fri Sep 27 17:15:07 PDT 2002
//    Destruct new data members for units.
//
//    Brad Whitlock, Fri Jul 23 18:21:16 PST 2004
//    Added more items and fixed a small memory leak.
//
// ****************************************************************************

vtkCubeAxesActor::~vtkCubeAxesActor()
{
  this->SetCamera(NULL);

  for (int i = 0; i < 4; i++)
    {
    if (this->XAxes[i])
      {
      this->XAxes[i]->Delete();
      this->XAxes[i] = NULL;
      }
    if (this->YAxes[i])
      {
      this->YAxes[i]->Delete();
      this->YAxes[i] = NULL;
      }
    if (this->ZAxes[i])
      {
      this->ZAxes[i]->Delete();
      this->ZAxes[i] = NULL;
      }
    }

  if (this->XLabelFormat)
    {
    delete [] this->XLabelFormat;
    this->XLabelFormat = NULL;
    }

  if (this->YLabelFormat)
    {
    delete [] this->YLabelFormat;
    this->YLabelFormat = NULL;
    }

  if (this->ZLabelFormat)
    {
    delete [] this->ZLabelFormat;
    this->ZLabelFormat = NULL;
    }

  if (this->XTitle)
    {
    delete [] this->XTitle;
    this->XTitle = NULL;
    }
  if (this->YTitle)
    {
    delete [] this->YTitle;
    this->YTitle = NULL;
    }
  if (this->ZTitle)
    {
    delete [] this->ZTitle;
    this->ZTitle = NULL;
    }

  if (this->XUnits)
    {
    delete [] this->XUnits;
    this->XUnits = NULL;
    }
  if (this->YUnits)
    {
    delete [] this->YUnits;
    this->YUnits = NULL;
    }
  if (this->ZUnits)
    {
    delete [] this->ZUnits;
    this->ZUnits = NULL;
    }

  if (this->ActualXLabel)
    {
    delete [] this->ActualXLabel;
    this->ActualXLabel = NULL;
    }
  if (this->ActualYLabel)
    {
    delete [] this->ActualYLabel;
    this->ActualYLabel = NULL;
    }
  if (this->ActualZLabel)
    {
    delete [] this->ActualZLabel;
    this->ActualZLabel = NULL;
    }
}

// *************************************************************************
// Project the bounding box and compute edges on the border of the bounding
// cube. Determine which parts of the edges are visible via intersection
// with the boundary of the viewport (minus borders).
//
//  Modifications:
//    Kathleen Bonnell, Wed Oct 31 07:57:49 PST 2001
//    Added calls to AdjustValues, AdjustRange.
//
//   Kathleen Bonnell, Wed Nov  7 16:19:16 PST 2001
//   Only render those axes needed for current FlyMode.
//   Moved bulk of 'build' code to BuildAxes method, added calls to
//   BuildAxes and DetermineRenderAxes methods.
//
//   Kathleen Bonnell, Fri Jul 25 14:37:32 PDT 2003
//   Added initial build of each axis.
// *************************************************************************

int vtkCubeAxesActor::RenderOpaqueGeometry(vtkViewport *viewport)
{
  int i, renderedSomething=0;
  static bool initialRender = true;
  // Initialization
  if (!this->Camera)
    {
    vtkErrorMacro(<<"No camera!");
    this->RenderSomething = 0;
    return 0;
    }

  this->BuildAxes(viewport);

  if (initialRender)
    {
    for (i = 0; i < 4; i++)
      {
      this->XAxes[i]->BuildAxis(viewport, true);
      this->YAxes[i]->BuildAxis(viewport, true);
      this->ZAxes[i]->BuildAxis(viewport, true);
      }
    }
  initialRender = false;

  this->DetermineRenderAxes(viewport);

  //Render the axes
  if (this->XAxisVisibility)
    {
    for (i = 0; i < this->NumberOfAxesX; i++)
      {
      renderedSomething +=
        this->XAxes[this->RenderAxesX[i]]->RenderOpaqueGeometry(viewport);
      }
    }

  if (this->YAxisVisibility)
    {
    for (i = 0; i < this->NumberOfAxesY; i++)
      {
      renderedSomething +=
        this->YAxes[this->RenderAxesY[i]]->RenderOpaqueGeometry(viewport);
      }
    }

  if (this->ZAxisVisibility)
    {
    for (i = 0; i < this->NumberOfAxesZ; i++)
      {
      renderedSomething +=
        this->ZAxes[this->RenderAxesZ[i]]->RenderOpaqueGeometry(viewport);
      }
    }
  return renderedSomething;
}

// Do final adjustment of axes to control offset, etc.
void vtkCubeAxesActor::AdjustAxes(double bounds[6], double xCoords[4][6],
                                  double yCoords[4][6], double zCoords[4][6],
                                  double xRange[2], double yRange[2],
                                  double zRange[2])
{
  xRange[0] = (this->XAxisRange[0] == VTK_DOUBLE_MAX ?
                                  bounds[0] : this->XAxisRange[0]);
  xRange[1] = (this->XAxisRange[1] == VTK_DOUBLE_MAX ?
                                  bounds[1] : this->XAxisRange[1]);
  yRange[0] = (this->YAxisRange[0] == VTK_DOUBLE_MAX ?
                                  bounds[2] : this->YAxisRange[0]);
  yRange[1] = (this->YAxisRange[1] == VTK_DOUBLE_MAX ?
                                  bounds[3] : this->YAxisRange[1]);
  zRange[0] = (this->ZAxisRange[0] == VTK_DOUBLE_MAX ?
                                  bounds[4] : this->ZAxisRange[0]);
  zRange[1] = (this->ZAxisRange[1] == VTK_DOUBLE_MAX ?
                                  bounds[5] : this->ZAxisRange[1]);

  const double xScale = (xRange[1] - xRange[0])/(bounds[1] - bounds[0]);
  const double yScale = (yRange[1] - yRange[0])/(bounds[3] - bounds[2]);
  const double zScale = (zRange[1] - zRange[0])/(bounds[5] - bounds[4]);

  // Pull back the corners if specified
  if (this->CornerOffset > 0.0)
    {
    for (int i = 0; i < 4; i++)
      {
      double ave;

      // x-axis
      ave = (xCoords[i][0] + xCoords[i][2]) / 2.0;
      xCoords[i][0] =
        xCoords[i][0] - this->CornerOffset * (xCoords[i][0] - ave);
      xCoords[i][2] =
        xCoords[i][2] - this->CornerOffset * (xCoords[i][2] - ave);

      ave = (xCoords[i][1] + xCoords[i][3]) / 2.0;
      xCoords[i][1] =
        xCoords[i][1] - this->CornerOffset * (xCoords[i][1] - ave);
      xCoords[i][3] =
        xCoords[i][3] - this->CornerOffset * (xCoords[i][3] - ave);

      ave = (xRange[1] + xRange[0]) / 2.0;
      xRange[0] = xRange[0] - this->CornerOffset * xScale * (xRange[0] - ave);
      xRange[1] = xRange[1] - this->CornerOffset * xScale * (xRange[1] - ave);

      // y-axis
      ave = (yCoords[i][0] + yCoords[i][2]) / 2.0;
      yCoords[i][0] =
        yCoords[i][0] - this->CornerOffset * (yCoords[i][0] - ave);
      yCoords[i][2] =
        yCoords[i][2] - this->CornerOffset * (yCoords[i][2] - ave);

      ave = (yCoords[i][1] + yCoords[i][3]) / 2.0;
      yCoords[i][1] =
        yCoords[i][1] - this->CornerOffset * (yCoords[i][1] - ave);
      yCoords[i][3] =
        yCoords[i][3] - this->CornerOffset * (yCoords[i][3] - ave);

      ave = (yRange[1] + yRange[0]) / 2.0;
      yRange[0] = yRange[0] - this->CornerOffset * yScale * (yRange[0] - ave);
      yRange[1] = yRange[1] - this->CornerOffset * yScale * (yRange[1] - ave);

      // z-axis
      ave = (zCoords[i][0] + zCoords[i][2]) / 2.0;
      zCoords[i][0] =
        zCoords[i][0] - this->CornerOffset * (zCoords[i][0] - ave);
      zCoords[i][2] =
        zCoords[i][2] - this->CornerOffset * (zCoords[i][2] - ave);

      ave = (zCoords[i][1] + zCoords[i][3]) / 2.0;
      zCoords[i][1] =
        zCoords[i][1] - this->CornerOffset * (zCoords[i][1] - ave);
      zCoords[i][3] =
        zCoords[i][3] - this->CornerOffset * (zCoords[i][3] - ave);

      ave = (zRange[1] + zRange[0]) / 2.0;
      zRange[0] = zRange[0] - this->CornerOffset * zScale * (zRange[0] - ave);
      zRange[1] = zRange[1] - this->CornerOffset * zScale * (zRange[1] - ave);
      }
    }
}

// Release any graphics resources that are being consumed by this actor.
// The parameter window could be used to determine which graphic
// resources to release.
void vtkCubeAxesActor::ReleaseGraphicsResources(vtkWindow *win)
{
  for (int i = 0; i < 4; i++)
    {
    this->XAxes[i]->ReleaseGraphicsResources(win);
    this->YAxes[i]->ReleaseGraphicsResources(win);
    this->ZAxes[i]->ReleaseGraphicsResources(win);
    }
}

// *************************************************************************
// Compute the bounds
//
// Modifications:
//   Kathleen Bonnell, Fri Jul 25 14:37:32 PDT 2003
//   Removed support for Prop and Input.
// *************************************************************************
void vtkCubeAxesActor::GetBounds(double bounds[6])
{
  for (int i=0; i< 6; i++)
    {
    bounds[i] = this->Bounds[i];
    }
}

// Compute the bounds
void vtkCubeAxesActor::GetBounds(double& xmin, double& xmax,
                                 double& ymin, double& ymax,
                                 double& zmin, double& zmax)
{
  xmin = this->Bounds[0];
  xmax = this->Bounds[1];
  ymin = this->Bounds[2];
  ymax = this->Bounds[3];
  zmin = this->Bounds[4];
  zmax = this->Bounds[5];
}

// Compute the bounds
double *vtkCubeAxesActor::GetBounds()
{
  return this->Bounds;
}

// ******************************************************************
// Modifications:
//   Kathleen Bonnell, Wed Mar  6 13:48:48 PST 2002
//   Call superclass method the new VTK 4.0 way.
//
//   Kathleen Bonnell, Fri Jul 25 14:37:32 PDT 2003
//   Removed Input and Prop.
//
// ******************************************************************

void vtkCubeAxesActor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Bounds: \n";
  os << indent << "  Xmin,Xmax: (" << this->Bounds[0] << ", "
     << this->Bounds[1] << ")\n";
  os << indent << "  Ymin,Ymax: (" << this->Bounds[2] << ", "
     << this->Bounds[3] << ")\n";
  os << indent << "  Zmin,Zmax: (" << this->Bounds[4] << ", "
     << this->Bounds[5] << ")\n";


  os << indent << "XAxisRange: [" << this->XAxisRange[0] << ", "
    << this->XAxisRange[1] << "] " << endl;
  os << indent << "YAxisRange: [" << this->YAxisRange[0] << ", "
    << this->YAxisRange[1] << "] " << endl;
  os << indent << "ZAxisRange: [" << this->ZAxisRange[0] << ", "
    << this->ZAxisRange[1] << "] " << endl;

  if (this->Camera)
    {
    os << indent << "Camera:\n";
    this->Camera->PrintSelf(os,indent.GetNextIndent());
    }
  else
    {
    os << indent << "Camera: (none)\n";
    }

  if (this->FlyMode == VTK_FLY_CLOSEST_TRIAD)
    {
    os << indent << "Fly Mode: CLOSEST_TRIAD\n";
    }
  else if (this->FlyMode == VTK_FLY_FURTHEST_TRIAD)
    {
    os << indent << "Fly Mode: FURTHEST_TRIAD\n";
    }
  else if (this->FlyMode == VTK_FLY_STATIC_TRIAD)
    {
    os << indent << "Fly Mode: STATIC_TRIAD\n";
    }
  else if (this->FlyMode == VTK_FLY_STATIC_EDGES)
    {
    os << indent << "Fly Mode: STATIC_EDGES\n";
    }
  else
    {
    os << indent << "Fly Mode: OUTER_EDGES\n";
    }

  os << indent << "X Axis Title: " << this->XTitle << "\n";
  os << indent << "Y Axis Title: " << this->YTitle << "\n";
  os << indent << "Z Axis Title: " << this->ZTitle << "\n";

  os << indent << "X Axis Visibility: "
     << (this->XAxisVisibility ? "On\n" : "Off\n");
  os << indent << "Y Axis Visibility: "
     << (this->YAxisVisibility ? "On\n" : "Off\n");
  os << indent << "Z Axis Visibility: "
     << (this->ZAxisVisibility ? "On\n" : "Off\n");

  os << indent << "X Axis Label Format: " << this->XLabelFormat << "\n";
  os << indent << "Y Axis Label Format: " << this->YLabelFormat << "\n";
  os << indent << "Z Axis Label Format: " << this->ZLabelFormat << "\n";
  os << indent << "Inertia: " << this->Inertia << "\n";
  os << indent << "Corner Offset: " << this->CornerOffset << "\n";

  os << indent << "XAxisTickVisibility: "
     << (this->XAxisTickVisibility ? "On" : "Off") << endl;
  os << indent << "YAxisTickVisibility: "
     << (this->YAxisTickVisibility ? "On" : "Off") << endl;
  os << indent << "ZAxisTickVisibility: "
     << (this->ZAxisTickVisibility ? "On" : "Off") << endl;

  os << indent << "XAxisMinorTickVisibility: "
     << (this->XAxisMinorTickVisibility ? "On" : "Off") << endl;
  os << indent << "YAxisMinorTickVisibility: "
     << (this->YAxisMinorTickVisibility ? "On" : "Off") << endl;
  os << indent << "ZAxisMinorTickVisibility: "
     << (this->ZAxisMinorTickVisibility ? "On" : "Off") << endl;

  os << indent << "XAxisLabelVisibility: "
     << (this->XAxisLabelVisibility ? "On" : "Off") << endl;
  os << indent << "YAxisLabelVisibility: "
     << (this->YAxisLabelVisibility ? "On" : "Off") << endl;
  os << indent << "ZAxisLabelVisibility: "
     << (this->ZAxisLabelVisibility ? "On" : "Off") << endl;

  os << indent << "XUnits: "
     << (this->XUnits ? this->XUnits : "(none)") << endl;
  os << indent << "YUnits: "
     << (this->YUnits ? this->YUnits : "(none)") << endl;
  os << indent << "ZUnits: "
     << (this->ZUnits ? this->ZUnits : "(none)") << endl;

  os << indent << "TickLocation: " << this->TickLocation << endl;

  os << indent << "DrawXGridlines: " << this->DrawXGridlines << endl;
  os << indent << "DrawYGridlines: " << this->DrawYGridlines << endl;
  os << indent << "DrawZGridlines: " << this->DrawZGridlines << endl;
}

void vtkCubeAxesActor::TransformBounds(vtkViewport *viewport,
                                       const double bounds[6],
                                       double pts[8][3])
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

// ***********************************************************************
//
//  Calculate the size (length) of major and minor ticks,
//  based on an average of the coordinate direction ranges.
//  Set the necessary Axes methods with the calculated information.
//
//  Returns:  false if tick size not recomputed, true otherwise.
//
//  Modifications:
//    Kathleen Bonnell, Wed Nov  7 16:19:16 PST 2001
//    Added logic for early-termination.
//
//    Kathleen Bonnell, Fri Jul 18 09:09:31 PDT 2003
//    Added return value, added calls to AdjustTicksComputeRange and
//    BuildLabels.
//
//    Kathleen Bonnell, Mon Dec 15 14:59:26 PST 2003
//    Use the actual range values instead of range-extents to determine
//    if tick size needs to be recomputed.
//
// ***********************************************************************

bool vtkCubeAxesActor::ComputeTickSize(double bounds[6])
{
  bool xRangeChanged = this->LastXRange[0] != bounds[0] ||
                       this->LastXRange[1] != bounds[1];

  bool yRangeChanged = this->LastYRange[0] != bounds[2] ||
                       this->LastYRange[1] != bounds[3];

  bool zRangeChanged = this->LastZRange[0] != bounds[4] ||
                       this->LastZRange[1] != bounds[5];

  if (!(xRangeChanged || yRangeChanged || zRangeChanged))
    {
    // no need to re-compute ticksize.
    return false;
    }

  int i;
  double xExt = bounds[1] - bounds[0];
  double yExt = bounds[3] - bounds[2];
  double zExt = bounds[5] - bounds[4];

  if (xRangeChanged)
    {
    this->AdjustTicksComputeRange(this->XAxes, bounds[0], bounds[1]);
    this->BuildLabels(this->XAxes);
    }
  if (yRangeChanged)
    {
    this->AdjustTicksComputeRange(this->YAxes, bounds[2], bounds[3]);
    this->BuildLabels(this->YAxes);
    }
  if (zRangeChanged)
    {
    this->AdjustTicksComputeRange(this->ZAxes, bounds[4], bounds[5]);
    this->BuildLabels(this->ZAxes);
    }

  this->LastXRange[0] = (this->XAxisRange[0] == VTK_DOUBLE_MAX ?
                                  bounds[0] : this->XAxisRange[0]);
  this->LastXRange[1] = (this->XAxisRange[1] == VTK_DOUBLE_MAX ?
                                  bounds[1] : this->XAxisRange[1]);
  this->LastYRange[0] = (this->YAxisRange[0] == VTK_DOUBLE_MAX ?
                                  bounds[2] : this->YAxisRange[0]);
  this->LastYRange[1] = (this->YAxisRange[1] == VTK_DOUBLE_MAX ?
                                  bounds[3] : this->YAxisRange[1]);
  this->LastZRange[0] = (this->ZAxisRange[0] == VTK_DOUBLE_MAX ?
                                  bounds[4] : this->ZAxisRange[0]);
  this->LastZRange[1] = (this->ZAxisRange[1] == VTK_DOUBLE_MAX ?
                                  bounds[5] : this->ZAxisRange[1]);

  double major = 0.02 * (xExt + yExt + zExt) / 3.;
  double minor = 0.5 * major;
  for (i = 0; i < 4; i++)
    {
    this->XAxes[i]->SetMajorTickSize(major);
    this->XAxes[i]->SetMinorTickSize(minor);

    this->YAxes[i]->SetMajorTickSize(major);
    this->YAxes[i]->SetMinorTickSize(minor);

    this->ZAxes[i]->SetMajorTickSize(major);
    this->ZAxes[i]->SetMinorTickSize(minor);

    this->XAxes[i]->SetGridlineXLength(xExt);
    this->XAxes[i]->SetGridlineYLength(yExt);
    this->XAxes[i]->SetGridlineZLength(zExt);

    this->YAxes[i]->SetGridlineXLength(xExt);
    this->YAxes[i]->SetGridlineYLength(yExt);
    this->YAxes[i]->SetGridlineZLength(zExt);

    this->ZAxes[i]->SetGridlineXLength(xExt);
    this->ZAxes[i]->SetGridlineYLength(yExt);
    this->ZAxes[i]->SetGridlineZLength(zExt);
    }
  return true;
}

// ****************************************************************************
//  Method: vtkCubeAxesActor::AdjustValues
//
//  Purpose:
//      If the range of values is too big or too small, put them in scientific
//      notation and changes the labels.
//
//  Arguments:
//      bnds     The min/max values in each coordinate direction:
//                 (min_x, max_x, min_y, max_y, min_z, max_x).
//
//  Note:       This code is partially stolen from old MeshTV code,
//              /meshtvx/toolkit/plotgrid.c, axlab[x|y].
//
//  Programmer: Hank Childs
//  Creation:   July 11, 2000
//
//  Modifications:
//    Kathleen Bonnell, Wed Oct 31 07:57:49 PST 2001
//    Regardless of individual ranges, if any coord direction has too
//    small/large a range, all will have a scale factor set for scaling their
//    label values, and their titles adjusted accordingly.
//
//    Kathleen Bonnell, Thu Sep  5 17:32:16 PDT 2002
//    Only use dimensions with range > 0 for determining scale factor.
//
//    Hank Childs, Fri Sep 27 17:15:07 PDT 2002
//    Account for units.
//
//    Kathleen Bonnell, Wed Aug  6 13:59:15 PDT 2003
//    Each axis type now has its own 'mustAdjustValue' and 'lastPow'.
//
//    Kathleen Bonnell, Tue Dec 16 11:23:31 PST 2003
//    Allow the LabelExponent to be user-settable (autLabelScaling is off).
//    For title use '10e' instead of just 'e' to designate that exponent
//    has been used.
//
//    Kathleen Bonnell, Tue Jul 20 11:41:45 PDT 2004
//    For title use 'x10^' instead of '10e' to designate that exponent.
//
//    Brad Whitlock, Fri Jul 23 18:27:30 PST 2004
//    Added support for using user-defined titles for axes.
//
// ****************************************************************************

void vtkCubeAxesActor::AdjustValues(const double xRange[2],
                                    const double yRange[2],
                                    const double zRange[2])
{
  char xTitle[64];

  int xPow, yPow, zPow;

  if (AutoLabelScaling)
    {
    xPow = this->LabelExponent(xRange[0], xRange[1]);
    yPow = this->LabelExponent(yRange[0], yRange[1]);
    zPow = this->LabelExponent(zRange[0], zRange[1]);
    }
  else
    {
    xPow = UserXPow;
    yPow = UserYPow;
    zPow = UserZPow;
    }

  if (xPow != 0)
    {
    if (!this->MustAdjustXValue || this->LastXPow != xPow)
      {
      this->ForceXLabelReset = true;
      }
    else
      {
      this->ForceXLabelReset = false;
      }
    this->MustAdjustXValue = true;

    if (XUnits == NULL || XUnits[0] == '\0')
      {
      sprintf(xTitle, "%s (x10^%d)", this->XTitle, xPow);
      }
    else
      {
      sprintf(xTitle, "%s (x10^%d %s)", this->XTitle, xPow, XUnits);
      }
    }
  else
    {
    if (this->MustAdjustXValue)
      {
      this->Modified();
      this->ForceXLabelReset = true;
      }
    else
      {
      this->ForceXLabelReset = false;
      }
    this->MustAdjustXValue = false;

    if (XUnits == NULL || XUnits[0] == '\0')
      {
      sprintf(xTitle,"%s",this->XTitle);
      }
    else
      {
      sprintf(xTitle, "%s (%s)", this->XTitle, XUnits);
      }
    }

  char yTitle[64];
  if (yPow != 0)
    {
    if (!this->MustAdjustYValue || this->LastYPow != yPow)
      {
      this->ForceYLabelReset = true;
      }
    else
      {
      this->ForceYLabelReset = false;
      }
    this->MustAdjustYValue = true;
    if (YUnits == NULL || YUnits[0] == '\0')
      {
      sprintf(yTitle, "%s (x10^%d)", this->YTitle, yPow);
      }
    else
      {
      sprintf(yTitle, "%s (x10^%d %s)", this->YTitle, yPow, YUnits);
      }
    }
  else
    {
    if (this->MustAdjustYValue)
      {
      this->Modified();
      this->ForceYLabelReset = true;
      }
    else
      {
      this->ForceYLabelReset = false;
      }
    this->MustAdjustYValue = false;
    if (YUnits == NULL || YUnits[0] == '\0')
      {
      sprintf(yTitle,"%s",this->YTitle);
      }
    else
      {
      sprintf(yTitle, "%s (%s)", this->YTitle, YUnits);
      }
    }

  char zTitle[64];
  if (zPow != 0)
    {
    if (!this->MustAdjustZValue || this->LastZPow != zPow)
      {
      this->ForceZLabelReset = true;
      }
    else
      {
      this->ForceZLabelReset = false;
      }
    this->MustAdjustZValue = true;

    if (ZUnits == NULL || ZUnits[0] == '\0')
      {
      sprintf(zTitle, "%s (x10^%d)", this->ZTitle, zPow);
      }
    else
      {
      sprintf(zTitle, "%s (x10^%d %s)", this->ZTitle, zPow, ZUnits);
      }
    }
  else
    {
    if (this->MustAdjustZValue)
      {
      this->Modified();
      this->ForceZLabelReset = true;
      }
    else
      {
      this->ForceZLabelReset = false;
      }
    this->MustAdjustZValue = false;

    if (ZUnits == NULL || ZUnits[0] == '\0')
      {
      sprintf(zTitle,"%s",this->ZTitle);
      }
    else
      {
      sprintf(zTitle, "%s (%s)", this->ZTitle, ZUnits);
      }
    }

  this->LastXPow = xPow;
  this->LastYPow = yPow;
  this->LastZPow = zPow;

  this->SetActualXLabel(xTitle);
  this->SetActualYLabel(yTitle);
  this->SetActualZLabel(zTitle);
}

// ****************************************************************************
//  Method: vtkCubeAxesActor::AdjustRange
//
//  Purpose:
//    If the range is small, adjust the precision of the values displayed.
//
//  Arguments:
//    bnds    The minimum and maximum values in each coordinate direction
//            (min_x, max_x, min_y, max_y, min_z, max_z).
//
//  Programmer: Hank Childs
//  Creation:   July 11, 2000
//
//  Modifications:
//    Kathleen Bonnell, Wed Oct 31 07:57:49 PST 2001
//    Moved from VisWinAxes3D.
//
//    Kathleen Bonnell, Thu Aug  1 14:05:05 PDT 2002
//    Send lastPos as argument to Digits.
//
//    Kathleen Bonnell, Wed Aug  6 13:59:15 PDT 2003
//    Adjust the range values using LastXPow, LastYPow, LastZPow.
//
// ****************************************************************************

void vtkCubeAxesActor::AdjustRange(const double bnds[6])
{
  double xrange[2], yrange[2], zrange[2];

  xrange[0] = (this->XAxisRange[0] == VTK_DOUBLE_MAX ?
                                  bnds[0] : this->XAxisRange[0]);
  xrange[1] = (this->XAxisRange[1] == VTK_DOUBLE_MAX ?
                                  bnds[1] : this->XAxisRange[1]);
  yrange[0] = (this->YAxisRange[0] == VTK_DOUBLE_MAX ?
                                  bnds[2] : this->YAxisRange[0]);
  yrange[1] = (this->YAxisRange[1] == VTK_DOUBLE_MAX ?
                                  bnds[3] : this->YAxisRange[1]);
  zrange[0] = (this->ZAxisRange[0] == VTK_DOUBLE_MAX ?
                                  bnds[4] : this->ZAxisRange[0]);
  zrange[1] = (this->ZAxisRange[1] == VTK_DOUBLE_MAX ?
                                  bnds[5] : this->ZAxisRange[1]);

  if (this->LastXPow != 0)
    {
    xrange[0] /= pow(10., this->LastXPow);
    xrange[1] /= pow(10., this->LastXPow);
    }
  if (this->LastYPow != 0)
    {
    yrange[0] /= pow(10., this->LastYPow);
    yrange[1] /= pow(10., this->LastYPow);
    }
  if (this->LastZPow != 0)
    {
    zrange[0] /= pow(10., this->LastZPow);
    zrange[1] /= pow(10., this->LastZPow);
    }

  int xAxisDigits = this->Digits(xrange[0], xrange[1]);
  if (xAxisDigits != this->LastXAxisDigits)
    {
    char  format[16];
    sprintf(format, "%%.%df", xAxisDigits);
    this->SetXLabelFormat(format);
    this->LastXAxisDigits = xAxisDigits;
    }

  int yAxisDigits = this->Digits(yrange[0], yrange[1]);
  if (yAxisDigits != this->LastYAxisDigits)
    {
    char  format[16];
    sprintf(format, "%%.%df", yAxisDigits);
    this->SetYLabelFormat(format);
    this->LastYAxisDigits = yAxisDigits;
    }

  int zAxisDigits = this->Digits(zrange[0], zrange[1]);
  if (zAxisDigits != this->LastZAxisDigits)
    {
    char  format[16];
    sprintf(format, "%%.%df", zAxisDigits);
    this->SetZLabelFormat(format);
    this->LastZAxisDigits = zAxisDigits;
    }
}

// ****************************************************************************
//  Method: Digits
//
//  Purpose:
//      Determines the appropriate number of digits for a given range.
//
//  Arguments:
//      min    The minimum value in the range.
//      max    The maximum value in the range.
//
//  Returns:   The appropriate number of digits.
//
//  Programmer: Hank Childs
//  Creation:   July 11, 2000
//
//  Modifications:
//
//    Hank Childs, Tue Sep 18 11:58:33 PDT 2001
//    Cast ipow10 to get rid of compiler warning.
//
//    Kathleen Bonnell, Wed Oct 31 07:57:49 PST 2001
//    Moved from VisWinAxes3D.
//
//    Kathleen Bonnell, Thu Aug  1 13:44:02 PDT 2002
//    Added lastPow argument, it specifies whether or not scientific notation
//    is being used on the labels.
//
//    Kathleen Bonnell, Wed Aug  6 13:59:15 PDT 2003
//    Removed lastPow argment, as the adjustment necessary is now taking
//    place in AdjustRange.
//
// ****************************************************************************

int vtkCubeAxesActor::Digits(double min, double max )
{
  double  range = max - min;
  double  pow10   = log10(range);
  int    ipow10  = static_cast<int>(floor(pow10));
  int    digitsPastDecimal = -ipow10;

  if (digitsPastDecimal < 0)
    {
    //
    // The range is more than 10, but not so big we need scientific
    // notation, we don't need to worry about decimals.
    //
    digitsPastDecimal = 0;
    }
  else
    {
    //
    // We want one more than the range since there is more than one
    // tick per decade.
    //
    digitsPastDecimal++;

    //
    // Anything more than 5 is just noise.  (and probably 5 is noise with
    // doubleing point if the part before the decimal is big).
    //
    if (digitsPastDecimal > 5)
      {
      digitsPastDecimal = 5;
      }
    }

  return digitsPastDecimal;
}

// ****************************************************************************
//  Method: LabelExponent
//
//  Purpose:
//      Determines the proper exponent for the min and max values.
//
//  Arguments:
//      min     The minimum value along a certain axis.
//      max     The maximum value along a certain axis.
//
//  Note:       This code is mostly stolen from old MeshTV code,
//              /meshtvx/toolkit/plotgrid.c, axlab_format.
//
//  Programmer: Hank Childs
//  Creation:   July 11, 2000
//
//  Modifications:
//    Eric Brugger, Tue Sep 18 09:18:17 PDT 2001
//    Change a few static local variables to be non-static to get around a
//    compiler bug with the MIPSpro 7.2.1.3 compiler.
//
//    Hank Childs, Tue Sep 18 11:58:33 PDT 2001
//    Cast return value to get rid of compiler warning.
//
//    Kathleen Bonnell, Wed Oct 31 07:57:49 PST 2001
//    Moved from VisWinAxes3D.
//
//    Kathleen Bonnell, Wed Aug  6 13:59:15 PDT 2003
//    Added test for min==max.
//
// ****************************************************************************

int vtkCubeAxesActor::LabelExponent(double min, double max)
{
  if (min == max)
    {
    return 0;
    }

  //
  // Determine power of 10 to scale axis labels to.
  //
  double range = (fabs(min) > fabs(max) ? fabs(min) : fabs(max));
  double pow10 = log10(range);

  //
  // Cutoffs for using scientific notation.  The following 4 variables
  // should all be static for maximum performance but were made non-static
  // to get around a compiler bug with the MIPSpro 7.2.1.3 compiler.
  //
  double eformat_cut_min = -1.5;
  double eformat_cut_max =  3.0;
  double cut_min = pow(10., eformat_cut_min);
  double cut_max = pow(10., eformat_cut_max);
  double ipow10;
  if (range < cut_min || range > cut_max)
    {
    //
    // We are going to use scientific notation and round the exponents to
    // the nearest multiple of three.
    //
    ipow10 = (floor(floor(pow10)/3.))*3;
    }
  else
    {
    ipow10 = 0;
    }

  return static_cast<int>(ipow10);
}

// *************************************************************************
//  Build the axes. Determine coordinates, position, etc.
//
//  Note:  Bulk of code moved here from RenderOpaqueGeomtry.
//         Early-termination test added.
//
//  Programmer:  Kathleen Bonnell
//  Creation:    November 7, 2001
//
//  Modifications:
//    Kathleen Bonnell, Mon Dec  3 16:49:01 PST 2001
//    Compare vtkTimeStamps correctly.
//
//    Kathleen Bonnell, Fri Jul 25 14:37:32 PDT 2003
//    Added logic to compute and set for each axis the labels and title
//    scale size.
//
//    Kathleen Bonnell, Wed Aug  6 13:59:15 PDT 2003
//    Indivdual axes now have their own ForceLabelReset.
//
// *************************************************************************

void vtkCubeAxesActor::BuildAxes(vtkViewport *viewport)
{
  double bounds[6];
  double pts[8][3];
  int i;

  if ((this->GetMTime() < this->BuildTime.GetMTime()))
    {
    return;
    }

  this->SetNonDependentAttributes();
  // determine the bounds to use (input, prop, or user-defined)
  this->GetBounds(bounds);

  // Build the axes (almost always needed so we don't check mtime)
  // Transform all points into display coordinates (to determine which closest
  // to camera).
  this->TransformBounds(viewport, bounds, pts);

  // Setup the axes for plotting
  double xCoords[4][6], yCoords[4][6], zCoords[4][6];

  // these arrays are accessed by 'location':  mm, mX, XX, or Xm.
  int mm1[4] = { 0, 0, 1, 1 };
  int mm2[4] = { 0, 1, 1, 0 };

  for (i = 0; i < 4; i++)
    {
    this->XAxes[i]->SetAxisPosition(i);
    xCoords[i][0] = bounds[0];
    xCoords[i][3] = bounds[1];
    xCoords[i][1] = xCoords[i][4] = bounds[2+mm1[i]];
    xCoords[i][2] = xCoords[i][5] = bounds[4+mm2[i]];

    this->YAxes[i]->SetAxisPosition(i);
    yCoords[i][0] = yCoords[i][3] = bounds[0+mm1[i]];
    yCoords[i][1] = bounds[2];
    yCoords[i][4] = bounds[3];
    yCoords[i][2] = yCoords[i][5] = bounds[4+mm2[i]];

    this->ZAxes[i]->SetAxisPosition(i);
    zCoords[i][0] = zCoords[i][3] = bounds[0+mm1[i]];
    zCoords[i][1] = zCoords[i][4] = bounds[2+mm2[i]];
    zCoords[i][2] = bounds[4];
    zCoords[i][5] = bounds[5];
    }

  double xRange[2], yRange[2], zRange[2];

  // this method sets the Coords, and offsets if necessary.
  this->AdjustAxes(bounds, xCoords, yCoords, zCoords, xRange, yRange, zRange);

  // adjust for sci. notation if necessary
  // May set a flag for each axis specifying that label values should
  // be scaled, may change title of each axis, may change label format.
  this->AdjustValues(xRange, yRange, zRange);
  this->AdjustRange(this->Bounds);

  // Prepare axes for rendering with user-definable options
  for (i = 0; i < 4; i++)
    {
    this->XAxes[i]->GetPoint1Coordinate()->SetValue(xCoords[i][0],
                                                    xCoords[i][1],
                                                    xCoords[i][2]);
    this->XAxes[i]->GetPoint2Coordinate()->SetValue(xCoords[i][3],
                                                    xCoords[i][4],
                                                    xCoords[i][5]);
    this->YAxes[i]->GetPoint1Coordinate()->SetValue(yCoords[i][0],
                                                    yCoords[i][1],
                                                    yCoords[i][2]);
    this->YAxes[i]->GetPoint2Coordinate()->SetValue(yCoords[i][3],
                                                    yCoords[i][4],
                                                    yCoords[i][5]);
    this->ZAxes[i]->GetPoint1Coordinate()->SetValue(zCoords[i][0],
                                                    zCoords[i][1],
                                                    zCoords[i][2]);
    this->ZAxes[i]->GetPoint2Coordinate()->SetValue(zCoords[i][3],
                                                    zCoords[i][4],
                                                    zCoords[i][5]);

    this->XAxes[i]->SetRange(xRange[0], xRange[1]);
    this->YAxes[i]->SetRange(yRange[0], yRange[1]);
    this->ZAxes[i]->SetRange(zRange[0], zRange[1]);

    this->XAxes[i]->SetTitle(this->ActualXLabel);
    this->YAxes[i]->SetTitle(this->ActualYLabel);
    this->ZAxes[i]->SetTitle(this->ActualZLabel);
    }

  bool ticksRecomputed = this->ComputeTickSize(bounds);

  //
  // Labels are built during ComputeTickSize. if
  // ticks were not recomputed, but we need a label
  // reset, then build the labels here.
  //
  if (!ticksRecomputed)
    {
    if (this->ForceXLabelReset)
      {
      this->BuildLabels(this->XAxes);
      }
    if (this->ForceYLabelReset)
      {
      this->BuildLabels(this->YAxes);
      }
    if (this->ForceZLabelReset)
      {
      this->BuildLabels(this->ZAxes);
      }
    }

  if (ticksRecomputed || this->ForceXLabelReset || this->ForceYLabelReset ||
      this->ForceZLabelReset)
    {
    // labels were re-built, need to recompute the scale.
    double center[3];

    center[0] = (this->Bounds[1] - this->Bounds[0]) * 0.5;
    center[1] = (this->Bounds[3] - this->Bounds[2]) * 0.5;
    center[2] = (this->Bounds[5] - this->Bounds[4]) * 0.5;

    double lenX = this->XAxes[0]->ComputeMaxLabelLength(center);
    double lenY = this->YAxes[0]->ComputeMaxLabelLength(center);
    double lenZ = this->ZAxes[0]->ComputeMaxLabelLength(center);
    double lenTitleX = this->XAxes[0]->ComputeTitleLength(center);
    double lenTitleY = this->YAxes[0]->ComputeTitleLength(center);
    double lenTitleZ = this->ZAxes[0]->ComputeTitleLength(center);
    double maxLabelLength = this->MaxOf(lenX, lenY, lenZ, 0.);
    double maxTitleLength = this->MaxOf(lenTitleX, lenTitleY, lenTitleZ, 0.);
    double bWidth  = this->Bounds[1] - this->Bounds[0];
    double bHeight = this->Bounds[3] - this->Bounds[2];

    double bLength = sqrt(bWidth*bWidth + bHeight*bHeight);

    double target = bLength *0.04;
    double labelscale = 1.;
    if (maxLabelLength != 0.)
      {
      labelscale = target / maxLabelLength;
      }
    target = bLength *0.10;
    double titlescale = 1.;
    if (maxTitleLength != 0.)
      {
      titlescale = target / maxTitleLength;
      }

    //
    // Allow a bit bigger title if we have units, otherwise
    // the title may be too small to read.
    //
    if (XUnits != NULL && XUnits[0] != '\0')
      {
      titlescale *= 2;
      }

    for (i = 0; i < 4; i++)
      {
      this->XAxes[i]->SetLabelScale(labelscale);
      this->YAxes[i]->SetLabelScale(labelscale);
      this->ZAxes[i]->SetLabelScale(labelscale);
      this->XAxes[i]->SetTitleScale(titlescale);
      this->YAxes[i]->SetTitleScale(titlescale);
      this->ZAxes[i]->SetTitleScale(titlescale);
      }
    }
  this->RenderSomething = 1;
  this->BuildTime.Modified();
  this->LastFlyMode = this->FlyMode;
}

// *************************************************************************
//  Sends attributes to each vtkAxisActor.  Only sets those that are
//  not dependent upon viewport changes, and thus do not need to be set
//  very often.
//
//  Programmer:  Kathleen Bonnell
//  Creation:    November 7, 2001
//
//  Modifications:
//    Kathleen Bonnell, Thu Oct  3 14:33:15 PDT 2002
//    Disable lighting for the axes by setting the ambient coefficient to 1
//    and the diffuse coeeficient to 0.
// *************************************************************************

void vtkCubeAxesActor::SetNonDependentAttributes()
{
  vtkProperty *prop = this->GetProperty();
  prop->SetAmbient(1.0);
  prop->SetDiffuse(0.0);
  for (int i = 0; i < 4; i++)
    {
    this->XAxes[i]->SetCamera(this->Camera);
    this->XAxes[i]->SetProperty(prop);
    this->XAxes[i]->SetTickLocation(this->TickLocation);
    this->XAxes[i]->SetDrawGridlines(this->DrawXGridlines);
    this->XAxes[i]->SetBounds(this->Bounds);
    this->XAxes[i]->AxisVisibilityOn();
    this->XAxes[i]->SetLabelVisibility(this->XAxisLabelVisibility);
    this->XAxes[i]->SetTitleVisibility(this->XAxisLabelVisibility);
    this->XAxes[i]->SetTickVisibility(this->XAxisTickVisibility);
    this->XAxes[i]->SetMinorTicksVisible(this->XAxisMinorTickVisibility);

    this->YAxes[i]->SetCamera(this->Camera);
    this->YAxes[i]->SetProperty(prop);
    this->YAxes[i]->SetTickLocation(this->TickLocation);
    this->YAxes[i]->SetDrawGridlines(this->DrawYGridlines);
    this->YAxes[i]->SetBounds(this->Bounds);
    this->YAxes[i]->AxisVisibilityOn();
    this->YAxes[i]->SetLabelVisibility(this->YAxisLabelVisibility);
    this->YAxes[i]->SetTitleVisibility(this->YAxisLabelVisibility);
    this->YAxes[i]->SetTickVisibility(this->YAxisTickVisibility);
    this->YAxes[i]->SetMinorTicksVisible(this->YAxisMinorTickVisibility);

    this->ZAxes[i]->SetCamera(this->Camera);
    this->ZAxes[i]->SetProperty(prop);
    this->ZAxes[i]->SetTickLocation(this->TickLocation);
    this->ZAxes[i]->SetDrawGridlines(this->DrawZGridlines);
    this->ZAxes[i]->SetBounds(this->Bounds);
    this->ZAxes[i]->AxisVisibilityOn();
    this->ZAxes[i]->SetLabelVisibility(this->ZAxisLabelVisibility);
    this->ZAxes[i]->SetTitleVisibility(this->ZAxisLabelVisibility);
    this->ZAxes[i]->SetTickVisibility(this->ZAxisTickVisibility);
    this->ZAxes[i]->SetMinorTicksVisible(this->ZAxisMinorTickVisibility);
    }
}

// Static variable describes locations in cube, relative to the type
// of axis:  mm for an X-axis means the x-edge at min-y and min-z.
// mX for a Y-axis means the y-edge at min-x and max-z, and so on.

enum {mm = 0, mX, XX, Xm };
//
// For CLOSEST_TRIAD, and FURTHEST_TRIAD, this variable determines
// which locations in the cube each 'Major' axis should take.
//
static int vtkCubeAxesActorTriads[8][3] = {
  {mm,mm,mm}, {mm,Xm,Xm}, {Xm,mm,mX}, {Xm,Xm,XX},
  {mX,mX,mm}, {mX,XX,Xm}, {XX,mX,mX}, {XX,XX,XX}};
static int vtkCubeAxesActorConn[8][3] = {{1,2,4}, {0,3,5}, {3,0,6}, {2,1,7},
                                         {5,6,0}, {4,7,1}, {7,4,2}, {6,5,3}};

// *************************************************************************
// Determine which of the axes in each coordinate direction actually should
// be rendered.  For STATIC FlyMode, all axes are rendered.  For other
// FlyModes, either 1 or 2 per coordinate direction are rendered.
//
// Programmer:  Kathleen Bonnell
// Creation:    November 7, 2001
//
// Modifications:
//   Kathleen Bonnell, Thu Jul 18 10:33:07 PDT 2002
//   Ensure that primary axes visibility flags are set properly, and
//   that secondary axes visibility flags are turned off.
// *************************************************************************
void vtkCubeAxesActor::DetermineRenderAxes(vtkViewport *viewport)
{
  double bounds[6], slope = 0.0, minSlope, num, den;
  double pts[8][3], d2, d2Min, min, max;
  int i = 0, idx = 0;
  int xIdx = 0, yIdx = 0, zIdx = 0, zIdx2 = 0;
  int xAxes = 0, yAxes = 0, zAxes = 0, xloc = 0, yloc = 0, zloc = 0;

  if (this->FlyMode == VTK_FLY_STATIC_EDGES)
    {
    for (i = 0; i < 4; i++)
      {
      this->RenderAxesX[i] = i;
      this->RenderAxesY[i] = i;
      this->RenderAxesZ[i] = i;
      }
    this->NumberOfAxesX = this->NumberOfAxesY = this->NumberOfAxesZ = 4;
    return;
    }
  if (this->FlyMode == VTK_FLY_STATIC_TRIAD)
    {
    this->RenderAxesX[0] = 0;
    this->RenderAxesY[0] = 0;
    this->RenderAxesZ[0] = 0;
    if (this->DrawXGridlines)
      {
      this->RenderAxesX[1] = 2;
      this->NumberOfAxesX = 2;
      this->XAxes[RenderAxesX[1]]->SetTickVisibility(0);
      this->XAxes[RenderAxesX[1]]->SetLabelVisibility(0);
      this->XAxes[RenderAxesX[1]]->SetTitleVisibility(0);
      this->XAxes[RenderAxesX[1]]->SetMinorTicksVisible(0);
      }
    else
      {
      this->NumberOfAxesX = 1;
      }
    if (this->DrawYGridlines)
      {
      this->RenderAxesY[1] = 2;
      this->NumberOfAxesY = 2;
      this->YAxes[RenderAxesY[1]]->SetTickVisibility(0);
      this->YAxes[RenderAxesY[1]]->SetLabelVisibility(0);
      this->YAxes[RenderAxesY[1]]->SetTitleVisibility(0);
      this->YAxes[RenderAxesY[1]]->SetMinorTicksVisible(0);
      }
    else
      {
      this->NumberOfAxesY = 1;
      }
    if (this->DrawZGridlines)
      {
      this->RenderAxesZ[1] = 2;
      this->NumberOfAxesZ = 2;
      this->ZAxes[RenderAxesZ[1]]->SetTickVisibility(0);
      this->ZAxes[RenderAxesZ[1]]->SetLabelVisibility(0);
      this->ZAxes[RenderAxesZ[1]]->SetTitleVisibility(0);
      this->ZAxes[RenderAxesZ[1]]->SetMinorTicksVisible(0);
      }
    else
      {
      this->NumberOfAxesZ = 1;
      }
    return;
    }

  // determine the bounds to use (input, prop, or user-defined)
  this->GetBounds(bounds);
  this->TransformBounds(viewport, bounds, pts);

  // Take into account the inertia. Process only so often.
  if (this->RenderCount++ == 0 || !(this->RenderCount % this->Inertia))
    {
    if (this->FlyMode == VTK_FLY_CLOSEST_TRIAD)
      {
      // Loop over points and find the closest point to the camera
      min = VTK_LARGE_FLOAT;
      for (i=0; i < 8; i++)
        {
        if (pts[i][2] < min)
          {
          idx = i;
          min = pts[i][2];
          }
        }
      xloc = vtkCubeAxesActorTriads[idx][0];
      yloc = vtkCubeAxesActorTriads[idx][1];
      zloc = vtkCubeAxesActorTriads[idx][2];

      } // closest-triad
    else if (this->FlyMode == VTK_FLY_FURTHEST_TRIAD)
      {
      // Loop over points and find the furthest point from the camera
      max = -VTK_LARGE_FLOAT;
      for (i=0; i < 8; i++)
        {
        if (pts[i][2] > max)
          {
          idx = i;
          max = pts[i][2];
          }
        }
      xloc = vtkCubeAxesActorTriads[idx][0];
      yloc = vtkCubeAxesActorTriads[idx][1];
      zloc = vtkCubeAxesActorTriads[idx][2];

      } // furthest-triad
    else
      {
      double e1[3], e2[3], e3[3];

      // Find distance to origin
      d2Min = VTK_LARGE_FLOAT;
      for (i=0; i < 8; i++)
        {
        d2 = pts[i][0]*pts[i][0] + pts[i][1]*pts[i][1];
        if (d2 < d2Min)
          {
          d2Min = d2;
          idx = i;
          }
        }

      // find minimum slope point connected to closest point and on
      // right side (in projected coordinates). This is the first edge.
      minSlope = VTK_LARGE_FLOAT;
      for (xIdx=0, i=0; i<3; i++)
        {
        num = (pts[vtkCubeAxesActorConn[idx][i]][1] - pts[idx][1]);
        den = (pts[vtkCubeAxesActorConn[idx][i]][0] - pts[idx][0]);
        if (den != 0.0)
          {
          slope = num / den;
          }
        if (slope < minSlope && den > 0)
          {
          xIdx = vtkCubeAxesActorConn[idx][i];
          yIdx = vtkCubeAxesActorConn[idx][(i+1)%3];
          zIdx = vtkCubeAxesActorConn[idx][(i+2)%3];
          xAxes = i;
          minSlope = slope;
          }
        }

      // find edge (connected to closest point) on opposite side
      for ( i=0; i<3; i++)
        {
        e1[i] = (pts[xIdx][i] - pts[idx][i]);
        e2[i] = (pts[yIdx][i] - pts[idx][i]);
        e3[i] = (pts[zIdx][i] - pts[idx][i]);
        }
      vtkMath::Normalize(e1);
      vtkMath::Normalize(e2);
      vtkMath::Normalize(e3);

      if (vtkMath::Dot(e1,e2) < vtkMath::Dot(e1,e3))
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
      if (pts[vtkCubeAxesActorConn[xIdx][zAxes]][2] <
          pts[vtkCubeAxesActorConn[yIdx][zAxes]][2])
        {
        zIdx = xIdx;
        zIdx2 = vtkCubeAxesActorConn[xIdx][zAxes];
        }
      else
        {
        zIdx = yIdx;
        zIdx2 = vtkCubeAxesActorConn[yIdx][zAxes];
        }

      int mini = (idx < xIdx ? idx : xIdx);
      switch (xAxes)
        {
        case 0:
          xloc = vtkCubeAxesActorTriads[mini][0];
          break;
        case 1:
          yloc = vtkCubeAxesActorTriads[mini][1];
          break;
        case 2:
          zloc = vtkCubeAxesActorTriads[mini][2];
          break;
        }
      mini = (idx < yIdx ? idx : yIdx);
      switch (yAxes)
        {
        case 0:
          xloc = vtkCubeAxesActorTriads[mini][0];
          break;
        case 1:
          yloc =vtkCubeAxesActorTriads[mini][1];
          break;
        case 2:
          zloc = vtkCubeAxesActorTriads[mini][2];
          break;
        }
      mini = (zIdx < zIdx2 ? zIdx : zIdx2);
      switch (zAxes)
        {
        case 0:
          xloc = vtkCubeAxesActorTriads[mini][0];
          break;
        case 1:
          yloc = vtkCubeAxesActorTriads[mini][1];
          break;
        case 2:
          zloc = vtkCubeAxesActorTriads[mini][2];
          break;
        }

      }//else boundary edges fly mode

    this->InertiaLocs[0] = xloc;
    this->InertiaLocs[1] = yloc;
    this->InertiaLocs[2] = zloc;
    } //inertia
  else
    {
    // don't change anything, use locations from last render
    xloc = this->InertiaLocs[0];
    yloc = this->InertiaLocs[1];
    zloc = this->InertiaLocs[2];
    }

  this->RenderAxesX[0] = xloc % 4;
  if (this->DrawXGridlines)
    {
    this->RenderAxesX[1] = (xloc + 2) % 4;
    this->NumberOfAxesX = 2;
    this->XAxes[RenderAxesX[1]]->SetTickVisibility(0);
    this->XAxes[RenderAxesX[1]]->SetLabelVisibility(0);
    this->XAxes[RenderAxesX[1]]->SetTitleVisibility(0);
    this->XAxes[RenderAxesX[1]]->SetMinorTicksVisible(0);
    }
  else
    {
    this->NumberOfAxesX = 1;
    }

  this->RenderAxesY[0] = yloc % 4;
  if (this->DrawYGridlines)
    {
    this->RenderAxesY[1] = (yloc + 2) % 4;
    this->NumberOfAxesY = 2;
    this->YAxes[RenderAxesY[1]]->SetTickVisibility(0);
    this->YAxes[RenderAxesY[1]]->SetLabelVisibility(0);
    this->YAxes[RenderAxesY[1]]->SetTitleVisibility(0);
    this->YAxes[RenderAxesY[1]]->SetMinorTicksVisible(0);
    }
  else
    {
    this->NumberOfAxesY = 1;
    }

  this->RenderAxesZ[0] = zloc % 4;
  if (this->DrawZGridlines)
    {
    this->RenderAxesZ[1] = (zloc + 2) % 4;
    this->NumberOfAxesZ = 2;
    this->ZAxes[RenderAxesZ[1]]->SetTickVisibility(0);
    this->ZAxes[RenderAxesZ[1]]->SetLabelVisibility(0);
    this->ZAxes[RenderAxesZ[1]]->SetTitleVisibility(0);
    this->ZAxes[RenderAxesZ[1]]->SetMinorTicksVisible(0);
    }
  else
    {
    this->NumberOfAxesZ = 1;
    }
  //
  //  Make sure that the primary axis visibility flags are set correctly.
  //
  this->XAxes[RenderAxesX[0]]->SetLabelVisibility(this->XAxisLabelVisibility);
  this->XAxes[RenderAxesX[0]]->SetTitleVisibility(this->XAxisLabelVisibility);
  this->XAxes[RenderAxesX[0]]->SetTickVisibility(this->XAxisTickVisibility);
  this->XAxes[RenderAxesX[0]]->SetMinorTicksVisible(
    this->XAxisMinorTickVisibility);

  this->YAxes[RenderAxesY[0]]->SetLabelVisibility(this->YAxisLabelVisibility);
  this->YAxes[RenderAxesY[0]]->SetTitleVisibility(this->YAxisLabelVisibility);
  this->YAxes[RenderAxesY[0]]->SetTickVisibility(this->YAxisTickVisibility);
  this->YAxes[RenderAxesY[0]]->SetMinorTicksVisible(
    this->YAxisMinorTickVisibility);

  this->ZAxes[RenderAxesZ[0]]->SetLabelVisibility(this->ZAxisLabelVisibility);
  this->ZAxes[RenderAxesZ[0]]->SetTitleVisibility(this->ZAxisLabelVisibility);
  this->ZAxes[RenderAxesZ[0]]->SetTickVisibility(this->ZAxisTickVisibility);
  this->ZAxes[RenderAxesZ[0]]->SetMinorTicksVisible(
    this->ZAxisMinorTickVisibility);
}

double vtkCubeAxesActor::MaxOf(double a, double b)
{
  return (a > b ? a : b);
}

double vtkCubeAxesActor::MaxOf(double a, double b, double c, double d)
{
  return this->MaxOf(this->MaxOf(a, b), this->MaxOf(c, d));
}

inline double vtkCubeAxesActor::FFix(double value)
{
  int ivalue = static_cast<int>(value);
  return ivalue;
}

inline double vtkCubeAxesActor::FSign(double value, double sign)
{
  value = fabs(value);
  if (sign < 0.)
    {
    value *= -1.;
    }
  return value;
}

// *******************************************************************
// Method: vtkCubeAxesActor::AdjustTicksComputeRange
//
// Purpose: Sets private members controlling the number and position
//          of ticks.
//
// Arguments:
//   inRange   The range for this axis.
//
// Note:    The bulk of this method was taken from vtkHankAxisActor.C
//          The original method was reduced to serve the purposes
//          of this class.
//
// Programmer: Kathleen Bonnell
// Creation:   29 August, 2001
//
// Modifications:
//   Kathleen Bonnell, Fri Jul 25 14:37:32 PDT 2003
//   Moved from vtkAxisActor. Added calls to set inividual axis'
//   MajorStart, MinorStart, deltaMajor, deltaMinor.
//
// *******************************************************************

void vtkCubeAxesActor::AdjustTicksComputeRange(vtkAxisActor *axes[4],
    double boundsMin, double boundsMax)
{
  double sortedRange[2], range;
  double fxt, fnt, frac;
  double div, major, minor;
  double majorStart, minorStart;
  int numTicks;
  double *inRange = axes[0]->GetRange();

  sortedRange[0] = inRange[0] < inRange[1] ? inRange[0] : inRange[1];
  sortedRange[1] = inRange[0] > inRange[1] ? inRange[0] : inRange[1];

  range = sortedRange[1] - sortedRange[0];

  // Find the integral points.
  double pow10 = log10(range);

  // Build in numerical tolerance
  if (pow10 != 0.)
    {
    double eps = 10.0e-10;
    pow10 = this->FSign((fabs(pow10) + eps), pow10);
    }

  // FFix move you in the wrong direction if pow10 is negative.
  if (pow10 < 0.)
    {
    pow10 = pow10 - 1.;
    }

  fxt = pow(10., this->FFix(pow10));

  // Find the number of integral points in the interval.
  fnt  = range/fxt;
  fnt  = this->FFix(fnt);
  frac = fnt;
  numTicks = frac <= 0.5 ? static_cast<int>(this->FFix(fnt)) : static_cast<int>(this->FFix(fnt) + 1);

  div = 1.;
  if (numTicks < 5)
    {
    div = 2.;
    }
  if (numTicks <= 2)
    {
    div = 5.;
    }

  // If there aren't enough major tick points in this decade, use the next
  // decade.
  major = fxt;
  if (div != 1.)
    {
    major /= div;
    }
  minor = (fxt/div) / 10.;

  // Figure out the first major and minor tick locations, relative to the
  // start of the axis.
  if (sortedRange[0] <= 0.)
    {
    majorStart = major*(this->FFix(sortedRange[0]*(1./major)) + 0.);
    minorStart = minor*(this->FFix(sortedRange[0]*(1./minor)) + 0.);
    }
  else
    {
    majorStart = major*(this->FFix(sortedRange[0]*(1./major)) + 1.);
    minorStart = minor*(this->FFix(sortedRange[0]*(1./minor)) + 1.);
    }

  for (int i = 0; i < 4; i++)
    {
    axes[i]->SetMinorRangeStart(minorStart);
    axes[i]->SetMajorRangeStart(majorStart);
    axes[i]->SetDeltaRangeMinor(minor);
    axes[i]->SetDeltaRangeMajor(major);
    }

  double t;
  t = (minorStart - sortedRange[0])/range;
  minorStart = t * boundsMax + (1-t) * boundsMin;
  t = (majorStart - sortedRange[0])/range;
  majorStart = t * boundsMax + (1-t) * boundsMin;
  const double scale = (boundsMax - boundsMin) / range;
  minor *= scale;
  major *= scale;

  for (int i = 0; i < 4; i++)
    {
    axes[i]->SetMinorStart(minorStart);
    axes[i]->SetMajorStart(majorStart);

    axes[i]->SetDeltaMinor(minor);
    axes[i]->SetDeltaMajor(major);
    }
}

// ****************************************************************
//  Determine what the labels should be and set them in each axis.
//
//  Modification:
//    Kathleen Bonnell, Wed Aug  6 13:59:15 PDT 2003
//    Each axis type now has it's own 'mustAdjustValue' and 'pow'.
//
//    Kathleen Bonnell, Tue Jul 20 14:29:10 PDT 2004
//    Ensure that '-0.0' is never used as a label.
//
//    Eric Brugger, Mon Jul 26 16:09:26 PDT 2004
//    Correct a bug with a misplaced closing parenthesis.
//
// ****************************************************************

void vtkCubeAxesActor::BuildLabels(vtkAxisActor *axes[4])
{
  char label[64];
  int i, labelCount = 0;
  double deltaMajor = axes[0]->GetDeltaMajor();
  const double *p2  = axes[0]->GetPoint2Coordinate()->GetValue();
  double *range     = axes[0]->GetRange();
  double lastVal = 0, val = axes[0]->GetMajorStart();
  double extents = range[1] - range[0];
  bool mustAdjustValue = 0;
  int lastPow = 0;

  vtkStringArray *labels = vtkStringArray::New();
  const char *format = "%s";
  switch (axes[0]->GetAxisType())
    {
    case VTK_AXIS_TYPE_X:
      lastVal = p2[0];
      format = this->XLabelFormat;
      mustAdjustValue = this->MustAdjustXValue;
      lastPow = this->LastXPow;
      break;
    case VTK_AXIS_TYPE_Y:
      lastVal = p2[1];
      format = this->YLabelFormat;
      mustAdjustValue = this->MustAdjustYValue;
      lastPow = this->LastYPow;
      break;
    case VTK_AXIS_TYPE_Z:
      lastVal = p2[2];
      format = this->ZLabelFormat;
      mustAdjustValue = this->MustAdjustZValue;
      lastPow = this->LastZPow;
      break;
    }

  // figure out how many labels we need:
  while (val <= lastVal && labelCount < VTK_MAX_LABELS)
    {
    labelCount++;
    val += deltaMajor;
    }

  labels->SetNumberOfValues(labelCount);

  val = axes[0]->GetMajorRangeStart();
  deltaMajor = axes[0]->GetDeltaRangeMajor();

  double scaleFactor = 1.;
  if (lastPow != 0)
    {
    scaleFactor = 1.0/pow(10., lastPow);
    }

  for (i = 0; i < labelCount; i++)
    {
    if (fabs(val) < 0.01 && extents > 1)
      {
      // We just happened to fall at something near zero and the range is
      // large, so set it to zero to avoid ugliness.
      val = 0.;
      }
    if (mustAdjustValue)
      {
      sprintf(label, format, val*scaleFactor);
      }
    else
      {
      sprintf(label, format, val);
      }
    if (fabs(val) < 0.01)
      {
      //
      // Ensure that -0.0 is never a label
      // The maximum number of digits that we allow past the decimal is 5.
      //
      if (strcmp(label, "-0") == 0)
        {
        sprintf(label, "0");
        }
      else if (strcmp(label, "-0.0") == 0)
        {
        sprintf(label, "0.0");
        }
      else if (strcmp(label, "-0.00") == 0)
        {
        sprintf(label, "0.00");
        }
      else if (strcmp(label, "-0.000") == 0)
        {
        sprintf(label, "0.000");
        }
      else if (strcmp(label, "-0.0000") == 0)
        {
        sprintf(label, "0.0000");
        }
      else if (strcmp(label, "-0.00000") == 0)
        {
        sprintf(label, "0.00000");
        }
      }
    labels->SetValue(i, label);
    val += deltaMajor;
    }
  for (i = 0; i < 4; i++)
    {
    axes[i]->SetLabels(labels);
    }
  labels->Delete();
}

// ****************************************************************************
//  Set automatic label scaling mode, set exponents for each axis type.
//
// ****************************************************************************
void vtkCubeAxesActor::SetLabelScaling(bool autoscale, int upowX, int upowY,
                                       int upowZ)
{
  if (autoscale != this->AutoLabelScaling || upowX != this->UserXPow ||
      upowY != this->UserYPow || upowZ != this->UserZPow)
    {
    this->AutoLabelScaling = autoscale;
    this->UserXPow = upowX;
    this->UserYPow = upowY;
    this->UserZPow = upowZ;
    this->Modified();
    }
}
