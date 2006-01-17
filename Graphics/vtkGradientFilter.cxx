// -*- c++ -*-
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGradientFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/

#include "vtkGradientFilter.h"

#include "vtkCell.h"
#include "vtkCellData.h"
#include "vtkCellDataToPointData.h"
#include "vtkDataArray.h"
#include "vtkDataSet.h"
#include "vtkIdList.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkUnstructuredGrid.h"

//-----------------------------------------------------------------------------

vtkCxxRevisionMacro(vtkGradientFilter, "1.2");
vtkStandardNewMacro(vtkGradientFilter);

template<class data_type>
void vtkGradientFilterDoComputePointGradients(vtkDataSet *structure,
                                              data_type *scalars,
                                              data_type *gradients);
template<class data_type>
int vtkGradientFilterAddCellContribution(vtkIdType pointId,
                                         double *pointCoord, vtkCell *cell,
                                         data_type *scalars, data_type *g);

template<class data_type>
void vtkGradientFilterDoComputeCellGradients(vtkDataSet *structure,
                                             data_type *scalars,
                                             data_type *gradients);

//-----------------------------------------------------------------------------

vtkGradientFilter::vtkGradientFilter()
{
  this->ResultArrayName = NULL;
  this->SetInputScalars(vtkDataObject::FIELD_ASSOCIATION_POINTS_THEN_CELLS,
                        vtkDataSetAttributes::SCALARS);
}

vtkGradientFilter::~vtkGradientFilter()
{
  this->SetResultArrayName(NULL);
}

void vtkGradientFilter::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Result Array Name:"
     << (this->ResultArrayName ? this->ResultArrayName : "Gradients") << endl;
}

//-----------------------------------------------------------------------------

void vtkGradientFilter::SetInputScalars(int fieldAssociation, const char *name)
{
  if (   (fieldAssociation != vtkDataObject::FIELD_ASSOCIATION_POINTS)
      && (fieldAssociation != vtkDataObject::FIELD_ASSOCIATION_CELLS)
      && (fieldAssociation != vtkDataObject::FIELD_ASSOCIATION_POINTS_THEN_CELLS) )
    {
    vtkErrorMacro("Input scalars must be associated with points or cells.");
    return;
    }

  this->SetInputArrayToProcess(0, 0, 0, fieldAssociation, name);
}

void vtkGradientFilter::SetInputScalars(int fieldAssociation,
                                        int fieldAttributeType)
{
  if (   (fieldAssociation != vtkDataObject::FIELD_ASSOCIATION_POINTS)
      && (fieldAssociation != vtkDataObject::FIELD_ASSOCIATION_CELLS)
      && (fieldAssociation != vtkDataObject::FIELD_ASSOCIATION_POINTS_THEN_CELLS) )
    {
    vtkErrorMacro("Input scalars must be associated with points or cells.");
    return;
    }

  this->SetInputArrayToProcess(0, 0, 0, fieldAssociation, fieldAttributeType);
}

//-----------------------------------------------------------------------------

int vtkGradientFilter::RequestUpdateExtent(vtkInformation *vtkNotUsed(request),
                                           vtkInformationVector **inputVector,
                                           vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // Technically, this code is only correct for pieces extent types.  However,
  // since this class is pretty inefficient for data types that use 3D extents,
  // we'll punt on the ghost levels for them, too.
  int piece, numPieces, ghostLevels;
  
  piece = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  numPieces = outInfo->Get(
                   vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
  ghostLevels = outInfo->Get(
             vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS());
  
  if (numPieces > 1)
    {
    ++ghostLevels;
    }

  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(), piece);
  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(),
              numPieces);
  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(),
              ghostLevels);
  inInfo->Set(vtkStreamingDemandDrivenPipeline::EXACT_EXTENT(), 1);

  return 1;
}


//-----------------------------------------------------------------------------

static int vtkGradientFilterHasArray(vtkFieldData *fieldData,
                                     vtkDataArray *array)
{
  int numarrays = fieldData->GetNumberOfArrays();
  for (int i = 0; i < numarrays; i++)
    {
    if (array == fieldData->GetArray(i))
      {
      return 1;
      }
    }
  return 0;
}

//-----------------------------------------------------------------------------

int vtkGradientFilter::RequestData(vtkInformation *vtkNotUsed(request),
                                   vtkInformationVector **inputVector,
                                   vtkInformationVector *outputVector)
{
  vtkDebugMacro("RequestData");

  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  vtkDataSet *input
    = vtkDataSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkDataSet *output
    = vtkDataSet::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkDataArray *scalars = this->GetInputArrayToProcess(0, inputVector);

  if (scalars == NULL)
    {
    vtkErrorMacro("No input scalars.");
    return 0;
    }
  if (scalars->GetNumberOfComponents() != 1)
    {
    vtkErrorMacro("Input scalars must have one component.");
    return 0;
    }

  int fieldAssociation;
  if (vtkGradientFilterHasArray(input->GetPointData(), scalars))
    {
    fieldAssociation = vtkDataObject::FIELD_ASSOCIATION_POINTS;
    }
  else if (vtkGradientFilterHasArray(input->GetCellData(), scalars))
    {
    fieldAssociation = vtkDataObject::FIELD_ASSOCIATION_CELLS;
    }
  else
    {
    vtkErrorMacro("Input scalars do not seem to be either point or cell scalars.");
    return 0;
    }

  output->CopyStructure(input);
  output->GetPointData()->PassData(input->GetPointData());
  output->GetCellData()->PassData(input->GetCellData());

  vtkDataArray *gradients
    = vtkDataArray::CreateDataArray(scalars->GetDataType());
  gradients->SetNumberOfComponents(3);
  gradients->SetNumberOfTuples(scalars->GetNumberOfTuples());
  if (this->ResultArrayName)
    {
    gradients->SetName(this->ResultArrayName);
    }
  else
    {
    gradients->SetName("Gradients");
    }

  if (fieldAssociation == vtkDataObject::FIELD_ASSOCIATION_POINTS)
    {
    switch (scalars->GetDataType())
      {
      vtkTemplateMacro(vtkGradientFilterDoComputePointGradients(
                                       input,
                                       (VTK_TT *)scalars->GetVoidPointer(0),
                                       (VTK_TT *)gradients->GetVoidPointer(0)));
      }

    output->GetPointData()->AddArray(gradients);
    }
  else  // fieldAssocation == vtkDataObject::FIELD_ASSOCIATION_CELLS
    {
    // We need to convert cell scalars to points scalars.
    vtkDataSet *dummy = input->NewInstance();
    dummy->CopyStructure(input);
    dummy->GetCellData()->SetScalars(scalars);

    vtkCellDataToPointData *cd2pd = vtkCellDataToPointData::New();
    cd2pd->SetInput(dummy);
    cd2pd->PassCellDataOff();
    cd2pd->Update();
    vtkDataArray *pointScalars
      = cd2pd->GetOutput()->GetPointData()->GetScalars();
    pointScalars->Register(this);
    cd2pd->Delete();
    dummy->Delete();

    switch (pointScalars->GetDataType())
      {
      vtkTemplateMacro(vtkGradientFilterDoComputeCellGradients(
                                     input,
                                     (VTK_TT *)pointScalars->GetVoidPointer(0),
                                     (VTK_TT *)gradients->GetVoidPointer(0)));
      }

    output->GetCellData()->AddArray(gradients);
    pointScalars->UnRegister(this);
    }

  gradients->Delete();

  // If necessary, remove a layer of ghost cells.
  int numPieces
    = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
  if (numPieces > 1)
    {
    int ghostLevel = outInfo->Get(
             vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS());
    vtkPolyData *pd = vtkPolyData::SafeDownCast(output);
    vtkUnstructuredGrid *ug = vtkUnstructuredGrid::SafeDownCast(output);
    if (pd) pd->RemoveGhostCells(ghostLevel+1);
    if (ug) ug->RemoveGhostCells(ghostLevel+1);
    }

  return 1;
}

//-----------------------------------------------------------------------------

template<class data_type>
void vtkGradientFilterDoComputePointGradients(vtkDataSet *structure,
                                              data_type *scalars,
                                              data_type *gradients)
{
  vtkIdType numpts;
  data_type *g;
  vtkIdType point;
  vtkIdList *currentPoint;
  vtkIdList *cellsOnPoint;

  currentPoint = vtkIdList::New();
  currentPoint->SetNumberOfIds(1);
  cellsOnPoint = vtkIdList::New();

  numpts = structure->GetNumberOfPoints();
  g = gradients;
  for (point = 0; point < numpts; point++)
    {
    g[0] = g[1] = g[2] = 0;
    currentPoint->SetId(0, point);

    double pointcoords[3];
    structure->GetPoint(point, pointcoords);

    // Get all cells touching this point.
    structure->GetCellNeighbors(-1, currentPoint, cellsOnPoint);
    vtkIdType numCellNeighbors = cellsOnPoint->GetNumberOfIds();
    vtkIdType numValidCellNeighbors = 0;

    // Iterate on all cells and find all points connected to current point
    // by an edge.
    for (vtkIdType neighbor = 0; neighbor < numCellNeighbors; neighbor++)
      {
      vtkCell *cell = structure->GetCell(cellsOnPoint->GetId(neighbor));

      numValidCellNeighbors += vtkGradientFilterAddCellContribution(
                                          point, pointcoords, cell, scalars, g);
      }

    if (numCellNeighbors > 0)
      {
      g[0] /= numCellNeighbors;
      g[1] /= numCellNeighbors;
      g[2] /= numCellNeighbors;
      }
    g += 3;
    }

  currentPoint->Delete();
  cellsOnPoint->Delete();
}

//-----------------------------------------------------------------------------

template<class data_type>
int vtkGradientFilterAddCellContribution(vtkIdType pointId,
                                         double *pointCoord, vtkCell *cell,
                                         data_type *scalars, data_type *g)
{
  double parametricCoord[3];
  int subId;
  double dummy;
  int numpoints = cell->GetNumberOfPoints();
  double *values = new double[numpoints];
  double derivative[3];
  int i;

  // Watch out for degenerate cells.  They make the derivative calculation
  // fail.
  int numedges = cell->GetNumberOfEdges();
  for (i = 0; i < numedges; i++)
    {
    vtkCell *edge = cell->GetEdge(i);
    if ((pointId == edge->GetPointId(0)) && (pointId == edge->GetPointId(1)))
      {
      // Found a collapsed edge associated with this point.  Not good.
      return 0;
      }
    }

  // Get parametric position of point.
  cell->EvaluatePosition(pointCoord, NULL, subId, parametricCoord,
                         dummy, values/*Really another dummy.*/);

  // Get values of scalars at cell points.
  for (i = 0; i < numpoints; i++)
    {
    values[i] = static_cast<double>(scalars[cell->GetPointId(i)]);
    }

  // Get derivitive of cell at point.
  cell->Derivatives(subId, parametricCoord, values, 1, derivative);

  g[0] += static_cast<data_type>(derivative[0]);
  g[1] += static_cast<data_type>(derivative[1]);
  g[2] += static_cast<data_type>(derivative[2]);

  return 1;
}

//-----------------------------------------------------------------------------

template<class data_type>
void vtkGradientFilterDoComputeCellGradients(vtkDataSet *structure,
                                             data_type *scalars,
                                             data_type *gradients)
{
  vtkIdType numcells;
  data_type *g;
  vtkIdType cellid;

  numcells = structure->GetNumberOfCells();
  g = gradients;
  for (cellid = 0; cellid < numcells; cellid++)
    {
    vtkCell *cell = structure->GetCell(cellid);

    int subId;
    double cellCenter[3];
    subId = cell->GetParametricCenter(cellCenter);

    int numpoints = cell->GetNumberOfPoints();
    double derivative[3];
    double *values = new double[numpoints];
    for (int i = 0; i < numpoints; i++)
      {
      values[i] = static_cast<double>(scalars[cell->GetPointId(i)]);
      }

    cell->Derivatives(subId, cellCenter, values, 1, derivative);

    g[0] = static_cast<data_type>(derivative[0]);
    g[1] = static_cast<data_type>(derivative[1]);
    g[2] = static_cast<data_type>(derivative[2]);
    g += 3;
    }
}
