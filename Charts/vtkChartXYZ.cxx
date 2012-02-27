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
  Private() : angle(0), isX(false) {}
  void SetMatrix(bool transformed = true);

  vector<vtkVector3f> points;
  vector< vtkSmartPointer<vtkAxis> > axes;
  vtkNew<vtkTransform> Transform;
  vtkNew<vtkTransform> Rotation;
  double angle;

  vtkVector3f origin;
  vtkVector3f other;
  vtkVector3f xyz[3];

  bool isX;
};

inline void vtkChartXYZ::Private::SetMatrix(bool transformed)
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

  //cout << "translation vector: " << translation << endl;
  //cout << "translation vector: " << mtranslation << endl;

  this->Rotation->Identity();
  this->Rotation->Translate(translation.GetData());
  if (isX)
    this->Rotation->RotateX(this->angle);
  else
    this->Rotation->RotateY(this->angle);
  this->Rotation->Translate(mtranslation.GetData());
  if (transformed)
    this->Rotation->Concatenate(this->Transform.GetPointer());

  // Construct a matrix of the correct size.
  double *M = this->Rotation->GetMatrix()->Element[0];
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
/*
  glColor4ub(255, 222, 0, 255);
  glBegin(GL_LINES);
  for (int i = 0; i < 3; ++i)
    {
    glVertex3fv(d->origin.GetData());
    glVertex3fv(d->xyz[i].GetData());
    }
  glEnd();
  */

  // This is where the magic happens for now...
  painter->PushMatrix();

  //if (d->angle < 90.0)
  //  d->angle += 0.5;

  d->SetMatrix();

  // First lets draw the points in 3d.
  glColor4ub(0, 0, 0, 255);
  glPointSize(5);
  glBegin(GL_POINTS);
  for (int i = 0; i < d->points.size(); ++i)
    {
    glVertex3fv(d->points[i].GetData());
    }
  glEnd();

  painter->PopMatrix();

  painter->PushMatrix();
  d->SetMatrix(false);
  // Now lets draw the axes over the top.
  glBegin(GL_LINES);
  for (int i = 0; i < 3; ++i)
    {
    switch (i)
      {
    case 0:
      glColor4ub(255, 0, 0, 255);
      break;
    case 1:
      glColor4ub(0, 255, 0, 255);
      break;
    case 2:
      glColor4ub(0, 0, 255, 255);
      break;
      }

    glVertex3fv(d->origin.GetData());
    glVertex3fv(d->xyz[i].GetData());
    //glVertex3fv(d->other.GetData());
    //glVertex3fv(d->xyz[i].GetData());
    }
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

void vtkChartXYZ::SetInput(vtkTable *input, const vtkStdString &xName,
                           const vtkStdString &yName, const vtkStdString &zName)
{

  cout << "Arrays: " << xName << " -> " << yName << " -> " << zName << endl;

  // Copy the points into our data structure for rendering - pack x, y, z...
  vtkFloatArray *xArr =
      vtkFloatArray::SafeDownCast(input->GetColumnByName(xName.c_str()));
  vtkFloatArray *yArr =
      vtkFloatArray::SafeDownCast(input->GetColumnByName(yName.c_str()));
  vtkFloatArray *zArr =
      vtkFloatArray::SafeDownCast(input->GetColumnByName(zName.c_str()));

  assert(xArr);
  assert(yArr);
  assert(zArr);

  cout << "Number of tuples: " << xArr->GetNumberOfTuples()
       << ", " << yArr->GetNumberOfTuples()
       << ", " << zArr->GetNumberOfTuples() << endl;

  assert(xArr->GetNumberOfTuples() == yArr->GetNumberOfTuples() &&
         xArr->GetNumberOfTuples() == zArr->GetNumberOfTuples());

  float *pts[3];
  pts[0] = static_cast<float *>(xArr->GetVoidPointer(0));
  pts[1] = static_cast<float *>(yArr->GetVoidPointer(0));
  pts[2] = static_cast<float *>(zArr->GetVoidPointer(0));

  vtkIdType n = xArr->GetNumberOfTuples();
  d->points.clear();
  d->points.reserve(n);
  for (vtkIdType i = 0; i < n; ++i)
    {
    d->points.push_back(vtkVector3f(*(pts[0]++), *(pts[1]++), *(pts[2]++)));
    }

//  for (int i = 0; i < n; ++i)
//    cout << i << ": " << d->points[i] << endl;

  // Now set up the axes, and ranges...
  double range[2];
  d->axes.resize(3);
  vtkNew<vtkAxis> x;
  d->axes[0] = x.GetPointer();
  x->SetPoint1(vtkVector2f(this->Geometry.X(),
                           this->Geometry.Y()));
  x->SetPoint2(vtkVector2f(this->Geometry.X() + this->Geometry.Width(),
                           this->Geometry.Y()));
  xArr->GetRange(range);
  x->SetRange(range);
  x->Update();
  cout << "X range is: " << range[0] << " -> " << range[1] << endl;

  d->origin.Set(this->Geometry.X(), this->Geometry.Y(), this->Geometry.X());
  d->other.Set(this->Geometry.X() + this->Geometry.Width(),
               this->Geometry.Y() + this->Geometry.Height(),
               this->Geometry.X() + this->Geometry.Width());
  d->xyz[0].Set(this->Geometry.X() + this->Geometry.Width(),
                this->Geometry.Y(),
                this->Geometry.X());
  d->xyz[1].Set(this->Geometry.X(),
                this->Geometry.Y() + this->Geometry.Height(),
                this->Geometry.X());
  d->xyz[2].Set(this->Geometry.X(),
                this->Geometry.Y(),
                this->Geometry.X() + this->Geometry.Width());

  cout << "origin: " << d->origin
       << ", x: " << d->xyz[0] << endl;

  vtkNew<vtkAxis> y;
  d->axes[1] = y.GetPointer();
  y->SetPoint1(vtkVector2f(this->Geometry.X(),
                           this->Geometry.Y()));
  y->SetPoint2(vtkVector2f(this->Geometry.X(),
                           this->Geometry.Y() + this->Geometry.Height()));
  yArr->GetRange(range);
  y->SetRange(range);
  y->Update();
  cout << "Y range is: " << range[0] << " -> " << range[1] << endl;

  // Z is faked, largely to get valid ranges and rounded numbers...
  vtkNew<vtkAxis> z;
  d->axes[2] = z.GetPointer();
  z->SetPoint1(vtkVector2f(this->Geometry.X(),
                           this->Geometry.Y()));
  z->SetPoint2(vtkVector2f(this->Geometry.X(),
                           this->Geometry.Y() + this->Geometry.Height()));
  zArr->GetRange(range);
  z->SetRange(range);
  z->Update();
  cout << "Z range is: " << range[0] << " -> " << range[1] << endl;

  this->CalculatePlotTransform(x.GetPointer(), y.GetPointer(), z.GetPointer(),
                               d->Transform.GetPointer());
  d->Transform->Print(cout);
}

void vtkChartXYZ::SetAnnotationLink(vtkAnnotationLink *link)
{
  // Copy the row numbers so that we can do the highlight...
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
  if (!x || !y || !z || !transform)
    {
    vtkWarningMacro("Called with null arguments.");
    return false;
    }
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
  transform->Translate(this->Geometry.X(), this->Geometry.Y(), this->Geometry.X());
  // Get the scale for the plot area from the x and y axes
  transform->Scale(1.0 / xScale, 1.0 / yScale, 1.0 / zScale);
  transform->Translate(-x->GetMinimum(), -y->GetMinimum(), -z->GetMinimum());

  return true;
}
