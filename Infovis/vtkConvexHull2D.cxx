/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkConvexHull2D.cxx

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/

#include "vtkConvexHull2D.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkCoordinate.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointsProjectedHull.h"
#include "vtkPolygon.h"
#include "vtkPolyLine.h"
#include "vtkRenderer.h"
#include "vtkSmartPointer.h"
#include "vtkTransform.h"
#include "vtkTransformPolyDataFilter.h"

vtkStandardNewMacro(vtkConvexHull2D);

//-----------------------------------------------------------------------------
vtkConvexHull2D::vtkConvexHull2D()
{
  this->SetNumberOfOutputPorts(2);
  this->ScaleFactor = 1.0;
  this->Outline = false;
  this->MinHullSizeInWorld = 1.0;
  this->MinHullSizeInDisplay = 10;
  this->HullShape = vtkConvexHull2D::ConvexHull;
  this->Renderer = NULL;

  this->Coordinate = vtkSmartPointer<vtkCoordinate>::New();
  this->Transform = vtkSmartPointer<vtkTransform>::New();
  this->OutputTransform = vtkSmartPointer<vtkTransform>::New();
  this->OutputTransformFilter = vtkSmartPointer<vtkTransformPolyDataFilter>::New();
  this->OutputTransformFilter->SetTransform(this->OutputTransform);
  this->OutlineSource = vtkSmartPointer<vtkPolyLine>::New();
  this->HullSource = vtkSmartPointer<vtkPolygon>::New();
}

//-----------------------------------------------------------------------------
vtkConvexHull2D::~vtkConvexHull2D()
{
  this->SetRenderer(0);
}

//-----------------------------------------------------------------------------
void vtkConvexHull2D::CalculateBoundingRectangle(vtkPoints* inPoints,
  vtkPoints* outPoints, double minimumHullSize)
{
  minimumHullSize /= 2.0;
  inPoints->ComputeBounds();
  double bounds[6];
  inPoints->GetBounds(bounds);

  double xDeficit = minimumHullSize - (bounds[1] - bounds[0]);
  if (xDeficit > 0.0)
    {
    bounds[0] -= xDeficit;
    bounds[1] += xDeficit;
    }

  double yDeficit = minimumHullSize - (bounds[3] - bounds[2]);
  if (yDeficit > 0.0)
    {
    bounds[2] -= yDeficit;
    bounds[3] += yDeficit;
    }

  outPoints->SetNumberOfPoints(4);
  outPoints->SetPoint(0, bounds[0], bounds[2], 0.0);
  outPoints->SetPoint(1, bounds[1], bounds[2], 0.0);
  outPoints->SetPoint(2, bounds[1], bounds[3], 0.0);
  outPoints->SetPoint(3, bounds[0], bounds[3], 0.0);
}

//-----------------------------------------------------------------------------
void vtkConvexHull2D::CalculateConvexHull(vtkPoints* inPoints,
  vtkPoints* outPoints, double minimumHullSize)
{
  if (inPoints->GetNumberOfPoints() == 1 ||
    inPoints->GetNumberOfPoints() == 2)
    {
    vtkConvexHull2D::CalculateBoundingRectangle(inPoints, outPoints,
     minimumHullSize);
    }
  else if (inPoints->GetNumberOfPoints() >= 3)
    {
    vtkPointsProjectedHull* ppHull = vtkPointsProjectedHull::New();
    ppHull->ShallowCopy(inPoints);
    int numHullPoints = ppHull->GetSizeCCWHullZ();
    double* pts = new double[2 * numHullPoints];
    ppHull->GetCCWHullZ(pts, numHullPoints);

    vtkPoints* hullPoints = vtkPoints::New();
    hullPoints->SetNumberOfPoints(numHullPoints);
    for (vtkIdType i = 0; i < numHullPoints; ++i)
      {
      hullPoints->SetPoint(i, pts[2 * i], pts[2 * i + 1], 0.0);
      }
    ppHull->Delete();
    delete[] pts;

    if (numHullPoints < 3)
      {
      vtkConvexHull2D::CalculateBoundingRectangle(hullPoints, outPoints,
        minimumHullSize);
      return;
      }

    double bounds[6];
    hullPoints->GetBounds(bounds);
    double xScale = std::max(1.0, minimumHullSize / (bounds[1] - bounds[0]));
    double yScale = std::max(1.0, minimumHullSize / (bounds[3] - bounds[2]));
    if (xScale > 1.0 || yScale > 1.0)
      {
      double scale[3] = {xScale, yScale, 1.0};
      double translate[3] = {
        (bounds[0] + (bounds[1] - bounds[0]) / 2.0),
        (bounds[2] + (bounds[3] - bounds[2]) / 2.0), 0.0};

      vtkTransform* transform = vtkTransform::New();
      transform->Translate(translate);
      transform->Scale(scale);
      transform->Translate(-translate[0], -translate[1], -translate[2]);
      transform->TransformPoints(hullPoints, outPoints);
      transform->Delete();
      }
    else
      {
      outPoints->ShallowCopy(hullPoints);
      }
    hullPoints->Delete();
    }
}

//-----------------------------------------------------------------------------
void vtkConvexHull2D::ResizeHullToMinimumInDisplay(vtkPolyData* hullPolyData)
{
  if (this->Renderer && this->Renderer->IsActiveCameraCreated())
      {
      double bounds[6];
      hullPolyData->ComputeBounds();
      hullPolyData->GetBounds(bounds);

      double leftBottom[2];
      double rightTop[2];
      double* coord;
      this->Coordinate->SetCoordinateSystemToWorld();
      this->Coordinate->SetValue(bounds[0], bounds[2], 0.0);
      coord = this->Coordinate->GetComputedDoubleDisplayValue(this->Renderer);
      leftBottom[0] = coord[0];
      leftBottom[1] = coord[1];
      this->Coordinate->SetValue(bounds[1], bounds[3], 0.0);
      coord = this->Coordinate->GetComputedDoubleDisplayValue(this->Renderer);
      rightTop[0] = coord[0];
      rightTop[1] = coord[1];
      double currentDisplaySize[2] = {rightTop[0] - leftBottom[0],
        rightTop[1] - leftBottom[1]};

      if (currentDisplaySize[0] == 0.0 || currentDisplaySize[1] == 0.0)
        {
        vtkWarningMacro(<< "Can not scale a hull with zero display area.")
        return;
        }

      if (currentDisplaySize[0] < this->MinHullSizeInDisplay ||
        currentDisplaySize[1] < this->MinHullSizeInDisplay)
        {
        double scaleFx = std::max(1.0, this->MinHullSizeInDisplay
          / currentDisplaySize[0]);
        double scaleFy = std::max(1.0, this->MinHullSizeInDisplay
          / currentDisplaySize[1]);
        double scale[3] = {scaleFx, scaleFy, 1.0};
        double translate[3] = {
          (bounds[0] + (bounds[1] - bounds[0]) / 2.0),
          (bounds[2] + (bounds[3] - bounds[2]) / 2.0), 0.0};

        this->Transform->Identity();
        this->Transform->Translate(translate);
        this->Transform->Scale(scale);
        this->Transform->Translate(-translate[0], -translate[1], -translate[2]);

        vtkPoints* outPoints = vtkPoints::New();
        this->Transform->TransformPoints(hullPolyData->GetPoints(), outPoints);
        hullPolyData->SetPoints(outPoints);
        outPoints->Delete();
        }
      }
}

//-----------------------------------------------------------------------------
void vtkConvexHull2D::SetRenderer(vtkRenderer* renderer)
{
  this->Renderer = renderer;
  this->Modified();
}

//-----------------------------------------------------------------------------
vtkRenderer* vtkConvexHull2D::GetRenderer()
{
  return this->Renderer;
}

//-----------------------------------------------------------------------------
unsigned long vtkConvexHull2D::GetMTime()
{
  if (this->Renderer)
    {
    return this->Renderer->GetMTime();
    }
  else
    {
    return this->MTime;
    }
}

//-----------------------------------------------------------------------------
int vtkConvexHull2D::RequestData(vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector, vtkInformationVector *outputVector)
{
  // Get the input and output.
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);

  vtkPolyData* input = vtkPolyData::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPoints* inputPoints = input->GetPoints();
  if (!inputPoints)
    {
    vtkErrorMacro("Input points needed");
    return 0;
    }

  vtkInformation *outInfo0 = outputVector->GetInformationObject(0);
  vtkInformation *outInfo1 = outputVector->GetInformationObject(1);

  vtkPolyData *outputHull = vtkPolyData::SafeDownCast(outInfo0->Get(
    vtkDataObject::DATA_OBJECT()));
  vtkPolyData *outputOutline = vtkPolyData::SafeDownCast(outInfo1->Get(
    vtkDataObject::DATA_OBJECT()));

  // Create filled polygon
  vtkPoints* hullPoints = vtkPoints::New();
  switch (this->HullShape)
  {
    case vtkConvexHull2D::BoundingRectangle:
      this->CalculateBoundingRectangle(inputPoints, hullPoints,
        this->MinHullSizeInWorld);
      break;
    default: // vtkConvexHull2D::ConvexHull
      this->CalculateConvexHull(inputPoints, hullPoints,
        this->MinHullSizeInWorld);
      break;
  }

  vtkIdType numHullPoints = hullPoints->GetNumberOfPoints();
  vtkIdType* hullPts = new vtkIdType[numHullPoints];
  for (int i = 0; i < numHullPoints; ++i)
    {
    hullPts[i] = i;
    }
  this->HullSource->Initialize(numHullPoints, hullPts, hullPoints);
  delete [] hullPts;

  vtkCellArray* hullCells = vtkCellArray::New();
  hullCells->InsertNextCell(this->HullSource);
  vtkSmartPointer<vtkPolyData> hullPolyData = vtkSmartPointer<vtkPolyData>::New();
  hullPolyData->SetPoints(hullPoints);
  hullPolyData->SetPolys(hullCells);
  hullPoints->Delete();
  hullCells->Delete();

  // Adjust for the scale-factor
  double* centre = hullPolyData->GetCenter();
  this->OutputTransform->Identity();
  this->OutputTransform->Translate(centre);
  this->OutputTransform->Scale(this->ScaleFactor, this->ScaleFactor, this->ScaleFactor);
  this->OutputTransform->Translate(-centre[0], -centre[1], -centre[2]);
  this->OutputTransformFilter->SetInput(hullPolyData);
  this->OutputTransformFilter->Update();
  hullPolyData = this->OutputTransformFilter->GetOutput();

  // Account for current camera zoom level
  this->ResizeHullToMinimumInDisplay(hullPolyData);

  // Copy hull to output
  outputHull->ShallowCopy(hullPolyData);

  if (this->Outline)
    {
    vtkIdType numOutlinePoints = outputHull->GetNumberOfPoints();
    vtkIdType* outlinePts = new vtkIdType[numOutlinePoints + 1];
    for (int i = 0; i < numOutlinePoints; ++i)
      {
      outlinePts[i] = i;
      }
    outlinePts[numOutlinePoints] = outlinePts[0];

    this->OutlineSource->Initialize(numOutlinePoints + 1, outlinePts, outputHull->GetPoints());

    vtkSmartPointer<vtkPolyData> outlinePolyData =
      vtkSmartPointer<vtkPolyData>::New();
    vtkCellArray* outlineCells = vtkCellArray::New();
    outlineCells->InsertNextCell(this->OutlineSource);
    outlinePolyData->SetPoints(outputHull->GetPoints());
    outlinePolyData->SetLines(outlineCells);
    outlineCells->Delete();
    delete[] outlinePts;

    // Copy outline to output
    outputOutline->ShallowCopy(outlinePolyData);
    }
  return 1;
}

//-----------------------------------------------------------------------------
void vtkConvexHull2D::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "ScaleFactor: " << this->ScaleFactor << "\n";
  os << indent << "Outline: " << (this->Outline ? "On" : "Off") << "\n";
  os << indent << "HullShape: ";
  switch (this->HullShape)
    {
    case ConvexHull:
      os << "ConvexHull\n";
      break;
    case BoundingRectangle:
      os << "BoundingRectangle\n";
      break;
    default:
      os << "Unknown\n";
      break;
    }
  os << indent << "MinHullSizeInDisplay: " << this->MinHullSizeInDisplay << "\n";
  os << indent << "MinHullSizeInWorld: " << this->MinHullSizeInWorld << "\n";
  os << indent << "Renderer: ";
  if (this->Renderer)
    {
    os << endl;
    this->Renderer->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "(none)" << endl;
    }
}
