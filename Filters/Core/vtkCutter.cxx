/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCutter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCutter.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkContourValues.h"
#include "vtkDataSet.h"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkGenericCell.h"
#include "vtkGridSynchronizedTemplates3D.h"
#include "vtkImageData.h"
#include "vtkImplicitFunction.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMergePoints.h"
#include "vtkObjectFactory.h"
#include "vtkPlane.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkRectilinearGrid.h"
#include "vtkRectilinearSynchronizedTemplates.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStructuredGrid.h"
#include "vtkSynchronizedTemplates3D.h"
#include "vtkSynchronizedTemplatesCutter3D.h"
#include "vtkUnstructuredGrid.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkIncrementalPointLocator.h"
#include "vtkTimerLog.h"
#include "vtkSmartPointer.h"
#include "vtkContourHelper.h"

#include <math.h>

vtkStandardNewMacro(vtkCutter);
vtkCxxSetObjectMacro(vtkCutter,CutFunction,vtkImplicitFunction);
vtkCxxSetObjectMacro(vtkCutter,Locator,vtkIncrementalPointLocator)

//----------------------------------------------------------------------------
// Construct with user-specified implicit function; initial value of 0.0; and
// generating cut scalars turned off.
vtkCutter::vtkCutter(vtkImplicitFunction *cf)
{
  this->ContourValues = vtkContourValues::New();
  this->SortBy = VTK_SORT_BY_VALUE;
  this->CutFunction = cf;
  this->GenerateCutScalars = 0;
  this->Locator = NULL;
  this->GenerateTriangles = 1;

  this->SynchronizedTemplates3D = vtkSynchronizedTemplates3D::New();
  this->SynchronizedTemplatesCutter3D = vtkSynchronizedTemplatesCutter3D::New();
  this->GridSynchronizedTemplates = vtkGridSynchronizedTemplates3D::New();
  this->RectilinearSynchronizedTemplates = vtkRectilinearSynchronizedTemplates::New();

  this->GetInformation()->Set(vtkAlgorithm::PRESERVES_RANGES(), 1);
  this->GetInformation()->Set(vtkAlgorithm::PRESERVES_BOUNDS(), 1);
}

//----------------------------------------------------------------------------
vtkCutter::~vtkCutter()
{
  this->ContourValues->Delete();
  this->SetCutFunction(NULL);
  this->SetLocator(NULL);

  this->SynchronizedTemplates3D->Delete();
  this->SynchronizedTemplatesCutter3D->Delete();
  this->GridSynchronizedTemplates->Delete();
  this->RectilinearSynchronizedTemplates->Delete();
}

//----------------------------------------------------------------------------
// Overload standard modified time function. If cut functions is modified,
// or contour values modified, then this object is modified as well.
unsigned long vtkCutter::GetMTime()
{
  unsigned long mTime=this->Superclass::GetMTime();
  unsigned long contourValuesMTime=this->ContourValues->GetMTime();
  unsigned long time;

  mTime = ( contourValuesMTime > mTime ? contourValuesMTime : mTime );

  if ( this->CutFunction != NULL )
    {
    time = this->CutFunction->GetMTime();
    mTime = ( time > mTime ? time : mTime );
    }

  return mTime;
}

//----------------------------------------------------------------------------
void vtkCutter::StructuredPointsCutter(vtkDataSet *dataSetInput,
                                       vtkPolyData *thisOutput,
                                       vtkInformation *request,
                                       vtkInformationVector **inputVector,
                                       vtkInformationVector *outputVector)
{
  vtkImageData *input = vtkImageData::SafeDownCast(dataSetInput);
  vtkPolyData *output;
  vtkIdType numPts = input->GetNumberOfPoints();

  if (numPts < 1)
    {
    return;
    }

  int numContours = this->GetNumberOfContours();

  // for one contour we use the SyncTempCutter which is faster and has a
  // smaller memory footprint
  if (numContours == 1)
    {
    this->SynchronizedTemplatesCutter3D->SetCutFunction(this->CutFunction);
    this->SynchronizedTemplatesCutter3D->SetValue(0, this->GetValue(0));
    this->SynchronizedTemplatesCutter3D->SetGenerateTriangles(this->GetGenerateTriangles());
    this->SynchronizedTemplatesCutter3D->ProcessRequest(request,inputVector,outputVector);
    return;
    }

  // otherwise compute scalar data then contour
  vtkFloatArray *cutScalars = vtkFloatArray::New();
  cutScalars->SetNumberOfTuples(numPts);
  cutScalars->SetName("cutScalars");

  vtkImageData *contourData = vtkImageData::New();
  contourData->ShallowCopy(input);
  if (this->GenerateCutScalars)
    {
    contourData->GetPointData()->SetScalars(cutScalars);
    }
  else
    {
    contourData->GetPointData()->AddArray(cutScalars);
    }

  int i,j,k;
  double scalar;
  double x[3];
  int *ext = input->GetExtent();
  double *origin = input->GetOrigin();
  double *spacing = input->GetSpacing();
  int count = 0;
  for (k = ext[4]; k <= ext[5]; ++k)
    {
    x[2] = origin[2] + spacing[2]*k;
    for (j = ext[2]; j <= ext[3]; ++j)
      {
      x[1] = origin[1] + spacing[1]*j;
      for (i = ext[0]; i <= ext[1]; i++)
        {
        x[0] = origin[0] + spacing[0]*i;
        scalar = this->CutFunction->FunctionValue(x);
        cutScalars->SetComponent(count, 0, scalar);
        count++;
        }
      }
    }

  this->SynchronizedTemplates3D->SetInputData(contourData);
  this->SynchronizedTemplates3D->
    SetInputArrayToProcess(0,0,0,vtkDataObject::FIELD_ASSOCIATION_POINTS,"cutScalars");
  this->SynchronizedTemplates3D->SetNumberOfContours(numContours);
  for (i = 0; i < numContours; i++)
    {
    this->SynchronizedTemplates3D->SetValue(i, this->GetValue(i));
    }
  this->SynchronizedTemplates3D->ComputeScalarsOff();
  this->SynchronizedTemplates3D->ComputeNormalsOff();
  output = this->SynchronizedTemplates3D->GetOutput();
  this->SynchronizedTemplatesCutter3D->SetGenerateTriangles(this->GetGenerateTriangles());
  this->SynchronizedTemplates3D->Update();
  output->Register(this);

  thisOutput->CopyStructure(output);
  thisOutput->GetPointData()->ShallowCopy(output->GetPointData());
  thisOutput->GetCellData()->ShallowCopy(output->GetCellData());
  output->UnRegister(this);

  cutScalars->Delete();
  contourData->Delete();
}

//----------------------------------------------------------------------------
void vtkCutter::StructuredGridCutter(vtkDataSet *dataSetInput,
                                     vtkPolyData *thisOutput)
{
  vtkStructuredGrid *input = vtkStructuredGrid::SafeDownCast(dataSetInput);
  vtkPolyData *output;
  vtkIdType numPts = input->GetNumberOfPoints();

  if (numPts < 1)
    {
    return;
    }

  vtkFloatArray *cutScalars = vtkFloatArray::New();
  cutScalars->SetNumberOfTuples(numPts);
  cutScalars->SetName("cutScalars");

  vtkStructuredGrid *contourData = vtkStructuredGrid::New();
  contourData->ShallowCopy(input);
  if (this->GenerateCutScalars)
    {
    contourData->GetPointData()->SetScalars(cutScalars);
    }
  else
    {
    contourData->GetPointData()->AddArray(cutScalars);
    }

  int i;
  double scalar;
  for (i = 0; i < numPts; i++)
    {
    scalar = this->CutFunction->FunctionValue(input->GetPoint(i));
    cutScalars->SetComponent(i, 0, scalar);
    }
  int numContours = this->GetNumberOfContours();

  this->GridSynchronizedTemplates->SetDebug(this->GetDebug());
  this->GridSynchronizedTemplates->SetInputData(contourData);
  this->GridSynchronizedTemplates->
    SetInputArrayToProcess(0,0,0,vtkDataObject::FIELD_ASSOCIATION_POINTS,"cutScalars");
  this->GridSynchronizedTemplates->SetNumberOfContours(numContours);
  for (i = 0; i < numContours; i++)
    {
    this->GridSynchronizedTemplates->SetValue(i, this->GetValue(i));
    }
  this->GridSynchronizedTemplates->ComputeScalarsOff();
  this->GridSynchronizedTemplates->ComputeNormalsOff();
  this->GridSynchronizedTemplates->SetGenerateTriangles(this->GetGenerateTriangles());
  output = this->GridSynchronizedTemplates->GetOutput();
  this->GridSynchronizedTemplates->Update();
  output->Register(this);

  thisOutput->ShallowCopy(output);
  output->UnRegister(this);

  cutScalars->Delete();
  contourData->Delete();
}

//----------------------------------------------------------------------------
void vtkCutter::RectilinearGridCutter(vtkDataSet *dataSetInput,
                                      vtkPolyData *thisOutput)
{
  vtkRectilinearGrid *input = vtkRectilinearGrid::SafeDownCast(dataSetInput);
  vtkPolyData *output;
  vtkIdType numPts = input->GetNumberOfPoints();

  if (numPts < 1)
    {
    return;
    }

  vtkFloatArray *cutScalars = vtkFloatArray::New();
  cutScalars->SetNumberOfTuples(numPts);
  cutScalars->SetName("cutScalars");

  vtkRectilinearGrid *contourData = vtkRectilinearGrid::New();
  contourData->ShallowCopy(input);
  if (this->GenerateCutScalars)
    {
    contourData->GetPointData()->SetScalars(cutScalars);
    }
  else
    {
    contourData->GetPointData()->AddArray(cutScalars);
    }

  int i;
  double scalar;
  for (i = 0; i < numPts; i++)
    {
    scalar = this->CutFunction->FunctionValue(input->GetPoint(i));
    cutScalars->SetComponent(i, 0, scalar);
    }
  int numContours = this->GetNumberOfContours();

  this->RectilinearSynchronizedTemplates->SetInputData(contourData);
  this->RectilinearSynchronizedTemplates->
    SetInputArrayToProcess(0,0,0,vtkDataObject::FIELD_ASSOCIATION_POINTS,"cutScalars");
  this->RectilinearSynchronizedTemplates->SetNumberOfContours(numContours);
  for (i = 0; i < numContours; i++)
    {
    this->RectilinearSynchronizedTemplates->SetValue(i, this->GetValue(i));
    }
  this->RectilinearSynchronizedTemplates->ComputeScalarsOff();
  this->RectilinearSynchronizedTemplates->ComputeNormalsOff();
  this->RectilinearSynchronizedTemplates->SetGenerateTriangles(this->GenerateTriangles);
  output = this->RectilinearSynchronizedTemplates->GetOutput();
  this->RectilinearSynchronizedTemplates->Update();
  output->Register(this);

  thisOutput->ShallowCopy(output);
  output->UnRegister(this);

  cutScalars->Delete();
  contourData->Delete();
}

//----------------------------------------------------------------------------
// Cut through data generating surface.
//
int vtkCutter::RequestData(
  vtkInformation *request,
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkDataSet *input = vtkDataSet::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData *output = vtkPolyData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkDebugMacro(<< "Executing cutter");
  if (!this->CutFunction)
    {
    vtkErrorMacro("No cut function specified");
    return 0;
    }

  if ( input->GetNumberOfPoints() < 1 || this->GetNumberOfContours() < 1 )
    {
    return 1;
    }

#ifdef TIMEME
  vtkSmartPointer<vtkTimerLog> timer = vtkSmartPointer<vtkTimerLog>::New();
  timer->StartTimer();
#endif

  if (input->GetDataObjectType() == VTK_STRUCTURED_POINTS ||
      input->GetDataObjectType() == VTK_IMAGE_DATA)
    {
    if ( input->GetCell(0) && input->GetCell(0)->GetCellDimension() >= 3 )
      {
      this->StructuredPointsCutter(input, output, request, inputVector, outputVector);
      }
    }
  else if (input->GetDataObjectType() == VTK_STRUCTURED_GRID)
    {
    if (input->GetCell(0))
      {
      int dim = input->GetCell(0)->GetCellDimension();
      // only do 3D structured grids (to be extended in the future)
      if (dim >= 3)
        {
        this->StructuredGridCutter(input, output);
        }
      }
    }
  else if (input->GetDataObjectType() == VTK_RECTILINEAR_GRID)
    {
    int dim = static_cast<vtkRectilinearGrid *>(input)->GetDataDimension();
    if ( dim == 3 )
      {
      this->RectilinearGridCutter(input, output);
      }
    }

  else if (input->GetDataObjectType() == VTK_UNSTRUCTURED_GRID)
    {
    vtkDebugMacro(<< "Executing Unstructured Grid Cutter");
    this->UnstructuredGridCutter(input, output);
    }
  else
    {
    vtkDebugMacro(<< "Executing DataSet Cutter");
    this->DataSetCutter(input, output);
    }

#ifdef TIMEME
  timer->StopTimer();
  cout << "Sliced "<<output->GetNumberOfCells()<<" cells in "<< timer->GetElapsedTime() <<" secs "<<endl;
#endif
  return 1;
}

//----------------------------------------------------------------------------
void vtkCutter::GetCellTypeDimensions(unsigned char* cellTypeDimensions)
{
  // Assume most cells will be 3d.
  memset(cellTypeDimensions, 3, VTK_NUMBER_OF_CELL_TYPES);
  cellTypeDimensions[VTK_EMPTY_CELL] = 0;
  cellTypeDimensions[VTK_VERTEX] = 0;
  cellTypeDimensions[VTK_POLY_VERTEX] = 0;
  cellTypeDimensions[VTK_LINE] = 1;
  cellTypeDimensions[VTK_CUBIC_LINE]=1;
  cellTypeDimensions[VTK_POLY_LINE] = 1;
  cellTypeDimensions[VTK_QUADRATIC_EDGE] = 1;
  cellTypeDimensions[VTK_PARAMETRIC_CURVE] = 1;
  cellTypeDimensions[VTK_HIGHER_ORDER_EDGE] = 1;
  cellTypeDimensions[VTK_TRIANGLE] = 2;
  cellTypeDimensions[VTK_TRIANGLE_STRIP] = 2;
  cellTypeDimensions[VTK_POLYGON] = 2;
  cellTypeDimensions[VTK_PIXEL] = 2;
  cellTypeDimensions[VTK_QUAD] = 2;
  cellTypeDimensions[VTK_QUADRATIC_TRIANGLE] = 2;
  cellTypeDimensions[VTK_BIQUADRATIC_TRIANGLE] = 2;
  cellTypeDimensions[VTK_QUADRATIC_QUAD] = 2;
  cellTypeDimensions[VTK_QUADRATIC_LINEAR_QUAD] = 2;
  cellTypeDimensions[VTK_BIQUADRATIC_QUAD] = 2;
  cellTypeDimensions[VTK_PARAMETRIC_SURFACE] = 2;
  cellTypeDimensions[VTK_PARAMETRIC_TRI_SURFACE] = 2;
  cellTypeDimensions[VTK_PARAMETRIC_QUAD_SURFACE] = 2;
  cellTypeDimensions[VTK_HIGHER_ORDER_TRIANGLE] = 2;
  cellTypeDimensions[VTK_HIGHER_ORDER_QUAD] = 2;
  cellTypeDimensions[VTK_HIGHER_ORDER_POLYGON] = 2;
}


//----------------------------------------------------------------------------
void vtkCutter::DataSetCutter(vtkDataSet *input, vtkPolyData *output)
{
  vtkIdType cellId, i;
  int iter;
  vtkPoints *cellPts;
  vtkDoubleArray *cellScalars;
  vtkGenericCell *cell;
  vtkCellArray *newVerts, *newLines, *newPolys;
  vtkPoints *newPoints;
  vtkDoubleArray *cutScalars;
  double value, s;
  vtkIdType estimatedSize, numCells=input->GetNumberOfCells();
  vtkIdType numPts=input->GetNumberOfPoints();
  int numCellPts;
  vtkPointData *inPD, *outPD;
  vtkCellData *inCD=input->GetCellData(), *outCD=output->GetCellData();
  vtkIdList *cellIds;
  int numContours=this->ContourValues->GetNumberOfContours();
  int abortExecute=0;

  cellScalars=vtkDoubleArray::New();

  // Create objects to hold output of contour operation
  //
  estimatedSize = static_cast<vtkIdType>(
    pow(static_cast<double>(numCells), .75)) * numContours;
  estimatedSize = estimatedSize / 1024 * 1024; //multiple of 1024
  if (estimatedSize < 1024)
    {
    estimatedSize = 1024;
    }

  newPoints = vtkPoints::New();
  newPoints->Allocate(estimatedSize,estimatedSize/2);
  newVerts = vtkCellArray::New();
  newVerts->Allocate(estimatedSize,estimatedSize/2);
  newLines = vtkCellArray::New();
  newLines->Allocate(estimatedSize,estimatedSize/2);
  newPolys = vtkCellArray::New();
  newPolys->Allocate(estimatedSize,estimatedSize/2);
  cutScalars = vtkDoubleArray::New();
  cutScalars->SetNumberOfTuples(numPts);

  // Interpolate data along edge. If generating cut scalars, do necessary setup
  if ( this->GenerateCutScalars )
    {
    inPD = vtkPointData::New();
    inPD->ShallowCopy(input->GetPointData());//copies original attributes
    inPD->SetScalars(cutScalars);
    }
  else
    {
    inPD = input->GetPointData();
    }
  outPD = output->GetPointData();
  outPD->InterpolateAllocate(inPD,estimatedSize,estimatedSize/2);
  outCD->CopyAllocate(inCD,estimatedSize,estimatedSize/2);

  // locator used to merge potentially duplicate points
  if ( this->Locator == NULL )
    {
    this->CreateDefaultLocator();
    }
  this->Locator->InitPointInsertion (newPoints, input->GetBounds());

  // Loop over all points evaluating scalar function at each point
  //
  for ( i=0; i < numPts; i++ )
    {
    s = this->CutFunction->FunctionValue(input->GetPoint(i));
    cutScalars->SetComponent(i,0,s);
    }

  // Compute some information for progress methods
  //
  cell = vtkGenericCell::New();
  vtkIdType numCuts = numContours*numCells;
  vtkIdType progressInterval = numCuts/20 + 1;
  int cut=0;

  vtkContourHelper helper(this->Locator, newVerts, newLines, newPolys,inPD, inCD, outPD,outCD, estimatedSize,this->GenerateTriangles!=0);
  if ( this->SortBy == VTK_SORT_BY_CELL )
    {
    // Loop over all contour values.  Then for each contour value,
    // loop over all cells.
    //
    // This is going to have a problem if the input has 2D and 3D cells.
    // I am fixing a bug where cell data is scrambled becauses with
    // vtkPolyData output, verts and lines have lower cell ids than triangles.
    for (iter=0; iter < numContours && !abortExecute; iter++)
      {
      // Loop over all cells; get scalar values for all cell points
      // and process each cell.
      //
      for (cellId=0; cellId < numCells && !abortExecute; cellId++)
        {
        if ( !(++cut % progressInterval) )
          {
          vtkDebugMacro(<<"Cutting #" << cut);
          this->UpdateProgress (static_cast<double>(cut)/numCuts);
          abortExecute = this->GetAbortExecute();
          }

        input->GetCell(cellId,cell);
        cellPts = cell->GetPoints();
        cellIds = cell->GetPointIds();

        numCellPts = cellPts->GetNumberOfPoints();
        cellScalars->SetNumberOfTuples(numCellPts);
        for (i=0; i < numCellPts; i++)
          {
          s = cutScalars->GetComponent(cellIds->GetId(i),0);
          cellScalars->SetTuple(i,&s);
          }

        value = this->ContourValues->GetValue(iter);

        helper.Contour(cell,value, cellScalars, cellId);
        } // for all cells
      } // for all contour values
    } // sort by cell

  else // VTK_SORT_BY_VALUE:
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
      for (cellId=0; cellId < numCells && !abortExecute; cellId++)
        {
        // I assume that "GetCellType" is fast.
        cellType = input->GetCellType(cellId);
        if (cellType >= VTK_NUMBER_OF_CELL_TYPES)
          { // Protect against new cell types added.
          vtkErrorMacro("Unknown cell type " << cellType);
          continue;
          }
        if (cellTypeDimensions[cellType] != dimensionality)
          {
          continue;
          }
        input->GetCell(cellId,cell);
        cellPts = cell->GetPoints();
        cellIds = cell->GetPointIds();

        numCellPts = cellPts->GetNumberOfPoints();
        cellScalars->SetNumberOfTuples(numCellPts);
        for (i=0; i < numCellPts; i++)
          {
          s = cutScalars->GetComponent(cellIds->GetId(i),0);
          cellScalars->SetTuple(i,&s);
          }

        // Loop over all contour values.
        for (iter=0; iter < numContours && !abortExecute; iter++)
          {
          if (dimensionality == 3 && !(++cut % progressInterval) )
            {
            vtkDebugMacro(<<"Cutting #" << cut);
            this->UpdateProgress (static_cast<double>(cut)/numCuts);
            abortExecute = this->GetAbortExecute();
            }
          value = this->ContourValues->GetValue(iter);
          helper.Contour(cell,value, cellScalars, cellId);
          } // for all contour values
        } // for all cells
      } // for all dimensions.
    } // sort by value

  // Update ourselves.  Because we don't know upfront how many verts, lines,
  // polys we've created, take care to reclaim memory.
  //
  cell->Delete();
  cellScalars->Delete();
  cutScalars->Delete();

  if ( this->GenerateCutScalars )
    {
    inPD->Delete();
    }

  output->SetPoints(newPoints);
  newPoints->Delete();

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

  this->Locator->Initialize();//release any extra memory
  output->Squeeze();
}

//----------------------------------------------------------------------------
void vtkCutter::UnstructuredGridCutter(vtkDataSet *input, vtkPolyData *output)
{
  vtkIdType cellId, i;
  int iter;
  vtkDoubleArray *cellScalars;
  vtkCellArray *newVerts, *newLines, *newPolys;
  vtkPoints *newPoints;
  vtkDoubleArray *cutScalars;
  double value, s;
  vtkIdType estimatedSize, numCells=input->GetNumberOfCells();
  vtkIdType numPts=input->GetNumberOfPoints();
  vtkIdType cellArrayIt = 0;
  int numCellPts;
  vtkPointData *inPD, *outPD;
  vtkCellData *inCD=input->GetCellData(), *outCD=output->GetCellData();
  vtkIdList *cellIds;
  int numContours = this->ContourValues->GetNumberOfContours();
  int abortExecute = 0;

  double range[2];

  // Create objects to hold output of contour operation
  //
  estimatedSize = static_cast<vtkIdType>(
    pow(static_cast<double>(numCells),.75)) * numContours;
  estimatedSize = estimatedSize / 1024 * 1024; //multiple of 1024
  if (estimatedSize < 1024)
    {
    estimatedSize = 1024;
    }

  newPoints = vtkPoints::New();
  newPoints->Allocate(estimatedSize,estimatedSize/2);
  newVerts = vtkCellArray::New();
  newVerts->Allocate(estimatedSize,estimatedSize/2);
  newLines = vtkCellArray::New();
  newLines->Allocate(estimatedSize,estimatedSize/2);
  newPolys = vtkCellArray::New();
  newPolys->Allocate(estimatedSize,estimatedSize/2);
  cutScalars = vtkDoubleArray::New();
  cutScalars->SetNumberOfTuples(numPts);

  // Interpolate data along edge. If generating cut scalars, do necessary setup
  if ( this->GenerateCutScalars )
    {
    inPD = vtkPointData::New();
    inPD->ShallowCopy(input->GetPointData());//copies original attributes
    inPD->SetScalars(cutScalars);
    }
  else
    {
    inPD = input->GetPointData();
    }
  outPD = output->GetPointData();
  outPD->InterpolateAllocate(inPD,estimatedSize,estimatedSize/2);
  outCD->CopyAllocate(inCD,estimatedSize,estimatedSize/2);

  // locator used to merge potentially duplicate points
  if ( this->Locator == NULL )
    {
    this->CreateDefaultLocator();
    }
  this->Locator->InitPointInsertion (newPoints, input->GetBounds());

  // Loop over all points evaluating scalar function at each point
  //
  for ( i=0; i < numPts; i++ )
    {
    s = this->CutFunction->FunctionValue(input->GetPoint(i));
    cutScalars->SetComponent(i,0,s);
    }

  // Compute some information for progress methods
  //
  vtkIdType numCuts = numContours*numCells;
  vtkIdType progressInterval = numCuts/20 + 1;
  int cut=0;

  vtkUnstructuredGrid *grid = static_cast<vtkUnstructuredGrid *>(input);
  vtkIdType *cellArrayPtr = grid->GetCells()->GetPointer();
  double *scalarArrayPtr = cutScalars->GetPointer(0);
  double tempScalar;
  cellScalars = cutScalars->NewInstance();
  cellScalars->SetNumberOfComponents(cutScalars->GetNumberOfComponents());
  cellScalars->Allocate(VTK_CELL_SIZE*cutScalars->GetNumberOfComponents());

  vtkContourHelper helper(this->Locator, newVerts, newLines, newPolys,inPD, inCD, outPD,outCD, estimatedSize,this->GenerateTriangles!=0);
  if ( this->SortBy == VTK_SORT_BY_CELL )
    {
    // Loop over all contour values.  Then for each contour value,
    // loop over all cells.
    //
    for (iter=0; iter < numContours && !abortExecute; iter++)
      {
      // Loop over all cells; get scalar values for all cell points
      // and process each cell.
      //
      for (cellId=0; cellId < numCells && !abortExecute; cellId++)
        {
        if ( !(++cut % progressInterval) )
          {
          vtkDebugMacro(<<"Cutting #" << cut);
          this->UpdateProgress (static_cast<double>(cut)/numCuts);
          abortExecute = this->GetAbortExecute();
          }

        numCellPts = cellArrayPtr[cellArrayIt];
        cellArrayIt++;

        //find min and max values in scalar data
        range[0] = scalarArrayPtr[cellArrayPtr[cellArrayIt]];
        range[1] = scalarArrayPtr[cellArrayPtr[cellArrayIt]];
        cellArrayIt++;

        for (i = 1; i < numCellPts; i++)
          {
          tempScalar = scalarArrayPtr[cellArrayPtr[cellArrayIt]];
          cellArrayIt++;
          if (tempScalar <= range[0])
            {
            range[0] = tempScalar;
            } //if tempScalar <= min range value
          if (tempScalar >= range[1])
            {
            range[1] = tempScalar;
            } //if tempScalar >= max range value
          } // for all points in this cell

        int needCell = 0;
        double val = this->ContourValues->GetValue(iter);
        if (val >= range[0] && val <= range[1])
          {
          needCell = 1;
          }

        if (needCell)
          {
          vtkCell *cell = input->GetCell(cellId);
          cellIds = cell->GetPointIds();
          cutScalars->GetTuples(cellIds,cellScalars);
          // Loop over all contour values.
          for (iter=0; iter < numContours && !abortExecute; iter++)
            {
            if ( !(++cut % progressInterval) )
              {
              vtkDebugMacro(<<"Cutting #" << cut);
              this->UpdateProgress (static_cast<double>(cut)/numCuts);
              abortExecute = this->GetAbortExecute();
              }
            value = this->ContourValues->GetValue(iter);

            helper.Contour(cell,value, cellScalars, cellId);
            }
          }

        } // for all cells
      } // for all contour values
    } // sort by cell

  else // SORT_BY_VALUE:
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
      cellArrayIt = 0;
      for (cellId=0; cellId < numCells && !abortExecute; cellId++)
        {
        numCellPts = cellArrayPtr[cellArrayIt];
        // I assume that "GetCellType" is fast.
        cellType = input->GetCellType(cellId);
        if (cellType >= VTK_NUMBER_OF_CELL_TYPES)
          { // Protect against new cell types added.
          vtkErrorMacro("Unknown cell type " << cellType);
          cellArrayIt += 1+numCellPts;
          continue;
          }
        if (cellTypeDimensions[cellType] != dimensionality)
          {
          cellArrayIt += 1+numCellPts;
          continue;
          }
        cellArrayIt++;

        //find min and max values in scalar data
        range[0] = scalarArrayPtr[cellArrayPtr[cellArrayIt]];
        range[1] = scalarArrayPtr[cellArrayPtr[cellArrayIt]];
        cellArrayIt++;

        for (i = 1; i < numCellPts; i++)
          {
          tempScalar = scalarArrayPtr[cellArrayPtr[cellArrayIt]];
          cellArrayIt++;
          if (tempScalar <= range[0])
            {
            range[0] = tempScalar;
            } //if tempScalar <= min range value
          if (tempScalar >= range[1])
            {
            range[1] = tempScalar;
            } //if tempScalar >= max range value
          } // for all points in this cell

        int needCell = 0;
        for (int cont = 0; cont < numContours; ++cont)
          {
          double val = this->ContourValues->GetValue(cont);
          if (val >= range[0] && val <= range[1])
            {
            needCell = 1;
            break;
            }
          }

        if (needCell)
          {
          vtkCell *cell = input->GetCell(cellId);
          cellIds = cell->GetPointIds();
          cutScalars->GetTuples(cellIds,cellScalars);
          // Loop over all contour values.
          for (iter=0; iter < numContours && !abortExecute; iter++)
            {
            if (dimensionality == 3 && !(++cut % progressInterval) )
              {
              vtkDebugMacro(<<"Cutting #" << cut);
              this->UpdateProgress (static_cast<double>(cut)/numCuts);
              abortExecute = this->GetAbortExecute();
              }
            value = this->ContourValues->GetValue(iter);
            helper.Contour(cell,value, cellScalars, cellId);
            } // for all contour values

          } // if need cell
        } // for all cells
      } // for all dimensions (1,2,3).
    } // sort by value

  // Update ourselves.  Because we don't know upfront how many verts, lines,
  // polys we've created, take care to reclaim memory.
  //
  cellScalars->Delete();
  cutScalars->Delete();

  if ( this->GenerateCutScalars )
    {
    inPD->Delete();
    }

  output->SetPoints(newPoints);
  newPoints->Delete();

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

  this->Locator->Initialize();//release any extra memory
  output->Squeeze();
}

//----------------------------------------------------------------------------
// Specify a spatial locator for merging points. By default,
// an instance of vtkMergePoints is used.
void vtkCutter::CreateDefaultLocator()
{
  if ( this->Locator == NULL )
    {
    this->Locator = vtkMergePoints::New();
    this->Locator->Register(this);
    this->Locator->Delete();
    }
}

//----------------------------------------------------------------------------
int vtkCutter::RequestUpdateExtent(
  vtkInformation *,
  vtkInformationVector **inputVector,
  vtkInformationVector *)
{
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  inInfo->Set(vtkStreamingDemandDrivenPipeline::EXACT_EXTENT(), 1);
  return 1;
}

//----------------------------------------------------------------------------
int vtkCutter::FillInputPortInformation(int, vtkInformation *info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  return 1;
}

//----------------------------------------------------------------------------
void vtkCutter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Cut Function: " << this->CutFunction << "\n";
  os << indent << "Sort By: " << this->GetSortByAsString() << "\n";

  if ( this->Locator )
    {
    os << indent << "Locator: " << this->Locator << "\n";
    }
  else
    {
    os << indent << "Locator: (none)\n";
    }

  this->ContourValues->PrintSelf(os,indent.GetNextIndent());

  os << indent << "Generate Cut Scalars: "
     << (this->GenerateCutScalars ? "On\n" : "Off\n");
}

//-----------------------------------------------------------------------

int vtkCutter::ProcessRequest(vtkInformation* request,
                              vtkInformationVector** inputVector,
                              vtkInformationVector* outputVector)
{
  if(request->Has(vtkStreamingDemandDrivenPipeline::
     REQUEST_UPDATE_EXTENT_INFORMATION()))
    {
    // compute the priority for this UpdateExtent
    vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
    if (!inInfo)
      {
      return 1;
      }

    double inPrior = 1;
    if (inInfo->Has(vtkStreamingDemandDrivenPipeline::PRIORITY()))
      {
      inPrior = inInfo->Get(vtkStreamingDemandDrivenPipeline::
                            PRIORITY());
      }

    // Get bounds and evaluate implicit function. If all bounds
    // evaluate to a value smaller than input value, this piece
    // has priority set to 0.

    static double bounds[] = {-1.0,1.0, -1.0,1.0, -1.0,1.0};
    double prior = 1;

    // determine geometric bounds of this piece
    double *wBBox =
      inInfo->
        Get(vtkStreamingDemandDrivenPipeline::PIECE_BOUNDING_BOX());

    if (!wBBox)
      {
      wBBox =
        inInfo->
        Get(vtkStreamingDemandDrivenPipeline::WHOLE_BOUNDING_BOX());
      }
    if (wBBox)
      {
      bounds[0] = wBBox[0];
      bounds[1] = wBBox[1];
      bounds[2] = wBBox[2];
      bounds[3] = wBBox[3];
      bounds[4] = wBBox[4];
      bounds[5] = wBBox[5];
      }
    else
      {
      //try to figure out geometric bounds
      double *origin = inInfo->Get(vtkDataObject::ORIGIN());
      double *spacing = inInfo->Get(vtkDataObject::SPACING());
      int *subExtent = inInfo->Get(
        vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT());
      if (origin && spacing && subExtent)
        {
        bounds[0] = origin[0]+subExtent[0]*spacing[0];
        bounds[1] = origin[0]+subExtent[1]*spacing[0];
        bounds[2] = origin[1]+subExtent[2]*spacing[1];
        bounds[3] = origin[1]+subExtent[3]*spacing[1];
        bounds[4] = origin[2]+subExtent[4]*spacing[2];
        bounds[5] = origin[2]+subExtent[5]*spacing[2];
        }
      else
        {
        //cerr << "Need geometric bounds meta information to evaluate priority" << endl;
        outputVector->GetInformationObject(0)->
          Set(vtkStreamingDemandDrivenPipeline::PRIORITY(),inPrior);
        return 1;
        }
      }

    vtkPlane* fPtr = vtkPlane::SafeDownCast(this->GetCutFunction());
    if (!fPtr)
      {
      //cerr << "Can not evaluate priority for that clip type" << endl;
      outputVector->GetInformationObject(0)->
        Set(vtkStreamingDemandDrivenPipeline::PRIORITY(),inPrior);
      return 1;
      }

    static double fVal[8];
    fVal[0] = fPtr->EvaluateFunction(bounds[0],bounds[2],bounds[4]);
    fVal[1] = fPtr->EvaluateFunction(bounds[0],bounds[2],bounds[5]);
    fVal[2] = fPtr->EvaluateFunction(bounds[0],bounds[3],bounds[4]);
    fVal[3] = fPtr->EvaluateFunction(bounds[0],bounds[3],bounds[5]);
    fVal[4] = fPtr->EvaluateFunction(bounds[1],bounds[2],bounds[4]);
    fVal[5] = fPtr->EvaluateFunction(bounds[1],bounds[2],bounds[5]);
    fVal[6] = fPtr->EvaluateFunction(bounds[1],bounds[3],bounds[4]);
    fVal[7] = fPtr->EvaluateFunction(bounds[1],bounds[3],bounds[5]);

    prior = 0;
    int numOffsets = this->ContourValues->GetNumberOfContours();
    for (int c=0; c < numOffsets; c++)
      {
      double value = this->ContourValues->GetValue(c);
      int i;
      bool less = fVal[0]<value;
      for (i=1; i<8;i++)
        {
        if ((fVal[i]<=value) != less)
          {
          //this corner is on different side than first, piece
          //intersects and cannot be rejected
          prior = inPrior;
          c = numOffsets;
          break;
          }
        }
      }
    if (prior != inPrior)
      {
      //cerr << "rejected something!" << endl;
      }
    outputVector->GetInformationObject(0)->
      Set(vtkStreamingDemandDrivenPipeline::PRIORITY(),prior);
    return 1;
    }

  //all other requests handled by superclass
  return this->Superclass::ProcessRequest(request, inputVector,
                                          outputVector);
}
