/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkContourGrid.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMPContourGrid.h"

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
#include "vtkInformationVector.h"
#include "vtkDemandDrivenPipeline.h"

#include "vtkTimerLog.h"

#include <math.h>

vtkStandardNewMacro(vtkSMPContourGrid);

// Construct object with initial range (0,1) and single contour value
// of 0.0.
vtkSMPContourGrid::vtkSMPContourGrid()
{
  this->MergePieces = true;
}

vtkSMPContourGrid::~vtkSMPContourGrid()
{
}

namespace
{

struct vtkLocalDataType
{
  vtkPolyData* Output;
  vtkSMPMergePoints* Locator;
  vtkIdList* VertOffsets;
  vtkIdList* LineOffsets;
  vtkIdList* PolyOffsets;

  vtkLocalDataType() : Output(0)
    {
    }
};

// This functor uses thread local storage to create one vtkPolyData per
// thread. Each execution of the functor adds to the vtkPolyData that is
// local to the thread it is running on.
template <typename T>
class vtkContourGridFunctor
{
public:
  vtkSMPContourGrid* Filter;

  vtkUnstructuredGrid* Input;
  vtkDataArray* InScalars;

  vtkDataObject* Output;

  vtkSMPThreadLocal<vtkDataArray*> CellScalars;

  vtkSMPThreadLocalObject<vtkGenericCell> Cell;
  vtkSMPThreadLocalObject<vtkPoints> NewPts;
  vtkSMPThreadLocalObject<vtkCellArray> NewVerts;
  vtkSMPThreadLocalObject<vtkCellArray> NewLines;
  vtkSMPThreadLocalObject<vtkCellArray> NewPolys;

  vtkSMPThreadLocal<vtkLocalDataType> LocalData;

  int NumValues;
  double* Values;

  vtkContourGridFunctor(vtkSMPContourGrid* filter,
                        vtkUnstructuredGrid* input,
                        vtkDataArray* inScalars,
                        int numValues,
                        double* values,
                        vtkDataObject* output) : Filter(filter),
                                                 Input(input),
                                                 InScalars(inScalars),
                                                 Output(output),
                                                 NumValues(numValues),
                                                 Values(values)
  {
  }

  virtual ~vtkContourGridFunctor()
  {
    // Cleanup all temporaries

    vtkSMPThreadLocal<vtkDataArray*>::iterator cellScalarsIter =
      this->CellScalars.begin();
    while(cellScalarsIter != this->CellScalars.end())
      {
      (*cellScalarsIter)->Delete();
      ++cellScalarsIter;
      }

    vtkSMPThreadLocal<vtkLocalDataType>::iterator dataIter =
      this->LocalData.begin();
    while(dataIter != this->LocalData.end())
      {
      (*dataIter).Output->Delete();
      (*dataIter).Locator->Delete();
      (*dataIter).VertOffsets->Delete();
      (*dataIter).LineOffsets->Delete();
      (*dataIter).PolyOffsets->Delete();
      ++dataIter;
      }
  }

  void Initialize()
  {
    // Initialize thread local object before any processing happens.
    // This gets called once per thread.

    vtkPointLocator* locator;
    vtkPolyData* output;
    vtkIdList* vertOffsets;
    vtkIdList* lineOffsets;
    vtkIdList* polyOffsets;

    vtkLocalDataType& localData = this->LocalData.Local();

    localData.Output = vtkPolyData::New();
    output = localData.Output;

    localData.Locator = vtkSMPMergePoints::New();
    locator = localData.Locator;

    localData.VertOffsets = vtkIdList::New();
    vertOffsets = localData.VertOffsets;

    localData.LineOffsets = vtkIdList::New();
    lineOffsets = localData.LineOffsets;

    localData.PolyOffsets = vtkIdList::New();
    polyOffsets = localData.PolyOffsets;

    vtkPoints*& newPts = this->NewPts.Local();

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

    output->SetPoints(newPts);

    vtkIdType numCells = this->Input->GetNumberOfCells();

    vtkIdType estimatedSize=static_cast<vtkIdType>(
      pow(static_cast<double>(numCells),.75));
    estimatedSize = estimatedSize / 1024 * 1024; //multiple of 1024
    if (estimatedSize < 1024)
      {
      estimatedSize = 1024;
      }

    newPts->Allocate(estimatedSize, estimatedSize);

    vertOffsets->Allocate(estimatedSize);
    lineOffsets->Allocate(estimatedSize);
    polyOffsets->Allocate(estimatedSize);

    //locator->SetPoints(newPts);
    locator->InitPointInsertion (newPts,
                                 this->Input->GetBounds(),
                                 this->Input->GetNumberOfPoints());

    vtkCellArray*& newVerts = this->NewVerts.Local();
    newVerts->Allocate(estimatedSize,estimatedSize);
    output->SetVerts(newVerts);

    vtkCellArray*& newLines = this->NewLines.Local();
    newLines->Allocate(estimatedSize,estimatedSize);
    output->SetLines(newLines);

    vtkCellArray*& newPolys = this->NewPolys.Local();
    newPolys->Allocate(estimatedSize,estimatedSize);
    output->SetPolys(newPolys);

    vtkDataArray*& cellScalars = this->CellScalars.Local();
    cellScalars = this->InScalars->NewInstance();
    cellScalars->SetNumberOfComponents(this->InScalars->GetNumberOfComponents());
    cellScalars->Allocate(VTK_CELL_SIZE*this->InScalars->GetNumberOfComponents());

    vtkPointData* outPd = output->GetPointData();
    vtkCellData* outCd = output->GetCellData();
    vtkPointData* inPd = this->Input->GetPointData();
    vtkCellData* inCd = this->Input->GetCellData();
    outPd->InterpolateAllocate(inPd, estimatedSize, estimatedSize);
    outCd->CopyAllocate(inCd, estimatedSize, estimatedSize);
  }

  void operator()(vtkIdType begin, vtkIdType end)
  {
    // Actual computation.
    // Note the usage of thread local objects. These objects
    // persist for each thread across multiple execution of the
    // functor.

    vtkLocalDataType& localData = this->LocalData.Local();

    vtkGenericCell* cell = this->Cell.Local();
    vtkDataArray* cs = this->CellScalars.Local();
    vtkPointData* inPd = this->Input->GetPointData();
    vtkCellData* inCd = this->Input->GetCellData();

    vtkPolyData* output = localData.Output;
    vtkPointData* outPd = output->GetPointData();
    vtkCellData* outCd = output->GetCellData();

    vtkCellArray* vrts = this->NewVerts.Local();
    vtkCellArray* lines = this->NewLines.Local();
    vtkCellArray* polys = this->NewPolys.Local();

    vtkPointLocator* loc = localData.Locator;

    vtkIdList* vertOffsets = localData.VertOffsets;
    vtkIdList* lineOffsets = localData.LineOffsets;
    vtkIdList* polyOffsets = localData.PolyOffsets;

    const double* values = this->Values;
    int numValues = this->NumValues;

    vtkNew<vtkIdList> pids;
    T range[2];

    for (vtkIdType cellid=begin; cellid<end; cellid++)
      {
      this->Input->GetCellPoints(cellid, pids.GetPointer());
      cs->SetNumberOfTuples(pids->GetNumberOfIds());
      this->InScalars->GetTuples(pids.GetPointer(), cs);
      int numCellScalars = cs->GetNumberOfComponents()
        * cs->GetNumberOfTuples();
      T* cellScalarPtr = static_cast<T*>(cs->GetVoidPointer(0));

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
          this->Input->GetCell(cellid, cell);

          for (int i=0; i < numValues; i++)
            {
            if ((values[i] >= range[0]) && (values[i] <= range[1]))
              {
              vtkIdType begVertSize = vrts->GetNumberOfConnectivityEntries();
              vtkIdType begLineSize = lines->GetNumberOfConnectivityEntries();
              vtkIdType begPolySize = polys->GetNumberOfConnectivityEntries();
              cell->Contour(values[i],
                            cs,
                            loc,
                            vrts,
                            lines,
                            polys,
                            inPd,
                            outPd,
                            inCd,
                            cellid,
                            outCd);
              // We keep track of the insertion point of verts, lines and polys.
              // This is later used when merging these data structures in parallel.
              // The reason this is needed is that vtkCellArray is not normally
              // random access with makes processing it in parallel very difficult.
              // So we create a semi-random-access structures in parallel. This
              // is only useful for merging since each of these indices can point
              // to multiple cells.
              if (vrts->GetNumberOfConnectivityEntries() > begVertSize)
                {
                vertOffsets->InsertNextId(begVertSize);
                }
              if (lines->GetNumberOfConnectivityEntries() > begLineSize)
                {
                lineOffsets->InsertNextId(begLineSize);
                }
              if (polys->GetNumberOfConnectivityEntries() > begPolySize)
                {
                polyOffsets->InsertNextId(begPolySize);
                }
              }
            }
          }
      }
  }

  void Reduce()
  {
    // Create the final multi-block dataset

    vtkNew<vtkMultiPieceDataSet> mp;
    int count = 0;

    vtkSMPThreadLocal<vtkLocalDataType>::iterator outIter =
      this->LocalData.begin();
    while(outIter != this->LocalData.end())
      {
      vtkPolyData* output = (*outIter).Output;

      if (output->GetVerts()->GetNumberOfCells() == 0)
        {
        output->SetVerts(0);
        }

      if (output->GetLines()->GetNumberOfCells() == 0)
        {
        output->SetLines(0);
        }

      if (output->GetPolys()->GetNumberOfCells() == 0)
        {
        output->SetPolys(0);
        }

      output->Squeeze();

      mp->SetPiece(count++, output);

      ++outIter;
      }

    vtkMultiBlockDataSet* output = vtkMultiBlockDataSet::SafeDownCast(this->Output);
    // If the output is a vtkPolyData (no merging), we throw away the multi-piece
    // dataset.
    if (output)
      {
      output->SetBlock(0, mp.GetPointer());
      }
  }
};
template <typename T>
void DoContour(vtkSMPContourGrid* filter,
               vtkUnstructuredGrid* input,
               vtkIdType numCells,
               vtkDataArray* inScalars,
               int numContours,
               double* values,
               vtkDataObject* output)
{
  // Contour in parallel
  vtkContourGridFunctor<T> functor(filter, input, inScalars, numContours, values, output);
  vtkSMPTools::For(0, numCells, functor);

  if (output->IsA("vtkPolyData"))
    {
    // Do the merging.
    vtkSMPThreadLocal<vtkLocalDataType>::iterator itr = functor.LocalData.begin();
    vtkSMPThreadLocal<vtkLocalDataType>::iterator end = functor.LocalData.end();

    std::vector<vtkSMPMergePolyDataHelper::InputData> mpData;
    while(itr != end)
      {
      mpData.push_back(vtkSMPMergePolyDataHelper::InputData((*itr).Output,
                                                            (*itr).Locator,
                                                            (*itr).VertOffsets,
                                                            (*itr).LineOffsets,
                                                            (*itr).PolyOffsets));
      ++itr;
      }

    vtkPolyData* moutput = vtkSMPMergePolyDataHelper::MergePolyData(mpData);
    output->ShallowCopy(moutput);
    moutput->Delete();
    }
}

}

int vtkSMPContourGrid::RequestDataObject(
  vtkInformation* vtkNotUsed(request),
  vtkInformationVector**,
  vtkInformationVector* outputVector)
{
  vtkInformation* info = outputVector->GetInformationObject(0);
  if (this->MergePieces)
    {
    vtkPolyData *output = vtkPolyData::GetData(info);
    if (!output)
      {
      vtkPolyData* newOutput = vtkPolyData::New();
      info->Set(vtkDataObject::DATA_OBJECT(), newOutput);
      newOutput->Delete();
      }
    }
  else
    {
    vtkMultiBlockDataSet *output = vtkMultiBlockDataSet::GetData(info);
    if (!output)
      {
      vtkMultiBlockDataSet* newOutput = vtkMultiBlockDataSet::New();
      info->Set(vtkDataObject::DATA_OBJECT(), newOutput);
      newOutput->Delete();
      }
    }
  return 1;
}

int vtkSMPContourGrid::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the input and output
  vtkUnstructuredGrid *input = vtkUnstructuredGrid::GetData(inputVector[0]);
  vtkDataObject *output = vtkDataObject::GetData(outputVector);

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

  if (inScalars->GetDataType() == VTK_FLOAT)
    {
    DoContour<float>(this, input, numCells, inScalars, numContours, values, output);
    }
  else if(inScalars->GetDataType() == VTK_DOUBLE)
    {
    DoContour<double>(this, input, numCells, inScalars, numContours, values, output);
    }

  return 1;
}

int vtkSMPContourGrid::FillOutputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkDataObject");
  return 1;
}

int vtkSMPContourGrid::ProcessRequest(vtkInformation* request,
                                      vtkInformationVector** inputVector,
                                      vtkInformationVector* outputVector)
{
  // generate the data
  if(request->Has(vtkDemandDrivenPipeline::REQUEST_DATA_OBJECT()))
    {
    return this->RequestDataObject(request, inputVector, outputVector);
    }

  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

void vtkSMPContourGrid::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
