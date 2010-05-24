/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkProbePolyhedron.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkProbePolyhedron.h"

#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkCellData.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkCellArray.h"
#include "vtkFloatArray.h"
#include "vtkDoubleArray.h"
#include "vtkMeanValueCoordinatesInterpolator.h"


vtkStandardNewMacro(vtkProbePolyhedron);

//----------------------------------------------------------------------------
vtkProbePolyhedron::vtkProbePolyhedron()
{
  this->ProbePointData = 1;
  this->ProbeCellData = 0;

  this->SetNumberOfInputPorts(2);
}

//----------------------------------------------------------------------------
vtkProbePolyhedron::~vtkProbePolyhedron()
{
}

//----------------------------------------------------------------------------
void vtkProbePolyhedron::SetSourceConnection(vtkAlgorithmOutput* algOutput)
{
  this->SetInputConnection(1, algOutput);
}
 
//----------------------------------------------------------------------------
void vtkProbePolyhedron::SetSource(vtkPolyData *input)
{
  this->SetInput(1, input);
}

//----------------------------------------------------------------------------
vtkPolyData *vtkProbePolyhedron::GetSource()
{
  if (this->GetNumberOfInputConnections(1) < 1)
    {
    return NULL;
    }
  
  return vtkPolyData::SafeDownCast(
    this->GetExecutive()->GetInputData(1, 0));
}

//----------------------------------------------------------------------------
int vtkProbePolyhedron::RequestData(
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
  vtkPolyData *source = vtkPolyData::SafeDownCast(
    sourceInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkDataSet *output = vtkDataSet::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  if (!source)
    {
    return 0;
    }

  // Make sure that the mesh consists of triangles. Bail out if not.
  vtkIdType numPolys = source->GetNumberOfPolys();
  vtkCellArray *srcPolys = source->GetPolys();
  
  if ( !numPolys || !srcPolys )
    {
    vtkErrorMacro("Probe polyhedron filter requires a non-empty mesh");
    return 0;
    }

  // Set up attribute interpolation. The input structure is passed to the
  // output.
  vtkIdType numInputPts = input->GetNumberOfPoints();
  vtkIdType numSrcPts = source->GetNumberOfPoints();
  vtkIdType numInputCells = input->GetNumberOfCells();
  output->CopyStructure(input);
  vtkPointData *outPD = output->GetPointData();
  vtkCellData *outCD = output->GetCellData();
  vtkPointData *srcPD = source->GetPointData();
  outPD->InterpolateAllocate(srcPD,numInputPts,1);
  outCD->InterpolateAllocate(srcPD,numInputCells,1);
  
  // Okay probe the polyhedral mesh. Have to loop over all points and compute
  // each points interpolation weights. These weights are used to perform the
  // interpolation.
  vtkPoints *srcPts = source->GetPoints();
  vtkDoubleArray *weights = vtkDoubleArray::New();
  weights->SetNumberOfComponents(1);
  weights->SetNumberOfTuples(numSrcPts);
  double *wPtr = weights->GetPointer(0);

  // InterpolatePoint requires knowing which points to interpolate from.
  vtkIdType ptId, cellId;
  vtkIdList *srcIds = vtkIdList::New();
  srcIds->SetNumberOfIds(numSrcPts);
  for (ptId=0; ptId < numSrcPts; ++ptId)
    {
    srcIds->SetId(ptId,ptId);
    }
        
  // Interpolate the point data (if requested)
  double x[3];
  int abort=0;
  vtkIdType idx=0, progressInterval=(numInputCells+numInputPts)/10 + 1;
  if ( this->ProbePointData )
    {
    for (ptId=0; ptId < numInputPts && !abort; ++ptId, ++idx)
      {
      if ( ! (idx % progressInterval) ) 
        {
        vtkDebugMacro(<<"Processing #" << idx);
        this->UpdateProgress(static_cast<double>(idx)/(numInputCells+numInputPts));
        abort = this->GetAbortExecute();
        }

      input->GetPoint(ptId, x);
      vtkMeanValueCoordinatesInterpolator::
        ComputeInterpolationWeights(x,srcPts,srcPolys,wPtr);

      outPD->InterpolatePoint(srcPD, ptId, srcIds, wPtr);
      }
    }
  
  // Interpolate the cell data (if requested)
  // Compute point value at the cell's parametric center.
  if ( this->ProbeCellData )
    {
    vtkCell *cell;
    int subId;
    double pcoords[3];
    x[0] = x[1] = x[2] = 0.0;

    for (cellId=0; cellId < numInputCells && !abort; ++cellId,++idx)
      {
      if ( ! (idx % progressInterval) ) 
        {
        vtkDebugMacro(<<"Processing #" << idx);
        this->UpdateProgress(static_cast<double>(idx)/(numInputCells+numInputPts));
        abort = this->GetAbortExecute();
        }

      cell = input->GetCell(cellId);
      if (cell->GetCellType() != VTK_EMPTY_CELL)
        {
        subId = cell->GetParametricCenter(pcoords);
        cell->EvaluateLocation(subId, pcoords, x, wPtr);
        }
      vtkMeanValueCoordinatesInterpolator::
        ComputeInterpolationWeights(x,srcPts,srcPolys,wPtr);

      outCD->InterpolatePoint(srcPD, cellId, srcIds, wPtr);
      }
    }

  // clean up
  srcIds->Delete();
  weights->Delete();

  return 1;
}

//----------------------------------------------------------------------------
int vtkProbePolyhedron::RequestInformation(
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
  outInfo->Set(vtkStreamingDemandDrivenPipeline::MAXIMUM_NUMBER_OF_PIECES(),
               inInfo->Get(
                 vtkStreamingDemandDrivenPipeline::MAXIMUM_NUMBER_OF_PIECES()));

  return 1;
}

//----------------------------------------------------------------------------
int vtkProbePolyhedron::RequestUpdateExtent(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  int usePiece = 0;

  // What ever happend to CopyUpdateExtent in vtkDataObject?
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
  
  return 1;
}

//----------------------------------------------------------------------------
void vtkProbePolyhedron::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  vtkDataObject *source = this->GetSource();
  os << indent << "Source: " << source << "\n";

  os << indent << "Probe Point Data: " 
     << (this->ProbePointData ? "true" : "false") << "\n";

  os << indent << "Probe Cell Data: " 
     << (this->ProbeCellData ? "true" : "false") << "\n";

}
