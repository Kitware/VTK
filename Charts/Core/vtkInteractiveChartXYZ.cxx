/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInteractiveChartXYZ.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkInteractiveChartXYZ.h"

#include "vtkAnnotationLink.h"
#include "vtkAxis.h"
#include "vtkCommand.h"
#include "vtkContext2D.h"
#include "vtkContext3D.h"
#include "vtkContextKeyEvent.h"
#include "vtkContextMouseEvent.h"
#include "vtkContextScene.h"
#include "vtkIdTypeArray.h"
#include "vtkLookupTable.h"
#include "vtkMath.h"
#include "vtkPen.h"
#include "vtkPlane.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkTable.h"
#include "vtkTextProperty.h"
#include "vtkTransform.h"
#include "vtkVector.h"
#include "vtkVectorOperators.h"

#include "vtkObjectFactory.h"

#include <vector>
#include <cassert>

using std::vector;

vtkStandardNewMacro(vtkInteractiveChartXYZ)

vtkInteractiveChartXYZ::vtkInteractiveChartXYZ()
{
  this->Translation->Identity();
  this->Translation->PostMultiply();
  this->Scale->Identity();
  this->Scale->PostMultiply();
  this->Interactive = true;
  this->NumberOfComponents = 0;
  this->InitializeAxesBoundaryPoints();
}

vtkInteractiveChartXYZ::~vtkInteractiveChartXYZ()
{
}

void vtkInteractiveChartXYZ::PrintSelf(ostream &os, vtkIndent indent)
{
  Superclass::PrintSelf(os, indent);
}

void vtkInteractiveChartXYZ::Update()
{
  if (this->Link)
    {
    // Copy the row numbers so that we can do the highlight...
    if (!this->points.empty())
      {
      vtkSelection *selection =
          vtkSelection::SafeDownCast(this->Link->GetOutputDataObject(2));
      if (selection->GetNumberOfNodes())
        {
        vtkSelectionNode *node = selection->GetNode(0);
        vtkIdTypeArray *idArray =
            vtkIdTypeArray::SafeDownCast(node->GetSelectionList());
        if (this->selectedPointsBuidTime > idArray->GetMTime() ||
            this->GetMTime() > this->selectedPointsBuidTime)
          {
          this->selectedPoints.resize(idArray->GetNumberOfTuples());
          for (vtkIdType i = 0; i < idArray->GetNumberOfTuples(); ++i)
            {
            this->selectedPoints[i] = this->points[idArray->GetValue(i)];
            }
          this->selectedPointsBuidTime.Modified();
          }
        }
      }
    }
}

bool vtkInteractiveChartXYZ::Paint(vtkContext2D *painter)
{
  if (!this->Visible || this->points.size() == 0)
    return false;

  // Get the 3D context.
  vtkContext3D *context = painter->GetContext3D();

  if (!context)
    return false;

  this->Update();

  // Check if the scene changed size
  bool resizeHappened = this->CheckForSceneResize();

  // Calculate the transforms required for the current rotation.
  this->CalculateTransforms();

  // Update the points that fall inside our axes
  this->UpdateClippedPoints();
  if (this->clipped_points.size() > 0)
    {
    context->PushMatrix();
    context->AppendTransform(this->ContextTransform.GetPointer());

    this->ComputeDataBounds();

    // First lets draw the points in 3d.
    context->ApplyPen(this->Pen.GetPointer());
    if (this->NumberOfComponents == 0)
      {
      context->DrawPoints(this->clipped_points[0].GetData(), this->clipped_points.size());
      }
    else
      {
      context->DrawPoints(this->clipped_points[0].GetData(), this->clipped_points.size(),
                          this->ClippedColors->GetPointer(0), this->NumberOfComponents);
      }

    // Now to render the selected points.
    if (!this->selectedPoints.empty())
      {
      context->ApplyPen(this->SelectedPen.GetPointer());
      context->DrawPoints(this->selectedPoints[0].GetData(), this->selectedPoints.size());
      }
    context->PopMatrix();
    }

  // Now to draw the axes - pretty basic for now but could be extended.
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

  this->DetermineWhichAxesToLabel();
  this->DrawTickMarks(context);
  this->DrawAxesLabels(painter);

  // If necessary, rescale the axes so they fits our scene nicely
  if (resizeHappened)
    {
    this->RescaleAxes();
    }

  return true;
}

void vtkInteractiveChartXYZ::UpdateClippedPoints()
{
  this->clipped_points.clear();
  this->ClippedColors->Reset();
  for( size_t i = 0; i < this->points.size(); ++i )
    {
    const unsigned char rgb[3] =
      {
      this->Colors->GetValue(i * this->NumberOfComponents),
      this->Colors->GetValue(i * this->NumberOfComponents + 1),
      this->Colors->GetValue(i * this->NumberOfComponents + 2)
      };
    if( !this->PointShouldBeClipped(this->points[i]) )
      {
      this->clipped_points.push_back(this->points[i]);
      this->ClippedColors->InsertNextTupleValue(&rgb[0]);
      this->ClippedColors->InsertNextTupleValue(&rgb[1]);
      this->ClippedColors->InsertNextTupleValue(&rgb[2]);
      }
    }
}

void vtkInteractiveChartXYZ::ComputeDataBounds()
{
  double xMin = DBL_MAX;
  double xMax = DBL_MIN;
  double yMin = DBL_MAX;
  double yMax = DBL_MIN;
  float transformedPoint[3];

  for (int i = 0; i < this->clipped_points.size(); ++i)
    {
    this->ContextTransform->TransformPoint(
      this->clipped_points[i].GetData(), transformedPoint);

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

  this->DataBounds[0] = xMin;
  this->DataBounds[1] = yMin;
  this->DataBounds[2] = xMax;
  this->DataBounds[3] = yMax;
}

void vtkInteractiveChartXYZ::DrawAxesLabels(vtkContext2D *painter)
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
    xLabelPos[0] += offset[0];
    xLabelPos[1] += offset[1];
    painter->DrawString(xLabelPos[0], xLabelPos[1], this->XAxisLabel);
    }

  if (shouldDrawAxis[1])
    {
    painter->ComputeStringBounds(this->YAxisLabel, bounds);
    offset[0] = 0;
    offset[1] = 0;
    this->GetOffsetForAxisLabel(1, bounds, offset);
    yLabelPos[0] += offset[0];
    yLabelPos[1] += offset[1];
    painter->DrawString(yLabelPos[0], yLabelPos[1], this->YAxisLabel);
    }

  if (shouldDrawAxis[2])
    {
    painter->ComputeStringBounds(this->ZAxisLabel, bounds);
    offset[0] = 0;
    offset[1] = 0;
    this->GetOffsetForAxisLabel(2, bounds, offset);
    zLabelPos[0] += offset[0];
    zLabelPos[1] += offset[1];
    painter->DrawString(zLabelPos[0], zLabelPos[1], this->ZAxisLabel);
    }
}

void vtkInteractiveChartXYZ::GetOffsetForAxisLabel(int axis, float *bounds, float *offset)
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

void vtkInteractiveChartXYZ::DrawTickMarks(vtkContext3D *context)
{
  //for now, tick marks will be drawn as black points
  context->ApplyPen(this->Pen.GetPointer());

  // treat each axis separately
  for (int axis = 0; axis < 3; ++axis)
    {
    // figure out how many tick marks can fit when we use a minimum spacing of
    // 30 pixels
    float start[3] = { 0, 0, 0 };
    float end[3] = { 0, 0, 0 };
    end[axis] = 1;
    this->Box->TransformPoint(start, start);
    this->Box->TransformPoint(end, end);
    float axisLength = sqrt(
      (end[0] - start[0]) * (end[0] - start[0]) +
      (end[1] - start[1]) * (end[1] - start[1]));
    int numTicks = 1;
    float currentSpacing = axisLength;
    float minimumSpacing = 30;
    while (currentSpacing > minimumSpacing)
    {
      ++numTicks;
      currentSpacing = axisLength / numTicks;
    }
    --numTicks;

    std::vector < vtkVector3f > tickPoints;
    for (int i = 0; i <= numTicks; ++i)
      {
      float offset = (float)i / (float)numTicks;
      vtkVector3f tick;
      switch (axis)
        {
        case 0:
          tick[0] = offset;
          tick[1] = this->XAxisToLabel[0];
          tick[2] = this->XAxisToLabel[1];
          break;
        case 1:
          tick[0] = this->YAxisToLabel[0];
          tick[1] = offset;
          tick[2] = this->YAxisToLabel[1];
          break;
        case 2:
        default:
          tick[0] = this->ZAxisToLabel[0];
          tick[1] = this->ZAxisToLabel[1];
          tick[2] = offset;
          break;
        }
      tickPoints.push_back(tick);
      }
    context->DrawPoints(tickPoints[0].GetData(), tickPoints.size());
    }

  //revert from drawing points.
  context->ApplyPen(this->AxisPen.GetPointer());
}

void vtkInteractiveChartXYZ::DetermineWhichAxesToLabel()
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
            double d1 = abs(midpoint[1] - this->DataBounds[1]);
            double d2 = abs(midpoint[1] - this->DataBounds[3]);
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
            double d1 = abs(midpoint[0] - this->DataBounds[0]);
            double d2 = abs(midpoint[0] - this->DataBounds[2]);
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
            else if (start[1] < this->DataBounds[1] && end[1] > this->DataBounds[3])
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
                this->XAxisToLabel[0] = j;
                this->XAxisToLabel[1] = k;
                break;
              case 1:
                this->YAxisToLabel[0] = i;
                this->YAxisToLabel[1] = k;
                break;
              case 2:
              default:
                this->ZAxisToLabel[0] = i;
                this->ZAxisToLabel[1] = j;
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

void vtkInteractiveChartXYZ::SetInput(vtkTable *input, const vtkStdString &xName,
                           const vtkStdString &yName, const vtkStdString &zName)
{
  this->Superclass::SetInput(input, xName, yName, zName);
  this->XAxisLabel = xName;
  this->YAxisLabel = yName;
  this->ZAxisLabel = zName;
}

void vtkInteractiveChartXYZ::SetInput(vtkTable *input, const vtkStdString &xName,
                           const vtkStdString &yName, const vtkStdString &zName,
                           const vtkStdString &colorName)
{
  this->Superclass::SetInput(input, xName, yName, zName);
  this->XAxisLabel = xName;
  this->YAxisLabel = yName;
  this->ZAxisLabel = zName;

  vtkDataArray *colorArr =
      vtkDataArray::SafeDownCast(input->GetColumnByName(colorName.c_str()));

  assert(colorArr);
  assert(colorArr->GetNumberOfTuples() == this->points.size());

  this->NumberOfComponents = 3;

  //generate a color lookup table
  vtkNew<vtkLookupTable> lookupTable;
  double min = DBL_MAX;
  double max = DBL_MIN;
  for (int i = 0; i < this->points.size(); ++i)
    {
    double value = colorArr->GetComponent(i, 0);
    if (value > max)
      {
      max = value;
      }
    else if (value < min)
      {
      min = value;
      }
    }

  lookupTable->SetNumberOfTableValues(256);
  lookupTable->SetRange(min, max);
  lookupTable->Build();

  double color[3];

  for (int i = 0; i < this->points.size(); ++i)
    {
    double value = colorArr->GetComponent(i, 0);
    unsigned char *rgb = lookupTable->MapValue(value);
    const unsigned char constRGB[3] = { rgb[0], rgb[1], rgb[2] };
    this->Colors->InsertNextTupleValue(&constRGB[0]);
    this->Colors->InsertNextTupleValue(&constRGB[1]);
    this->Colors->InsertNextTupleValue(&constRGB[2]);
    }
}

void vtkInteractiveChartXYZ::SetScene(vtkContextScene *scene)
{
  this->Superclass::SetScene(scene);
  this->SceneWidth = this->Scene->GetSceneWidth();
  this->SceneHeight = this->Scene->GetSceneHeight();
}


//-----------------------------------------------------------------------------
bool vtkInteractiveChartXYZ::Hit(const vtkContextMouseEvent &vtkNotUsed(mouse))
{
  // If we are interactive, we want to catch anything that propagates to the
  // background, otherwise we do not want any mouse events.
  return this->Interactive;
}

//-----------------------------------------------------------------------------
bool vtkInteractiveChartXYZ::MouseButtonPressEvent(const vtkContextMouseEvent &mouse)
{
  if (mouse.GetButton() == vtkContextMouseEvent::LEFT_BUTTON)
    {
    return true;
    }
  return false;
}

//-----------------------------------------------------------------------------
bool vtkInteractiveChartXYZ::MouseMoveEvent(const vtkContextMouseEvent &mouse)
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
bool vtkInteractiveChartXYZ::MouseWheelEvent(const vtkContextMouseEvent &mouse,
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
void vtkInteractiveChartXYZ::ZoomAxes(int delta)
{
  float scaling = pow(2.0f, delta/10.0f);
  this->BoxScale->Scale(scaling, scaling, scaling);

  // Mark the scene as dirty
  this->Scene->SetDirty(true);
}

//-----------------------------------------------------------------------------
bool vtkInteractiveChartXYZ::Rotate(const vtkContextMouseEvent &mouse)
{
  // Figure out how much the mouse has moved in plot coordinates
  vtkVector2d screenPos(mouse.GetScreenPos().Cast<double>().GetData());
  vtkVector2d lastScreenPos(mouse.GetLastScreenPos().Cast<double>().GetData());

  double dx = screenPos[0] - lastScreenPos[0];
  double dy = screenPos[1] - lastScreenPos[1];

  double delta_elevation = -20.0 / this->Scene->GetSceneHeight();
  double delta_azimuth = -20.0 / this->Scene->GetSceneWidth();

  double rxf = dx * delta_azimuth * 10.0;
  double ryf = dy * delta_elevation * 10.0;

  this->Rotation->RotateY(rxf);
  this->Rotation->RotateX(-ryf);

  // Mark the scene as dirty
  this->Scene->SetDirty(true);

  this->InvokeEvent(vtkCommand::InteractionEvent);
  return true;
}

//-----------------------------------------------------------------------------
bool vtkInteractiveChartXYZ::Pan(const vtkContextMouseEvent &mouse)
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
bool vtkInteractiveChartXYZ::Zoom(const vtkContextMouseEvent &mouse)
{
  // Figure out how much the mouse has moved and scale accordingly
  vtkVector2d screenPos(mouse.GetScreenPos().Cast<double>().GetData());
  vtkVector2d lastScreenPos(mouse.GetLastScreenPos().Cast<double>().GetData());

  float delta = 0.0f;
  if (this->Scene->GetSceneHeight() > 0)
    {
    delta = static_cast<float>(mouse.GetLastScreenPos()[1] - mouse.GetScreenPos()[1])/this->Scene->GetSceneHeight();
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
bool vtkInteractiveChartXYZ::Spin(const vtkContextMouseEvent &mouse)
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
bool vtkInteractiveChartXYZ::KeyPressEvent(const vtkContextKeyEvent &key)
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
void vtkInteractiveChartXYZ::LookDownX()
{
  this->InvokeEvent(vtkCommand::InteractionEvent);
  this->Rotation->Identity();
  this->Rotation->RotateY(90.0);
  this->Scene->SetDirty(true);
}

//-----------------------------------------------------------------------------
void vtkInteractiveChartXYZ::LookDownY()
{
  this->Rotation->Identity();
  this->Rotation->RotateX(90.0);
  this->InvokeEvent(vtkCommand::InteractionEvent);
  this->Scene->SetDirty(true);
}

//-----------------------------------------------------------------------------
void vtkInteractiveChartXYZ::LookDownZ()
{
  this->Rotation->Identity();
  this->InvokeEvent(vtkCommand::InteractionEvent);
  this->Scene->SetDirty(true);
}

//-----------------------------------------------------------------------------
void vtkInteractiveChartXYZ::LookUpX()
{
  this->InvokeEvent(vtkCommand::InteractionEvent);
  this->Rotation->Identity();
  this->Rotation->RotateY(-90.0);
  this->Scene->SetDirty(true);
}

//-----------------------------------------------------------------------------
void vtkInteractiveChartXYZ::LookUpY()
{
  this->Rotation->Identity();
  this->Rotation->RotateX(-90.0);
  this->InvokeEvent(vtkCommand::InteractionEvent);
  this->Scene->SetDirty(true);
}

//-----------------------------------------------------------------------------
void vtkInteractiveChartXYZ::LookUpZ()
{
  this->Rotation->Identity();
  this->Rotation->RotateZ(180.0);
  this->InvokeEvent(vtkCommand::InteractionEvent);
  this->Scene->SetDirty(true);
}

void vtkInteractiveChartXYZ::CalculateTransforms()
{
  // Calculate the correct translation vector so that rotation and scale
  // are applied about the middle of the axes box.
  vtkVector3f translation(
        (axes[0]->GetPosition2()[0] - axes[0]->GetPosition1()[0]) / 2.0
        + axes[0]->GetPosition1()[0],
        (axes[1]->GetPosition2()[1] - axes[1]->GetPosition1()[1]) / 2.0
        + axes[1]->GetPosition1()[1],
        (axes[2]->GetPosition2()[1] - axes[2]->GetPosition1()[1]) / 2.0
        + axes[2]->GetPosition1()[1]);
  vtkVector3f mtranslation = -1.0 * translation;

  this->ContextTransform->Identity();
  this->ContextTransform->Concatenate(this->Translation.GetPointer());
  this->ContextTransform->Translate(translation.GetData());
  this->ContextTransform->Concatenate(this->Rotation.GetPointer());
  this->ContextTransform->Concatenate(this->BoxScale.GetPointer());
  this->ContextTransform->Concatenate(this->Scale.GetPointer());
  this->ContextTransform->Translate(mtranslation.GetData());
  this->ContextTransform->Translate(axes[0]->GetPosition1()[0] - this->Geometry.X(),
                                    axes[1]->GetPosition1()[1] - this->Geometry.Y(),
                                    axes[2]->GetPosition1()[1]);
  this->ContextTransform->Concatenate(this->Transform.GetPointer());

  // Next construct the transform for the box axes.
  double scale[3] = { 300, 300, 300 };
  for (int i = 0; i < 3; ++i)
    {
    if (i == 0)
      scale[i] = axes[i]->GetPosition2()[0] - axes[i]->GetPosition1()[0];
    else
      scale[i] = axes[i]->GetPosition2()[1] - axes[i]->GetPosition1()[1];
    }

  this->Box->Identity();
  this->Box->PostMultiply();
  this->Box->Translate(-0.5, -0.5, -0.5);
  this->Box->Concatenate(this->Rotation.GetPointer());
  this->Box->Concatenate(this->BoxScale.GetPointer());
  this->Box->Translate(0.5, 0.5, 0.5);
  this->Box->Scale(scale);
  this->Box->Translate(axes[0]->GetPosition1()[0],
                       axes[1]->GetPosition1()[1],
                       axes[2]->GetPosition1()[1]);

  // setup clipping planes
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
  vtkMath::Cross((transformedCube[1] - transformedCube[0]).GetData(),
                 (transformedCube[2] - transformedCube[0]).GetData(), norm1);
  this->Face1->SetNormal(norm1);
  this->Face1->SetOrigin(transformedCube[3].GetData());

  vtkMath::Cross((transformedCube[5] - transformedCube[4]).GetData(),
                 (transformedCube[6] - transformedCube[4]).GetData(), norm2);
  this->Face2->SetNormal(norm2);
  this->Face2->SetOrigin(transformedCube[7].GetData());

  //face 0,1,4,5 opposes face 2,3,6,7
  vtkMath::Cross((transformedCube[1] - transformedCube[0]).GetData(),
                 (transformedCube[4] - transformedCube[0]).GetData(), norm3);
  this->Face3->SetNormal(norm3);
  this->Face3->SetOrigin(transformedCube[5].GetData());

  vtkMath::Cross((transformedCube[3] - transformedCube[2]).GetData(),
                 (transformedCube[6] - transformedCube[2]).GetData(), norm4);
  this->Face4->SetNormal(norm4);
  this->Face4->SetOrigin(transformedCube[7].GetData());

  //face 0,2,4,6 opposes face 1,3,5,7
  vtkMath::Cross((transformedCube[2] - transformedCube[0]).GetData(),
                 (transformedCube[4] - transformedCube[0]).GetData(), norm5);
  this->Face5->SetNormal(norm5);
  this->Face5->SetOrigin(transformedCube[6].GetData());

  vtkMath::Cross((transformedCube[3] - transformedCube[1]).GetData(),
                 (transformedCube[5] - transformedCube[1]).GetData(), norm6);
  this->Face6->SetNormal(norm6);
  this->Face6->SetOrigin(transformedCube[7].GetData());

  this->MaxDistance = this->Face1->DistanceToPlane(transformedCube[7].GetData());
}

bool vtkInteractiveChartXYZ::PointShouldBeClipped(vtkVector3f point)
{
  double pointD[3];
  pointD[0] = point.GetData()[0];
  pointD[1] = point.GetData()[1];
  pointD[2] = point.GetData()[2];

  double transformedPoint[3];
  this->ContextTransform->TransformPoint(pointD, transformedPoint);

  double d1 = this->Face1->DistanceToPlane(transformedPoint);
  double d2 = this->Face2->DistanceToPlane(transformedPoint);
  double d3 = this->Face3->DistanceToPlane(transformedPoint);
  double d4 = this->Face4->DistanceToPlane(transformedPoint);
  double d5 = this->Face5->DistanceToPlane(transformedPoint);
  double d6 = this->Face6->DistanceToPlane(transformedPoint);

  if (d1 > this->MaxDistance || d2 > this->MaxDistance ||
      d3 > this->MaxDistance || d4 > this->MaxDistance ||
      d5 > this->MaxDistance || d6 > this->MaxDistance)
    {
    return true;
    }
  return false;
}

void vtkInteractiveChartXYZ::ScaleUpAxes()
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

void vtkInteractiveChartXYZ::ScaleDownAxes()
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
      }
    }
  if (numSteps > 0)
    {
      this->ZoomAxes(-numSteps);
      this->Scene->SetDirty(true);
    }
}

void vtkInteractiveChartXYZ::InitializeFutureBox()
{
  double scale[3] = { 300, 300, 300 };
  for (int i = 0; i < 3; ++i)
    {
    if (i == 0)
      scale[i] = axes[i]->GetPosition2()[0] - axes[i]->GetPosition1()[0];
    else
      scale[i] = axes[i]->GetPosition2()[1] - axes[i]->GetPosition1()[1];
    }

  this->FutureBoxScale->DeepCopy(this->BoxScale.GetPointer());

  this->FutureBox->Identity();
  this->FutureBox->PostMultiply();
  this->FutureBox->Translate(-0.5, -0.5, -0.5);
  this->FutureBox->Concatenate(this->Rotation.GetPointer());
  this->FutureBox->Concatenate(this->FutureBoxScale.GetPointer());
  this->FutureBox->Translate(0.5, 0.5, 0.5);
  this->FutureBox->Scale(scale);
  this->FutureBox->Translate(axes[0]->GetPosition1()[0],
                             axes[1]->GetPosition1()[1],
                             axes[2]->GetPosition1()[1]);
}

bool vtkInteractiveChartXYZ::CheckForSceneResize()
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

      vtkVector2f axisPt = axes[0]->GetPosition1();
      axisPt[0] += dx;
      axisPt[1] += dy;
      axes[0]->SetPoint1(axisPt);
      axisPt = axes[0]->GetPosition2();
      axisPt[0] += dx;
      axisPt[1] += dy;
      axes[0]->SetPoint2(axisPt);
      axisPt = axes[1]->GetPosition1();
      axisPt[0] += dx;
      axisPt[1] += dy;
      axes[1]->SetPoint1(axisPt);
      axisPt = axes[1]->GetPosition2();
      axisPt[0] += dx;
      axisPt[1] += dy;
      axes[1]->SetPoint2(axisPt);
      axisPt = axes[2]->GetPosition1();
      axisPt[0] += dx;
      axes[2]->SetPoint1(axisPt);
      axisPt = axes[2]->GetPosition2();
      axisPt[0] += dx;
      axes[2]->SetPoint2(axisPt);
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

void vtkInteractiveChartXYZ::RescaleAxes()
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

void vtkInteractiveChartXYZ::InitializeAxesBoundaryPoints()
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
