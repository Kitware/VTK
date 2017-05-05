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
#include "vtkCellData.h"
#include "vtkCellType.h"
#include "vtkCompositeDataIterator.h"
#include "vtkIdList.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkSmartPointer.h"
#include "vtkUnstructuredGrid.h"

vtkStandardNewMacro(vtkReflectionFilter);

//---------------------------------------------------------------------------
vtkReflectionFilter::vtkReflectionFilter()
{
  this->Plane = USE_X_MIN;
  this->Center = 0.0;
  this->CopyInput = 1;
}

//---------------------------------------------------------------------------
vtkReflectionFilter::~vtkReflectionFilter()
{
}

//---------------------------------------------------------------------------
void vtkReflectionFilter::FlipVector(double tuple[3], int mirrorDir[3])
{
  for(int j=0; j<3; j++)
  {
    tuple[j] *= mirrorDir[j];
  }
}

//---------------------------------------------------------------------------
int vtkReflectionFilter::ComputeBounds(vtkDataObject* input, double bounds[6])
{
  // get the input and output
  vtkDataSet *inputDS = vtkDataSet::SafeDownCast(input);;
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
  vtkDataSet* input, vtkUnstructuredGrid* output, vtkIdType cellId,
  vtkIdType numInputPoints)
{
  vtkNew<vtkIdList> cellPts;
  input->GetCellPoints(cellId, cellPts.GetPointer());
  int numCellPts = cellPts->GetNumberOfIds();
  std::vector<vtkIdType> newCellPts(numCellPts);
  for (int j = numCellPts-1; j >= 0; j--)
  {
    newCellPts[numCellPts-1-j] = cellPts->GetId(j);
    if (this->CopyInput)
    {
      newCellPts[numCellPts-1-j] += numInputPoints;
    }
  }
  return output->InsertNextCell(
    input->GetCellType(cellId), numCellPts, &newCellPts[0]);
}

//---------------------------------------------------------------------------
int vtkReflectionFilter::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the input and output
  vtkDataSet *inputDS = vtkDataSet::GetData(inputVector[0], 0);
  vtkUnstructuredGrid *outputUG = vtkUnstructuredGrid::GetData(outputVector, 0);

  vtkCompositeDataSet* inputCD = vtkCompositeDataSet::GetData(inputVector[0], 0);
  vtkCompositeDataSet *outputCD = vtkCompositeDataSet::GetData(outputVector, 0);

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
        vtkSmartPointer<vtkUnstructuredGrid> ug =
          vtkSmartPointer<vtkUnstructuredGrid>::New();
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
  vtkDataSet* input, vtkUnstructuredGrid* output,
  double bounds[6])
{
  vtkPointData *inPD = input->GetPointData();
  vtkPointData *outPD = output->GetPointData();
  vtkCellData *inCD = input->GetCellData();
  vtkCellData *outCD = output->GetCellData();
  vtkIdType numPts = input->GetNumberOfPoints();
  vtkIdType numCells = input->GetNumberOfCells();
  double tuple[3];
  double point[3];
  double constant[3] = {0.0, 0.0, 0.0};
  int mirrorDir[3] = { 1, 1, 1};
  vtkSmartPointer<vtkIdList> ptIds = vtkSmartPointer<vtkIdList>::New();
  vtkSmartPointer<vtkPoints> outPoints = vtkSmartPointer<vtkPoints>::New();

  if (this->CopyInput)
  {
    outPoints->Allocate(2* numPts);
    output->Allocate(numCells * 2);
  }
  else
  {
    outPoints->Allocate(numPts);
    output->Allocate(numCells);
  }
  outPD->CopyAllocate(inPD);
  outCD->CopyAllocate(inCD);

  vtkDataArray *inPtVectors, *outPtVectors, *inPtNormals, *outPtNormals;
  vtkDataArray *inCellVectors, *outCellVectors, *inCellNormals;
  vtkDataArray *outCellNormals;

  inPtVectors = inPD->GetVectors();
  outPtVectors = outPD->GetVectors();
  inPtNormals = inPD->GetNormals();
  outPtNormals = outPD->GetNormals();
  inCellVectors = inCD->GetVectors();
  outCellVectors = outCD->GetVectors();
  inCellNormals = inCD->GetNormals();
  outCellNormals = outCD->GetNormals();

  // Copy first points.
  if (this->CopyInput)
  {
    for (vtkIdType i = 0; i < numPts; i++)
    {
      input->GetPoint(i, point);
      outPD->CopyData(inPD, i, outPoints->InsertNextPoint(point));
    }
  }

  // Copy reflected points.
  switch (this->Plane)
  {
    case USE_X_MIN:
      constant[0] = 2*bounds[0];
      mirrorDir[0] = -1;
      break;
    case USE_X_MAX:
      constant[0] = 2*bounds[1];
      mirrorDir[0] = -1;
      break;
    case USE_X:
      constant[0] = 2*this->Center;
      mirrorDir[0] = -1;
      break;
    case USE_Y_MIN:
      constant[1] = 2*bounds[2];
      mirrorDir[1] = -1;
      break;
    case USE_Y_MAX:
      constant[1] = 2*bounds[3];
      mirrorDir[1] = -1;
      break;
    case USE_Y:
      constant[1] = 2*this->Center;
      mirrorDir[1] = -1;
      break;
    case USE_Z_MIN:
      constant[2] = 2*bounds[4];
      mirrorDir[2] = -1;
      break;
    case USE_Z_MAX:
      constant[2] = 2*bounds[5];
      mirrorDir[2] = -1;
      break;
    case USE_Z:
      constant[2] = 2*this->Center;
      mirrorDir[2] = -1;
      break;
  }

  for (vtkIdType i = 0; i < numPts; i++)
  {
    input->GetPoint(i, point);
    vtkIdType ptId =
      outPoints->InsertNextPoint( mirrorDir[0]*point[0] + constant[0],
                                  mirrorDir[1]*point[1] + constant[1],
                                  mirrorDir[2]*point[2] + constant[2] );
    outPD->CopyData(inPD, i, ptId);
    if (inPtVectors)
    {
      inPtVectors->GetTuple(i, tuple);
      this->FlipVector(tuple, mirrorDir);
      outPtVectors->SetTuple(ptId, tuple);
    }
    if (inPtNormals)
    {
      inPtNormals->GetTuple(i, tuple);
      this->FlipVector(tuple, mirrorDir);
      outPtNormals->SetTuple(ptId, tuple);
    }
  }

  vtkNew<vtkIdList> cellPts;

  // Copy original cells.
  if (this->CopyInput)
  {
    for (vtkIdType i = 0; i < numCells; i++)
    {
      // special handling for polyhedron cells
      if (vtkUnstructuredGrid::SafeDownCast(input) &&
          input->GetCellType(i) == VTK_POLYHEDRON)
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

  // Generate reflected cells.
  for (vtkIdType i = 0; i < numCells; i++)
  {
    vtkIdType outputCellId = -1;
    int cellType = input->GetCellType(i);
    switch (cellType)
    {
    case VTK_TRIANGLE_STRIP:
    {
      input->GetCellPoints(i, cellPts.GetPointer());
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
        input->GetCellPoints(i, cellPts.GetPointer());
        numCellPts++;
        std::vector<vtkIdType> newCellPts(numCellPts);
        newCellPts[0] = cellPts->GetId(0);
        newCellPts[1] = cellPts->GetId(2);
        newCellPts[2] = cellPts->GetId(1);
        newCellPts[3] = cellPts->GetId(2);
        for (int j = 4; j < numCellPts; j++)
        {
          newCellPts[j] = cellPts->GetId(j-1);
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
      input->GetCellPoints(i, cellPts.GetPointer());
      vtkIdType newCellPts[4] = {
        cellPts->GetId(3), cellPts->GetId(1), cellPts->GetId(2), cellPts->GetId(0)};
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
      input->GetCellPoints(i, cellPts.GetPointer());
      vtkIdType newCellPts[8] = {
        cellPts->GetId(4), cellPts->GetId(5), cellPts->GetId(6), cellPts->GetId(7),
        cellPts->GetId(0), cellPts->GetId(1), cellPts->GetId(2), cellPts->GetId(3)};
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
      input->GetCellPoints(i, cellPts.GetPointer());
      vtkIdType newCellPts[6] = {
        cellPts->GetId(3), cellPts->GetId(4), cellPts->GetId(5),
        cellPts->GetId(0), cellPts->GetId(1), cellPts->GetId(2)};
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
    case VTK_PYRAMID:
    {
      input->GetCellPoints(i, cellPts.GetPointer());
      vtkIdType newCellPts[5];
      for (int j = 3; j >= 0; j--)
      {
        newCellPts[3-j] = cellPts->GetId(j);
        if (this->CopyInput)
        {
          newCellPts[3-j] += numPts;
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
      input->GetCellPoints(i, cellPts.GetPointer());
      vtkIdType newCellPts[10] = {
        cellPts->GetId(5), cellPts->GetId(6), cellPts->GetId(7),
        cellPts->GetId(8), cellPts->GetId(9),
        cellPts->GetId(0), cellPts->GetId(1), cellPts->GetId(2),
        cellPts->GetId(3), cellPts->GetId(4)};
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
      input->GetCellPoints(i, cellPts.GetPointer());
      vtkIdType newCellPts[12] = {
        cellPts->GetId(6), cellPts->GetId(7), cellPts->GetId(8),
        cellPts->GetId(9), cellPts->GetId(10), cellPts->GetId(11),
        cellPts->GetId(0), cellPts->GetId(1), cellPts->GetId(2),
        cellPts->GetId(3), cellPts->GetId(4), cellPts->GetId(5)};
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
      input->GetCellPoints(i, cellPts.GetPointer());
      vtkIdType newCellPts[10] = {
        cellPts->GetId(3), cellPts->GetId(1), cellPts->GetId(2), cellPts->GetId(0),
        cellPts->GetId(8), cellPts->GetId(5), cellPts->GetId(9), cellPts->GetId(7),
        cellPts->GetId(4), cellPts->GetId(6)};
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
      input->GetCellPoints(i, cellPts.GetPointer());
      vtkIdType newCellPts[20] = {
        cellPts->GetId(4), cellPts->GetId(5), cellPts->GetId(6), cellPts->GetId(7),
        cellPts->GetId(0), cellPts->GetId(1), cellPts->GetId(2), cellPts->GetId(3),
        cellPts->GetId(12), cellPts->GetId(13), cellPts->GetId(14), cellPts->GetId(15),
        cellPts->GetId(8), cellPts->GetId(9), cellPts->GetId(10), cellPts->GetId(11),
        cellPts->GetId(16), cellPts->GetId(17), cellPts->GetId(18), cellPts->GetId(19)};
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
      input->GetCellPoints(i, cellPts.GetPointer());
      vtkIdType newCellPts[15] = {
        cellPts->GetId(3), cellPts->GetId(4), cellPts->GetId(5),
        cellPts->GetId(0), cellPts->GetId(1), cellPts->GetId(2),
        cellPts->GetId(9), cellPts->GetId(10), cellPts->GetId(11),
        cellPts->GetId(6), cellPts->GetId(7), cellPts->GetId(8),
        cellPts->GetId(12), cellPts->GetId(13), cellPts->GetId(14)};
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
      input->GetCellPoints(i, cellPts.GetPointer());
      vtkIdType newCellPts[113] = {
        cellPts->GetId(2), cellPts->GetId(1), cellPts->GetId(0),
        cellPts->GetId(3), cellPts->GetId(4), cellPts->GetId(6),
        cellPts->GetId(5), cellPts->GetId(8), cellPts->GetId(7),
        cellPts->GetId(11), cellPts->GetId(10), cellPts->GetId(9),
        cellPts->GetId(12)};
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
      input->GetCellPoints(i, cellPts.GetPointer());
      vtkIdType newCellPts[27] = {
        cellPts->GetId(4), cellPts->GetId(5), cellPts->GetId(6), cellPts->GetId(7),
        cellPts->GetId(0), cellPts->GetId(1), cellPts->GetId(2), cellPts->GetId(3),
        cellPts->GetId(12), cellPts->GetId(13), cellPts->GetId(14), cellPts->GetId(15),
        cellPts->GetId(8), cellPts->GetId(9), cellPts->GetId(10), cellPts->GetId(11),
        cellPts->GetId(16), cellPts->GetId(17), cellPts->GetId(18), cellPts->GetId(19),
        cellPts->GetId(20), cellPts->GetId(21), cellPts->GetId(22), cellPts->GetId(23),
        cellPts->GetId(25), cellPts->GetId(24), cellPts->GetId(26)};
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
      input->GetCellPoints(i, cellPts.GetPointer());
      vtkIdType newCellPts[12] = {
        cellPts->GetId(3), cellPts->GetId(4), cellPts->GetId(5),
        cellPts->GetId(0), cellPts->GetId(1), cellPts->GetId(2),
        cellPts->GetId(9), cellPts->GetId(10), cellPts->GetId(11),
        cellPts->GetId(6), cellPts->GetId(7), cellPts->GetId(8)};
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
      input->GetCellPoints(i, cellPts.GetPointer());
      vtkIdType newCellPts[18] = {
        cellPts->GetId(3), cellPts->GetId(4), cellPts->GetId(5),
        cellPts->GetId(0), cellPts->GetId(1), cellPts->GetId(2),
        cellPts->GetId(9), cellPts->GetId(10), cellPts->GetId(11),
        cellPts->GetId(6), cellPts->GetId(7), cellPts->GetId(8),
        cellPts->GetId(12), cellPts->GetId(13), cellPts->GetId(14),
        cellPts->GetId(15), cellPts->GetId(16), cellPts->GetId(17)};
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
      input->GetCellPoints(i, cellPts.GetPointer());
      vtkIdType newCellPts[24] = {
        cellPts->GetId(4), cellPts->GetId(5), cellPts->GetId(6), cellPts->GetId(7),
        cellPts->GetId(0), cellPts->GetId(1), cellPts->GetId(2), cellPts->GetId(3),
        cellPts->GetId(12), cellPts->GetId(13), cellPts->GetId(14), cellPts->GetId(15),
        cellPts->GetId(8), cellPts->GetId(9), cellPts->GetId(10), cellPts->GetId(11),
        cellPts->GetId(16), cellPts->GetId(17), cellPts->GetId(18), cellPts->GetId(19),
        cellPts->GetId(20), cellPts->GetId(21), cellPts->GetId(22), cellPts->GetId(23)};
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
      vtkUnstructuredGrid::SafeDownCast(input)->GetFaceStream(i, cellPts.GetPointer());
      vtkIdType* idPtr = cellPts->GetPointer(0);
      int nfaces = static_cast<int>(*idPtr++);
      for (int j = 0; j < nfaces; j++)
      {
        vtkIdType npts = *idPtr++;
        for (vtkIdType k = 0; k < (npts+1)/2; k++)
        {
          vtkIdType temp = idPtr[k];
          idPtr[k] = idPtr[npts-1-k];
          idPtr[npts-1-k] = temp;
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
      outputCellId = output->InsertNextCell(cellType, cellPts.GetPointer());
      break;
    }
   default:
    {
      if (cellType > VTK_POLYHEDRON)
      {
        vtkWarningMacro("Cell may be inverted");
      }
      outputCellId = this->ReflectNon3DCell(input, output, i, numPts);
    }
    }
    outCD->CopyData(inCD, i, outputCellId);
    if (inCellVectors)
    {
      inCellVectors->GetTuple(i, tuple);
      this->FlipVector(tuple, mirrorDir);
      outCellVectors->SetTuple(outputCellId, tuple);
    }
    if (inCellNormals)
    {
      inCellNormals->GetTuple(i, tuple);
      this->FlipVector(tuple, mirrorDir);
      outCellNormals->SetTuple(outputCellId, tuple);
    }
  }

  output->SetPoints(outPoints);
  output->CheckAttributes();

  return 1;
}

//---------------------------------------------------------------------------
int vtkReflectionFilter::FillInputPortInformation(int, vtkInformation *info)
{
  // Input can be a dataset or a composite of datasets.
  info->Remove(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE());
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkCompositeDataSet");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  return 1;
}

//---------------------------------------------------------------------------
int vtkReflectionFilter::RequestDataObject(
  vtkInformation*,
  vtkInformationVector** inputVector ,
  vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  if (!inInfo)
  {
    return 0;
  }

  vtkDataObject *input = vtkDataObject::GetData(inInfo);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  if (input)
  {
    vtkDataObject *output = vtkDataObject::GetData(outInfo);
    // If input is composite dataset, output is a vtkMultiBlockDataSet of
    // unstructrued grids.
    // If input is a dataset, output is an unstructured grid.
    if (!output ||
      (input->IsA("vtkCompositeDataSet") && !output->IsA("vtkMultiBlockDataSet")) ||
      (input->IsA("vtkDataSet") && !output->IsA("vtkUnstructuredGrid")))
    {
      vtkDataObject* newOutput = 0;
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
void vtkReflectionFilter::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Plane: " << this->Plane << endl;
  os << indent << "Center: " << this->Center << endl;
  os << indent << "CopyInput: " << this->CopyInput << endl;
}
