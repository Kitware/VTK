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
#include "vtkContourGrid.h"

#include "vtkCell.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkCellIterator.h"
#include "vtkContourValues.h"
#include "vtkFloatArray.h"
#include "vtkGenericCell.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkPolyDataNormals.h"
#include "vtkSimpleScalarTree.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkUnstructuredGridBase.h"
#include "vtkCutter.h"
#include "vtkMergePoints.h"
#include "vtkPointLocator.h"
#include "vtkIncrementalPointLocator.h"
#include "vtkContourHelper.h"
#include <math.h>

vtkStandardNewMacro(vtkContourGrid);

// Construct object with initial range (0,1) and single contour value
// of 0.0.
vtkContourGrid::vtkContourGrid()
{
  this->ContourValues = vtkContourValues::New();

  this->ComputeNormals = 0;
  this->ComputeGradients = 0;
  this->ComputeScalars = 1;
  this->GenerateTriangles = 1;

  this->Locator = NULL;

  this->UseScalarTree = 0;
  this->ScalarTree = NULL;

  this->OutputPointsPrecision = DEFAULT_PRECISION;

  // by default process active point scalars
  this->SetInputArrayToProcess(0,0,0,vtkDataObject::FIELD_ASSOCIATION_POINTS,
                               vtkDataSetAttributes::SCALARS);
}

vtkContourGrid::~vtkContourGrid()
{
  this->ContourValues->Delete();
  if ( this->Locator )
    {
    this->Locator->UnRegister(this);
    this->Locator = NULL;
    }
  if ( this->ScalarTree )
    {
    this->ScalarTree->Delete();
    }
}

// Overload standard modified time function. If contour values are modified,
// then this object is modified as well.
unsigned long vtkContourGrid::GetMTime()
{
  unsigned long mTime=this->Superclass::GetMTime();
  unsigned long time;

  if (this->ContourValues)
    {
    time = this->ContourValues->GetMTime();
    mTime = ( time > mTime ? time : mTime );
    }
  if (this->Locator)
    {
    time = this->Locator->GetMTime();
    mTime = ( time > mTime ? time : mTime );
    }

  return mTime;
}

template <class Scalar>
void vtkContourGridExecute(vtkContourGrid *self, vtkDataSet *input,
                           vtkPolyData *output,
                           vtkDataArray *inScalars,
                           int numContours, double *values,
                           int computeScalars,
                           int useScalarTree, vtkScalarTree *&scalarTree,
                           bool generateTriangles)
{
  vtkIdType i;
  int abortExecute=0;
  vtkIncrementalPointLocator *locator = self->GetLocator();
  vtkNew<vtkGenericCell> cell;
  Scalar range[2];
  vtkCellArray *newVerts, *newLines, *newPolys;
  vtkPoints *newPts;
  vtkIdType numCells, estimatedSize;
  vtkPointData *inPd=input->GetPointData(), *outPd=output->GetPointData();
  vtkCellData *inCd=input->GetCellData(), *outCd=output->GetCellData();
  vtkDataArray *cellScalars;
  Scalar *cellScalarPtr;
  vtkIdType numCellScalars;
  //In this case, we know that the input is an unstructured grid.
  vtkUnstructuredGridBase *grid = static_cast<vtkUnstructuredGridBase *>(input);
  int needCell = 0;
  vtkSmartPointer<vtkCellIterator> cellIter =
      vtkSmartPointer<vtkCellIterator>::Take(input->NewCellIterator());

  numCells = input->GetNumberOfCells();

  //
  // Create objects to hold output of contour operation. First estimate
  // allocation size.
  //
  estimatedSize=static_cast<vtkIdType>(pow(static_cast<double>(numCells),.75));
  estimatedSize *= numContours;
  estimatedSize = estimatedSize / 1024 * 1024; //multiple of 1024
  if (estimatedSize < 1024)
    {
    estimatedSize = 1024;
    }

  newPts = vtkPoints::New();

  // set precision for the points in the output
  if(self->GetOutputPointsPrecision() == vtkAlgorithm::DEFAULT_PRECISION)
    {
    newPts->SetDataType(grid->GetPoints()->GetDataType());
    }
  else if(self->GetOutputPointsPrecision() == vtkAlgorithm::SINGLE_PRECISION)
    {
    newPts->SetDataType(VTK_FLOAT);
    }
  else if(self->GetOutputPointsPrecision() == vtkAlgorithm::DOUBLE_PRECISION)
    {
    newPts->SetDataType(VTK_DOUBLE);
    }

  newPts->Allocate(estimatedSize,estimatedSize);
  newVerts = vtkCellArray::New();
  newVerts->Allocate(estimatedSize,estimatedSize);
  newLines = vtkCellArray::New();
  newLines->Allocate(estimatedSize,estimatedSize);
  newPolys = vtkCellArray::New();
  newPolys->Allocate(estimatedSize,estimatedSize);
  cellScalars = inScalars->NewInstance();
  cellScalars->SetNumberOfComponents(inScalars->GetNumberOfComponents());
  cellScalars->Allocate(VTK_CELL_SIZE*inScalars->GetNumberOfComponents());

   // locator used to merge potentially duplicate points
  locator->InitPointInsertion (newPts, input->GetBounds(),
                               input->GetNumberOfPoints());

  // interpolate data along edge
  // if we did not ask for scalars to be computed, don't copy them
  if (!computeScalars)
    {
    outPd->CopyScalarsOff();
    }
  outPd->InterpolateAllocate(inPd,estimatedSize,estimatedSize);
  outCd->CopyAllocate(inCd,estimatedSize,estimatedSize);

  vtkContourHelper helper(locator, newVerts, newLines, newPolys, inPd, inCd,
                          outPd, outCd, estimatedSize, generateTriangles);
  // If enabled, build a scalar tree to accelerate search
  //
  if ( !useScalarTree )
    {
    // Three passes over the cells to process lower dimensional cells first.
    // For poly data output cells need to be added in the order:
    // verts, lines and then polys, or cell data gets mixed up.
    // A better solution is to have an unstructured grid output.
    // I create a table that maps cell type to cell dimensionality,
    // because I need a fast way to get cell dimensionality.
    // This assumes GetCell is slow and GetCellType is fast.
    // I do not like hard coding a list of cell types here,
    // but I do not want to add GetCellDimension(vtkIdType cellId)
    // to the vtkDataSet API.  Since I anticipate that the output
    // will change to vtkUnstructuredGrid.  This temporary solution
    // is acceptable.
    //
    int cellType;
    unsigned char cellTypeDimensions[VTK_NUMBER_OF_CELL_TYPES];
    vtkCutter::GetCellTypeDimensions(cellTypeDimensions);
    int dimensionality;
    // We skip 0d cells (points), because they cannot be cut (generate no data).
    for (dimensionality = 1; dimensionality <= 3; ++dimensionality)
      {
      // Loop over all cells; get scalar values for all cell points
      // and process each cell.
      //
      for (cellIter->InitTraversal(); !cellIter->IsDoneWithTraversal();
           cellIter->GoToNextCell())
        {
        if (abortExecute)
          {
          break;
          }

        cellType = cellIter->GetCellType();
        if (cellType >= VTK_NUMBER_OF_CELL_TYPES)
          { // Protect against new cell types added.
          vtkGenericWarningMacro("Unknown cell type " << cellType);
          continue;
          }
        if (cellTypeDimensions[cellType] != dimensionality)
          {
          continue;
          }

        cellScalars->SetNumberOfTuples(cellIter->GetNumberOfPoints());
        inScalars->GetTuples(cellIter->GetPointIds(), cellScalars);
        numCellScalars = cellScalars->GetNumberOfComponents()
            * cellScalars->GetNumberOfTuples();
        cellScalarPtr = static_cast<Scalar*>(cellScalars->GetVoidPointer(0));

        //find min and max values in scalar data
        range[0] = range[1] = cellScalarPtr[0];

        for (Scalar *it = cellScalarPtr + 1,
             *itEnd = cellScalarPtr + numCellScalars; it != itEnd; ++it)
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

        if (dimensionality == 3 &&  ! (cellIter->GetCellId() % 5000) )
          {
          self->UpdateProgress(static_cast<double>(cellIter->GetCellId())
                               / numCells);
          if (self->GetAbortExecute())
            {
            abortExecute = 1;
            break;
            }
          }

        for (i = 0; i < numContours; i++)
          {
          if ((values[i] >= range[0]) && (values[i] <= range[1]))
            {
            needCell = 1;
            } // if contour value in range for this cell
          } // end for numContours

        if (needCell)
          {
          cellIter->GetCell(cell.GetPointer());

          for (i=0; i < numContours; i++)
            {
            if ((values[i] >= range[0]) && (values[i] <= range[1]))
              {
              helper.Contour(cell.GetPointer(), values[i], cellScalars,
                             cellIter->GetCellId());
              } // if contour value in range of values for this cell
            } // for all contour values
          } // if contour goes through this cell
        needCell = 0;
        } // for all cells
      } // For all dimensions.
    } //if using scalar tree
  else
    {
    // Note: This will have problems when input contains 2D and 3D cells.
    // CellData will get scrabled because of the implicit ordering of
    // verts, lines and polys in vtkPolyData.  The solution
    // is to convert this filter to create unstructured grid.
    //
    if ( scalarTree == NULL )
      {
      scalarTree = vtkSimpleScalarTree::New();
      }
    scalarTree->SetDataSet(input);
    //
    // Loop over all contour values.  Then for each contour value,
    // loop over all cells.
    //
    vtkCell *tmpCell;
    vtkIdList *dummyIdList = NULL;
    vtkIdType cellId = cellIter->GetCellId();
    for (i=0; i < numContours; i++)
      {
      for (scalarTree->InitTraversal(values[i]);
          (tmpCell = scalarTree->GetNextCell(cellId, dummyIdList,
                                             cellScalars)); )
        {
        helper.Contour(tmpCell, values[i], cellScalars, cellId);

        //don't want to call Contour any more than necessary
        } //for all cells
      } //for all contour values
    } //using scalar tree

  //
  // Update ourselves.  Because we don't know up front how many verts, lines,
  // polys we've created, take care to reclaim memory.
  //
  output->SetPoints(newPts);
  newPts->Delete();
  cellScalars->Delete();

  if (newVerts->GetNumberOfCells())
    {
    output->SetVerts(newVerts);
    }
  newVerts->Delete();

  if (newLines->GetNumberOfCells())
    {
    output->SetLines(newLines);
    }
  newLines->Delete();

  if (newPolys->GetNumberOfCells())
    {
    output->SetPolys(newPolys);
    }
  newPolys->Delete();

  locator->Initialize();//releases leftover memory
  output->Squeeze();
}

//
// Contouring filter for unstructured grids.
//
int vtkContourGrid::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkUnstructuredGridBase *input = vtkUnstructuredGridBase::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData *output = vtkPolyData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkDataArray *inScalars;
  vtkIdType numCells;
  int numContours = this->ContourValues->GetNumberOfContours();
  double *values = this->ContourValues->GetValues();
  int computeScalars = this->ComputeScalars;
  int useScalarTree = this->UseScalarTree;
  vtkScalarTree *&scalarTree = this->ScalarTree;

  vtkDebugMacro(<< "Executing contour filter");

  if ( this->Locator == NULL )
    {
    this->CreateDefaultLocator();
    }

  numCells = input->GetNumberOfCells();
  inScalars = this->GetInputArrayToProcess(0,inputVector);
  if ( ! inScalars || numCells < 1 )
    {
    vtkDebugMacro(<<"No data to contour");
    return 1;
    }

  switch (inScalars->GetDataType())
    {
    vtkTemplateMacro(vtkContourGridExecute<VTK_TT>(
            this, input, output, inScalars, numContours, values,
            computeScalars, useScalarTree, scalarTree,
            this->GenerateTriangles != 0));
    default:
      vtkErrorMacro(<< "Execute: Unknown ScalarType");
      return 1;
    }

  if(this->ComputeNormals)
    {
    vtkInformation* info = outputVector->GetInformationObject(0);
    vtkNew<vtkPolyDataNormals> normalsFilter;
    normalsFilter->SetOutputPointsPrecision(this->OutputPointsPrecision);
    vtkNew<vtkPolyData> tempInput;
    tempInput->ShallowCopy(output);
    normalsFilter->SetInputData(tempInput.GetPointer());
    normalsFilter->SetFeatureAngle(180.);
    normalsFilter->SetUpdateExtent(
      0,
      info->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()),
      info->Get(vtkStreamingDemandDrivenPipeline:: UPDATE_NUMBER_OF_PIECES()),
      info->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS()));
    normalsFilter->Update();
    output->ShallowCopy(normalsFilter->GetOutput());
    }

  return 1;
}

// Specify a spatial locator for merging points. By default,
// an instance of vtkMergePoints is used.
void vtkContourGrid::SetLocator(vtkIncrementalPointLocator *locator)
{
  if ( this->Locator == locator )
    {
    return;
    }
  if ( this->Locator )
    {
    this->Locator->UnRegister(this);
    this->Locator = NULL;
    }
  if ( locator )
    {
    locator->Register(this);
    }
  this->Locator = locator;
  this->Modified();
}

void vtkContourGrid::CreateDefaultLocator()
{
  if ( this->Locator == NULL )
    {
    this->Locator = vtkMergePoints::New();
    this->Locator->Register(this);
    this->Locator->Delete();
    }
}

void vtkContourGrid::SetOutputPointsPrecision(int precision)
{
  this->OutputPointsPrecision = precision;
  this->Modified();
}

int vtkContourGrid::GetOutputPointsPrecision() const
{
  return this->OutputPointsPrecision;
}

int vtkContourGrid::FillInputPortInformation(int, vtkInformation *info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(),
            "vtkUnstructuredGridBase");
  return 1;
}

void vtkContourGrid::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Compute Gradients: "
     << (this->ComputeGradients ? "On\n" : "Off\n");
  os << indent << "Compute Normals: "
     << (this->ComputeNormals ? "On\n" : "Off\n");
  os << indent << "Compute Scalars: "
     << (this->ComputeScalars ? "On\n" : "Off\n");
  os << indent << "Use Scalar Tree: "
     << (this->UseScalarTree ? "On\n" : "Off\n");

  this->ContourValues->PrintSelf(os,indent.GetNextIndent());

  if ( this->Locator )
    {
    os << indent << "Locator: " << this->Locator << "\n";
    }
  else
    {
    os << indent << "Locator: (none)\n";
    }

  os << indent << "Precision of the output points: "
     << this->OutputPointsPrecision << "\n";
}
