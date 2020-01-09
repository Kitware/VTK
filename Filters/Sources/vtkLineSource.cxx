/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLineSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkLineSource.h"

#include "vtkCellArray.h"
#include "vtkFloatArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkVector.h"
#include "vtkVectorOperators.h"

#include <cmath>

vtkStandardNewMacro(vtkLineSource);
vtkCxxSetObjectMacro(vtkLineSource, Points, vtkPoints);

// ----------------------------------------------------------------------
vtkLineSource::vtkLineSource(int res)
{
  this->Point1[0] = -.5;
  this->Point1[1] = .0;
  this->Point1[2] = .0;

  this->Point2[0] = .5;
  this->Point2[1] = .0;
  this->Point2[2] = .0;

  this->Points = nullptr;

  this->Resolution = (res < 1 ? 1 : res);
  this->OutputPointsPrecision = SINGLE_PRECISION;
  this->UseRegularRefinement = true;
  this->SetNumberOfInputPorts(0);
}

// ----------------------------------------------------------------------
vtkLineSource::~vtkLineSource()
{
  this->SetPoints(nullptr);
}

// ----------------------------------------------------------------------
void vtkLineSource::SetNumberOfRefinementRatios(int val)
{
  if (val < 0)
  {
    vtkErrorMacro("Value cannot be negative: " << val);
  }
  else if (static_cast<int>(this->RefinementRatios.size()) != val)
  {
    this->RefinementRatios.resize(val);
    this->Modified();
  }
}

// ----------------------------------------------------------------------
void vtkLineSource::SetRefinementRatio(int index, double value)
{
  if (index >= 0 && index < static_cast<int>(this->RefinementRatios.size()))
  {
    if (this->RefinementRatios[index] != value)
    {
      this->RefinementRatios[index] = value;
      this->Modified();
    }
  }
  else
  {
    vtkErrorMacro("Invalid index: " << index);
  }
}

// ----------------------------------------------------------------------
int vtkLineSource::GetNumberOfRefinementRatios()
{
  return static_cast<int>(this->RefinementRatios.size());
}

// ----------------------------------------------------------------------
double vtkLineSource::GetRefinementRatio(int index)
{
  if (index >= 0 && index < static_cast<int>(this->RefinementRatios.size()))
  {
    return this->RefinementRatios[index];
  }
  vtkErrorMacro("Invalid index: " << index);
  return 0.0;
}

// ----------------------------------------------------------------------
int vtkLineSource::RequestInformation(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector)
{
  // get the info object
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  outInfo->Set(CAN_HANDLE_PIECE_REQUEST(), 1);
  return 1;
}

// ----------------------------------------------------------------------
int vtkLineSource::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector)
{
  // Reject meaningless parameterizations
  const vtkIdType nSegments = this->Points ? this->Points->GetNumberOfPoints() - 1 : 1;
  if (nSegments < 1)
  {
    vtkWarningMacro(<< "Cannot define a broken line with given input.");
    return 0;
  }

  // get the info object
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  if (outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()) > 0)
  {
    // we'll only produce data for piece 0, and produce empty datasets on
    // others since splitting a line source into pieces is generally not what's
    // expected.
    return 1;
  }

  // get the output
  vtkPolyData* output = vtkPolyData::GetData(outInfo);

  // This is a vector giving the positions of intermediate points. Thus, if empty, only the
  // end points for each line segment are generated.
  std::vector<double> refinements;
  if (this->UseRegularRefinement)
  {
    assert(this->Resolution >= 1);
    refinements.reserve(this->Resolution + 1);
    for (int cc = 0; cc < this->Resolution; ++cc)
    {
      refinements.push_back(static_cast<double>(cc) / this->Resolution);
    }
    refinements.push_back(1.0);
  }
  else
  {
    refinements = this->RefinementRatios;
  }

  vtkSmartPointer<vtkPoints> pts = this->Points;
  if (this->Points == nullptr)
  {
    // using end points.
    pts = vtkSmartPointer<vtkPoints>::New();
    pts->SetDataType(VTK_DOUBLE);
    pts->SetNumberOfPoints(2);
    pts->SetPoint(0, this->Point1);
    pts->SetPoint(1, this->Point2);
  }

  // Create and allocate lines
  vtkIdType numPts = nSegments * static_cast<vtkIdType>(refinements.size());

  // Create and allocate points
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

  // Generate points

  // Point index offset for fast insertion
  vtkIdType offset = 0;

  // Iterate over segments
  for (vtkIdType seg = 0; seg < nSegments; ++seg)
  {
    assert((seg + 1) < pts->GetNumberOfPoints());

    // Get coordinates of endpoints
    vtkVector3d point1, point2;

    pts->GetPoint(seg, point1.GetData());
    pts->GetPoint(seg + 1, point2.GetData());

    // Calculate segment vector
    const vtkVector3d v = point2 - point1;

    // Generate points along segment
    for (size_t i = 0; i < refinements.size(); ++i)
    {
      if (seg > 0 && i == 0 && refinements.front() == 0.0 && refinements.back() == 1.0)
      {
        // skip adding first point in the segment if it is same as the last point
        // from previously added segment.
        continue;
      }
      const vtkVector3d pt = point1 + refinements[i] * v;
      newPoints->InsertPoint(offset, pt.GetData());
      ++offset;
    }
  } // seg

  // update number of points estimate.
  numPts = offset;

  //  Generate lines
  vtkNew<vtkCellArray> newLines;
  newLines->AllocateEstimate(1, numPts);
  newLines->InsertNextCell(numPts);
  for (vtkIdType i = 0; i < numPts; ++i)
  {
    newLines->InsertCellPoint(i);
  }

  // Generate texture coordinates
  vtkNew<vtkFloatArray> newTCoords;
  newTCoords->SetNumberOfComponents(2);
  newTCoords->SetNumberOfTuples(numPts);
  newTCoords->SetName("Texture Coordinates");
  newTCoords->FillValue(0.0f);

  float length_sum = 0.0f;
  for (vtkIdType cc = 1; cc < numPts; ++cc)
  {
    vtkVector3d p1, p2;
    newPoints->GetPoint(cc - 1, p1.GetData());
    newPoints->GetPoint(cc, p2.GetData());

    length_sum += static_cast<float>((p2 - p1).Norm());
    newTCoords->SetTypedComponent(cc, 0, length_sum);
  }

  // now normalize the tcoord
  if (length_sum)
  {
    for (vtkIdType cc = 1; cc < numPts; ++cc)
    {
      newTCoords->SetTypedComponent(cc, 0, newTCoords->GetTypedComponent(cc, 0) / length_sum);
    }
  }

  // Update ourselves and release memory
  output->SetPoints(newPoints);
  output->GetPointData()->SetTCoords(newTCoords);
  output->SetLines(newLines);
  return 1;
}

// ----------------------------------------------------------------------
void vtkLineSource::SetPoint1(float point1f[3])
{
  double point1d[3];
  point1d[0] = point1f[0];
  point1d[1] = point1f[1];
  point1d[2] = point1f[2];
  SetPoint1(point1d);
}

// ----------------------------------------------------------------------
void vtkLineSource::SetPoint2(float point2f[3])
{
  double point2d[3];
  point2d[0] = point2f[0];
  point2d[1] = point2f[1];
  point2d[2] = point2f[2];
  SetPoint2(point2d);
}

// ----------------------------------------------------------------------
void vtkLineSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Resolution: " << this->Resolution << "\n";

  os << indent << "Point 1: (" << this->Point1[0] << ", " << this->Point1[1] << ", "
     << this->Point1[2] << ")\n";

  os << indent << "Point 2: (" << this->Point2[0] << ", " << this->Point2[1] << ", "
     << this->Point2[2] << ")\n";

  os << indent << "Points: ";
  if (this->Points)
  {
    this->Points->PrintSelf(os, indent);
  }
  else
  {
    os << "(none)" << endl;
  }
  os << indent << "UseRegularRefinement: " << this->UseRegularRefinement << endl;
  os << indent << "RefinementRatios: [";
  for (const auto& r : this->RefinementRatios)
  {
    os << r << " ";
  }
  os << "]" << endl;

  os << indent << "Output Points Precision: " << this->OutputPointsPrecision << "\n";
}
