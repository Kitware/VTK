// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkRandomAttributeGenerator.h"

#include "vtkBitArray.h"
#include "vtkCellData.h"
#include "vtkCharArray.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataSet.h"
#include "vtkDoubleArray.h"
#include "vtkFieldData.h"
#include "vtkFloatArray.h"
#include "vtkHyperTreeGrid.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkIntArray.h"
#include "vtkLongArray.h"
#include "vtkLongLongArray.h"
#include "vtkMath.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkShortArray.h"
#include "vtkSmartPointer.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnsignedIntArray.h"
#include "vtkUnsignedLongArray.h"
#include "vtkUnsignedLongLongArray.h"
#include "vtkUnsignedShortArray.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkRandomAttributeGenerator);

namespace
{

//------------------------------------------------------------------------------
template <class T>
void GenerateRandomTuple(
  T* data, vtkIdType i, int numComp, int minComp, int maxComp, double min, double max)
{
  for (int comp = minComp; comp <= maxComp; comp++)
  {
    // Now generate a random component value
    data[i * numComp + comp] = static_cast<T>(vtkMath::Random(min, max));
  }
}

//------------------------------------------------------------------------------
void GenerateRandomTupleBit(vtkDataArray* data, vtkIdType i, int minComp, int maxComp)
{
  for (int comp = minComp; comp <= maxComp; comp++)
  {
    // Now generate a random component value
    data->SetComponent(i, comp, vtkMath::Random(0.0, 1.0) < 0.5 ? 0 : 1);
  }
}

//------------------------------------------------------------------------------
template <class T>
void CopyTupleFrom0(T* data, vtkIdType i, int numComp, int minComp, int maxComp)
{
  memcpy(data + i * numComp + minComp, data + minComp, (maxComp - minComp + 1) * sizeof(T));
}
//------------------------------------------------------------------------------
void CopyTupleFrom0Bit(vtkDataArray* data, vtkIdType i, int minComp, int maxComp)
{
  for (int comp = minComp; comp <= maxComp; comp++)
  {
    data->SetComponent(i, comp, data->GetComponent(0, comp));
  }
}

}

//------------------------------------------------------------------------------
// This function template creates random attributes within a given range. It is
// assumed that the input data array may have a variable number of components.
template <class T>
void vtkRandomAttributeGenerator::GenerateRandomTuples(
  T* data, vtkIdType numTuples, int numComp, int minComp, int maxComp, double min, double max)
{
  if (numTuples == 0)
  {
    return;
  }
  vtkIdType total = numComp * numTuples;
  vtkIdType tenth = total / 10 + 1;
  ::GenerateRandomTuple(data, 0, numComp, minComp, maxComp, min, max);
  for (vtkIdType i = 1; i < numTuples; i++)
  {
    // update progress and check for aborts
    if (!(i % tenth))
    {
      this->UpdateProgress(static_cast<double>(i) / total);
      if (this->CheckAbort())
      {
        break;
      }
    }
    if (this->AttributesConstantPerBlock)
    {
      ::CopyTupleFrom0(data, i, numComp, minComp, maxComp);
    }
    else
    {
      ::GenerateRandomTuple(data, i, numComp, minComp, maxComp, min, max);
    }
  }
}

//------------------------------------------------------------------------------
// This method does the data type allocation and switching for various types.
vtkDataArray* vtkRandomAttributeGenerator::GenerateData(
  int dataType, vtkIdType numTuples, int numComp, int minComp, int maxComp, double min, double max)
{
  vtkDataArray* dataArray = nullptr;

  switch (dataType)
  {
    case VTK_CHAR:
    {
      dataArray = vtkCharArray::New();
      dataArray->SetNumberOfComponents(numComp);
      dataArray->SetNumberOfTuples(numTuples);
      char* data = static_cast<vtkCharArray*>(dataArray)->GetPointer(0);
      this->GenerateRandomTuples(data, numTuples, numComp, minComp, maxComp, min, max);
    }
    break;
    case VTK_UNSIGNED_CHAR:
    {
      dataArray = vtkUnsignedCharArray::New();
      dataArray->SetNumberOfComponents(numComp);
      dataArray->SetNumberOfTuples(numTuples);
      unsigned char* data = static_cast<vtkUnsignedCharArray*>(dataArray)->GetPointer(0);
      this->GenerateRandomTuples(data, numTuples, numComp, minComp, maxComp, min, max);
    }
    break;
    case VTK_SHORT:
    {
      dataArray = vtkShortArray::New();
      dataArray->SetNumberOfComponents(numComp);
      dataArray->SetNumberOfTuples(numTuples);
      short* data = static_cast<vtkShortArray*>(dataArray)->GetPointer(0);
      this->GenerateRandomTuples(data, numTuples, numComp, minComp, maxComp, min, max);
    }
    break;
    case VTK_UNSIGNED_SHORT:
    {
      dataArray = vtkUnsignedShortArray::New();
      dataArray->SetNumberOfComponents(numComp);
      dataArray->SetNumberOfTuples(numTuples);
      unsigned short* data = static_cast<vtkUnsignedShortArray*>(dataArray)->GetPointer(0);
      this->GenerateRandomTuples(data, numTuples, numComp, minComp, maxComp, min, max);
    }
    break;
    case VTK_INT:
    {
      dataArray = vtkIntArray::New();
      dataArray->SetNumberOfComponents(numComp);
      dataArray->SetNumberOfTuples(numTuples);
      int* data = static_cast<vtkIntArray*>(dataArray)->GetPointer(0);
      this->GenerateRandomTuples(data, numTuples, numComp, minComp, maxComp, min, max);
    }
    break;
    case VTK_UNSIGNED_INT:
    {
      dataArray = vtkUnsignedIntArray::New();
      dataArray->SetNumberOfComponents(numComp);
      dataArray->SetNumberOfTuples(numTuples);
      unsigned int* data = static_cast<vtkUnsignedIntArray*>(dataArray)->GetPointer(0);
      this->GenerateRandomTuples(data, numTuples, numComp, minComp, maxComp, min, max);
    }
    break;
    case VTK_LONG:
    {
      dataArray = vtkLongArray::New();
      dataArray->SetNumberOfComponents(numComp);
      dataArray->SetNumberOfTuples(numTuples);
      long* data = static_cast<vtkLongArray*>(dataArray)->GetPointer(0);
      this->GenerateRandomTuples(data, numTuples, numComp, minComp, maxComp, min, max);
    }
    break;
    case VTK_UNSIGNED_LONG:
    {
      dataArray = vtkUnsignedLongArray::New();
      dataArray->SetNumberOfComponents(numComp);
      dataArray->SetNumberOfTuples(numTuples);
      unsigned long* data = static_cast<vtkUnsignedLongArray*>(dataArray)->GetPointer(0);
      this->GenerateRandomTuples(data, numTuples, numComp, minComp, maxComp, min, max);
    }
    break;
    case VTK_LONG_LONG:
    {
      dataArray = vtkLongLongArray::New();
      dataArray->SetNumberOfComponents(numComp);
      dataArray->SetNumberOfTuples(numTuples);
      long long* data = static_cast<vtkLongLongArray*>(dataArray)->GetPointer(0);
      this->GenerateRandomTuples(data, numTuples, numComp, minComp, maxComp, min, max);
    }
    break;
    case VTK_UNSIGNED_LONG_LONG:
    {
      dataArray = vtkUnsignedLongLongArray::New();
      dataArray->SetNumberOfComponents(numComp);
      dataArray->SetNumberOfTuples(numTuples);
      unsigned long long* data = static_cast<vtkUnsignedLongLongArray*>(dataArray)->GetPointer(0);
      this->GenerateRandomTuples(data, numTuples, numComp, minComp, maxComp, min, max);
    }
    break;
    case VTK_FLOAT:
    {
      dataArray = vtkFloatArray::New();
      dataArray->SetNumberOfComponents(numComp);
      dataArray->SetNumberOfTuples(numTuples);
      float* data = static_cast<vtkFloatArray*>(dataArray)->GetPointer(0);
      this->GenerateRandomTuples(data, numTuples, numComp, minComp, maxComp, min, max);
    }
    break;
    case VTK_DOUBLE:
    {
      dataArray = vtkDoubleArray::New();
      dataArray->SetNumberOfComponents(numComp);
      dataArray->SetNumberOfTuples(numTuples);
      double* data = static_cast<vtkDoubleArray*>(dataArray)->GetPointer(0);
      this->GenerateRandomTuples(data, numTuples, numComp, minComp, maxComp, min, max);
    }
    break;
    case VTK_ID_TYPE:
    {
      dataArray = vtkIdTypeArray::New();
      dataArray->SetNumberOfComponents(numComp);
      dataArray->SetNumberOfTuples(numTuples);
      vtkIdType* data = static_cast<vtkIdTypeArray*>(dataArray)->GetPointer(0);
      this->GenerateRandomTuples(data, numTuples, numComp, minComp, maxComp, min, max);
    }
    break;
    case VTK_BIT: // we'll do something special for bit arrays
    {
      vtkIdType total = numComp * numTuples;
      vtkIdType tenth = total / 10 + 1;
      dataArray = vtkBitArray::New();
      dataArray->SetNumberOfComponents(numComp);
      dataArray->SetNumberOfTuples(numTuples);
      if (numTuples == 0)
      {
        break;
      }
      ::GenerateRandomTupleBit(dataArray, 0, minComp, maxComp);
      for (vtkIdType i = 1; i < numTuples; i++)
      {
        // update progress and check for aborts
        if (!(i % tenth))
        {
          this->UpdateProgress(static_cast<double>(i) / total);
          if (this->CheckAbort())
          {
            break;
          }
        }
        if (this->AttributesConstantPerBlock)
        {
          ::CopyTupleFrom0Bit(dataArray, i, minComp, maxComp);
        }
        else
        {
          ::GenerateRandomTupleBit(dataArray, i, minComp, maxComp);
        }
      }
    }
    break;

    default:
      vtkGenericWarningMacro("Cannot create random data array\n");
  }

  return dataArray;
}

//------------------------------------------------------------------------------
// VTK_DEPRECATED_IN_9_4_0()
int vtkRandomAttributeGenerator::RequestData(
  vtkCompositeDataSet* input, vtkCompositeDataSet* output)
{
  return this->ProcessComposite(input, output);
}

//------------------------------------------------------------------------------
// VTK_DEPRECATED_IN_9_4_0()
int vtkRandomAttributeGenerator::RequestData(vtkDataSet* input, vtkDataSet* output)
{
  return this->ProcessDataSet(input, output);
}

//------------------------------------------------------------------------------
int vtkRandomAttributeGenerator::ProcessComposite(
  vtkCompositeDataSet* input, vtkCompositeDataSet* output)
{
  if (input == nullptr || output == nullptr)
  {
    return 0;
  }
  output->CopyStructure(input);

  vtkSmartPointer<vtkCompositeDataIterator> it;
  it.TakeReference(input->NewIterator());
  for (it->InitTraversal(); !it->IsDoneWithTraversal(); it->GoToNextItem())
  {
    if (this->CheckAbort())
    {
      break;
    }

    vtkDataSet* inputDS = vtkDataSet::SafeDownCast(it->GetCurrentDataObject());
    if (inputDS)
    {
      vtkSmartPointer<vtkDataSet> outputDS;
      outputDS.TakeReference(inputDS->NewInstance());
      output->SetDataSet(it, outputDS);
      this->ProcessDataSet(inputDS, outputDS);
      continue;
    }

    vtkHyperTreeGrid* inputHTG = vtkHyperTreeGrid::SafeDownCast(it->GetCurrentDataObject());
    if (inputHTG)
    {
      vtkSmartPointer<vtkHyperTreeGrid> outputHTG;
      outputHTG.TakeReference(inputHTG->NewInstance());
      output->SetDataSet(it, outputHTG);
      this->ProcessHTG(inputHTG, outputHTG);
      continue;
    }
  }
  return 1;
}

//------------------------------------------------------------------------------
int vtkRandomAttributeGenerator::ProcessDataSet(vtkDataSet* input, vtkDataSet* output)
{
  vtkIdType numPts = input->GetNumberOfPoints();
  vtkIdType numCells = input->GetNumberOfCells();

  output->CopyStructure(input);
  output->CopyAttributes(input);

  if (numPts >= 1)
  {
    vtkPointData* outputPD = output->GetPointData();
    this->GeneratePointData(outputPD, numPts);
  }

  if (numCells >= 1)
  {
    vtkCellData* outputCD = output->GetCellData();
    this->GenerateCellData(outputCD, numCells);
  }

  vtkFieldData* outputFD = output->GetFieldData();
  this->GenerateFieldData(outputFD);

  return 1;
}

//------------------------------------------------------------------------------
int vtkRandomAttributeGenerator::ProcessHTG(vtkHyperTreeGrid* input, vtkHyperTreeGrid* output)
{
  vtkIdType numCells = input->GetNumberOfCells();

  output->CopyStructure(input);

  // No point data in HTGs
  output->GetCellData()->PassData(input->GetCellData());
  output->GetFieldData()->PassData(input->GetFieldData());

  if (numCells >= 1)
  {
    vtkCellData* outputCD = output->GetCellData();
    this->GenerateCellData(outputCD, numCells);
  }

  vtkFieldData* outputFD = output->GetFieldData();
  this->GenerateFieldData(outputFD);

  return 1;
}

//------------------------------------------------------------------------------
int vtkRandomAttributeGenerator::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // composite case
  vtkCompositeDataSet* compositeInput = vtkCompositeDataSet::GetData(inInfo);
  vtkCompositeDataSet* compositeOutput = vtkCompositeDataSet::GetData(outInfo);
  if (compositeInput && compositeOutput)
  {
    return this->ProcessComposite(compositeInput, compositeOutput);
  }

  // dataset case
  vtkDataSet* inputDS = vtkDataSet::GetData(inInfo);
  vtkDataSet* outputDS = vtkDataSet::GetData(outInfo);
  if (inputDS && outputDS)
  {
    return this->ProcessDataSet(inputDS, outputDS);
  }

  // htg case
  vtkHyperTreeGrid* inputHTG = vtkHyperTreeGrid::GetData(inInfo);
  vtkHyperTreeGrid* outputHTG = vtkHyperTreeGrid::GetData(outInfo);
  if (inputHTG && outputHTG)
  {
    return this->ProcessHTG(inputHTG, outputHTG);
  }

  vtkErrorMacro(<< "Unable to retrieve input / output as supported type.\n");
  return 0;
}

//------------------------------------------------------------------------------
void vtkRandomAttributeGenerator::GeneratePointData(vtkPointData* outputPD, vtkIdType numPts)
{
  if (this->GeneratePointScalars)
  {
    vtkDataArray* ptScalars = this->GenerateData(this->DataType, numPts, this->NumberOfComponents,
      0, this->NumberOfComponents - 1, this->MinimumComponentValue, this->MaximumComponentValue);
    ptScalars->SetName("RandomPointScalars");
    outputPD->SetScalars(ptScalars);
    ptScalars->Delete();
  }
  if (this->GeneratePointVectors)
  {
    vtkDataArray* ptVectors = this->GenerateData(
      this->DataType, numPts, 3, 0, 2, this->MinimumComponentValue, this->MaximumComponentValue);
    ptVectors->SetName("RandomPointVectors");
    outputPD->SetVectors(ptVectors);
    ptVectors->Delete();
  }
  if (this->GeneratePointNormals)
  {
    vtkDataArray* ptNormals = this->GenerateData(
      this->DataType, numPts, 3, 0, 2, this->MinimumComponentValue, this->MaximumComponentValue);
    double v[3];
    for (vtkIdType id = 0; id < numPts; id++)
    {
      ptNormals->GetTuple(id, v);
      vtkMath::Normalize(v);
      ptNormals->SetTuple(id, v);
    }
    outputPD->SetNormals(ptNormals);
    ptNormals->Delete();
  }
  if (this->GeneratePointTensors)
  {
    // fill in 6 components, and then shift them around to make them symmetric
    vtkDataArray* ptTensors = this->GenerateData(
      this->DataType, numPts, 9, 0, 5, this->MinimumComponentValue, this->MaximumComponentValue);
    ptTensors->SetName("RandomPointTensors");
    double t[9];
    for (vtkIdType id = 0; id < numPts; id++)
    {
      ptTensors->GetTuple(id, t);
      t[8] = t[3]; // make sure the tensor is symmetric
      t[3] = t[1];
      t[6] = t[2];
      t[7] = t[5];
      ptTensors->SetTuple(id, t);
    }
    outputPD->SetTensors(ptTensors);
    ptTensors->Delete();
  }
  if (this->GeneratePointTCoords)
  {
    int numComp = this->NumberOfComponents < 1
      ? 1
      : (this->NumberOfComponents > 3 ? 3 : this->NumberOfComponents);
    vtkDataArray* ptTCoords = this->GenerateData(this->DataType, numPts, numComp, 0,
      this->NumberOfComponents - 1, this->MinimumComponentValue, this->MaximumComponentValue);
    outputPD->SetTCoords(ptTCoords);
    ptTCoords->Delete();
  }
  if (this->GeneratePointArray)
  {
    vtkDataArray* ptData = this->GenerateData(this->DataType, numPts, this->NumberOfComponents, 0,
      this->NumberOfComponents - 1, this->MinimumComponentValue, this->MaximumComponentValue);
    ptData->SetName("RandomPointArray");
    outputPD->AddArray(ptData);
    ptData->Delete();
  }
}

//------------------------------------------------------------------------------
void vtkRandomAttributeGenerator::GenerateCellData(vtkCellData* outputCD, vtkIdType numCells)
{
  if (this->GenerateCellScalars)
  {
    vtkDataArray* cellScalars =
      this->GenerateData(this->DataType, numCells, this->NumberOfComponents, 0,
        this->NumberOfComponents - 1, this->MinimumComponentValue, this->MaximumComponentValue);
    cellScalars->SetName("RandomCellScalars");
    outputCD->SetScalars(cellScalars);
    cellScalars->Delete();
  }
  if (this->GenerateCellVectors)
  {
    vtkDataArray* cellVectors = this->GenerateData(
      this->DataType, numCells, 3, 0, 2, this->MinimumComponentValue, this->MaximumComponentValue);
    cellVectors->SetName("RandomCellVectors");
    outputCD->SetVectors(cellVectors);
    cellVectors->Delete();
  }
  if (this->GenerateCellNormals)
  {
    vtkDataArray* cellNormals = this->GenerateData(
      this->DataType, numCells, 3, 0, 2, this->MinimumComponentValue, this->MaximumComponentValue);
    double v[3];
    for (vtkIdType id = 0; id < numCells; id++)
    {
      cellNormals->GetTuple(id, v);
      vtkMath::Normalize(v);
      cellNormals->SetTuple(id, v);
    }
    outputCD->SetNormals(cellNormals);
    cellNormals->Delete();
  }
  if (this->GenerateCellTensors)
  {
    vtkDataArray* cellTensors = this->GenerateData(
      this->DataType, numCells, 9, 0, 5, this->MinimumComponentValue, this->MaximumComponentValue);
    cellTensors->SetName("RandomCellTensors");
    double t[9];
    for (vtkIdType id = 0; id < numCells; id++)
    {
      cellTensors->GetTuple(id, t);
      t[6] = t[1]; // make sure the tensor is symmetric
      t[7] = t[2];
      t[8] = t[4];
      cellTensors->SetTuple(id, t);
    }
    outputCD->SetTensors(cellTensors);
    cellTensors->Delete();
  }
  if (this->GenerateCellTCoords)
  {
    int numComp = this->NumberOfComponents < 1
      ? 1
      : (this->NumberOfComponents > 3 ? 3 : this->NumberOfComponents);
    vtkDataArray* cellTCoords = this->GenerateData(this->DataType, numCells, numComp, 0,
      this->NumberOfComponents - 1, this->MinimumComponentValue, this->MaximumComponentValue);
    outputCD->SetTCoords(cellTCoords);
    cellTCoords->Delete();
  }
  if (this->GenerateCellArray)
  {
    vtkDataArray* cellArray = this->GenerateData(this->DataType, numCells, this->NumberOfComponents,
      0, this->NumberOfComponents - 1, this->MinimumComponentValue, this->MaximumComponentValue);
    cellArray->SetName("RandomCellArray");
    outputCD->AddArray(cellArray);
    cellArray->Delete();
  }
}

//------------------------------------------------------------------------------
void vtkRandomAttributeGenerator::GenerateFieldData(vtkFieldData* outputFD)
{
  if (this->GenerateFieldArray)
  {
    vtkDataArray* data =
      this->GenerateData(this->DataType, this->NumberOfTuples, this->NumberOfComponents, 0,
        this->NumberOfComponents - 1, this->MinimumComponentValue, this->MaximumComponentValue);
    data->SetName("RandomFieldArray");
    outputFD->AddArray(data);
    data->Delete();
  }
}

//------------------------------------------------------------------------------
void vtkRandomAttributeGenerator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Data Type: " << this->DataType << endl;
  os << indent << "Number of Components: " << this->NumberOfComponents << endl;
  os << indent << "Number of Tuples: " << this->NumberOfTuples << endl;
  os << indent << "Minimum Component Value: " << this->MinimumComponentValue << endl;
  os << indent << "Maximum Component Value: " << this->MaximumComponentValue << endl;

  os << indent << "Generate Point Scalars: " << (this->GeneratePointScalars ? "On\n" : "Off\n");
  os << indent << "Generate Point Vectors: " << (this->GeneratePointVectors ? "On\n" : "Off\n");
  os << indent << "Generate Point Normals: " << (this->GeneratePointNormals ? "On\n" : "Off\n");
  os << indent << "Generate Point TCoords: " << (this->GeneratePointTCoords ? "On\n" : "Off\n");
  os << indent << "Generate Point Tensors: " << (this->GeneratePointTensors ? "On\n" : "Off\n");
  os << indent << "Generate Point Array: " << (this->GeneratePointArray ? "On\n" : "Off\n");

  os << indent << "Generate Cell Scalars: " << (this->GenerateCellScalars ? "On\n" : "Off\n");
  os << indent << "Generate Cell Vectors: " << (this->GenerateCellVectors ? "On\n" : "Off\n");
  os << indent << "Generate Cell Normals: " << (this->GenerateCellNormals ? "On\n" : "Off\n");
  os << indent << "Generate Cell TCoords: " << (this->GenerateCellTCoords ? "On\n" : "Off\n");
  os << indent << "Generate Cell Tensors: " << (this->GenerateCellTensors ? "On\n" : "Off\n");
  os << indent << "Generate Cell Array: " << (this->GenerateCellArray ? "On\n" : "Off\n");

  os << indent << "Generate Field Array: " << (this->GenerateFieldArray ? "On\n" : "Off\n");
}

//------------------------------------------------------------------------------
int vtkRandomAttributeGenerator::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkCompositeDataSet");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkHyperTreeGrid");
  return 1;
}
VTK_ABI_NAMESPACE_END
