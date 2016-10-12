/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWarpScalar.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkWarpScalar.h"

#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkDataSetAttributes.h"
#include "vtkImageData.h"
#include "vtkImageDataToPointSet.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPointSet.h"
#include "vtkPoints.h"
#include "vtkRectilinearGrid.h"
#include "vtkRectilinearGridToPointSet.h"

#include "vtkNew.h"
#include "vtkSmartPointer.h"

vtkStandardNewMacro(vtkWarpScalar);

//----------------------------------------------------------------------------
vtkWarpScalar::vtkWarpScalar()
{
  this->ScaleFactor = 1.0;
  this->UseNormal = 0;
  this->Normal[0] = 0.0;
  this->Normal[1] = 0.0;
  this->Normal[2] = 1.0;
  this->XYPlane = 0;

  // by default process active point scalars
  this->SetInputArrayToProcess(0,0,0,vtkDataObject::FIELD_ASSOCIATION_POINTS,
                               vtkDataSetAttributes::SCALARS);
}

//----------------------------------------------------------------------------
vtkWarpScalar::~vtkWarpScalar()
{
}

//----------------------------------------------------------------------------
double *vtkWarpScalar::DataNormal(vtkIdType id, vtkDataArray *normals)
{
  return normals->GetTuple(id);
}

//----------------------------------------------------------------------------
double *vtkWarpScalar::InstanceNormal(vtkIdType vtkNotUsed(id),
                                     vtkDataArray *vtkNotUsed(normals))
{
  return this->Normal;
}

//----------------------------------------------------------------------------
double *vtkWarpScalar::ZNormal(vtkIdType vtkNotUsed(id),
                              vtkDataArray *vtkNotUsed(normals))
{
  static double zNormal[3]={0.0,0.0,1.0};
  return zNormal;
}

//----------------------------------------------------------------------------
int vtkWarpScalar::FillInputPortInformation(int vtkNotUsed(port),
                                            vtkInformation *info)
{
  info->Remove(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE());
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPointSet");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkImageData");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkRectilinearGrid");
  return 1;
}

//----------------------------------------------------------------------------
int vtkWarpScalar::RequestDataObject(vtkInformation *request,
                                     vtkInformationVector **inputVector,
                                     vtkInformationVector *outputVector)
{
  vtkImageData *inImage = vtkImageData::GetData(inputVector[0]);
  vtkRectilinearGrid *inRect = vtkRectilinearGrid::GetData(inputVector[0]);

  if (inImage || inRect)
  {
    vtkStructuredGrid *output = vtkStructuredGrid::GetData(outputVector);
    if (!output)
    {
      vtkNew<vtkStructuredGrid> newOutput;
      outputVector->GetInformationObject(0)->Set(
        vtkDataObject::DATA_OBJECT(), newOutput.GetPointer());
    }
    return 1;
  }
  else
  {
    return this->Superclass::RequestDataObject(request,
                                               inputVector,
                                               outputVector);
  }
}

//----------------------------------------------------------------------------
int vtkWarpScalar::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  vtkSmartPointer<vtkPointSet> input = vtkPointSet::GetData(inputVector[0]);
  vtkPointSet *output = vtkPointSet::GetData(outputVector);

  if (!input)
  {
    // Try converting image data.
    vtkImageData *inImage = vtkImageData::GetData(inputVector[0]);
    if (inImage)
    {
      vtkNew<vtkImageDataToPointSet> image2points;
      image2points->SetInputData(inImage);
      image2points->Update();
      input = image2points->GetOutput();
    }
  }

  if (!input)
  {
    // Try converting rectilinear grid.
    vtkRectilinearGrid *inRect = vtkRectilinearGrid::GetData(inputVector[0]);
    if (inRect)
    {
      vtkNew<vtkRectilinearGridToPointSet> rect2points;
      rect2points->SetInputData(inRect);
      rect2points->Update();
      input = rect2points->GetOutput();
    }
  }

  if (!input)
  {
    vtkErrorMacro(<< "Invalid or missing input");
    return 0;
  }

  vtkPoints *inPts;
  vtkDataArray *inNormals;
  vtkDataArray *inScalars;
  vtkPoints *newPts;
  vtkPointData *pd;
  int i;
  vtkIdType ptId, numPts;
  double x[3], *n, s, newX[3];

  vtkDebugMacro(<<"Warping data with scalars");

  // First, copy the input to the output as a starting point
  output->CopyStructure( input );

  inPts = input->GetPoints();
  pd = input->GetPointData();
  inNormals = pd->GetNormals();

  inScalars = this->GetInputArrayToProcess(0,inputVector);
  if ( !inPts || !inScalars )
  {
    vtkDebugMacro(<<"No data to warp");
    return 1;
  }

  numPts = inPts->GetNumberOfPoints();

  if ( inNormals && !this->UseNormal )
  {
    this->PointNormal = &vtkWarpScalar::DataNormal;
    vtkDebugMacro(<<"Using data normals");
  }
  else if ( this->XYPlane )
  {
    this->PointNormal = &vtkWarpScalar::ZNormal;
    vtkDebugMacro(<<"Using x-y plane normal");
  }
  else
  {
    this->PointNormal = &vtkWarpScalar::InstanceNormal;
    vtkDebugMacro(<<"Using Normal instance variable");
  }

  newPts = vtkPoints::New();
  newPts->SetNumberOfPoints(numPts);

  // Loop over all points, adjusting locations
  //
  for (ptId=0; ptId < numPts; ptId++)
  {
    if ( ! (ptId % 10000) )
    {
      this->UpdateProgress ((double)ptId/numPts);
      if (this->GetAbortExecute())
      {
        break;
      }
    }

    inPts->GetPoint(ptId, x);
    n = (this->*(this->PointNormal))(ptId,inNormals);
    if ( this->XYPlane )
    {
      s = x[2];
    }
    else
    {
      s = inScalars->GetComponent(ptId,0);
    }
    for (i=0; i<3; i++)
    {
      newX[i] = x[i] + this->ScaleFactor * s * n[i];
    }
    newPts->SetPoint(ptId, newX);
  }

  // Update ourselves and release memory
  //
  output->GetPointData()->CopyNormalsOff(); // distorted geometry
  output->GetPointData()->PassData(input->GetPointData());
  output->GetCellData()->CopyNormalsOff(); // distorted geometry
  output->GetCellData()->PassData(input->GetCellData());

  output->SetPoints(newPts);
  newPts->Delete();

  return 1;
}

//----------------------------------------------------------------------------
void vtkWarpScalar::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Scale Factor: " << this->ScaleFactor << "\n";
  os << indent << "Use Normal: " << (this->UseNormal ? "On\n" : "Off\n");
  os << indent << "Normal: (" << this->Normal[0] << ", "
     << this->Normal[1] << ", " << this->Normal[2] << ")\n";
  os << indent << "XY Plane: " << (this->XYPlane ? "On\n" : "Off\n");
}
