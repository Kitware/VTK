/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAxisActor.cxx
  Thanks:    Kathleen Bonnell, B Division, Lawrence Livermore Nat'l Laboratory

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#include "vtkAxisActor.h"

#include "vtkCamera.h"
#include "vtkCellArray.h"
#include "vtkCoordinate.h"
#include "vtkFollower.h"
#include "vtkObjectFactory.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkStringArray.h"
#include "vtkVectorText.h"
#include "vtkViewport.h"

// ****************************************************************
// Modifications:
//   Kathleen Bonnell, Wed Mar  6 13:48:48 PST 2002 
//   Replace 'New' method with macro to match VTK 4.0 API.
// ****************************************************************

vtkStandardNewMacro(vtkAxisActor);
vtkCxxSetObjectMacro(vtkAxisActor, Camera, vtkCamera); 

// ****************************************************************
// Instantiate this object.
//
// Modifications:
//   Kathleen Bonnell, Wed Oct 31 07:57:49 PST 2001
//   Initialize new members mustAdjustValue and valueScaleFactor.
//
//   Kathleen Bonnell, Wed Nov  7 16:19:16 PST 2001 
//   No longer allocate large amounts of memory for labels, instead
//   allocate dynamically.  Initialize new members: 
//   LastLabelStart; LastAxisPosition; LastTickLocation; LastTickVisibility; 
//   LastDrawGridlines; LastMinorTicksVisible; LastRange; MinorTickPts; 
//   MajorTickPts; GridlinePts.
//
//   Kathleen Bonnell, Thu May 16 10:13:56 PDT 2002 
//   Initialize new member AxisHasZeroLength. 
//
//   Kathleen Bonnell, Thu Aug  1 13:44:02 PDT 2002 
//   Initialize new member ForceLabelReset. 
//
//   Kathleen Bonnell, Fri Jul 25 14:37:32 PDT 2003 
//   Removed mustAdjustValue, valueScaleFator, ForceLabelReset.
//
//   Kathleen Bonnell, Thu Apr 29 17:02:10 PDT 2004
//   Initialize MinorStart, MajorStart, DeltaMinor, DeltaMajor.
//
// ****************************************************************

vtkAxisActor::vtkAxisActor()
{
  this->Point1Coordinate = vtkCoordinate::New();
  this->Point1Coordinate->SetCoordinateSystemToWorld();
  this->Point1Coordinate->SetValue(0.0, 0.0, 0.0);

  this->Point2Coordinate = vtkCoordinate::New();
  this->Point2Coordinate->SetCoordinateSystemToWorld();
  this->Point2Coordinate->SetValue(0.75, 0.0, 0.0);

  this->Camera = NULL;
  this->Title = NULL;
  this->MinorTicksVisible = 1;
  this->MajorTickSize = 1.0;
  this->MinorTickSize = 0.5;
  this->TickLocation = VTK_TICKS_INSIDE; 
  this->Range[0] = 0.0;
  this->Range[1] = 1.0;
  
  this->Bounds[0] = this->Bounds[2] = this->Bounds[4] = -1;
  this->Bounds[1] = this->Bounds[3] = this->Bounds[5] = 1;

  this->LabelFormat = new char[8]; 
  sprintf(this->LabelFormat, "%s", "%-#6.3g");

  this->TitleVector = vtkVectorText::New();
  this->TitleMapper = vtkPolyDataMapper::New();
  this->TitleMapper->SetInput(this->TitleVector->GetOutput());
  this->TitleActor = vtkFollower::New();
  this->TitleActor->SetMapper(this->TitleMapper);
  
  // to avoid deleting/rebuilding create once up front
  this->NumberOfLabelsBuilt = 0;
  this->LabelVectors = NULL; 
  this->LabelMappers = NULL; 
  this->LabelActors = NULL; 

  this->Axis = vtkPolyData::New();
  this->AxisMapper = vtkPolyDataMapper::New();
  this->AxisMapper->SetInput(this->Axis);
  this->AxisActor = vtkActor::New();
  this->AxisActor->SetMapper(this->AxisMapper);
  
  this->AxisVisibility = 1;
  this->TickVisibility = 1;
  this->LabelVisibility = 1;
  this->TitleVisibility = 1;
  
  this->DrawGridlines = 0;
  this->GridlineXLength = 1.;  
  this->GridlineYLength = 1.;  
  this->GridlineZLength = 1.;  

  this->AxisType = VTK_AXIS_TYPE_X; 
  //
  // AxisPosition denotes which of the four possibilities in relation
  // to the bounding box.  An x-Type axis with min min, means the x-axis
  // at minimum y and minimum z values of the bbox.
  //
  this->AxisPosition = VTK_AXIS_POS_MINMIN;

  this->LastLabelStart = 100000;

  this->LastAxisPosition = -1;
  this->LastTickLocation = -1; 
  this->LastTickVisibility = -1; 
  this->LastDrawGridlines = -1; 
  this->LastMinorTicksVisible = -1; 
  this->LastRange[0] = -1.0;
  this->LastRange[1] = -1.0;

  this->MinorTickPts = vtkPoints::New();
  this->MajorTickPts = vtkPoints::New();
  this->GridlinePts  = vtkPoints::New();

  this->AxisHasZeroLength = false;

  this->MinorStart = 0.;
  this->MajorStart = 0.;
  this->DeltaMinor = 1.;
  this->DeltaMajor = 1.;
}

// ****************************************************************
// Modifications:
//   Kathleen Bonnell, Wed Mar  6 13:48:48 PST 2002 
//   Added call to set camera to null.
// ****************************************************************

vtkAxisActor::~vtkAxisActor()
{
  this->SetCamera(NULL);

  if (this->Point1Coordinate)
    {
    this->Point1Coordinate->Delete();
    this->Point1Coordinate = NULL;
    }

  if (this->Point2Coordinate)
    {
    this->Point2Coordinate->Delete();
    this->Point2Coordinate = NULL;
    }

  if (this->LabelFormat) 
    {
    delete [] this->LabelFormat;
    this->LabelFormat = NULL;
    }

  if (this->TitleVector)
    {
    this->TitleVector->Delete();
    this->TitleVector = NULL;
    }
  if (this->TitleMapper)
    {
    this->TitleMapper->Delete();
    this->TitleMapper = NULL;
    }
  if (this->TitleActor)
    {
    this->TitleActor->Delete();
    this->TitleActor = NULL;
    }

  if (this->Title)
    {
    delete [] this->Title;
    this->Title = NULL;
    }

  if (this->LabelMappers != NULL)
    {
    for (int i=0; i < this->NumberOfLabelsBuilt; i++)
      {
      this->LabelVectors[i]->Delete();
      this->LabelMappers[i]->Delete();
      this->LabelActors[i]->Delete();
      }
    this->NumberOfLabelsBuilt = 0;
    delete [] this->LabelVectors;
    delete [] this->LabelMappers;
    delete [] this->LabelActors;
    this->LabelVectors = NULL;
    this->LabelMappers = NULL;
    this->LabelActors = NULL;
    }

  if (this->Axis)
    {
    this->Axis->Delete();
    this->Axis = NULL;
    }
  if (this->AxisMapper)
    {
    this->AxisMapper->Delete();
    this->AxisMapper = NULL;
    }
  if (this->AxisActor)
    {
    this->AxisActor->Delete();
    this->AxisActor = NULL;
    }

  if (this->MinorTickPts)
    {
    this->MinorTickPts ->Delete();
    this->MinorTickPts = NULL; 
    }
  if (this->MajorTickPts)
    {
    this->MajorTickPts->Delete();
    this->MajorTickPts = NULL; 
    }
  if (this->GridlinePts)
    {
    this->GridlinePts->Delete();
    this->GridlinePts = NULL; 
    }
}

// Release any graphics resources that are being consumed by this actor.
// The parameter window could be used to determine which graphic
// resources to release.
void vtkAxisActor::ReleaseGraphicsResources(vtkWindow *win)
{
  this->TitleActor->ReleaseGraphicsResources(win);
  for (int i=0; i < this->NumberOfLabelsBuilt; i++)
    {
    this->LabelActors[i]->ReleaseGraphicsResources(win);
    }
  this->AxisActor->ReleaseGraphicsResources(win);
}

// ****************************************************************
//
// Modifications:
//   Kathleen Bonnell, Wed Oct 31 07:57:49 PST 2001
//   Copy over mustAdjustValue and valueScaleFactor.
//
//   Kathleen Bonnell, Wed Mar  6 13:48:48 PST 2002
//   Call superclass's method in new VTK 4.0 way.
//
//   Kathleen Bonnell, Fri Jul 25 14:37:32 PDT 2003 
//   Removed mustAdjustValue, valueScaleFator.
//
// ****************************************************************

void vtkAxisActor::ShallowCopy(vtkProp *prop)
{
  vtkAxisActor *a = vtkAxisActor::SafeDownCast(prop);
  if (a != NULL)
    {
    this->SetPoint1(a->GetPoint1());
    this->SetPoint2(a->GetPoint2());
    this->SetCamera(a->GetCamera());
    this->SetRange(a->GetRange());
    this->SetLabelFormat(a->GetLabelFormat());
    this->SetTitle(a->GetTitle());
    this->SetAxisVisibility(a->GetAxisVisibility());
    this->SetTickVisibility(a->GetTickVisibility());
    this->SetLabelVisibility(a->GetLabelVisibility());
    this->SetTitleVisibility(a->GetTitleVisibility());
    }

  // Now do superclass
  this->Superclass::ShallowCopy(prop);
}

// ****************************************************************
// Build the axis, ticks, title, and labels and render.
//
// Modifications:
//   Kathleen Bonnell, Thu May 16 10:13:56 PDT 2002 
//   Don't render a zero-length axis. 
//
//   Kathleen Bonnell, Fri Jul 25 14:37:32 PDT 2003 
//   Added bool argument to BuildAxis. 
//
// ****************************************************************

int vtkAxisActor::RenderOpaqueGeometry(vtkViewport *viewport)
{
  int i, renderedSomething=0;

  this->BuildAxis(viewport, false);

  // Everything is built, just have to render

  if (!this->AxisHasZeroLength)
    {
    if (this->Title != NULL && this->Title[0] != 0 && this->TitleVisibility)
      {
      renderedSomething += this->TitleActor->RenderOpaqueGeometry(viewport);
      }

    if (this->AxisVisibility || this->TickVisibility)
      {
      renderedSomething += this->AxisActor->RenderOpaqueGeometry(viewport);
      }

    if (this->LabelVisibility)
      {
      for (i=0; i<this->NumberOfLabelsBuilt; i++)
        {
        renderedSomething +=
          this->LabelActors[i]->RenderOpaqueGeometry(viewport);
        }
      }
    }

  return renderedSomething;
}

// **************************************************************************
// Perform some initialization, determine which Axis type we are
// and call the appropriate build method.
//
// Modifications:
//   Kathleen Bonnell, Wed Nov  7 17:45:20 PST 2001
//   Added logic to only rebuild sub-parts if necessary.
//
//   Kathleen Bonnell, Fri Nov 30 17:02:41 PST 2001 
//   Moved setting values for LastRange to end of method, so they
//   can be used in comparisons elsewhere.
//
//   Kathleen Bonnell, Mon Dec  3 16:49:01 PST 2001
//   Compare vtkTimeStamps correctly.
//
//   Kathleen Bonnell, Thu May 16 10:13:56 PDT 2002 
//   Test for zero length axis. 
//
//   Kathleen Bonnell,  Fri Jul 25 14:37:32 PDT 2003
//   Added bool argument that will allow all axis components to be built
//   if set to true.  Removed call to AdjustTicksComputeRange (handled by
//   vtkCubeAxesActor.  Remvoed call to Build?TypeAxis, added calls
//   to BuildLabels, SetAxisPointsAndLines and BuildTitle, (which used to
//   be handled in Build?TypeAxis). .
//    
// **************************************************************************

void vtkAxisActor::BuildAxis(vtkViewport *viewport, bool force)
{
  // We'll do our computation in world coordinates. First determine the
  // location of the endpoints.
  double *x, p1[3], p2[3];
  x = this->Point1Coordinate->GetValue();
  p1[0] = x[0]; p1[1] = x[1]; p1[2] = x[2];
  x = this->Point2Coordinate->GetValue();
  p2[0] = x[0]; p2[1] = x[1]; p2[2] = x[2];

  //
  //  Test for axis of zero length.
  //
  if (p1[0] == p2[0] && p1[1] == p2[1] && p1[2] == p2[2])
    {
    vtkDebugMacro(<<"Axis has zero length, not building.");
    this->AxisHasZeroLength = true;
    return;
    }
  this->AxisHasZeroLength = false;

  if (!force && this->GetMTime() < this->BuildTime.GetMTime() &&
      viewport->GetMTime() < this->BuildTime.GetMTime())
    {
    return; //already built
    }

  vtkDebugMacro(<<"Rebuilding axis");

  if (force || this->GetProperty()->GetMTime() > this->BuildTime.GetMTime())
    {
    this->AxisActor->SetProperty(this->GetProperty());
    this->TitleActor->SetProperty(this->GetProperty());
    }

  //
  // Generate the axis and tick marks.
  //
  bool ticksRebuilt;
  if (this->AxisType == VTK_AXIS_TYPE_X)
    {
    ticksRebuilt = this->BuildTickPointsForXType(p1, p2, force);
    }
  else if (this->AxisType == VTK_AXIS_TYPE_Y)
    {
    ticksRebuilt = this->BuildTickPointsForYType(p1, p2, force);
    }
  else
    {
    ticksRebuilt = this->BuildTickPointsForZType(p1, p2, force);
    }

  bool tickVisChanged = this->TickVisibilityChanged();

  if (force || ticksRebuilt || tickVisChanged)
   {
   this->SetAxisPointsAndLines();
   }

  this->BuildLabels(viewport, force);

  if (this->Title != NULL && this->Title[0] != 0) 
    {
    this->BuildTitle(force);
    }

  this->LastAxisPosition = this->AxisPosition;
  this->LastTickLocation = this->TickLocation;

  this->LastRange[0] = this->Range[0];
  this->LastRange[1] = this->Range[1];
  this->BuildTime.Modified();
}

// ****************************************************************
//
//  Set label values and properties. 
//
// Modifications:
//   Kathleen Bonnell, Wed Oct 31 07:57:49 PST 2001
//   Use valueScaleFactor to scale value if necessary. 
//
//   Kathleen Bonnell, Wed Nov  7 17:45:20 PST 2001 
//   Added code for early termination.  Added call to SetNumberOfLabels
//   for dynamic memory allocation. Number of labels limited to 200.
//
//   Kathleen Bonnell, Fri Nov 30 17:02:41 PST 2001
//   Added test for modified range to determine if labels really need to 
//   be built.
//
//   Kathleen Bonnell, Thu May 16 10:13:56 PDT 2002 
//   Use defined constant to limit number of labels.   
//
//   Kathleen Bonnell, Fri Jul 25 14:37:32 PDT 2003 
//   Remvoed determination of label text, added call to 
//   SetLabelPositions.
//
// ****************************************************************

void
vtkAxisActor::BuildLabels(vtkViewport *viewport, bool force)
{
  if (!force && !this->LabelVisibility)
    {
    return;
    }
 
  for (int i = 0; i < this->NumberOfLabelsBuilt; i++)
    {
    this->LabelActors[i]->SetCamera(this->Camera);
    this->LabelActors[i]->SetProperty(this->GetProperty());
    }

  if (force || this->BuildTime.GetMTime() <  this->BoundsTime.GetMTime() || 
      this->AxisPosition != this->LastAxisPosition ||
      this->LastRange[0] != this->Range[0] ||
      this->LastRange[1] != this->Range[1])
    {
    this->SetLabelPositions(viewport, force);
    }
}

int vtkAxisActorMultiplierTable1[4] = { -1, -1, 1,  1};
int vtkAxisActorMultiplierTable2[4] = { -1,  1, 1, -1};

// *******************************************************************
// Determine and set scale factor and position for labels.
//
// Modifications:
//   Kathleen Bonnell, Fri Nov 30 17:02:41 PST 2001
//   Reset labels scale to 1. before testing length, in order to
//   ensure proper scaling.  Use Bounds[1] and Bounds[0] for bWidth
//   instead of Bounds[5] and Bounds[4].
//
//   Kathleen Bonnell, Tue Dec  4 09:55:03 PST 2001 
//   Ensure that scale does not go below MinScale. 
//
//   Kathleen Bonnell, Tue Apr  9 14:41:08 PDT 2002  
//   Removed MinScale as it allowed axes with very small ranges
//   to have labels scaled too large for the dataset. 
//
//   Kathleen Bonnell, Fri Jul 18 09:09:31 PDT 2003 
//   Renamed to SetLabelPosition.  Removed calculation of label
//   scale factor, added check for no labels to early return test.
//
//   Eric Brugger, Tue Jul 29 14:42:44 PDT 2003
//   Corrected the test that causes the routine to exit early when
//   no work needs to be done.
//
// *******************************************************************

void vtkAxisActor::SetLabelPositions(vtkViewport *viewport, bool force) 
{
  if (!force && (!this->LabelVisibility || this->NumberOfLabelsBuilt == 0))
    {
    return;
    }

  double bounds[6], center[3], tick[3], pos[3];
  int i = 0;
  int xmult = 0;
  int ymult = 0;

  switch (this->AxisType)
    {
    case VTK_AXIS_TYPE_X : 
      xmult = 0; 
      ymult = vtkAxisActorMultiplierTable1[this->AxisPosition]; 
      break;
    case VTK_AXIS_TYPE_Y : 
      xmult = vtkAxisActorMultiplierTable1[this->AxisPosition];
      ymult = 0; 
      break;
    case VTK_AXIS_TYPE_Z :
      xmult = vtkAxisActorMultiplierTable1[this->AxisPosition];
      ymult = vtkAxisActorMultiplierTable2[this->AxisPosition]; 
      break;
    }

  int ptIdx;
  //
  // xadjust & yadjust are used for positioning the label correctly
  // depending upon the 'orientation' of the axis as determined
  // by its position in view space (via transformed bounds). 
  //
  double displayBounds[6] = { 0., 0., 0., 0., 0., 0.};
  this->TransformBounds(viewport, displayBounds);
  double xadjust = (displayBounds[0] > displayBounds[1] ? -1 : 1);
  double yadjust = (displayBounds[2] > displayBounds[3] ? -1 : 1);

  for (i=0; i < this->NumberOfLabelsBuilt && 
    i < this->MajorTickPts->GetNumberOfPoints(); i++)
    {
    ptIdx = 4*i + 1;
    this->MajorTickPts->GetPoint(ptIdx, tick);

    this->LabelActors[i]->GetBounds(bounds);

    double halfWidth  = (bounds[1] - bounds[0]) * 0.5;
    double halfHeight = (bounds[3] - bounds[2]) * 0.5;

    center[0] = tick[0] + xmult * (halfWidth  + this->MinorTickSize);
    center[1] = tick[1] + ymult * (halfHeight + this->MinorTickSize); 
    pos[0] = (center[0] - xadjust *halfWidth);
    pos[1] = (center[1] - yadjust *halfHeight);
    pos[2] = tick[2]; 
    this->LabelActors[i]->SetPosition(pos[0], pos[1], pos[2]);
    }
}

// **********************************************************************
//  Determines scale and position for the Title.  Currently,
//  title can only be centered with respect to its axis.
//  
//  Modifications:
//    Kathleen Bonnell, Wed Nov  7 17:45:20 PST 2001
//    Added logic for early-termination.
//
//    Kathleen Bonnell, Mon Dec  3 16:49:01 PST 2001
//    Test for modified bounds before early termination, use
//    MinScale, modified target so title size is a bit more reasonable. 
//
//    Kathleen Bonnell, Tue Apr  9 14:41:08 PDT 2002  
//    Removed MinScale as it allowed axes with very small ranges
//    to have labels scaled too large for the dataset. 
//
//    Kathleen Bonnell, Fri Jul 25 14:37:32 PDT 2003 
//    Added bool argument that allow the build to be forced, even if
//    the title won't be visible. 
//   
//    Kathleen Bonnell, Tue Aug 31 16:17:43 PDT 2004 
//    Added TitleTime test so that the title can be rebuilt when its
//    text has changed.
//
//    Hank Childs, Sun May 13 11:06:12 PDT 2007
//    Fix bug with positioning of titles (the titles were being placed
//    far away from the bounding box in some cases).
//
// **********************************************************************

void vtkAxisActor::BuildTitle(bool force)
{
  if (!force && !this->TitleVisibility)
    {
    return;
    }
  double labBounds[6], titleBounds[6], center[3], pos[3];
  double labHeight, maxHeight = 0, labWidth, maxWidth = 0;
  double halfTitleWidth, halfTitleHeight;

  double *p1 = this->Point1Coordinate->GetValue();
  double *p2 = this->Point2Coordinate->GetValue();
  int xmult = 0;
  int ymult = 0;

  if (!force && this->LabelBuildTime.GetMTime() < this->BuildTime.GetMTime() &&
      this->BoundsTime.GetMTime() < this->BuildTime.GetMTime() &&
      this->AxisPosition == this->LastAxisPosition &&
      this->TitleTextTime.GetMTime() < this->BuildTime.GetMTime())
    {
    return;
    }

  switch (this->AxisType)
    {
    case VTK_AXIS_TYPE_X : 
      xmult = 0; 
      ymult = vtkAxisActorMultiplierTable1[this->AxisPosition]; 
      break;
    case VTK_AXIS_TYPE_Y :
      xmult = vtkAxisActorMultiplierTable1[this->AxisPosition];
      ymult = 0; 
      break;
    case VTK_AXIS_TYPE_Z : 
      xmult = vtkAxisActorMultiplierTable1[this->AxisPosition];
      ymult = vtkAxisActorMultiplierTable2[this->AxisPosition]; 
      break;
    }
  //
  //  Title should be in relation to labels (if any)
  //  so find out information about them.
  //
  for (int i = 0; i < this->NumberOfLabelsBuilt; i++)
    {
    this->LabelActors[i]->GetBounds(labBounds);
    labWidth = labBounds[1] - labBounds[0]; 
    maxWidth = (labWidth > maxWidth ? labWidth : maxWidth); 
    labHeight = labBounds[3] - labBounds[2]; 
    maxHeight = (labHeight > maxHeight ? labHeight : maxHeight); 
    }
  this->TitleVector->SetText(this->Title);
  this->TitleActor->SetCamera(this->Camera);
  this->TitleActor->SetPosition(p2[0], p2[1], p2[2]);
  this->TitleActor->GetBounds(titleBounds);
  halfTitleWidth  = (titleBounds[1] - titleBounds[0]) * 0.5; 
  halfTitleHeight = (titleBounds[3] - titleBounds[2]) * 0.5;

  center[0] = p1[0] + (p2[0] - p1[0]) / 2.0;
  center[1] = p1[1] + (p2[1] - p1[1]) / 2.0;
  center[2] = p1[2] + (p2[2] - p1[2]) / 2.0;

  center[0] += xmult * (halfTitleWidth + maxWidth); 
  center[1] += ymult * (halfTitleHeight + 2*maxHeight);

  pos[0] = center[0] - xmult*halfTitleWidth;
  pos[1] = center[1] - ymult*halfTitleHeight;
  pos[2] = center[2]; 
  this->TitleActor->SetPosition(pos[0], pos[1], pos[2]);
}

//
//  Transform the bounding box to display coordinates.  Used
//  in determining orientation of the axis.
//
void vtkAxisActor::TransformBounds(vtkViewport *viewport, double bnds[6])
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

inline double ffix(double value)
{
  int ivalue = static_cast<int>(value);
  return static_cast<double>(ivalue);
}

inline double fsign(double value, double sign)
{
  value = fabs(value);
  if (sign < 0.)
    {
    value *= -1.;
    }
  return value;
}

// ****************************************************************
// Modifications:
//   Kathleen Bonnell, Wed Mar  6 13:48:48 PST 2002
//   Call superclass's method in new VTK 4.0 way.
// ****************************************************************

void vtkAxisActor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Title: " << (this->Title ? this->Title : "(none)") << "\n";
  os << indent << "Number Of Labels Built: " 
     << this->NumberOfLabelsBuilt << "\n";
  os << indent << "Range: (" << this->Range[0] 
     << ", " << this->Range[1] << ")\n";

  os << indent << "Label Format: " << this->LabelFormat << "\n";
  
  os << indent << "Axis Visibility: " 
     << (this->AxisVisibility ? "On\n" : "Off\n");
  
  os << indent << "Tick Visibility: " 
     << (this->TickVisibility ? "On\n" : "Off\n");
  
  os << indent << "Label Visibility: " 
     << (this->LabelVisibility ? "On\n" : "Off\n");
  
  os << indent << "Title Visibility: " 
     << (this->TitleVisibility ? "On\n" : "Off\n");
  
  os << indent << "Point1 Coordinate: " << this->Point1Coordinate << "\n";
  this->Point1Coordinate->PrintSelf(os, indent.GetNextIndent());
  
  os << indent << "Point2 Coordinate: " << this->Point2Coordinate << "\n";
  this->Point2Coordinate->PrintSelf(os, indent.GetNextIndent());

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
      ;
    }

  os << indent << "DeltaMajor: " << this->DeltaMajor << endl;
  os << indent << "DeltaMinor: " << this->DeltaMinor << endl;

  os << indent << "MinorTicksVisible: " << this->MinorTicksVisible << endl;

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

  os << indent << "MajorStart: " << this->MajorStart << endl;
  os << indent << "MinorStart: " << this->MinorStart << endl;

  os << indent << "AxisPosition: " << this->AxisPosition << endl;

  os << indent << "GridlineXLength: " << this->GridlineXLength << endl;
  os << indent << "GridlineYLength: " << this->GridlineYLength << endl;
  os << indent << "GridlineZLength: " << this->GridlineZLength << endl;

  os << indent << "TickLocation: " << this->TickLocation << endl;
}

// **************************************************************************
// Sets text string for label vectors.  Allocates memory if necessary. 
//
// Programmer:  Kathleen Bonnell
// Creation:    July 18, 2003
// **************************************************************************
void vtkAxisActor::SetLabels(vtkStringArray *labels)
{
  //
  // If the number of labels has changed, re-allocate the correct
  // amount of memory.
  //
  int i, numLabels = labels->GetNumberOfValues();
  if (this->NumberOfLabelsBuilt != numLabels)
    {
    if (this->LabelMappers != NULL)
      {
      for (i = 0; i < this->NumberOfLabelsBuilt; i++)
        {
        this->LabelVectors[i]->Delete();
        this->LabelMappers[i]->Delete();
        this->LabelActors[i]->Delete();
        }
      delete [] this->LabelVectors;
      delete [] this->LabelMappers;
      delete [] this->LabelActors;
      }

    this->LabelVectors = new vtkVectorText * [numLabels];
    this->LabelMappers = new vtkPolyDataMapper * [numLabels];
    this->LabelActors = new vtkFollower * [numLabels];

    for (i = 0; i < numLabels; i++)
      {
      this->LabelVectors[i] = vtkVectorText::New();
      this->LabelMappers[i] = vtkPolyDataMapper::New();
      this->LabelMappers[i]->SetInput(this->LabelVectors[i]->GetOutput());
      this->LabelActors[i] = vtkFollower::New();
      this->LabelActors[i]->SetMapper(this->LabelMappers[i]);
      }
    }

  //
  // Set the label vector text. 
  //
  for (i = 0; i < numLabels; i++)
    {
    this->LabelVectors[i]->SetText(labels->GetValue(i).c_str());
    }
  this->NumberOfLabelsBuilt = numLabels;
  this->LabelBuildTime.Modified();
}

// **************************************************************************
// Creates points for ticks (minor, major, gridlines) in correct position 
// for X-type axsis.
//
// Programmer:  Kathleen Bonnell
// Creation:    November 7, 2001
//
// Modifications:
//   Kathleen Bonnell, Mon Dec  3 16:49:01 PST 2001
//   Compare vtkTimeStamps correctly.
//
//   Kathleen Bonnell, Thu May 16 10:13:56 PDT 2002 
//   Use defined constant VTK_MAX_TICKS to prevent infinite loops. 
// 
//   Kathleen Bonnell, Fri Jul 25 14:37:32 PDT 2003 
//   Allow a forced build, despite previous build time.
//
// **************************************************************************
bool vtkAxisActor::BuildTickPointsForXType(double p1[3], double p2[3],
                                           bool force)
{
  if (!force && (this->AxisPosition == this->LastAxisPosition) &&
      (this->TickLocation == this->LastTickLocation ) &&
      (this->BoundsTime.GetMTime() < this->BuildTime.GetMTime()))
    {
    return false;
    }

  double xPoint1[3], xPoint2[3], yPoint[3], zPoint[3], x;
  int numTicks;

  this->MinorTickPts->Reset();
  this->MajorTickPts->Reset();
  this->GridlinePts->Reset();

  //
  // Ymult & Zmult control adjustments to tick position based
  // upon "where" this axis is located in relation to the underlying
  // assumed bounding box.
  //
  int yMult = vtkAxisActorMultiplierTable1[this->AxisPosition];
  int zMult = vtkAxisActorMultiplierTable2[this->AxisPosition];

  //
  // Build Minor Ticks
  //
  if (this->TickLocation == VTK_TICKS_OUTSIDE) 
    {
    xPoint1[1] = xPoint2[1] = zPoint[1] = p1[1]; 
    xPoint1[2] = xPoint2[2] = yPoint[2] = p1[2]; 
    yPoint[1] = p1[1] + yMult * this->MinorTickSize; 
    zPoint[2] = p1[2] + zMult * this->MinorTickSize; 
    }
  else if (this->TickLocation == VTK_TICKS_INSIDE) 
    {
    yPoint[1] = xPoint2[1] = zPoint[1] = p1[1]; 
    xPoint1[2] = yPoint[2] = zPoint[2] = p1[2]; 
    xPoint1[1] = p1[1] - yMult * this->MinorTickSize; 
    xPoint2[2] = p1[2] - zMult * this->MinorTickSize; 
    }
  else // both sides
    {
    xPoint2[1] = zPoint[1] = p1[1]; 
    xPoint1[2] = yPoint[2] = p1[2]; 
    yPoint[1] = p1[1] + yMult * this->MinorTickSize; 
    zPoint[2] = p1[2] + zMult * this->MinorTickSize; 
    xPoint1[1] = p1[1] - yMult * this->MinorTickSize; 
    xPoint2[2] = p1[2] - zMult * this->MinorTickSize; 
    }
  x = this->MinorStart;
  numTicks = 0;
  while (x <= p2[0] && numTicks < VTK_MAX_TICKS)
    {
    xPoint1[0] = xPoint2[0] = yPoint[0] = zPoint[0] = x;
    // xy-portion
    this->MinorTickPts->InsertNextPoint(xPoint1);
    this->MinorTickPts->InsertNextPoint(yPoint);
    // xz-portion
    this->MinorTickPts->InsertNextPoint(xPoint2);
    this->MinorTickPts->InsertNextPoint(zPoint);
    x+= this->DeltaMinor;
    numTicks++;
    }

  //
  // Gridline points 
  //
  yPoint[1] = xPoint2[1] = zPoint[1] = p1[1];
  xPoint1[1] = p1[1] - yMult * this->GridlineYLength; 
  xPoint1[2] = yPoint[2] = zPoint[2] = p1[2]; 
  xPoint2[2] = p1[2] - zMult * this->GridlineZLength; 

  x = this->MajorStart;
  numTicks = 0;
  while (x <= p2[0] && numTicks < VTK_MAX_TICKS)
    {
    xPoint1[0] = xPoint2[0] = yPoint[0] = zPoint[0] = x;
    // xy-portion
    this->GridlinePts->InsertNextPoint(xPoint1);
    this->GridlinePts->InsertNextPoint(yPoint);
    // xz-portion
    this->GridlinePts->InsertNextPoint(xPoint2);
    this->GridlinePts->InsertNextPoint(zPoint);
    x += this->DeltaMajor;
    numTicks++;
    }

  //
  // Major ticks
  //
  if (this->TickLocation == VTK_TICKS_OUTSIDE) 
    {
    xPoint1[1] = xPoint2[1] = zPoint[1] = p1[1]; 
    xPoint1[2] = xPoint2[2] = yPoint[2] = p1[2]; 
    yPoint[1] = p1[1] + yMult * this->MajorTickSize; 
    zPoint[2] = p1[2] + zMult * this->MajorTickSize; 
    }
  else if (this->TickLocation == VTK_TICKS_INSIDE) 
    {
    yPoint[1] = xPoint2[1] = zPoint[1] = p1[1]; 
    xPoint1[2] = yPoint[2] = zPoint[2] = p1[2]; 
    xPoint1[1] = p1[1] - yMult * this->MajorTickSize; 
    xPoint2[2] = p1[2] - zMult * this->MajorTickSize; 
    }
  else // both sides
    {
    xPoint2[1] = zPoint[1] = p1[1]; 
    xPoint1[2] = yPoint[2] = p1[2]; 
    yPoint[1] = p1[1] + yMult * this->MajorTickSize; 
    zPoint[2] = p1[2] + zMult * this->MajorTickSize; 
    xPoint1[1] = p1[1] - yMult * this->MajorTickSize; 
    xPoint2[2] = p1[2] - zMult * this->MajorTickSize; 
    }
  x = this->MajorStart;
  numTicks = 0;
  while (x <= p2[0] && numTicks < VTK_MAX_TICKS)
    {
    xPoint1[0] = xPoint2[0] = yPoint[0] = zPoint[0] = x;
    // xy-portion
    this->MajorTickPts->InsertNextPoint(xPoint1);
    this->MajorTickPts->InsertNextPoint(yPoint);
    // xz-portion
    this->MajorTickPts->InsertNextPoint(xPoint2);
    this->MajorTickPts->InsertNextPoint(zPoint);
    x += this->DeltaMajor;
    numTicks++;
    }

  return true;
}

// **************************************************************************
// Creates points for ticks (minor, major, gridlines) in correct position 
// for Y-type axis.
//
// Programmer:  Kathleen Bonnell
// Creation:    November 7, 2001
//
// Modifications:
//   Kathleen Bonnell, Mon Dec  3 16:49:01 PST 2001
//   Compare vtkTimeStamps correctly.
//
//   Kathleen Bonnell, Thu May 16 10:13:56 PDT 2002 
//   Use defined constant VTK_MAX_TICKS to prevent infinite loops. 
//
//   Kathleen Bonnell, Fri Jul 25 14:37:32 PDT 2003 
//   Allow a forced build, despite previous build time.
//
// **************************************************************************
bool vtkAxisActor::BuildTickPointsForYType(double p1[3], double p2[3],
                                           bool force)
{
  if (!force && (this->AxisPosition  == this->LastAxisPosition) &&
      (this->TickLocation == this->LastTickLocation) &&
      (this->BoundsTime.GetMTime() < this->BuildTime.GetMTime()))
    {
    return false;
    }

  double yPoint1[3], yPoint2[3], xPoint[3], zPoint[3], y;
  int numTicks;

  this->MinorTickPts->Reset();
  this->MajorTickPts->Reset();
  this->GridlinePts->Reset();
  //
  // xMult & zMult control adjustments to tick position based
  // upon "where" this axis is located in relation to the underlying
  // assumed bounding box.
  //

  int xMult = vtkAxisActorMultiplierTable1[this->AxisPosition];
  int zMult = vtkAxisActorMultiplierTable2[this->AxisPosition];

  //
  // The ordering of the tick endpoints is important because
  // label position is defined by them.
  //

  //
  // minor ticks
  //
  if (this->TickLocation == VTK_TICKS_INSIDE)      
    {
    yPoint1[2] = xPoint[2] = zPoint[2] = p1[2]; 
    yPoint2[0] = xPoint[0] = zPoint[0] = p1[0];
    yPoint1[0] = p1[0] - xMult * this->MinorTickSize;
    yPoint2[2] = p1[2] - zMult * this->MinorTickSize; 
    }
  else if (this->TickLocation == VTK_TICKS_OUTSIDE) 
    {
    yPoint1[0] = yPoint2[0] = zPoint[0] = p1[0];
    yPoint1[2] = yPoint2[2] = xPoint[2] = p1[2];
    xPoint[0] = p1[0] + xMult * this->MinorTickSize;
    zPoint[2] = p1[2] + zMult * this->MinorTickSize;
    }
  else                              // both sides
    {
    yPoint1[2] = xPoint[2] = p1[2]; 
    yPoint2[0] = zPoint[0] = p1[0];
    yPoint1[0] = p1[0] - xMult * this->MinorTickSize;
    yPoint2[2] = p1[2] + zMult * this->MinorTickSize;
    xPoint[0]  = p1[0] + xMult * this->MinorTickSize;
    zPoint[2]  = p1[2] - zMult * this->MinorTickSize;
    }
  y = this->MinorStart;
  numTicks = 0;
  while (y < p2[1] && numTicks < VTK_MAX_TICKS)
    {
    yPoint1[1] = xPoint[1] = yPoint2[1] = zPoint[1] = y;
    // yx portion
    this->MinorTickPts->InsertNextPoint(yPoint1);
    this->MinorTickPts->InsertNextPoint(xPoint);
    // yz portion
    this->MinorTickPts->InsertNextPoint(yPoint2);
    this->MinorTickPts->InsertNextPoint(zPoint);
    y += this->DeltaMinor;
    numTicks++;
    }

  //
  // gridlines
  // 
  yPoint1[0] = p1[0] - xMult * this->GridlineXLength;
  yPoint2[2] = p1[2] - zMult * this->GridlineZLength;
  yPoint2[0] = xPoint[0] = zPoint[0]  = p1[0];
  yPoint1[2] = xPoint[2] = zPoint[2]  = p1[2];

  y = this->MajorStart;
  numTicks = 0;
  while (y <= p2[1] && numTicks < VTK_MAX_TICKS)
    {
    yPoint1[1] = xPoint[1] = yPoint2[1] = zPoint[1] = y;
    // yx portion
    this->GridlinePts->InsertNextPoint(yPoint1);
    this->GridlinePts->InsertNextPoint(xPoint);
    // yz portion
    this->GridlinePts->InsertNextPoint(yPoint2);
    this->GridlinePts->InsertNextPoint(zPoint);
    y += this->DeltaMajor;
    numTicks++;
    }

  //
  // major ticks
  //
  if (this->TickLocation == VTK_TICKS_INSIDE)
    {
    yPoint1[2] = xPoint[2] = zPoint[2] = p1[2]; 
    yPoint2[0] = xPoint[0] = zPoint[0] = p1[0];
    yPoint1[0] = p1[0] - xMult * this->MajorTickSize;
    yPoint2[2] = p1[2] - zMult * this->MajorTickSize; 
    }
  else if (this->TickLocation == VTK_TICKS_OUTSIDE) 
    {
    yPoint1[0] = yPoint2[0] = zPoint[0] = p1[0];
    yPoint1[2] = yPoint2[2] = xPoint[2] = p1[2];
    xPoint[0] = p1[0] + xMult * this->MajorTickSize;
    zPoint[2] = p1[2] + zMult * this->MajorTickSize;
    }
  else                              // both sides
    {
    yPoint1[2] = xPoint[2] = p1[2]; 
    yPoint2[0] = zPoint[0] = p1[0];
    yPoint1[0] = p1[0] - xMult * this->MajorTickSize;
    yPoint2[2] = p1[2] + zMult * this->MajorTickSize;
    xPoint[0]  = p1[0] + xMult * this->MajorTickSize;
    zPoint[2]  = p1[2] - zMult * this->MajorTickSize;
    }
  y = this->MajorStart;
  numTicks = 0;
  while (y <= p2[1] && numTicks < VTK_MAX_TICKS)
    {
    yPoint1[1] = xPoint[1] = yPoint2[1] = zPoint[1] = y;
    // yx portion
    this->MajorTickPts->InsertNextPoint(yPoint1);
    this->MajorTickPts->InsertNextPoint(xPoint);
    // yz portion
    this->MajorTickPts->InsertNextPoint(yPoint2);
    this->MajorTickPts->InsertNextPoint(zPoint);
    y += this->DeltaMajor;
    numTicks++;
    }
  return true;
}

// **************************************************************************
// Creates points for ticks (minor, major, gridlines) in correct position 
// for Z-type axis.
//
// Programmer:  Kathleen Bonnell
// Creation:    November 7, 2001
//
// Modifications:
//   Kathleen Bonnell, Mon Dec  3 16:49:01 PST 2001
//   Compare vtkTimeStamps correctly.
//
//   Kathleen Bonnell, Thu May 16 10:13:56 PDT 2002 
//   Use defined constant VTK_MAX_TICKS to prevent infinite loops. 
//
//   Kathleen Bonnell, Fri Jul 25 14:37:32 PDT 2003 
//   Allow a forced build, despite previous build time.
//
// **************************************************************************

bool vtkAxisActor::BuildTickPointsForZType(double p1[3], double p2[3],
                                           bool force)
{
  if (!force && (this->AxisPosition  == this->LastAxisPosition) &&
      (this->TickLocation == this->LastTickLocation) &&
      (this->BoundsTime.GetMTime() < this->BuildTime.GetMTime()))
    {
    return false;
    }

  this->MinorTickPts->Reset();
  this->MajorTickPts->Reset();
  this->GridlinePts->Reset();

  //
  // xMult & yMult control adjustments to tick position based
  // upon "where" this axis is located in relation to the underlying
  // assumed bounding box.
  //
  int xMult = vtkAxisActorMultiplierTable1[this->AxisPosition];
  int yMult = vtkAxisActorMultiplierTable2[this->AxisPosition];

  double zPoint1[3], zPoint2[3], xPoint[3], yPoint[3], z;
  int numTicks;

  //
  // The ordering of the tick endpoints is important because
  // label position is defined by them.
  //

  //
  // minor ticks
  //
  if (this->TickLocation == VTK_TICKS_INSIDE)      
    {
    zPoint1[0] = p1[0] - xMult * this->MinorTickSize;
    zPoint2[1] = p1[1] - yMult * this->MinorTickSize;
    zPoint2[0] = xPoint[0] = yPoint[0]  = p1[0];
    zPoint1[1] = xPoint[1] = yPoint[1]  = p1[1];
    }
  else if (this->TickLocation == VTK_TICKS_OUTSIDE) 
    {
    xPoint[0]  = p1[0] + xMult * this->MinorTickSize;
    yPoint[1]  = p1[1] +yMult * this->MinorTickSize;
    zPoint1[0] = zPoint2[0] = yPoint[0] = p1[0];
    zPoint1[1] = zPoint2[1] = xPoint[1] = p1[1];
    }
  else                              // both sides
    {
    zPoint1[0] = p1[0] - xMult * this->MinorTickSize;
    xPoint[0]  = p1[0] + xMult * this->MinorTickSize;
    zPoint2[1] = p1[1] - yMult * this->MinorTickSize;
    yPoint[1]  = p1[1] + yMult * this->MinorTickSize;
    zPoint1[1] = xPoint[1] = p1[1];
    zPoint2[0] = yPoint[0] = p1[0];
    }
  z = this->MinorStart;
  numTicks = 0;
  while (z < p2[2] && numTicks < VTK_MAX_TICKS)
    {
    zPoint1[2] = zPoint2[2] = xPoint[2] = yPoint[2] = z;
    // zx-portion
    this->MinorTickPts->InsertNextPoint(zPoint1);
    this->MinorTickPts->InsertNextPoint(xPoint);
    // zy-portion
    this->MinorTickPts->InsertNextPoint(zPoint2);
    this->MinorTickPts->InsertNextPoint(yPoint);
    z += this->DeltaMinor;
    numTicks++;
    }

  //
  // gridlines
  // 
  zPoint1[0] = p1[0] - xMult * this->GridlineXLength;
  zPoint2[1] = p1[1] - yMult * this->GridlineYLength;
  zPoint1[1] = xPoint[1] = yPoint[1] = p1[1];
  zPoint2[0] = xPoint[0] = yPoint[0] = p1[0];

  z = this->MajorStart;
  numTicks = 0;
  while (z <= p2[2] && numTicks < VTK_MAX_TICKS)
    {
    zPoint1[2] = zPoint2[2] = xPoint[2] = yPoint[2] = z;
    // zx-portion
    this->GridlinePts->InsertNextPoint(zPoint1);
    this->GridlinePts->InsertNextPoint(xPoint);
    // zy-portion
    this->GridlinePts->InsertNextPoint(zPoint2);
    this->GridlinePts->InsertNextPoint(yPoint);
    z += this->DeltaMajor;
    numTicks++;
    }

  //
  // major ticks
  //

  if (this->TickLocation == VTK_TICKS_INSIDE)   
    {
    zPoint1[0] = p1[0] - xMult * this->MajorTickSize;
    zPoint2[1] = p1[1] - yMult * this->MajorTickSize;
    zPoint2[0] = xPoint[0] = yPoint[0]  = p1[0];
    zPoint1[1] = xPoint[1] = yPoint[1]  = p1[1];
    }
  else if (this->TickLocation == VTK_TICKS_OUTSIDE) 
    {
    xPoint[0]  = p1[0] + xMult * this->MajorTickSize;
    yPoint[2]  = p1[1] + yMult * this->MajorTickSize;
    zPoint1[0] = zPoint2[0] = yPoint[0] = p1[0];
    zPoint1[1] = zPoint2[1] = xPoint[1] = p1[1];
    }
  else                              // both sides
    {
    zPoint1[0] = p1[0] - xMult * this->MajorTickSize;
    xPoint[0]  = p1[0] + xMult * this->MajorTickSize;
    zPoint2[1] = p1[1] - yMult * this->MajorTickSize;
    yPoint[1]  = p1[1] + yMult * this->MajorTickSize;
    zPoint1[1] = xPoint[1] = p1[1];
    zPoint2[0] = yPoint[0] = p1[0];
    }
  z = this->MajorStart;
  numTicks = 0;
  while (z <= p2[2] && numTicks < VTK_MAX_TICKS)
    {
    zPoint1[2] = zPoint2[2] = xPoint[2] = yPoint[2] = z;
    // zx-portion
    this->MajorTickPts->InsertNextPoint(zPoint1);
    this->MajorTickPts->InsertNextPoint(xPoint);
    // zy-portion
    this->MajorTickPts->InsertNextPoint(zPoint2);
    this->MajorTickPts->InsertNextPoint(yPoint);
    z += this->DeltaMajor;
    numTicks++;
    }
  return true;
}

// **************************************************************************
// Creates Poly data (lines) from tickmarks (minor/major), gridlines, and axis.
//
// Programmer:  Kathleen Bonnell
// Creation:    November 7, 2001
//
// Modifications:
//   Kathleen Bonnell, Thu Nov 15 13:20:44 PST 2001
//   Make ptIds of type vtkIdType to match VTK 4.0 API.
//
//   Kathleen Bonnell, Thu Jul 18 13:24:03 PDT 2002 
//   Allow gridlines to be drawn when enabled, even if ticks are disabled. 
//
//   Kathleen Bonnell, Fri Jul 25 15:10:24 PDT 2003
//   Removed arguments.
//
// **************************************************************************
void vtkAxisActor::SetAxisPointsAndLines()
{
  vtkPoints *pts = vtkPoints::New();
  vtkCellArray *lines = vtkCellArray::New();
  this->Axis->SetPoints(pts);
  this->Axis->SetLines(lines);
  pts->Delete();
  lines->Delete();
  int i, numPts, numLines; 
  vtkIdType ptIds[2];

  if (this->TickVisibility)
    {
    if (this->MinorTicksVisible)
      {
      numPts = this->MinorTickPts->GetNumberOfPoints();
      for (i = 0; i < numPts; i++)
        {
        pts->InsertNextPoint(this->MinorTickPts->GetPoint(i));
        }
      }

    if (this->DrawGridlines)
      {
      numPts = this->GridlinePts->GetNumberOfPoints();
      for (i = 0; i < numPts; i++)
        {
        pts->InsertNextPoint(this->GridlinePts->GetPoint(i));
        }
      }
    else  // major tick points
      {
      numPts = this->MajorTickPts->GetNumberOfPoints();
      for (i = 0; i < numPts; i++)
        {
        pts->InsertNextPoint(this->MajorTickPts->GetPoint(i));
        }
      }
    }
  else if (this->DrawGridlines) // grids are enabled but ticks are off
    {
    numPts = this->GridlinePts->GetNumberOfPoints();
    for (i = 0; i < numPts; i++)
      {
      pts->InsertNextPoint(this->GridlinePts->GetPoint(i));
      }
    }

  // create lines
  numLines = pts->GetNumberOfPoints() / 2;
  for (i=0; i < numLines; i++)
    {
    ptIds[0] = 2*i;
    ptIds[1] = 2*i + 1;
    lines->InsertNextCell(2, ptIds);
    }

  if (this->AxisVisibility)
    {
    //first axis point
    ptIds[0] = pts->InsertNextPoint(this->Point1Coordinate->GetValue()); 
    //last axis point
    ptIds[1] = pts->InsertNextPoint(this->Point2Coordinate->GetValue()); 
    lines->InsertNextCell(2, ptIds);
    }
}

// *********************************************************************
// Returns true if any tick vis attribute has changed since last check.
//
// Programmer:  Kathleen Bonnell
// Creation:    November 7, 2001
// *********************************************************************
bool vtkAxisActor::TickVisibilityChanged()
{
  bool retVal = (this->TickVisibility != this->LastTickVisibility) ||
                (this->DrawGridlines != this->LastDrawGridlines)   || 
                (this->MinorTicksVisible != this->LastMinorTicksVisible);

  this->LastTickVisibility = this->TickVisibility;
  this->LastDrawGridlines = this->DrawGridlines;
  this->LastMinorTicksVisible = this->MinorTicksVisible;

  return retVal; 
}

// *********************************************************************
// Set the bounds for this actor to use.  Sets timestamp BoundsModified.
//
// Programmer:  Kathleen Bonnell
// Creation:    November 7, 2001
// *********************************************************************
void
vtkAxisActor::SetBounds(double b[6])
{
  if ((this->Bounds[0] != b[0]) ||
      (this->Bounds[1] != b[1]) ||
      (this->Bounds[2] != b[2]) ||
      (this->Bounds[3] != b[3]) ||
      (this->Bounds[4] != b[4]) ||
      (this->Bounds[5] != b[5]) )
    {
    for (int i = 0; i < 6; i++)
      {
      this->Bounds[i] = b[i];
      }
    this->BoundsTime.Modified();
    }
}

// *********************************************************************
// Retrieves the bounds of this actor. 
//
// Programmer:  Kathleen Bonnell
// Creation:    November 7, 2001
// *********************************************************************
double* vtkAxisActor::GetBounds()
{
  return this->Bounds;
}

// *********************************************************************
// Retrieves the bounds of this actor. 
//
// Programmer:  Kathleen Bonnell
// Creation:    November 7, 2001
// *********************************************************************

void vtkAxisActor::GetBounds(double b[6])
{
  for (int i = 0; i < 6; i++)
    {
    b[i] = this->Bounds[i];
    }
}

// *********************************************************************
// Method:  vtkAxisActor::ComputeMaxLabelLength
//
// Purpose: Determines the maximum length that a label will occupy
//          if placed at point 'center' and with a scale of 1. 
//
// Arguments:
//   center    The position to use for the label actor 
//
// Returns:
//   the maximum length of all the labels, 0 if there are no labels.
//
// Programmer:  Kathleen Bonnell
// Creation:    July 18, 2003 
//
// Modifications:
//   Kathleen Bonnell, Tue Dec 16 11:06:21 PST 2003
//   Reset the actor's position and scale.
//
// *********************************************************************

double vtkAxisActor::ComputeMaxLabelLength(const double center[3])
{
  double length, maxLength = 0.;
  double pos[3];
  double scale;
  for (int i = 0; i < this->NumberOfLabelsBuilt; i++)
    {
    this->LabelActors[i]->GetPosition(pos);
    scale = this->LabelActors[i]->GetScale()[0];

    this->LabelActors[i]->SetCamera(this->Camera);
    this->LabelActors[i]->SetProperty(this->GetProperty());
    this->LabelActors[i]->SetPosition(center[0], center[1] , center[2]);
    this->LabelActors[i]->SetScale(1.);
    length = this->LabelActors[i]->GetLength();
    maxLength = (length > maxLength ? length : maxLength); 

    this->LabelActors[i]->SetPosition(pos);
    this->LabelActors[i]->SetScale(scale);
    }
  return maxLength;
}

// *********************************************************************
// Method:  vtkAxisActor::ComputeTitleLength
//
// Purpose: Determines the length that the title will occupy
//          if placed at point 'center' and with a scale of 1. 
//
// Arguments:
//   center    The position to use for the title actor 
//
// Returns:
//   the length of all the title, 
//
// Programmer:  Kathleen Bonnell
// Creation:    July 25, 2003 
//
// Modifications:
//   Kathleen Bonnell, Tue Dec 16 11:06:21 PST 2003
//   Reset the actor's position and scale.
//
// *********************************************************************

double vtkAxisActor::ComputeTitleLength(const double center[3])
{
  double pos[3], scale, len;
  this->TitleActor->GetPosition(pos);
  scale = this->TitleActor->GetScale()[0];
  this->TitleVector->SetText(this->Title);
  this->TitleActor->SetCamera(this->Camera);
  this->TitleActor->SetProperty(this->GetProperty());
  this->TitleActor->SetPosition(center[0], center[1] , center[2]);
  this->TitleActor->SetScale(1.);
  len = this->TitleActor->GetLength();

  this->TitleActor->SetPosition(pos);
  this->TitleActor->SetScale(scale);
  return len;
}

// *********************************************************************
// Method:  vtkAxisActor::SetLabelScale
//
// Purpose: Sets the scaling factor for label actors.
//
// Arguments:
//   s      The scale factor to use.
//
// Programmer:  Kathleen Bonnell
// Creation:    July 18, 2003 
//
// *********************************************************************

void vtkAxisActor::SetLabelScale(const double s)
{
  for (int i=0; i < this->NumberOfLabelsBuilt; i++)
    {
    this->LabelActors[i]->SetScale(s);
    }
}

// *********************************************************************
// Method:  vtkAxisActor::SetTitleScale
//
// Purpose: Sets the scaling factor for the title actor.
//
// Arguments:
//   s      The scale factor to use.
//
// Programmer:  Kathleen Bonnell
// Creation:    July 18, 2003 
//
// *********************************************************************

void vtkAxisActor::SetTitleScale(const double s)
{
  this->TitleActor->SetScale(s);
}

// *********************************************************************
// Method:  vtkAxisActor::SetTitle
//
// Purpose: Sets the text for the title.  
//
// Notes:   Not using vtkSetStringMacro so that the modification time of 
//          the text can be kept (so the title will be updated as
//          appropriate when the text changes.)
//
// Arguments:
//   t          The text to use. 
//
// Programmer:  Kathleen Bonnell
// Creation:    August 31, 2004 
//
// *********************************************************************

void vtkAxisActor::SetTitle(const char *t)
{
  if (this->Title == NULL && t == NULL)
    {
    return;
    }
  if (this->Title && (!strcmp(this->Title, t)))
    {
    return;
    }
  if (this->Title)
    {
    delete [] this->Title;
    }
  if (t)
    {
    this->Title = new char[strlen(t)+1];
    strcpy(this->Title, t);
    }
  else 
    {
    this->Title = NULL;
    }
  this->TitleTextTime.Modified();
  this->Modified();
}

//---------------------------------------------------------------------------
// endpoint-related methods
vtkCoordinate* vtkAxisActor::GetPoint1Coordinate()
{
  vtkDebugMacro(<< this->GetClassName() << " (" << this
                << "): returning Point1 Coordinate address "
                << this->Point1Coordinate );
  return this->Point1Coordinate;
}

vtkCoordinate* vtkAxisActor::GetPoint2Coordinate()
{
  vtkDebugMacro(<< this->GetClassName() << " (" << this
                << "): returning Point2 Coordinate address "
                << this->Point2Coordinate );
  return this->Point2Coordinate;
}

void vtkAxisActor::SetPoint1(double x, double y, double z)
{
  this->Point1Coordinate->SetValue(x, y, z);
}

void vtkAxisActor::SetPoint2(double x, double y, double z)
{
  this->Point2Coordinate->SetValue(x, y, z);
}

double* vtkAxisActor::GetPoint1()
{
  return this->Point1Coordinate->GetValue();
}

double* vtkAxisActor::GetPoint2()
{
  return this->Point2Coordinate->GetValue();
}
