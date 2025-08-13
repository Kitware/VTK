// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkAxisAlignedTransformFilter.h"
#include "vtkBitArray.h"
#include "vtkCellData.h"
#include "vtkDataSet.h"
#include "vtkDemandDrivenPipeline.h"
#include "vtkDoubleArray.h"
#include "vtkExplicitStructuredGrid.h"
#include "vtkHyperTree.h"
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridNonOrientedCursor.h"
#include "vtkHyperTreeGridScales.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMatrix3x3.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkRectilinearGrid.h"
#include "vtkStructuredGrid.h"
#include "vtkTransform.h"
#include "vtkTransformFilter.h"
#include "vtkUnstructuredGrid.h"
#include "vtkVector.h"

VTK_ABI_NAMESPACE_BEGIN

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkAxisAlignedTransformFilter);

namespace
{
//------------------------------------------------------------------------------
std::string AxisEnumToStr(int axis)
{
  switch (axis)
  {
    case vtkAxisAlignedTransformFilter::Axis::X:
    default:
      return "X Axis";
    case vtkAxisAlignedTransformFilter::Axis::Y:
      return "Y Axis";
    case vtkAxisAlignedTransformFilter::Axis::Z:
      return "Z Axis";
  }
}

//------------------------------------------------------------------------------
std::string AngleEnumToStr(int axis)
{
  switch (axis)
  {
    case vtkAxisAlignedTransformFilter::Angle::ROT0:
    default:
      return "0° rotation";
    case vtkAxisAlignedTransformFilter::Angle::ROT90:
      return "90° rotation";
    case vtkAxisAlignedTransformFilter::Angle::ROT180:
      return "180° rotation";
    case vtkAxisAlignedTransformFilter::Angle::ROT270:
      return "270° rotation";
  }
}

//------------------------------------------------------------------------------
// TODO: Use implicit array to optimize memory
void ReverseDoubleArray(vtkDoubleArray* arr)
{
  vtkNew<vtkDoubleArray> tmp;
  tmp->DeepCopy(arr);
  for (int i = 0; i < arr->GetNumberOfTuples(); i++)
  {
    double value = tmp->GetTuple1(i);
    arr->SetTuple1(arr->GetNumberOfTuples() - i - 1, -value);
  }
}

//------------------------------------------------------------------------------
void SwapXYZCoordinates(vtkDoubleArray*& XCoordinate, vtkDoubleArray*& YCoordinate,
  vtkDoubleArray*& ZCoordinate, int rotation, int axis)
{
  if (rotation == vtkAxisAlignedTransformFilter::Angle::ROT90 ||
    rotation == vtkAxisAlignedTransformFilter::Angle::ROT270)
  {
    switch (axis)
    {
      case vtkAxisAlignedTransformFilter::Axis::X:
      default:
        std::swap(YCoordinate, ZCoordinate);
        break;

      case vtkAxisAlignedTransformFilter::Axis::Y:
        std::swap(XCoordinate, ZCoordinate);
        break;

      case vtkAxisAlignedTransformFilter::Axis::Z:
        std::swap(XCoordinate, YCoordinate);
        break;
    }
  }
}

//------------------------------------------------------------------------------
template <typename GridType>
void ApplyTranslation(GridType* grid, double Translation[3])
{
  vtkDoubleArray* xCoordinate = vtkDoubleArray::SafeDownCast(grid->GetXCoordinates());
  for (vtkIdType i = 0; i < xCoordinate->GetNumberOfTuples(); ++i)
  {
    xCoordinate->SetTuple1(i, xCoordinate->GetTuple1(i) + Translation[0]);
  }
  vtkDoubleArray* yCoordinate = vtkDoubleArray::SafeDownCast(grid->GetYCoordinates());
  for (vtkIdType i = 0; i < yCoordinate->GetNumberOfTuples(); ++i)
  {
    yCoordinate->SetTuple1(i, yCoordinate->GetTuple1(i) + Translation[1]);
  }
  vtkDoubleArray* zCoordinate = vtkDoubleArray::SafeDownCast(grid->GetZCoordinates());
  for (vtkIdType i = 0; i < zCoordinate->GetNumberOfTuples(); ++i)
  {
    zCoordinate->SetTuple1(i, zCoordinate->GetTuple1(i) + Translation[2]);
  }
}

//------------------------------------------------------------------------------
template <typename GridType>
void ApplyScale(GridType* grid, double Scale[3])
{
  vtkDoubleArray* xCoordinate = vtkDoubleArray::SafeDownCast(grid->GetXCoordinates());
  for (vtkIdType i = 0; i < xCoordinate->GetNumberOfTuples(); ++i)
  {
    xCoordinate->SetTuple1(i, xCoordinate->GetTuple1(i) * Scale[0]);
  }
  vtkDoubleArray* yCoordinate = vtkDoubleArray::SafeDownCast(grid->GetYCoordinates());
  for (vtkIdType i = 0; i < yCoordinate->GetNumberOfTuples(); ++i)
  {
    yCoordinate->SetTuple1(i, yCoordinate->GetTuple1(i) * Scale[1]);
  }
  vtkDoubleArray* zCoordinate = vtkDoubleArray::SafeDownCast(grid->GetZCoordinates());
  for (vtkIdType i = 0; i < zCoordinate->GetNumberOfTuples(); ++i)
  {
    zCoordinate->SetTuple1(i, zCoordinate->GetTuple1(i) * Scale[2]);
  }
}

//------------------------------------------------------------------------------
int HashLut(int rotationAxis, int normalAxis, int angle)
{
  // Hash based on enumeration int value
  return 100 * rotationAxis + 10 * normalAxis + angle;
}

//------------------------------------------------------------------------------
std::map<int, std::function<std::pair<int, int>(int, int, int)>> Create2DHtgLut()
{
  using Axis = vtkAxisAlignedTransformFilter::Axis;
  using Angle = vtkAxisAlignedTransformFilter::Angle;

  return { { ::HashLut(Axis::X, Axis::Z, Angle::ROT180),
             [](int i, int j, int branchfactor)
             { return std::make_pair(i, branchfactor - 1 - j); } },
    { ::HashLut(Axis::X, Axis::Z, Angle::ROT270),
      [](int i, int j, int branchfactor) { return std::make_pair(i, branchfactor - 1 - j); } },
    { ::HashLut(Axis::X, Axis::Y, Angle::ROT90),
      [](int i, int j, int branchfactor) { return std::make_pair(i, branchfactor - 1 - j); } },
    { ::HashLut(Axis::X, Axis::Y, Angle::ROT180),
      [](int i, int j, int branchfactor) { return std::make_pair(i, branchfactor - 1 - j); } },
    { ::HashLut(Axis::Z, Axis::X, Angle::ROT90),
      [](int i, int j, int branchfactor) { return std::make_pair(branchfactor - 1 - i, j); } },
    { ::HashLut(Axis::Z, Axis::X, Angle::ROT180),
      [](int i, int j, int branchfactor) { return std::make_pair(branchfactor - 1 - i, j); } },
    { ::HashLut(Axis::Z, Axis::Y, Angle::ROT180),
      [](int i, int j, int branchfactor) { return std::make_pair(branchfactor - 1 - i, j); } },
    { ::HashLut(Axis::Z, Axis::Y, Angle::ROT270),
      [](int i, int j, int branchfactor) { return std::make_pair(branchfactor - 1 - i, j); } },
    { ::HashLut(Axis::Y, Axis::X, Angle::ROT90),
      [](int i, int j, int vtkNotUsed(branchfactor)) { return std::make_pair(j, i); } },
    { ::HashLut(Axis::Y, Axis::X, Angle::ROT180),
      [](int i, int j, int branchfactor) { return std::make_pair(i, branchfactor - 1 - j); } },
    { ::HashLut(Axis::Y, Axis::X, Angle::ROT270),
      [](int i, int j, int branchfactor) { return std::make_pair(j, branchfactor - 1 - i); } },
    { ::HashLut(Axis::Y, Axis::Y, Angle::ROT90),
      [](int i, int j, int branchfactor) { return std::make_pair(branchfactor - 1 - j, i); } },
    { ::HashLut(Axis::Y, Axis::Y, Angle::ROT270),
      [](int i, int j, int branchfactor) { return std::make_pair(j, branchfactor - 1 - i); } },
    { ::HashLut(Axis::Y, Axis::Z, Angle::ROT90),
      [](int i, int j, int branchfactor) { return std::make_pair(branchfactor - 1 - j, i); } },
    { ::HashLut(Axis::Y, Axis::Z, Angle::ROT180),
      [](int i, int j, int branchfactor) { return std::make_pair(branchfactor - 1 - i, j); } },
    { ::HashLut(Axis::Y, Axis::Z, Angle::ROT270),
      [](int i, int j, int vtkNotUsed(branchfactor)) { return std::make_pair(j, i); } } };
}

//----------------------------------------------------------------------------
void ReverseAxes(
  int axis, vtkDoubleArray* XCoordinate, vtkDoubleArray* YCoordinate, vtkDoubleArray* ZCoordinate)
{
  switch (axis)
  {
    case 0:
    default:
      ::ReverseDoubleArray(XCoordinate);
      break;

    case 1:
      ::ReverseDoubleArray(YCoordinate);
      break;

    case 2:
      ::ReverseDoubleArray(ZCoordinate);
      break;
  }
}

} // anonymous namespace

//----------------------------------------------------------------------------
void vtkAxisAlignedTransformFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent.GetNextIndent());
  os << indent << "Translation: " << Translation[0] << ", " << Translation[1] << ", "
     << Translation[2] << endl;
  os << indent << "Scale: " << Scale[0] << ", " << Scale[1] << ", " << Scale[2] << endl;
  os << indent << "Rotation Axis: " << ::AxisEnumToStr(RotationAxis) << endl;
  os << indent << "Rotation Angle: " << ::AngleEnumToStr(RotationAngle) << endl;
}

//------------------------------------------------------------------------------
int vtkAxisAlignedTransformFilter::FillInputPortInformation(int, vtkInformation* info)
{
  info->Remove(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE());
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkHyperTreeGrid");
  return 1;
}

//------------------------------------------------------------------------------
vtkTypeBool vtkAxisAlignedTransformFilter::ProcessRequest(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  if (request->Has(vtkDemandDrivenPipeline::REQUEST_DATA_OBJECT()))
  {
    return this->RequestDataObject(request, inputVector, outputVector);
  }
  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

//------------------------------------------------------------------------------
int vtkAxisAlignedTransformFilter::RequestDataObject(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  if (!inInfo)
  {
    return 0;
  }
  vtkDataObject* input = vtkDataObject::GetData(inInfo);
  if (input)
  {
    vtkInformation* outInfo = outputVector->GetInformationObject(0);
    vtkDataObject* output = outInfo->Get(vtkDataObject::DATA_OBJECT());

    if (!output || !output->IsA(input->GetClassName()))
    {
      vtkDataObject* newOutput = input->NewInstance();
      outInfo->Set(vtkDataObject::DATA_OBJECT(), newOutput);
      newOutput->FastDelete();
    }
  }
  return 1;
}

//------------------------------------------------------------------------------
void vtkAxisAlignedTransformFilter::GetTransform(vtkTransform* transform)
{
  transform->Scale(Scale);
  transform->Translate(Translation);
  double angle = RotationAngle * 90.0;

  switch (RotationAxis)
  {
    case Axis::X:
    default:
      transform->RotateX(angle);
      break;

    case Axis::Y:
      transform->RotateY(angle);
      break;

    case Axis::Z:
      transform->RotateZ(angle);
      break;
  }
}

//------------------------------------------------------------------------------
void vtkAxisAlignedTransformFilter::GetRotationMatrix(
  int axis, int rotation, int rotationMatrix[3][3])
{
  int cosTheta = 0, sinTheta = 0;
  switch (rotation)
  {
    case ROT0:
    default:
      cosTheta = 1;
      sinTheta = 0;
      break;

    case ROT90:
      cosTheta = 0;
      sinTheta = 1;
      break;

    case ROT180:
      cosTheta = -1;
      sinTheta = 0;
      break;

    case ROT270:
      cosTheta = 0;
      sinTheta = -1;
      break;
  }

  int I[3][3] = { { 1, 0, 0 }, { 0, 1, 0 }, { 0, 0, 1 } };
  std::memcpy(rotationMatrix, I, 9 * sizeof(int));

  switch (axis)
  {
    case Axis::X:
    default:
      rotationMatrix[1][1] = cosTheta;
      rotationMatrix[1][2] = -sinTheta;
      rotationMatrix[2][1] = sinTheta;
      rotationMatrix[2][2] = cosTheta;
      break;

    case Axis::Y:
      rotationMatrix[0][0] = cosTheta;
      rotationMatrix[0][2] = sinTheta;
      rotationMatrix[2][0] = -sinTheta;
      rotationMatrix[2][2] = cosTheta;
      break;

    case Axis::Z:
      rotationMatrix[0][0] = cosTheta;
      rotationMatrix[0][1] = -sinTheta;
      rotationMatrix[1][0] = sinTheta;
      rotationMatrix[1][1] = cosTheta;
      break;
  }
}

//------------------------------------------------------------------------------
std::vector<unsigned int> vtkAxisAlignedTransformFilter::ComputePermutation(
  unsigned int branchFactor, int axis, int normalAxis, int rotationAngle, int dimension)
{
  assert(dimension == 2 || dimension == 3);
  assert(rotationAngle != Angle::ROT0);
  unsigned int total =
    (dimension == 3) ? branchFactor * branchFactor * branchFactor : branchFactor * branchFactor;
  std::vector<unsigned int> perm(total);

  // To correctly handle 2D HTG rotations edge cases using a LuT
  auto lut = ::Create2DHtgLut();
  int hash = ::HashLut(axis, normalAxis, rotationAngle);
  if (auto it = lut.find(hash); it != lut.end() && dimension == 2)
  {
    rotationAngle = Angle::ROT0;
  }

  // Create permutation list for 90° rotation
  for (unsigned int newIndex = 0; newIndex < total; ++newIndex)
  {
    unsigned int iNew = 0, jNew = 0, kNew = 0;
    if (dimension == 3)
    {
      iNew = newIndex % branchFactor;
      jNew = (newIndex / branchFactor) % branchFactor;
      kNew = newIndex / (branchFactor * branchFactor);
    }
    else
    {
      iNew = newIndex % branchFactor;
      jNew = newIndex / branchFactor;
    }

    unsigned int iOrigin = 0, jOrigin = 0, kOrigin = 0;
    if (dimension == 3)
    {
      switch (axis)
      {
        case Axis::X:
        default:
          iOrigin = iNew;
          jOrigin = kNew;
          kOrigin = branchFactor - 1 - jNew;
          break;

        case Axis::Y:
          iOrigin = branchFactor - 1 - kNew;
          jOrigin = jNew;
          kOrigin = iNew;
          break;

        case Axis::Z:
          iOrigin = jNew;
          jOrigin = branchFactor - 1 - iNew;
          kOrigin = kNew;
          break;
      }
      unsigned int originIndex =
        iOrigin + branchFactor * jOrigin + branchFactor * branchFactor * kOrigin;
      perm[newIndex] = originIndex;
    }
    else
    {
      if (normalAxis == axis)
      {
        iOrigin = jNew;
        jOrigin = branchFactor - 1 - iNew;
      }
      else
      {
        iOrigin = iNew;
        jOrigin = jNew;
      }

      if (auto it = lut.find(hash); it != lut.end())
      {
        std::pair<int, int> ij = it->second(iNew, jNew, branchFactor);
        iOrigin = ij.first;
        jOrigin = ij.second;
      }

      unsigned int originIndex = iOrigin + branchFactor * jOrigin;
      perm[newIndex] = originIndex;
    }
  }

  // Re-apply permutation for rotations != 90°
  std::vector<unsigned int> perm90 = perm;
  for (int r = 1; r < rotationAngle; ++r)
  {
    std::vector<unsigned int> tmpPerm(total);
    for (unsigned int i = 0; i < total; ++i)
    {
      tmpPerm[i] = perm90[perm[i]];
    }
    perm = std::move(tmpPerm);
  }
  return perm;
}

//------------------------------------------------------------------------------
void vtkAxisAlignedTransformFilter::CopyAndRotate(vtkHyperTree* input, vtkHyperTree* output,
  vtkIdType inputIndex, vtkIdType outputIndex, const std::vector<unsigned int>& permutation,
  unsigned int depth, vtkHyperTreeGridNonOrientedCursor* cursor)
{
  if (cursor->IsLeaf() || cursor->IsMasked())
  {
    if (!cursor->IsRoot())
    {
      cursor->ToParent();
    }
    return;
  }

  output->SubdivideLeaf(outputIndex, depth);

  vtkIdType inputBase = input->GetElderChildIndex(inputIndex);
  vtkIdType outputBase = output->GetElderChildIndex(outputIndex);

  size_t nChildren = permutation.size();

  // For each child index in the output, determine the corresponding
  // child in the input using the permutation vector
  for (size_t i = 0; i < nChildren; ++i)
  {
    vtkIdType inputChildIndex = inputBase + permutation[i];
    vtkIdType outputChildIndex = outputBase + i;

    cursor->ToChild(permutation[i]);
    this->CopyAndRotate(
      input, output, inputChildIndex, outputChildIndex, permutation, depth + 1, cursor);
  }
  if (!cursor->IsRoot())
  {
    cursor->ToParent();
  }
}

//------------------------------------------------------------------------------
void ApplyMask(vtkHyperTree* input, vtkHyperTree* output, vtkIdType inputIndex,
  vtkIdType outputIndex, const std::vector<unsigned int>& permutation,
  vtkHyperTreeGridNonOrientedCursor* cursorInput, vtkHyperTreeGridNonOrientedCursor* cursorOutput)
{
  auto returnRecursion = [cursorInput, cursorOutput]()
  {
    if (!cursorInput->IsRoot())
    {
      cursorInput->ToParent();
      cursorOutput->ToParent();
    }
  };

  if (cursorInput->IsMasked())
  {
    cursorOutput->SetMask(true);
    returnRecursion();
    return;
  }

  if (cursorInput->IsLeaf())
  {
    returnRecursion();
    return;
  }

  vtkIdType inputBase = input->GetElderChildIndex(inputIndex);
  vtkIdType outputBase = output->GetElderChildIndex(outputIndex);

  size_t nChildren = permutation.size();
  for (size_t i = 0; i < nChildren; ++i)
  {
    vtkIdType inputChildIndex = inputBase + permutation[i];
    vtkIdType outputChildIndex = outputBase + i;
    cursorInput->ToChild(permutation[i]);
    cursorOutput->ToChild(static_cast<unsigned char>(i));

    ApplyMask(
      input, output, inputChildIndex, outputChildIndex, permutation, cursorInput, cursorOutput);
  }
  returnRecursion();
}

//------------------------------------------------------------------------------
void CopyRotatedDataHTG(vtkHyperTree* input, vtkHyperTree* output, vtkHyperTreeGrid* inputHTG,
  vtkHyperTreeGrid* outputHTG, vtkIdType inputIndex, vtkIdType outputIndex,
  const std::vector<unsigned int>& permutation, vtkHyperTreeGridNonOrientedCursor* cursorInput,
  vtkHyperTreeGridNonOrientedCursor* cursorOutput)
{
  auto returnRecursion = [cursorInput, cursorOutput]()
  {
    if (!cursorInput->IsRoot())
    {
      cursorInput->ToParent();
      cursorOutput->ToParent();
    }
  };

  if (cursorInput->IsMasked())
  {
    returnRecursion();
    return;
  }

  // Actual copy of the data
  vtkCellData* inputCellData = inputHTG->GetCellData();
  vtkCellData* outputCellData = outputHTG->GetCellData();
  for (int arrayId = 0; arrayId < inputCellData->GetNumberOfArrays(); ++arrayId)
  {
    vtkDataArray* inputArray = inputCellData->GetArray(arrayId);
    vtkDataArray* outputArray = outputCellData->GetArray(arrayId);

    int inputTuple = cursorInput->GetGlobalNodeIndex();
    int outputTuple = cursorOutput->GetGlobalNodeIndex();

    outputArray->SetTuple(outputTuple, inputArray->GetTuple(inputTuple));
  }

  if (cursorInput->IsLeaf())
  {
    returnRecursion();
    return;
  }

  vtkIdType inputBase = input->GetElderChildIndex(inputIndex);
  vtkIdType outputBase = output->GetElderChildIndex(outputIndex);

  size_t nChildren = permutation.size();
  for (size_t i = 0; i < nChildren; ++i)
  {
    vtkIdType inputChildIndex = inputBase + permutation[i];
    vtkIdType outputChildIndex = outputBase + i;

    cursorInput->ToChild(permutation[i]);
    cursorOutput->ToChild(static_cast<unsigned char>(i));
    CopyRotatedDataHTG(input, output, inputHTG, outputHTG, inputChildIndex, outputChildIndex,
      permutation, cursorInput, cursorOutput);
  }
  returnRecursion();
}

//------------------------------------------------------------------------------
vtkHyperTree* vtkAxisAlignedTransformFilter::CreateNewRotatedHyperTree(
  vtkHyperTreeGrid* htg, vtkHyperTree* dest, const std::vector<unsigned int>& permutation)
{
  int branchFactor = dest->GetBranchFactor();
  int dimension = dest->GetDimension();

  vtkHyperTree* newTree = vtkHyperTree::CreateInstance(branchFactor, dimension);

  vtkNew<vtkHyperTreeGridNonOrientedCursor> cursor;
  cursor->Initialize(htg, dest->GetTreeIndex(), false);

  this->CopyAndRotate(dest, newTree, 0, 0, permutation, 0, cursor);

  return newTree;
}

//------------------------------------------------------------------------------
void vtkAxisAlignedTransformFilter::ComputeCellScale(vtkDataArray* xCoords, vtkDataArray* yCoords,
  vtkDataArray* zCoords, int dims[3], double scales[3])
{
  if (dims[0] != 1)
  {
    scales[0] = xCoords->GetTuple1(1) - xCoords->GetTuple1(0);
  }
  if (dims[1] != 1)
  {
    scales[1] = yCoords->GetTuple1(1) - yCoords->GetTuple1(0);
  }
  if (dims[2] != 1)
  {
    scales[2] = zCoords->GetTuple1(1) - zCoords->GetTuple1(0);
  }
}

//------------------------------------------------------------------------------
void vtkAxisAlignedTransformFilter::ApplyCellScale(
  vtkHyperTreeGridNonOrientedCursor* cursor, double scales[3])
{
  if (cursor->IsMasked())
  {
    cursor->ToParent();
    return;
  }

  cursor->GetTree()->InitializeScales(scales, true);

  if (!cursor->IsLeaf())
  {
    for (int i = 0; i < cursor->GetTree()->GetNumberOfChildren(); ++i)
    {
      cursor->ToChild(i);
      this->ApplyCellScale(cursor, scales);
    }
  }
  if (!cursor->IsRoot())
  {
    cursor->ToParent();
  }
}

//----------------------------------------------------------------------------
int vtkAxisAlignedTransformFilter::GetRotatedId(
  int id, int rotationMatrix[3][3], int newDims[3], int dims[3], int Tvec[3], bool transposed)
{
  int x, y, z;
  if (transposed)
  {
    z = id % std::max(newDims[2] - 1, 1);
    y = (id / (std::max(newDims[2] - 1, 1))) % std::max(newDims[1] - 1, 1);
    x = id / std::max(newDims[2] - 1, 1) / std::max(newDims[1] - 1, 1);
  }
  else
  {
    x = id % std::max(newDims[0] - 1, 1);
    y = (id / (std::max(newDims[0] - 1, 1))) % std::max(newDims[1] - 1, 1);
    z = id / std::max(newDims[0] - 1, 1) / std::max(newDims[1] - 1, 1);
  }

  int newX =
    rotationMatrix[0][0] * x + rotationMatrix[1][0] * y + rotationMatrix[2][0] * z + Tvec[0];
  int newY =
    rotationMatrix[0][1] * x + rotationMatrix[1][1] * y + rotationMatrix[2][1] * z + Tvec[1];
  int newZ =
    rotationMatrix[0][2] * x + rotationMatrix[1][2] * y + rotationMatrix[2][2] * z + Tvec[2];

  if (transposed)
  {
    return newX * std::max(dims[2] - 1, 1) * std::max(dims[1] - 1, 1) +
      newY * std::max(dims[2] - 1, 1) + newZ;
  }
  else
  {
    return newZ * std::max(dims[0] - 1, 1) * std::max(dims[1] - 1, 1) +
      newY * std::max(dims[0] - 1, 1) + newX;
  }
}

//----------------------------------------------------------------------------
vtkAxisAlignedTransformFilter::Axis vtkAxisAlignedTransformFilter::FindNormalAxis(int dims[3])
{
  if (dims[0] == 1)
  {
    return Axis::X;
  }
  if (dims[1] == 1)
  {
    return Axis::Y;
  }
  return Axis::Z;
}

//----------------------------------------------------------------------------
int vtkAxisAlignedTransformFilter::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  vtkDataSet* inputDS = vtkDataSet::GetData(inputVector[0], 0);
  vtkHyperTreeGrid* inputHTG = vtkHyperTreeGrid::GetData(inputVector[0], 0);

  if (inputDS || inputHTG)
  {
    vtkDataObject* input = vtkDataObject::GetData(inputVector[0], 0);
    vtkDataObject* output = vtkDataObject::GetData(outInfo);
    return this->Dispatch(input, output);
  }
  else
  {
    vtkErrorMacro("Unhandled data type: " << inputVector[0]->GetClassName());
    return 0;
  }
}

//----------------------------------------------------------------------------
bool vtkAxisAlignedTransformFilter::ProcessGeneric(
  vtkDataObject* inputDataObject, vtkDataObject* outputDataObject)
{
  vtkNew<vtkTransformFilter> transformFilter;

  transformFilter->SetInputData(inputDataObject);
  vtkNew<vtkTransform> transform;
  this->GetTransform(transform);
  transformFilter->SetTransform(transform);
  transformFilter->Update();
  vtkDataSet* transformedOutput = transformFilter->GetOutput();

  outputDataObject->ShallowCopy(transformedOutput);
  return true;
}

//----------------------------------------------------------------------------
bool vtkAxisAlignedTransformFilter::ProcessImageData(
  vtkImageData* inputID, vtkImageData* outputID, int rotationMatrix[3][3])
{
  outputID->DeepCopy(inputID);

  outputID->SetOrigin(inputID->GetOrigin()[0] + Translation[0],
    inputID->GetOrigin()[1] + Translation[1], inputID->GetOrigin()[2] + Translation[2]);

  outputID->SetDirectionMatrix(Scale[0] * rotationMatrix[0][0], Scale[1] * rotationMatrix[0][1],
    Scale[2] * rotationMatrix[0][2], Scale[0] * rotationMatrix[1][0],
    Scale[1] * rotationMatrix[1][1], Scale[2] * rotationMatrix[1][2],
    Scale[0] * rotationMatrix[2][0], Scale[1] * rotationMatrix[2][1],
    Scale[2] * rotationMatrix[2][2]);
  return true;
}

//----------------------------------------------------------------------------
bool vtkAxisAlignedTransformFilter::ProcessRectilinearGrid(
  vtkRectilinearGrid* inputRG, vtkRectilinearGrid* outputRG, int rotationMatrix[3][3])
{
  outputRG->DeepCopy(inputRG);
  ::ApplyScale(outputRG, Scale);
  if (RotationAngle == ROT0)
  {
    ::ApplyTranslation(outputRG, Translation);
    return true;
  }

  int dims[3];
  outputRG->GetDimensions(dims);

  if (outputRG->GetDataDimension() == 1)
  {
    vtkErrorMacro("Rotations for 1D RectilinearGrid are not supported.");
    return false;
  }

  // Swap dimensions according to rotation
  int newDims[3];
  std::memcpy(newDims, dims, 3 * sizeof(int));
  for (int i = 0; i < 3; ++i)
  {
    for (int j = 0; j < 3; ++j)
    {
      if (rotationMatrix[i][j] != 0)
      {
        newDims[i] = dims[j];
        break;
      }
    }
  }

  vtkDoubleArray* XCoordinate = vtkDoubleArray::SafeDownCast(outputRG->GetXCoordinates());
  vtkDoubleArray* YCoordinate = vtkDoubleArray::SafeDownCast(outputRG->GetYCoordinates());
  vtkDoubleArray* ZCoordinate = vtkDoubleArray::SafeDownCast(outputRG->GetZCoordinates());

  // Compute a translation vector for inverted dimensions
  // after the rotation to stay in the positive quadrant
  int tvec[3] = { 0, 0, 0 };
  for (int i = 0; i < 3; i++)
  {
    for (int j = 0; j < 3; j++)
    {
      if (rotationMatrix[i][j] < 0)
      {
        tvec[j] += std::max(newDims[i] - 1, 1) - 1;
        ::ReverseAxes(j, XCoordinate, YCoordinate, ZCoordinate);
      }
    }
  }

  outputRG->SetDimensions(newDims[0], newDims[1], newDims[2]);
  XCoordinate->Register(nullptr);
  YCoordinate->Register(nullptr);
  ZCoordinate->Register(nullptr);
  ::SwapXYZCoordinates(XCoordinate, YCoordinate, ZCoordinate, RotationAngle, RotationAxis);
  outputRG->SetXCoordinates(XCoordinate);
  outputRG->SetYCoordinates(YCoordinate);
  outputRG->SetZCoordinates(ZCoordinate);
  XCoordinate->Delete();
  YCoordinate->Delete();
  ZCoordinate->Delete();

  vtkPointData* inPD = inputRG->GetPointData();
  vtkPointData* outPD = outputRG->GetPointData();
  vtkCellData* inCD = inputRG->GetCellData();
  vtkCellData* outCD = outputRG->GetCellData();

  // Lambda to copy data arrays
  auto copyRotatedData =
    [](vtkDataSetAttributes* input, vtkDataSetAttributes* output, int oldId, int newId)
  {
    for (int arrayId = 0; arrayId < input->GetNumberOfArrays(); ++arrayId)
    {
      vtkDataArray* inputArray = input->GetArray(arrayId);
      vtkDataArray* outputArray = output->GetArray(arrayId);
      outputArray->SetTuple(oldId, inputArray->GetTuple(newId));
    }
  };

  int maxIndex =
    std::max(newDims[0] - 1, 1) * std::max(newDims[1] - 1, 1) * std::max(newDims[2] - 1, 1);
  for (int i = 0; i < maxIndex; ++i)
  {
    int newId = this->GetRotatedId(i, rotationMatrix, newDims, dims, tvec, false);
    copyRotatedData(inCD, outCD, i, newId);
    copyRotatedData(inPD, outPD, i, newId);
  }

  ::ApplyTranslation(outputRG, Translation);
  return true;
}

//----------------------------------------------------------------------------
bool vtkAxisAlignedTransformFilter::ProcessHTG(
  vtkHyperTreeGrid* inputHTG, vtkHyperTreeGrid* outputHTG, int rotationMatrix[3][3])
{
  outputHTG->DeepCopy(inputHTG);

  // If HTG is empty, nothing to do
  if (inputHTG->GetMaxNumberOfTrees() == 0)
  {
    return true;
  }

  ::ApplyScale(outputHTG, Scale);

  // Lambda to apply cell level scaling
  auto applyCellScale = [this](vtkHyperTreeGrid* htg, double scales[3])
  {
    vtkHyperTreeGrid::vtkHyperTreeGridIterator iterator;
    iterator.Initialize(htg);
    while (vtkHyperTree* tree = iterator.GetNextTree())
    {
      vtkNew<vtkHyperTreeGridNonOrientedCursor> cursor;
      cursor->Initialize(htg, tree->GetTreeIndex(), false);
      if (!cursor->IsMasked())
      {
        this->ApplyCellScale(cursor, scales);
      }
    }
  };

  // Lambda to correctly handle interfaces intercepts and normals
  auto interfaceUpdate = [inputHTG, outputHTG](
                           double translation[3], double scale[3], int rotMatrix[3][3])
  {
    if (inputHTG->GetHasInterface())
    {
      vtkDataArray* interceptArray =
        outputHTG->GetCellData()->GetArray(outputHTG->GetInterfaceInterceptsName());
      vtkDataArray* normalsArray =
        outputHTG->GetCellData()->GetArray(outputHTG->GetInterfaceNormalsName());
      for (vtkIdType i = 0; i < interceptArray->GetNumberOfTuples(); ++i)
      {
        double distance = interceptArray->GetComponent(i, 0);
        double distance2 = interceptArray->GetComponent(i, 1);
        double* normal = normalsArray->GetTuple(i);
        vtkVector3d t(translation);
        vtkVector3d n(rotMatrix[0][0] * normal[0] / scale[0] +
            rotMatrix[0][1] * normal[1] / scale[1] + rotMatrix[0][2] * normal[2] / scale[2],
          rotMatrix[1][0] * normal[0] / scale[0] + rotMatrix[1][1] * normal[1] / scale[1] +
            rotMatrix[1][2] * normal[2] / scale[2],
          rotMatrix[2][0] * normal[0] / scale[0] + rotMatrix[2][1] * normal[1] / scale[1] +
            rotMatrix[2][2] * normal[2] / scale[2]);

        interceptArray->SetComponent(i, 0, (distance - n.Dot(t)));
        interceptArray->SetComponent(i, 1, (distance2 - n.Dot(t)));
        normalsArray->SetTuple(i, n.GetData());
      }
    }
  };

  int dims[3];
  outputHTG->GetDimensions(dims);

  if (outputHTG->GetDimension() == 1)
  {
    vtkErrorMacro("Rotations for 1D HTG are not supported.");
    return false;
  }

  int normalAxis = Axis::X;
  if (outputHTG->GetDimension() == 2)
  {
    normalAxis = this->FindNormalAxis(dims);
  }

  vtkDoubleArray* XCoordinate = vtkDoubleArray::SafeDownCast(outputHTG->GetXCoordinates());
  vtkDoubleArray* YCoordinate = vtkDoubleArray::SafeDownCast(outputHTG->GetYCoordinates());
  vtkDoubleArray* ZCoordinate = vtkDoubleArray::SafeDownCast(outputHTG->GetZCoordinates());
  double scales[3];
  this->ComputeCellScale(XCoordinate, YCoordinate, ZCoordinate, dims, scales);

  // Swap dimensions and cell scales according to rotation
  int newDims[3];
  std::memcpy(newDims, dims, 3 * sizeof(int));
  double newScales[3];
  std::memcpy(newScales, scales, 3 * sizeof(double));
  for (int i = 0; i < 3; ++i)
  {
    for (int j = 0; j < 3; ++j)
    {
      if (rotationMatrix[i][j] != 0)
      {
        newDims[i] = dims[j];
        newScales[i] = scales[j];
        break;
      }
    }
  }

  // No need to construct rotated HTG for 0° rotation
  if (RotationAngle == ROT0)
  {
    applyCellScale(outputHTG, newScales);
    interfaceUpdate(Translation, Scale, rotationMatrix);
    ::ApplyTranslation(outputHTG, Translation);
    return true;
  }

  int dimension = inputHTG->GetDimension();
  int branchFactor = inputHTG->GetBranchFactor();
  std::vector<unsigned int> permutation =
    ComputePermutation(branchFactor, RotationAxis, normalAxis, RotationAngle, dimension);

  // Compute a translation vector for inverted dimensions
  // after the rotation to stay in the positive quadrant
  int tvec[3] = { 0, 0, 0 };
  for (int i = 0; i < 3; i++)
  {
    for (int j = 0; j < 3; j++)
    {
      if (rotationMatrix[i][j] < 0)
      {
        tvec[j] += std::max(newDims[i] - 1, 1) - 1;
        ::ReverseAxes(j, XCoordinate, YCoordinate, ZCoordinate);
      }
    }
  }

  // Swap XYZ coordinates
  outputHTG->SetDimensions(newDims[0], newDims[1], newDims[2]);
  XCoordinate->Register(nullptr);
  YCoordinate->Register(nullptr);
  ZCoordinate->Register(nullptr);
  ::SwapXYZCoordinates(XCoordinate, YCoordinate, ZCoordinate, RotationAngle, RotationAxis);
  outputHTG->SetXCoordinates(XCoordinate);
  outputHTG->SetYCoordinates(YCoordinate);
  outputHTG->SetZCoordinates(ZCoordinate);
  XCoordinate->Delete();
  YCoordinate->Delete();
  ZCoordinate->Delete();

  // Rotate HTG
  int cumulativeVertices = 0;
  int maxIndex =
    std::max(newDims[0] - 1, 1) * std::max(newDims[1] - 1, 1) * std::max(newDims[2] - 1, 1);
  for (int i = 0; i < maxIndex; ++i)
  {
    int newId = this->GetRotatedId(
      i, rotationMatrix, newDims, dims, tvec, outputHTG->GetTransposedRootIndexing());

    vtkHyperTree* ht = inputHTG->GetTree(newId);
    if (!ht)
    {
      outputHTG->RemoveTree(i);
      continue;
    }
    vtkHyperTree* rotatedHT = CreateNewRotatedHyperTree(inputHTG, ht, permutation);
    rotatedHT->SetGlobalIndexStart(cumulativeVertices);
    cumulativeVertices += rotatedHT->GetNumberOfVertices();

    outputHTG->SetTree(i, rotatedHT);
    rotatedHT->Delete();
  }

  // Create and set empty mask
  vtkNew<vtkBitArray> mask;
  mask->SetNumberOfTuples(outputHTG->GetNumberOfCells());
  for (int i = 0; i < mask->GetNumberOfTuples(); ++i)
  {
    mask->SetTuple1(i, 0);
  }
  outputHTG->SetMask(mask);

  applyCellScale(outputHTG, newScales);

  // Apply masking
  for (int i = 0; i < maxIndex; ++i)
  {
    int newId =
      GetRotatedId(i, rotationMatrix, newDims, dims, tvec, outputHTG->GetTransposedRootIndexing());

    vtkHyperTree* inputHT = inputHTG->GetTree(newId);
    if (!inputHT)
    {
      continue;
    }
    vtkHyperTree* outputHT = outputHTG->GetTree(i);
    vtkNew<vtkHyperTreeGridNonOrientedCursor> cursorIn;
    vtkNew<vtkHyperTreeGridNonOrientedCursor> cursorOut;
    cursorIn->Initialize(inputHTG, inputHT->GetTreeIndex(), true);
    cursorOut->Initialize(outputHTG, outputHT->GetTreeIndex(), true);
    ApplyMask(inputHT, outputHT, 0, 0, permutation, cursorIn, cursorOut);
  }

  // Reset cell datas
  vtkCellData* inputCellData = inputHTG->GetCellData();
  vtkCellData* outputCellData = outputHTG->GetCellData();
  for (int arrayId = 0; arrayId < inputCellData->GetNumberOfArrays(); ++arrayId)
  {
    vtkDataArray* inputArray = inputCellData->GetArray(arrayId);
    vtkDataArray* outputArray = outputCellData->GetArray(arrayId);

    outputArray->Initialize();

    vtkIdType numComponents = inputArray->GetNumberOfComponents();
    vtkIdType numTuples = inputArray->GetNumberOfTuples();

    outputArray->SetNumberOfComponents(numComponents);
    outputArray->SetNumberOfTuples(numTuples);
    outputArray->Fill(inputArray->GetComponent(0, 0));
  }

  // Copy data
  for (int i = 0; i < maxIndex; ++i)
  {
    int newId =
      GetRotatedId(i, rotationMatrix, newDims, dims, tvec, outputHTG->GetTransposedRootIndexing());
    vtkHyperTree* inputHT = inputHTG->GetTree(newId);
    if (!inputHT)
    {
      continue;
    }
    vtkHyperTree* outputHT = outputHTG->GetTree(i);
    vtkNew<vtkHyperTreeGridNonOrientedCursor> cursorIn;
    vtkNew<vtkHyperTreeGridNonOrientedCursor> cursorOut;
    cursorIn->Initialize(inputHTG, inputHT->GetTreeIndex(), true);
    cursorOut->Initialize(outputHTG, outputHT->GetTreeIndex(), true);

    CopyRotatedDataHTG(
      inputHT, outputHT, inputHTG, outputHTG, 0, 0, permutation, cursorIn, cursorOut);
  }

  interfaceUpdate(Translation, Scale, rotationMatrix);

  ::ApplyTranslation(outputHTG, Translation);
  return true;
}

//------------------------------------------------------------------------------
bool vtkAxisAlignedTransformFilter::Dispatch(
  vtkDataObject* inputDataObject, vtkDataObject* outputDataObject)
{

  if (vtkUnstructuredGrid::SafeDownCast(inputDataObject) ||
    vtkExplicitStructuredGrid::SafeDownCast(inputDataObject) ||
    vtkStructuredGrid::SafeDownCast(inputDataObject) || vtkPolyData::SafeDownCast(inputDataObject))
  {
    return this->ProcessGeneric(inputDataObject, outputDataObject);
  }
  else
  {
    int rotationMatrix[3][3];
    this->GetRotationMatrix(RotationAxis, RotationAngle, rotationMatrix);

    if (auto imgData = vtkImageData::SafeDownCast(inputDataObject))
    {
      auto output = vtkImageData::SafeDownCast(outputDataObject);
      return this->ProcessImageData(imgData, output, rotationMatrix);
    }
    else if (auto rg = vtkRectilinearGrid::SafeDownCast(inputDataObject))
    {
      auto output = vtkRectilinearGrid::SafeDownCast(outputDataObject);
      return this->ProcessRectilinearGrid(rg, output, rotationMatrix);
    }
    else if (auto htg = vtkHyperTreeGrid::SafeDownCast(inputDataObject))
    {
      auto output = vtkHyperTreeGrid::SafeDownCast(outputDataObject);
      return this->ProcessHTG(htg, output, rotationMatrix);
    }
    else
    {
      vtkErrorMacro("AxisAlignedTransform: Unhandled type of DataSet ("
        << inputDataObject->GetClassName() << ")");
      return false;
    }
  }
}

VTK_ABI_NAMESPACE_END
