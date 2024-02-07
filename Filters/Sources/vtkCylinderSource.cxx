// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkCylinderSource.h"

#include "vtkCellArray.h"
#include "vtkFloatArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"

#include <cmath>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkCylinderSource);

//------------------------------------------------------------------------------
vtkCylinderSource::vtkCylinderSource(int res)
{
  this->Resolution = res;
  this->Height = 1.0;
  this->Radius = 0.5;
  this->Capping = 1;
  this->CapsuleCap = 0;
  this->LatLongTessellation = 0;
  this->Center[0] = this->Center[1] = this->Center[2] = 0.0;
  this->OutputPointsPrecision = SINGLE_PRECISION;

  this->SetNumberOfInputPorts(0);
}

//------------------------------------------------------------------------------
int vtkCylinderSource::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector)
{
  // get the info object
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the output
  vtkPolyData* output = vtkPolyData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  double angle = 2.0 * vtkMath::Pi() / this->Resolution;
  int numPolys, numPts;
  double xbot[3], tcbot[2], nbot[3];
  double xtop[3], tctop[2], ntop[3];
  double* center = this->Center;
  int i;
  int idx = 0;
  vtkIdType pts[VTK_CELL_SIZE];

  //
  // Set things up; allocate memory
  //

  numPts = 2 * this->Resolution;
  numPolys = this->Resolution;
  if (this->Capping)
  {
    if (this->CapsuleCap)
    {
      numPts += 2 * (this->Resolution * this->Resolution + 2);
      numPolys += this->Resolution * this->Resolution * 4;
    }
    else
    {
      numPts += 2 * this->Resolution;
      numPolys += 2;
    }
  }

  vtkNew<vtkPoints> newPoints;

  // Set the desired precision for the points in the output.
  if (this->OutputPointsPrecision == vtkAlgorithm::DOUBLE_PRECISION)
  {
    newPoints->SetDataType(VTK_DOUBLE);
  }
  else
  {
    newPoints->SetDataType(VTK_FLOAT);
  }

  newPoints->Allocate(numPts);
  vtkNew<vtkFloatArray> newNormals;
  newNormals->SetNumberOfComponents(3);
  newNormals->Allocate(numPts);
  newNormals->SetName("Normals");
  vtkNew<vtkFloatArray> newTCoords;
  newTCoords->SetNumberOfComponents(2);
  newTCoords->Allocate(numPts);
  newTCoords->SetName("TCoords");

  vtkNew<vtkCellArray> newPolys;
  newPolys->AllocateEstimate(numPolys, this->Resolution);
  //
  // Generate points and point data for sides
  //

  // Generate the north pole points
  if (this->Capping && this->CapsuleCap)
  {
    idx = this->CreateHemisphere(newPoints, newNormals, newTCoords, newPolys, idx);
  }

  int cylIdx = 0;
  // Now, comes the cylinder
  for (i = 0; i < this->Resolution; i++)
  {
    // x coordinate
    nbot[0] = ntop[0] = cos(i * angle);
    xbot[0] = (nbot[0] * this->Radius) + center[0];
    xtop[0] = (ntop[0] * this->Radius) + center[0];
    tcbot[0] = tctop[0] = fabs(2.0 * i / this->Resolution - 1.0);

    // y coordinate
    xbot[1] = 0.5 * this->Height + center[1];
    xtop[1] = -0.5 * this->Height + center[1];
    nbot[1] = ntop[1] = 0.0;
    if (this->Capping && this->CapsuleCap)
    {
      tctop[1] =
        1.0 - (xtop[1] + 0.5 * this->Height + this->Radius) / (2 * this->Radius + this->Height);
      tcbot[1] =
        1.0 - (xbot[1] + 0.5 * this->Height + this->Radius) / (2 * this->Radius + this->Height);
    }
    else
    {
      tcbot[1] = 0.0;
      tctop[1] = 1.0;
    }

    // z coordinate
    nbot[2] = ntop[2] = -sin(i * angle);
    xbot[2] = (nbot[2] * this->Radius) + center[2];
    xtop[2] = (ntop[2] * this->Radius) + center[2];

    cylIdx = idx + 2 * i;
    newPoints->InsertPoint(cylIdx, xbot);
    newPoints->InsertPoint(cylIdx + 1, xtop);
    newTCoords->InsertTuple(cylIdx, tcbot);
    newTCoords->InsertTuple(cylIdx + 1, tctop);
    newNormals->InsertTuple(cylIdx, nbot);
    newNormals->InsertTuple(cylIdx + 1, ntop);
  }

  // Generate polygons for sides
  for (i = 0; i < this->Resolution; i++)
  {
    pts[0] = idx + 2 * i;
    pts[1] = pts[0] + 1;
    pts[2] = idx + (pts[1] + 2 - idx) % (2 * this->Resolution);
    pts[3] = pts[2] - 1;
    newPolys->InsertNextCell(4, pts);
  }

  // Update the point index
  idx += this->Resolution * 2 - 1;

  if (this->Capping)
  {
    if (this->CapsuleCap)
    {
      idx = this->CreateHemisphere(newPoints, newNormals, newTCoords, newPolys, idx);
    }
    else
    {
      //
      // Generate points and point data for the top/bottom polygons
      //
      for (i = 0; i < this->Resolution; i++)
      {
        // x coordinate
        xbot[0] = xtop[0] = this->Radius * cos(i * angle);
        nbot[0] = ntop[0] = 0.0;
        tcbot[0] = tctop[0] = xbot[0];
        xbot[0] += center[0];
        xtop[0] += center[0];

        // y coordinate
        xbot[1] = 0.5 * this->Height;
        xtop[1] = -0.5 * this->Height;
        nbot[1] = 1.0;
        ntop[1] = -1.0;
        xbot[1] += center[1];
        xtop[1] += center[1];

        // z coordinate
        xbot[2] = xtop[2] = -this->Radius * sin(i * angle);
        tcbot[1] = tctop[1] = xbot[2];
        xbot[2] += center[2];
        xtop[2] += center[2];
        nbot[2] = 0.0;
        ntop[2] = 0.0;

        idx = 2 * this->Resolution;
        newPoints->InsertPoint(idx + i, xbot);
        newTCoords->InsertTuple(idx + i, tcbot);
        newNormals->InsertTuple(idx + i, nbot);

        idx = 3 * this->Resolution;
        newPoints->InsertPoint(idx + this->Resolution - i - 1, xtop);
        newTCoords->InsertTuple(idx + this->Resolution - i - 1, tctop);
        newNormals->InsertTuple(idx + this->Resolution - i - 1, ntop);
      }
      //
      // Generate polygons for top/bottom polygons
      //
      for (i = 0; i < this->Resolution; i++)
      {
        pts[i] = 2 * this->Resolution + i;
      }
      newPolys->InsertNextCell(this->Resolution, pts);
      for (i = 0; i < this->Resolution; i++)
      {
        pts[i] = 3 * this->Resolution + i;
      }
      newPolys->InsertNextCell(this->Resolution, pts);
    }
  }

  //
  // Update ourselves and release memory
  //
  output->SetPoints(newPoints);

  output->GetPointData()->SetNormals(newNormals);

  output->GetPointData()->SetTCoords(newTCoords);

  newPolys->Squeeze(); // since we've estimated size; reclaim some space
  output->SetPolys(newPolys);

  return 1;
}

//------------------------------------------------------------------------------
int vtkCylinderSource::CreateHemisphere(vtkPoints* newPoints, vtkFloatArray* newNormals,
  vtkFloatArray* newTCoords, vtkCellArray* newPolys, int startIdx)
{
  double n[3], x[3], norm, theta;
  const double halfHeight = this->Height * 0.5;
  const int thetaResolution = this->Resolution;
  const int numPoles = 1;
  double* center = this->Center;
  const int phiResolution = this->Resolution - numPoles;
  const int base = (phiResolution - 1) * thetaResolution;
  const double deltaTheta = 2 * vtkMath::Pi() / thetaResolution;
  const double deltaPhi = 0.5 * vtkMath::Pi() / (phiResolution - 1);
  bool isNorthernHemisphere = (startIdx == 0);
  const double startPhi = isNorthernHemisphere ? 0.0 : 0.5 * vtkMath::Pi();
  int lidx = startIdx;
  vtkIdType pts[VTK_CELL_SIZE];
  double tc[2];
  if (isNorthernHemisphere)
  {
    x[0] = center[0];
    x[1] = center[1] + this->Radius + halfHeight;
    x[2] = center[2];
    newPoints->InsertPoint(lidx, x);
    n[0] = 0.0;
    n[1] = 1.0;
    n[2] = 0.0;
    newNormals->InsertTuple(lidx, n);
    tc[0] = 0.0;
    tc[1] = 0.0;
    newTCoords->InsertTuple(lidx, tc);
  }
  int jStart = isNorthernHemisphere ? 1 : 0;
  int jEnd = isNorthernHemisphere ? phiResolution : phiResolution - 1;
  for (int i = 0; i < thetaResolution; ++i)
  {
    theta = i * deltaTheta;
    tc[0] = fabs(2.0 * i / thetaResolution - 1.0);
    for (int j = jStart; j < jEnd; ++j)
    {
      lidx++;
      const double phi = startPhi + j * deltaPhi;
      const double r = this->Radius * std::sin(phi);
      n[0] = r * std::cos(theta);
      n[2] = r * std::sin(theta);
      n[1] = this->Radius * std::cos(phi);
      x[0] = n[0] + center[0];
      if (isNorthernHemisphere)
      {
        x[1] = n[1] + center[1] + halfHeight;
      }
      else
      {
        x[1] = n[1] - center[1] - halfHeight;
      }
      x[2] = n[2] + center[2];
      newPoints->InsertPoint(lidx, x);
      if ((norm = vtkMath::Norm(n)) == 0.0)
      {
        norm = 1.0;
      }
      n[0] /= norm;
      n[1] /= norm;
      n[2] /= norm;
      newNormals->InsertTuple(lidx, n);
      tc[1] = 1.0 - (x[1] + halfHeight + this->Radius) / (2 * this->Radius + this->Height);
      newTCoords->InsertTuple(lidx, tc);
    }
  }
  if (!isNorthernHemisphere)
  {
    // Finally insert southern pole point
    lidx++;
    x[0] = center[0];
    x[1] = center[1] - this->Radius - halfHeight;
    x[2] = center[2];
    newPoints->InsertPoint(lidx, x);
    n[0] = 0.0;
    n[1] = -1.0;
    n[2] = 0.0;
    newNormals->InsertTuple(lidx, n);
    tc[0] = 0.0;
    tc[1] = 1.0;
    newTCoords->InsertTuple(lidx, tc);
  }

  // Cells for the bands
  for (int i = 0; i < thetaResolution; i++)
  {
    for (int j = 0; j < (phiResolution - 2); j++)
    {
      pts[0] = startIdx + (phiResolution - 1) * i + j + numPoles;
      pts[1] = startIdx + (((phiResolution - 1) * (i + 1) + j) % base) + numPoles;
      pts[2] = pts[1] + 1;
      if (!this->LatLongTessellation)
      {
        newPolys->InsertNextCell(3, pts);
        pts[1] = pts[2];
        pts[2] = pts[0] + 1;
        newPolys->InsertNextCell(3, pts);
      }
      else
      {
        pts[3] = pts[0] + 1;
        newPolys->InsertNextCell(4, pts);
      }
    }
    // this->UpdateProgress(0.70 + 0.30 * i / static_cast<double>(localThetaResolution));
  }
  // Cells around the poles
  if (isNorthernHemisphere)
  {
    for (int i = 0; i < thetaResolution; i++)
    {
      pts[0] = (phiResolution - 1) * i + numPoles;
      pts[2] = (((phiResolution - 1) * (i + 1)) % base) + numPoles;
      pts[1] = numPoles - 1;
      newPolys->InsertNextCell(3, pts);
    }
  }
  else
  {
    // Cells around the southern pole
    const int numOffset = startIdx + phiResolution - 1 + numPoles;
    for (int i = 0; i < thetaResolution; i++)
    {
      pts[0] = (phiResolution - 1) * i + numOffset - 1;
      pts[1] = (((phiResolution - 1) * (i + 1)) % base) + numOffset - 1;
      pts[2] = lidx;
      newPolys->InsertNextCell(3, pts);
    }
  }
  // startIdx += lidx;
  return lidx + 1;
}

//------------------------------------------------------------------------------
void vtkCylinderSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Resolution: " << this->Resolution << "\n";
  os << indent << "Height: " << this->Height << "\n";
  os << indent << "Radius: " << this->Radius << "\n";
  os << indent << "Center: (" << this->Center[0] << ", " << this->Center[1] << ", "
     << this->Center[2] << " )\n";
  os << indent << "Capping: " << (this->Capping ? "On\n" : "Off\n");
  os << indent << "Output Points Precision: " << this->OutputPointsPrecision << "\n";
}
VTK_ABI_NAMESPACE_END
