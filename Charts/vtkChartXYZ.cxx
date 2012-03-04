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
#include "vtkTable.h"
#include "vtkFloatArray.h"
#include "vtkTransform.h"
#include "vtkVector.h"
#include "vtkVectorOperators.h"
#include "vtkNew.h"

#include "vtkObjectFactory.h"

// This obviously needs to go away, but use GL directly for now...
#include "vtkgl.h"

#include <vector>
#include <cassert>

using std::vector;

class vtkChartXYZ::Private
{
public:
  Private() : angle(0), isX(false), init(false) {}
  void SetMatrix(bool transformed = true);

  vector<vtkVector3f> points;
  vector< vtkSmartPointer<vtkAxis> > axes;
  vtkNew<vtkTransform> Transform;
  vtkNew<vtkTransform> Rotation;
  vtkNew<vtkTransform> Box;
  double angle;

  vtkVector3f origin;
  vtkVector3f other;
  vtkVector3f xyz[3];

  bool isX;
  bool init;
};

inline void vtkChartXYZ::Private::SetMatrix(bool transformed)
{
  double *M = 0;

  if (transformed)
    {
    // Calculate the correct translation vector before the rotation is applied
    vtkVector3f translation(
          (axes[0]->GetPosition2()[0] - axes[0]->GetPosition1()[0]) / 2.0
          + axes[0]->GetPosition1()[0],
          (axes[1]->GetPosition2()[1] - axes[1]->GetPosition1()[1]) / 2.0
          + axes[1]->GetPosition1()[1],
          (axes[2]->GetPosition2()[1] - axes[2]->GetPosition1()[1]) / 2.0
          + axes[2]->GetPosition1()[1]);
    vtkVector3f mtranslation = -1.0 * translation;

    this->Rotation->Identity();
    this->Rotation->Translate(translation.GetData());
    if (isX)
      this->Rotation->RotateX(this->angle);
    else
      this->Rotation->RotateY(this->angle);

    this->Rotation->Translate(mtranslation.GetData());

    this->Rotation->Concatenate(this->Transform.GetPointer());

    // Construct a matrix of the correct size.
    M = this->Rotation->GetMatrix()->Element[0];
    }
  else
    {
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
    if (isX)
      this->Box->RotateX(this->angle);
    else
      this->Box->RotateY(this->angle);
    this->Box->Translate(0.5, 0.5, 0.5);

    this->Box->Scale(scale);

    if (isX)
      {
      this->Box->Translate(axes[0]->GetPosition1()[0],
                           axes[1]->GetPosition1()[1],
                           axes[2]->GetPosition1()[1]);
      }
    else
      {
      this->Box->Translate(axes[0]->GetPosition1()[0],
                           axes[1]->GetPosition1()[1],
                           axes[2]->GetPosition1()[0]);
      }

    M = this->Box->GetMatrix()->Element[0];
    }

  double matrix[16];

  // Convert the row/column ordering...
  matrix[0] = M[0];
  matrix[1] = M[4];
  matrix[2] = M[8];
  matrix[3] = M[12];

  matrix[4] = M[1];
  matrix[5] = M[5];
  matrix[6] = M[9];
  matrix[7] = M[13];

  matrix[8] = M[2];
  matrix[9] = M[6];
  matrix[10] = M[10];
  matrix[11] = M[14];

  matrix[12] = M[3];
  matrix[13] = M[7];
  matrix[14] = M[11];
  matrix[15] = M[15];

  glMultMatrixd(matrix);
}

vtkStandardNewMacro(vtkChartXYZ)

void vtkChartXYZ::PrintSelf(ostream &os, vtkIndent indent)
{
  Superclass::PrintSelf(os, indent);
}

bool vtkChartXYZ::Paint(vtkContext2D *painter)
{
  if (!this->Visible || d->points.size() == 0)
    return false;

  // This is where the magic happens for now...
  painter->PushMatrix();

  d->SetMatrix();
  // First lets draw the points in 3d.
  glColor4ub(0, 0, 0, 255);
  glPointSize(5);
  glEnableClientState(GL_VERTEX_ARRAY);
  glVertexPointer(3, GL_FLOAT, 0, d->points[0].GetData());
  glDrawArrays(GL_POINTS, 0, d->points.size());
  glDisableClientState(GL_VERTEX_ARRAY);
  painter->PopMatrix();


  painter->PushMatrix();
  d->SetMatrix(false);
  glColor4ub(0, 0, 0, 255);
  // Now lets draw the axes over the top.
  glBegin(GL_LINE_LOOP);
  // Front
  glVertex3f(0, 0, 0);
  glVertex3f(0, 1, 0);
  glVertex3f(1, 1, 0);
  glVertex3f(1, 0, 0);
  glEnd();

  glBegin(GL_LINE_LOOP);
  // Back
  glVertex3f(0, 0, 1);
  glVertex3f(0, 1, 1);
  glVertex3f(1, 1, 1);
  glVertex3f(1, 0, 1);
  glEnd();

  glBegin(GL_LINES);
  glVertex3f(0, 0, 0);
  glVertex3f(0, 0, 1);
  glVertex3f(0, 1, 0);
  glVertex3f(0, 1, 1);
  glVertex3f(1, 1, 0);
  glVertex3f(1, 1, 1);
  glVertex3f(1, 0, 0);
  glVertex3f(1, 0, 1);
  glEnd();

  painter->PopMatrix();

  return true;
}

vtkPlot * vtkChartXYZ::AddPlot(int)
{
  return 0;
}

void vtkChartXYZ::SetAngle(double angle)
{
  d->angle = angle;
}

void vtkChartXYZ::SetAroundX(bool isX)
{
  d->isX = isX;
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
  d->points.resize(n);
  float *data = d->points[0].GetData();

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

  // Now set up the axes, and ranges...
  d->axes.resize(3);
  vtkNew<vtkAxis> x;
  d->axes[0] = x.GetPointer();
  x->SetPoint1(vtkVector2f(this->Geometry.X(),
                           this->Geometry.Y()));
  x->SetPoint2(vtkVector2f(this->Geometry.X() + this->Geometry.Width(),
                           this->Geometry.Y()));

  vtkNew<vtkAxis> y;
  d->axes[1] = y.GetPointer();
  y->SetPoint1(vtkVector2f(this->Geometry.X(),
                           this->Geometry.Y()));
  y->SetPoint2(vtkVector2f(this->Geometry.X(),
                           this->Geometry.Y() + this->Geometry.Height()));

  // Z is faked, largely to get valid ranges and rounded numbers...
  vtkNew<vtkAxis> z;
  d->axes[2] = z.GetPointer();
  z->SetPoint1(vtkVector2f(this->Geometry.X(),
                           0));
  if (d->isX)
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
  this->CalculatePlotTransform(d->axes[0].GetPointer(),
                               d->axes[1].GetPointer(),
                               d->axes[2].GetPointer(),
                               d->Transform.GetPointer());
}

void vtkChartXYZ::RecalculateBounds()
{
  // Need to calculate the bounds in three dimensions and set up the axes.
  vector<vtkVector3f>::const_iterator it = d->points.begin();
  double bounds[] = { (*it).X(), (*it).X(),
                      (*it).Y(), (*it).Y(),
                      (*it).Z(), (*it).Z()};
  for (++it; it != d->points.end(); ++it)
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
    d->axes[i]->SetRange(&bounds[2*i]);
    }
}

void vtkChartXYZ::SetAnnotationLink(vtkAnnotationLink *link)
{
  // Copy the row numbers so that we can do the highlight...
}

vtkAxis * vtkChartXYZ::GetAxis(int axis)
{
  assert(axis >= 0 && axis < 3);
  return d->axes[axis].GetPointer();
}

void vtkChartXYZ::SetGeometry(const vtkRectf &bounds)
{
  this->Geometry = bounds;
}

vtkChartXYZ::vtkChartXYZ() : Geometry(0, 0, 10, 10)
{
  d = new Private;
}

vtkChartXYZ::~vtkChartXYZ()
{
  delete d;
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
