/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkChartXYZPrivate.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkChartXYZPrivate - Private implementation for 3D charts
//
// .SECTION Description
//
// \internal

#ifndef __vtkChartXYZPrivate__
#define __vtkChartXYZPrivate__

#include "vtkAxis.h"
#include "vtkNew.h"
#include "vtkSmartPointer.h"
#include "vtkTimeStamp.h"
#include "vtkTransform.h"
#include "vtkVector.h"
#include "vtkVectorOperators.h"

#include <vector>
using std::vector;

class vtkChartXYZPrivate
{
public:
  vtkChartXYZPrivate();

  // Calculate the required transforms for the XYZ chart.
  void CalculateTransforms();

  vector<vtkVector3f> points;
  vtkTimeStamp pointsBuidTime;
  vector<vtkVector3f> selectedPoints;
  vtkTimeStamp selectedPointsBuidTime;

  vector< vtkSmartPointer<vtkAxis> > axes;
  vtkNew<vtkTransform> Transform;
  vtkNew<vtkTransform> Translation;
  vtkNew<vtkTransform> Rotation;
  vtkNew<vtkTransform> Rotate;
  vtkNew<vtkTransform> Box;
  double angle;

  vtkVector3f origin;
  vtkVector3f other;
  vtkVector3f xyz[3];

  bool isX;
  bool init;
};
  
inline vtkChartXYZPrivate::vtkChartXYZPrivate() : angle(0), isX(false), init(false)
{
  this->Rotate->Identity();
  this->Rotate->PostMultiply();
}

inline void vtkChartXYZPrivate::CalculateTransforms()
{
  // First the rotation transform...
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
  this->Rotation->Concatenate(this->Rotate.GetPointer());
/*
  if (isX)
    this->Rotation->RotateX(this->angle);
  else
    this->Rotation->RotateY(this->angle);
    */

  this->Rotation->Translate(mtranslation.GetData());
  this->Rotation->Concatenate(this->Transform.GetPointer());


  // Next the box rotation transform.
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
  this->Box->Concatenate(this->Rotate.GetPointer());
  /*
  if (isX)
    this->Box->RotateX(this->angle);
  else
    this->Box->RotateY(this->angle);
    */
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
}

#endif //__vtkChartXYZPrivate__
// VTK-HeaderTest-Exclude: vtkChartXYZPrivate.h
