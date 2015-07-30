/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageProbeFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include <cstdio>
#include <cmath>

#include "vtkImageProbeFilter.h"

#include "vtkCell.h"
#include "vtkCellData.h"
#include "vtkCharArray.h"
#include "vtkIdTypeArray.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkImageProbeFilter.h"
#include "vtkPoints.h"

#include <vector>

vtkStandardNewMacro(vtkImageProbeFilter);

class vtkImageProbeFilter::vtkVectorOfArrays :
  public std::vector<vtkDataArray*>
{
};

//----------------------------------------------------------------------------
vtkImageProbeFilter::vtkImageProbeFilter()
{
  this->SpatialMatch = 0;
  this->ValidPoints = vtkIdTypeArray::New();
  this->MaskPoints = NULL;
  this->SetNumberOfInputPorts(2);
  this->ValidPointMaskArrayName = 0;
  this->SetValidPointMaskArrayName("vtkValidPointMask");
  this->CellArrays = new vtkVectorOfArrays();
  this->NumberOfValidPoints = 0;

  this->PointList = 0;
  this->CellList = 0;

  this->UseNullPoint = true;

  this->PassCellArrays = 0;
  this->PassPointArrays = 0;
  this->PassFieldArrays = 1;
  this->Tolerance = 1.0;
  this->ComputeTolerance = 1;
}

//----------------------------------------------------------------------------
vtkImageProbeFilter::~vtkImageProbeFilter()
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
void vtkImageProbeFilter::SetSourceConnection(vtkAlgorithmOutput* algOutput)
{
  this->SetInputConnection(1, algOutput);
}

//----------------------------------------------------------------------------
void vtkImageProbeFilter::SetSourceData(vtkDataObject *input)
{
  this->SetInputData(1, input);
}

//----------------------------------------------------------------------------
vtkDataObject *vtkImageProbeFilter::GetSource()
{
  if (this->GetNumberOfInputConnections(1) < 1)
    {
    return NULL;
    }

  return this->GetExecutive()->GetInputData(1, 0);
}

//----------------------------------------------------------------------------
int vtkImageProbeFilter::RequestData(
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

  if (!source)
    {
    return 0;
    }

  this->Probe(input, source, output);

  this->PassAttributeData(input, source, output);
  return 1;
}

//----------------------------------------------------------------------------
void vtkImageProbeFilter::PassAttributeData(
  vtkDataSet* input, vtkDataObject* vtkNotUsed(source), vtkDataSet* output)
{
  // copy point data arrays
  if (this->PassPointArrays)
    {
    int numPtArrays = input->GetPointData()->GetNumberOfArrays();
    for (int i=0; i<numPtArrays; ++i)
      {
      output->GetPointData()->AddArray(input->GetPointData()->GetArray(i));
      }
    }

  // copy cell data arrays
  if (this->PassCellArrays)
    {
    int numCellArrays = input->GetCellData()->GetNumberOfArrays();
    for (int i=0; i<numCellArrays; ++i)
      {
      output->GetCellData()->AddArray(input->GetCellData()->GetArray(i));
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
void vtkImageProbeFilter::BuildFieldList(vtkDataSet* source)
{
  delete this->PointList;
  delete this->CellList;

  this->PointList = new vtkDataSetAttributes::FieldList(1);
  this->PointList->InitializeFieldList(source->GetPointData());

  this->CellList = new vtkDataSetAttributes::FieldList(1);
  this->CellList->InitializeFieldList(source->GetCellData());
}

//----------------------------------------------------------------------------
// * input -- dataset probed with  (geometry)
// * source -- dataset probed into (solution)
// * output - output.
void vtkImageProbeFilter::InitializeForProbing(vtkDataSet* input, vtkDataSet* output)
{
  if (!this->PointList || !this->CellList)
    {
    vtkErrorMacro("BuildFieldList() must be called before calling this method.");
    return;
    }

  vtkIdType numPts = input->GetNumberOfPoints();

  // Initialize valid points/mask points arrays.
  this->NumberOfValidPoints = 0;
  //this->ValidPoints->Allocate(numPts);
  // if this is repeatedly called by the pipeline for a composite mesh,
  // you need a new array for each block
  // (that is you need to reinitialize the object)
  if (this->MaskPoints)
    {
    this->MaskPoints->Delete();
    }
  this->MaskPoints = vtkCharArray::New();
  this->MaskPoints->SetNumberOfComponents(1);
  //this->MaskPoints->SetNumberOfTuples(numPts);
  //this->MaskPoints->FillComponent(0, 0);  // Will fill this later
  this->MaskPoints->SetName(this->ValidPointMaskArrayName?
    this->ValidPointMaskArrayName: "vtkValidPointMask");

  // First, copy the input to the output as a starting point
  output->CopyStructure(input);

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
    else
      {
      this->CellArrays->push_back(NULL); // in order to align both lists
      }

    }
  tempCellData->Delete();

  outPD->AddArray(this->MaskPoints);

  // Since we haven't resize the point arrays, we need to fill them up with
  // nulls whenever we have a miss when probing.
  this->UseNullPoint = true;

}

//----------------------------------------------------------------------------
void vtkImageProbeFilter::Probe(vtkDataSet *input, vtkDataSet *source,
                           vtkDataSet *output)
{
  this->BuildFieldList(source);
  this->InitializeForProbing(input, output);
  this->ProbeEmptyPoints(input, 0, source, output);
}

// find grid point ids within given bmin and bmax
void vtkImageProbeFilter::get_intersect_idx(double bmin, double bmax, double origin, double stepsize, int steps, int &minidx, int &nidx)
{
  minidx = ceil( (bmin - origin)/stepsize );
  if ( minidx < 0 ) minidx = 0;
  int maxidx;
  maxidx = floor( (bmax - origin)/stepsize );
  if (maxidx > steps-1) maxidx = steps-1;
  nidx = maxidx - minidx + 1;
  if (nidx < 0) nidx = 0;
}

//----------------------------------------------------------------------------
// * input -- dataset probed with  (geometry)
// * source -- dataset probed into (solution)
// * output - output.
void vtkImageProbeFilter::ProbeEmptyPoints(vtkDataSet *input_,
  int srcIdx,
  vtkDataSet *source, vtkDataSet *output_)
{
  vtkIdType numPts;
  vtkPointData *pd, *outPD;
  vtkCellData* cd;
  int subId;
  double pcoords[3], *weights;
  double fastweights[256];

  vtkImageData *input= vtkImageData::SafeDownCast(input_);
  vtkImageData *output=vtkImageData::SafeDownCast(output_);
  if (input==NULL || output == NULL) {
    vtkErrorMacro("input and output should be in type vtkImageData.");
    return;
  }

  vtkDebugMacro(<<"Probing data");

  pd = source->GetPointData();
  cd = source->GetCellData();
  assert (cd->GetNumberOfArrays() == this->CellArrays->size());

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

  // initialize arrays
  for (int i=0; i<outPD->GetNumberOfArrays(); i++)
  {
    outPD->GetArray(i)->SetNumberOfTuples(numPts);
    // initialize the values to 0
    for (int j = 0 ; j < outPD->GetArray(i)->GetNumberOfComponents(); j++)
      outPD->GetArray(i)->FillComponent(j, 0);
  }

  char* maskArray = this->MaskPoints->GetPointer(0);

  //----------------------------------------
  int nPointsOut = input->GetNumberOfPoints();
  int nDataCells = source->GetNumberOfCells();
  int cellId;
  int abort = 0;
  double *origin  = input->GetOrigin();
  double *spacing = input->GetSpacing();
  int    *dim     = input->GetDimensions();
  vtkIdType progressInterval=nDataCells/20 + 1;

  // Loop over all source cells
  for (cellId=0; cellId < nDataCells && !abort; cellId++)
  {
    if ( !(cellId % progressInterval) )
      {
      this->UpdateProgress(static_cast<double>(cellId)/nDataCells);
      abort = GetAbortExecute();
      }

    vtkCell * cell = source->GetCell(cellId);

    //for (int i=0; i<4; i++)
    //  printf("Cell points: %lf %lf %lf\n", cell->GetPoints()->GetPoint(i)[0], cell->GetPoints()->GetPoint(i)[1], cell->GetPoints()->GetPoint(i)[2]);

    // get coordinates of sampling grids
    double bounds[6] ;    cell->GetBounds(bounds);
    int nidx, nidy, nidz, minidx, minidy, minidz;
    get_intersect_idx(bounds[0], bounds[1], origin[0], spacing[0], dim[0],
        /*output:*/minidx, nidx);
    get_intersect_idx(bounds[2], bounds[3], origin[1], spacing[1], dim[1],
        /*output:*/minidy, nidy);
    get_intersect_idx(bounds[4], bounds[5], origin[2], spacing[2], dim[2],
        /*output:*/minidz, nidz);

    if (nidx==0 || nidy==0 || nidz==0)
      continue;

    int ix,iy,iz;
    double p[3];
    for (iz=minidz; iz<minidz+nidz; iz++)
    {
      p[2] = origin[2]+iz*spacing[2];
      for (iy=minidy; iy<minidy+nidy; iy++)
      {
        p[1] = origin[1]+iy*spacing[1];
        for (ix=minidx; ix<minidx+nidx; ix++)
        {
          // For each grid point within the cell bound, interpolate values
          p[0] = origin[0]+ix*spacing[0];

          double closestPoint[3];
          double dist2;
          int inside = cell->EvaluatePosition(p, closestPoint, subId, pcoords, dist2, weights);

          if (inside && (dist2==0)) // ensure it's inside the cell
          {
            int global_id = ix+dim[0]*(iy + dim[1]*iz);

            // Interpolate the point data
            outPD->InterpolatePoint((*this->PointList), pd, srcIdx, global_id,
              cell->PointIds, weights);

            // Assign cell data
            int i;
            for (i=0; i < this->CellArrays->size(); i++)
            {
              vtkDataArray* inArray = cd->GetArray(i);
              vtkDataArray *outArray = this->CellArrays->at(i);
              if (inArray && outArray)
              {
                outPD->CopyTuple(inArray, outArray, cellId, global_id);
              }
            }

            maskArray[global_id] = static_cast<char>(1);
          }
        }
      }
    }
  }

  if (mcs>256)
    {
    delete [] weights;
    }
}

//----------------------------------------------------------------------------
int vtkImageProbeFilter::RequestInformation(
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
int vtkImageProbeFilter::RequestUpdateExtent(
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
void vtkImageProbeFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkDataObject *source = this->GetSource();

  this->Superclass::PrintSelf(os,indent);
  os << indent << "Source: " << source << "\n";
  os << indent << "SpatialMatch: " << ( this->SpatialMatch ? "On" : "Off" ) << "\n";
  os << indent << "ValidPointMaskArrayName: " << (this->ValidPointMaskArrayName?
    this->ValidPointMaskArrayName : "vtkValidPointMask") << "\n";
  os << indent << "ValidPoints: " << this->ValidPoints << "\n";
  os << indent << "PassFieldArrays: "
     << (this->PassFieldArrays? "On" : " Off") << "\n";
}
