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
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkRectilinearGrid.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStructuredGrid.h"
#include "vtkUnstructuredGrid.h"

#include <vector>

//-----------------------------------------------------------------------------

vtkStandardNewMacro(vtkGradientFilter);

namespace
{
  // helper function to replace the gradient of a vector
  // with the vorticity/curl of that vector
//-----------------------------------------------------------------------------
  template<class data_type>
  void ComputeVorticityFromGradient(data_type* gradients, data_type* vorticity)
  {
    vorticity[0] = gradients[7] - gradients[5];
    vorticity[1] = gradients[2] - gradients[6];
    vorticity[2] = gradients[3] - gradients[1];
  }

  template<class data_type>
  void ComputeQCriterionFromGradient(data_type* gradients, data_type* qCriterion)
  {
    data_type t1 = ( (gradients[7]-gradients[5])*(gradients[7]-gradients[5]) +
                     (gradients[3]-gradients[1])*(gradients[3]-gradients[1]) +
                     (gradients[2]-gradients[6])*(gradients[2]-gradients[6]) ) / 2;
    data_type t2 = gradients[0]*gradients[0]+gradients[4]*gradients[4]+
      gradients[8]*gradients[8]+ (
        (gradients[3]+gradients[1])*(gradients[3]+gradients[1]) +
        (gradients[6]+gradients[2])*(gradients[6]+gradients[2]) +
        (gradients[7]+gradients[5])*(gradients[7]+gradients[5]) ) / 2;

    qCriterion[0] = (t1 - t2) / 2;
  }

  // Functions for unstructured grids and polydatas
  template<class data_type>
  void ComputePointGradientsUG(
    vtkDataSet *structure, data_type *array, data_type *gradients,
    int numberOfInputComponents, data_type* vorticity, data_type* qCriterion);

  int GetCellParametricData(
    vtkIdType pointId, double pointCoord[3], vtkCell *cell, int & subId,
    double parametricCoord[3]);

  template<class data_type>
  void ComputeCellGradientsUG(
    vtkDataSet *structure, data_type *array, data_type *gradients,
    int numberOfInputComponents, data_type* vorticity, data_type* qCriterion);

  // Functions for image data and structured grids
  template<class Grid, class data_type>
  void ComputeGradientsSG(Grid output, data_type* array, data_type* gradients,
                          int numberOfInputComponents, int fieldAssociation,
                          data_type* vorticity, data_type* qCriterion);

  bool vtkGradientFilterHasArray(vtkFieldData *fieldData,
                                 vtkDataArray *array)
  {
    int numarrays = fieldData->GetNumberOfArrays();
    for (int i = 0; i < numarrays; i++)
      {
      if (array == fieldData->GetArray(i))
        {
        return true;
        }
      }
    return false;
  }

  // generic way to get the coordinate for either a cell (using
  // the parametric center) or a point
  void GetGridEntityCoordinate(vtkDataSet* grid, int fieldAssociation,
                               vtkIdType index, double coords[3])
  {
    if(fieldAssociation == vtkDataObject::FIELD_ASSOCIATION_POINTS)
      {
      grid->GetPoint(index, coords);
      }
    else
      {
      vtkCell* cell = grid->GetCell(index);
      double pcoords[3];
      int subId = cell->GetParametricCenter(pcoords);
      std::vector<double> weights(cell->GetNumberOfPoints()+1);
      cell->EvaluateLocation(subId, pcoords, coords, &weights[0]);
      }
  }
} // end anonymous namespace

//-----------------------------------------------------------------------------
vtkGradientFilter::vtkGradientFilter()
{
  this->ResultArrayName = NULL;
  this->VorticityArrayName = NULL;
  this->QCriterionArrayName = NULL;
  this->FasterApproximation = 0;
  this->ComputeVorticity = 0;
  this->ComputeQCriterion = 0;
  this->SetInputScalars(vtkDataObject::FIELD_ASSOCIATION_POINTS_THEN_CELLS,
                        vtkDataSetAttributes::SCALARS);
}

//-----------------------------------------------------------------------------
vtkGradientFilter::~vtkGradientFilter()
{
  this->SetResultArrayName(NULL);
  this->SetVorticityArrayName(NULL);
  this->SetQCriterionArrayName(NULL);
}

//-----------------------------------------------------------------------------
void vtkGradientFilter::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "ResultArrayName:"
     << (this->ResultArrayName ? this->ResultArrayName : "Gradients") << endl;
  os << indent << "VorticityArrayName:"
     << (this->VorticityArrayName ? this->VorticityArrayName : "Vorticity") << endl;
  os << indent << "QCriterionArrayName:"
     << (this->QCriterionArrayName ? this->QCriterionArrayName : "Q-criterion") << endl;
  os << indent << "FasterApproximation:" << this->FasterApproximation << endl;
  os << indent << "ComputeVorticity:" << this->ComputeVorticity << endl;
  os << indent << "ComputeQCriterion:" << this->ComputeQCriterion << endl;
}

//-----------------------------------------------------------------------------
void vtkGradientFilter::SetInputScalars(int fieldAssociation, const char *name)
{
  if (   (fieldAssociation != vtkDataObject::FIELD_ASSOCIATION_POINTS)
      && (fieldAssociation != vtkDataObject::FIELD_ASSOCIATION_CELLS)
      && (fieldAssociation != vtkDataObject::FIELD_ASSOCIATION_POINTS_THEN_CELLS) )
    {
    vtkErrorMacro("Input Array must be associated with points or cells.");
    return;
    }

  this->SetInputArrayToProcess(0, 0, 0, fieldAssociation, name);
}

//-----------------------------------------------------------------------------
void vtkGradientFilter::SetInputScalars(int fieldAssociation,
                                        int fieldAttributeType)
{
  if (   (fieldAssociation != vtkDataObject::FIELD_ASSOCIATION_POINTS)
      && (fieldAssociation != vtkDataObject::FIELD_ASSOCIATION_CELLS)
      && (fieldAssociation != vtkDataObject::FIELD_ASSOCIATION_POINTS_THEN_CELLS) )
    {
    vtkErrorMacro("Input Array must be associated with points or cells.");
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

  vtkDataArray *array = this->GetInputArrayToProcess(0, inputVector);

  if (array == NULL)
    {
    vtkErrorMacro("No input array.");
    return 0;
    }
  if (array->GetNumberOfComponents() == 0)
    {
    vtkErrorMacro("Input array must have at least one component.");
    return 0;
    }

  // we can only compute vorticity and Q criterion if the input
  // array has 3 components. if we can't compute them because of
  // this we only mark internally the we aren't computing them
  // since we don't want to change the state of the filter.
  bool computeVorticity = this->ComputeVorticity != 0;
  bool computeQCriterion = this->ComputeQCriterion != 0;
  if( (this->ComputeQCriterion || this->ComputeVorticity)
      && array->GetNumberOfComponents() != 3)
    {
    vtkWarningMacro("Input array must have exactly three components "
                    << "with ComputeVorticity or ComputeQCriterion flag turned "
                    << "on. Skipping vorticity and Q-criterion computation.");
    computeVorticity = false;
    computeQCriterion = false;
    }

  int fieldAssociation;
  if (vtkGradientFilterHasArray(input->GetPointData(), array))
    {
    fieldAssociation = vtkDataObject::FIELD_ASSOCIATION_POINTS;
    }
  else if (vtkGradientFilterHasArray(input->GetCellData(), array))
    {
    fieldAssociation = vtkDataObject::FIELD_ASSOCIATION_CELLS;
    }
  else
    {
    vtkErrorMacro("Input arrays do not seem to be either point or cell arrays.");
    return 0;
    }

  output->CopyStructure(input);
  output->GetPointData()->PassData(input->GetPointData());
  output->GetCellData()->PassData(input->GetCellData());

  if(output->IsA("vtkImageData") || output->IsA("vtkStructuredGrid") ||
          output->IsA("vtkRectilinearGrid") )
    {
    this->ComputeRegularGridGradient(
      array, fieldAssociation, computeVorticity, computeQCriterion, output);
    }
  else
    {
    this->ComputeUnstructuredGridGradient(
      array, fieldAssociation, input, computeVorticity, computeQCriterion, output);
    }

  // If necessary, remove a layer of ghost cells.
  int numPieces = outInfo->Get(
    vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
  if (numPieces > 1)
    {
    int ghostLevel = outInfo->Get(
      vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS());
    vtkPolyData *pd = vtkPolyData::SafeDownCast(output);
    vtkUnstructuredGrid *ug = vtkUnstructuredGrid::SafeDownCast(output);
    // Currently the only grids that ghost cells can be removed from
    // are unstructured grids and polydatas
    if (pd)
      {
      pd->RemoveGhostCells(ghostLevel+1);
      }
    else if (ug)
      {
      ug->RemoveGhostCells(ghostLevel+1);
      }
    }

  return 1;
}

//-----------------------------------------------------------------------------
int vtkGradientFilter::ComputeUnstructuredGridGradient(
  vtkDataArray* array, int fieldAssociation, vtkDataSet* input,
  bool computeVorticity, bool computeQCriterion, vtkDataSet* output)
{
  vtkDataArray *gradients
    = vtkDataArray::CreateDataArray(array->GetDataType());
  int numberOfInputComponents = array->GetNumberOfComponents();
  gradients->SetNumberOfComponents(3*numberOfInputComponents);
  gradients->SetNumberOfTuples(array->GetNumberOfTuples());
  if (this->ResultArrayName)
    {
    gradients->SetName(this->ResultArrayName);
    }
  else
    {
    gradients->SetName("Gradients");
    }
  vtkSmartPointer<vtkDataArray> vorticity;
  if(computeVorticity)
    {
    vorticity.TakeReference(vtkDataArray::CreateDataArray(array->GetDataType()));
    vorticity->SetNumberOfComponents(3);
    vorticity->SetNumberOfTuples(array->GetNumberOfTuples());
    if (this->VorticityArrayName)
      {
      vorticity->SetName(this->VorticityArrayName);
      }
    else
      {
      vorticity->SetName("Vorticity");
      }
    }
  vtkSmartPointer<vtkDataArray> qCriterion;
  if(computeQCriterion)
    {
    qCriterion.TakeReference(vtkDataArray::CreateDataArray(array->GetDataType()));
    qCriterion->SetNumberOfTuples(array->GetNumberOfTuples());
    if (this->QCriterionArrayName)
      {
      qCriterion->SetName(this->QCriterionArrayName);
      }
    else
      {
      qCriterion->SetName("Q-criterion");
      }
    }

  if (fieldAssociation == vtkDataObject::FIELD_ASSOCIATION_POINTS)
    {
    if (!this->FasterApproximation)
      {
      switch (array->GetDataType())
        {
        vtkTemplateMacro(ComputePointGradientsUG(
                           input,
                           static_cast<VTK_TT *>(array->GetVoidPointer(0)),
                           static_cast<VTK_TT *>(gradients->GetVoidPointer(0)),
                           numberOfInputComponents,
                           (vorticity == NULL ? NULL :
                            static_cast<VTK_TT *>(vorticity->GetVoidPointer(0))),
                           (qCriterion == NULL ? NULL :
                            static_cast<VTK_TT *>(qCriterion->GetVoidPointer(0)))));
        }

      output->GetPointData()->AddArray(gradients);
      if(vorticity)
        {
        output->GetPointData()->AddArray(vorticity);
        }
      if(qCriterion)
        {
        output->GetPointData()->AddArray(qCriterion);
        }
      }
    else // this->FasterApproximation
      {
      // The cell computation is faster and works off of point data anyway.  The
      // faster approximation is to use the cell algorithm and then convert the
      // result to point data.
      vtkDataArray *cellGradients
        = vtkDataArray::CreateDataArray(gradients->GetDataType());
      cellGradients->SetName(gradients->GetName());
      cellGradients->SetNumberOfComponents(3*array->GetNumberOfComponents());
      cellGradients->SetNumberOfTuples(input->GetNumberOfCells());

      switch (array->GetDataType())
        {
        vtkTemplateMacro(
          ComputeCellGradientsUG(
            input, static_cast<VTK_TT *>(array->GetVoidPointer(0)),
            static_cast<VTK_TT *>(cellGradients->GetVoidPointer(0)),
            numberOfInputComponents,
            (vorticity == NULL ? NULL :
             static_cast<VTK_TT *>(vorticity->GetVoidPointer(0))),
            (qCriterion == NULL ? NULL :
             static_cast<VTK_TT *>(qCriterion->GetVoidPointer(0)))));
        }

      // We need to convert cell Array to points Array.
      vtkDataSet *dummy = input->NewInstance();
      dummy->CopyStructure(input);
      dummy->GetCellData()->AddArray(cellGradients);
      if(vorticity)
        {
        dummy->GetCellData()->AddArray(vorticity);
        }
      if(qCriterion)
        {
        dummy->GetCellData()->AddArray(qCriterion);
        }

      vtkCellDataToPointData *cd2pd = vtkCellDataToPointData::New();
      cd2pd->SetInputData(dummy);
      cd2pd->PassCellDataOff();
      cd2pd->Update();

      // Set the gradients array in the output and cleanup.
      vtkDataArray *pointGradients
        = cd2pd->GetOutput()->GetPointData()->GetArray(gradients->GetName());
      output->GetPointData()->AddArray(pointGradients);
      if(qCriterion)
        {
        output->GetPointData()->AddArray(qCriterion);
        }
      cd2pd->Delete();
      dummy->Delete();
      cellGradients->Delete();
      }
    }
  else  // fieldAssocation == vtkDataObject::FIELD_ASSOCIATION_CELLS
    {
    // We need to convert cell Array to points Array.
    vtkDataSet *dummy = input->NewInstance();
    dummy->CopyStructure(input);
    dummy->GetCellData()->SetScalars(array);

    vtkCellDataToPointData *cd2pd = vtkCellDataToPointData::New();
    cd2pd->SetInputData(dummy);
    cd2pd->PassCellDataOff();
    cd2pd->Update();
    vtkDataArray *pointScalars
      = cd2pd->GetOutput()->GetPointData()->GetScalars();
    pointScalars->Register(this);
    cd2pd->Delete();
    dummy->Delete();

    switch (pointScalars->GetDataType())
      {
      vtkTemplateMacro(ComputeCellGradientsUG(
                         input,
                         static_cast<VTK_TT *>(pointScalars->GetVoidPointer(0)),
                         static_cast<VTK_TT *>(gradients->GetVoidPointer(0)),
                         numberOfInputComponents,
                         (vorticity == NULL ? NULL :
                          static_cast<VTK_TT *>(vorticity->GetVoidPointer(0))),
                         (qCriterion == NULL ? NULL :
                          static_cast<VTK_TT *>(qCriterion->GetVoidPointer(0)))));
      }

    output->GetCellData()->AddArray(gradients);
    if(vorticity)
      {
      output->GetCellData()->AddArray(vorticity);
      }
    if(qCriterion)
      {
      output->GetCellData()->AddArray(qCriterion);
      }
    pointScalars->UnRegister(this);
    }

  gradients->Delete();

  return 1;
}

//-----------------------------------------------------------------------------
int vtkGradientFilter::ComputeRegularGridGradient(
  vtkDataArray* array, int fieldAssociation, bool computeVorticity,
  bool computeQCriterion, vtkDataSet* output)
{
  vtkDataArray *gradients
    = vtkDataArray::CreateDataArray(array->GetDataType());
  int numberOfInputComponents = array->GetNumberOfComponents();
  gradients->SetNumberOfComponents(3*numberOfInputComponents);
  gradients->SetNumberOfTuples(array->GetNumberOfTuples());
  if (this->ResultArrayName)
    {
    gradients->SetName(this->ResultArrayName);
    }
  else
    {
    gradients->SetName("Gradients");
    }
  vtkSmartPointer<vtkDataArray> vorticity;
  if(computeVorticity)
    {
    vorticity.TakeReference(vtkDataArray::CreateDataArray(array->GetDataType()));
    vorticity->SetNumberOfComponents(3);
    vorticity->SetNumberOfTuples(array->GetNumberOfTuples());
    if (this->VorticityArrayName)
      {
      vorticity->SetName(this->VorticityArrayName);
      }
    else
      {
      vorticity->SetName("Vorticity");
      }
    }
  vtkSmartPointer<vtkDataArray> qCriterion;
  if(computeQCriterion)
    {
    qCriterion.TakeReference(vtkDataArray::CreateDataArray(array->GetDataType()));
    qCriterion->SetNumberOfTuples(array->GetNumberOfTuples());
    if (this->QCriterionArrayName)
      {
      qCriterion->SetName(this->QCriterionArrayName);
      }
    else
      {
      qCriterion->SetName("Q-criterion");
      }
    }

  if(vtkStructuredGrid* structuredGrid = vtkStructuredGrid::SafeDownCast(output))
    {
    switch (array->GetDataType())
      {
      vtkTemplateMacro(ComputeGradientsSG(
                         structuredGrid,
                         static_cast<VTK_TT *>(array->GetVoidPointer(0)),
                         static_cast<VTK_TT *>(gradients->GetVoidPointer(0)),
                         numberOfInputComponents, fieldAssociation,
                         (vorticity == NULL ? NULL :
                          static_cast<VTK_TT *>(vorticity->GetVoidPointer(0))),
                         (qCriterion == NULL ? NULL :
                          static_cast<VTK_TT *>(qCriterion->GetVoidPointer(0)))));
      }
    }
  else if(vtkImageData* imageData = vtkImageData::SafeDownCast(output))
    {
    switch (array->GetDataType())
      {
      vtkTemplateMacro(ComputeGradientsSG(
                         imageData,
                         static_cast<VTK_TT *>(array->GetVoidPointer(0)),
                         static_cast<VTK_TT *>(gradients->GetVoidPointer(0)),
                         numberOfInputComponents, fieldAssociation,
                         (vorticity == NULL ? NULL :
                          static_cast<VTK_TT *>(vorticity->GetVoidPointer(0))),
                         (qCriterion == NULL ? NULL :
                          static_cast<VTK_TT *>(qCriterion->GetVoidPointer(0)))));
      }
    }
  else if(vtkRectilinearGrid* rectilinearGrid = vtkRectilinearGrid::SafeDownCast(output))
    {
    switch (array->GetDataType())
      {
      vtkTemplateMacro(ComputeGradientsSG(
                         rectilinearGrid,
                         static_cast<VTK_TT *>(array->GetVoidPointer(0)),
                         static_cast<VTK_TT *>(gradients->GetVoidPointer(0)),
                         numberOfInputComponents, fieldAssociation,
                         (vorticity == NULL ? NULL :
                          static_cast<VTK_TT *>(vorticity->GetVoidPointer(0))),
                         (qCriterion == NULL ? NULL :
                          static_cast<VTK_TT *>(qCriterion->GetVoidPointer(0)))));
      }
    }
  if(fieldAssociation == vtkDataObject::FIELD_ASSOCIATION_POINTS)
    {
    output->GetPointData()->AddArray(gradients);
    if(vorticity)
      {
      output->GetPointData()->AddArray(vorticity);
      }
    if(qCriterion)
      {
      output->GetPointData()->AddArray(qCriterion);
      }
    }
  else if(fieldAssociation == vtkDataObject::FIELD_ASSOCIATION_CELLS)
    {
    output->GetCellData()->AddArray(gradients);
    if(vorticity)
      {
      output->GetCellData()->AddArray(vorticity);
      }
    if(qCriterion)
      {
      output->GetCellData()->AddArray(qCriterion);
      }
    }
  else
    {
    vtkErrorMacro("Bad fieldAssociation value " << fieldAssociation << endl);
    }
  gradients->Delete();

  return 1;
}

namespace {
//-----------------------------------------------------------------------------
  template<class data_type>
  void ComputePointGradientsUG(
    vtkDataSet *structure, data_type *array, data_type *gradients,
    int numberOfInputComponents, data_type* vorticity, data_type* qCriterion)
  {
    vtkIdList* currentPoint = vtkIdList::New();
    currentPoint->SetNumberOfIds(1);
    vtkIdList* cellsOnPoint = vtkIdList::New();

    vtkIdType numpts = structure->GetNumberOfPoints();
    std::vector<data_type> g(3*numberOfInputComponents);

    int numberOfOutputComponents = 3*numberOfInputComponents;

    for (vtkIdType point = 0; point < numpts; point++)
      {
      currentPoint->SetId(0, point);
      double pointcoords[3];
      structure->GetPoint(point, pointcoords);
      // Get all cells touching this point.
      structure->GetCellNeighbors(-1, currentPoint, cellsOnPoint);
      vtkIdType numCellNeighbors = cellsOnPoint->GetNumberOfIds();
      vtkIdType numValidCellNeighbors = 0;

      for(int i=0;i<numberOfInputComponents*3;i++)
        {
        g[i] = 0;
        }

      // Iterate on all cells and find all points connected to current point
      // by an edge.
      for (vtkIdType neighbor = 0; neighbor < numCellNeighbors; neighbor++)
        {
        vtkCell *cell = structure->GetCell(cellsOnPoint->GetId(neighbor));
        int subId;
        double parametricCoord[3];
        if(GetCellParametricData(point, pointcoords, cell,
                                 subId, parametricCoord))
          {
          numValidCellNeighbors++;
          for(int InputComponent=0;InputComponent<numberOfInputComponents;InputComponent++)
            {
            int NumberOfCellPoints = cell->GetNumberOfPoints();
            std::vector<double> values(NumberOfCellPoints);
            // Get values of Array at cell points.
            for (int i = 0; i < NumberOfCellPoints; i++)
              {
              values[i] = static_cast<double>(
                array[cell->GetPointId(i)*numberOfInputComponents+InputComponent]);
              }

            double derivative[3];
            // Get derivative of cell at point.
            cell->Derivatives(subId, parametricCoord, &values[0], 1, derivative);

            g[InputComponent*3] += static_cast<data_type>(derivative[0]);
            g[InputComponent*3+1] += static_cast<data_type>(derivative[1]);
            g[InputComponent*3+2] += static_cast<data_type>(derivative[2]);
            } // iterating over Components
          } // if(GetCellParametricData())
        } // iterating over neighbors

      if (numCellNeighbors > 0)
        {
        for(int i=0;i<3*numberOfInputComponents;i++)
          {
          g[i] /= numCellNeighbors;
          }
        }

      if(vorticity)
        {
        ComputeVorticityFromGradient(&g[0], vorticity+3*point);
        }
      if(qCriterion)
        {
        ComputeQCriterionFromGradient(&g[0], qCriterion+point);
        }
      for(int i=0;i<numberOfOutputComponents;i++)
        {
        gradients[point*numberOfOutputComponents+i] = g[i];
        }
      }  // iterating over points in grid

    currentPoint->Delete();
    cellsOnPoint->Delete();
  }

//-----------------------------------------------------------------------------
  int GetCellParametricData(vtkIdType pointId, double pointCoord[3],
                            vtkCell *cell, int &subId, double parametricCoord[3])
  {
    // Watch out for degenerate cells.  They make the derivative calculation
    // fail.
    vtkIdList *pointIds = cell->GetPointIds();
    int timesPointRegistered = 0;
    for (int i = 0; i < pointIds->GetNumberOfIds(); i++)
      {
      if (pointId == pointIds->GetId(i))
        {
        timesPointRegistered++;
        }
      }
    if (timesPointRegistered != 1)
      {
      // The cell should have the point exactly once.  Not good.
      return 0;
      }

    double dummy;
    int numpoints = cell->GetNumberOfPoints();
    std::vector<double> values(numpoints);
    // Get parametric position of point.
    cell->EvaluatePosition(pointCoord, NULL, subId, parametricCoord,
                           dummy, &values[0]/*Really another dummy.*/);

    return 1;
  }

//-----------------------------------------------------------------------------
  template<class data_type>
    void ComputeCellGradientsUG(
      vtkDataSet *structure, data_type *array, data_type *gradients,
      int numberOfInputComponents, data_type* vorticity, data_type* qCriterion)
  {
    vtkIdType numcells = structure->GetNumberOfCells();
    std::vector<double> values(8);
    for (vtkIdType cellid = 0; cellid < numcells; cellid++)
      {
      vtkCell *cell = structure->GetCell(cellid);

      int subId;
      double cellCenter[3];
      subId = cell->GetParametricCenter(cellCenter);

      int numpoints = cell->GetNumberOfPoints();
      if(static_cast<size_t>(numpoints) > values.size())
        {
        values.resize(numpoints);
        }
      double derivative[3];
      for(int inputComponent=0;inputComponent<numberOfInputComponents;
          inputComponent++)
        {
        for (int i = 0; i < numpoints; i++)
          {
          values[i] = static_cast<double>(
            array[cell->GetPointId(i)*numberOfInputComponents+inputComponent]);
          }

        cell->Derivatives(subId, cellCenter, &values[0], 1, derivative);
        gradients[cellid*3*numberOfInputComponents+inputComponent*3] =
          static_cast<data_type>(derivative[0]);
        gradients[cellid*3*numberOfInputComponents+inputComponent*3+1] =
          static_cast<data_type>(derivative[1]);
        gradients[cellid*3*numberOfInputComponents+inputComponent*3+2] =
          static_cast<data_type>(derivative[2]);
        }
      if(vorticity)
        {
        ComputeVorticityFromGradient(gradients+3*cellid*numberOfInputComponents, vorticity+3*cellid);
        }
      if(qCriterion)
        {
        ComputeQCriterionFromGradient(gradients+3*cellid*numberOfInputComponents, qCriterion+cellid);
        }
      }
  }

//-----------------------------------------------------------------------------
  template<class Grid, class data_type>
  void ComputeGradientsSG(Grid output, data_type* array, data_type* gradients,
                          int numberOfInputComponents, int fieldAssociation,
                          data_type* vorticity, data_type* qCriterion)
  {
    int i, j, k, idx, idx2, ii, inputComponent;
    double xp[3], xm[3], factor;
    double xxi, yxi, zxi, xeta, yeta, zeta, xzeta, yzeta, zzeta;
    double aj, xix, xiy, xiz, etax, etay, etaz, zetax, zetay, zetaz;
    // for finite differencing -- the values on the "plus" side and
    // "minus" side of the point to be computed at
    std::vector<double> plusvalues(numberOfInputComponents);
    std::vector<double> minusvalues(numberOfInputComponents);

    std::vector<double> dValuesdXi(numberOfInputComponents);
    std::vector<double> dValuesdEta(numberOfInputComponents);
    std::vector<double> dValuesdZeta(numberOfInputComponents);

    int dims[3];
    output->GetDimensions(dims);
    if(fieldAssociation == vtkDataObject::FIELD_ASSOCIATION_CELLS)
      {
      // reduce the dimensions by 1 for cells
      for(i=0;i<3;i++)
        {
        dims[i]--;
        }
      }
    int ijsize = dims[0]*dims[1];

    for (k=0; k<dims[2]; k++)
      {
      for (j=0; j<dims[1]; j++)
        {
        for (i=0; i<dims[0]; i++)
          {
          //  Xi derivatives.
          if ( dims[0] == 1 ) // 2D in this direction
            {
            factor = 1.0;
            for (ii=0; ii<3; ii++)
              {
              xp[ii] = xm[ii] = 0.0;
              }
            xp[0] = 1.0;
            for(inputComponent=0;inputComponent<numberOfInputComponents;
                inputComponent++)
              {
              plusvalues[inputComponent] = minusvalues[inputComponent] = 0;
              }
            }
          else if ( i == 0 )
            {
            factor = 1.0;
            idx = (i+1) + j*dims[0] + k*ijsize;
            idx2 = i + j*dims[0] + k*ijsize;
            GetGridEntityCoordinate(output, fieldAssociation, idx, xp);
            GetGridEntityCoordinate(output, fieldAssociation, idx2, xm);
            for(inputComponent=0;inputComponent<numberOfInputComponents;
                inputComponent++)
              {
              plusvalues[inputComponent] = array[idx*numberOfInputComponents+inputComponent];
              minusvalues[inputComponent] = array[idx2*numberOfInputComponents+inputComponent];
              }
            }
          else if ( i == (dims[0]-1) )
            {
            factor = 1.0;
            idx = i + j*dims[0] + k*ijsize;
            idx2 = i-1 + j*dims[0] + k*ijsize;
            GetGridEntityCoordinate(output, fieldAssociation, idx, xp);
            GetGridEntityCoordinate(output, fieldAssociation, idx2, xm);
            for(inputComponent=0;inputComponent<numberOfInputComponents;
                inputComponent++)
              {
              plusvalues[inputComponent] = array[idx*numberOfInputComponents+inputComponent];
              minusvalues[inputComponent] = array[idx2*numberOfInputComponents+inputComponent];
              }
            }
          else
            {
            factor = 0.5;
            idx = (i+1) + j*dims[0] + k*ijsize;
            idx2 = (i-1) + j*dims[0] + k*ijsize;
            GetGridEntityCoordinate(output, fieldAssociation, idx, xp);
            GetGridEntityCoordinate(output, fieldAssociation, idx2, xm);
            for(inputComponent=0;inputComponent<numberOfInputComponents;
                inputComponent++)
              {
              plusvalues[inputComponent] = array[idx*numberOfInputComponents+inputComponent];
              minusvalues[inputComponent] = array[idx2*numberOfInputComponents+inputComponent];
              }
            }

          xxi = factor * (xp[0] - xm[0]);
          yxi = factor * (xp[1] - xm[1]);
          zxi = factor * (xp[2] - xm[2]);
          for(inputComponent=0;inputComponent<numberOfInputComponents;inputComponent++)
            {
            dValuesdXi[inputComponent] = factor *
              (plusvalues[inputComponent] - minusvalues[inputComponent]);
            }

          //  Eta derivatives.
          if ( dims[1] == 1 ) // 2D in this direction
            {
            factor = 1.0;
            for (ii=0; ii<3; ii++)
              {
              xp[ii] = xm[ii] = 0.0;
              }
            xp[1] = 1.0;
            for(inputComponent=0;inputComponent<numberOfInputComponents;
                inputComponent++)
              {
              plusvalues[inputComponent] = minusvalues[inputComponent] = 0;
              }
            }
          else if ( j == 0 )
            {
            factor = 1.0;
            idx = i + (j+1)*dims[0] + k*ijsize;
            idx2 = i + j*dims[0] + k*ijsize;
            GetGridEntityCoordinate(output, fieldAssociation, idx, xp);
            GetGridEntityCoordinate(output, fieldAssociation, idx2, xm);
            for(inputComponent=0;inputComponent<numberOfInputComponents;
                inputComponent++)
              {
              plusvalues[inputComponent] = array[idx*numberOfInputComponents+inputComponent];
              minusvalues[inputComponent] = array[idx2*numberOfInputComponents+inputComponent];
              }
            }
          else if ( j == (dims[1]-1) )
            {
            factor = 1.0;
            idx = i + j*dims[0] + k*ijsize;
            idx2 = i + (j-1)*dims[0] + k*ijsize;
            GetGridEntityCoordinate(output, fieldAssociation, idx, xp);
            GetGridEntityCoordinate(output, fieldAssociation, idx2, xm);
            for(inputComponent=0;inputComponent<numberOfInputComponents;
                inputComponent++)
              {
              plusvalues[inputComponent] = array[idx*numberOfInputComponents+inputComponent];
              minusvalues[inputComponent] = array[idx2*numberOfInputComponents+inputComponent];
              }
            }
          else
            {
            factor = 0.5;
            idx = i + (j+1)*dims[0] + k*ijsize;
            idx2 = i + (j-1)*dims[0] + k*ijsize;
            GetGridEntityCoordinate(output, fieldAssociation, idx, xp);
            GetGridEntityCoordinate(output, fieldAssociation, idx2, xm);
            for(inputComponent=0;inputComponent<numberOfInputComponents;
                inputComponent++)
              {
              plusvalues[inputComponent] = array[idx*numberOfInputComponents+inputComponent];
              minusvalues[inputComponent] = array[idx2*numberOfInputComponents+inputComponent];
              }
            }

          xeta = factor * (xp[0] - xm[0]);
          yeta = factor * (xp[1] - xm[1]);
          zeta = factor * (xp[2] - xm[2]);
          for(inputComponent=0;inputComponent<numberOfInputComponents;inputComponent++)
            {
            dValuesdEta[inputComponent] = factor *
              (plusvalues[inputComponent] - minusvalues[inputComponent]);
            }

          //  Zeta derivatives.
          if ( dims[2] == 1 ) // 2D in this direction
            {
            factor = 1.0;
            for (ii=0; ii<3; ii++)
              {
              xp[ii] = xm[ii] = 0.0;
              }
            for(inputComponent=0;inputComponent<numberOfInputComponents;
                inputComponent++)
              {
              plusvalues[inputComponent] = minusvalues[inputComponent] = 0;
              }
            xp[2] = 1.0;
            }
          else if ( k == 0 )
            {
            factor = 1.0;
            idx = i + j*dims[0] + (k+1)*ijsize;
            idx2 = i + j*dims[0] + k*ijsize;
            GetGridEntityCoordinate(output, fieldAssociation, idx, xp);
            GetGridEntityCoordinate(output, fieldAssociation, idx2, xm);
            for(inputComponent=0;inputComponent<numberOfInputComponents;
                inputComponent++)
              {
              plusvalues[inputComponent] = array[idx*numberOfInputComponents+inputComponent];
              minusvalues[inputComponent] = array[idx2*numberOfInputComponents+inputComponent];
              }
            }
          else if ( k == (dims[2]-1) )
            {
            factor = 1.0;
            idx = i + j*dims[0] + k*ijsize;
            idx2 = i + j*dims[0] + (k-1)*ijsize;
            GetGridEntityCoordinate(output, fieldAssociation, idx, xp);
            GetGridEntityCoordinate(output, fieldAssociation, idx2, xm);
            for(inputComponent=0;inputComponent<numberOfInputComponents;
                inputComponent++)
              {
              plusvalues[inputComponent] = array[idx*numberOfInputComponents+inputComponent];
              minusvalues[inputComponent] = array[idx2*numberOfInputComponents+inputComponent];
              }
            }
          else
            {
            factor = 0.5;
            idx = i + j*dims[0] + (k+1)*ijsize;
            idx2 = i + j*dims[0] + (k-1)*ijsize;
            GetGridEntityCoordinate(output, fieldAssociation, idx, xp);
            GetGridEntityCoordinate(output, fieldAssociation, idx2, xm);
            for(inputComponent=0;inputComponent<numberOfInputComponents;
                inputComponent++)
              {
              plusvalues[inputComponent] = array[idx*numberOfInputComponents+inputComponent];
              minusvalues[inputComponent] = array[idx2*numberOfInputComponents+inputComponent];
              }
            }

          xzeta = factor * (xp[0] - xm[0]);
          yzeta = factor * (xp[1] - xm[1]);
          zzeta = factor * (xp[2] - xm[2]);
          for(inputComponent=0;inputComponent<numberOfInputComponents;inputComponent++)
            {
            dValuesdZeta[inputComponent] = factor *
              (plusvalues[inputComponent] - minusvalues[inputComponent]);
            }

          // Now calculate the Jacobian.  Grids occasionally have
          // singularities, or points where the Jacobian is infinite (the
          // inverse is zero).  For these cases, we'll set the Jacobian to
          // zero, which will result in a zero derivative.
          //
          aj =  xxi*yeta*zzeta+yxi*zeta*xzeta+zxi*xeta*yzeta
            -zxi*yeta*xzeta-yxi*xeta*zzeta-xxi*zeta*yzeta;
          if (aj != 0.0)
            {
            aj = 1. / aj;
            }

          //  Xi metrics.
          xix  =  aj*(yeta*zzeta-zeta*yzeta);
          xiy  = -aj*(xeta*zzeta-zeta*xzeta);
          xiz  =  aj*(xeta*yzeta-yeta*xzeta);

          //  Eta metrics.
          etax = -aj*(yxi*zzeta-zxi*yzeta);
          etay =  aj*(xxi*zzeta-zxi*xzeta);
          etaz = -aj*(xxi*yzeta-yxi*xzeta);

          //  Zeta metrics.
          zetax=  aj*(yxi*zeta-zxi*yeta);
          zetay= -aj*(xxi*zeta-zxi*xeta);
          zetaz=  aj*(xxi*yeta-yxi*xeta);

          // Finally compute the actual derivatives
          idx = i + j*dims[0] + k*ijsize;
          for(inputComponent=0;inputComponent<numberOfInputComponents;inputComponent++)
            {
            gradients[idx*numberOfInputComponents*3+inputComponent*3] = static_cast<data_type>(
              xix*dValuesdXi[inputComponent]+etax*dValuesdEta[inputComponent]+
              zetax*dValuesdZeta[inputComponent]);

            gradients[idx*numberOfInputComponents*3+inputComponent*3+1] = static_cast<data_type>(
              xiy*dValuesdXi[inputComponent]+etay*dValuesdEta[inputComponent]+
              zetay*dValuesdZeta[inputComponent]);

            gradients[idx*numberOfInputComponents*3+inputComponent*3+2] = static_cast<data_type>(
              xiz*dValuesdXi[inputComponent]+etaz*dValuesdEta[inputComponent]+
              zetaz*dValuesdZeta[inputComponent]);
            }

          if(vorticity)
            {
            ComputeVorticityFromGradient(gradients+idx*numberOfInputComponents*3, vorticity+3*idx);
            }
          if(qCriterion)
            {
            ComputeQCriterionFromGradient(gradients+idx*numberOfInputComponents*3, qCriterion+idx);
            }
          }
        }
      }
  }

} // end anonymous namespace
