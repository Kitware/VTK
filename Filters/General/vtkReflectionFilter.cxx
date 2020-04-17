/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkReflectionFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkReflectionFilter.h"

#include "vtkBoundingBox.h"
#include "vtkCell.h"
#include "vtkCellData.h"
#include "vtkCellType.h"
#include "vtkCompositeDataIterator.h"
#include "vtkHigherOrderHexahedron.h"
#include "vtkHigherOrderQuadrilateral.h"
#include "vtkHigherOrderTetra.h"
#include "vtkHigherOrderWedge.h"
#include "vtkIdList.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkSmartPointer.h"
#include "vtkUnstructuredGrid.h"

namespace
{
/**
 * method to determine which arrays from a field data can be flipped.
 * Only 3/6/9 component signed data array are considered flippable.
 */
static void FindFlippableArrays(
  vtkFieldData* fd, std::vector<std::pair<vtkIdType, int> >& flippableArrays)
{
  // Find all flippable arrays
  for (int iArr = 0; iArr < fd->GetNumberOfArrays(); iArr++)
  {
    vtkDataArray* array = vtkDataArray::SafeDownCast(fd->GetAbstractArray(iArr));
    if (!array)
    {
      continue;
    }

    // Only signed arrays are flippable
    int dataType = array->GetDataType();
    if ((dataType == VTK_CHAR && VTK_TYPE_CHAR_IS_SIGNED) || dataType == VTK_SIGNED_CHAR ||
      dataType == VTK_SHORT || dataType == VTK_INT || dataType == VTK_LONG ||
      dataType == VTK_FLOAT || dataType == VTK_DOUBLE || dataType == VTK_ID_TYPE)
    {
      // Only vectors and tensors are flippable
      int nComp = array->GetNumberOfComponents();
      if (nComp == 3 || nComp == 6 || nComp == 9)
      {
        flippableArrays.push_back(std::make_pair(iArr, nComp));
      }
    }
  }
}
}

vtkStandardNewMacro(vtkReflectionFilter);

//---------------------------------------------------------------------------
vtkReflectionFilter::vtkReflectionFilter()
{
  this->Plane = USE_X_MIN;
  this->Center = 0.0;
  this->CopyInput = 1;
  this->FlipAllInputArrays = false;
}

//---------------------------------------------------------------------------
vtkReflectionFilter::~vtkReflectionFilter() = default;

//---------------------------------------------------------------------------
void vtkReflectionFilter::FlipTuple(double* tuple, int* mirrorDir, int nComp)
{
  for (int j = 0; j < nComp; j++)
  {
    tuple[j] *= mirrorDir[j];
  }
}

//---------------------------------------------------------------------------
int vtkReflectionFilter::ComputeBounds(vtkDataObject* input, double bounds[6])
{
  // get the input and output
  vtkDataSet* inputDS = vtkDataSet::SafeDownCast(input);
  vtkCompositeDataSet* inputCD = vtkCompositeDataSet::SafeDownCast(input);

  if (inputDS)
  {
    inputDS->GetBounds(bounds);
    return 1;
  }

  if (inputCD)
  {
    vtkBoundingBox bbox;

    vtkSmartPointer<vtkCompositeDataIterator> iter;
    iter.TakeReference(inputCD->NewIterator());
    for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
      vtkDataSet* ds = vtkDataSet::SafeDownCast(iter->GetCurrentDataObject());
      if (!ds)
      {
        vtkErrorMacro("Input composite dataset must be comprised for vtkDataSet "
                      "subclasses alone.");
        return 0;
      }
      bbox.AddBounds(ds->GetBounds());
    }
    if (bbox.IsValid())
    {
      bbox.GetBounds(bounds);
      return 1;
    }
  }

  return 0;
}

//---------------------------------------------------------------------------
vtkIdType vtkReflectionFilter::ReflectNon3DCell(
  vtkDataSet* input, vtkUnstructuredGrid* output, vtkIdType cellId, vtkIdType numInputPoints)
{
  vtkNew<vtkIdList> cellPts;
  input->GetCellPoints(cellId, cellPts);
  int numCellPts = cellPts->GetNumberOfIds();
  std::vector<vtkIdType> newCellPts(numCellPts);
  int cellType = input->GetCellType(cellId);
  switch (cellType)
  {
    case VTK_QUADRATIC_EDGE:
    case VTK_CUBIC_LINE:
    case VTK_BEZIER_CURVE:
    case VTK_LAGRANGE_CURVE:
    {
      for (int i = 0; i < numCellPts; i++)
      {
        newCellPts[i] = cellPts->GetId(i);
      }
      break;
    }
    case VTK_QUADRATIC_TRIANGLE:
    {
      newCellPts[0] = cellPts->GetId(2);
      newCellPts[1] = cellPts->GetId(1);
      newCellPts[2] = cellPts->GetId(0);
      newCellPts[3] = cellPts->GetId(4);
      newCellPts[4] = cellPts->GetId(3);
      newCellPts[5] = cellPts->GetId(5);
      break;
    }
    case VTK_BEZIER_TRIANGLE:
    case VTK_LAGRANGE_TRIANGLE:
    {
      if (numCellPts == 7)
      {
        newCellPts[0] = cellPts->GetId(0);
        newCellPts[1] = cellPts->GetId(2);
        newCellPts[2] = cellPts->GetId(1);
        newCellPts[3] = cellPts->GetId(5);
        newCellPts[4] = cellPts->GetId(4);
        newCellPts[5] = cellPts->GetId(3);
        newCellPts[6] = cellPts->GetId(6);
      }
      else
      {
        int order = (sqrt(8 * numCellPts + 1) - 3) / 2;
        int offset = 0;
        while (order > 0)
        {
          newCellPts[offset + 0] = cellPts->GetId(offset + 0);
          newCellPts[offset + 1] = cellPts->GetId(offset + 2);
          newCellPts[offset + 2] = cellPts->GetId(offset + 1);
          const int contour_n = 3 * (order - 1);
          for (int contour = 0; contour < contour_n; contour++)
          {
            newCellPts[offset + 3 + contour] = cellPts->GetId(offset + 3 + contour_n - 1 - contour);
          }
          if (order == 3) // This is is there is a single point in the middle
          {
            newCellPts[offset + 3 + contour_n] = cellPts->GetId(offset + 3 + contour_n);
          }
          order -= 3;
          offset += 3 * order;
        }
      }

      break;
    }
    case VTK_QUADRATIC_QUAD:
    {
      newCellPts[0] = cellPts->GetId(1);
      newCellPts[1] = cellPts->GetId(0);
      newCellPts[2] = cellPts->GetId(3);
      newCellPts[3] = cellPts->GetId(2);
      newCellPts[4] = cellPts->GetId(4);
      newCellPts[5] = cellPts->GetId(7);
      newCellPts[6] = cellPts->GetId(6);
      newCellPts[7] = cellPts->GetId(5);
      break;
    }
    case VTK_BIQUADRATIC_QUAD:
    {
      newCellPts[0] = cellPts->GetId(1);
      newCellPts[1] = cellPts->GetId(0);
      newCellPts[2] = cellPts->GetId(3);
      newCellPts[3] = cellPts->GetId(2);
      newCellPts[4] = cellPts->GetId(4);
      newCellPts[5] = cellPts->GetId(7);
      newCellPts[6] = cellPts->GetId(6);
      newCellPts[7] = cellPts->GetId(5);
      newCellPts[8] = cellPts->GetId(8);
      break;
    }
    case VTK_QUADRATIC_LINEAR_QUAD:
    {
      newCellPts[0] = cellPts->GetId(1);
      newCellPts[1] = cellPts->GetId(0);
      newCellPts[2] = cellPts->GetId(3);
      newCellPts[3] = cellPts->GetId(2);
      newCellPts[4] = cellPts->GetId(4);
      newCellPts[5] = cellPts->GetId(5);
      break;
    }
    case VTK_BEZIER_QUADRILATERAL:
    case VTK_LAGRANGE_QUADRILATERAL:
    {
      vtkCell* cell = input->GetCell(cellId);
      vtkHigherOrderQuadrilateral* cellQuad = dynamic_cast<vtkHigherOrderQuadrilateral*>(cell);
      const int* order = cellQuad->GetOrder();
      const int i_max_half = (order[0] % 2 == 0) ? (order[0] + 2) / 2 : (order[0] + 1) / 2;
      for (int i = 0; i < i_max_half; i++)
      {
        const int i_reversed = order[0] - i;
        for (int j = 0; j < order[1] + 1; j++)
        {
          const int node_id = vtkHigherOrderQuadrilateral::PointIndexFromIJK(i, j, order);
          if (i != i_reversed)
          {
            const int node_id_reversed =
              vtkHigherOrderQuadrilateral::PointIndexFromIJK(i_reversed, j, order);
            newCellPts[node_id_reversed] = cellPts->GetId(node_id);
            newCellPts[node_id] = cellPts->GetId(node_id_reversed);
          }
          else
          {
            newCellPts[node_id] = cellPts->GetId(node_id);
          }
        }
      }
      break;
    }
    default:
    {
      if (input->GetCell(cellId)->IsA("vtkNonLinearCell") || cellType > VTK_POLYHEDRON)
      {
        vtkWarningMacro("Cell may be inverted");
      }
      for (int j = numCellPts - 1; j >= 0; j--)
      {
        newCellPts[numCellPts - 1 - j] = cellPts->GetId(j);
      }
    }
  } // end switch
  if (this->CopyInput)
  {
    for (int j = 0; j < numCellPts; j++)
    {
      newCellPts[j] += numInputPoints;
    }
  }
  return output->InsertNextCell(cellType, numCellPts, &newCellPts[0]);
}

//---------------------------------------------------------------------------
int vtkReflectionFilter::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the input and output
  vtkDataSet* inputDS = vtkDataSet::GetData(inputVector[0], 0);
  vtkUnstructuredGrid* outputUG = vtkUnstructuredGrid::GetData(outputVector, 0);

  vtkCompositeDataSet* inputCD = vtkCompositeDataSet::GetData(inputVector[0], 0);
  vtkCompositeDataSet* outputCD = vtkCompositeDataSet::GetData(outputVector, 0);

  if (inputDS && outputUG)
  {
    double bounds[6];
    this->ComputeBounds(inputDS, bounds);
    return this->RequestDataInternal(inputDS, outputUG, bounds);
  }

  if (inputCD && outputCD)
  {
    outputCD->CopyStructure(inputCD);
    double bounds[6];
    if (this->ComputeBounds(inputCD, bounds))
    {
      vtkSmartPointer<vtkCompositeDataIterator> iter;
      iter.TakeReference(inputCD->NewIterator());
      for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
      {
        vtkDataSet* ds = vtkDataSet::SafeDownCast(iter->GetCurrentDataObject());
        vtkSmartPointer<vtkUnstructuredGrid> ug = vtkSmartPointer<vtkUnstructuredGrid>::New();
        if (!this->RequestDataInternal(ds, ug, bounds))
        {
          return 0;
        }

        outputCD->SetDataSet(iter, ug);
      }
    }
    return 1;
  }

  return 0;
}

//---------------------------------------------------------------------------
int vtkReflectionFilter::RequestDataInternal(
  vtkDataSet* input, vtkUnstructuredGrid* output, double bounds[6])
{
  vtkPointData* inPD = input->GetPointData();
  vtkPointData* outPD = output->GetPointData();
  vtkCellData* inCD = input->GetCellData();
  vtkCellData* outCD = output->GetCellData();
  vtkIdType numPts = input->GetNumberOfPoints();
  vtkIdType numCells = input->GetNumberOfCells();
  double tuple[9];
  double point[3];
  double constant[3] = { 0.0, 0.0, 0.0 };
  int mirrorDir[3] = { 1, 1, 1 };
  int mirrorSymmetricTensorDir[6] = { 1, 1, 1, 1, 1, 1 };
  int mirrorTensorDir[9] = { 1, 1, 1, 1, 1, 1, 1, 1, 1 };
  vtkSmartPointer<vtkIdList> ptIds = vtkSmartPointer<vtkIdList>::New();
  vtkSmartPointer<vtkPoints> outPoints = vtkSmartPointer<vtkPoints>::New();

  if (this->CopyInput)
  {
    outPoints->Allocate(2 * numPts);
    output->Allocate(numCells * 2);
  }
  else
  {
    outPoints->Allocate(numPts);
    output->Allocate(numCells);
  }
  outPD->CopyAllOn();
  outCD->CopyAllOn();
  outPD->CopyAllocate(inPD);
  outCD->CopyAllocate(inCD);

  // Copy first points.
  if (this->CopyInput)
  {
    for (vtkIdType i = 0; i < numPts; i++)
    {
      input->GetPoint(i, point);
      outPD->CopyData(inPD, i, outPoints->InsertNextPoint(point));
    }
  }

  // Compture transformation
  switch (this->Plane)
  {
    case USE_X_MIN:
      constant[0] = 2 * bounds[0];
      break;
    case USE_X_MAX:
      constant[0] = 2 * bounds[1];
      break;
    case USE_X:
      constant[0] = 2 * this->Center;
      break;
    case USE_Y_MIN:
      constant[1] = 2 * bounds[2];
      break;
    case USE_Y_MAX:
      constant[1] = 2 * bounds[3];
      break;
    case USE_Y:
      constant[1] = 2 * this->Center;
      break;
    case USE_Z_MIN:
      constant[2] = 2 * bounds[4];
      break;
    case USE_Z_MAX:
      constant[2] = 2 * bounds[5];
      break;
    case USE_Z:
      constant[2] = 2 * this->Center;
      break;
  }

  // Compute the element-wise multiplication needed for
  // vectors/sym tensors/tensors depending on the flipping axis
  //
  // For vectors it is as following
  // X axis
  // -1  1  1
  // Y axis
  //  1 -1  1
  // Z axis
  //  1  1 -1
  //
  // For symmetric tensor it is as following
  // X axis
  //  1 -1 -1
  //     1  1
  //        1
  // Y axis
  //  1 -1  1
  //     1 -1
  //        1
  // Z axis
  //  1  1 -1
  //     1 -1
  //        1
  //
  // For tensors it is as following :
  // X axis
  //  1 -1 -1
  // -1  1  1
  // -1  1  1
  // Y axis
  //  1 -1  1
  // -1  1 -1
  //  1 -1  1
  // Z axis
  //  1  1 -1
  //  1  1 -1
  // -1 -1  1
  //
  switch (this->Plane)
  {
    case USE_X_MIN:
    case USE_X_MAX:
    case USE_X:
      mirrorDir[0] = -1;
      mirrorSymmetricTensorDir[3] = -1;
      mirrorSymmetricTensorDir[5] = -1;
      break;
    case USE_Y_MIN:
    case USE_Y_MAX:
    case USE_Y:
      mirrorDir[1] = -1;
      mirrorSymmetricTensorDir[3] = -1;
      mirrorSymmetricTensorDir[4] = -1;
      break;
    case USE_Z_MIN:
    case USE_Z_MAX:
    case USE_Z:
      mirrorDir[2] = -1;
      mirrorSymmetricTensorDir[4] = -1;
      mirrorSymmetricTensorDir[5] = -1;
      break;
  }
  vtkMath::TensorFromSymmetricTensor(mirrorSymmetricTensorDir, mirrorTensorDir);

  // Find all flippable arrays
  std::vector<std::pair<vtkIdType, int> > flippableArrays;
  if (this->FlipAllInputArrays)
  {
    FindFlippableArrays(inPD, flippableArrays);
  }
  else
  {
    // Flip only vectors, normals and tensors
    vtkDataArray* vectors = inPD->GetVectors();
    vtkDataArray* normals = inPD->GetNormals();
    vtkDataArray* tensors = inPD->GetTensors();
    for (int iArr = 0; iArr < inPD->GetNumberOfArrays(); iArr++)
    {
      vtkAbstractArray* array = inPD->GetAbstractArray(iArr);
      if (array == vectors || array == normals || array == tensors)
      {
        flippableArrays.push_back(std::make_pair(iArr, array->GetNumberOfComponents()));
      }
    }
  }

  // Copy reflected points.
  for (vtkIdType i = 0; i < numPts; i++)
  {
    input->GetPoint(i, point);
    vtkIdType ptId = outPoints->InsertNextPoint(mirrorDir[0] * point[0] + constant[0],
      mirrorDir[1] * point[1] + constant[1], mirrorDir[2] * point[2] + constant[2]);
    outPD->CopyData(inPD, i, ptId);

    // Flip flippable arrays
    for (size_t iFlip = 0; iFlip < flippableArrays.size(); iFlip++)
    {
      vtkDataArray* inArray =
        vtkDataArray::SafeDownCast(inPD->GetAbstractArray(flippableArrays[iFlip].first));
      vtkDataArray* outArray =
        vtkDataArray::SafeDownCast(outPD->GetAbstractArray(flippableArrays[iFlip].first));
      inArray->GetTuple(i, tuple);
      int nComp = flippableArrays[iFlip].second;
      switch (nComp)
      {
        case 3:
          this->FlipTuple(tuple, mirrorDir, nComp);
          break;
        case 6:
          this->FlipTuple(tuple, mirrorSymmetricTensorDir, nComp);
          break;
        case 9:
          this->FlipTuple(tuple, mirrorTensorDir, nComp);
          break;
        default:
          break;
      }
      outArray->SetTuple(ptId, tuple);
    }
  }

  vtkNew<vtkIdList> cellPts;

  // Copy original cells.
  if (this->CopyInput)
  {
    for (vtkIdType i = 0; i < numCells; i++)
    {
      // special handling for polyhedron cells
      if (vtkUnstructuredGrid::SafeDownCast(input) && input->GetCellType(i) == VTK_POLYHEDRON)
      {
        vtkUnstructuredGrid::SafeDownCast(input)->GetFaceStream(i, ptIds);
        output->InsertNextCell(VTK_POLYHEDRON, ptIds);
      }
      else
      {
        input->GetCellPoints(i, ptIds);
        output->InsertNextCell(input->GetCellType(i), ptIds);
      }
      outCD->CopyData(inCD, i, i);
    }
  }

  // Find all flippable arrays
  flippableArrays.clear();
  if (this->FlipAllInputArrays)
  {
    FindFlippableArrays(inCD, flippableArrays);
  }
  else
  {
    // Flip only vectors, normals and tensors
    vtkDataArray* vectors = inCD->GetVectors();
    vtkDataArray* normals = inCD->GetNormals();
    vtkDataArray* tensors = inCD->GetTensors();
    for (int iArr = 0; iArr < inCD->GetNumberOfArrays(); iArr++)
    {
      vtkAbstractArray* array = inCD->GetAbstractArray(iArr);
      if (array == vectors || array == normals || array == tensors)
      {
        flippableArrays.push_back(std::make_pair(iArr, array->GetNumberOfComponents()));
      }
    }
  }

  // Generate reflected cells.
  for (vtkIdType i = 0; i < numCells; i++)
  {
    vtkIdType outputCellId = -1;
    int cellType = input->GetCellType(i);
    switch (cellType)
    {
      case VTK_TRIANGLE_STRIP:
      {
        input->GetCellPoints(i, cellPts);
        int numCellPts = cellPts->GetNumberOfIds();
        if (numCellPts % 2 != 0)
        {
          this->ReflectNon3DCell(input, output, i, numPts);
        }
        else
        {
          // Triangle strips with even number of triangles have
          // to be handled specially. A degenerate triangle is
          // introduce to flip all the triangles properly.
          input->GetCellPoints(i, cellPts);
          numCellPts++;
          std::vector<vtkIdType> newCellPts(numCellPts);
          newCellPts[0] = cellPts->GetId(0);
          newCellPts[1] = cellPts->GetId(2);
          newCellPts[2] = cellPts->GetId(1);
          newCellPts[3] = cellPts->GetId(2);
          for (int j = 4; j < numCellPts; j++)
          {
            newCellPts[j] = cellPts->GetId(j - 1);
            if (this->CopyInput)
            {
              newCellPts[j] += numPts;
            }
          }
          outputCellId = output->InsertNextCell(cellType, numCellPts, &newCellPts[0]);
        }
        break;
      }
      case VTK_TETRA:
      {
        input->GetCellPoints(i, cellPts);
        vtkIdType newCellPts[4] = { cellPts->GetId(3), cellPts->GetId(1), cellPts->GetId(2),
          cellPts->GetId(0) };
        if (this->CopyInput)
        {
          for (int j = 0; j < 4; j++)
          {
            newCellPts[j] += numPts;
          }
        }
        outputCellId = output->InsertNextCell(cellType, 4, newCellPts);
        break;
      }
      case VTK_HEXAHEDRON:
      {
        input->GetCellPoints(i, cellPts);
        vtkIdType newCellPts[8] = { cellPts->GetId(4), cellPts->GetId(5), cellPts->GetId(6),
          cellPts->GetId(7), cellPts->GetId(0), cellPts->GetId(1), cellPts->GetId(2),
          cellPts->GetId(3) };
        if (this->CopyInput)
        {
          for (int j = 0; j < 8; j++)
          {
            newCellPts[j] += numPts;
          }
        }
        outputCellId = output->InsertNextCell(cellType, 8, newCellPts);
        break;
      }
      case VTK_WEDGE:
      {
        input->GetCellPoints(i, cellPts);
        vtkIdType newCellPts[6] = { cellPts->GetId(3), cellPts->GetId(4), cellPts->GetId(5),
          cellPts->GetId(0), cellPts->GetId(1), cellPts->GetId(2) };
        if (this->CopyInput)
        {
          for (int j = 0; j < 6; j++)
          {
            newCellPts[j] += numPts;
          }
        }
        outputCellId = output->InsertNextCell(cellType, 6, newCellPts);
        break;
      }
      case VTK_PYRAMID:
      {
        input->GetCellPoints(i, cellPts);
        vtkIdType newCellPts[5];
        for (int j = 3; j >= 0; j--)
        {
          newCellPts[3 - j] = cellPts->GetId(j);
          if (this->CopyInput)
          {
            newCellPts[3 - j] += numPts;
          }
        }
        newCellPts[4] = cellPts->GetId(4);
        if (this->CopyInput)
        {
          newCellPts[4] += numPts;
        }
        outputCellId = output->InsertNextCell(cellType, 5, newCellPts);
        break;
      }
      case VTK_PENTAGONAL_PRISM:
      {
        input->GetCellPoints(i, cellPts);
        vtkIdType newCellPts[10] = { cellPts->GetId(5), cellPts->GetId(6), cellPts->GetId(7),
          cellPts->GetId(8), cellPts->GetId(9), cellPts->GetId(0), cellPts->GetId(1),
          cellPts->GetId(2), cellPts->GetId(3), cellPts->GetId(4) };
        if (this->CopyInput)
        {
          for (int j = 0; j < 10; j++)
          {
            newCellPts[j] += numPts;
          }
        }
        outputCellId = output->InsertNextCell(cellType, 10, newCellPts);
        break;
      }
      case VTK_HEXAGONAL_PRISM:
      {
        input->GetCellPoints(i, cellPts);
        vtkIdType newCellPts[12] = { cellPts->GetId(6), cellPts->GetId(7), cellPts->GetId(8),
          cellPts->GetId(9), cellPts->GetId(10), cellPts->GetId(11), cellPts->GetId(0),
          cellPts->GetId(1), cellPts->GetId(2), cellPts->GetId(3), cellPts->GetId(4),
          cellPts->GetId(5) };
        if (this->CopyInput)
        {
          for (int j = 0; j < 12; j++)
          {
            newCellPts[j] += numPts;
          }
        }
        outputCellId = output->InsertNextCell(cellType, 12, newCellPts);
        break;
      }
      case VTK_QUADRATIC_TETRA:
      {
        input->GetCellPoints(i, cellPts);
        vtkIdType newCellPts[10] = { cellPts->GetId(3), cellPts->GetId(1), cellPts->GetId(2),
          cellPts->GetId(0), cellPts->GetId(8), cellPts->GetId(5), cellPts->GetId(9),
          cellPts->GetId(7), cellPts->GetId(4), cellPts->GetId(6) };
        if (this->CopyInput)
        {
          for (int j = 0; j < 10; j++)
          {
            newCellPts[j] += numPts;
          }
        }
        outputCellId = output->InsertNextCell(cellType, 10, newCellPts);
        break;
      }
      case VTK_QUADRATIC_HEXAHEDRON:
      {
        input->GetCellPoints(i, cellPts);
        vtkIdType newCellPts[20] = { cellPts->GetId(4), cellPts->GetId(5), cellPts->GetId(6),
          cellPts->GetId(7), cellPts->GetId(0), cellPts->GetId(1), cellPts->GetId(2),
          cellPts->GetId(3), cellPts->GetId(12), cellPts->GetId(13), cellPts->GetId(14),
          cellPts->GetId(15), cellPts->GetId(8), cellPts->GetId(9), cellPts->GetId(10),
          cellPts->GetId(11), cellPts->GetId(16), cellPts->GetId(17), cellPts->GetId(18),
          cellPts->GetId(19) };
        if (this->CopyInput)
        {
          for (int j = 0; j < 20; j++)
          {
            newCellPts[j] += numPts;
          }
        }
        outputCellId = output->InsertNextCell(cellType, 20, newCellPts);
        break;
      }
      case VTK_QUADRATIC_WEDGE:
      {
        input->GetCellPoints(i, cellPts);
        vtkIdType newCellPts[15] = { cellPts->GetId(3), cellPts->GetId(4), cellPts->GetId(5),
          cellPts->GetId(0), cellPts->GetId(1), cellPts->GetId(2), cellPts->GetId(9),
          cellPts->GetId(10), cellPts->GetId(11), cellPts->GetId(6), cellPts->GetId(7),
          cellPts->GetId(8), cellPts->GetId(12), cellPts->GetId(13), cellPts->GetId(14) };
        if (this->CopyInput)
        {
          for (int j = 0; j < 15; j++)
          {
            newCellPts[j] += numPts;
          }
        }
        outputCellId = output->InsertNextCell(cellType, 15, newCellPts);
        break;
      }
      case VTK_QUADRATIC_PYRAMID:
      {
        input->GetCellPoints(i, cellPts);
        vtkIdType newCellPts[113] = { cellPts->GetId(2), cellPts->GetId(1), cellPts->GetId(0),
          cellPts->GetId(3), cellPts->GetId(4), cellPts->GetId(6), cellPts->GetId(5),
          cellPts->GetId(8), cellPts->GetId(7), cellPts->GetId(11), cellPts->GetId(10),
          cellPts->GetId(9), cellPts->GetId(12) };
        if (this->CopyInput)
        {
          for (int j = 0; j < 13; j++)
          {
            newCellPts[j] += numPts;
          }
        }
        outputCellId = output->InsertNextCell(cellType, 13, newCellPts);
        break;
      }
      case VTK_TRIQUADRATIC_HEXAHEDRON:
      {
        input->GetCellPoints(i, cellPts);
        vtkIdType newCellPts[27] = { cellPts->GetId(4), cellPts->GetId(5), cellPts->GetId(6),
          cellPts->GetId(7), cellPts->GetId(0), cellPts->GetId(1), cellPts->GetId(2),
          cellPts->GetId(3), cellPts->GetId(12), cellPts->GetId(13), cellPts->GetId(14),
          cellPts->GetId(15), cellPts->GetId(8), cellPts->GetId(9), cellPts->GetId(10),
          cellPts->GetId(11), cellPts->GetId(16), cellPts->GetId(17), cellPts->GetId(18),
          cellPts->GetId(19), cellPts->GetId(20), cellPts->GetId(21), cellPts->GetId(22),
          cellPts->GetId(23), cellPts->GetId(25), cellPts->GetId(24), cellPts->GetId(26) };
        if (this->CopyInput)
        {
          for (int j = 0; j < 27; j++)
          {
            newCellPts[j] += numPts;
          }
        }
        outputCellId = output->InsertNextCell(cellType, 27, newCellPts);
        break;
      }
      case VTK_QUADRATIC_LINEAR_WEDGE:
      {
        input->GetCellPoints(i, cellPts);
        vtkIdType newCellPts[12] = { cellPts->GetId(3), cellPts->GetId(4), cellPts->GetId(5),
          cellPts->GetId(0), cellPts->GetId(1), cellPts->GetId(2), cellPts->GetId(9),
          cellPts->GetId(10), cellPts->GetId(11), cellPts->GetId(6), cellPts->GetId(7),
          cellPts->GetId(8) };
        if (this->CopyInput)
        {
          for (int j = 0; j < 12; j++)
          {
            newCellPts[j] += numPts;
          }
        }
        outputCellId = output->InsertNextCell(cellType, 12, newCellPts);
        break;
      }
      case VTK_BIQUADRATIC_QUADRATIC_WEDGE:
      {
        input->GetCellPoints(i, cellPts);
        vtkIdType newCellPts[18] = { cellPts->GetId(3), cellPts->GetId(4), cellPts->GetId(5),
          cellPts->GetId(0), cellPts->GetId(1), cellPts->GetId(2), cellPts->GetId(9),
          cellPts->GetId(10), cellPts->GetId(11), cellPts->GetId(6), cellPts->GetId(7),
          cellPts->GetId(8), cellPts->GetId(12), cellPts->GetId(13), cellPts->GetId(14),
          cellPts->GetId(15), cellPts->GetId(16), cellPts->GetId(17) };
        if (this->CopyInput)
        {
          for (int j = 0; j < 18; j++)
          {
            newCellPts[j] += numPts;
          }
        }
        outputCellId = output->InsertNextCell(cellType, 18, newCellPts);
        break;
      }
      case VTK_BIQUADRATIC_QUADRATIC_HEXAHEDRON:
      {
        input->GetCellPoints(i, cellPts);
        vtkIdType newCellPts[24] = { cellPts->GetId(4), cellPts->GetId(5), cellPts->GetId(6),
          cellPts->GetId(7), cellPts->GetId(0), cellPts->GetId(1), cellPts->GetId(2),
          cellPts->GetId(3), cellPts->GetId(12), cellPts->GetId(13), cellPts->GetId(14),
          cellPts->GetId(15), cellPts->GetId(8), cellPts->GetId(9), cellPts->GetId(10),
          cellPts->GetId(11), cellPts->GetId(16), cellPts->GetId(17), cellPts->GetId(18),
          cellPts->GetId(19), cellPts->GetId(20), cellPts->GetId(21), cellPts->GetId(22),
          cellPts->GetId(23) };
        if (this->CopyInput)
        {
          for (int j = 0; j < 24; j++)
          {
            newCellPts[j] += numPts;
          }
        }
        outputCellId = output->InsertNextCell(cellType, 24, newCellPts);
        break;
      }
      case VTK_POLYHEDRON:
      {
        vtkUnstructuredGrid::SafeDownCast(input)->GetFaceStream(i, cellPts);
        vtkIdType* idPtr = cellPts->GetPointer(0);
        int nfaces = static_cast<int>(*idPtr++);
        for (int j = 0; j < nfaces; j++)
        {
          vtkIdType npts = *idPtr++;
          for (vtkIdType k = 0; k < (npts + 1) / 2; k++)
          {
            vtkIdType temp = idPtr[k];
            idPtr[k] = idPtr[npts - 1 - k];
            idPtr[npts - 1 - k] = temp;
          }
          if (this->CopyInput)
          {
            for (vtkIdType k = 0; k < npts; k++)
            {
              idPtr[k] += numPts;
            }
          }
          idPtr += npts;
        }
        outputCellId = output->InsertNextCell(cellType, cellPts);
        break;
      }
      case VTK_BEZIER_HEXAHEDRON:
      case VTK_LAGRANGE_HEXAHEDRON:
      {
        input->GetCellPoints(i, cellPts);
        const int numCellPts = cellPts->GetNumberOfIds();
        std::vector<vtkIdType> newCellPts(numCellPts);

        vtkCell* cell = input->GetCell(i);
        vtkHigherOrderHexahedron* cellHex = dynamic_cast<vtkHigherOrderHexahedron*>(cell);
        const int* order = cellHex->GetOrder();
        const int k_max_half = (order[2] % 2 == 0) ? (order[2] + 2) / 2 : (order[2] + 1) / 2;
        for (int ii = 0; ii < order[0] + 1; ii++)
        {
          for (int jj = 0; jj < order[1] + 1; jj++)
          {
            for (int kk = 0; kk < k_max_half; kk++)
            {
              const int kk_reversed = order[2] - kk;
              const int node_id = vtkHigherOrderHexahedron::PointIndexFromIJK(ii, jj, kk, order);
              if (kk != kk_reversed)
              {
                const int node_id_reversed =
                  vtkHigherOrderHexahedron::PointIndexFromIJK(ii, jj, kk_reversed, order);
                newCellPts[node_id_reversed] = cellPts->GetId(node_id);
                newCellPts[node_id] = cellPts->GetId(node_id_reversed);
              }
              else
              {
                newCellPts[node_id] = cellPts->GetId(node_id);
              }
            }
          }
        }
        if (this->CopyInput)
        {
          for (int j = 0; j < numCellPts; j++)
          {
            newCellPts[j] += numPts;
          }
        }
        outputCellId = output->InsertNextCell(cellType, numCellPts, &newCellPts[0]);
        break;
      }
      case VTK_BEZIER_WEDGE:
      case VTK_LAGRANGE_WEDGE:
      {
        input->GetCellPoints(i, cellPts);
        const int numCellPts = cellPts->GetNumberOfIds();
        std::vector<vtkIdType> newCellPts(numCellPts);
        if (numCellPts == 21)
        {
          newCellPts = { cellPts->GetId(3), cellPts->GetId(4), cellPts->GetId(5), cellPts->GetId(0),
            cellPts->GetId(1), cellPts->GetId(2), cellPts->GetId(9), cellPts->GetId(10),
            cellPts->GetId(11), cellPts->GetId(6), cellPts->GetId(7), cellPts->GetId(8),
            cellPts->GetId(12), cellPts->GetId(13), cellPts->GetId(14), cellPts->GetId(16),
            cellPts->GetId(15), cellPts->GetId(17), cellPts->GetId(18), cellPts->GetId(19),
            cellPts->GetId(20) };
        }
        else
        {
          vtkCell* cell = input->GetCell(i);
          vtkHigherOrderWedge* cellWedge = dynamic_cast<vtkHigherOrderWedge*>(cell);
          const int* order = cellWedge->GetOrder();
          const int k_max_half = (order[2] % 2 == 0) ? (order[2] + 2) / 2 : (order[2] + 1) / 2;
          for (int ii = 0; ii < order[0] + 1; ii++)
          {
            for (int jj = 0; jj < order[0] + 1 - ii; jj++)
            {
              for (int kk = 0; kk < k_max_half; kk++)
              {
                const int kk_reversed = order[2] - kk;
                const int node_id = vtkHigherOrderWedge::PointIndexFromIJK(ii, jj, kk, order);
                if (kk != kk_reversed)
                {
                  const int node_id_reversed =
                    vtkHigherOrderWedge::PointIndexFromIJK(ii, jj, kk_reversed, order);
                  newCellPts[node_id_reversed] = cellPts->GetId(node_id);
                  newCellPts[node_id] = cellPts->GetId(node_id_reversed);
                }
                else
                {
                  newCellPts[node_id] = cellPts->GetId(node_id);
                }
              }
            }
          }
        }
        if (this->CopyInput)
        {
          for (int j = 0; j < numCellPts; j++)
          {
            newCellPts[j] += numPts;
          }
        }
        outputCellId = output->InsertNextCell(cellType, numCellPts, &newCellPts[0]);
        break;
      }
      case VTK_BEZIER_TETRAHEDRON:
      case VTK_LAGRANGE_TETRAHEDRON:
      {
        input->GetCellPoints(i, cellPts);
        const int numCellPts = cellPts->GetNumberOfIds();
        std::vector<vtkIdType> newCellPts(numCellPts);
        if (numCellPts == 15)
        {
          newCellPts = { cellPts->GetId(0), cellPts->GetId(2), cellPts->GetId(1), cellPts->GetId(3),
            cellPts->GetId(6), cellPts->GetId(5), cellPts->GetId(4), cellPts->GetId(7),
            cellPts->GetId(9), cellPts->GetId(8), cellPts->GetId(10), cellPts->GetId(13),
            cellPts->GetId(12), cellPts->GetId(11), cellPts->GetId(14) };
        }
        else
        {
          const int order = vtkHigherOrderTetra::ComputeOrder(numCellPts);
          for (int ii = 0; ii < order + 1; ii++)
          {
            for (int jj = 0; jj < order + 1 - ii; jj++)
            {
              for (int kk = 0; kk < order + 1 - jj; kk++)
              {
                for (int ll = 0; ll < order + 1 - kk; ll++)
                {
                  if ((ii + jj + kk + ll) == order)
                  {
                    const vtkIdType bindex[4]{ ii, jj, kk, ll };
                    const vtkIdType bindex_reversed[4]{ ii, jj, ll, kk };
                    const vtkIdType node_id = vtkHigherOrderTetra::Index(bindex, order);
                    const vtkIdType node_id_reversed =
                      vtkHigherOrderTetra::Index(bindex_reversed, order);
                    newCellPts[node_id] = cellPts->GetId(node_id_reversed);
                  }
                }
              }
            }
          }
        }
        if (this->CopyInput)
        {
          for (int j = 0; j < numCellPts; j++)
          {
            newCellPts[j] += numPts;
          }
        }
        outputCellId = output->InsertNextCell(cellType, numCellPts, &newCellPts[0]);
        break;
      }
      default:
      {
        outputCellId = this->ReflectNon3DCell(input, output, i, numPts);
      }
    }
    outCD->CopyData(inCD, i, outputCellId);

    // Flip flippable arrays
    for (size_t iFlip = 0; iFlip < flippableArrays.size(); iFlip++)
    {
      vtkDataArray* inArray =
        vtkDataArray::SafeDownCast(inCD->GetAbstractArray(flippableArrays[iFlip].first));
      vtkDataArray* outArray =
        vtkDataArray::SafeDownCast(outCD->GetAbstractArray(flippableArrays[iFlip].first));
      inArray->GetTuple(i, tuple);
      int nComp = flippableArrays[iFlip].second;
      switch (nComp)
      {
        case 3:
          this->FlipTuple(tuple, mirrorDir, nComp);
          break;
        case 6:
          this->FlipTuple(tuple, mirrorSymmetricTensorDir, nComp);
          break;
        case 9:
          this->FlipTuple(tuple, mirrorTensorDir, nComp);
          break;
        default:
          break;
      }
      outArray->SetTuple(outputCellId, tuple);
    }
  }

  output->SetPoints(outPoints);
  output->CheckAttributes();

  return 1;
}

//---------------------------------------------------------------------------
int vtkReflectionFilter::FillInputPortInformation(int, vtkInformation* info)
{
  // Input can be a dataset or a composite of datasets.
  info->Remove(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE());
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkCompositeDataSet");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  return 1;
}

//---------------------------------------------------------------------------
int vtkReflectionFilter::RequestDataObject(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  if (!inInfo)
  {
    return 0;
  }

  vtkDataObject* input = vtkDataObject::GetData(inInfo);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  if (input)
  {
    vtkDataObject* output = vtkDataObject::GetData(outInfo);
    // If input is composite dataset, output is a vtkMultiBlockDataSet of
    // unstructrued grids.
    // If input is a dataset, output is an unstructured grid.
    if (!output || (input->IsA("vtkCompositeDataSet") && !output->IsA("vtkMultiBlockDataSet")) ||
      (input->IsA("vtkDataSet") && !output->IsA("vtkUnstructuredGrid")))
    {
      vtkDataObject* newOutput = nullptr;
      if (input->IsA("vtkCompositeDataSet"))
      {
        newOutput = vtkMultiBlockDataSet::New();
      }
      else // if (input->IsA("vtkDataSet"))
      {
        newOutput = vtkUnstructuredGrid::New();
      }
      outInfo->Set(vtkDataSet::DATA_OBJECT(), newOutput);
      newOutput->FastDelete();
    }
    return 1;
  }

  return 0;
}

//---------------------------------------------------------------------------
void vtkReflectionFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Plane: " << this->Plane << endl;
  os << indent << "Center: " << this->Center << endl;
  os << indent << "CopyInput: " << this->CopyInput << endl;
}
