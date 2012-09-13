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
#include "vtkContext2D.h"
#include "vtkContext3D.h"
#include "vtkContextScene.h"
#include "vtkTable.h"
#include "vtkFloatArray.h"
#include "vtkTransform.h"
#include "vtkVector.h"
#include "vtkVectorOperators.h"
#include "vtkPen.h"
#include "vtkAnnotationLink.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkIdTypeArray.h"

#include "vtkObjectFactory.h"

#include <vector>
#include <cassert>

using std::vector;

vtkStandardNewMacro(vtkChartXYZ)

void vtkChartXYZ::PrintSelf(ostream &os, vtkIndent indent)
{
  Superclass::PrintSelf(os, indent);
}

void vtkChartXYZ::Update()
{
  if (this->Link)
    {
    // Copy the row numbers so that we can do the highlight...
    if (!this->Points.empty())
      {
      vtkSelection *selection =
          vtkSelection::SafeDownCast(this->Link->GetOutputDataObject(2));
      if (selection->GetNumberOfNodes())
        {
        vtkSelectionNode *node = selection->GetNode(0);
        vtkIdTypeArray *idArray =
            vtkIdTypeArray::SafeDownCast(node->GetSelectionList());
        if (this->SelectedPointsBuildTime > idArray->GetMTime() ||
            this->GetMTime() > this->SelectedPointsBuildTime)
          {
          this->SelectedPoints.resize(idArray->GetNumberOfTuples());
          for (vtkIdType i = 0; i < idArray->GetNumberOfTuples(); ++i)
            {
            this->SelectedPoints[i] = this->Points[idArray->GetValue(i)];
            }
          this->SelectedPointsBuildTime.Modified();
          }
        }
      }
    }
}

bool vtkChartXYZ::Paint(vtkContext2D *painter)
{
  if (!this->Visible || this->Points.size() == 0)
    return false;

  // Get the 3D context.
  vtkContext3D *context = painter->GetContext3D();

  if (!context)
    return false;

  this->Update();

  // Calculate the transforms required for the current rotation.
  this->CalculateTransforms();

  context->PushMatrix();
  context->AppendTransform(this->ContextTransform.GetPointer());

  // First lets draw the points in 3d.
  context->ApplyPen(this->Pen.GetPointer());
  context->DrawPoints(d->points[0].GetData(),
                      static_cast<int>(d->points.size()));

  // Now to render the selected points.
  if (!this->SelectedPoints.empty())
    {
    context->ApplyPen(this->SelectedPen.GetPointer());
    context->DrawPoints(d->selectedPoints[0].GetData(),
                        static_cast<int>(d->selectedPoints.size()));
    }

  context->PopMatrix();

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
  context->PopMatrix();

  return true;
}

vtkPlot * vtkChartXYZ::AddPlot(int)
{
  return 0;
}

void vtkChartXYZ::SetAngle(double angle)
{
  this->Angle = angle;
}

void vtkChartXYZ::SetAroundX(bool IsX)
{
  this->IsX = IsX;
}

namespace
{
// FIXME: Put this in a central header, as it is used across several classes.
// Copy the two arrays into the points array
template<class A>
void CopyToPoints(float *data, A *input, size_t offset, size_t n)
{
  for (size_t i = 0; i < n; ++i)
    {
    data[3 * i + offset] = *(input++);
    }
}

}

void vtkChartXYZ::SetInput(vtkTable *input, const vtkStdString &xName,
                           const vtkStdString &yName, const vtkStdString &zName)
{

  //cout << "Arrays: " << xName << " -> " << yName << " -> " << zName << endl;
  // Copy the points into our data structure for rendering - pack x, y, z...
  vtkDataArray *xArr =
      vtkDataArray::SafeDownCast(input->GetColumnByName(xName.c_str()));
  vtkDataArray *yArr =
      vtkDataArray::SafeDownCast(input->GetColumnByName(yName.c_str()));
  vtkDataArray *zArr =
      vtkDataArray::SafeDownCast(input->GetColumnByName(zName.c_str()));

  // Ensure that we have valid data arrays, and that they are of the same length.
  assert(xArr);
  assert(yArr);
  assert(zArr);
  assert(xArr->GetNumberOfTuples() == yArr->GetNumberOfTuples() &&
         xArr->GetNumberOfTuples() == zArr->GetNumberOfTuples());

  size_t n = xArr->GetNumberOfTuples();
  this->Points.resize(n);
  float *data = this->Points[0].GetData();

  switch(xArr->GetDataType())
    {
    vtkTemplateMacro(CopyToPoints(data,
                                  static_cast<VTK_TT*>(xArr->GetVoidPointer(0)),
                                  0, n));
    }
  switch(yArr->GetDataType())
    {
    vtkTemplateMacro(CopyToPoints(data,
                                  static_cast<VTK_TT*>(yArr->GetVoidPointer(0)),
                                  1, n));
    }
  switch(zArr->GetDataType())
    {
    vtkTemplateMacro(CopyToPoints(data,
                                  static_cast<VTK_TT*>(zArr->GetVoidPointer(0)),
                                  2, n));
    }
  this->PointsBuildTime.Modified();

  // Now set up the axes, and ranges...
  this->Axes.resize(3);
  vtkNew<vtkAxis> x;
  this->Axes[0] = x.GetPointer();
  x->SetPoint1(vtkVector2f(this->Geometry.X(),
                           this->Geometry.Y()));
  x->SetPoint2(vtkVector2f(this->Geometry.X() + this->Geometry.Width(),
                           this->Geometry.Y()));

  vtkNew<vtkAxis> y;
  this->Axes[1] = y.GetPointer();
  y->SetPoint1(vtkVector2f(this->Geometry.X(),
                           this->Geometry.Y()));
  y->SetPoint2(vtkVector2f(this->Geometry.X(),
                           this->Geometry.Y() + this->Geometry.Height()));

  // Z is faked, largely to get valid ranges and rounded numbers...
  vtkNew<vtkAxis> z;
  this->Axes[2] = z.GetPointer();
  z->SetPoint1(vtkVector2f(this->Geometry.X(),
                           0));
  if (this->IsX)
    {
    z->SetPoint2(vtkVector2f(this->Geometry.X(),
                             this->Geometry.Height()));
    }
  else
    {
    z->SetPoint2(vtkVector2f(this->Geometry.X(),
                             this->Geometry.Width()));
    }
}

void vtkChartXYZ::RecalculateTransform()
{
  this->CalculatePlotTransform(this->Axes[0].GetPointer(),
                               this->Axes[1].GetPointer(),
                               this->Axes[2].GetPointer(),
                               this->Transform.GetPointer());
}

void vtkChartXYZ::RecalculateBounds()
{
  // Need to calculate the bounds in three dimensions and set up the axes.
  vector<vtkVector3f>::const_iterator it = this->Points.begin();
  double bounds[] = { (*it).X(), (*it).X(),
                      (*it).Y(), (*it).Y(),
                      (*it).Z(), (*it).Z()};
  for (++it; it != this->Points.end(); ++it)
    {
    const vtkVector3f &v = *it;
    for (int i = 0; i < 3; ++i)
      {
      if (v[i] < bounds[2 * i])
        bounds[2 * i] = v[i];
      else if (v[i] > bounds[2 * i + 1])
        bounds[2 * i + 1] = v[i];
      }
    }
  for (int i = 0; i < 3; ++i)
    {
    this->Axes[i]->SetRange(&bounds[2*i]);
    }
}

void vtkChartXYZ::SetAnnotationLink(vtkAnnotationLink *link)
{
  if (this->Link != link)
    {
    this->Link = link;
    this->Modified();
    }
}

vtkAxis * vtkChartXYZ::GetAxis(int axis)
{
  assert(axis >= 0 && axis < 3);
  return this->Axes[axis].GetPointer();
}

void vtkChartXYZ::SetGeometry(const vtkRectf &bounds)
{
  this->Geometry = bounds;
  this->Scene->SetGeometry((int)bounds.GetWidth(), (int)bounds.GetHeight());
}

vtkChartXYZ::vtkChartXYZ() : Geometry(0, 0, 10, 10), Link(NULL), Angle(0),
  IsX(false)
{
  this->Pen->SetWidth(5);
  this->Pen->SetColor(0, 0, 0, 255);
  this->SelectedPen->SetWidth(6);
  this->SelectedPen->SetColor(255, 0, 0, 255);
  this->AxisPen->SetWidth(1);
  this->AxisPen->SetColor(0, 0, 0, 255);
  this->Rotation->Identity();
  this->Rotation->PostMultiply();
}

vtkChartXYZ::~vtkChartXYZ()
{
}

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
  float xScale = (x->GetMaximum() - x->GetMinimum()) / (max[0] - min[0]);

  // Now the y axis
  min = y->GetPoint1();
  max = y->GetPoint2();
  if (fabs(max[1] - min[1]) == 0.0f)
    {
    return false;
    }
  float yScale = (y->GetMaximum() - y->GetMinimum()) / (max[1] - min[1]);

  // Now the z axis
  min = z->GetPoint1();
  max = z->GetPoint2();
  if (fabs(max[1] - min[1]) == 0.0f)
    {
    return false;
    }
  float zScale = (z->GetMaximum() - z->GetMinimum()) / (max[1] - min[1]);

  transform->Identity();
  transform->Translate(this->Geometry.X(), this->Geometry.Y(), 0);
  // Get the scale for the plot area from the x and y axes
  transform->Scale(1.0 / xScale, 1.0 / yScale, 1.0 / zScale);
  transform->Translate(-x->GetMinimum(), -y->GetMinimum(), -z->GetMinimum());

  return true;
}

void vtkChartXYZ::CalculateTransforms()
{
  // First the rotation transform...
  // Calculate the correct translation vector before the rotation is applied
  vtkVector3f translation(
        (Axes[0]->GetPosition2()[0] - Axes[0]->GetPosition1()[0]) / 2.0
        + Axes[0]->GetPosition1()[0],
        (Axes[1]->GetPosition2()[1] - Axes[1]->GetPosition1()[1]) / 2.0
        + Axes[1]->GetPosition1()[1],
        (Axes[2]->GetPosition2()[1] - Axes[2]->GetPosition1()[1]) / 2.0
        + Axes[2]->GetPosition1()[1]);
  vtkVector3f mtranslation = -1.0 * translation;

  this->ContextTransform->Identity();
  this->ContextTransform->Translate(translation.GetData());
  this->ContextTransform->Concatenate(this->Rotation.GetPointer());
  if (this->IsX)
    this->ContextTransform->RotateX(this->Angle);
  else
    this->ContextTransform->RotateY(this->Angle);

  this->ContextTransform->Translate(mtranslation.GetData());
  this->ContextTransform->Concatenate(this->Transform.GetPointer());


  // Next the box rotation transform.
  double scale[3] = { 300, 300, 300 };
  for (int i = 0; i < 3; ++i)
    {
    if (i == 0)
      scale[i] = Axes[i]->GetPosition2()[0] - Axes[i]->GetPosition1()[0];
    else
      scale[i] = Axes[i]->GetPosition2()[1] - Axes[i]->GetPosition1()[1];
    }

  this->Box->Identity();
  this->Box->PostMultiply();
  this->Box->Translate(-0.5, -0.5, -0.5);
  this->Box->Concatenate(this->Rotation.GetPointer());
  if (this->IsX)
    this->Box->RotateX(this->Angle);
  else
    this->Box->RotateY(this->Angle);
  this->Box->Translate(0.5, 0.5, 0.5);

  this->Box->Scale(scale);

  if (this->IsX)
    {
    this->Box->Translate(Axes[0]->GetPosition1()[0],
                         Axes[1]->GetPosition1()[1],
                         Axes[2]->GetPosition1()[1]);
    }
  else
    {
    this->Box->Translate(Axes[0]->GetPosition1()[0],
                         Axes[1]->GetPosition1()[1],
                         Axes[2]->GetPosition1()[0]);
    }
}
