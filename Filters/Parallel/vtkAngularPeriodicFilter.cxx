/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPeriodicFiler.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

    This software is distributed WITHOUT ANY WARRANTY; without even
    the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
    PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkAngularPeriodicFilter.h"

#include "vtkAngularPeriodicDataArray.h"
#include "vtkCellData.h"
#include "vtkDoubleArray.h"
#include "vtkFieldData.h"
#include "vtkFloatArray.h"
#include "vtkInformation.h"
#include "vtkMath.h"
#include "vtkMultiPieceDataSet.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPointSet.h"
#include "vtkPolyData.h"
#include "vtkStructuredGrid.h"
#include "vtkTransform.h"
#include "vtkTransformFilter.h"
#include "vtkUnstructuredGrid.h"

#include <sstream>

vtkStandardNewMacro(vtkAngularPeriodicFilter);

//----------------------------------------------------------------------------
vtkAngularPeriodicFilter::vtkAngularPeriodicFilter()
{
  this->ComputeRotationsOnTheFly = true;
  this->RotationMode = VTK_ROTATION_MODE_DIRECT_ANGLE;
  this->RotationAngle = 180.;
  this->RotationArrayName = 0;
  this->RotationAxis = static_cast<int>(VTK_PERIODIC_ARRAY_AXIS_X);
  this->Center[0] = 0;
  this->Center[1] = 0;
  this->Center[2] = 0;
}

//----------------------------------------------------------------------------
vtkAngularPeriodicFilter::~vtkAngularPeriodicFilter()
{
  this->SetRotationArrayName(0);
}

//----------------------------------------------------------------------------
void vtkAngularPeriodicFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Compute Rotations on-the-fly: " << this->ComputeRotationsOnTheFly
               << endl;
  if (this->RotationMode == VTK_ROTATION_MODE_DIRECT_ANGLE)
  {
    os << indent << "Rotation Mode: Direct Angle" << endl;
    os << indent << "Rotation Angle: " << this->RotationAngle << endl;
  }
  else
  {
    os << indent << "Rotation Mode: Array Value" << endl;
    os << indent << "Rotation Angle Array Name: " << this->RotationArrayName << endl;
  }
  switch (this->RotationAxis)
  {
    case VTK_PERIODIC_ARRAY_AXIS_X:
      os << indent << "Rotation Axis: X" << endl;
      break;
    case VTK_PERIODIC_ARRAY_AXIS_Y:
      os << indent << "Rotation Axis: Y" << endl;
      break;
    case VTK_PERIODIC_ARRAY_AXIS_Z:
      os << indent << "Rotation Axis: Z" << endl;
      break;
    default:
      break;
  }
}

//----------------------------------------------------------------------------
void vtkAngularPeriodicFilter::SetRotationAxisToX()
{
  this->SetRotationAxis(VTK_PERIODIC_ARRAY_AXIS_X);
}

//----------------------------------------------------------------------------
void vtkAngularPeriodicFilter::SetRotationAxisToY()
{
  this->SetRotationAxis(VTK_PERIODIC_ARRAY_AXIS_Y);
}

//----------------------------------------------------------------------------
void vtkAngularPeriodicFilter::SetRotationAxisToZ()
{
  this->SetRotationAxis(VTK_PERIODIC_ARRAY_AXIS_Z);
}

//----------------------------------------------------------------------------
void vtkAngularPeriodicFilter::CreatePeriodicDataSet(
  vtkCompositeDataIterator* loc,
  vtkCompositeDataSet* output,
  vtkCompositeDataSet* input)
{
  vtkDataObject* inputNode = input->GetDataSet(loc);
  vtkNew<vtkMultiPieceDataSet> multiPiece;

  // Number of periods
  int periodsNb = 0;

  // Rotation angle in degree
  double angle = this->GetRotationAngle();
  switch (this->GetRotationMode())
  {
    case VTK_ROTATION_MODE_DIRECT_ANGLE:
      break;
    case VTK_ROTATION_MODE_ARRAY_VALUE:
    {
      if (inputNode != NULL)
      {
        vtkDataArray* angleArray =
          inputNode->GetFieldData()->GetArray(this->GetRotationArrayName());
        if (!angleArray)
        {
          vtkErrorMacro(<< "Bad rotation mode.");
          return;
        }
        double angleRad = angleArray->GetTuple1(0);
        angle = vtkMath::DegreesFromRadians(angleRad);
      }
      else
      {
        angle = 360;
      }
      break;
    }
    default:
    {
      vtkErrorMacro(<< "Bad rotation mode.");
      return;
    }
  }

  switch (this->GetIterationMode())
  {
    case VTK_ITERATION_MODE_DIRECT_NB:
    {
      periodsNb = this->GetNumberOfPeriods();
      break;
    }

    case VTK_ITERATION_MODE_MAX:
    {
      periodsNb = vtkMath::Round(360. / std::abs(angle));
      break;
    }

    default:
    {
      vtkErrorMacro(<< "Bad iteration mode.");
      return;
    }
  }

  multiPiece->SetNumberOfPieces(periodsNb);
  if (periodsNb > 0 && inputNode != NULL)
  {
    // Shallow copy the first piece, it is not transformed
    vtkDataObject* firstDataSet = inputNode->NewInstance();
    firstDataSet->ShallowCopy(inputNode);
    multiPiece->SetPiece(0, firstDataSet);
    firstDataSet->Delete();
    this->GeneratePieceName(input, loc, multiPiece.Get(), 0);

    for (vtkIdType iPiece = 1; iPiece < periodsNb; iPiece++)
    {
      this->AppendPeriodicPiece(angle, iPiece, inputNode, multiPiece.Get());
      this->GeneratePieceName(input, loc, multiPiece.Get(), iPiece);
    }
  }
  this->PeriodNumbers.push_back(periodsNb);
  output->SetDataSet(loc, multiPiece.Get());
}

//----------------------------------------------------------------------------
void vtkAngularPeriodicFilter::SetPeriodNumber(vtkCompositeDataIterator* loc,
                             vtkCompositeDataSet* output,
                             int nbPeriod)
{
  vtkMultiPieceDataSet* mp = vtkMultiPieceDataSet::SafeDownCast(output->GetDataSet(loc));
  if (mp)
  {
    mp->SetNumberOfPieces(nbPeriod);
  }
  else
  {
    vtkErrorMacro(<< "Setting period on a non existent vtkMultiPieceDataSet");
  }
}

//----------------------------------------------------------------------------
void vtkAngularPeriodicFilter::AppendPeriodicPiece(double angle,
  vtkIdType iPiece, vtkDataObject* inputNode, vtkMultiPieceDataSet* multiPiece)
{
  vtkPointSet* dataset = vtkPointSet::SafeDownCast(inputNode);
  vtkPointSet* transformedDataset = NULL;

  int pieceAlterner =  ((iPiece % 2) * 2 - 1) * ((iPiece + 1) / 2);
  double pieceAngle = angle * pieceAlterner;

  // MappedData supported type are pointset
  if (dataset)
  {
    transformedDataset = dataset->NewInstance();

    // Transform periodic points and cells
    this->ComputePeriodicMesh(dataset, transformedDataset, pieceAngle);
    multiPiece->SetPiece(iPiece, transformedDataset);
    transformedDataset->Delete();
  }
  else
  {
    // Legacy non mapped code, for unsuported type dataset
    vtkWarningMacro(
      "Unsupported Dataset Type for mapped array, using vtkTransformFilter instead.");
    vtkNew<vtkTransform> transform;
    switch (this->RotationAxis)
    {
      case VTK_PERIODIC_ARRAY_AXIS_X:
        transform->RotateX(pieceAngle);
        break;
      case VTK_PERIODIC_ARRAY_AXIS_Y:
        transform->RotateY(pieceAngle);
        break;
      case VTK_PERIODIC_ARRAY_AXIS_Z:
        transform->RotateZ(pieceAngle);
        break;
    }

    vtkNew<vtkTransformFilter> transformFilter;
    transformFilter->SetInputData(inputNode);
    transformFilter->SetTransform(transform.Get());
    transformFilter->Update();

    multiPiece->SetPiece(iPiece, transformFilter->GetOutput());
  }
}

//----------------------------------------------------------------------------
vtkDataArray* vtkAngularPeriodicFilter::TransformDataArray(
  vtkDataArray* inputArray, double angle, bool useCenter, bool normalize)
{
  vtkDataArray* periodicArray = 0;
  switch (inputArray->GetDataType())
  {
    case VTK_FLOAT:
    {
      vtkAngularPeriodicDataArray<float>* pArray =
        vtkAngularPeriodicDataArray<float>::New();
      pArray->SetAxis(this->RotationAxis);
      pArray->SetAngle(angle);
      if (useCenter)
      {
        pArray->SetCenter(this->Center);
      }
      pArray->SetNormalize(normalize);
      pArray->InitializeArray(vtkArrayDownCast<vtkFloatArray>(inputArray));
      if (this->ComputeRotationsOnTheFly)
      {
        periodicArray = pArray;
      }
      else
      {
        vtkFloatArray *concrete = vtkFloatArray::New();
        concrete->DeepCopy(pArray); // instantiate the array
        periodicArray = concrete;
        pArray->Delete();
      }
      break;
    }
    case VTK_DOUBLE:
    {
      vtkAngularPeriodicDataArray<double>* pArray =
        vtkAngularPeriodicDataArray<double>::New();
      pArray->SetAxis(this->RotationAxis);
      pArray->SetAngle(angle);
      if (useCenter)
      {
        pArray->SetCenter(this->Center);
      }
      pArray->SetNormalize(normalize);
      pArray->InitializeArray(vtkArrayDownCast<vtkDoubleArray>(inputArray));
      if (this->ComputeRotationsOnTheFly)
      {
        periodicArray = pArray;
      }
      else
      {
        vtkDoubleArray *concrete = vtkDoubleArray::New();
        concrete->DeepCopy(pArray); // instantiate the array
        periodicArray = concrete;
        pArray->Delete();
      }
      break;
    }
    default:
    {
      vtkErrorMacro("Unknown data type " << inputArray->GetDataType());
      periodicArray = vtkDataArray::CreateDataArray(inputArray->GetDataType());
      periodicArray->DeepCopy(inputArray);
      break;
    }
  }

  return periodicArray;
}

//----------------------------------------------------------------------------
void vtkAngularPeriodicFilter::ComputeAngularPeriodicData(
  vtkDataSetAttributes* data, vtkDataSetAttributes* transformedData, double angle)
{
  for (int i = 0; i < data->GetNumberOfArrays(); i++)
  {
    int attribute = data->IsArrayAnAttribute(i);
    vtkDataArray* array = data->GetArray(i);
    vtkDataArray* transformedArray;
    // Perdiodic copy of vector (3 components) or symmectric tensor (6 component, converted to 9 )
    // or tensor (9 components) data
    int numComp = array->GetNumberOfComponents();
    if (numComp == 3 || numComp == 6 || numComp == 9)
    {
      transformedArray = this->TransformDataArray(array, angle, false,
        attribute == vtkDataSetAttributes::NORMALS);
    }
    else
    {
      transformedArray = array;
      array->Register(0);
    }
    transformedData->AddArray(transformedArray);
    if (attribute >= 0)
    {
      transformedData->SetAttribute(transformedArray, attribute);
    }
    transformedArray->Delete();
  }
}

//----------------------------------------------------------------------------
void vtkAngularPeriodicFilter::ComputePeriodicMesh(vtkPointSet* dataset,
  vtkPointSet* transformedDataset, double angle)
{
  // Shallow copy data structure
  transformedDataset->CopyStructure(dataset);

  // Transform points coordinates array
  vtkPoints* points = dataset->GetPoints();
  if (points != NULL)
  {
    vtkDataArray* pointArray = dataset->GetPoints()->GetData();
    vtkNew<vtkPoints> rotatedPoints;
    vtkDataArray* transformedArray = this->TransformDataArray(pointArray, angle, true);
    rotatedPoints->SetData(transformedArray);
    transformedArray->Delete();
    // Set the points
    transformedDataset->SetPoints(rotatedPoints.Get());
  }

  // Transform point data
  this->ComputeAngularPeriodicData(
    dataset->GetPointData(), transformedDataset->GetPointData(), angle);

  // Transform cell data
  this->ComputeAngularPeriodicData(
    dataset->GetCellData(), transformedDataset->GetCellData(), angle);

  // Shallow copy field data
  transformedDataset->GetFieldData()->ShallowCopy(dataset->GetFieldData());
}

//----------------------------------------------------------------------------
int vtkAngularPeriodicFilter::RequestData(vtkInformation *request,
                                   vtkInformationVector **inputVector,
                                   vtkInformationVector *outputVector)
{
  if (this->GetRotationMode() == VTK_ROTATION_MODE_ARRAY_VALUE &&
      this->GetIterationMode() == VTK_ITERATION_MODE_MAX)
  {
    this->ReducePeriodNumbers = true;
  }
  return this->Superclass::RequestData(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
void vtkAngularPeriodicFilter::GeneratePieceName(vtkCompositeDataSet* input,
  vtkCompositeDataIterator* inputLoc, vtkMultiPieceDataSet* output, vtkIdType outputId)
{
  vtkDataObjectTree* inputTree = vtkDataObjectTree::SafeDownCast(input);
  if (!inputTree)
  {
    return;
  }
  std::ostringstream ss;
  const char* parentName =
    inputTree->GetMetaData(inputLoc)->Get(vtkCompositeDataSet::NAME());
  if (parentName)
  {
    ss << parentName;
  }
  else
  {
    ss << "Piece";
  }
  ss << "_period" << outputId;
  output->GetMetaData(outputId)->Set(vtkCompositeDataSet::NAME(), ss.str().c_str());
}
