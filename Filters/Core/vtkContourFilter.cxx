// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkContourFilter.h"

#include "vtkCallbackCommand.h"
#include "vtkCell.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkContour3DLinearGrid.h"
#include "vtkContourGrid.h"
#include "vtkContourHelper.h"
#include "vtkContourValues.h"
#include "vtkCutter.h"
#include "vtkFlyingEdges2D.h"
#include "vtkFlyingEdges3D.h"
#include "vtkGarbageCollector.h"
#include "vtkGenericCell.h"
#include "vtkGridSynchronizedTemplates3D.h"
#include "vtkImageData.h"
#include "vtkIncrementalPointLocator.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMergePoints.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkPolyDataNormals.h"
#include "vtkRectilinearGrid.h"
#include "vtkRectilinearSynchronizedTemplates.h"
#include "vtkSpanSpace.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStructuredGrid.h"
#include "vtkSynchronizedTemplates2D.h"
#include "vtkSynchronizedTemplates3D.h"
#include "vtkUniformGrid.h"
#include "vtkUnstructuredGrid.h"

#include <cmath>

VTK_ABI_NAMESPACE_BEGIN
vtkObjectFactoryNewMacro(vtkContourFilter);
vtkCxxSetObjectMacro(vtkContourFilter, ScalarTree, vtkScalarTree);

//------------------------------------------------------------------------------
// Construct object with initial range (0,1) and single contour value
// of 0.0.
vtkContourFilter::vtkContourFilter()
{
  // -1 == uninitialized. This is so we know if ComputeNormals has been set
  // by the user, so that we can preserve old (broken) behavior that ignored
  // this setting for certain dataset types.
  this->ComputeNormals = -1;
  this->ComputeGradients = 0;
  this->ComputeScalars = 1;

  this->Locator = nullptr;

  this->UseScalarTree = 0;
  this->ScalarTree = nullptr;

  this->OutputPointsPrecision = DEFAULT_PRECISION;

  this->GenerateTriangles = 1;
  this->ArrayComponent = 0;
  this->FastMode = false;

  this->ContourGrid->SetContainerAlgorithm(this);
  this->Contour3DLinearGrid->SetContainerAlgorithm(this);
  this->FlyingEdges2D->SetContainerAlgorithm(this);
  this->FlyingEdges3D->SetContainerAlgorithm(this);
  this->GridSynchronizedTemplates->SetContainerAlgorithm(this);
  this->RectilinearSynchronizedTemplates->SetContainerAlgorithm(this);
  this->SynchronizedTemplates2D->SetContainerAlgorithm(this);
  this->SynchronizedTemplates3D->SetContainerAlgorithm(this);

  this->InternalProgressCallbackCommand->SetCallback(
    &vtkContourFilter::InternalProgressCallbackFunction);
  this->InternalProgressCallbackCommand->SetClientData(this);

  this->ContourGrid->AddObserver(vtkCommand::ProgressEvent, this->InternalProgressCallbackCommand);
  this->Contour3DLinearGrid->AddObserver(
    vtkCommand::ProgressEvent, this->InternalProgressCallbackCommand);
  this->FlyingEdges2D->AddObserver(
    vtkCommand::ProgressEvent, this->InternalProgressCallbackCommand);
  this->FlyingEdges3D->AddObserver(
    vtkCommand::ProgressEvent, this->InternalProgressCallbackCommand);
  this->GridSynchronizedTemplates->AddObserver(
    vtkCommand::ProgressEvent, this->InternalProgressCallbackCommand);
  this->RectilinearSynchronizedTemplates->AddObserver(
    vtkCommand::ProgressEvent, this->InternalProgressCallbackCommand);
  this->SynchronizedTemplates2D->AddObserver(
    vtkCommand::ProgressEvent, this->InternalProgressCallbackCommand);
  this->SynchronizedTemplates3D->AddObserver(
    vtkCommand::ProgressEvent, this->InternalProgressCallbackCommand);

  // by default process active point scalars
  this->SetInputArrayToProcess(
    0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, vtkDataSetAttributes::SCALARS);
}

//------------------------------------------------------------------------------
vtkContourFilter::~vtkContourFilter()
{
  if (this->Locator)
  {
    this->Locator->UnRegister(this);
    this->Locator = nullptr;
  }
  if (this->ScalarTree)
  {
    this->ScalarTree->Delete();
    this->ScalarTree = nullptr;
  }
}

//------------------------------------------------------------------------------
// Overload standard modified time function. If contour values are modified,
// then this object is modified as well.
vtkMTimeType vtkContourFilter::GetMTime()
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

//------------------------------------------------------------------------------
int vtkContourFilter::RequestUpdateExtent(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkDataSet* input = vtkDataSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkInformation* fInfo = vtkDataObject::GetActiveFieldInformation(
    inInfo, vtkDataObject::FIELD_ASSOCIATION_POINTS, vtkDataSetAttributes::SCALARS);
  int sType = VTK_DOUBLE;
  if (fInfo)
  {
    sType = fInfo->Get(vtkDataObject::FIELD_ARRAY_TYPE());
  }

  // handle 2D images
  if (vtkImageData::SafeDownCast(input) && sType != VTK_BIT && !vtkUniformGrid::SafeDownCast(input))
  {
    int dim = 3;
    int* uExt = inInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT());
    if (uExt[0] == uExt[1])
    {
      --dim;
    }
    if (uExt[2] == uExt[3])
    {
      --dim;
    }
    if (uExt[4] == uExt[5])
    {
      --dim;
    }

    if (dim == 2)
    {
      if (this->FastMode)
      {
        return this->FlyingEdges2D->ProcessRequest(request, inputVector, outputVector);
      }
      else
      {
        return this->SynchronizedTemplates2D->ProcessRequest(request, inputVector, outputVector);
      }
    }
    else if (dim == 3)
    {
      if (this->FastMode && this->GenerateTriangles)
      {
        this->FlyingEdges3D->SetComputeNormals(this->ComputeNormals);
        this->FlyingEdges3D->SetComputeGradients(this->ComputeGradients);
        return this->FlyingEdges3D->ProcessRequest(request, inputVector, outputVector);
      }
      else
      {
        this->SynchronizedTemplates3D->SetComputeNormals(this->ComputeNormals);
        this->SynchronizedTemplates3D->SetComputeGradients(this->ComputeGradients);
        return this->SynchronizedTemplates3D->ProcessRequest(request, inputVector, outputVector);
      }
    }
  } // if image data

  // handle 3D RGrids
  if (vtkRectilinearGrid::SafeDownCast(input) && sType != VTK_BIT)
  {
    int* uExt = inInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT());
    // if 3D
    if (uExt[0] < uExt[1] && uExt[2] < uExt[3] && uExt[4] < uExt[5])
    {
      this->RectilinearSynchronizedTemplates->SetComputeNormals(this->ComputeNormals);
      this->RectilinearSynchronizedTemplates->SetComputeGradients(this->ComputeGradients);
      return this->RectilinearSynchronizedTemplates->ProcessRequest(
        request, inputVector, outputVector);
    }
  } // if 3D RGrid

  // handle 3D SGrids
  if (vtkStructuredGrid::SafeDownCast(input) && sType != VTK_BIT)
  {
    int* uExt = inInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT());
    // if 3D
    if (uExt[0] < uExt[1] && uExt[2] < uExt[3] && uExt[4] < uExt[5])
    {
      this->GridSynchronizedTemplates->SetComputeNormals(this->ComputeNormals);
      this->GridSynchronizedTemplates->SetComputeGradients(this->ComputeGradients);
      return this->GridSynchronizedTemplates->ProcessRequest(request, inputVector, outputVector);
    }
  } // if 3D SGrid

  inInfo->Set(vtkStreamingDemandDrivenPipeline::EXACT_EXTENT(), 1);
  return 1;
}

//------------------------------------------------------------------------------
// General contouring filter.  Handles arbitrary input.
//
int vtkContourFilter::RequestData(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the input
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  vtkDataSet* input = vtkDataSet::GetData(inputVector[0]);
  vtkPolyData* output = vtkPolyData::GetData(outputVector);
  if (!input)
  {
    return 0;
  }
  if (!output)
  {
    return 0;
  }

  // get the contours
  const int numContours = this->ContourValues->GetNumberOfContours();
  double* values = this->ContourValues->GetValues();

  // is there data to process?
  vtkDataArray* inScalars = this->GetInputArrayToProcess(0, inputVector);
  if (!inScalars)
  {
    return 1;
  }

  int sType = inScalars->GetDataType();

  // handle 2D images
  auto uG = vtkUniformGrid::SafeDownCast(input);
  if (vtkImageData::SafeDownCast(input) && sType != VTK_BIT && (!uG || uG->GetDataDimension() == 3))
  {
    int dim = 3;
    int* uExt = inInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT());
    if (uExt[0] == uExt[1])
    {
      --dim;
    }
    if (uExt[2] == uExt[3])
    {
      --dim;
    }
    if (uExt[4] == uExt[5])
    {
      --dim;
    }

    if (dim == 2)
    {
      if (this->FastMode)
      {
        this->FlyingEdges2D->SetNumberOfContours(numContours);
        std::copy_n(values, numContours, this->FlyingEdges2D->GetValues());
        this->FlyingEdges2D->SetArrayComponent(this->ArrayComponent);
        this->FlyingEdges2D->SetComputeScalars(this->ComputeScalars);
        this->FlyingEdges2D->SetInputArrayToProcess(0, this->GetInputArrayInformation(0));
        return this->FlyingEdges2D->ProcessRequest(request, inputVector, outputVector);
      }
      else
      {
        this->SynchronizedTemplates2D->SetNumberOfContours(numContours);
        std::copy_n(values, numContours, this->SynchronizedTemplates2D->GetValues());
        this->SynchronizedTemplates2D->SetArrayComponent(this->ArrayComponent);
        this->SynchronizedTemplates2D->SetComputeScalars(this->ComputeScalars);
        this->SynchronizedTemplates2D->SetInputArrayToProcess(0, this->GetInputArrayInformation(0));
        return this->SynchronizedTemplates2D->ProcessRequest(request, inputVector, outputVector);
      }
    }
    else if (dim == 3)
    {
      int retVal = 1;
      if (this->FastMode && this->GenerateTriangles)
      {
        this->FlyingEdges3D->SetNumberOfContours(numContours);
        std::copy_n(values, numContours, this->FlyingEdges3D->GetValues());
        this->FlyingEdges3D->SetArrayComponent(this->ArrayComponent);
        this->FlyingEdges3D->SetComputeNormals(this->ComputeNormals);
        this->FlyingEdges3D->SetComputeGradients(this->ComputeGradients);
        this->FlyingEdges3D->SetComputeScalars(this->ComputeScalars);
        this->FlyingEdges3D->SetInterpolateAttributes(true);
        this->FlyingEdges3D->SetInputArrayToProcess(0, this->GetInputArrayInformation(0));
        retVal = this->FlyingEdges3D->ProcessRequest(request, inputVector, outputVector);
      }
      else
      {
        this->SynchronizedTemplates3D->SetNumberOfContours(numContours);
        std::copy_n(values, numContours, this->SynchronizedTemplates3D->GetValues());
        this->SynchronizedTemplates3D->SetArrayComponent(this->ArrayComponent);
        this->SynchronizedTemplates3D->SetComputeNormals(this->ComputeNormals);
        this->SynchronizedTemplates3D->SetComputeGradients(this->ComputeGradients);
        this->SynchronizedTemplates3D->SetComputeScalars(this->ComputeScalars);
        this->SynchronizedTemplates3D->SetGenerateTriangles(this->GenerateTriangles);
        this->SynchronizedTemplates3D->SetInputArrayToProcess(0, this->GetInputArrayInformation(0));
        retVal = this->SynchronizedTemplates3D->ProcessRequest(request, inputVector, outputVector);
      }
      output = vtkPolyData::GetData(outputVector);
      if (output->GetCellGhostArray())
      {
        output->RemoveGhostCells();
      }
      return retVal;
    }
  } // if image data

  // handle 3D RGrids
  if (vtkRectilinearGrid::SafeDownCast(input) && sType != VTK_BIT)
  {
    int* uExt = inInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT());
    // if 3D
    if (uExt[0] < uExt[1] && uExt[2] < uExt[3] && uExt[4] < uExt[5])
    {
      this->RectilinearSynchronizedTemplates->SetNumberOfContours(numContours);
      std::copy_n(values, numContours, this->RectilinearSynchronizedTemplates->GetValues());
      this->RectilinearSynchronizedTemplates->SetArrayComponent(this->ArrayComponent);
      this->RectilinearSynchronizedTemplates->SetComputeNormals(this->ComputeNormals);
      this->RectilinearSynchronizedTemplates->SetComputeGradients(this->ComputeGradients);
      this->RectilinearSynchronizedTemplates->SetComputeScalars(this->ComputeScalars);
      this->RectilinearSynchronizedTemplates->SetGenerateTriangles(this->GenerateTriangles);
      this->RectilinearSynchronizedTemplates->SetInputArrayToProcess(
        0, this->GetInputArrayInformation(0));
      return this->RectilinearSynchronizedTemplates->ProcessRequest(
        request, inputVector, outputVector);
    }
  } // if 3D Rgrid

  // handle 3D SGrids
  if (vtkStructuredGrid::SafeDownCast(input) && sType != VTK_BIT)
  {
    int* uExt = inInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT());
    // if 3D
    if (uExt[0] < uExt[1] && uExt[2] < uExt[3] && uExt[4] < uExt[5])
    {
      this->GridSynchronizedTemplates->SetNumberOfContours(numContours);
      std::copy_n(values, numContours, this->GridSynchronizedTemplates->GetValues());
      this->GridSynchronizedTemplates->SetComputeNormals(this->ComputeNormals);
      this->GridSynchronizedTemplates->SetComputeGradients(this->ComputeGradients);
      this->GridSynchronizedTemplates->SetComputeScalars(this->ComputeScalars);
      this->GridSynchronizedTemplates->SetOutputPointsPrecision(this->OutputPointsPrecision);
      this->GridSynchronizedTemplates->SetGenerateTriangles(this->GenerateTriangles);
      this->GridSynchronizedTemplates->SetInputArrayToProcess(0, this->GetInputArrayInformation(0));
      return this->GridSynchronizedTemplates->ProcessRequest(request, inputVector, outputVector);
    }
  } // if 3D SGrid

  this->CreateDefaultLocator();

  if (auto ugridBase = vtkUnstructuredGridBase::SafeDownCast(input))
  {
    auto ugrid = vtkUnstructuredGrid::SafeDownCast(ugridBase);
    if (ugrid && this->GenerateTriangles && sType != VTK_BIT &&
      vtkContour3DLinearGrid::CanFullyProcessDataObject(ugrid, inScalars->GetName()))
    {
      this->Contour3DLinearGrid->SetNumberOfContours(numContours);
      std::copy_n(values, numContours, this->Contour3DLinearGrid->GetValues());
      this->Contour3DLinearGrid->SetInterpolateAttributes(true);
      this->Contour3DLinearGrid->SetComputeNormals(this->ComputeNormals);
      this->Contour3DLinearGrid->SetComputeScalars(this->ComputeScalars);
      this->Contour3DLinearGrid->SetOutputPointsPrecision(this->OutputPointsPrecision);
      this->Contour3DLinearGrid->SetUseScalarTree(this->UseScalarTree);
      this->ContourGrid->SetScalarTree(this->ScalarTree);

      bool mergePoints = !this->GetLocator()->IsA("vtkNonMergingPointLocator");
      this->Contour3DLinearGrid->SetMergePoints(mergePoints);
      this->Contour3DLinearGrid->SetInputArrayToProcess(0, this->GetInputArrayInformation(0));
      return this->Contour3DLinearGrid->ProcessRequest(request, inputVector, outputVector);
    }
    else
    {
      this->ContourGrid->SetNumberOfContours(numContours);
      std::copy_n(values, numContours, this->ContourGrid->GetValues());
      this->ContourGrid->SetComputeNormals(this->ComputeNormals);
      this->ContourGrid->SetComputeScalars(this->ComputeScalars);
      this->ContourGrid->SetOutputPointsPrecision(this->OutputPointsPrecision);
      this->ContourGrid->SetGenerateTriangles(this->GenerateTriangles);
      this->ContourGrid->SetUseScalarTree(this->UseScalarTree);
      if (this->UseScalarTree) // special treatment to reuse it
      {
        if (this->ScalarTree == nullptr)
        {
          this->ScalarTree = vtkSpanSpace::New();
        }
        this->ScalarTree->SetDataSet(input);
        this->ContourGrid->SetScalarTree(this->ScalarTree);
      }
      if (this->Locator)
      {
        this->ContourGrid->SetLocator(this->Locator);
      }
      this->ContourGrid->SetInputArrayToProcess(0, this->GetInputArrayInformation(0));
      return this->ContourGrid->ProcessRequest(request, inputVector, outputVector);
    }
  } // if type VTK_UNSTRUCTURED_GRID
  else
  {
    vtkDebugMacro(<< "Executing contour filter");
    vtkIdType cellId;
    bool abortExecute = false;
    vtkIdList* cellPts;
    vtkCellArray *newVerts, *newLines, *newPolys;
    vtkPoints* newPts;
    vtkIdType numCells, estimatedSize;
    vtkDataArray* cellScalars;

    // We don't want to change the active scalars in the input, but we
    // need to set the active scalars to match the input array to
    // process so that the point data copying works as expected. Create
    // a shallow copy of point data so that we can do this without
    // changing the input.
    vtkNew<vtkPointData> inPD;
    inPD->ShallowCopy(input->GetPointData());

    // Keep track of the old active scalars because when we set the new
    // scalars, the old scalars are removed from the point data entirely
    // and we have to add them back.
    vtkAbstractArray* oldScalars = inPD->GetScalars();
    inPD->SetScalars(inScalars);
    if (oldScalars)
    {
      inPD->AddArray(oldScalars);
    }
    vtkPointData* outPd = output->GetPointData();

    vtkCellData* inCd = input->GetCellData();
    vtkCellData* outCd = output->GetCellData();

    numCells = input->GetNumberOfCells();
    inScalars = this->GetInputArrayToProcess(0, inputVector);
    if (!inScalars || numCells < 1)
    {
      vtkDebugMacro(<< "No data to contour");
      return 1;
    }

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
    if (this->OutputPointsPrecision == vtkAlgorithm::DEFAULT_PRECISION)
    {
      vtkPointSet* inputPointSet = vtkPointSet::SafeDownCast(input);
      if (inputPointSet)
      {
        newPts->SetDataType(inputPointSet->GetPoints()->GetDataType());
      }
      else
      {
        newPts->SetDataType(VTK_FLOAT);
      }
    }
    else if (this->OutputPointsPrecision == vtkAlgorithm::SINGLE_PRECISION)
    {
      newPts->SetDataType(VTK_FLOAT);
    }
    else if (this->OutputPointsPrecision == vtkAlgorithm::DOUBLE_PRECISION)
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
    cellScalars = inScalars->NewInstance();
    cellScalars->SetNumberOfComponents(inScalars->GetNumberOfComponents());
    cellScalars->Allocate(cellScalars->GetNumberOfComponents() * VTK_CELL_SIZE);

    // locator used to merge potentially duplicate points
    this->Locator->InitPointInsertion(newPts, input->GetBounds(), input->GetNumberOfPoints());

    // interpolate data along edge
    // if we did not ask for scalars to be computed, don't copy them
    if (!this->ComputeScalars)
    {
      outPd->CopyScalarsOff();
    }
    outPd->InterpolateAllocate(inPD, estimatedSize, estimatedSize);
    outCd->CopyAllocate(inCd, estimatedSize, estimatedSize);

    vtkContourHelper helper(this->Locator, newVerts, newLines, newPolys, inPD, inCd, outPd, outCd,
      estimatedSize, this->GenerateTriangles != 0);
    // If enabled, build a scalar tree to accelerate search
    //
    if (!this->UseScalarTree)
    {
      vtkNew<vtkGenericCell> cell;
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
      int dimensionality;
      // We skip 0d cells (points), because they cannot be cut (generate no data).
      for (dimensionality = 1; dimensionality <= 3; ++dimensionality)
      {
        // Loop over all cells; get scalar values for all cell points
        // and process each cell.
        //
        for (cellId = 0; cellId < numCells && !abortExecute; cellId++)
        {
          // I assume that "GetCellType" is fast.
          cellType = input->GetCellType(cellId);
          if (cellType >= VTK_NUMBER_OF_CELL_TYPES)
          { // Protect against new cell types added.
            vtkErrorMacro("Unknown cell type " << cellType);
            continue;
          }
          if (vtkCellTypes::GetDimension(cellType) != dimensionality)
          {
            continue;
          }
          input->GetCell(cellId, cell);
          cellPts = cell->GetPointIds();
          cellScalars->SetNumberOfTuples(cellPts->GetNumberOfIds());
          inScalars->GetTuples(cellPts, cellScalars);

          if (dimensionality == 3 && !(cellId % 5000))
          {
            vtkDebugMacro(<< "Contouring #" << cellId);
            this->UpdateProgress(static_cast<double>(cellId) / numCells);
            abortExecute = this->CheckAbort();
          }

          for (int i = 0; i < numContours; i++)
          {
            helper.Contour(cell, values[i], cellScalars, cellId);
          } // for all contour values
        }   // for all cells
      }     // for all dimensions
    }       // if using scalar tree
    else
    {
      if (this->ScalarTree == nullptr)
      {
        this->ScalarTree = vtkSpanSpace::New();
      }
      this->ScalarTree->SetDataSet(input);

      vtkCell* cell;
      // Note: This will have problems when input contains 2D and 3D cells.
      // CellData will get scrabled because of the implicit ordering of
      // verts, lines and polys in vtkPolyData.  The solution
      // is to convert this filter to create unstructured grid.
      //
      // Loop over all contour values.  Then for each contour value,
      // loop over all cells.
      //
      int progressCounter = 0;
      int checkAbortInterval = 0;
      for (int i = 0; i < numContours && !abortExecute; i++)
      {
        progressCounter = 0;
        checkAbortInterval =
          std::min(this->ScalarTree->GetNumberOfCellBatches(values[i]), (vtkIdType)1000);
        for (this->ScalarTree->InitTraversal(values[i]);
             (cell = this->ScalarTree->GetNextCell(cellId, cellPts, cellScalars)) != nullptr;)
        {
          if (progressCounter % checkAbortInterval == 0 && this->CheckAbort())
          {
            abortExecute = true;
            break;
          }
          progressCounter++;
          helper.Contour(cell, values[i], cellScalars, cellId);
        } // for all cells
      }   // for all contour values
    }     // using scalar tree

    vtkDebugMacro(<< "Created: " << newPts->GetNumberOfPoints() << " points, "
                  << newVerts->GetNumberOfCells() << " verts, " << newLines->GetNumberOfCells()
                  << " lines, " << newPolys->GetNumberOfCells() << " triangles");

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

    // -1 == uninitialized. This setting used to be ignored, and we preserve the
    // old behavior for backward compatibility. Normals will be computed here
    // if and only if the user has explicitly set the option.
    if (this->ComputeNormals != 0 && this->ComputeNormals != -1)
    {
      vtkNew<vtkPolyDataNormals> normalsFilter;
      normalsFilter->SetContainerAlgorithm(this);
      normalsFilter->SetOutputPointsPrecision(this->OutputPointsPrecision);
      vtkNew<vtkPolyData> tempInput;
      tempInput->ShallowCopy(output);
      normalsFilter->SetInputData(tempInput);
      normalsFilter->SetFeatureAngle(180.);
      normalsFilter->UpdatePiece(
        outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()),
        outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES()),
        outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS()));
      output->ShallowCopy(normalsFilter->GetOutput());
    }

    this->Locator->Initialize(); // releases leftover memory
    output->Squeeze();
  } // else if not vtkUnstructuredGrid

  return 1;
}

//------------------------------------------------------------------------------
// Specify a spatial locator for merging points. By default,
// an instance of vtkMergePoints is used.
void vtkContourFilter::SetLocator(vtkIncrementalPointLocator* locator)
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

//------------------------------------------------------------------------------
void vtkContourFilter::CreateDefaultLocator()
{
  if (this->Locator == nullptr)
  {
    this->Locator = vtkMergePoints::New();
    this->Locator->Register(this);
    this->Locator->Delete();
  }
}

//------------------------------------------------------------------------------
int vtkContourFilter::FillInputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  return 1;
}

//------------------------------------------------------------------------------
void vtkContourFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Compute Gradients: " << (this->ComputeGradients ? "On\n" : "Off\n");
  os << indent << "Compute Normals: " << (this->ComputeNormals ? "On\n" : "Off\n");
  os << indent << "Compute Scalars: " << (this->ComputeScalars ? "On\n" : "Off\n");

  this->ContourValues->PrintSelf(os, indent.GetNextIndent());

  os << indent << "Use Scalar Tree: " << (this->UseScalarTree ? "On\n" : "Off\n");
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
  os << indent << "ArrayComponent: " << this->ArrayComponent << "\n";
  os << indent << "Fast Mode: " << (this->FastMode ? "On\n" : "Off\n");
}

//------------------------------------------------------------------------------
void vtkContourFilter::ReportReferences(vtkGarbageCollector* collector)
{
  this->Superclass::ReportReferences(collector);
  // These filters share our input and are therefore involved in a
  // reference loop.
  vtkGarbageCollectorReport(collector, this->ScalarTree, "ScalarTree");
}

//------------------------------------------------------------------------------
void vtkContourFilter::InternalProgressCallbackFunction(
  vtkObject* vtkNotUsed(caller), unsigned long vtkNotUsed(eid), void* clientData, void* callData)
{
  vtkContourFilter* contourFilter = static_cast<vtkContourFilter*>(clientData);
  double progress = *static_cast<double*>(callData);
  contourFilter->UpdateProgress(progress);
}
VTK_ABI_NAMESPACE_END
