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
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStructuredGrid.h"
#include "vtkUnstructuredGrid.h"

#include <vtkstd/vector>

//-----------------------------------------------------------------------------

vtkStandardNewMacro(vtkGradientFilter);

namespace 
{
  // helper function to replace the gradient of a vector 
  // with the vorticity/curl of that vector
//-----------------------------------------------------------------------------
  template<class data_type>
  void ReplaceGradientWithVorticity(data_type* Gradients)
  {
    Gradients[0] = Gradients[7] - Gradients[5];
    data_type tmp = Gradients[3] - Gradients[1]; //temp for Gradients[2] output
    Gradients[1] = Gradients[2] - Gradients[6];
    Gradients[2] = tmp;
  }

  // Functions for unstructured grids and polydatas
  template<class data_type>
  void ComputePointGradientsUG(
    vtkDataSet *structure, data_type *Array, data_type *gradients,
    int NumberOfInputComponents, int ComputeVorticity);

  int GetCellParametricData(
    vtkIdType pointId, double pointCoord[3], vtkCell *cell, int & subId, 
    double parametricCoord[3]);
  
  template<class data_type>
  void ComputeCellGradientsUG(
    vtkDataSet *structure, data_type *Array, data_type *gradients,
    int NumberOfInputComponents, int ComputeVorticity);

  // Functions for image data and structured grids
  template<class Grid, class data_type>
  void ComputeGradientsSG(Grid output, data_type* Array, data_type* gradients,
                          int NumberOfInputComponents, int fieldAssociation,
                          int ComputeVorticity);

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

  // generic way to get the coordinate for either a cell (using 
  // the parametric center) or a point
  void GetGridEntityCoordinate(vtkDataSet* Grid, int fieldAssociation, 
                               vtkIdType Index, double Coords[3])
  {
    if(fieldAssociation == vtkDataObject::FIELD_ASSOCIATION_POINTS)
      {
      Grid->GetPoint(Index, Coords);
      }
    else
      {
      vtkCell* Cell = Grid->GetCell(Index);
      double pcoords[3];
      int subId = Cell->GetParametricCenter(pcoords);
      vtkstd::vector<double> weights(Cell->GetNumberOfPoints());
      Cell->EvaluateLocation(subId, pcoords, Coords, &weights[0]);
      }
  }
} // end anonymous namespace

//-----------------------------------------------------------------------------
vtkGradientFilter::vtkGradientFilter()
{
  this->ResultArrayName = NULL;
  this->FasterApproximation = 0;
  this->ComputeVorticity = 0;
  this->SetInputScalars(vtkDataObject::FIELD_ASSOCIATION_POINTS_THEN_CELLS,
                        vtkDataSetAttributes::SCALARS);
}

//-----------------------------------------------------------------------------
vtkGradientFilter::~vtkGradientFilter()
{
  this->SetResultArrayName(NULL);
}

//-----------------------------------------------------------------------------
void vtkGradientFilter::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "ResultArrayName:"
     << (this->ResultArrayName ? this->ResultArrayName : "Gradients") << endl;
  os << indent << "FasterApproximation:" << this->FasterApproximation << endl;
  os << indent << "ComputeVorticity:" << this->ComputeVorticity << endl;
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

  vtkDataArray *Array = this->GetInputArrayToProcess(0, inputVector);
  
  if (Array == NULL)
    {
    vtkErrorMacro("No input array.");
    return 0;
    }
  if (Array->GetNumberOfComponents() == 0)
    {
    vtkErrorMacro("Input array must have at least one component.");
    return 0;
    }
  if(this->ComputeVorticity && Array->GetNumberOfComponents() != 3)
    {
    vtkErrorMacro("Input array must have exactly three components "
                  << "with ComputeVorticity flag turned on.");
    return 0;
    }

  int fieldAssociation;
  if (vtkGradientFilterHasArray(input->GetPointData(), Array))
    {
    fieldAssociation = vtkDataObject::FIELD_ASSOCIATION_POINTS;
    }
  else if (vtkGradientFilterHasArray(input->GetCellData(), Array))
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

  int retVal = 0;
  if(output->IsA("vtkImageData") || output->IsA("vtkStructuredGrid") ||
          output->IsA("vtkRectilinearGrid") )
    {
    retVal = this->ComputeRegularGridGradient(
      Array, fieldAssociation, output);
    }
  else
    {
    retVal = this->ComputeUnstructuredGridGradient(
      Array, fieldAssociation, input, output);
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
  vtkDataArray* Array, int fieldAssociation, vtkDataSet* input,
  vtkDataSet* output)
{
  vtkDataArray *gradients
    = vtkDataArray::CreateDataArray(Array->GetDataType());
  int NumberOfInputComponents = Array->GetNumberOfComponents();
  if(this->ComputeVorticity == 0)
    {
    gradients->SetNumberOfComponents(3*NumberOfInputComponents);
    }
  else
    {
    gradients->SetNumberOfComponents(3);
    }
  gradients->SetNumberOfTuples(Array->GetNumberOfTuples());
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
    if (!this->FasterApproximation)
      {
      switch (Array->GetDataType())
        {
        vtkTemplateMacro(ComputePointGradientsUG(
                           input,
                           static_cast<VTK_TT *>(Array->GetVoidPointer(0)),
                           static_cast<VTK_TT *>(gradients->GetVoidPointer(0)),
                           NumberOfInputComponents, this->ComputeVorticity));
        }

      output->GetPointData()->AddArray(gradients);
      }
    else // this->FasterApproximation
      {
      // The cell computation is faster and works off of point data anyway.  The
      // faster approximation is to use the cell algorithm and then convert the
      // result to point data.
      vtkDataArray *cellGradients
        = vtkDataArray::CreateDataArray(gradients->GetDataType());
      cellGradients->SetName(gradients->GetName());
      cellGradients->SetNumberOfComponents(3*Array->GetNumberOfComponents());
      cellGradients->SetNumberOfTuples(input->GetNumberOfCells());

      switch (Array->GetDataType())
        {
        vtkTemplateMacro(
          ComputeCellGradientsUG(
            input, static_cast<VTK_TT *>(Array->GetVoidPointer(0)),
            static_cast<VTK_TT *>(cellGradients->GetVoidPointer(0)),
            NumberOfInputComponents, this->ComputeVorticity));
        }

      // We need to convert cell Array to points Array.
      vtkDataSet *dummy = input->NewInstance();
      dummy->CopyStructure(input);
      dummy->GetCellData()->AddArray(cellGradients);

      vtkCellDataToPointData *cd2pd = vtkCellDataToPointData::New();
      cd2pd->SetInput(dummy);
      cd2pd->PassCellDataOff();
      cd2pd->Update();

      // Set the gradients array in the output and cleanup.
      vtkDataArray *pointGradients
        = cd2pd->GetOutput()->GetPointData()->GetArray(gradients->GetName());
      output->GetPointData()->AddArray(pointGradients);
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
    dummy->GetCellData()->SetScalars(Array);

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
      vtkTemplateMacro(ComputeCellGradientsUG(
                         input,
                         static_cast<VTK_TT *>(pointScalars->GetVoidPointer(0)),
                         static_cast<VTK_TT *>(gradients->GetVoidPointer(0)),
                         NumberOfInputComponents, this->ComputeVorticity));
      }

    output->GetCellData()->AddArray(gradients);
    pointScalars->UnRegister(this);
    }

  gradients->Delete();

  return 1;
}

//-----------------------------------------------------------------------------
int vtkGradientFilter::ComputeRegularGridGradient(
  vtkDataArray* Array, int fieldAssociation, vtkDataSet* output)
{
  vtkDataArray *gradients
    = vtkDataArray::CreateDataArray(Array->GetDataType());
  int NumberOfInputComponents = Array->GetNumberOfComponents();
  if(this->ComputeVorticity == 0)
    {
    gradients->SetNumberOfComponents(3*NumberOfInputComponents);
    }
  else
    {
    gradients->SetNumberOfComponents(3);
    }
  gradients->SetNumberOfTuples(Array->GetNumberOfTuples());
  if (this->ResultArrayName)
    {
    gradients->SetName(this->ResultArrayName);
    }
  else
    {
    gradients->SetName("Gradients");
    }

  vtkStructuredGrid* StructuredGrid = vtkStructuredGrid::SafeDownCast(output);
  vtkImageData* ImageData = vtkImageData::SafeDownCast(output);
  vtkRectilinearGrid* RectilinearGrid = vtkRectilinearGrid::SafeDownCast(output);
  if(StructuredGrid)
    {
    switch (Array->GetDataType())
      {
      vtkTemplateMacro(ComputeGradientsSG(
                         StructuredGrid,
                         static_cast<VTK_TT *>(Array->GetVoidPointer(0)),
                         static_cast<VTK_TT *>(gradients->GetVoidPointer(0)),
                         NumberOfInputComponents, fieldAssociation,
                         this->ComputeVorticity));
      }
    }
  else if(ImageData)
    {
    switch (Array->GetDataType())
      {
      vtkTemplateMacro(ComputeGradientsSG(
                         ImageData,
                         static_cast<VTK_TT *>(Array->GetVoidPointer(0)),
                         static_cast<VTK_TT *>(gradients->GetVoidPointer(0)),
                         NumberOfInputComponents, fieldAssociation,
                         this->ComputeVorticity));
      }
    }
  else if(RectilinearGrid)
    {
    switch (Array->GetDataType())
      {
      vtkTemplateMacro(ComputeGradientsSG(
                         RectilinearGrid,
                         static_cast<VTK_TT *>(Array->GetVoidPointer(0)),
                         static_cast<VTK_TT *>(gradients->GetVoidPointer(0)),
                         NumberOfInputComponents, fieldAssociation,
                         this->ComputeVorticity));
      }
    }
  int retVal = 1;
  if(fieldAssociation == vtkDataObject::FIELD_ASSOCIATION_POINTS)
    {
    output->GetPointData()->AddArray(gradients);
    }
  else if(fieldAssociation == vtkDataObject::FIELD_ASSOCIATION_CELLS)
    {
    output->GetCellData()->AddArray(gradients);
    }
  else
    {
    vtkErrorMacro("Bad fieldAssociation value " << fieldAssociation << endl);
    retVal = 0;
    }
  gradients->Delete();

  return 1;
}

namespace {
//-----------------------------------------------------------------------------
  template<class data_type>
  void ComputePointGradientsUG(
    vtkDataSet *structure, data_type *Array,
    data_type *gradients, int NumberOfInputComponents, int ComputeVorticity)
  {
    vtkIdList* currentPoint = vtkIdList::New();
    currentPoint->SetNumberOfIds(1);
    vtkIdList* cellsOnPoint = vtkIdList::New();
    
    vtkIdType numpts = structure->GetNumberOfPoints();
    vtkstd::vector<data_type> g(3*NumberOfInputComponents);

    int NumberOfOutputComponents = 3*NumberOfInputComponents;
    if(ComputeVorticity)
      {
      NumberOfOutputComponents = 3;
      }

    for (vtkIdType point = 0; point < numpts; point++)
      {
      currentPoint->SetId(0, point);
      double pointcoords[3];
      structure->GetPoint(point, pointcoords);
      // Get all cells touching this point.
      structure->GetCellNeighbors(-1, currentPoint, cellsOnPoint);
      vtkIdType numCellNeighbors = cellsOnPoint->GetNumberOfIds();
      vtkIdType numValidCellNeighbors = 0;

      for(int i=0;i<NumberOfInputComponents*3;i++)
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
          for(int InputComponent=0;InputComponent<NumberOfInputComponents;InputComponent++)
            {
            int NumberOfCellPoints = cell->GetNumberOfPoints();
            vtkstd::vector<double> values(NumberOfCellPoints);
            // Get values of Array at cell points.
            for (int i = 0; i < NumberOfCellPoints; i++)
              {
              values[i] = static_cast<double>(
                Array[cell->GetPointId(i)*NumberOfInputComponents+InputComponent]);
              }
            
            double derivative[3];
            // Get derivitive of cell at point.
            cell->Derivatives(subId, parametricCoord, &values[0], 1, derivative);
            
            g[InputComponent*3] += static_cast<data_type>(derivative[0]);
            g[InputComponent*3+1] += static_cast<data_type>(derivative[1]);
            g[InputComponent*3+2] += static_cast<data_type>(derivative[2]);
            } // iterating over Components
          } // if(GetCellParametricData())
        } // iterating over neighbors

      if (numCellNeighbors > 0)
        {
        for(int i=0;i<3*NumberOfInputComponents;i++)
          {
          g[i] /= numCellNeighbors;
          }
        }

      if(ComputeVorticity)
        {
        ReplaceGradientWithVorticity(&g[0]);
        }
      for(int i=0;i<NumberOfOutputComponents;i++)
        {
        gradients[point*NumberOfOutputComponents+i] = g[i];
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
    vtkstd::vector<double> values(numpoints);    
    // Get parametric position of point.
    cell->EvaluatePosition(pointCoord, NULL, subId, parametricCoord,
                           dummy, &values[0]/*Really another dummy.*/);
    
    return 1;
  }

//-----------------------------------------------------------------------------
  template<class data_type>
    void ComputeCellGradientsUG(
      vtkDataSet *structure, data_type *Array, data_type *gradients,
      int NumberOfInputComponents, int ComputeVorticity)
  {
    vtkIdType numcells = structure->GetNumberOfCells();
    int NumberOfOutputComponents = 3*NumberOfInputComponents;
    if(ComputeVorticity)
      {
      NumberOfOutputComponents = 3;
      }
    vtkstd::vector<data_type> g(3*NumberOfInputComponents);

    for (vtkIdType cellid = 0; cellid < numcells; cellid++)
      {
      vtkCell *cell = structure->GetCell(cellid);
      
      int subId;
      double cellCenter[3];
      subId = cell->GetParametricCenter(cellCenter);
      
      int numpoints = cell->GetNumberOfPoints();
      double derivative[3];
      vtkstd::vector<double> values(numpoints);
      for(int InputComponent=0;InputComponent<NumberOfInputComponents;
          InputComponent++)
        {
        for (int i = 0; i < numpoints; i++)
          {
          values[i] = static_cast<double>(
            Array[cell->GetPointId(i)*NumberOfInputComponents+InputComponent]);
          }
      
        cell->Derivatives(subId, cellCenter, &values[0], 1, derivative);
        g[InputComponent*3+0] = static_cast<data_type>(derivative[0]);
        g[InputComponent*3+1] = static_cast<data_type>(derivative[1]);
        g[InputComponent*3+2] = static_cast<data_type>(derivative[2]);
        }
      if(ComputeVorticity)
        {
        ReplaceGradientWithVorticity(&g[0]);
        }
      for(int i=0;i<NumberOfOutputComponents;i++)
        {
        gradients[cellid*NumberOfOutputComponents+i] = g[i];
        }
      }
  }

//-----------------------------------------------------------------------------
  template<class Grid, class data_type>
  void ComputeGradientsSG(Grid output, data_type* Array, data_type* gradients,
                          int NumberOfInputComponents, int fieldAssociation,
                          int ComputeVorticity)
  {
    int i, j, k, idx, idx2, ii, InputComponent;
    double xp[3], xm[3], factor;
    double xxi, yxi, zxi, xeta, yeta, zeta, xzeta, yzeta, zzeta;
    double aj, xix, xiy, xiz, etax, etay, etaz, zetax, zetay, zetaz;
    // for finite differencing -- the values on the "plus" side and
    // "minus" side of the point to be computed at
    vtkstd::vector<double> plusvalues(NumberOfInputComponents);
    vtkstd::vector<double> minusvalues(NumberOfInputComponents);

    vtkstd::vector<double> dValuesdXi(NumberOfInputComponents);
    vtkstd::vector<double> dValuesdEta(NumberOfInputComponents);
    vtkstd::vector<double> dValuesdZeta(NumberOfInputComponents);

    int NumberOfOutputComponents = 3*NumberOfInputComponents;
    if(ComputeVorticity)
      {
      NumberOfOutputComponents = 3;
      }
    vtkstd::vector<data_type> g(3*NumberOfInputComponents);

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
            for(InputComponent=0;InputComponent<NumberOfInputComponents;
                InputComponent++)
              {
              plusvalues[InputComponent] = minusvalues[InputComponent] = 0;
              }
            }
          else if ( i == 0 ) 
            {
            factor = 1.0;
            idx = (i+1) + j*dims[0] + k*ijsize;
            idx2 = i + j*dims[0] + k*ijsize;
            GetGridEntityCoordinate(output, fieldAssociation, idx, xp);
            GetGridEntityCoordinate(output, fieldAssociation, idx2, xm);
            for(InputComponent=0;InputComponent<NumberOfInputComponents;
                InputComponent++)
              {
              plusvalues[InputComponent] = Array[idx*NumberOfInputComponents+InputComponent];
              minusvalues[InputComponent] = Array[idx2*NumberOfInputComponents+InputComponent];
              }
            } 
          else if ( i == (dims[0]-1) ) 
            {
            factor = 1.0;
            idx = i + j*dims[0] + k*ijsize;
            idx2 = i-1 + j*dims[0] + k*ijsize;
            GetGridEntityCoordinate(output, fieldAssociation, idx, xp);
            GetGridEntityCoordinate(output, fieldAssociation, idx2, xm);
            for(InputComponent=0;InputComponent<NumberOfInputComponents;
                InputComponent++)
              {
              plusvalues[InputComponent] = Array[idx*NumberOfInputComponents+InputComponent];
              minusvalues[InputComponent] = Array[idx2*NumberOfInputComponents+InputComponent];
              }
            } 
          else 
            {
            factor = 0.5;
            idx = (i+1) + j*dims[0] + k*ijsize;
            idx2 = (i-1) + j*dims[0] + k*ijsize;
            GetGridEntityCoordinate(output, fieldAssociation, idx, xp);
            GetGridEntityCoordinate(output, fieldAssociation, idx2, xm);
            for(InputComponent=0;InputComponent<NumberOfInputComponents;
                InputComponent++)
              {
              plusvalues[InputComponent] = Array[idx*NumberOfInputComponents+InputComponent];
              minusvalues[InputComponent] = Array[idx2*NumberOfInputComponents+InputComponent];
              }
            }
          
          xxi = factor * (xp[0] - xm[0]);
          yxi = factor * (xp[1] - xm[1]);
          zxi = factor * (xp[2] - xm[2]);
          for(InputComponent=0;InputComponent<NumberOfInputComponents;InputComponent++)
            {
            dValuesdXi[InputComponent] = factor * 
              (plusvalues[InputComponent] - minusvalues[InputComponent]);
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
            for(InputComponent=0;InputComponent<NumberOfInputComponents;
                InputComponent++)
              {
              plusvalues[InputComponent] = minusvalues[InputComponent] = 0;
              }
            }
          else if ( j == 0 ) 
            {
            factor = 1.0;
            idx = i + (j+1)*dims[0] + k*ijsize;
            idx2 = i + j*dims[0] + k*ijsize;
            GetGridEntityCoordinate(output, fieldAssociation, idx, xp);
            GetGridEntityCoordinate(output, fieldAssociation, idx2, xm);
            for(InputComponent=0;InputComponent<NumberOfInputComponents;
                InputComponent++)
              {
              plusvalues[InputComponent] = Array[idx*NumberOfInputComponents+InputComponent];
              minusvalues[InputComponent] = Array[idx2*NumberOfInputComponents+InputComponent];
              }
            } 
          else if ( j == (dims[1]-1) ) 
            {
            factor = 1.0;
            idx = i + j*dims[0] + k*ijsize;
            idx2 = i + (j-1)*dims[0] + k*ijsize;
            GetGridEntityCoordinate(output, fieldAssociation, idx, xp);
            GetGridEntityCoordinate(output, fieldAssociation, idx2, xm);
            for(InputComponent=0;InputComponent<NumberOfInputComponents;
                InputComponent++)
              {
              plusvalues[InputComponent] = Array[idx*NumberOfInputComponents+InputComponent];
              minusvalues[InputComponent] = Array[idx2*NumberOfInputComponents+InputComponent];
              }
            } 
          else 
            {
            factor = 0.5;
            idx = i + (j+1)*dims[0] + k*ijsize;
            idx2 = i + (j-1)*dims[0] + k*ijsize;
            GetGridEntityCoordinate(output, fieldAssociation, idx, xp);
            GetGridEntityCoordinate(output, fieldAssociation, idx2, xm);
            for(InputComponent=0;InputComponent<NumberOfInputComponents;
                InputComponent++)
              {
              plusvalues[InputComponent] = Array[idx*NumberOfInputComponents+InputComponent];
              minusvalues[InputComponent] = Array[idx2*NumberOfInputComponents+InputComponent];
              }
            }

          xeta = factor * (xp[0] - xm[0]);
          yeta = factor * (xp[1] - xm[1]);
          zeta = factor * (xp[2] - xm[2]);
          for(InputComponent=0;InputComponent<NumberOfInputComponents;InputComponent++)
            {
            dValuesdEta[InputComponent] = factor * 
              (plusvalues[InputComponent] - minusvalues[InputComponent]);
            }
          
          //  Zeta derivatives.
          if ( dims[2] == 1 ) // 2D in this direction
            {
            factor = 1.0;
            for (ii=0; ii<3; ii++)
              {
              xp[ii] = xm[ii] = 0.0;
              }
            for(InputComponent=0;InputComponent<NumberOfInputComponents;
                InputComponent++)
              {
              plusvalues[InputComponent] = minusvalues[InputComponent] = 0;
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
            for(InputComponent=0;InputComponent<NumberOfInputComponents;
                InputComponent++)
              {
              plusvalues[InputComponent] = Array[idx*NumberOfInputComponents+InputComponent];
              minusvalues[InputComponent] = Array[idx2*NumberOfInputComponents+InputComponent];
              }
            } 
          else if ( k == (dims[2]-1) ) 
            {
            factor = 1.0;
            idx = i + j*dims[0] + k*ijsize;
            idx2 = i + j*dims[0] + (k-1)*ijsize;
            GetGridEntityCoordinate(output, fieldAssociation, idx, xp);
            GetGridEntityCoordinate(output, fieldAssociation, idx2, xm);
            for(InputComponent=0;InputComponent<NumberOfInputComponents;
                InputComponent++)
              {
              plusvalues[InputComponent] = Array[idx*NumberOfInputComponents+InputComponent];
              minusvalues[InputComponent] = Array[idx2*NumberOfInputComponents+InputComponent];
              }
            } 
          else 
            {
            factor = 0.5;
            idx = i + j*dims[0] + (k+1)*ijsize;
            idx2 = i + j*dims[0] + (k-1)*ijsize;
            GetGridEntityCoordinate(output, fieldAssociation, idx, xp);
            GetGridEntityCoordinate(output, fieldAssociation, idx2, xm);
            for(InputComponent=0;InputComponent<NumberOfInputComponents;
                InputComponent++)
              {
              plusvalues[InputComponent] = Array[idx*NumberOfInputComponents+InputComponent];
              minusvalues[InputComponent] = Array[idx2*NumberOfInputComponents+InputComponent];
              }
            }
          
          xzeta = factor * (xp[0] - xm[0]);
          yzeta = factor * (xp[1] - xm[1]);
          zzeta = factor * (xp[2] - xm[2]);
          for(InputComponent=0;InputComponent<NumberOfInputComponents;InputComponent++)
            {
            dValuesdZeta[InputComponent] = factor * 
              (plusvalues[InputComponent] - minusvalues[InputComponent]);
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
          for(InputComponent=0;InputComponent<NumberOfInputComponents;InputComponent++)
            {

            g[InputComponent*3] = static_cast<data_type>(
              xix*dValuesdXi[InputComponent]+etax*dValuesdEta[InputComponent]+
              zetax*dValuesdZeta[InputComponent]);

            g[InputComponent*3+1] = static_cast<data_type>(
              xiy*dValuesdXi[InputComponent]+etay*dValuesdEta[InputComponent]+
              zetay*dValuesdZeta[InputComponent]);
            
            g[InputComponent*3+2] = static_cast<data_type>(
              xiz*dValuesdXi[InputComponent]+etaz*dValuesdEta[InputComponent]+
              zetaz*dValuesdZeta[InputComponent]);
            }

          if(ComputeVorticity)
            {
            ReplaceGradientWithVorticity(&g[0]);
            }
          for(int Component=0;Component<NumberOfOutputComponents;Component++)
            {
            gradients[idx*NumberOfOutputComponents+Component] = g[Component];
            }  
          }
        }
      } 
  }

} // end anonymous namespace
