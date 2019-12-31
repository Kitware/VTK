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
#include "vtkContourHelper.h"
#include "vtkContourValues.h"
#include "vtkCutter.h"
#include "vtkDataArrayRange.h"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkGenericCell.h"
#include "vtkIncrementalPointLocator.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMergePoints.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPointLocator.h"
#include "vtkPolyData.h"
#include "vtkPolyDataNormals.h"
#include "vtkSimpleScalarTree.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkUnstructuredGridBase.h"

#include <algorithm>
#include <cmath>
#include <limits>

vtkStandardNewMacro(vtkContourGrid);

//-----------------------------------------------------------------------------
// Construct object with initial range (0,1) and single contour value
// of 0.0.
vtkContourGrid::vtkContourGrid()
{
  this->ContourValues = vtkContourValues::New();

  this->ComputeNormals = 0;
#ifndef VTK_LEGACY_REMOVE
  this->ComputeGradients = 0;
#endif
  this->ComputeScalars = 1;
  this->GenerateTriangles = 1;

  this->Locator = nullptr;

  this->UseScalarTree = 0;
  this->ScalarTree = nullptr;

  this->OutputPointsPrecision = DEFAULT_PRECISION;

  // by default process active point scalars
  this->SetInputArrayToProcess(
    0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, vtkDataSetAttributes::SCALARS);

  this->EdgeTable = nullptr;
}

//-----------------------------------------------------------------------------
vtkContourGrid::~vtkContourGrid()
{
  this->ContourValues->Delete();
  if (this->Locator)
  {
    this->Locator->UnRegister(this);
    this->Locator = nullptr;
  }
  if (this->ScalarTree)
  {
    this->ScalarTree->Delete();
  }
}

//-----------------------------------------------------------------------------
// Overload standard modified time function. If contour values are modified,
// then this object is modified as well.
vtkMTimeType vtkContourGrid::GetMTime()
{
  vtkMTimeType mTime = this->Superclass::GetMTime();
  vtkMTimeType time;

  if (this->ContourValues)
  {
    time = this->ContourValues->GetMTime();
    mTime = (time > mTime ? time : mTime);
  }
  if (this->Locator)
  {
    time = this->Locator->GetMTime();
    mTime = (time > mTime ? time : mTime);
  }

  return mTime;
}

//-----------------------------------------------------------------------------
void vtkContourGridExecute(vtkContourGrid* self, vtkDataSet* input, vtkPolyData* output,
  vtkDataArray* inScalars, vtkIdType numContours, double* values, int computeScalars,
  int useScalarTree, vtkScalarTree* scalarTree, bool generateTriangles)
{
  vtkIdType i;
  int abortExecute = 0;
  vtkIncrementalPointLocator* locator = self->GetLocator();
  vtkNew<vtkGenericCell> cell;
  vtkCellArray *newVerts, *newLines, *newPolys;
  vtkPoints* newPts;
  vtkIdType numCells, estimatedSize;
  vtkNew<vtkDoubleArray> cellScalars;

  vtkPointData* inPdOriginal = input->GetPointData();

  // We don't want to change the active scalars in the input, but we
  // need to set the active scalars to match the input array to
  // process so that the point data copying works as expected. Create
  // a shallow copy of point data so that we can do this without
  // changing the input.
  vtkSmartPointer<vtkPointData> inPd = vtkSmartPointer<vtkPointData>::New();
  inPd->ShallowCopy(inPdOriginal);

  // Keep track of the old active scalars because when we set the new
  // scalars, the old scalars are removed from the point data entirely
  // and we have to add them back.
  vtkAbstractArray* oldScalars = inPd->GetScalars();
  inPd->SetScalars(inScalars);
  if (oldScalars)
  {
    inPd->AddArray(oldScalars);
  }
  vtkPointData* outPd = output->GetPointData();

  vtkCellData* inCd = input->GetCellData();
  vtkCellData* outCd = output->GetCellData();

  // In this case, we know that the input is an unstructured grid.
  vtkUnstructuredGridBase* grid = static_cast<vtkUnstructuredGridBase*>(input);
  int needCell = 0;
  vtkSmartPointer<vtkCellIterator> cellIter =
    vtkSmartPointer<vtkCellIterator>::Take(input->NewCellIterator());

  numCells = input->GetNumberOfCells();

  //
  // Create objects to hold output of contour operation. First estimate
  // allocation size.
  //
  estimatedSize = static_cast<vtkIdType>(pow(static_cast<double>(numCells), .75));
  estimatedSize *= numContours;
  estimatedSize = estimatedSize / 1024 * 1024; // multiple of 1024
  if (estimatedSize < 1024)
  {
    estimatedSize = 1024;
  }

  newPts = vtkPoints::New();

  // set precision for the points in the output
  if (self->GetOutputPointsPrecision() == vtkAlgorithm::DEFAULT_PRECISION)
  {
    newPts->SetDataType(grid->GetPoints()->GetDataType());
  }
  else if (self->GetOutputPointsPrecision() == vtkAlgorithm::SINGLE_PRECISION)
  {
    newPts->SetDataType(VTK_FLOAT);
  }
  else if (self->GetOutputPointsPrecision() == vtkAlgorithm::DOUBLE_PRECISION)
  {
    newPts->SetDataType(VTK_DOUBLE);
  }

  newPts->Allocate(estimatedSize, estimatedSize);
  newVerts = vtkCellArray::New();
  newVerts->AllocateEstimate(estimatedSize, 1);
  newLines = vtkCellArray::New();
  newLines->AllocateEstimate(estimatedSize, 2);
  newPolys = vtkCellArray::New();
  newPolys->AllocateEstimate(estimatedSize, 4);
  cellScalars->SetNumberOfComponents(inScalars->GetNumberOfComponents());
  cellScalars->Allocate(VTK_CELL_SIZE * inScalars->GetNumberOfComponents());

  // locator used to merge potentially duplicate points
  locator->InitPointInsertion(newPts, input->GetBounds(), input->GetNumberOfPoints());

  // interpolate data along edge
  // if we did not ask for scalars to be computed, don't copy them
  if (!computeScalars)
  {
    outPd->CopyScalarsOff();
  }
  outPd->InterpolateAllocate(inPd, estimatedSize, estimatedSize);
  outCd->CopyAllocate(inCd, estimatedSize, estimatedSize);

  vtkContourHelper helper(locator, newVerts, newLines, newPolys, inPd, inCd, outPd, outCd,
    estimatedSize, generateTriangles);
  // If enabled, build a scalar tree to accelerate search
  //
  vtkIdType numCellsContoured = 0;
  if (!useScalarTree)
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
    const int numComps = cellScalars->GetNumberOfComponents();
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
      for (cellIter->InitTraversal(); !cellIter->IsDoneWithTraversal(); cellIter->GoToNextCell())
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

        double range[2] = { std::numeric_limits<double>::max(),
          std::numeric_limits<double>::lowest() };

        if (numComps == 1)
        { // fast path:
          for (const double val : vtk::DataArrayValueRange<1>(cellScalars))
          {
            range[0] = std::min(range[0], val);
            range[1] = std::max(range[1], val);
          }
        }
        else
        {
          for (const double val : vtk::DataArrayValueRange(cellScalars))
          {
            range[0] = std::min(range[0], val);
            range[1] = std::max(range[1], val);
          }
        }

        if (dimensionality == 3 && !(cellIter->GetCellId() % 5000))
        {
          self->UpdateProgress(static_cast<double>(cellIter->GetCellId()) / numCells);
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
        }   // end for numContours

        if (needCell)
        {
          cellIter->GetCell(cell);
          vtkIdType cellId = cellIter->GetCellId();
          input->SetCellOrderAndRationalWeights(cellId, cell);
          for (i = 0; i < numContours; i++)
          {
            if ((values[i] >= range[0]) && (values[i] <= range[1]))
            {
              helper.Contour(cell, values[i], cellScalars, cellIter->GetCellId());
            } // if contour value in range of values for this cell
          }   // for all contour values
        }     // if contour goes through this cell
        needCell = 0;
      } // for all cells
    }   // For all dimensions.
  }     // if using scalar tree
  else
  {
    // Note: This will have problems when input contains 2D and 3D cells.
    // CellData will get scrambled because of the implicit ordering of
    // verts, lines and polys in vtkPolyData.  The solution
    // is to convert this filter to create unstructured grid.
    //

    //
    // Loop over all contour values.  Then for each contour value,
    // loop over all cells.
    //
    vtkCell* tmpCell;
    vtkIdList* dummyIdList = nullptr;
    vtkIdType cellId = cellIter->GetCellId();
    for (i = 0; i < numContours; i++)
    {
      for (scalarTree->InitTraversal(values[i]);
           (tmpCell = scalarTree->GetNextCell(cellId, dummyIdList, cellScalars));)
      {
        helper.Contour(tmpCell, values[i], cellScalars, cellId);
        numCellsContoured++;

        // don't want to call Contour any more than necessary
      } // for all cells
    }   // for all contour values
  }     // using scalar tree

  //
  // Update ourselves.  Because we don't know up front how many verts, lines,
  // polys we've created, take care to reclaim memory.
  //
  output->SetPoints(newPts);
  newPts->Delete();

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

  locator->Initialize(); // releases leftover memory
  output->Squeeze();
}

//-----------------------------------------------------------------------------
// Contouring filter for unstructured grids.
//
int vtkContourGrid::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkUnstructuredGridBase* input =
    vtkUnstructuredGridBase::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData* output = vtkPolyData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkDataArray* inScalars;
  vtkIdType numCells;
  vtkIdType numContours = this->ContourValues->GetNumberOfContours();
  double* values = this->ContourValues->GetValues();
  int computeScalars = this->ComputeScalars;

  vtkDebugMacro(<< "Executing contour filter");

  if (this->Locator == nullptr)
  {
    this->CreateDefaultLocator();
  }

  numCells = input->GetNumberOfCells();
  inScalars = this->GetInputArrayToProcess(0, inputVector);
  if (!inScalars || numCells < 1)
  {
    vtkDebugMacro(<< "No data to contour");
    return 1;
  }

  // Create scalar tree if necessary and if requested
  int useScalarTree = this->GetUseScalarTree();
  vtkScalarTree* scalarTree = this->ScalarTree;
  if (useScalarTree)
  {
    if (scalarTree == nullptr)
    {
      this->ScalarTree = scalarTree = vtkSimpleScalarTree::New();
    }
    scalarTree->SetDataSet(input);
    scalarTree->SetScalars(inScalars);
  }

  vtkContourGridExecute(this, input, output, inScalars, numContours, values, computeScalars,
    useScalarTree, scalarTree, this->GenerateTriangles != 0);

  if (this->ComputeNormals)
  {
    vtkInformation* info = outputVector->GetInformationObject(0);
    vtkNew<vtkPolyDataNormals> normalsFilter;
    normalsFilter->SetOutputPointsPrecision(this->OutputPointsPrecision);
    vtkNew<vtkPolyData> tempInput;
    tempInput->ShallowCopy(output);
    normalsFilter->SetInputData(tempInput);
    normalsFilter->SetFeatureAngle(180.);
    normalsFilter->UpdatePiece(info->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()),
      info->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES()),
      info->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS()));
    output->ShallowCopy(normalsFilter->GetOutput());
  }

  return 1;
}

//-----------------------------------------------------------------------------
// Specify a spatial locator for merging points. By default,
// an instance of vtkMergePoints is used.
void vtkContourGrid::SetScalarTree(vtkScalarTree* sTree)
{
  if (this->ScalarTree == sTree)
  {
    return;
  }
  if (this->ScalarTree)
  {
    this->ScalarTree->UnRegister(this);
    this->ScalarTree = nullptr;
  }
  if (sTree)
  {
    sTree->Register(this);
  }
  this->ScalarTree = sTree;
  this->Modified();
}

//-----------------------------------------------------------------------------
// Specify a spatial locator for merging points. By default,
// an instance of vtkMergePoints is used.
void vtkContourGrid::SetLocator(vtkIncrementalPointLocator* locator)
{
  if (this->Locator == locator)
  {
    return;
  }
  if (this->Locator)
  {
    this->Locator->UnRegister(this);
    this->Locator = nullptr;
  }
  if (locator)
  {
    locator->Register(this);
  }
  this->Locator = locator;
  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkContourGrid::CreateDefaultLocator()
{
  if (this->Locator == nullptr)
  {
    this->Locator = vtkMergePoints::New();
    this->Locator->Register(this);
    this->Locator->Delete();
  }
}

//-----------------------------------------------------------------------------
void vtkContourGrid::SetOutputPointsPrecision(int precision)
{
  this->OutputPointsPrecision = precision;
  this->Modified();
}

//-----------------------------------------------------------------------------
int vtkContourGrid::GetOutputPointsPrecision() const
{
  return this->OutputPointsPrecision;
}

//-----------------------------------------------------------------------------
int vtkContourGrid::FillInputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkUnstructuredGridBase");
  return 1;
}

//-----------------------------------------------------------------------------
void vtkContourGrid::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

#ifndef VTK_LEGACY_REMOVE
  os << indent << "Compute Gradients: " << (this->ComputeGradients ? "On\n" : "Off\n");
#endif
  os << indent << "Compute Normals: " << (this->ComputeNormals ? "On\n" : "Off\n");
  os << indent << "Compute Scalars: " << (this->ComputeScalars ? "On\n" : "Off\n");
  os << indent << "Use Scalar Tree: " << (this->UseScalarTree ? "On\n" : "Off\n");

  this->ContourValues->PrintSelf(os, indent.GetNextIndent());

  if (this->ScalarTree)
  {
    os << indent << "Scalar Tree: " << this->ScalarTree << "\n";
  }
  else
  {
    os << indent << "Scalar Tree: (none)\n";
  }

  if (this->Locator)
  {
    os << indent << "Locator: " << this->Locator << "\n";
  }
  else
  {
    os << indent << "Locator: (none)\n";
  }

  os << indent << "Precision of the output points: " << this->OutputPointsPrecision << "\n";
}
