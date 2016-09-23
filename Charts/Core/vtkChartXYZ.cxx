/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkChartXYZ.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkChartXYZ.h"

#include "vtkAnnotationLink.h"
#include "vtkAxis.h"
#include "vtkCommand.h"
#include "vtkContext2D.h"
#include "vtkContext3D.h"
#include "vtkContextKeyEvent.h"
#include "vtkContextMouseEvent.h"
#include "vtkContextScene.h"
#include "vtkLookupTable.h"
#include "vtkMath.h"
#include "vtkPen.h"
#include "vtkPlane.h"
#include "vtkPlaneCollection.h"
#include "vtkPlot3D.h"
#include "vtkTable.h"
#include "vtkTextProperty.h"
#include "vtkTransform.h"
#include "vtkVector.h"
#include "vtkVectorOperators.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkIdTypeArray.h"

#include "vtkObjectFactory.h"

#include <sstream>

vtkStandardNewMacro(vtkChartXYZ)

//-----------------------------------------------------------------------------
vtkChartXYZ::vtkChartXYZ() : Geometry(0, 0, 10, 10), IsX(false), Angle(0)
{
  this->Pen->SetWidth(5);
  this->Pen->SetColor(0, 0, 0, 255);
  this->AxisPen->SetWidth(1);
  this->AxisPen->SetColor(0, 0, 0, 255);
  this->Rotation->Identity();
  this->Rotation->PostMultiply();
  this->Translation->Identity();
  this->Translation->PostMultiply();
  this->Scale->Identity();
  this->Scale->PostMultiply();
  this->Interactive = true;
  this->SceneWidth = 0;
  this->SceneHeight = 0;
  this->InitializeAxesBoundaryPoints();
  this->AutoRotate = false;
  this->DrawAxesDecoration = true;
  this->FitToScene = true;
  this->Axes.resize(3);
  for(unsigned int i = 0; i < 3; ++i)
  {
    vtkNew<vtkAxis> axis;
    this->Axes[i] = axis.GetPointer();
  }
}

//-----------------------------------------------------------------------------
vtkChartXYZ::~vtkChartXYZ()
{
}

//-----------------------------------------------------------------------------
void vtkChartXYZ::SetAngle(double angle)
{
  this->Angle = angle;
}

//-----------------------------------------------------------------------------
void vtkChartXYZ::SetAroundX(bool IsX_)
{
  this->IsX = IsX_;
}

//-----------------------------------------------------------------------------
void vtkChartXYZ::SetAutoRotate(bool b)
{
  this->AutoRotate = b;
}

//-----------------------------------------------------------------------------
void vtkChartXYZ::SetDecorateAxes(bool b)
{
  this->DrawAxesDecoration = b;
}

//-----------------------------------------------------------------------------
void vtkChartXYZ::SetAnnotationLink(vtkAnnotationLink *link)
{
  if (this->Link != link)
  {
    this->Link = link;
    this->Modified();
  }
}

//-----------------------------------------------------------------------------
vtkAxis * vtkChartXYZ::GetAxis(int axis)
{
  assert(axis >= 0 && axis < 3);
  return this->Axes[axis].GetPointer();
}

//-----------------------------------------------------------------------------
void vtkChartXYZ::SetAxisColor(const vtkColor4ub& color)
{
  this->AxisPen->SetColor(color);
  this->Modified();
}

//-----------------------------------------------------------------------------
vtkColor4ub vtkChartXYZ::GetAxisColor()
{
  return this->AxisPen->GetColorObject();
}

//-----------------------------------------------------------------------------
void vtkChartXYZ::SetGeometry(const vtkRectf &bounds)
{
  this->Geometry = bounds;

  this->Axes[0]->SetPoint1(vtkVector2f(this->Geometry.GetX(),
                           this->Geometry.GetY()));
  this->Axes[0]->SetPoint2(vtkVector2f(this->Geometry.GetX() + this->Geometry.GetWidth(),
                           this->Geometry.GetY()));

  this->Axes[1]->SetPoint1(vtkVector2f(this->Geometry.GetX(),
                           this->Geometry.GetY()));
  this->Axes[1]->SetPoint2(vtkVector2f(this->Geometry.GetX(),
                           this->Geometry.GetY() + this->Geometry.GetHeight()));

  // Z is faked, largely to get valid ranges and rounded numbers...
  this->Axes[2]->SetPoint1(vtkVector2f(this->Geometry.GetX(),
                           0));
  if (this->IsX)
  {
    this->Axes[2]->SetPoint2(vtkVector2f(this->Geometry.GetX(),
                             this->Geometry.GetHeight()));
  }
  else
  {
    this->Axes[2]->SetPoint2(vtkVector2f(this->Geometry.GetX(),
                             this->Geometry.GetWidth()));
  }
}

//-----------------------------------------------------------------------------
void vtkChartXYZ::RecalculateBounds()
{
  if (this->Plots.empty())
  {
    return;
  }

  double bounds[] = { VTK_DOUBLE_MAX, VTK_DOUBLE_MIN,
                      VTK_DOUBLE_MAX, VTK_DOUBLE_MIN,
                      VTK_DOUBLE_MAX, VTK_DOUBLE_MIN};

  // Need to calculate the bounds in three dimensions and set up the axes.
  for (unsigned int i = 0; i < this->Plots.size(); ++i)
  {
    std::vector<vtkVector3f> const& points = this->Plots[i]->GetPoints();
    for (unsigned int j = 0; j < points.size(); ++j)
    {
      const vtkVector3f &v = points[j];
      for (int k = 0; k < 3; ++k)
      {
        if (v[k] < bounds[2 * k])
        {
          bounds[2 * k] = v[k];
        }
        if (v[k] > bounds[2 * k + 1])
        {
          bounds[2 * k + 1] = v[k];
        }
      }
    }
  }
  for (int i = 0; i < 3; ++i)
  {
    this->Axes[i]->SetUnscaledRange(&bounds[2*i]);
  }

  // Recalculate transform since axes' ranges were modified
  this->RecalculateTransform();
}

//-----------------------------------------------------------------------------
void vtkChartXYZ::PrintSelf(ostream &os, vtkIndent indent)
{
  Superclass::PrintSelf(os, indent);
}

//-----------------------------------------------------------------------------
void vtkChartXYZ::Update()
{
  if (this->Link)
  {
    vtkSelection *selection =
        vtkSelection::SafeDownCast(this->Link->GetOutputDataObject(2));
    if (selection->GetNumberOfNodes())
    {
      vtkSelectionNode *node = selection->GetNode(0);
      vtkIdTypeArray *idArray =
          vtkArrayDownCast<vtkIdTypeArray>(node->GetSelectionList());
      for (size_t i = 0; i < this->Plots.size(); ++i)
      {
        this->Plots[i]->SetSelection(idArray);
      }
    }
  }
}

//-----------------------------------------------------------------------------
bool vtkChartXYZ::Paint(vtkContext2D *painter)
{
  if (!this->Visible)
    return false;

  this->Update();

  // Get the 3D context.
  vtkContext3D *context = painter->GetContext3D();

  if (!context)
    return false;

  this->Update();

  // Check if the scene changed size
  bool resizeHappened = false;
  if (this->FitToScene)
  {
    resizeHappened = this->CheckForSceneResize();
  }

  // Calculate the transforms required for the current rotation.
  this->CalculateTransforms();

  // Set up clipping planes
  for (int i = 0; i < 6; i++)
  {
    double planeEquation[4];
    this->GetClippingPlaneEquation(i, planeEquation);
    context->EnableClippingPlane(i, planeEquation);
  }

  // Draw plots
  context->PushMatrix();
  context->AppendTransform(this->ContextTransform.GetPointer());
  this->PaintChildren(painter);

  // Remove clipping planes
  for (int i = 0; i < 6; i++)
  {
    context->DisableClippingPlane(i);
  }

  // Calculate the bounds of the data within the axes
  this->ComputeDataBounds();

  // Pop the ContextTransform now that we're done drawing data within the axes
  context->PopMatrix();

  // Draw the axes, tick marks, and labels
  this->DrawAxes(context);
  if(this->DrawAxesDecoration)
  {
    this->DetermineWhichAxesToLabel();
    this->DrawTickMarks(painter);
    this->DrawAxesLabels(painter);
  }

  // If necessary, rescale the axes so they fits our scene nicely
  if (resizeHappened)
  {
    this->RescaleAxes();
  }

  return true;
}

//-----------------------------------------------------------------------------
void vtkChartXYZ::DrawAxes(vtkContext3D *context)
{
  context->PushMatrix();
  context->AppendTransform(this->Box.GetPointer());
  context->ApplyPen(this->AxisPen.GetPointer());

  vtkVector3f box[4];
  box[0] = vtkVector3f(0, 0, 0);
  box[1] = vtkVector3f(0, 1, 0);
  box[2] = vtkVector3f(1, 1, 0);
  box[3] = vtkVector3f(1, 0, 0);
  context->DrawLine(box[0], box[1]);
  context->DrawLine(box[1], box[2]);
  context->DrawLine(box[2], box[3]);
  context->DrawLine(box[3], box[0]);
  for (int i = 0; i < 4; ++i)
  {
    box[i].SetZ(1);
  }
  context->DrawLine(box[0], box[1]);
  context->DrawLine(box[1], box[2]);
  context->DrawLine(box[2], box[3]);
  context->DrawLine(box[3], box[0]);
  context->DrawLine(vtkVector3f(0, 0, 0), vtkVector3f(0, 0, 1));
  context->DrawLine(vtkVector3f(1, 0, 0), vtkVector3f(1, 0, 1));
  context->DrawLine(vtkVector3f(0, 1, 0), vtkVector3f(0, 1, 1));
  context->DrawLine(vtkVector3f(1, 1, 0), vtkVector3f(1, 1, 1));
}

//-----------------------------------------------------------------------------
void vtkChartXYZ::ComputeDataBounds()
{
  double xMin = VTK_DOUBLE_MAX;
  double xMax = VTK_DOUBLE_MIN;
  double yMin = VTK_DOUBLE_MAX;
  double yMax = VTK_DOUBLE_MIN;
  float transformedPoint[3];

  for (unsigned int i = 0; i < this->Plots.size(); ++i)
  {
    vtkPlot3D *plot = this->Plots[i];

    // examine the eight corners of this plot's bounding cube
    for (unsigned int j = 0; j < 8; ++j)
    {
      this->ContextTransform->TransformPoint(
        plot->GetDataBounds()[j].GetData(), transformedPoint);

      if (transformedPoint[0] < xMin)
      {
        xMin = transformedPoint[0];
      }
      if (transformedPoint[0] > xMax)
      {
        xMax = transformedPoint[0];
      }
      if (transformedPoint[1] < yMin)
      {
        yMin = transformedPoint[1];
      }
      if (transformedPoint[1] > yMax)
      {
        yMax = transformedPoint[1];
      }
    }
  }

  this->DataBounds[0] = xMin;
  this->DataBounds[1] = yMin;
  this->DataBounds[2] = xMax;
  this->DataBounds[3] = yMax;
}

//-----------------------------------------------------------------------------
void vtkChartXYZ::DrawAxesLabels(vtkContext2D *painter)
{
  vtkContext3D *context = painter->GetContext3D();

  // set up text property
  vtkNew<vtkTextProperty> textProperties;
  textProperties->SetJustificationToCentered();
  textProperties->SetVerticalJustificationToCentered();
  textProperties->SetColor(0.0, 0.0, 0.0);
  textProperties->SetFontFamilyToArial();
  textProperties->SetFontSize(14);
  painter->ApplyTextProp(textProperties.GetPointer());

  // if we're looking directly down any dimension, we shouldn't draw the
  // corresponding label
  bool shouldDrawAxis[3];
  for (int axis = 0; axis < 3; ++axis)
  {
    shouldDrawAxis[axis] = true;
    float start[3] = { 0, 0, 0 };
    float end[3] = { 0, 0, 0 };
    end[axis] = 1;
    this->Box->TransformPoint(start, start);
    this->Box->TransformPoint(end, end);
    float axisLength = sqrt(
      (end[0] - start[0]) * (end[0] - start[0]) +
      (end[1] - start[1]) * (end[1] - start[1]));
    if (axisLength == 0)
    {
      shouldDrawAxis[axis] = false;
    }
  }

  float bounds[4];
  float xLabelPos[3];
  float yLabelPos[3];
  float zLabelPos[3];
  float offset[2] = {0, 0};

  // calculate the pixel coordinates of the lines we wish to label
  if (shouldDrawAxis[0])
  {
    xLabelPos[0]  = 0.5;
    xLabelPos[1]  = this->XAxisToLabel[0];
    xLabelPos[2]  = this->XAxisToLabel[1];
    this->Box->TransformPoint(xLabelPos, xLabelPos);
  }
  if (shouldDrawAxis[1])
  {
    yLabelPos[0]  = this->YAxisToLabel[0];
    yLabelPos[1]  = 0.5;
    yLabelPos[2]  = this->YAxisToLabel[1];
    this->Box->TransformPoint(yLabelPos, yLabelPos);
  }
  if (shouldDrawAxis[2])
  {
    zLabelPos[0]  = this->ZAxisToLabel[0];
    zLabelPos[1]  = this->ZAxisToLabel[1];
    zLabelPos[2]  = 0.5;
    this->Box->TransformPoint(zLabelPos, zLabelPos);
  }

  context->PopMatrix();

  if (shouldDrawAxis[0])
  {
    painter->ComputeStringBounds(this->XAxisLabel, bounds);
    this->GetOffsetForAxisLabel(0, bounds, offset);
    xLabelPos[0] += (offset[0] + this->TickLabelOffset[0][0]);
    xLabelPos[1] += (offset[1] + this->TickLabelOffset[0][1]);
    painter->DrawString(xLabelPos[0], xLabelPos[1], this->XAxisLabel);
  }

  if (shouldDrawAxis[1])
  {
    painter->ComputeStringBounds(this->YAxisLabel, bounds);
    offset[0] = 0;
    offset[1] = 0;
    this->GetOffsetForAxisLabel(1, bounds, offset);
    yLabelPos[0] += (offset[0] + this->TickLabelOffset[1][0]);
    yLabelPos[1] += (offset[1] + this->TickLabelOffset[1][1]);
    painter->DrawString(yLabelPos[0], yLabelPos[1], this->YAxisLabel);
  }

  if (shouldDrawAxis[2])
  {
    painter->ComputeStringBounds(this->ZAxisLabel, bounds);
    offset[0] = 0;
    offset[1] = 0;
    this->GetOffsetForAxisLabel(2, bounds, offset);
    zLabelPos[0] += (offset[0] + this->TickLabelOffset[2][0]);
    zLabelPos[1] += (offset[1] + this->TickLabelOffset[2][1]);
    painter->DrawString(zLabelPos[0], zLabelPos[1], this->ZAxisLabel);
  }
}

//-----------------------------------------------------------------------------
void vtkChartXYZ::GetOffsetForAxisLabel(int axis, float *bounds,
                                                   float *offset)
{
  offset[0] = 0;
  offset[1] = 0;
  switch (this->DirectionToData[axis])
  {
    // data is to the north
    // offset is -y
    case 0:
      offset[1] = -bounds[3];
      break;

    // data is northeast
    // offset is -x, -y
    case 1:
      offset[0] = -bounds[2];
      offset[1] = -bounds[3];
      break;

    // data is east
    // offset is -x
    case 2:
      offset[0] = -bounds[2];
      break;

    // data is southeast
    // offset is -x, +y
    case 3:
      offset[0] = -bounds[2];
      offset[1] = bounds[3];
      break;

    // data is south
    // offset is +y
    case 4:
      offset[1] = bounds[3];
      break;

    // data is southwest
    // offset is +x, +y
    case 5:
      offset[0] = bounds[2];
      offset[1] = bounds[3];
      break;

    // data is west
    // offset is +y
    case 6:
      offset[0] = bounds[2];
      break;

    // data is northwest
    // offset is +x, -y
    case 7:
    default:
      offset[0] = bounds[2];
      offset[1] = -bounds[3];
      break;
  }
}

//-----------------------------------------------------------------------------
void vtkChartXYZ::DrawTickMarks(vtkContext2D *painter)
{
  vtkContext3D *context = painter->GetContext3D();
  float bounds[4];

  // draw points instead of lines
  context->ApplyPen(this->Pen.GetPointer());

  // treat each axis separately
  for (int axis = 0; axis < 3; ++axis)
  {
    // pop matrix since we'll be drawing text in 2D before we draw the
    // actual tick marks
    context->PopMatrix();
    float labelOffset[2] = { 0, 0 };

    // initialize start and end of the axis to label in box coordinates
    double startBox[3] = { 0, 0, 0 };
    double endBox[3] = { 0, 0, 0 };
    switch (axis)
    {
      case 0:
        startBox[0] = 0;
        endBox[0]   = 1;
        startBox[1] = endBox[1] = this->XAxisToLabel[0];
        startBox[2] = endBox[2] = this->XAxisToLabel[1];
        break;
      case 1:
        startBox[0] = this->YAxisToLabel[0];
        startBox[1] = 0;
        endBox[1]   = 1;
        startBox[2] = endBox[2] = this->YAxisToLabel[1];
        break;
      case 2:
      default:
        startBox[0] = endBox[0] = this->ZAxisToLabel[0];
        startBox[1] = endBox[1] = this->ZAxisToLabel[1];
        startBox[2] = 0;
        endBox[2]   = 1;
        break;
    }

    // convert these values to pixel coordinates
    double start[3];
    double end[3];
    this->Box->TransformPoint(startBox, start);
    this->Box->TransformPoint(endBox, end);

    // ...and then into data coordinates
    this->ContextTransform->GetInverse()->TransformPoint(start, start);
    this->ContextTransform->GetInverse()->TransformPoint(end, end);

    // get "nice" values for min, max, and spacing (again, in data coordinates)
    double tickSpacing =
      this->CalculateNiceMinMax(start[axis], end[axis], axis);

    if (tickSpacing == -1)
    {
      continue;
    }

    std::vector < vtkVector3f > tickPoints;
    int currentTick = 0;
    float tickPositionAlongAxis = start[axis];
    while (tickPositionAlongAxis < end[axis])
    {
      vtkVector3f tick;
      // convert tick position back into box coordinates
      // during this process, we save the tick position in pixels for labeling
      float tickPosition[3];
      tickPosition[0] = start[0];
      tickPosition[1] = start[1];
      tickPosition[2] = start[2];
      tickPosition[axis] = tickPositionAlongAxis;
      float tickPositionInPixels[3];
      this->ContextTransform->TransformPoint(tickPosition,
                                             tickPositionInPixels);
      this->Box->GetInverse()->TransformPoint(tickPositionInPixels,
                                              tickPosition);

      // determine the location of this tick mark and push it onto the vector
      // if it falls within the bounds of the axis
      tick[0] = startBox[0];
      tick[1] = startBox[1];
      tick[2] = startBox[2];
      tick[axis] = tickPosition[axis];

      if (tick[axis] >= startBox[axis] && tick[axis] <= endBox[axis])
      {
        tickPoints.push_back(tick);

        // get the tick mark label
        std::stringstream sstream;
        sstream << std::fixed << setprecision(1) << tickPositionAlongAxis;
        std::string tickLabel = sstream.str();

        // offset the label from the axis
        float offset[2] = {0, 0};
        painter->ComputeStringBounds(tickLabel, bounds);
        this->GetOffsetForAxisLabel(axis, bounds, offset);
        tickPositionInPixels[0] += offset[0];
        tickPositionInPixels[1] += offset[1];

        // we store this offset so we know where to draw the axis label later
        if (fabs(offset[0]) > fabs(labelOffset[0]))
        {
          labelOffset[0] = offset[0];
        }
        if (fabs(offset[1]) > fabs(labelOffset[1]))
        {
          labelOffset[1] = offset[1];
        }

        // draw the label for this tick mark
        painter->DrawString(tickPositionInPixels[0], tickPositionInPixels[1],
                            tickLabel);
      }
      ++currentTick;
      tickPositionAlongAxis = start[axis] + (tickSpacing * currentTick);
    }

    // re-apply the Box matrix and draw the tick marks as points
    if (tickPoints.size() != 0)
    {
      context->PushMatrix();
      context->AppendTransform(this->Box.GetPointer());
      context->DrawPoints(tickPoints[0].GetData(),
                          static_cast<int>(tickPoints.size()));
      this->TickLabelOffset[axis][0] = labelOffset[0];
      this->TickLabelOffset[axis][1] = labelOffset[1];
    }
  }

  //revert from drawing points.
  context->ApplyPen(this->AxisPen.GetPointer());
}

//-----------------------------------------------------------------------------
void vtkChartXYZ::DetermineWhichAxesToLabel()
{
  // for each dimension (XYZ)
  for (int axis = 0; axis < 3; ++axis)
  {
    double maxDistance = -1;
    // for each of the four "axis" lines corresponding to this dimension
    for (float i = 0; i < 2; ++i)
    {
      for (float j = 0; j < 2; ++j)
      {
        for (float k = 0; k < 2; ++k)
        {
          // convert this line's midpoint to screen (pixel) coordinates
          float midpoint[3] = { i, j, k };
          midpoint[axis] = 0.5;
          this->Box->TransformPoint(midpoint, midpoint);

          // ignore any lines whose midpoint falls within the data range.
          // we increment the iterators so we don't evaluate the same line
          // twice.
          if (midpoint[0] > this->DataBounds[0] &&
              midpoint[1] > this->DataBounds[1] &&
              midpoint[0] < this->DataBounds[2] &&
              midpoint[1] < this->DataBounds[3])
          {
            switch (axis)
            {
              case 0:
                ++i;
                break;
              case 1:
                ++j;
                break;
              case 2:
                ++k;
                break;
              default:
                break;
            }
            continue;
          }

          // calculate the distance from this line's midpoint to the data range
          double d = 0;
          int directionToData = 0;

          // case 1: midpoint falls within x range (but not y)
          if (midpoint[0] > this->DataBounds[0] &&
              midpoint[0] < this->DataBounds[2])
          {
            double d1 = fabs(midpoint[1] - this->DataBounds[1]);
            double d2 = fabs(midpoint[1] - this->DataBounds[3]);
            if (d1 < d2)
            {
              directionToData = 0;  // data is "up" from the axis
              d = d1;
            }
            else
            {
              directionToData = 4;  // data is "down" from the axis
              d = d2;
            }
          }

          // case 2: midpoint falls within y range (but not x)
          else if (midpoint[1] > this->DataBounds[1] &&
                   midpoint[1] < this->DataBounds[3])
          {
            double d1 = fabs(midpoint[0] - this->DataBounds[0]);
            double d2 = fabs(midpoint[0] - this->DataBounds[2]);
            if (d1 < d2)
            {
              directionToData = 2;  // data is "right" from the axis
              d = d1;
            }
            else
            {
              directionToData = 6;  // data is "left" from the axis
              d = d2;
            }
          }

          // case 3: compute distance to nearest corner
          else
          {
            //x min, y min
            d = sqrt( (this->DataBounds[0] - midpoint[0]) *
                      (this->DataBounds[0] - midpoint[0]) +
                      (this->DataBounds[1] - midpoint[1]) *
                      (this->DataBounds[1] - midpoint[1]) );
            directionToData = 1;  // data is to the northeast

            //x min, y max
            double d0 =
              sqrt( (this->DataBounds[0] - midpoint[0]) *
                    (this->DataBounds[0] - midpoint[0]) +
                    (this->DataBounds[3] - midpoint[1]) *
                    (this->DataBounds[3] - midpoint[1]) );
            if (d0 < d)
            {
              d = d0;
              directionToData = 3;  // data is to the southeast
            }
            //x max, y min
            d0 = sqrt( (this->DataBounds[2] - midpoint[0]) *
                       (this->DataBounds[2] - midpoint[0]) +
                       (this->DataBounds[1] - midpoint[1]) *
                       (this->DataBounds[1] - midpoint[1]) );
            if (d0 < d)
            {
              d = d0;
              directionToData = 7;  // data is to the northwest
            }
            //x max, y max
            d0 = sqrt( (this->DataBounds[2] - midpoint[0]) *
                       (this->DataBounds[2] - midpoint[0]) +
                       (this->DataBounds[3] - midpoint[1]) *
                       (this->DataBounds[3] - midpoint[1]) );
            if (d0 < d)
            {
              d = d0;
              directionToData = 5;  // data is to the southwest
            }

            // Test if the data falls within the bounds of our axis line,
            // despite the fact that it is diagonal from the line's midpoint.
            // This is performed to determine how the label should be offset
            // from the line.  To do this, we transform the line's start and
            // end point to pixel coordinates.
            float start[3] = { i, j, k };
            start[axis] = 0;
            this->Box->TransformPoint(start, start);
            float end[3] = { i, j, k };
            end[axis] = 1;
            this->Box->TransformPoint(end, end);

            if (start[0] < this->DataBounds[0] && end[0] > this->DataBounds[2])
            {
                // data falls within horizontal range of this axis line
                // set directionToData as purely up or purely down
                if (directionToData == 1 || directionToData == 7)
                {
                  directionToData = 0;
                }
                else
                {
                  directionToData = 4;
                }
            }
            else if (start[1] < this->DataBounds[1] &&
                     end[1] > this->DataBounds[3])
            {
              // data falls within vertical range of this axis line
              // set directionToData as purely left or purely right
                if (directionToData == 1 || directionToData == 3)
                {
                  directionToData = 2;
                }
                else
                {
                  directionToData = 6;
                }
            }
          }

          // record this axis line if it has the greatest distance to the data
          if (d > maxDistance)
          {
            this->DirectionToData[axis] = directionToData;
            maxDistance = d;
            switch (axis)
            {
              case 0:
                this->XAxisToLabel[0] = static_cast<int>(j);
                this->XAxisToLabel[1] = static_cast<int>(k);
                break;
              case 1:
                this->YAxisToLabel[0] = static_cast<int>(i);
                this->YAxisToLabel[1] = static_cast<int>(k);
                break;
              case 2:
              default:
                this->ZAxisToLabel[0] = static_cast<int>(i);
                this->ZAxisToLabel[1] = static_cast<int>(j);
                break;
            }
          }

          // these three cases keep us from evaluating the same line twice.
          if (axis == 2)
          {
            ++k;
          }
        }
        if (axis == 1)
        {
          ++j;
        }
      }
      if (axis == 0)
      {
        ++i;
      }
    }
  }
}

//-----------------------------------------------------------------------------
bool vtkChartXYZ::Hit(const vtkContextMouseEvent& vtkNotUsed(mouse))
{
  if (!this->Interactive || !this->Visible || this->AutoRotate)
  {
    return false;
  }

  return true;
}

//-----------------------------------------------------------------------------
bool vtkChartXYZ::MouseButtonPressEvent(const vtkContextMouseEvent
                                                   &mouse)
{
  if (mouse.GetButton() == vtkContextMouseEvent::LEFT_BUTTON)
  {
    return true;
  }
  return false;
}

//-----------------------------------------------------------------------------
bool vtkChartXYZ::MouseMoveEvent(const vtkContextMouseEvent &mouse)
{
  if (mouse.GetButton() == vtkContextMouseEvent::LEFT_BUTTON)
  {
    if (mouse.GetModifiers() == vtkContextMouseEvent::SHIFT_MODIFIER)
    {
      return this->Spin(mouse);
    }
    else
    {
      return this->Rotate(mouse);
    }
  }
  if (mouse.GetButton() == vtkContextMouseEvent::RIGHT_BUTTON)
  {
    if (mouse.GetModifiers() == vtkContextMouseEvent::SHIFT_MODIFIER)
    {
      return this->Pan(mouse);
    }
    else
    {
      return this->Zoom(mouse);
    }
  }
  return false;
}

//-----------------------------------------------------------------------------
bool vtkChartXYZ::MouseWheelEvent(const vtkContextMouseEvent&,
                                             int delta)
{
  // Ten "wheels" to double/halve zoom level
  float scaling = pow(2.0f, delta/10.0f);
  this->Scale->Scale(scaling, scaling, scaling);

  // Mark the scene as dirty
  this->Scene->SetDirty(true);

  this->InvokeEvent(vtkCommand::InteractionEvent);
  return true;
}

//-----------------------------------------------------------------------------
void vtkChartXYZ::ZoomAxes(int delta)
{
  float scaling = pow(2.0f, delta/10.0f);
  this->BoxScale->Scale(scaling, scaling, scaling);

  // Mark the scene as dirty
  this->Scene->SetDirty(true);
}

//-----------------------------------------------------------------------------
bool vtkChartXYZ::Rotate(const vtkContextMouseEvent &mouse)
{
  // avoid NaNs in our transformation matrix if the scene has not yet been
  // rendered.
  if (this->Scene->GetSceneHeight() == 0 || this->Scene->GetSceneWidth() == 0)
  {
    return false;
  }

  // Figure out how much the mouse has moved in plot coordinates
  vtkVector2d screenPos(mouse.GetScreenPos().Cast<double>().GetData());
  vtkVector2d lastScreenPos(mouse.GetLastScreenPos().Cast<double>().GetData());

  double dx = screenPos[0] - lastScreenPos[0];
  double dy = screenPos[1] - lastScreenPos[1];

  double delta_elevation = -20.0 / this->Scene->GetSceneHeight();
  double delta_azimuth = -20.0 / this->Scene->GetSceneWidth();

  double rxf = -dx * delta_azimuth * 10.0;
  double ryf = -dy * delta_elevation * 10.0;

  this->Rotation->RotateY(rxf);
  this->Rotation->RotateX(-ryf);

  // Mark the scene as dirty
  this->Scene->SetDirty(true);

  this->InvokeEvent(vtkCommand::InteractionEvent);
  return true;
}

//-----------------------------------------------------------------------------
bool vtkChartXYZ::Pan(const vtkContextMouseEvent &mouse)
{
  // Figure out how much the mouse has moved in plot coordinates
  vtkVector2d screenPos(mouse.GetScreenPos().Cast<double>().GetData());
  vtkVector2d lastScreenPos(mouse.GetLastScreenPos().Cast<double>().GetData());

  double dx = (screenPos[0] - lastScreenPos[0]);
  double dy = (screenPos[1] - lastScreenPos[1]);

  this->Translation->Translate(dx, dy, 0.0);

  // Mark the scene as dirty
  this->Scene->SetDirty(true);

  this->InvokeEvent(vtkCommand::InteractionEvent);
  return true;
}

//-----------------------------------------------------------------------------
bool vtkChartXYZ::Zoom(const vtkContextMouseEvent &mouse)
{
  // Figure out how much the mouse has moved and scale accordingly
  vtkVector2d screenPos(mouse.GetScreenPos().Cast<double>().GetData());
  vtkVector2d lastScreenPos(mouse.GetLastScreenPos().Cast<double>().GetData());

  float delta = 0.0f;
  if (this->Scene->GetSceneHeight() > 0)
  {
    delta = static_cast<float>(
      mouse.GetLastScreenPos()[1] - mouse.GetScreenPos()[1]) /
      this->Scene->GetSceneHeight();
  }

  // Dragging full screen height zooms 4x.
  float scaling = pow(4.0f, delta);
  this->Scale->Scale(scaling, scaling, scaling);

  // Mark the scene as dirty
  this->Scene->SetDirty(true);

  this->InvokeEvent(vtkCommand::InteractionEvent);
  return true;
}

//-----------------------------------------------------------------------------
bool vtkChartXYZ::Spin(const vtkContextMouseEvent &mouse)
{
  // Figure out how much the mouse has moved in plot coordinates
  vtkVector2d screenPos(mouse.GetScreenPos().Cast<double>().GetData());
  vtkVector2d lastScreenPos(mouse.GetLastScreenPos().Cast<double>().GetData());

  double newAngle =
    vtkMath::DegreesFromRadians(atan2(screenPos[1], screenPos[0]));
  double oldAngle =
    vtkMath::DegreesFromRadians(atan2(lastScreenPos[1], lastScreenPos[0]));

  this->Rotation->RotateZ(-(newAngle - oldAngle));

  // Mark the scene as dirty
  this->Scene->SetDirty(true);

  this->InvokeEvent(vtkCommand::InteractionEvent);
  return true;
}

//-----------------------------------------------------------------------------
bool vtkChartXYZ::KeyPressEvent(const vtkContextKeyEvent &key)
{
  switch (key.GetKeyCode())
  {
    // Change view to 2D, YZ chart
    case 'x':
      this->LookDownX();
      break;
    case 'X':
      this->LookUpX();
      break;
    // Change view to 2D, XZ chart
    case 'y':
      this->LookDownY();
      break;
    case 'Y':
      this->LookUpY();
      break;
    // Change view to 2D, XY chart
    case 'z':
      this->LookDownZ();
      break;
    case 'Z':
      this->LookUpZ();
      break;
  }

  return true;
}

//-----------------------------------------------------------------------------
void vtkChartXYZ::LookDownX()
{
  this->InvokeEvent(vtkCommand::InteractionEvent);
  this->Rotation->Identity();
  this->Rotation->RotateY(-90.0);
  this->Scene->SetDirty(true);
}

//-----------------------------------------------------------------------------
void vtkChartXYZ::LookDownY()
{
  this->Rotation->Identity();
  this->Rotation->RotateX(90.0);
  this->InvokeEvent(vtkCommand::InteractionEvent);
  this->Scene->SetDirty(true);
}

//-----------------------------------------------------------------------------
void vtkChartXYZ::LookDownZ()
{
  this->Rotation->Identity();
  this->InvokeEvent(vtkCommand::InteractionEvent);
  this->Scene->SetDirty(true);
}

//-----------------------------------------------------------------------------
void vtkChartXYZ::LookUpX()
{
  this->InvokeEvent(vtkCommand::InteractionEvent);
  this->Rotation->Identity();
  this->Rotation->RotateY(90.0);
  this->Scene->SetDirty(true);
}

//-----------------------------------------------------------------------------
void vtkChartXYZ::LookUpY()
{
  this->Rotation->Identity();
  this->Rotation->RotateX(-90.0);
  this->InvokeEvent(vtkCommand::InteractionEvent);
  this->Scene->SetDirty(true);
}

//-----------------------------------------------------------------------------
void vtkChartXYZ::LookUpZ()
{
  this->Rotation->Identity();
  this->Rotation->RotateY(180.0);
  this->InvokeEvent(vtkCommand::InteractionEvent);
  this->Scene->SetDirty(true);
}

//-----------------------------------------------------------------------------
void vtkChartXYZ::CalculateTransforms()
{
  // Calculate the correct translation vector so that rotation and scale
  // are applied about the middle of the axes box.
  vtkVector3f translation(
    (this->Axes[0]->GetPosition2()[0] - this->Axes[0]->GetPosition1()[0]) / 2.0
      + this->Axes[0]->GetPosition1()[0],
    (this->Axes[1]->GetPosition2()[1] - this->Axes[1]->GetPosition1()[1]) / 2.0
      + this->Axes[1]->GetPosition1()[1],
    (this->Axes[2]->GetPosition2()[1] - this->Axes[2]->GetPosition1()[1]) / 2.0
      + this->Axes[2]->GetPosition1()[1]);
  vtkVector3f mtranslation = -1.0 * translation;

  this->ContextTransform->Identity();
  this->ContextTransform->Concatenate(this->Translation.GetPointer());
  this->ContextTransform->Translate(translation.GetData());
  this->ContextTransform->Concatenate(this->Rotation.GetPointer());
  this->ContextTransform->Concatenate(this->BoxScale.GetPointer());
  if (this->AutoRotate)
  {
    if (this->IsX)
    {
      this->ContextTransform->RotateX(this->Angle);
    }
    else
    {
      this->ContextTransform->RotateY(this->Angle);
    }
  }
  this->ContextTransform->Concatenate(this->Scale.GetPointer());
  this->ContextTransform->Translate(mtranslation.GetData());
  this->ContextTransform->Translate(
    this->Axes[0]->GetPosition1()[0] - this->Geometry.GetX(),
    this->Axes[1]->GetPosition1()[1] - this->Geometry.GetY(),
    this->Axes[2]->GetPosition1()[1]);
  this->ContextTransform->Concatenate(this->PlotTransform.GetPointer());

  // Next construct the transform for the box axes.
  double scale[3] = { 300, 300, 300 };
  for (int i = 0; i < 3; ++i)
  {
    if (i == 0)
    {
      scale[i] = this->Axes[i]->GetPosition2()[0] -
                 this->Axes[i]->GetPosition1()[0];
    }
    else
    {
      scale[i] = this->Axes[i]->GetPosition2()[1] -
                 this->Axes[i]->GetPosition1()[1];
    }
  }

  this->Box->Identity();
  this->Box->PostMultiply();
  this->Box->Translate(-0.5, -0.5, -0.5);
  this->Box->Concatenate(this->Rotation.GetPointer());
  this->Box->Concatenate(this->BoxScale.GetPointer());
  if (this->AutoRotate)
  {
    if (this->IsX)
    {
      this->Box->RotateX(this->Angle);
    }
    else
    {
      this->Box->RotateY(this->Angle);
    }
  }
  this->Box->Translate(0.5, 0.5, 0.5);
  this->Box->Scale(scale);
  this->Box->Translate(Axes[0]->GetPosition1()[0],
                       Axes[1]->GetPosition1()[1],
                       Axes[2]->GetPosition1()[1]);

  // setup clipping planes
  this->BoundingCube->RemoveAllItems();
  vtkVector3d cube[8];
  vtkVector3d transformedCube[8];

  cube[0] = vtkVector3d(0, 0, 0);
  cube[1] = vtkVector3d(0, 0, 1);
  cube[2] = vtkVector3d(0, 1, 0);
  cube[3] = vtkVector3d(0, 1, 1);
  cube[4] = vtkVector3d(1, 0, 0);
  cube[5] = vtkVector3d(1, 0, 1);
  cube[6] = vtkVector3d(1, 1, 0);
  cube[7] = vtkVector3d(1, 1, 1);

  for (int i = 0; i < 8; ++i)
  {
    this->Box->TransformPoint(cube[i].GetData(), transformedCube[i].GetData());
  }

  double norm1[3];
  double norm2[3];
  double norm3[3];
  double norm4[3];
  double norm5[3];
  double norm6[3];

  //face 0,1,2,3 opposes face 4,5,6,7
  vtkNew<vtkPlane> face1;
  vtkMath::Cross((transformedCube[2] - transformedCube[0]).GetData(),
                 (transformedCube[1] - transformedCube[0]).GetData(), norm1);
  vtkMath::Normalize(norm1);
  face1->SetNormal(norm1);
  face1->SetOrigin(transformedCube[3].GetData());
  this->BoundingCube->AddItem(face1.GetPointer());

  vtkNew<vtkPlane> face2;
  vtkMath::Cross((transformedCube[5] - transformedCube[4]).GetData(),
                 (transformedCube[6] - transformedCube[4]).GetData(), norm2);
  vtkMath::Normalize(norm2);
  face2->SetNormal(norm2);
  face2->SetOrigin(transformedCube[7].GetData());
  this->BoundingCube->AddItem(face2.GetPointer());

  //face 0,1,4,5 opposes face 2,3,6,7
  vtkNew<vtkPlane> face3;
  vtkMath::Cross((transformedCube[1] - transformedCube[0]).GetData(),
                 (transformedCube[4] - transformedCube[0]).GetData(), norm3);
  vtkMath::Normalize(norm3);
  face3->SetNormal(norm3);
  face3->SetOrigin(transformedCube[5].GetData());
  this->BoundingCube->AddItem(face3.GetPointer());

  vtkNew<vtkPlane> face4;
  vtkMath::Cross((transformedCube[6] - transformedCube[2]).GetData(),
                 (transformedCube[3] - transformedCube[2]).GetData(), norm4);
  vtkMath::Normalize(norm4);
  face4->SetNormal(norm4);
  face4->SetOrigin(transformedCube[7].GetData());
  this->BoundingCube->AddItem(face4.GetPointer());

  //face 0,2,4,6 opposes face 1,3,5,7
  vtkNew<vtkPlane> face5;
  vtkMath::Cross((transformedCube[4] - transformedCube[0]).GetData(),
                 (transformedCube[2] - transformedCube[0]).GetData(), norm5);
  vtkMath::Normalize(norm5);
  face5->SetNormal(norm5);
  face5->SetOrigin(transformedCube[6].GetData());
  this->BoundingCube->AddItem(face5.GetPointer());

  vtkNew<vtkPlane> face6;
  vtkMath::Cross((transformedCube[3] - transformedCube[1]).GetData(),
                 (transformedCube[5] - transformedCube[1]).GetData(), norm6);
  vtkMath::Normalize(norm6);
  face6->SetNormal(norm6);
  face6->SetOrigin(transformedCube[7].GetData());
  this->BoundingCube->AddItem(face6.GetPointer());
}

//-----------------------------------------------------------------------------
void vtkChartXYZ::ScaleUpAxes()
{
  float point[3];
  int sceneWidth = this->Scene->GetSceneWidth();
  int sceneHeight = this->Scene->GetSceneHeight();
  float scaleStep = pow(2.0f, 1.0f/10.0f);
  float stepBack = pow(2.0f, -1.0f/10.0f);
  int numSteps = 0;
  bool shouldScaleUp = true;

  while (shouldScaleUp)
  {
    for (int i = 0; i < 14; ++i)
    {
      point[0] = this->AxesBoundaryPoints[i][0];
      point[1] = this->AxesBoundaryPoints[i][1];
      point[2] = this->AxesBoundaryPoints[i][2];
      this->FutureBox->TransformPoint(point, point);
      if (point[0] < 0 || point[0] > sceneWidth ||
          point[1] < 0 || point[1] > sceneHeight)
      {
        shouldScaleUp = false;
      }
    }
    if (shouldScaleUp)
    {
      this->FutureBoxScale->Scale(scaleStep, scaleStep, scaleStep);
      ++numSteps;
      if (numSteps > 500)
      {
        // Break out of the loop.
        shouldScaleUp = false;
      }
    }
  }
  // this while loop overshoots the mark by one step,
  // so we take a step back afterwards.
  this->FutureBoxScale->Scale(stepBack, stepBack, stepBack);

  if (numSteps > 1)
  {
      this->ZoomAxes(numSteps - 1);
      this->Scene->SetDirty(true);
  }
}

//-----------------------------------------------------------------------------
void vtkChartXYZ::ScaleDownAxes()
{
  float point[3];
  int sceneWidth = this->Scene->GetSceneWidth();
  int sceneHeight = this->Scene->GetSceneHeight();

  float scaleStep = pow(2.0f, -1.0f/10.0f);
  int numSteps = 0;
  bool shouldScaleDown = true;

  while (shouldScaleDown)
  {
    shouldScaleDown = false;
    for (int i = 0; i < 14; ++i)
    {
      point[0] = this->AxesBoundaryPoints[i][0];
      point[1] = this->AxesBoundaryPoints[i][1];
      point[2] = this->AxesBoundaryPoints[i][2];
      this->FutureBox->TransformPoint(point, point);
      if (point[0] < 0 || point[0] > sceneWidth ||
          point[1] < 0 || point[1] > sceneHeight)
      {
        shouldScaleDown = true;
        break;
      }
    }
    if (shouldScaleDown)
    {
      this->FutureBoxScale->Scale(scaleStep, scaleStep, scaleStep);
      ++numSteps;
      if (numSteps > 500)
      {
        // Break out of the loop.
        shouldScaleDown = false;
      }
    }
  }
  if (numSteps > 0)
  {
      this->ZoomAxes(-numSteps);
      this->Scene->SetDirty(true);
  }
}

//-----------------------------------------------------------------------------
void vtkChartXYZ::InitializeFutureBox()
{
  double scale[3] = { 300, 300, 300 };
  for (int i = 0; i < 3; ++i)
  {
    if (i == 0)
      scale[i] = this->Axes[i]->GetPosition2()[0] -
                 this->Axes[i]->GetPosition1()[0];
    else
      scale[i] = this->Axes[i]->GetPosition2()[1] -
                 this->Axes[i]->GetPosition1()[1];
  }

  this->FutureBoxScale->DeepCopy(this->BoxScale.GetPointer());

  this->FutureBox->Identity();
  this->FutureBox->PostMultiply();
  this->FutureBox->Translate(-0.5, -0.5, -0.5);
  this->FutureBox->Concatenate(this->Rotation.GetPointer());
  this->FutureBox->Concatenate(this->FutureBoxScale.GetPointer());
  this->FutureBox->Translate(0.5, 0.5, 0.5);
  this->FutureBox->Scale(scale);
  this->FutureBox->Translate(this->Axes[0]->GetPosition1()[0],
                             this->Axes[1]->GetPosition1()[1],
                             this->Axes[2]->GetPosition1()[1]);
}

//-----------------------------------------------------------------------------
bool vtkChartXYZ::CheckForSceneResize()
{
  int currentWidth = this->Scene->GetSceneWidth();
  int currentHeight = this->Scene->GetSceneHeight();
  if (this->SceneWidth != currentWidth ||
      this->SceneHeight != currentHeight)
  {
    // treat the initial render as a special case, as the scene size
    // has not been recorded yet
    if (this->SceneWidth > 0)
    {
      int dx = (currentWidth - this->SceneWidth) / 2;
      int dy = (currentHeight - this->SceneHeight) / 2;

      vtkVector2f axisPt = this->Axes[0]->GetPosition1();
      axisPt[0] += dx;
      axisPt[1] += dy;
      this->Axes[0]->SetPoint1(axisPt);
      axisPt = this->Axes[0]->GetPosition2();
      axisPt[0] += dx;
      axisPt[1] += dy;
      this->Axes[0]->SetPoint2(axisPt);
      axisPt = this->Axes[1]->GetPosition1();
      axisPt[0] += dx;
      axisPt[1] += dy;
      this->Axes[1]->SetPoint1(axisPt);
      axisPt = this->Axes[1]->GetPosition2();
      axisPt[0] += dx;
      axisPt[1] += dy;
      this->Axes[1]->SetPoint2(axisPt);
      axisPt = this->Axes[2]->GetPosition1();
      axisPt[0] += dx;
      this->Axes[2]->SetPoint1(axisPt);
      axisPt = this->Axes[2]->GetPosition2();
      axisPt[0] += dx;
      this->Axes[2]->SetPoint2(axisPt);
      this->RecalculateTransform();
    }
    else
    {
      this->SceneWidth = currentWidth;
      this->SceneHeight = currentHeight;
      this->InitializeFutureBox();
      this->ScaleUpAxes();
      this->ScaleDownAxes();
    }
    return true;
  }
  return false;
}

//-----------------------------------------------------------------------------
void vtkChartXYZ::RescaleAxes()
{
  int currentWidth = this->Scene->GetSceneWidth();
  int currentHeight = this->Scene->GetSceneHeight();
  this->InitializeFutureBox();
  if (currentWidth * currentHeight < this->SceneWidth * this->SceneHeight)
  {
    this->ScaleDownAxes();
  }
  else
  {
    this->ScaleUpAxes();
  }
  this->SceneWidth = currentWidth;
  this->SceneHeight = currentHeight;
}

//-----------------------------------------------------------------------------
void vtkChartXYZ::InitializeAxesBoundaryPoints()
{
  int currentPoint = 0;
  for (int i = 0; i < 2; ++i)
  {
    for (int j = 0; j < 2; ++j)
    {
      for (int k = 0; k < 2; ++k)
      {
        this->AxesBoundaryPoints[currentPoint][0] = i;
        this->AxesBoundaryPoints[currentPoint][1] = j;
        this->AxesBoundaryPoints[currentPoint][2] = k;
        ++currentPoint;
      }
    }
  }

  for (int i = 0; i < 3; ++i)
  {
    this->AxesBoundaryPoints[currentPoint][0] = 0.5;
    this->AxesBoundaryPoints[currentPoint][1] = 0.5;
    this->AxesBoundaryPoints[currentPoint][2] = 0.5;
    this->AxesBoundaryPoints[currentPoint][i] += sqrt(0.75);
    ++currentPoint;
    this->AxesBoundaryPoints[currentPoint][0] = 0.5;
    this->AxesBoundaryPoints[currentPoint][1] = 0.5;
    this->AxesBoundaryPoints[currentPoint][2] = 0.5;
    this->AxesBoundaryPoints[currentPoint][i] -= sqrt(0.75);
    ++currentPoint;
  }
}

//-----------------------------------------------------------------------------
double vtkChartXYZ::CalculateNiceMinMax(double &min, double &max,
                                                   int axis)
{
  // Calculate an upper limit on the number of tick marks - at least 30 pixels
  // should be between each tick mark.
  float start[3] = { 0, 0, 0 };
  float end[3] = { 0, 0, 0 };
  end[axis] = 1;

  this->Box->TransformPoint(start, start);
  this->Box->TransformPoint(end, end);

  float pixelRange = sqrt(
    (end[0] - start[0]) * (end[0] - start[0]) +
    (end[1] - start[1]) * (end[1] - start[1]));

  return vtkAxis::NiceMinMax(min, max, pixelRange, 30.0f);
}

//-----------------------------------------------------------------------------
void vtkChartXYZ::RecalculateTransform()
{
  this->CalculatePlotTransform(this->Axes[0].GetPointer(),
                               this->Axes[1].GetPointer(),
                               this->Axes[2].GetPointer(),
                               this->PlotTransform.GetPointer());
}

//-----------------------------------------------------------------------------
bool vtkChartXYZ::CalculatePlotTransform(vtkAxis *x, vtkAxis *y, vtkAxis *z,
                                         vtkTransform *transform)
{
  // Need to calculate the 3D transform this time.
  assert(x && y && z && transform);

  // Get the scale for the plot area from the x and y axes
  float *min = x->GetPoint1();
  float *max = x->GetPoint2();
  if (fabs(max[0] - min[0]) == 0.0f)
  {
    return false;
  }
  float xScale =
    (x->GetUnscaledMaximum() - x->GetUnscaledMinimum()) / (max[0] - min[0]);

  // Now the y axis
  min = y->GetPoint1();
  max = y->GetPoint2();
  if (fabs(max[1] - min[1]) == 0.0f)
  {
    return false;
  }
  float yScale =
    (y->GetUnscaledMaximum() - y->GetUnscaledMinimum()) / (max[1] - min[1]);

  // Now the z axis
  min = z->GetPoint1();
  max = z->GetPoint2();
  if (fabs(max[1] - min[1]) == 0.0f)
  {
    return false;
  }
  float zScale =
    (z->GetUnscaledMaximum() - z->GetUnscaledMinimum()) / (max[1] - min[1]);

  transform->Identity();
  transform->Translate(this->Geometry.GetX(), this->Geometry.GetY(), 0);
  // Get the scale for the plot area from the x and y axes
  transform->Scale(1.0 / xScale, 1.0 / yScale, 1.0 / zScale);
  transform->Translate(
    -x->GetUnscaledMinimum(),
    -y->GetUnscaledMinimum(),
    -z->GetUnscaledMinimum());

  return true;
}

//-----------------------------------------------------------------------------
vtkIdType vtkChartXYZ::AddPlot(vtkPlot3D * plot)
{
  if (plot == NULL)
  {
    return -1;
  }
  this->AddItem(plot);
  plot->SetChart(this);
  this->Plots.push_back(plot);
  vtkIdType plotIndex = this->Plots.size() - 1;

  // the first plot added to the chart defines the names of the axes
  if (plotIndex == 0)
  {
    this->XAxisLabel = plot->GetXAxisLabel();
    this->YAxisLabel = plot->GetYAxisLabel();
    this->ZAxisLabel = plot->GetZAxisLabel();
  }

  this->RecalculateBounds();

  // Mark the scene as dirty
  if (this->Scene)
  {
    this->Scene->SetDirty(true);
  }
  return plotIndex;
}

//-----------------------------------------------------------------------------
void vtkChartXYZ::ClearPlots()
{
  this->ClearItems();
  this->Plots.clear();
}

//-----------------------------------------------------------------------------
void vtkChartXYZ::SetFitToScene(bool b)
{
  this->FitToScene = b;
}

//-----------------------------------------------------------------------------
void vtkChartXYZ::GetClippingPlaneEquation(int i, double *planeEquation)
{
  int n = this->BoundingCube->GetNumberOfItems();
  if (i >= 0 && i < n)
  {
    // Get the plane
    vtkPlane *plane = this->BoundingCube->GetItem(i);
    double *normal = plane->GetNormal();
    double *origin = plane->GetOrigin();

    // Compute the plane equation
    planeEquation[0] = normal[0];
    planeEquation[1] = normal[1];
    planeEquation[2] = normal[2];
    planeEquation[3] = -(normal[0] * origin[0] +
                         normal[1] * origin[1] +
                         normal[2] * origin[2]);
  }
}
