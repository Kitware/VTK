/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVPICReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkVPICReader.h"

#include "vtkCallbackCommand.h"
#include "vtkDataArraySelection.h"
#include "vtkDataObject.h"
#include "vtkErrorCode.h"
#include "vtkFloatArray.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkIntArray.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTableExtentTranslator.h"
#include "vtkToolkits.h"

#include "vtkMultiProcessController.h"

#include "VPICDataSet.h"
#include "GridExchange.h"
#include "VPICView.h"

vtkStandardNewMacro(vtkVPICReader);

//----------------------------------------------------------------------------
// Constructor for VPIC Reader
//----------------------------------------------------------------------------
vtkVPICReader::vtkVPICReader()
{
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);

  this->FileName = NULL;
  this->NumberOfNodes = 0;
  this->NumberOfVariables = 0;
  this->CurrentTimeStep = -1;
  this->PointDataArraySelection = vtkDataArraySelection::New();

  // Setup selection callback to modify this object when array selection changes
  this->SelectionObserver = vtkCallbackCommand::New();
  this->SelectionObserver->SetCallback(&vtkVPICReader::SelectionCallback);
  this->SelectionObserver->SetClientData(this);
  this->PointDataArraySelection->AddObserver(vtkCommand::ModifiedEvent,
                                             this->SelectionObserver);
  // External VPICDataSet for actually reading files
  this->vpicData = 0;
  this->exchanger = 0;
  this->VariableName = 0;
  this->VariableStruct = 0;
  this->TimeSteps = 0;
  this->dataLoaded = 0;
  this->data = 0;

  // One overlap cell on first plane and one extra on last plane
  this->ghostLevel0 = 1;
  this->ghostLevel1 = 2;

  this->Stride[0] = 1;
  this->Stride[1] = 1;
  this->Stride[2] = 1;

  this->XLayout[0] = 1;
  this->YLayout[0] = 1;
  this->ZLayout[0] = 1;
  this->XLayout[1] = -1;
  this->YLayout[1] = -1;
  this->ZLayout[1] = -1;

  this->MPIController = vtkMultiProcessController::GetGlobalController();

  if(this->MPIController)
    {
    this->Rank = this->MPIController->GetLocalProcessId();
    this->TotalRank = this->MPIController->GetNumberOfProcesses();
    }
  else
    {
    this->Rank = 0;
    this->TotalRank = 1;
    }
}

//----------------------------------------------------------------------------
// Destructor for VPIC Reader
//----------------------------------------------------------------------------
vtkVPICReader::~vtkVPICReader()
{
  if (this->FileName)
    {
    delete [] this->FileName;
    }
  this->PointDataArraySelection->Delete();

  if (this->vpicData)
    delete this->vpicData;
  if (this->VariableName)
    delete [] this->VariableName;
  if (this->VariableStruct)
    delete [] this->VariableStruct;
  if (this->TimeSteps)
    delete [] this->TimeSteps;
  if (this->dataLoaded)
    delete [] this->dataLoaded;

  if (this->exchanger)
    delete this->exchanger;

  if (this->data)
    {
    for (int var = 0; var < this->NumberOfVariables; var++)
      {
      if (this->data[var])
        {
        this->data[var]->Delete();
        }
      }
    delete [] this->data;
    }

  this->SelectionObserver->Delete();

  // Do not delete the MPIController it is Singleton like and will
  // cleanup itself;
  this->MPIController = NULL;
}

//----------------------------------------------------------------------------
// Verify that the file exists
//----------------------------------------------------------------------------
int vtkVPICReader::RequestInformation(
  vtkInformation *vtkNotUsed(reqInfo),
  vtkInformationVector **vtkNotUsed(inVector),
  vtkInformationVector *outVector)
{
  // Verify that file exists
  if ( !this->FileName ) {
    vtkErrorMacro("No filename specified");
    return 0;
  }

  // Get ParaView information and output pointers
  vtkInformation* outInfo = outVector->GetInformationObject(0);
  vtkImageData *output = vtkImageData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // RequestInformation() is called for every Modified() event which means
  // when more variable data is selected, time step is changed or stride
  // is changed it will be called again
  // Only want to create the VPICDataSet one time

  if (this->vpicData == 0) {

    // Create the general VPICDataSet structure first time method is called
    // At this point we only know the file name driving the data set but
    // no variables or strides have been selected

    // Object which will know all of structure and processor part of the data
    this->vpicData = new VPICDataSet();
    this->vpicData->setRank(this->Rank);
    this->vpicData->setTotalRank(this->TotalRank);

    // Set the variable names and types
    // Build the partition table which shows the relation of each file
    // within the entire problem set, but does not partition between processors
    this->vpicData->initialize(this->FileName);

    // Copy in variable names to be offered
    this->NumberOfVariables = this->vpicData->getNumberOfVariables();
    this->VariableName = new vtkStdString[this->NumberOfVariables];

    // Data is SCALAR, VECTOR or TENSOR
    this->VariableStruct = new int[this->NumberOfVariables];

    for (int var = 0; var < this->NumberOfVariables; var++) {
      this->VariableName[var] = this->vpicData->getVariableName(var);
      this->VariableStruct[var] = this->vpicData->getVariableStruct(var);
      this->PointDataArraySelection->AddArray(this->VariableName[var].c_str());
    }

    // Allocate the ParaView data arrays which will hold the variable data
    this->data = new vtkFloatArray*[this->NumberOfVariables];
    this->dataLoaded = new int[this->NumberOfVariables];
    for (int var = 0; var < this->NumberOfVariables; var++) {
      this->data[var] = vtkFloatArray::New();
      this->data[var]->SetName(VariableName[var].c_str());
      this->dataLoaded[var] = 0;
    }

    // Set the overall problem file decomposition for the GUI extent range
    int layoutSize[DIMENSION];
    this->vpicData->getLayoutSize(layoutSize);
    this->XLayout[0] = 0;       this->XLayout[1] = layoutSize[0] - 1;
    this->YLayout[0] = 0;       this->YLayout[1] = layoutSize[1] - 1;
    this->ZLayout[0] = 0;       this->ZLayout[1] = layoutSize[2] - 1;

    // Maximum number of pieces (processors) is number of files
    this->NumberOfPieces = this->vpicData->getNumberOfParts();
    outInfo->Set(vtkStreamingDemandDrivenPipeline::MAXIMUM_NUMBER_OF_PIECES(),
                 this->NumberOfPieces);

    // Collect temporal information
    this->NumberOfTimeSteps = this->vpicData->getNumberOfTimeSteps();
    this->TimeSteps = NULL;

    if (this->NumberOfTimeSteps > 0) {
      this->TimeSteps = new double[this->NumberOfTimeSteps];

      for (int step = 0; step < this->NumberOfTimeSteps; step++)
         this->TimeSteps[step] = (double) this->vpicData->getTimeStep(step);

      // Tell the pipeline what steps are available
      outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(),
                   this->TimeSteps, this->NumberOfTimeSteps);

      // Range is required to get GUI to show things
      double tRange[2];
      tRange[0] = this->TimeSteps[0];
      tRange[1] = this->TimeSteps[this->NumberOfTimeSteps - 1];
      outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(),
                   tRange, 2);
    } else {
      outInfo->Remove(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
      outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(),
                   this->TimeSteps, this->NumberOfTimeSteps);
    }
  }

  // Set the current stride within the dataset
  // If it is a new stride the dataset will indicate that a new partition
  // must be done so that new grid subextents are set on each processor
  this->vpicData->setView(this->XExtent, this->YExtent, this->ZExtent);
  this->vpicData->setStride(this->Stride);

  // Repartition only has to be done when the stride changes
  // To handle the loading for the very first time, vpicData stride is set
  // to 0 so that by setting to the default of 1, the partition has be to done
  if (this->vpicData->needsGridCalculation() == true) {

    // If grid is recalculated all data must be realoaded
    for (int var = 0; var < this->NumberOfVariables; var++)
      this->dataLoaded[var] = 0;

    // Partitions the data between processors and sets grid extents
    this->vpicData->calculateGridExtents();

    this->NumberOfCells = this->vpicData->getNumberOfCells();
    this->NumberOfNodes = this->vpicData->getNumberOfNodes();

    // Set the whole extent
    this->vpicData->getGridSize(this->Dimension);
    this->vpicData->getWholeExtent(this->WholeExtent);
    output->SetDimensions(this->Dimension);

    outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),
                 this->WholeExtent, 6);

    // Let the pipeline know how we want the data to be broken up
    // Some processors might not get a piece of data to render
    vtkTableExtentTranslator *extentTable = vtkTableExtentTranslator::New();
    int processorUsed = this->vpicData->getProcessorUsed();

    if(this->MPIController)
      {
      this->MPIController->AllReduce(&processorUsed, &this->UsedRank,
                                     1, vtkCommunicator::SUM_OP);
      }

    extentTable->SetNumberOfPieces(this->UsedRank);

    for (int piece = 0; piece < this->UsedRank; piece++) {
      int subextent[6];
      this->vpicData->getSubExtent(piece, subextent);
      extentTable->SetExtentForPiece(piece, subextent);
    }
    this->vpicData->getSubExtent(this->Rank, this->SubExtent);
    extentTable->SetPiece(this->Rank);
    extentTable->SetWholeExtent(this->WholeExtent);
    extentTable->SetExtent(this->SubExtent);

    vtkStreamingDemandDrivenPipeline::SetExtentTranslator(outInfo, extentTable);
    extentTable->Delete();

    // Reset the SubExtent on this processor to include ghost cells
    // Leave the subextents in the extent table as the size without ghosts
    for (int dim = 0; dim < DIMENSION; dim++) {
      if (this->SubExtent[dim*2] != 0)
        this->SubExtent[dim*2] -= 1;
      if (this->SubExtent[dim*2+1] != this->Dimension[dim] - 1)
        this->SubExtent[dim*2+1] += 1;
    }

    // Set the subextent dimension size
    if (processorUsed == 1) {
      this->SubDimension[0] = this->SubExtent[1] - this->SubExtent[0] + 1;
      this->SubDimension[1] = this->SubExtent[3] - this->SubExtent[2] + 1;
      this->SubDimension[2] = this->SubExtent[5] - this->SubExtent[4] + 1;
    } else {
      this->SubDimension[0] = 0;
      this->SubDimension[1] = 0;
      this->SubDimension[2] = 0;
    }

    // Total size of the subextent
    this->NumberOfTuples = 1;
    for (int dim = 0; dim < DIMENSION; dim++)
      this->NumberOfTuples *= this->SubDimension[dim];

    // Set ghost cell edges
    this->NumberOfGhostTuples = 1;
    for (int dim = 0; dim < DIMENSION; dim++) {

      // Local block dimensions for loading a component of data
      // Different number of ghost cells are added depending on where the
      // processor is in the problem grid
      this->GhostDimension[dim] = this->SubDimension[dim];

      // If processor is on an edge don't write a ghost cell (offset the start)
      this->Start[dim] = 0;
      if (SubExtent[dim*2] == 0) {
        this->Start[dim] = this->ghostLevel0;
        this->GhostDimension[dim] += this->ghostLevel0;
      }

      // Processors not on last plane already have one overlap cell
      if (SubExtent[dim*2 + 1] == (this->Dimension[dim] - 1)) {
        this->GhostDimension[dim] += this->ghostLevel1;
      }

      // Size of the local block for loading a component of data with ghosts
      this->NumberOfGhostTuples *= this->GhostDimension[dim];
    }

    if (this->TotalRank>1)
      {
      // Set up the GridExchange for sharing ghost cells on this view
      int decomposition[DIMENSION];
      this->vpicData->getDecomposition(decomposition);

      if (this->exchanger)
        delete this->exchanger;

      this->exchanger = new GridExchange
        (this->Rank, this->TotalRank, decomposition,
         this->GhostDimension, this->ghostLevel0, this->ghostLevel1);
      }
  }
  return 1;
}

//----------------------------------------------------------------------------
// Data is read into a vtkImageData
// BLOCK structured means data is organized by variable and then by cell
//----------------------------------------------------------------------------
int vtkVPICReader::RequestData(
  vtkInformation *vtkNotUsed(reqInfo),
  vtkInformationVector **vtkNotUsed(inVector),
  vtkInformationVector *outVector)
{
  vtkInformation *outInfo = outVector->GetInformationObject(0);
  vtkImageData *output = vtkImageData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // Even if the pipeline asks for a smaller subextent, give it the
  // full subextent with ghosts
  vtkStreamingDemandDrivenPipeline::SetUpdateExtent(outInfo, this->SubExtent);

  // Set the subextent for this processor
  output->SetExtent(this->SubExtent);

  // Ask VPICDataSet to check for additional time steps
  // If found VPICDataSet will update its structure
  this->vpicData->addNewTimeSteps();
  int numberOfTimeSteps = this->vpicData->getNumberOfTimeSteps();

  // If more time steps ParaView must update information
  if (numberOfTimeSteps > this->NumberOfTimeSteps) {

    this->NumberOfTimeSteps = numberOfTimeSteps;
    delete [] this->TimeSteps;
    this->TimeSteps = new double[this->NumberOfTimeSteps];

    for (int step = 0; step < this->NumberOfTimeSteps; step++)
      this->TimeSteps[step] = (double) this->vpicData->getTimeStep(step);

    // Tell the pipeline what steps are available
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(),
                 this->TimeSteps, this->NumberOfTimeSteps);

    // Range is required to get GUI to show things
    double tRange[2];
    tRange[0] = this->TimeSteps[0];
    tRange[1] = this->TimeSteps[this->NumberOfTimeSteps - 1];
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(), tRange, 2);
  }

  // Collect the time step requested
  double requestedTimeStep(0);
  vtkInformationDoubleKey* timeKey =
    static_cast<vtkInformationDoubleKey*>
      (vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());

  // Actual time for the time step
  double dTime = this->TimeSteps[0];
  if (outInfo->Has(timeKey)) {
    requestedTimeStep = outInfo->Get(timeKey);
    dTime = requestedTimeStep;
  }

  output->GetInformation()->Set(vtkDataObject::DATA_TIME_STEP(), dTime);

  // Index of the time step to request
  int timeStep = 0;
  while (timeStep < this->NumberOfTimeSteps &&
         this->TimeSteps[timeStep] < dTime)
    timeStep++;

  // If this is a new time step read all the data from files
  int timeChanged = 0;
  if (this->CurrentTimeStep != timeStep) {
    timeChanged = 1;
    this->CurrentTimeStep = timeStep;
  }

  // Get size information from the VPICDataSet to set ImageData
  double origin[DIMENSION], step[DIMENSION];
  this->vpicData->getOrigin(origin);
  this->vpicData->getStep(step);
  output->SetSpacing(step);
  output->SetOrigin(origin);

  // Examine each variable to see if it is selected
  for (int var = 0; var < this->NumberOfVariables; var++) {

    // Is this variable requested
    if (this->PointDataArraySelection->GetArraySetting(var)) {
      if (this->dataLoaded[var] == 0 || timeChanged) {
        LoadVariableData(var, timeStep);
        this->dataLoaded[var] = 1;
      }
      output->GetPointData()->AddArray(this->data[var]);

    } else {
      this->dataLoaded[var] = 0;
    }
  }
  return 1;
}

//----------------------------------------------------------------------------
// Load one variable data array of BLOCK structure into ParaView
//----------------------------------------------------------------------------
void vtkVPICReader::LoadVariableData(int var, int timeStep)
{
  this->data[var]->Delete();
  this->data[var] = vtkFloatArray::New();
  this->data[var]->SetName(VariableName[var].c_str());

  /*
  if (this->Rank == 0)
    cout << "LoadVariableData " << this->VariableName[var]
         << " time " << timeStep << endl;
  */

  // First set the number of components for this variable
  int numberOfComponents = 0;
  if (this->VariableStruct[var] == SCALAR) {
    numberOfComponents = 1;
    this->data[var]->SetNumberOfComponents(numberOfComponents);
  }
  else if (this->VariableStruct[var] == VECTOR) {
    numberOfComponents = DIMENSION;
    this->data[var]->SetNumberOfComponents(numberOfComponents);
  }
  else if (this->VariableStruct[var] == TENSOR) {
    numberOfComponents = TENSOR_DIMENSION;
    this->data[var]->SetNumberOfComponents(TENSOR9_DIMENSION);
  }

  // Second set the number of tuples which will allocate all tuples
  this->data[var]->SetNumberOfTuples(this->NumberOfTuples);

  // For each component of the requested variable load data
  float* block = new float[this->NumberOfGhostTuples];
  float* varData = this->data[var]->GetPointer(0);

  for (int comp = 0; comp < numberOfComponents; comp++) {

    // Fetch the data for a single component into temporary storage
    this->vpicData->loadVariableData(block, this->ghostLevel0,
                                     this->GhostDimension, timeStep, var, comp);

    // Exchange the single component block retrieved from files to get ghosts
    if (this->TotalRank>1)
      {
      this->exchanger->exchangeGrid(block);
      }

    // Load the ghost component block into ParaView array
    if (this->VariableStruct[var] != TENSOR) {
      LoadComponent(varData, block, comp, numberOfComponents);
    }

    else {
      // Tensors are 6 point and must be written as 9 point
      // (0->0) (1->4) (2->8) (3->5,7) (4->2,6) (5->1,3)
      switch (comp) {
      case 0:
        LoadComponent(varData, block, 0, TENSOR9_DIMENSION);
        break;
      case 1:
        LoadComponent(varData, block, 4, TENSOR9_DIMENSION);
        break;
      case 2:
        LoadComponent(varData, block, 8, TENSOR9_DIMENSION);
        break;
      case 3:
        LoadComponent(varData, block, 5, TENSOR9_DIMENSION);
        LoadComponent(varData, block, 7, TENSOR9_DIMENSION);
        break;
      case 4:
        LoadComponent(varData, block, 2, TENSOR9_DIMENSION);
        LoadComponent(varData, block, 6, TENSOR9_DIMENSION);
        break;
      case 5:
        LoadComponent(varData, block, 1, TENSOR9_DIMENSION);
        LoadComponent(varData, block, 3, TENSOR9_DIMENSION);
        break;
      }
    }
  }
  delete [] block;
}

//----------------------------------------------------------------------------
// Load one component from the local VPIC ghost enhanced block into the
// ParaView vtkFloatArray taking into account whether the processor is
// on the front plane, the back plane or in the middle which affects
// the ghost cells which can be loaded.  ParaView array is contiguous
// memory so start at the right location and offset by number of components
//----------------------------------------------------------------------------
void vtkVPICReader::LoadComponent(float* varData, float* block,
                                  int comp, int numberOfComponents)
{

  // Load into the data array by tuple so place data every comp'th spot
  int pos = comp;
  for (int k = 0; k < this->SubDimension[2]; k++) {
    int kk = k + this->Start[2];
    for (int j = 0; j < this->SubDimension[1]; j++) {
      int jj = j + this->Start[1];
      for (int i = 0; i < this->SubDimension[0]; i++) {
        int ii = i + this->Start[0];

        int index = (kk * this->GhostDimension[0] * this->GhostDimension[1]) +
                    (jj * this->GhostDimension[0]) + ii;

        varData[pos] = block[index];
        pos += numberOfComponents;
      }
    }
  }
}

//----------------------------------------------------------------------------
void vtkVPICReader::SelectionCallback(vtkObject*, unsigned long vtkNotUsed(eventid),
                                      void* clientdata, void* vtkNotUsed(calldata))
{
  static_cast<vtkVPICReader*>(clientdata)->Modified();
}

//----------------------------------------------------------------------------
vtkImageData* vtkVPICReader::GetOutput()
{
  return this->GetOutput(0);
}

//----------------------------------------------------------------------------
vtkImageData* vtkVPICReader::GetOutput(int idx)
{
  if (idx)
    {
    return NULL;
    }
  else
    {
    return vtkImageData::SafeDownCast( this->GetOutputDataObject(idx) );
    }
}

//----------------------------------------------------------------------------
int vtkVPICReader::GetNumberOfPointArrays()
{
  return this->PointDataArraySelection->GetNumberOfArrays();
}

//----------------------------------------------------------------------------
void vtkVPICReader::EnableAllPointArrays()
{
    this->PointDataArraySelection->EnableAllArrays();
}

//----------------------------------------------------------------------------
void vtkVPICReader::DisableAllPointArrays()
{
    this->PointDataArraySelection->DisableAllArrays();
}

//----------------------------------------------------------------------------
const char* vtkVPICReader::GetPointArrayName(int index)
{
  return this->VariableName[index].c_str();
}

//----------------------------------------------------------------------------
int vtkVPICReader::GetPointArrayStatus(const char* name)
{
  return this->PointDataArraySelection->ArrayIsEnabled(name);
}

//----------------------------------------------------------------------------
void vtkVPICReader::SetPointArrayStatus(const char* name, int status)
{
  if (status)
    this->PointDataArraySelection->EnableArray(name);
  else
    this->PointDataArraySelection->DisableArray(name);
}

void vtkVPICReader::PrintSelf(ostream& os, vtkIndent indent)
{
  os << indent << "FileName: " << (this->FileName != NULL ? this->FileName : "") << endl;
  os << indent << "Stride: {" << this->Stride[0] << ", " << this->Stride[1]
     << ", " << this->Stride[2] << "}" << endl;
  os << indent << "XLayout: {" << this->XLayout[0] << ", " << this->XLayout[1] << "}" << endl;
  os << indent << "YLayout: {" << this->YLayout[0] << ", " << this->YLayout[1] << "}" << endl;
  os << indent << "ZLayout: {" << this->ZLayout[0] << ", " << this->ZLayout[1] << "}" << endl;
  os << indent << "XExtent: {" << this->XExtent[0] << ", " << this->XExtent[1] << "}" << endl;
  os << indent << "YExtent: {" << this->YExtent[0] << ", " << this->YExtent[1] << "}" << endl;
  os << indent << "ZExtent: {" << this->ZExtent[0] << ", " << this->ZExtent[1] << "}" << endl;

  this->Superclass::PrintSelf(os, indent);
}
