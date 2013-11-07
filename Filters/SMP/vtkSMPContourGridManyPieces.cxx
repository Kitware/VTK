/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkContourGridManyPieces.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMPContourGridManyPieces.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkGenericCell.h"
#include "vtkNew.h"
#include "vtkNonMergingPointLocator.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkSmartPointer.h"
#include "vtkUnstructuredGrid.h"
#include "vtkMergePoints.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiPieceDataSet.h"
#include "vtkSMPTools.h"
#include "vtkSMPThreadLocal.h"
#include "vtkSMPThreadLocalObject.h"
#include "vtkInformation.h"
#include "vtkSMPMergePoints.h"
#include "vtkSMPMergePolyDataHelper.h"

#include "vtkTimerLog.h"

#include <math.h>

vtkStandardNewMacro(vtkSMPContourGridManyPieces);

// Construct object with initial range (0,1) and single contour value
// of 0.0.
vtkSMPContourGridManyPieces::vtkSMPContourGridManyPieces()
{
}

vtkSMPContourGridManyPieces::~vtkSMPContourGridManyPieces()
{
}

namespace
{
// This functor creates a new vtkPolyData piece each time it runs.
// This is less efficient that the previous version but can be used
// to generate more piece to exploit coarse-grained parallelism
// downstream.
template <typename T>
class vtkContourGridManyPiecesFunctor
{
  vtkSMPContourGridManyPieces* Filter;

  vtkUnstructuredGrid* Input;
  vtkDataArray* InScalars;

  vtkMultiBlockDataSet* Output;

  int NumValues;
  double* Values;

  vtkSMPThreadLocal<std::vector<vtkPolyData*> > Outputs;

public:

  vtkContourGridManyPiecesFunctor(vtkSMPContourGridManyPieces* filter,
                         vtkUnstructuredGrid* input,
                         vtkDataArray* inScalars,
                         int numValues,
                         double* values,
                         vtkMultiBlockDataSet* output) : Filter(filter),
                                                         Input(input),
                                                         InScalars(inScalars),
                                                         Output(output),
                                                         NumValues(numValues),
                                                         Values(values)
  {
  }

  ~vtkContourGridManyPiecesFunctor()
  {
  }

  void Initialize()
  {
  }


  void operator()(vtkIdType begin, vtkIdType end)
  {
    vtkNew<vtkPolyData> output;

    vtkNew<vtkPoints> newPts;

    // set precision for the points in the output
    if(this->Filter->GetOutputPointsPrecision() == vtkAlgorithm::DEFAULT_PRECISION)
      {
      newPts->SetDataType(this->Input->GetPoints()->GetDataType());
      }
    else if(this->Filter->GetOutputPointsPrecision() == vtkAlgorithm::SINGLE_PRECISION)
      {
      newPts->SetDataType(VTK_FLOAT);
      }
    else if(this->Filter->GetOutputPointsPrecision() == vtkAlgorithm::DOUBLE_PRECISION)
      {
      newPts->SetDataType(VTK_DOUBLE);
      }

    output->SetPoints(newPts.GetPointer());

    vtkIdType numCells = this->Input->GetNumberOfCells();

    vtkIdType estimatedSize=static_cast<vtkIdType>(
      pow(static_cast<double>(numCells),.75));
    estimatedSize = estimatedSize / 1024 * 1024; //multiple of 1024
    if (estimatedSize < 1024)
      {
      estimatedSize = 1024;
      }

    newPts->Allocate(estimatedSize, estimatedSize);

    // vtkNew<vtkNonMergingPointLocator> locator;
    // locator->SetPoints(newPts.GetPointer());

    vtkNew<vtkMergePoints> locator;
    locator->InitPointInsertion (newPts.GetPointer(),
                                 this->Input->GetBounds(),
                                 this->Input->GetNumberOfPoints());

    // vtkNew<vtkPointLocator> locator;
    // locator->InitPointInsertion (newPts.GetPointer(),
    //                              this->Input->GetBounds(),
    //                              this->Input->GetNumberOfPoints());

    vtkNew<vtkCellArray> newVerts;
    newVerts->Allocate(estimatedSize,estimatedSize);

    vtkNew<vtkCellArray> newLines;
    newLines->Allocate(estimatedSize,estimatedSize);

    vtkNew<vtkCellArray> newPolys;
    newPolys->Allocate(estimatedSize,estimatedSize);

    vtkSmartPointer<vtkDataArray> cellScalars;
    cellScalars.TakeReference(this->InScalars->NewInstance());
    cellScalars->SetNumberOfComponents(this->InScalars->GetNumberOfComponents());
    cellScalars->Allocate(VTK_CELL_SIZE*this->InScalars->GetNumberOfComponents());

    vtkPointData* outPd = output->GetPointData();
    vtkCellData* outCd = output->GetCellData();
    vtkPointData* inPd = this->Input->GetPointData();
    vtkCellData* inCd = this->Input->GetCellData();
    outPd->InterpolateAllocate(inPd, estimatedSize, estimatedSize);
    outCd->CopyAllocate(inCd, estimatedSize, estimatedSize);

    vtkNew<vtkGenericCell> cell;

    const double* values = this->Values;
    int numValues = this->NumValues;

    vtkNew<vtkIdList> pids;
    T range[2];

    for (vtkIdType cellid=begin; cellid<end; cellid++)
      {
      this->Input->GetCellPoints(cellid, pids.GetPointer());
      cellScalars->SetNumberOfTuples(pids->GetNumberOfIds());
      this->InScalars->GetTuples(pids.GetPointer(), cellScalars);
      int numCellScalars = cellScalars->GetNumberOfComponents()
        * cellScalars->GetNumberOfTuples();
      T* cellScalarPtr = static_cast<T*>(cellScalars->GetVoidPointer(0));

      //find min and max values in scalar data
      range[0] = range[1] = cellScalarPtr[0];

      for (T *it = cellScalarPtr + 1, *itEnd = cellScalarPtr + numCellScalars;
           it != itEnd;
           ++it)
        {
        if (*it <= range[0])
          {
          range[0] = *it;
          } //if scalar <= min range value
        if (*it >= range[1])
          {
          range[1] = *it;
          } //if scalar >= max range value
        } // for all cellScalars

      bool needCell = false;
      for (int i = 0; i < numValues; i++)
        {
        if ((values[i] >= range[0]) && (values[i] <= range[1]))
            {
            needCell = true;
            } // if contour value in range for this cell
          } // end for numContours

      if (needCell)
          {
          this->Input->GetCell(cellid, cell.GetPointer());

          for (int i=0; i < numValues; i++)
            {
            if ((values[i] >= range[0]) && (values[i] <= range[1]))
              {
              cell->Contour(values[i],
                            cellScalars,
                            locator.GetPointer(),
                            newVerts.GetPointer(),
                            newLines.GetPointer(),
                            newPolys.GetPointer(),
                            inPd,
                            outPd,
                            inCd,
                            cellid,
                            outCd);
              }
            }
          }
      }

    if (newVerts->GetNumberOfCells())
      {
      output->SetVerts(newVerts.GetPointer());
      }

    if (newLines->GetNumberOfCells())
      {
      output->SetLines(newLines.GetPointer());
      }

    if (newPolys->GetNumberOfCells())
      {
      output->SetPolys(newPolys.GetPointer());
      }

    output->Squeeze();

    output->Register(0);
    this->Outputs.Local().push_back(output.GetPointer());
  }

  void Reduce()
  {
    vtkNew<vtkMultiPieceDataSet> mp;
    int count = 0;

    vtkSMPThreadLocal<std::vector<vtkPolyData*> >::iterator outIter =
      this->Outputs.begin();
    while(outIter != this->Outputs.end())
      {
      std::vector<vtkPolyData*>& outs = *outIter;
      std::vector<vtkPolyData*>::iterator iter = outs.begin();
      while (iter != outs.end())
        {
        mp->SetPiece(count++, *iter);
        (*iter)->Delete();
        iter++;
        }
      ++outIter;
      }

    this->Output->SetBlock(0, mp.GetPointer());
  }
};

}

int vtkSMPContourGridManyPieces::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the input and output
  vtkUnstructuredGrid *input = vtkUnstructuredGrid::GetData(inputVector[0]);
  vtkMultiBlockDataSet *output = vtkMultiBlockDataSet::GetData(outputVector);

  if (input->GetNumberOfCells() == 0)
    {
    return 1;
    }

  vtkDataArray* inScalars = this->GetInputArrayToProcess(0,inputVector);
  if (!inScalars)
    {
    return 1;
    }

  // Not thread safe so calculate first.
  input->GetBounds();

  int numContours = this->GetNumberOfContours();
  if (numContours < 1)
    {
    return 1;
    }

  double *values=this->GetValues();

  vtkIdType numCells = input->GetNumberOfCells();

  // When using vtkContourGridManyPiecesFunctor, it is crucial to set the grain
  // right. When the grain is too small, which tends to be the default,
  // the overhead of allocating data structures, building locators etc.
  // ends up being too big.
  if (inScalars->GetDataType() == VTK_FLOAT)
    {
    vtkContourGridManyPiecesFunctor<float> functor(this, input, inScalars, numContours, values, output);
    vtkIdType grain = numCells > 100000 ? numCells / 100 : numCells;
    vtkSMPTools::For(0, numCells, grain, functor);
    }
  else if(inScalars->GetDataType() == VTK_DOUBLE)
    {
    vtkContourGridManyPiecesFunctor<double> functor(this, input, inScalars, numContours, values, output);
    vtkIdType grain = numCells > 100000 ? numCells / 100 : numCells;
    vtkSMPTools::For(0, numCells, grain, functor);
    }

  return 1;
}

int vtkSMPContourGridManyPieces::FillOutputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  // now add our info
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkMultiBlockDataSet");
  return 1;
}

void vtkSMPContourGridManyPieces::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
