// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkAngularPeriodicFilter.h"

#include "vtkCellData.h"
#include "vtkDoubleArray.h"
#include "vtkFieldData.h"
#include "vtkFloatArray.h"
#include "vtkInformation.h"
#include "vtkMath.h"
#include "vtkMatrix3x3.h"
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

VTK_ABI_NAMESPACE_BEGIN
//------------------------------------------------------------------------------
template <typename ValueType>
class vtkAngularPeriodicFilter::vtkAngularPeriodicBackend
{
  vtkSmartPointer<vtkAOSDataArrayTemplate<ValueType>> Input;
  double AngleInRadians;
  int Axis;
  double Center[3];
  vtkIdType NumberOfComponents;
  bool Normalize;
  vtkNew<vtkMatrix3x3> RotationMatrix;

public:
  vtkAngularPeriodicBackend(vtkAOSDataArrayTemplate<ValueType>* input, double angleDegrees,
    int axis, const double center[3], bool normalize = false)
    : Input(input)
    , Axis(axis)
    , NumberOfComponents(input->GetNumberOfComponents())
    , Normalize(normalize)
  {
    this->AngleInRadians = vtkMath::RadiansFromDegrees(angleDegrees);
    std::copy(center, center + 3, this->Center);

    // Precompute Rotation Matrix
    this->RotationMatrix->Identity();
    int axis0 = (this->Axis + 1) % 3;
    int axis1 = (this->Axis + 2) % 3;
    this->RotationMatrix->SetElement(this->Axis, this->Axis, 1.0);
    this->RotationMatrix->SetElement(axis0, axis0, std::cos(this->AngleInRadians));
    this->RotationMatrix->SetElement(axis0, axis1, -std::sin(this->AngleInRadians));
    this->RotationMatrix->SetElement(axis1, axis0, std::sin(this->AngleInRadians));
    this->RotationMatrix->SetElement(axis1, axis1, std::cos(this->AngleInRadians));
  }

  void Transform(ValueType* pos) const
  {
    if (this->NumberOfComponents == 3)
    {
      // Axis rotation
      int axis0 = (this->Axis + 1) % this->NumberOfComponents;
      int axis1 = (this->Axis + 2) % this->NumberOfComponents;
      double posx = static_cast<double>(pos[axis0]) - this->Center[axis0];
      double posy = static_cast<double>(pos[axis1]) - this->Center[axis1];

      pos[axis0] = this->Center[axis0] +
        static_cast<ValueType>(cos(this->AngleInRadians) * posx - sin(this->AngleInRadians) * posy);
      pos[axis1] = this->Center[axis1] +
        static_cast<ValueType>(sin(this->AngleInRadians) * posx + cos(this->AngleInRadians) * posy);
      if (this->Normalize)
      {
        vtkMath::Normalize(pos);
      }
    }
    else if (this->NumberOfComponents == 6 || this->NumberOfComponents == 9)
    {
      // Template type force a copy to a double array for tensor
      double localPos[9];
      double tmpMat[9];
      double tmpMat2[9];
      std::copy(pos, pos + this->NumberOfComponents, localPos);
      if (this->NumberOfComponents == 6)
      {
        vtkMath::TensorFromSymmetricTensor(localPos);
      }

      vtkMatrix3x3::Transpose(this->RotationMatrix->GetData(), tmpMat);
      vtkMatrix3x3::Multiply3x3(this->RotationMatrix->GetData(), localPos, tmpMat2);
      vtkMatrix3x3::Multiply3x3(tmpMat2, tmpMat, localPos);
      std::copy(localPos, localPos + this->NumberOfComponents, pos);
    }
  }

  void mapTuple(vtkIdType tupleId, ValueType* tuple) const
  {
    this->Input->GetTypedTuple(tupleId, tuple);
    this->Transform(tuple);
  }

  ValueType map(vtkIdType index) const
  {
    const auto div = std::div(index, this->NumberOfComponents);
    ValueType tuple[9];
    this->mapTuple(div.quot, tuple);
    return tuple[div.rem];
  }
};

vtkStandardNewMacro(vtkAngularPeriodicFilter);

//------------------------------------------------------------------------------
vtkAngularPeriodicFilter::vtkAngularPeriodicFilter()
{
  this->ComputeRotationsOnTheFly = true;
  this->RotationMode = VTK_ROTATION_MODE_DIRECT_ANGLE;
  this->RotationAngle = 180.;
  this->RotationArrayName = nullptr;
  this->RotationAxis = 0;
  this->Center[0] = 0;
  this->Center[1] = 0;
  this->Center[2] = 0;
}

//------------------------------------------------------------------------------
vtkAngularPeriodicFilter::~vtkAngularPeriodicFilter()
{
  this->SetRotationArrayName(nullptr);
}

//------------------------------------------------------------------------------
void vtkAngularPeriodicFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Compute Rotations on-the-fly: " << this->ComputeRotationsOnTheFly << endl;
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
    case 0:
      os << indent << "Rotation Axis: X" << endl;
      break;
    case 1:
      os << indent << "Rotation Axis: Y" << endl;
      break;
    case 2:
      os << indent << "Rotation Axis: Z" << endl;
      break;
    default:
      break;
  }
}

//------------------------------------------------------------------------------
void vtkAngularPeriodicFilter::SetRotationAxisToX()
{
  this->SetRotationAxis(0);
}

//------------------------------------------------------------------------------
void vtkAngularPeriodicFilter::SetRotationAxisToY()
{
  this->SetRotationAxis(1);
}

//------------------------------------------------------------------------------
void vtkAngularPeriodicFilter::SetRotationAxisToZ()
{
  this->SetRotationAxis(2);
}

//------------------------------------------------------------------------------
void vtkAngularPeriodicFilter::CreatePeriodicDataSet(
  vtkCompositeDataIterator* loc, vtkCompositeDataSet* output, vtkCompositeDataSet* input)
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
      if (inputNode != nullptr)
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
      periodsNb = static_cast<int>(std::round(360. / std::abs(angle)));
      break;
    }

    default:
    {
      vtkErrorMacro(<< "Bad iteration mode.");
      return;
    }
  }

  multiPiece->SetNumberOfPieces(periodsNb);
  if (periodsNb > 0 && inputNode != nullptr)
  {
    // Shallow copy the first piece, it is not transformed
    vtkDataObject* firstDataSet = inputNode->NewInstance();
    firstDataSet->ShallowCopy(inputNode);
    multiPiece->SetPiece(0, firstDataSet);
    firstDataSet->Delete();
    this->GeneratePieceName(input, loc, multiPiece, 0);

    for (vtkIdType iPiece = 1; iPiece < periodsNb; iPiece++)
    {
      this->AppendPeriodicPiece(angle, iPiece, inputNode, multiPiece);
      this->GeneratePieceName(input, loc, multiPiece, iPiece);
    }
  }
  this->PeriodNumbers.push_back(periodsNb);
  output->SetDataSet(loc, multiPiece);
}

//------------------------------------------------------------------------------
void vtkAngularPeriodicFilter::SetPeriodNumber(
  vtkCompositeDataIterator* loc, vtkCompositeDataSet* output, int nbPeriod)
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

//------------------------------------------------------------------------------
void vtkAngularPeriodicFilter::AppendPeriodicPiece(
  double angle, vtkIdType iPiece, vtkDataObject* inputNode, vtkMultiPieceDataSet* multiPiece)
{
  vtkPointSet* dataset = vtkPointSet::SafeDownCast(inputNode);
  vtkPointSet* transformedDataset = nullptr;

  int pieceAlterner = ((iPiece % 2) * 2 - 1) * ((iPiece + 1) / 2);
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
    // Legacy non mapped code, for unsupported type dataset
    vtkWarningMacro("Unsupported Dataset Type for mapped array, using vtkTransformFilter instead.");
    vtkNew<vtkTransform> transform;
    switch (this->RotationAxis)
    {
      case 0:
        transform->RotateX(pieceAngle);
        break;
      case 1:
        transform->RotateY(pieceAngle);
        break;
      case 2:
        transform->RotateZ(pieceAngle);
        break;
    }

    vtkNew<vtkTransformFilter> transformFilter;
    transformFilter->SetInputData(inputNode);
    transformFilter->SetTransform(transform);
    transformFilter->Update();

    multiPiece->SetPiece(iPiece, transformFilter->GetOutput());
  }
}

//------------------------------------------------------------------------------
vtkDataArray* vtkAngularPeriodicFilter::TransformDataArray(
  vtkDataArray* inputArray, double angle, bool useCenter, bool normalize)
{
  vtkDataArray* periodicArray = nullptr;
  double defaultCenter[3] = { 0., 0., 0. };
  switch (inputArray->GetDataType())
  {
    case VTK_FLOAT:
    {
      auto array = vtkArrayDownCast<vtkAOSDataArrayTemplate<float>>(inputArray);
      auto* pArray = vtkImplicitArray<vtkAngularPeriodicBackend<float>>::New();
      pArray->ConstructBackend(
        array, angle, this->RotationAxis, useCenter ? this->Center : defaultCenter, normalize);
      pArray->SetName(inputArray->GetName());
      pArray->SetNumberOfComponents(inputArray->GetNumberOfComponents());
      pArray->SetNumberOfTuples(inputArray->GetNumberOfTuples());
      if (this->ComputeRotationsOnTheFly)
      {
        periodicArray = pArray;
      }
      else
      {
        vtkFloatArray* concrete = vtkFloatArray::New();
        concrete->DeepCopy(pArray); // instantiate the array
        periodicArray = concrete;
        pArray->Delete();
      }
      break;
    }
    case VTK_DOUBLE:
    {
      auto array = vtkArrayDownCast<vtkAOSDataArrayTemplate<double>>(inputArray);
      auto* pArray = vtkImplicitArray<vtkAngularPeriodicBackend<double>>::New();
      pArray->ConstructBackend(
        array, angle, this->RotationAxis, useCenter ? this->Center : defaultCenter, normalize);
      pArray->SetName(inputArray->GetName());
      pArray->SetNumberOfComponents(inputArray->GetNumberOfComponents());
      pArray->SetNumberOfTuples(inputArray->GetNumberOfTuples());
      if (this->ComputeRotationsOnTheFly)
      {
        periodicArray = pArray;
      }
      else
      {
        vtkDoubleArray* concrete = vtkDoubleArray::New();
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

//------------------------------------------------------------------------------
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
      transformedArray =
        this->TransformDataArray(array, angle, false, attribute == vtkDataSetAttributes::NORMALS);
    }
    else
    {
      transformedArray = array;
      array->Register(nullptr);
    }
    transformedData->AddArray(transformedArray);
    if (attribute >= 0)
    {
      transformedData->SetAttribute(transformedArray, attribute);
    }
    transformedArray->Delete();
  }
}

//------------------------------------------------------------------------------
void vtkAngularPeriodicFilter::ComputePeriodicMesh(
  vtkPointSet* dataset, vtkPointSet* transformedDataset, double angle)
{
  // Shallow copy data structure
  transformedDataset->CopyStructure(dataset);

  // Transform points coordinates array
  vtkPoints* points = dataset->GetPoints();
  if (points != nullptr)
  {
    vtkDataArray* pointArray = dataset->GetPoints()->GetData();
    vtkNew<vtkPoints> rotatedPoints;
    vtkDataArray* transformedArray = this->TransformDataArray(pointArray, angle, true);
    rotatedPoints->SetData(transformedArray);
    transformedArray->Delete();
    // Set the points
    transformedDataset->SetPoints(rotatedPoints);
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

//------------------------------------------------------------------------------
int vtkAngularPeriodicFilter::RequestData(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  if (this->GetRotationMode() == VTK_ROTATION_MODE_ARRAY_VALUE &&
    this->GetIterationMode() == VTK_ITERATION_MODE_MAX)
  {
    this->ReducePeriodNumbers = true;
  }
  return this->Superclass::RequestData(request, inputVector, outputVector);
}

//------------------------------------------------------------------------------
void vtkAngularPeriodicFilter::GeneratePieceName(vtkCompositeDataSet* input,
  vtkCompositeDataIterator* inputLoc, vtkMultiPieceDataSet* output, vtkIdType outputId)
{
  vtkDataObjectTree* inputTree = vtkDataObjectTree::SafeDownCast(input);
  if (!inputTree)
  {
    return;
  }
  std::ostringstream ss;
  const char* parentName = inputTree->GetMetaData(inputLoc)->Get(vtkCompositeDataSet::NAME());
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
VTK_ABI_NAMESPACE_END
