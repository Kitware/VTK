// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkTransformFilter.h"

#include "vtkCellData.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataObject.h"
#include "vtkDataObjectAlgorithm.h"
#include "vtkDataObjectMeshCache.h"
#include "vtkDataObjectTypes.h"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkImageData.h"
#include "vtkImageDataToPointSet.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkLinearTransform.h"
#include "vtkMeshCacheRunner.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPointSet.h"
#include "vtkRectilinearGrid.h"
#include "vtkRectilinearGridToPointSet.h"
#include "vtkSetGet.h"
#include "vtkSmartPointer.h"
#include "vtkStructuredGrid.h"

#include <vector>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkTransformFilter);
vtkCxxSetObjectMacro(vtkTransformFilter, Transform, vtkAbstractTransform);

//------------------------------------------------------------------------------
vtkTransformFilter::vtkTransformFilter()
{
  this->Transform = nullptr;
  this->OutputPointsPrecision = vtkAlgorithm::DEFAULT_PRECISION;
  this->TransformAllInputVectors = false;
  this->MeshCache->SetConsumer(this);
  this->MeshCache->ForwardAttribute(vtkDataObject::POINT);
  this->MeshCache->ForwardAttribute(vtkDataObject::CELL);
  this->MeshCache->PreservedInputAllAttributes();
}

//------------------------------------------------------------------------------
vtkTransformFilter::~vtkTransformFilter()
{
  this->SetTransform(nullptr);
}

//------------------------------------------------------------------------------
int vtkTransformFilter::FillInputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  info->Remove(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE());
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPointSet");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkImageData");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkRectilinearGrid");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkCompositeDataSet");
  return 1;
}

//------------------------------------------------------------------------------
int vtkTransformFilter::FillOutputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkDataObject");
  return 1;
}

//------------------------------------------------------------------------------
int vtkTransformFilter::RequestDataObject(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  auto input = vtkDataObject::GetData(inputVector[0]);

  vtkDataObject* newOutput = nullptr;
  if (input->IsA("vtkDataSet"))
  {
    auto current = vtkDataSet::GetData(outputVector);
    auto inputDS = vtkDataSet::SafeDownCast(input);
    auto outDS = this->CreateNewDataSetIfNeeded(inputDS, current);
    if (outDS != current)
    {
      newOutput = outDS;
    }
  }
  else if (input->IsA("vtkCompositeDataSet"))
  {
    auto output = vtkCompositeDataSet::GetData(outputVector);
    if (!output || !output->IsA(input->GetClassName()))
    {
      newOutput = input->NewInstance();
    }
  }
  else
  {
    vtkErrorMacro("Unsupported input type " << input->GetDataObjectType());
    return 0;
  }

  if (newOutput)
  {
    outputVector->GetInformationObject(0)->Set(vtkDataObject::DATA_OBJECT(), newOutput);
    newOutput->FastDelete();
  }
  return 1;
}

//------------------------------------------------------------------------------
vtkSmartPointer<vtkPointSet> vtkTransformFilter::ConvertInput(vtkDataSet* inputDS)
{
  vtkSmartPointer<vtkPointSet> input = vtkPointSet::SafeDownCast(inputDS);
  if (!input)
  {
    // Try converting image data.
    vtkImageData* inImage = vtkImageData::SafeDownCast(inputDS);
    if (inImage)
    {
      vtkNew<vtkImageDataToPointSet> image2points;
      image2points->SetInputData(inImage);
      image2points->SetContainerAlgorithm(this);
      image2points->Update();
      input = image2points->GetOutput();
    }
  }

  if (!input)
  {
    // Try converting rectilinear grid.
    vtkRectilinearGrid* inRect = vtkRectilinearGrid::SafeDownCast(inputDS);
    if (inRect)
    {
      vtkNew<vtkRectilinearGridToPointSet> rect2points;
      rect2points->SetInputData(inRect);
      rect2points->SetContainerAlgorithm(this);
      rect2points->Update();
      input = rect2points->GetOutput();
    }
  }

  return input;
}

//------------------------------------------------------------------------------
void vtkTransformFilter::InitializeOutputPointSet(vtkPointSet* input, vtkPointSet* output)
{
  // First, copy the input to the output as a starting point
  output->ShallowCopy(input);

  // Allocate transformed points
  vtkPoints* inPts = input->GetPoints();
  vtkNew<vtkPoints> newPts;
  // Set the desired precision for the points in the output.
  if (this->OutputPointsPrecision == vtkAlgorithm::DEFAULT_PRECISION)
  {
    newPts->SetDataType(inPts->GetDataType());
  }
  else if (this->OutputPointsPrecision == vtkAlgorithm::SINGLE_PRECISION)
  {
    newPts->SetDataType(VTK_FLOAT);
  }
  else if (this->OutputPointsPrecision == vtkAlgorithm::DOUBLE_PRECISION)
  {
    newPts->SetDataType(VTK_DOUBLE);
  }
  vtkIdType numPts = inPts->GetNumberOfPoints();
  newPts->Reserve(numPts);
  output->SetPoints(newPts);

  vtkDataArray* inVectors;
  vtkDataArray* inNormals;
  vtkPointData* pd = input->GetPointData();
  vtkPointData* outPD = output->GetPointData();

  inVectors = pd->GetVectors();
  inNormals = pd->GetNormals();

  // always transform Vectors and Normals
  vtkSmartPointer<vtkDataArray> newVectors;
  if (inVectors)
  {
    newVectors.TakeReference(this->CreateFromArray(inVectors));
    outPD->SetVectors(newVectors);
  }
  vtkSmartPointer<vtkDataArray> newNormals;
  if (inNormals)
  {
    newNormals.TakeReference(this->CreateFromArray(inNormals));
    outPD->SetNormals(newNormals);
  }

  // Initialize new empty arrays when required.
  // Looks like Transform need empty but allocated buffers
  if (this->TransformAllInputVectors)
  {
    int nArrays = pd->GetNumberOfArrays();
    vtkSmartPointer<vtkDataArray> outArray;
    for (int arrayIndex = 0; arrayIndex < nArrays; arrayIndex++)
    {
      vtkDataArray* inputArray = pd->GetArray(arrayIndex);
      if (inputArray != inVectors && inputArray != inNormals &&
        inputArray->GetNumberOfComponents() == 3)
      {
        outArray.TakeReference(this->CreateFromArray(inputArray));
        outPD->AddArray(outArray);
      }
    }
  }
}

//------------------------------------------------------------------------------
void vtkTransformFilter::TransformPointData(
  vtkPointSet* input, vtkPointSet* output, bool preservePoints)
{
  auto inPts = input->GetPoints();
  auto newPts = output->GetPoints();
  auto pd = input->GetPointData();
  auto outPD = output->GetPointData();

  auto inVectors = pd->GetVectors();
  auto inNormals = pd->GetNormals();

  std::vector<vtkDataArray*> inAdditionalVectors;
  std::vector<vtkDataArray*> outAdditionalVectors;
  if (this->TransformAllInputVectors)
  {
    int nArrays = pd->GetNumberOfArrays();
    for (int arrayIndex = 0; arrayIndex < nArrays; arrayIndex++)
    {
      auto inArray = pd->GetArray(arrayIndex);
      if (!inArray || inArray->GetNumberOfComponents() != 3)
      {
        continue;
      }

      vtkSmartPointer<vtkDataArray> outArray = outPD->GetArray(inArray->GetName());
      // The vtkTransform API append data to the output array. Thus we need to clear it.
      outArray->Initialize();

      /**
       * Here we identify the arrays to transform.
       * The canonical way to apply the transform is to call
       * vtkAbstractTransform::TransformPointsNormalsVectors that modifies the vtkPoints
       * and the given arrays.
       *
       * When using Cache, we want to update only arrays, and not the Points.
       * In that case, use the per-array API.
       */
      if (preservePoints)
      {
        if (inArray == inNormals)
        {
          this->Transform->TransformNormals(inArray, outArray);
        }
        else
        {
          this->Transform->TransformVectors(inArray, outArray);
        }
      }
      else if (inArray != inVectors && inArray != inNormals)
      {
        inAdditionalVectors.push_back(inArray);
        outAdditionalVectors.push_back(outPD->GetArray(inArray->GetName()));
      }
    }
  }

  if (!preservePoints)
  {
    // Loop over all points, updating position
    //
    if (inVectors || inNormals || !inAdditionalVectors.empty())
    {
      auto newNormals = outPD->GetNormals();
      auto newVectors = outPD->GetVectors();
      this->Transform->TransformPointsNormalsVectors(inPts, newPts, inNormals, newNormals,
        inVectors, newVectors, static_cast<int>(inAdditionalVectors.size()),
        inAdditionalVectors.data(), outAdditionalVectors.data());
    }
    else
    {
      this->Transform->TransformPoints(inPts, newPts);
    }
  }
}

//------------------------------------------------------------------------------
void vtkTransformFilter::TransformCellData(vtkPointSet* input, vtkPointSet* output)
{
  vtkCellData* cd = input->GetCellData();
  vtkCellData* outCD = output->GetCellData();
  auto inCellVectors = cd->GetVectors();
  auto inCellNormals = cd->GetNormals();

  vtkSmartPointer<vtkDataArray> newCellVectors;
  vtkSmartPointer<vtkDataArray> newCellNormals;
  if (inCellVectors)
  {

    newCellVectors.TakeReference(this->CreateFromArray(inCellVectors));
    outCD->AddArray(newCellVectors);
    this->Transform->TransformVectors(inCellVectors, newCellVectors);
  }

  if (inCellNormals)
  {
    newCellNormals.TakeReference(this->CreateFromArray(inCellNormals));
    outCD->AddArray(newCellNormals);
    this->Transform->TransformNormals(inCellNormals, newCellNormals);
  }

  if (this->TransformAllInputVectors)
  {
    vtkSmartPointer<vtkDataArray> tmpOutArray;
    for (int i = 0; i < cd->GetNumberOfArrays(); i++)
    {
      if (this->CheckAbort())
      {
        break;
      }
      vtkDataArray* tmpArray = cd->GetArray(i);
      if (tmpArray != inCellVectors && tmpArray != inCellNormals &&
        tmpArray->GetNumberOfComponents() == 3)
      {
        tmpOutArray.TakeReference(this->CreateFromArray(tmpArray));
        this->Transform->TransformVectors(tmpArray, tmpOutArray);
        outCD->AddArray(tmpOutArray);
      }
    }
  }
}

//------------------------------------------------------------------------------
bool vtkTransformFilter::ExecuteDataSet(
  vtkDataSet* inputDS, vtkPointSet* output, bool useCachedGeometry)
{
  vtkSmartPointer<vtkPointSet> input = vtkPointSet::SafeDownCast(inputDS);
  if (!useCachedGeometry)
  {
    input = this->ConvertInput(inputDS);
    if (!input)
    {
      vtkErrorMacro(<< "Invalid or missing input");
      return false;
    }

    // Check input
    //
    if (this->Transform == nullptr)
    {
      vtkErrorMacro(<< "No transform defined!");
      return true;
    }

    if (!input->GetPoints())
    {
      return true;
    }

    this->InitializeOutputPointSet(input, output);
  }
  this->UpdateProgress(.2);

  bool isLinear = vtkLinearTransform::SafeDownCast(this->Transform) != nullptr;
  useCachedGeometry = useCachedGeometry && isLinear;
  this->TransformPointData(input, output, useCachedGeometry);
  this->UpdateProgress(.6);
  this->TransformCellData(input, output);
  this->UpdateProgress(1.);

  return true;
}

//------------------------------------------------------------------------------
int vtkTransformFilter::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkDataObject* inputDataObj = vtkDataObject::GetData(inputVector[0]);
  vtkDataObject* outputDataObj = vtkDataObject::GetData(outputVector);

  vtkMeshCacheRunner utils{ this->MeshCache, inputDataObj, outputDataObj, false };

  if (inputDataObj->IsA("vtkCompositeDataSet") && outputDataObj->IsA("vtkCompositeDataSet"))
  {
    vtkCompositeDataSet* inputComposite = vtkCompositeDataSet::SafeDownCast(inputDataObj);
    vtkCompositeDataSet* outComposite = vtkCompositeDataSet::SafeDownCast(outputDataObj);

    if (!utils.GetCacheLoaded())
    {
      outComposite->CopyStructure(inputComposite);
    }

    vtkSmartPointer<vtkCompositeDataIterator> inputIter;
    inputIter.TakeReference(inputComposite->NewIterator());
    inputIter->SkipEmptyNodesOn();

    vtkSmartPointer<vtkCompositeDataIterator> outIter;
    outIter.TakeReference(outComposite->NewIterator());
    outIter->SkipEmptyNodesOn();
    outIter->InitTraversal();

    vtkIdType numBlocks = 0;
    // a quick iteration to get the total number of blocks to iterate over which
    // is necessary to scale progress events.
    for (inputIter->InitTraversal(); !inputIter->IsDoneWithTraversal(); inputIter->GoToNextItem())
    {
      ++numBlocks;
    }
    const double progressScale = 1.0 / numBlocks;
    vtkIdType blockIndex = 0;

    for (inputIter->InitTraversal(); !inputIter->IsDoneWithTraversal(); inputIter->GoToNextItem())
    {
      this->SetProgressShiftScale(progressScale * blockIndex, progressScale);
      auto inputDataSet = vtkDataSet::SafeDownCast(inputIter->GetCurrentDataObject());
      auto outDataSet = vtkDataSet::SafeDownCast(outIter->GetCurrentDataObject());
      // create output
      auto outPointSet =
        vtkPointSet::SafeDownCast(this->CreateNewDataSetIfNeeded(inputDataSet, outDataSet));
      assert(outPointSet);
      this->ExecuteDataSet(inputDataSet, outPointSet, utils.GetCacheLoaded());

      if (outPointSet != outDataSet && !utils.GetCacheLoaded())
      {
        outComposite->SetDataSet(inputIter, outPointSet);
        outPointSet->FastDelete();
      }

      outIter->GoToNextItem();
      blockIndex++;
    }
  }
  else
  {
    vtkDataSet* realInput = vtkDataSet::GetData(inputVector[0]);
    vtkPointSet* output = vtkPointSet::GetData(outputVector);
    this->SetProgressShiftScale(0, 1);
    if (!this->ExecuteDataSet(realInput, output, utils.GetCacheLoaded()))
    {
      return 0;
    }
  }

  utils.UpdateCache();

  this->UpdateProgress(1.);

  return 1;
}

//------------------------------------------------------------------------------
vtkMTimeType vtkTransformFilter::GetMTime()
{
  vtkMTimeType mTime = this->MTime.GetMTime();
  vtkMTimeType transMTime;

  if (this->Transform)
  {
    transMTime = this->Transform->GetMTime();
    mTime = (transMTime > mTime ? transMTime : mTime);
  }

  return mTime;
}

//------------------------------------------------------------------------------
vtkDataArray* vtkTransformFilter::CreateNewDataArray(vtkDataArray* input)
{
  if (this->OutputPointsPrecision == vtkAlgorithm::DEFAULT_PRECISION && input != nullptr)
  {
    return input->NewInstance();
  }

  switch (this->OutputPointsPrecision)
  {
    case vtkAlgorithm::DOUBLE_PRECISION:
      return vtkDoubleArray::New();
    case vtkAlgorithm::SINGLE_PRECISION:
    default:
      return vtkFloatArray::New();
  }
}

//------------------------------------------------------------------------------
vtkDataArray* vtkTransformFilter::CreateFromArray(vtkDataArray* input)
{
  auto output = this->CreateNewDataArray(input);
  output->SetName(input->GetName());
  output->SetNumberOfComponents(input->GetNumberOfComponents());
  output->ReserveTuples(input->GetNumberOfTuples());
  return output;
}

//------------------------------------------------------------------------------
vtkDataSet* vtkTransformFilter::CreateNewDataSetIfNeeded(vtkDataSet* input, vtkDataSet* current)
{
  vtkDataSet* validOutput = current;
  vtkImageData* inImage = vtkImageData::SafeDownCast(input);
  vtkRectilinearGrid* inRect = vtkRectilinearGrid::SafeDownCast(input);
  if (inImage || inRect)
  {
    vtkStructuredGrid* output = vtkStructuredGrid::SafeDownCast(current);
    if (!output)
    {
      validOutput = vtkStructuredGrid::New();
    }
  }
  else
  {
    vtkPointSet* output = vtkPointSet::SafeDownCast(current);
    if (!output || !output->IsA(input->GetClassName()))
    {
      validOutput = input->NewInstance();
    }
  }

  return validOutput;
}

//------------------------------------------------------------------------------
void vtkTransformFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Transform: " << this->Transform << "\n";
  os << indent << "Output Points Precision: " << this->OutputPointsPrecision << "\n";
}
VTK_ABI_NAMESPACE_END
