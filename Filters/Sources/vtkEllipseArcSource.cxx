/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkEllipseArcSource.cxx

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkEllipseArcSource.h"

#include "vtkCellArray.h"
#include "vtkFloatArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkMathUtilities.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPoints.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkStreamingDemandDrivenPipeline.h"

vtkStandardNewMacro(vtkEllipseArcSource);

// --------------------------------------------------------------------------
vtkEllipseArcSource::vtkEllipseArcSource()
{
  // Default center is origin
  this->Center[0] = 0.0;
  this->Center[1] = 0.0;
  this->Center[2] = 0.0;

  // Default normal vector is unit in Oz direction
  this->Normal[0] = 0.0;
  this->Normal[1] = 0.0;
  this->Normal[2] = 1.0;

  this->MajorRadiusVector[0] = 1.0;
  this->MajorRadiusVector[1] = 0.0;
  this->MajorRadiusVector[2] = 0.0;

  // Default arc is a quarter-circle
  this->StartAngle = 0.0;
  this->SegmentAngle = 90.;

  // Default resolution
  this->Resolution = 100;

  this->OutputPointsPrecision = SINGLE_PRECISION;

  // Default Ratio
  this->Ratio = 1.0;

  // This is a source
  this->SetNumberOfInputPorts(0);
}

// --------------------------------------------------------------------------
int vtkEllipseArcSource::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector),
  vtkInformationVector* outputVector)
{
  int numLines = this->Resolution;
  int numPts = this->Resolution + 1;
  double tc[3] = { 0.0, 0.0, 0.0 };

  // get the info object
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the ouptut
  vtkPolyData* output =
    vtkPolyData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  double a = 1.0, b = 1.0;
  double majorRadiusVect[3];
  double orthogonalVect[3];
  double startAngleRad = 0.0, segmentAngleRad, angleIncRad;

  // make sure the normal vector is normalized
  vtkMath::Normalize(this->Normal);

  // get orthonal vector between user-defined major radius and this->Normal
  vtkMath::Cross(this->Normal, this->MajorRadiusVector, orthogonalVect);
  if (vtkMathUtilities::FuzzyCompare(vtkMath::Norm(orthogonalVect), 0.0))
  {
    vtkErrorMacro(<< "Ellipse normal vector and major radius axis are collinear");
    return 0;
  }

  vtkMath::Normalize(orthogonalVect);

  // get major Radius Vector adjusted to be on the plane defined by this->Normal
  vtkMath::Cross(orthogonalVect, this->Normal, majorRadiusVect);

  vtkMath::Normalize(majorRadiusVect);

  // set the major and minor Radius values
  a = vtkMath::Norm(this->MajorRadiusVector);
  b = a * this->Ratio;

  // user-defined angles (positive only)
  startAngleRad = vtkMath::RadiansFromDegrees(this->StartAngle);
  if (startAngleRad < 0)
  {
    startAngleRad += 2.0 * vtkMath::Pi();
  }

  segmentAngleRad = vtkMath::RadiansFromDegrees(this->SegmentAngle);

  // Calcute angle increment
  angleIncRad = segmentAngleRad / this->Resolution;

  // Now create arc points and segments
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
  vtkNew<vtkFloatArray> newTCoords;
  newTCoords->SetNumberOfComponents(2);
  newTCoords->Allocate(2 * numPts);
  newTCoords->SetName("Texture Coordinates");
  vtkNew<vtkCellArray> newLines;
  newLines->Allocate(newLines->EstimateSize(numLines, 2));

  double theta = startAngleRad;
  double thetaEllipse;
  // Iterate over angle increments
  for (int i = 0; i <= this->Resolution; ++i, theta += angleIncRad)
  {
    // convert section angle to an angle applied to ellipse equation.
    // the result point with the ellipse angle, will be located on section angle
    int quotient = (int)(theta / (2.0 * vtkMath::Pi()));
    theta = theta - quotient * 2.0 * vtkMath::Pi();

    // result range: -pi/2, pi/2
    thetaEllipse = atan(tan(theta) / this->Ratio);

    //theta range: 0, 2 * pi
    if (theta > vtkMath::Pi() / 2 && theta <= vtkMath::Pi())
    {
      thetaEllipse += vtkMath::Pi();
    }
    else if (theta > vtkMath::Pi() && theta <= 1.5 * vtkMath::Pi())
    {
      thetaEllipse -= vtkMath::Pi();
    }

    const double cosTheta = cos(thetaEllipse);
    const double sinTheta = sin(thetaEllipse);
    double p[3] =
      {
      this->Center[0] + a * cosTheta * majorRadiusVect[0] + b * sinTheta * orthogonalVect[0],
      this->Center[1] + a * cosTheta * majorRadiusVect[1] + b * sinTheta * orthogonalVect[1],
      this->Center[2] + a * cosTheta * majorRadiusVect[2] + b * sinTheta * orthogonalVect[2]
      };

    tc[0] = static_cast<double>(i) / this->Resolution;
    newPoints->InsertPoint(i , p);
    newTCoords->InsertTuple(i, tc);
  }

  newLines->InsertNextCell(numPts);
  for (int k = 0; k < numPts; ++ k)
  {
    newLines->InsertCellPoint(k);
  }

  output->SetPoints(newPoints.Get());
  output->GetPointData()->SetTCoords(newTCoords.Get());
  output->SetLines(newLines.Get());
  return 1;
}

// --------------------------------------------------------------------------
void vtkEllipseArcSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Resolution: " << this->Resolution << "\n";

  os << indent << "Center: (" << this->Center[0] << ", "
    << this->Center[1] << ", "
    << this->Center[2] << ")\n";

  os << indent << "Normal: (" << this->Normal[0] << ", "
    << this->Normal[1] << ", "
    << this->Normal[2] << ")\n";

  os << indent << "Major Radius Vector: (" << this->MajorRadiusVector[0] << ", "
    << this->MajorRadiusVector[1] << ", "
    << this->MajorRadiusVector[2] << ")\n";

  os << indent << "StartAngle: " << this->StartAngle << "\n";
  os << indent << "SegmentAngle: " << this->SegmentAngle << "\n";

  os << indent << "Resolution: " << this->Resolution << "\n";
  os << indent << "Ratio: " << this->Ratio << "\n";

  os << indent << "Output Points Precision: " << this->OutputPointsPrecision << "\n";
}
