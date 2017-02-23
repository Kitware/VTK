/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkProbeFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkProbeFilter.h"

#include "vtkBoundingBox.h"
#include "vtkCell.h"
#include "vtkCellData.h"
#include "vtkCharArray.h"
#include "vtkGenericCell.h"
#include "vtkIdTypeArray.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkSMPTools.h"
#include "vtkSMPThreadLocal.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include <algorithm>
#include <vector>

vtkStandardNewMacro(vtkProbeFilter);

#define CELL_TOLERANCE_FACTOR_SQR  1e-6

class vtkProbeFilter::vtkVectorOfArrays :
  public std::vector<vtkDataArray*>
{
};

//----------------------------------------------------------------------------
vtkProbeFilter::vtkProbeFilter()
{
  this->CategoricalData = 0;
  this->SpatialMatch = 0;
  this->ValidPoints = vtkIdTypeArray::New();
  this->MaskPoints = NULL;
  this->SetNumberOfInputPorts(2);
  this->ValidPointMaskArrayName = 0;
  this->SetValidPointMaskArrayName("vtkValidPointMask");
  this->CellArrays = new vtkVectorOfArrays();

  this->PointList = 0;
  this->CellList = 0;

  this->PassCellArrays = 0;
  this->PassPointArrays = 0;
  this->PassFieldArrays = 1;
  this->Tolerance = 1.0;
  this->ComputeTolerance = 1;
}

//----------------------------------------------------------------------------
vtkProbeFilter::~vtkProbeFilter()
{
  if (this->MaskPoints)
  {
    this->MaskPoints->Delete();
    this->MaskPoints = 0;
  }
  this->ValidPoints->Delete();
  this->ValidPoints = NULL;
  this->SetValidPointMaskArrayName(0);
  delete this->CellArrays;

  delete this->PointList;
  delete this->CellList;
}

//----------------------------------------------------------------------------
void vtkProbeFilter::SetSourceConnection(vtkAlgorithmOutput* algOutput)
{
  this->SetInputConnection(1, algOutput);
}

//----------------------------------------------------------------------------
void vtkProbeFilter::SetSourceData(vtkDataObject *input)
{
  this->SetInputData(1, input);
}

//----------------------------------------------------------------------------
vtkDataObject *vtkProbeFilter::GetSource()
{
  if (this->GetNumberOfInputConnections(1) < 1)
  {
    return NULL;
  }

  return this->GetExecutive()->GetInputData(1, 0);
}

//----------------------------------------------------------------------------
vtkIdTypeArray* vtkProbeFilter::GetValidPoints()
{
  if (this->MaskPoints &&
      this->MaskPoints->GetMTime() > this->ValidPoints->GetMTime())
  {
    char* maskArray = this->MaskPoints->GetPointer(0);
    vtkIdType numPts = this->MaskPoints->GetNumberOfTuples();
    vtkIdType numValidPoints = std::count(maskArray, maskArray + numPts,
                                          static_cast<char>(1));
    this->ValidPoints->Allocate(numValidPoints);
    for (vtkIdType i = 0; i < numPts; ++i)
    {
      if (maskArray[i])
      {
        this->ValidPoints->InsertNextValue(i);
      }
    }
    this->ValidPoints->Modified();
  }

  return this->ValidPoints;
}

//----------------------------------------------------------------------------
int vtkProbeFilter::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *sourceInfo = inputVector[1]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkDataSet *input = vtkDataSet::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkDataSet *source = vtkDataSet::SafeDownCast(
    sourceInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkDataSet *output = vtkDataSet::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // First, copy the input to the output as a starting point
  output->CopyStructure(input);

  if (this->CategoricalData == 1)
  {
    // If the categorical data flag is enabled, then a) there must be scalars
    // to treat as categorical data, and b) the scalars must have one component.
    if (!source->GetPointData()->GetScalars())
    {
      vtkErrorMacro(<<"No input scalars!");
      return 1;
    }
    if (source->GetPointData()->GetScalars()->GetNumberOfComponents() != 1)
    {
      vtkErrorMacro(<<"Source scalars have more than one component! Cannot categorize!");
      return 1;
    }

    // Set the scalar to interpolate via nearest neighbor. That way, we won't
    // get any false values (for example, a zone 4 cell appearing on the
    // boundary of zone 3 and zone 5).
    output->GetPointData()->SetCopyAttribute(vtkDataSetAttributes::SCALARS, 2,
                                             vtkDataSetAttributes::INTERPOLATE);
  }

  if (source)
  {
    this->Probe(input, source, output);
  }

  this->PassAttributeData(input, source, output);
  return 1;
}

//----------------------------------------------------------------------------
void vtkProbeFilter::PassAttributeData(
  vtkDataSet* input, vtkDataObject* vtkNotUsed(source), vtkDataSet* output)
{
  // copy point data arrays
  if (this->PassPointArrays)
  {
    int numPtArrays = input->GetPointData()->GetNumberOfArrays();
    for (int i=0; i<numPtArrays; ++i)
    {
      vtkDataArray *da = input->GetPointData()->GetArray(i);
      if (!output->GetPointData()->HasArray(da->GetName()))
      {
        output->GetPointData()->AddArray(da);
      }
    }

    // Set active attributes in the output to the active attributes in the input
    for (int i = 0; i < vtkDataSetAttributes::NUM_ATTRIBUTES; ++i)
    {
      vtkAbstractArray* da = input->GetPointData()->GetAttribute(i);
      if (da && da->GetName() && !output->GetPointData()->GetAttribute(i))
      {
        output->GetPointData()->SetAttribute(da, i);
      }
    }
  }

  // copy cell data arrays
  if (this->PassCellArrays)
  {
    int numCellArrays = input->GetCellData()->GetNumberOfArrays();
    for (int i=0; i<numCellArrays; ++i)
    {
      vtkDataArray *da = input->GetCellData()->GetArray(i);
      if (!output->GetCellData()->HasArray(da->GetName()))
      {
        output->GetCellData()->AddArray(da);
      }
    }

    // Set active attributes in the output to the active attributes in the input
    for (int i = 0; i < vtkDataSetAttributes::NUM_ATTRIBUTES; ++i)
    {
      vtkAbstractArray* da = input->GetCellData()->GetAttribute(i);
      if (da && da->GetName() && !output->GetCellData()->GetAttribute(i))
      {
        output->GetCellData()->SetAttribute(da, i);
      }
    }
  }

  if (this->PassFieldArrays)
  {
    // nothing to do, vtkDemandDrivenPipeline takes care of that.
  }
  else
  {
    output->GetFieldData()->Initialize();
  }
}

//----------------------------------------------------------------------------
void vtkProbeFilter::BuildFieldList(vtkDataSet* source)
{
  delete this->PointList;
  delete this->CellList;

  this->PointList = new vtkDataSetAttributes::FieldList(1);
  this->PointList->InitializeFieldList(source->GetPointData());

  this->CellList = new vtkDataSetAttributes::FieldList(1);
  this->CellList->InitializeFieldList(source->GetCellData());
}

//----------------------------------------------------------------------------
// * input -- dataset probed with
// * source -- dataset probed into
// * output - output.
void vtkProbeFilter::InitializeForProbing(vtkDataSet* input,
  vtkDataSet* output)
{
  if (!this->PointList || !this->CellList)
  {
    vtkErrorMacro("BuildFieldList() must be called before calling this method.");
    return;
  }

  vtkIdType numPts = input->GetNumberOfPoints();

  // if this is repeatedly called by the pipeline for a composite mesh,
  // you need a new array for each block
  // (that is you need to reinitialize the object)
  if (this->MaskPoints)
  {
    this->MaskPoints->Delete();
  }
  this->MaskPoints = vtkCharArray::New();
  this->MaskPoints->SetNumberOfComponents(1);
  this->MaskPoints->SetNumberOfTuples(numPts);
  this->MaskPoints->FillValue(0);
  this->MaskPoints->SetName(this->ValidPointMaskArrayName?
    this->ValidPointMaskArrayName: "vtkValidPointMask");

  // Allocate storage for output PointData
  // All input PD is passed to output as PD. Those arrays in input CD that are
  // not present in output PD will be passed as output PD.
  vtkPointData* outPD = output->GetPointData();
  outPD->InterpolateAllocate((*this->PointList), numPts, numPts);

  vtkCellData* tempCellData = vtkCellData::New();
  tempCellData->InterpolateAllocate( (*this->CellList), numPts, numPts);

  this->CellArrays->clear();
  int numCellArrays = tempCellData->GetNumberOfArrays();
  for (int cc=0; cc < numCellArrays; cc++)
  {
    vtkDataArray* inArray = tempCellData->GetArray(cc);
    if (inArray && inArray->GetName() && !outPD->GetArray(inArray->GetName()))
    {
      outPD->AddArray(inArray);
      this->CellArrays->push_back(inArray);
    }
  }
  tempCellData->Delete();

  this->InitializeOutputArrays(outPD, numPts);
  outPD->AddArray(this->MaskPoints);
}

//----------------------------------------------------------------------------
void vtkProbeFilter::InitializeOutputArrays(vtkPointData *outPD, vtkIdType numPts)
{
  for (int i = 0; i < outPD->GetNumberOfArrays(); ++i)
  {
    vtkDataArray* da = outPD->GetArray(i);
    if (da)
    {
      da->SetNumberOfTuples(numPts);
      da->Fill(0);
    }
  }
}

//----------------------------------------------------------------------------
void vtkProbeFilter::DoProbing(vtkDataSet *input, int srcIdx, vtkDataSet *source,
                               vtkDataSet *output)
{
  vtkBoundingBox sbox(source->GetBounds());
  vtkBoundingBox ibox(input->GetBounds());
  if (!sbox.Intersects(ibox))
  {
    return;
  }

  if (vtkImageData::SafeDownCast(input))
  {
    vtkImageData *inImage = vtkImageData::SafeDownCast(input);
    vtkImageData *outImage = vtkImageData::SafeDownCast(output);
    this->ProbePointsImageData(inImage, srcIdx, source, outImage);
  }
  else
  {
    this->ProbeEmptyPoints(input, srcIdx, source, output);
  }
}

//----------------------------------------------------------------------------
void vtkProbeFilter::Probe(vtkDataSet *input, vtkDataSet *source,
                           vtkDataSet *output)
{
  this->BuildFieldList(source);
  this->InitializeForProbing(input, output);
  this->DoProbing(input, 0, source, output);
}

//----------------------------------------------------------------------------
void vtkProbeFilter::ProbeEmptyPoints(vtkDataSet *input,
  int srcIdx,
  vtkDataSet *source, vtkDataSet *output)
{
  vtkIdType ptId, numPts;
  double x[3], tol2;
  vtkCell *cell;
  vtkPointData *pd, *outPD;
  vtkCellData* cd;
  int subId;
  double pcoords[3], *weights;
  double fastweights[256];

  vtkDebugMacro(<<"Probing data");

  pd = source->GetPointData();
  cd = source->GetCellData();

  // lets use a stack allocated array if possible for performance reasons
  int mcs = source->GetMaxCellSize();
  if (mcs<=256)
  {
    weights = fastweights;
  }
  else
  {
    weights = new double[mcs];
  }

  numPts = input->GetNumberOfPoints();
  outPD = output->GetPointData();

  char* maskArray = this->MaskPoints->GetPointer(0);

  tol2 = this->ComputeTolerance ? VTK_DOUBLE_MAX :
         (this->Tolerance * this->Tolerance);

  // Loop over all input points, interpolating source data
  //
  int abort=0;
  vtkIdType progressInterval=numPts/20 + 1;
  for (ptId=0; ptId < numPts && !abort; ptId++)
  {
    if ( !(ptId % progressInterval) )
    {
      this->UpdateProgress(static_cast<double>(ptId)/numPts);
      abort = GetAbortExecute();
    }

    if (maskArray[ptId] == static_cast<char>(1))
    {
      // skip points which have already been probed with success.
      // This is helpful for multiblock dataset probing.
      continue;
    }

    // Get the xyz coordinate of the point in the input dataset
    input->GetPoint(ptId, x);

    // Find the cell that contains xyz and get it
    vtkIdType cellId = source->FindCell(x,NULL,-1,tol2,subId,pcoords,weights);
    if (cellId >= 0)
    {
      cell = source->GetCell(cellId);
      if (this->ComputeTolerance)
      {
        // If ComputeTolerance is set, compute a tolerance proportional to the
        // cell length.
        double dist2;
        double closestPoint[3];
        cell->EvaluatePosition(x, closestPoint, subId, pcoords, dist2, weights);
        if (dist2 > (cell->GetLength2() * CELL_TOLERANCE_FACTOR_SQR))
        {
          cell = 0;
        }
      }
    }
    else
    {
      cell = 0;
    }

    if (cell)
    {
      // Interpolate the point data
      outPD->InterpolatePoint((*this->PointList), pd, srcIdx, ptId,
        cell->PointIds, weights);
      vtkVectorOfArrays::iterator iter;
      for (iter = this->CellArrays->begin(); iter != this->CellArrays->end();
        ++iter)
      {
        vtkDataArray* inArray = cd->GetArray((*iter)->GetName());
        if (inArray)
        {
          outPD->CopyTuple(inArray, *iter, cellId, ptId);
        }
      }
      maskArray[ptId] = static_cast<char>(1);
    }
  }

  this->MaskPoints->Modified();
  if (mcs>256)
  {
    delete [] weights;
  }
}

//---------------------------------------------------------------------------
static void GetPointIdsInRange(double rangeMin, double rangeMax, double start,
  double stepsize, int numSteps, int &minid, int &maxid)
{
  if (stepsize == 0)
  {
    minid = maxid = 0;
    return;
  }

  minid = vtkMath::Ceil((rangeMin - start)/stepsize);
  if (minid < 0)
  {
    minid = 0;
  }

  maxid = vtkMath::Floor((rangeMax - start)/stepsize);
  if (maxid > numSteps-1)
  {
    maxid = numSteps-1;
  }
}

void vtkProbeFilter::ProbeImagePointsInCell(vtkCell *cell, vtkIdType cellId,
                                            vtkDataSet *source, int srcBlockId,
                                            const double start[3],
                                            const double spacing[3],
                                            const int dim[3],
                                            vtkPointData *outPD,
                                            char *maskArray,
                                            double *wtsBuff)
{
  vtkPointData *pd = source->GetPointData();
  vtkCellData *cd = source->GetCellData();

  // get coordinates of sampling grids
  double cellBounds[6];
  cell->GetBounds(cellBounds);

  int idxBounds[6];
  GetPointIdsInRange(cellBounds[0], cellBounds[1], start[0], spacing[0],
    dim[0], idxBounds[0], idxBounds[1]);
  GetPointIdsInRange(cellBounds[2], cellBounds[3], start[1], spacing[1],
    dim[1], idxBounds[2], idxBounds[3]);
  GetPointIdsInRange(cellBounds[4], cellBounds[5], start[2], spacing[2],
    dim[2], idxBounds[4], idxBounds[5]);

  if ((idxBounds[1] - idxBounds[0]) < 0 ||
      (idxBounds[3] - idxBounds[2]) < 0 ||
      (idxBounds[5] - idxBounds[4]) < 0)
  {
    return;
  }

  double cpbuf[3];
  double dist2 = 0;
  double *closestPoint = cpbuf;
  if (cell->IsA("vtkCell3D"))
  {
    // we only care about closest point and its distance for 2D cells
    closestPoint = NULL;
  }

  double userTol2 = this->Tolerance * this->Tolerance;
  for (int iz=idxBounds[4]; iz<=idxBounds[5]; iz++)
  {
    double p[3];
    p[2] = start[2] + iz*spacing[2];
    for (int iy=idxBounds[2]; iy<=idxBounds[3]; iy++)
    {
      p[1] = start[1] + iy*spacing[1];
      for (int ix=idxBounds[0]; ix<=idxBounds[1]; ix++)
      {
        // For each grid point within the cell bound, interpolate values
        p[0] = start[0] + ix*spacing[0];

        double pcoords[3];
        int subId;
        int inside = cell->EvaluatePosition(p, closestPoint, subId, pcoords,
                                            dist2, wtsBuff);

        // If ComputeTolerance is set, compute a tolerance proportional to the
        // cell length. Otherwise, use the user specified absolute tolerance.
        double tol2 = this->ComputeTolerance ?
                      (CELL_TOLERANCE_FACTOR_SQR * cell->GetLength2()) :
                      userTol2;

        if ((inside == 1) && (dist2 <= tol2))
        {
          vtkIdType ptId = ix + dim[0]*(iy + dim[1]*iz);

          // Interpolate the point data
          outPD->InterpolatePoint((*this->PointList), pd, srcBlockId, ptId,
                                  cell->PointIds, wtsBuff);

          // Assign cell data
          vtkVectorOfArrays::iterator iter;
          for (iter = this->CellArrays->begin();
              iter != this->CellArrays->end(); ++iter)
          {
            vtkDataArray* inArray = cd->GetArray((*iter)->GetName());
            if (inArray)
            {
              outPD->CopyTuple(inArray, *iter, cellId, ptId);
            }
          }

          maskArray[ptId] = static_cast<char>(1);
        }
      }
    }
  }
}

namespace {

class CellStorage
{
public:
  CellStorage()
  {
    this->Initialize();
  }

  ~CellStorage()
  {
    this->Clear();
  }

  // Copying does not make sense for this class but vtkSMPThreadLocal needs
  // these functions to compile. Just initialize the object.
  CellStorage(const CellStorage&)
  {
    this->Initialize();
  }

  CellStorage& operator=(const CellStorage&)
  {
    this->Clear();
    this->Initialize();
    return *this;
  }

  vtkCell* GetCell(vtkDataSet *dataset, vtkIdType cellId)
  {
    int celltype = dataset->GetCellType(cellId);
    vtkGenericCell* &gc = this->Cells[celltype];
    if (!gc)
    {
      gc = vtkGenericCell::New();
    }
    dataset->GetCell(cellId, gc);
    return gc->GetRepresentativeCell();
  }

private:
  void Initialize()
  {
    vtkGenericCell *null = NULL;
    std::fill(this->Cells, this->Cells + VTK_NUMBER_OF_CELL_TYPES, null);
  }

  void Clear()
  {
    for (int i = 0; i < VTK_NUMBER_OF_CELL_TYPES; ++i)
    {
      if (this->Cells[i])
      {
        this->Cells[i]->Delete();
      }
    }
  }

  vtkGenericCell *Cells[VTK_NUMBER_OF_CELL_TYPES];
};

} // anonymous namespace

class vtkProbeFilter::ProbeImageDataWorklet
{
public:
  ProbeImageDataWorklet(vtkProbeFilter *probeFilter,
                        vtkDataSet *source, int srcBlockId,
                        const double start[3], const double spacing[3],
                        const int dim[3], vtkPointData *outPD, char *maskArray,
                        int maxCellSize)
    : ProbeFilter(probeFilter), Source(source), SrcBlockId(srcBlockId),
      Start(start), Spacing(spacing), Dim(dim), OutPointData(outPD),
      MaskArray(maskArray), MaxCellSize(maxCellSize)
  {
  }

  void operator()(vtkIdType cellBegin, vtkIdType cellEnd)
  {
    double fastweights[256];
    double *weights;
    if (this->MaxCellSize <= 256)
    {
      weights = fastweights;
    }
    else
    {
      std::vector<double> &dynamicweights = this->WeightsBuffer.Local();
      dynamicweights.resize(this->MaxCellSize);
      weights = &dynamicweights[0];
    }

    CellStorage &cs = this->Cells.Local();
    for (vtkIdType cellId = cellBegin; cellId < cellEnd; ++cellId)
    {
      vtkCell *cell = cs.GetCell(this->Source, cellId);
      this->ProbeFilter->ProbeImagePointsInCell(cell, cellId, this->Source,
        this->SrcBlockId, this->Start, this->Spacing, this->Dim,
        this->OutPointData, this->MaskArray, weights);
    }
  }

private:
  vtkProbeFilter *ProbeFilter;
  vtkDataSet *Source;
  int SrcBlockId;
  const double *Start;
  const double *Spacing;
  const int *Dim;
  vtkPointData *OutPointData;
  char *MaskArray;
  int MaxCellSize;

  vtkSMPThreadLocal<std::vector<double> > WeightsBuffer;
  vtkSMPThreadLocal<CellStorage> Cells;
};

//----------------------------------------------------------------------------
void vtkProbeFilter::ProbePointsImageData(vtkImageData *input,
  int srcIdx, vtkDataSet *source, vtkImageData *output)
{
  vtkPointData *outPD = output->GetPointData();
  char* maskArray = this->MaskPoints->GetPointer(0);

  //----------------------------------------
  double spacing[3];
  input->GetSpacing(spacing);
  int extent[6];
  input->GetExtent(extent);
  int dim[3];
  input->GetDimensions(dim);
  double start[3];
  input->GetOrigin(start);
  start[0] += static_cast<double>(extent[0]) * spacing[0];
  start[1] += static_cast<double>(extent[2]) * spacing[1];
  start[2] += static_cast<double>(extent[4]) * spacing[2];

  vtkIdType numSrcCells = source->GetNumberOfCells();

   // dummy call required before multithreaded calls
  static_cast<void>(source->GetCellType(0));
  ProbeImageDataWorklet worklet(this, source, srcIdx, start, spacing, dim,
                                outPD, maskArray, source->GetMaxCellSize());
  vtkSMPTools::For(0, numSrcCells, worklet);

  this->MaskPoints->Modified();
}

//----------------------------------------------------------------------------
int vtkProbeFilter::RequestInformation(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *sourceInfo = inputVector[1]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  outInfo->CopyEntry(sourceInfo,
                     vtkStreamingDemandDrivenPipeline::TIME_STEPS());
  outInfo->CopyEntry(sourceInfo,
                     vtkStreamingDemandDrivenPipeline::TIME_RANGE());

  outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),
               inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()),
               6);

  // A variation of the bug fix from John Biddiscombe.
  // Make sure that the scalar type and number of components
  // are propagated from the source not the input.
  if (vtkImageData::HasScalarType(sourceInfo))
  {
    vtkImageData::SetScalarType(vtkImageData::GetScalarType(sourceInfo),
                                outInfo);
  }
  if (vtkImageData::HasNumberOfScalarComponents(sourceInfo))
  {
    vtkImageData::SetNumberOfScalarComponents(
      vtkImageData::GetNumberOfScalarComponents(sourceInfo),
      outInfo);
  }

  return 1;
}

//----------------------------------------------------------------------------
int vtkProbeFilter::RequestUpdateExtent(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *sourceInfo = inputVector[1]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  int usePiece = 0;

  // What ever happened to CopyUpdateExtent in vtkDataObject?
  // Copying both piece and extent could be bad.  Setting the piece
  // of a structured data set will affect the extent.
  vtkDataObject* output = outInfo->Get(vtkDataObject::DATA_OBJECT());
  if (output &&
      (!strcmp(output->GetClassName(), "vtkUnstructuredGrid") ||
       !strcmp(output->GetClassName(), "vtkPolyData")))
  {
    usePiece = 1;
  }

  inInfo->Set(vtkStreamingDemandDrivenPipeline::EXACT_EXTENT(), 1);

  sourceInfo->Remove(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT());
  if (sourceInfo->Has(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()))
  {
    sourceInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),
      sourceInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()), 6);
  }

  if ( ! this->SpatialMatch)
  {
    sourceInfo->Set(
      vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(), 0);
    sourceInfo->Set(
      vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(), 1);
    sourceInfo->Set(
      vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(), 0);
  }
  else if (this->SpatialMatch == 1)
  {
    if (usePiece)
    {
      // Request an extra ghost level because the probe
      // gets external values with computation prescision problems.
      // I think the probe should be changed to have an epsilon ...
      sourceInfo->Set(
        vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(),
        outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()));
      sourceInfo->Set(
        vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(),
        outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES()));
      sourceInfo->Set(
        vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(),
        outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS())+1);
    }
    else
    {
      sourceInfo->Set(
        vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),
        outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT()), 6);
    }
  }

  if (usePiece)
  {
    inInfo->Set(
      vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(),
      outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()));
    inInfo->Set(
      vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(),
      outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES()));
    inInfo->Set(
      vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(),
      outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS()));
  }
  else
  {
    inInfo->Set(
      vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),
      outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT()), 6);
  }

  // Use the whole input in all processes, and use the requested update
  // extent of the output to divide up the source.
  if (this->SpatialMatch == 2)
  {
    inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(), 0);
    inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(), 1);
    inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(), 0);
    sourceInfo->Set(
      vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(),
      outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()));
    sourceInfo->Set(
      vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(),
      outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES()));
    sourceInfo->Set(
      vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(),
      outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS()));
  }
  return 1;
}

//----------------------------------------------------------------------------
void vtkProbeFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkDataObject *source = this->GetSource();

  this->Superclass::PrintSelf(os,indent);
  os << indent << "Source: " << source << "\n";
  os << indent << "SpatialMatch: " << ( this->SpatialMatch ? "On" : "Off" ) << "\n";
  os << indent << "ValidPointMaskArrayName: " << (this->ValidPointMaskArrayName?
    this->ValidPointMaskArrayName : "vtkValidPointMask") << "\n";
  os << indent << "PassFieldArrays: "
     << (this->PassFieldArrays? "On" : " Off") << "\n";
}
