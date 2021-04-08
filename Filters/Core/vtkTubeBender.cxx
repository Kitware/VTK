/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTubeFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkTubeBender.h"

#include "vtkPoints.h"
#include "vtkPolyLine.h"
#include "vtkTransform.h"

//----------------------------------------------------------------------------

namespace
{
void initRotateAboutLineWXYZQuaternion(
  const double normalizedLineVector[3], const double theta, double quaternion[4])
{
  /* w */ quaternion[0] = theta;
  /* x */ quaternion[1] = normalizedLineVector[0];
  /* y */ quaternion[2] = normalizedLineVector[1];
  /* z */ quaternion[3] = normalizedLineVector[2];
}
} // end anon namespace

//----------------------------------------------------------------------------

vtkStandardNewMacro(vtkTubeBender);

vtkTubeBender::vtkTubeBender()
{
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);

  this->Radius = 1.0;
}

vtkTubeBender::~vtkTubeBender() = default;

int vtkTubeBender::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  const double INCREMENT = 2.0 * vtkMath::Pi() / 12;

  // get the input and output
  vtkPolyData* input = vtkPolyData::GetData(inputVector[0], 0);
  if (input == nullptr)
  {
    vtkErrorMacro("Input is null.");
    return 0;
  }
  vtkPolyData* output = vtkPolyData::GetData(outputVector, 0);
  if (output == nullptr)
  {
    vtkErrorMacro("Output is null.");
    return 0;
  }

  output->ShallowCopy(input);

  auto iPoints = input->GetPoints();
  auto iLines = input->GetLines();

  if (iPoints == nullptr || iLines == nullptr || iPoints->GetNumberOfPoints() == 0 ||
    iLines->GetNumberOfCells() == 0)
  {
    return 1;
  }

  // Setup the new output
  vtkNew<vtkPoints> oPoints;
  vtkNew<vtkCellArray> oLines;
  oPoints->DeepCopy(iPoints);

  // Traverse all new cells to insert new points into the paths
  vtkIdType linePoints = 0;
  const vtkIdType* linePointIds = nullptr;
  for (iLines->InitTraversal(); iLines->GetNextCell(linePoints, linePointIds);)
  {
    // Process each point in each line cell
    vtkNew<vtkPolyLine> oLine;
    for (vtkIdType lpIndex = 0; lpIndex < linePoints; lpIndex++)
    {
      if (lpIndex == 0 || lpIndex == linePoints - 1)
      {
        // The first and last points in each line should be preserved
        oLine->GetPointIds()->InsertNextId(linePointIds[lpIndex]);
      }
      else
      {
        // Insert new intermediate points to create a curve
        auto pointBeforeId = linePointIds[lpIndex - 1];
        auto pointID = linePointIds[lpIndex];
        auto pointAfterId = linePointIds[lpIndex + 1];

        double pointBefore[3], point[3], pointAfter[3];
        vtkMath::Assign(iPoints->GetPoint(pointBeforeId), pointBefore);
        vtkMath::Assign(iPoints->GetPoint(pointID), point);
        vtkMath::Assign(iPoints->GetPoint(pointAfterId), pointAfter);

        // Determine the contributing vectors
        double before[3], after[3];
        vtkMath::Subtract(pointBefore, point, before);
        vtkMath::Subtract(pointAfter, point, after);
        double beforeDistance = vtkMath::Norm(before);
        double afterDistance = vtkMath::Norm(after);

        // Determine if we need to add any intermediate points or if the original value will do
        // instead
        auto angle = vtkMath::AngleBetweenVectors(before, after);
        if (std::isnan(angle))
        {
          vtkErrorMacro("Line " << lpIndex << " Point " << pointID
                                << " has an invalid angle and will be omitted");
          continue; // If any input point has a nan value, the angle will be nan, creating an
                    // infinite loop of point insertions
        }
        else if (angle >= vtkMath::Pi() - INCREMENT)
        {
          // Point has a small deviation, add on it's own
          oLine->GetPointIds()->InsertNextId(linePointIds[lpIndex]);
        }
        else // Point has an acute angle; break it up
        {
          // Determine what axis we will be rotating around
          double rotationAxis[3];
          vtkMath::Cross(before, after, rotationAxis);
          vtkMath::Normalize(rotationAxis);

          // Determine what the origin of rotation will be. It will usually be a point, on the
          // exterior of the tube, on the inside of the curve, for the original point.
          double ba[3];
          vtkMath::Subtract(pointAfter, pointBefore, ba);
          vtkMath::MultiplyScalar(ba, 0.5);
          vtkMath::Add(pointBefore, ba, ba);
          double rotationPoint[3];       // The origin of rotation
          double rotationPointVector[3]; // Distance between the origin and the new points which
                                         // will be created
          double rotationPointOffsetVector[3]; // Distance to offset the rotation point from the
                                               // original point
          vtkMath::Subtract(ba, point, rotationPointVector);
          vtkMath::Normalize(rotationPointVector);
          // While the rotation point and the offset are usually of radius size, if the angle is too
          // acute, it will cause the outer edge of the tube to bulge because sin(angle/2) < radius
          // To reduce this bulging, we are offseting the rotation point by an approximation towards
          // the inside of the curve
          auto quarterPi = vtkMath::Pi() / 4;
          if ((angle / 2.0) >= quarterPi)
          {
            vtkMath::MultiplyScalar(rotationPointVector, this->Radius);
            vtkMath::Assign(rotationPointVector, rotationPointOffsetVector);
          }
          else
          {
            vtkMath::Assign(rotationPointVector, rotationPointOffsetVector);
            auto percent = (quarterPi - angle / 2.0) / quarterPi * 0.8 + 1.0;
            vtkMath::MultiplyScalar(rotationPointOffsetVector, this->Radius * percent);

            vtkMath::MultiplyScalar(rotationPointVector, this->Radius);
          }
          vtkMath::Add(point, rotationPointOffsetVector, rotationPoint);
          vtkMath::MultiplyScalar(rotationPointVector, -1.0);

          // Add the Intermediate Points
          int numIntervals =
            (std::max)(std::floor((vtkMath::Pi() - angle / 2.0) / INCREMENT) - 1, 1.0);
          for (int interval = numIntervals - 2; interval >= 0; interval--)
          {
            // Avoid adding points which conflict with neighbouring original points
            // Done in loop to ensure the original point with offset will not be filtered out
            if (beforeDistance <= this->Radius && interval != 0)
            {
              continue;
            }

            // Insert Previous Interval Points & the Midpoint
            double p[3];
            vtkMath::Assign(rotationPointVector, p);
            double q[4];
            initRotateAboutLineWXYZQuaternion(
              rotationAxis, 1.0 * static_cast<double>(interval) * INCREMENT / 2.0, q);
            vtkMath::RotateVectorByWXYZ(p, q, p);
            vtkMath::Add(rotationPoint, p, p);

            oPoints->InsertNextPoint(p);
            oLine->GetPointIds()->InsertNextId(oPoints->GetNumberOfPoints() - 1);
          }

          // Add the original point -> This is done by the previous loop above instead of here
          // because it could be offset towards the center of the curve by the bulge compensation
          // oLine->GetPointIds()->InsertNextId(pointID);

          // Avoid adding points which conflict with neighbouring original points
          if (afterDistance >= this->Radius)
          {
            for (int i = 1; i < numIntervals - 1; i++)
            {
              // Insert After Interval Points
              double p[3];
              vtkMath::Assign(rotationPointVector, p);
              double q[4];
              initRotateAboutLineWXYZQuaternion(
                rotationAxis, -1.0 * static_cast<double>(i) * INCREMENT / 2.0, q);
              vtkMath::RotateVectorByWXYZ(p, q, p);
              vtkMath::Add(rotationPoint, p, p);

              oPoints->InsertNextPoint(p);
              oLine->GetPointIds()->InsertNextId(oPoints->GetNumberOfPoints() - 1);
            }
          }
        }
      }
    }
    oLines->InsertNextCell(oLine);
  }

  output->SetPoints(oPoints);
  output->SetLines(oLines);

  return 1; // 1 = Pipeline Stage Successful
}

void vtkTubeBender::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Radius: " << this->Radius << "\n";
}
