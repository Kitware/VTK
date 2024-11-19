// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkReflectionUtilities.h"

#include "vtkCellData.h"
#include "vtkCellType.h"
#include "vtkDataArray.h"
#include "vtkDataSet.h"
#include "vtkDataSetAttributes.h"
#include "vtkFieldData.h"
#include "vtkHigherOrderHexahedron.h"
#include "vtkHigherOrderQuadrilateral.h"
#include "vtkHigherOrderTetra.h"
#include "vtkHigherOrderWedge.h"
#include "vtkPointData.h"

#include <cmath>

VTK_ABI_NAMESPACE_BEGIN

//------------------------------------------------------------------------------
void vtkReflectionUtilities::FindReflectableArrays(
  vtkFieldData* fd, std::vector<std::pair<vtkIdType, int>>& reflectableArrays)
{
  // Find all reflectable arrays
  for (int iArr = 0; iArr < fd->GetNumberOfArrays(); iArr++)
  {
    vtkDataArray* array = vtkDataArray::SafeDownCast(fd->GetAbstractArray(iArr));
    if (!array)
    {
      continue;
    }

    // Only signed arrays are reflectable
    int dataType = array->GetDataType();
    if ((dataType == VTK_CHAR && VTK_TYPE_CHAR_IS_SIGNED) || dataType == VTK_SIGNED_CHAR ||
      dataType == VTK_SHORT || dataType == VTK_INT || dataType == VTK_LONG ||
      dataType == VTK_FLOAT || dataType == VTK_DOUBLE || dataType == VTK_ID_TYPE)
    {
      // Only vectors and tensors are reflectable
      int nComp = array->GetNumberOfComponents();
      if (nComp == 3 || nComp == 6 || nComp == 9)
      {
        reflectableArrays.emplace_back(iArr, nComp);
      }
    }
  }
}

//------------------------------------------------------------------------------
void vtkReflectionUtilities::ReflectTuple(double* tuple, int* mirrorDir, int nComp)
{
  for (int j = 0; j < nComp; j++)
  {
    tuple[j] *= mirrorDir[j];
  }
}

//------------------------------------------------------------------------------
void vtkReflectionUtilities::ReflectReflectableArrays(
  std::vector<std::pair<vtkIdType, int>>& reflectableArrays, vtkDataSetAttributes* inData,
  vtkDataSetAttributes* outData, vtkIdType i, int mirrorDir[3], int mirrorSymmetricTensorDir[6],
  int mirrorTensorDir[9], vtkIdType id)
{
  // Reflect reflectable arrays
  for (size_t iReflect = 0; iReflect < reflectableArrays.size(); iReflect++)
  {
    vtkDataArray* inArray =
      vtkDataArray::SafeDownCast(inData->GetAbstractArray(reflectableArrays[iReflect].first));
    vtkDataArray* outArray =
      vtkDataArray::SafeDownCast(outData->GetAbstractArray(reflectableArrays[iReflect].first));

    double tuple[9];
    inArray->GetTuple(i, tuple);
    int nComp = reflectableArrays[iReflect].second;
    switch (nComp)
    {
      case 3:
        vtkReflectionUtilities::ReflectTuple(tuple, mirrorDir, nComp);
        break;
      case 6:
        vtkReflectionUtilities::ReflectTuple(tuple, mirrorSymmetricTensorDir, nComp);
        break;
      case 9:
        vtkReflectionUtilities::ReflectTuple(tuple, mirrorTensorDir, nComp);
        break;
      default:
        break;
    }
    outArray->SetTuple(id, tuple);
  }
}

//------------------------------------------------------------------------------
void vtkReflectionUtilities::FindAllReflectableArrays(
  std::vector<std::pair<vtkIdType, int>>& reflectableArrays, vtkDataSetAttributes* inData,
  bool reflectAllInputArrays)
{
  if (reflectAllInputArrays)
  {
    vtkReflectionUtilities::FindReflectableArrays(inData, reflectableArrays);
  }
  else
  {
    // Reflect only vectors, normals and tensors
    vtkDataArray* vectors = inData->GetVectors();
    vtkDataArray* normals = inData->GetNormals();
    vtkDataArray* tensors = inData->GetTensors();
    for (int iArr = 0; iArr < inData->GetNumberOfArrays(); iArr++)
    {
      vtkAbstractArray* array = inData->GetAbstractArray(iArr);
      if (array == vectors || array == normals || array == tensors)
      {
        reflectableArrays.emplace_back(iArr, array->GetNumberOfComponents());
      }
    }
  }
}

//------------------------------------------------------------------------------
vtkIdType vtkReflectionUtilities::ReflectNon3DCellInternal(vtkDataSet* input,
  vtkUnstructuredGrid* output, vtkIdType cellId, vtkIdType numInputPoints, bool copyInput)
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
    case VTK_PIXEL:
    {
      newCellPts[0] = cellPts->GetId(0);
      newCellPts[2] = cellPts->GetId(1);
      newCellPts[1] = cellPts->GetId(2);
      newCellPts[3] = cellPts->GetId(3);
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
          const int contourN = 3 * (order - 1);
          for (int contour = 0; contour < contourN; contour++)
          {
            newCellPts[offset + 3 + contour] = cellPts->GetId(offset + 3 + contourN - 1 - contour);
          }
          if (order == 3) // This is it, there is a single point in the middle
          {
            newCellPts[offset + 3 + contourN] = cellPts->GetId(offset + 3 + contourN);
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
      const int iMaxHalf = (order[0] % 2 == 0) ? (order[0] + 2) / 2 : (order[0] + 1) / 2;
      for (int i = 0; i < iMaxHalf; i++)
      {
        const int iReversed = order[0] - i;
        for (int j = 0; j < order[1] + 1; j++)
        {
          const int nodeId = vtkHigherOrderQuadrilateral::PointIndexFromIJK(i, j, order);
          if (i != iReversed)
          {
            const int nodeIdReversed =
              vtkHigherOrderQuadrilateral::PointIndexFromIJK(iReversed, j, order);
            newCellPts[nodeIdReversed] = cellPts->GetId(nodeId);
            newCellPts[nodeId] = cellPts->GetId(nodeIdReversed);
          }
          else
          {
            newCellPts[nodeId] = cellPts->GetId(nodeId);
          }
        }
      }
      break;
    }
    default:
    {
      if (input->GetCell(cellId)->IsA("vtkNonLinearCell") || cellType > VTK_POLYHEDRON)
      {
        vtkGenericWarningMacro("Cell may be inverted");
      }
      for (int j = 0; j != numCellPts; j++)
      {
        // Indexing in this way ensures proper reflection of quad triangulation
        newCellPts[(numCellPts - j) % numCellPts] = cellPts->GetId(j);
      }
    }
  } // end switch
  if (copyInput)
  {
    for (int j = 0; j < numCellPts; j++)
    {
      newCellPts[j] += numInputPoints;
    }
  }
  return output->InsertNextCell(cellType, numCellPts, newCellPts.data());
}

//------------------------------------------------------------------------------
void vtkReflectionUtilities::ProcessUnstructuredGrid(vtkDataSet* input, vtkUnstructuredGrid* output,
  double constant[3], int mirrorDir[3], int mirrorSymmetricTensorDir[6], int mirrorTensorDir[9],
  bool copyInput, bool reflectAllInputArrays, vtkAlgorithm* algorithm)
{
  vtkPointData* inPD = input->GetPointData();
  vtkPointData* outPD = output->GetPointData();
  vtkCellData* inCD = input->GetCellData();
  vtkCellData* outCD = output->GetCellData();
  vtkIdType numPts = input->GetNumberOfPoints();
  vtkIdType numCells = input->GetNumberOfCells();
  double point[3];
  vtkSmartPointer<vtkIdList> ptIds = vtkSmartPointer<vtkIdList>::New();
  vtkSmartPointer<vtkPoints> outPoints = vtkSmartPointer<vtkPoints>::New();

  if (copyInput)
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
  if (copyInput)
  {
    for (vtkIdType i = 0; i < numPts; i++)
    {
      input->GetPoint(i, point);
      outPD->CopyData(inPD, i, outPoints->InsertNextPoint(point));
    }
  }

  std::vector<std::pair<vtkIdType, int>> reflectableArrays;
  vtkReflectionUtilities::FindAllReflectableArrays(reflectableArrays, inPD, reflectAllInputArrays);

  // Copy reflected points.
  for (vtkIdType i = 0; i < numPts; i++)
  {
    if (algorithm->CheckAbort())
    {
      break;
    }
    input->GetPoint(i, point);
    vtkIdType ptId = outPoints->InsertNextPoint(mirrorDir[0] * point[0] + constant[0],
      mirrorDir[1] * point[1] + constant[1], mirrorDir[2] * point[2] + constant[2]);
    outPD->CopyData(inPD, i, ptId);

    vtkReflectionUtilities::ReflectReflectableArrays(reflectableArrays, inPD, outPD, i, mirrorDir,
      mirrorSymmetricTensorDir, mirrorTensorDir, ptId);
  }

  vtkNew<vtkIdList> cellPts;

  // Copy original cells.
  if (copyInput)
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

  // Find all reflectable arrays
  reflectableArrays.clear();
  vtkReflectionUtilities::FindAllReflectableArrays(reflectableArrays, inCD, reflectAllInputArrays);

  // Generate reflected cells.
  for (vtkIdType i = 0; i < numCells; i++)
  {
    if (algorithm->CheckAbort())
    {
      break;
    }
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
          vtkReflectionUtilities::ReflectNon3DCellInternal(input, output, i, numPts, copyInput);
        }
        else
        {
          // Triangle strips with even number of triangles have
          // to be handled specially. A degenerate triangle is
          // introduced to reflect all the triangles properly.
          input->GetCellPoints(i, cellPts);
          numCellPts++;
          std::vector<vtkIdType> newCellPts(numCellPts);
          vtkIdType pointIdOffset = 0;
          if (copyInput)
          {
            pointIdOffset = numPts;
          }
          newCellPts[0] = cellPts->GetId(0) + pointIdOffset;
          newCellPts[1] = cellPts->GetId(2) + pointIdOffset;
          newCellPts[2] = cellPts->GetId(1) + pointIdOffset;
          newCellPts[3] = cellPts->GetId(2) + pointIdOffset;
          for (int j = 4; j < numCellPts; j++)
          {
            newCellPts[j] = cellPts->GetId(j - 1) + pointIdOffset;
          }
          outputCellId = output->InsertNextCell(cellType, numCellPts, newCellPts.data());
        }
        break;
      }
      case VTK_TETRA:
      {
        input->GetCellPoints(i, cellPts);
        vtkIdType newCellPts[4] = { cellPts->GetId(3), cellPts->GetId(1), cellPts->GetId(2),
          cellPts->GetId(0) };
        if (copyInput)
        {
          for (int j = 0; j < 4; j++)
          {
            newCellPts[j] += numPts;
          }
        }
        outputCellId = output->InsertNextCell(cellType, 4, newCellPts);
        break;
      }
      case VTK_VOXEL:
      case VTK_HEXAHEDRON:
      {
        input->GetCellPoints(i, cellPts);
        vtkIdType newCellPts[8] = { cellPts->GetId(4), cellPts->GetId(5), cellPts->GetId(6),
          cellPts->GetId(7), cellPts->GetId(0), cellPts->GetId(1), cellPts->GetId(2),
          cellPts->GetId(3) };
        if (copyInput)
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
        if (copyInput)
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
          if (copyInput)
          {
            newCellPts[3 - j] += numPts;
          }
        }
        newCellPts[4] = cellPts->GetId(4);
        if (copyInput)
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
        if (copyInput)
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
        if (copyInput)
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
        if (copyInput)
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
        if (copyInput)
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
        if (copyInput)
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
        vtkIdType newCellPts[13] = { cellPts->GetId(2), cellPts->GetId(1), cellPts->GetId(0),
          cellPts->GetId(3), cellPts->GetId(4), cellPts->GetId(6), cellPts->GetId(5),
          cellPts->GetId(8), cellPts->GetId(7), cellPts->GetId(11), cellPts->GetId(10),
          cellPts->GetId(9), cellPts->GetId(12) };
        if (copyInput)
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
        if (copyInput)
        {
          for (int j = 0; j < 27; j++)
          {
            newCellPts[j] += numPts;
          }
        }
        outputCellId = output->InsertNextCell(cellType, 27, newCellPts);
        break;
      }
      case VTK_TRIQUADRATIC_PYRAMID:
      {
        input->GetCellPoints(i, cellPts);
        vtkIdType newCellPts[19] = { cellPts->GetId(2), cellPts->GetId(1), cellPts->GetId(0),
          cellPts->GetId(3), cellPts->GetId(4), cellPts->GetId(6), cellPts->GetId(5),
          cellPts->GetId(8), cellPts->GetId(7), cellPts->GetId(11), cellPts->GetId(10),
          cellPts->GetId(9), cellPts->GetId(12), cellPts->GetId(13), cellPts->GetId(15),
          cellPts->GetId(14), cellPts->GetId(17), cellPts->GetId(16), cellPts->GetId(18) };
        if (copyInput)
        {
          for (int j = 0; j < 19; j++)
          {
            newCellPts[j] += numPts;
          }
        }
        outputCellId = output->InsertNextCell(cellType, 19, newCellPts);
        break;
      }
      case VTK_QUADRATIC_LINEAR_WEDGE:
      {
        input->GetCellPoints(i, cellPts);
        vtkIdType newCellPts[12] = { cellPts->GetId(3), cellPts->GetId(4), cellPts->GetId(5),
          cellPts->GetId(0), cellPts->GetId(1), cellPts->GetId(2), cellPts->GetId(9),
          cellPts->GetId(10), cellPts->GetId(11), cellPts->GetId(6), cellPts->GetId(7),
          cellPts->GetId(8) };
        if (copyInput)
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
        if (copyInput)
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
        if (copyInput)
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
          if (copyInput)
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
        const int kMaxHalf = (order[2] % 2 == 0) ? (order[2] + 2) / 2 : (order[2] + 1) / 2;
        for (int ii = 0; ii < order[0] + 1; ii++)
        {
          for (int jj = 0; jj < order[1] + 1; jj++)
          {
            for (int kk = 0; kk < kMaxHalf; kk++)
            {
              const int kkReversed = order[2] - kk;
              const int nodeId = vtkHigherOrderHexahedron::PointIndexFromIJK(ii, jj, kk, order);
              if (kk != kkReversed)
              {
                const int nodeIdReversed =
                  vtkHigherOrderHexahedron::PointIndexFromIJK(ii, jj, kkReversed, order);
                newCellPts[nodeIdReversed] = cellPts->GetId(nodeId);
                newCellPts[nodeId] = cellPts->GetId(nodeIdReversed);
              }
              else
              {
                newCellPts[nodeId] = cellPts->GetId(nodeId);
              }
            }
          }
        }
        if (copyInput)
        {
          for (int j = 0; j < numCellPts; j++)
          {
            newCellPts[j] += numPts;
          }
        }
        outputCellId = output->InsertNextCell(cellType, numCellPts, newCellPts.data());
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
          const int kMaxHalf = (order[2] % 2 == 0) ? (order[2] + 2) / 2 : (order[2] + 1) / 2;
          for (int ii = 0; ii < order[0] + 1; ii++)
          {
            for (int jj = 0; jj < order[0] + 1 - ii; jj++)
            {
              for (int kk = 0; kk < kMaxHalf; kk++)
              {
                const int kkReversed = order[2] - kk;
                const int nodeId = vtkHigherOrderWedge::PointIndexFromIJK(ii, jj, kk, order);
                if (kk != kkReversed)
                {
                  const int nodeIdReversed =
                    vtkHigherOrderWedge::PointIndexFromIJK(ii, jj, kkReversed, order);
                  newCellPts[nodeIdReversed] = cellPts->GetId(nodeId);
                  newCellPts[nodeId] = cellPts->GetId(nodeIdReversed);
                }
                else
                {
                  newCellPts[nodeId] = cellPts->GetId(nodeId);
                }
              }
            }
          }
        }
        if (copyInput)
        {
          for (int j = 0; j < numCellPts; j++)
          {
            newCellPts[j] += numPts;
          }
        }
        outputCellId = output->InsertNextCell(cellType, numCellPts, newCellPts.data());
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
                    const vtkIdType bindexReversed[4]{ ii, jj, ll, kk };
                    const vtkIdType nodeId = vtkHigherOrderTetra::Index(bindex, order);
                    const vtkIdType nodeIdReversed =
                      vtkHigherOrderTetra::Index(bindexReversed, order);
                    newCellPts[nodeId] = cellPts->GetId(nodeIdReversed);
                  }
                }
              }
            }
          }
        }
        if (copyInput)
        {
          for (int j = 0; j < numCellPts; j++)
          {
            newCellPts[j] += numPts;
          }
        }
        outputCellId = output->InsertNextCell(cellType, numCellPts, newCellPts.data());
        break;
      }
      default:
      {
        outputCellId =
          vtkReflectionUtilities::ReflectNon3DCellInternal(input, output, i, numPts, copyInput);
      }
    }
    outCD->CopyData(inCD, i, outputCellId);

    vtkReflectionUtilities::ReflectReflectableArrays(reflectableArrays, inCD, outCD, i, mirrorDir,
      mirrorSymmetricTensorDir, mirrorTensorDir, outputCellId);
  }

  output->SetPoints(outPoints);
  output->CheckAttributes();
}

VTK_ABI_NAMESPACE_END
