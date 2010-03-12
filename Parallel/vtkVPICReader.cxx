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

#include "VPICDataSet.h"

#ifdef VTK_USE_MPI
#include <mpi.h>
#endif

vtkCxxRevisionMacro(vtkVPICReader, "1.5");
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
  this->PointDataArraySelection = vtkDataArraySelection::New();

  // Setup selection callback to modify this object when array selection changes
  this->SelectionObserver = vtkCallbackCommand::New();
  this->SelectionObserver->SetCallback(&vtkVPICReader::SelectionCallback);
  this->SelectionObserver->SetClientData(this);
  this->PointDataArraySelection->AddObserver(vtkCommand::ModifiedEvent,
                                             this->SelectionObserver);
  // External VPICDataSet for actually reading files
  this->vpicData = 0;

  // Set rank and total number of processors
#ifdef VTK_USE_MPI
  MPI_Comm_rank(MPI_COMM_WORLD, &this->Rank);
  MPI_Comm_size(MPI_COMM_WORLD, &this->TotalRank);
#else
  this->Rank = 0;
  this->TotalRank = 1;
#endif
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
  for (int var = 0; var < this->NumberOfVariables; var++) {
      this->data[var]->Delete();
    }
  this->SelectionObserver->Delete();
}

//----------------------------------------------------------------------------
// Verify that the file exists
//----------------------------------------------------------------------------
int vtkVPICReader::RequestInformation(
  vtkInformation *reqInfo,
  vtkInformationVector **inVector,
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
    for (int var = 0; var < this->NumberOfVariables; var++) {
      this->data[var] = vtkFloatArray::New();
      this->data[var]->SetName(VariableName[var].c_str());
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

    // Set ghost cell level
    outInfo->Set(
      vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(), 1);

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

    // Partitions the data between processors and sets grid extents
    this->vpicData->calculateGridExtents();

    this->NumberOfCells = this->vpicData->getNumberOfCells();
    this->NumberOfNodes = this->vpicData->getNumberOfNodes();

    // Set the whole extent
    this->vpicData->getGridSize(this->Dimension);
    this->vpicData->getWholeExtent(this->WholeExtent);
    output->SetDimensions(this->Dimension);
    output->SetWholeExtent(this->WholeExtent);

    outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),
                 this->WholeExtent, 6);

    // Let the pipeline know how we want the data to be broken up
    // Some processors might not get a piece of data to render
    vtkTableExtentTranslator *extentTable = vtkTableExtentTranslator::New();
    int processorUsed = this->vpicData->getProcessorUsed();
#ifdef VTK_USE_MPI
    MPI_Allreduce((void*) &processorUsed,
                  (void*) &this->UsedRank,
                  1, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
#endif
    extentTable->SetNumberOfPieces(this->UsedRank);

    for (int piece = 0; piece < this->UsedRank; piece++) {
      int subextent[6];
      this->vpicData->getSubExtent(piece, subextent);
      extentTable->SetExtentForPiece(piece, subextent);
    }
    extentTable->SetMaximumGhostLevel(0);
    vtkStreamingDemandDrivenPipeline* pipeline =
      vtkStreamingDemandDrivenPipeline::SafeDownCast(this->GetExecutive());
    pipeline->SetExtentTranslator(outInfo, extentTable);
    extentTable->Delete();

    // Set the subextent dimension size
    this->vpicData->getSubExtent(this->Rank, this->SubExtent);
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
  // get the info object
  vtkInformation *outInfo = outVector->GetInformationObject(0);
     
  // Output will be an ImageData
  vtkImageData *output = vtkImageData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

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
  double* requestedTimeSteps = NULL;
  int numRequestedTimeSteps = 0;
  vtkInformationDoubleVectorKey* timeKey = 
    static_cast<vtkInformationDoubleVectorKey*>
      (vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEPS());

  if (outInfo->Has(timeKey)) {
    numRequestedTimeSteps = outInfo->Length(timeKey);
    requestedTimeSteps = outInfo->Get(timeKey);
  }

  // Actual time for the time step
  double dTime = requestedTimeSteps[0];
  output->GetInformation()->Set(vtkDataObject::DATA_TIME_STEPS(), &dTime, 1);

  // Index of the time step to request
  int timeStep = 0;
  while (timeStep < this->NumberOfTimeSteps && 
         this->TimeSteps[timeStep] < dTime)
    timeStep++;

  // Get size information from the VPICDataSet to set ImageData
  double origin[DIMENSION], step[DIMENSION];
  this->vpicData->getOrigin(origin);
  this->vpicData->getStep(step);
  output->SetSpacing(step);
  output->SetOrigin(origin);

  // Set the subextent for this processor
  outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), 
               this->SubExtent);
  output->SetExtent(this->SubExtent);

  // Examine each variable to see if it is selected
  for (int var = 0; var < this->NumberOfVariables; var++) {

    // Is this variable requested
    if (this->PointDataArraySelection->GetArraySetting(var)) {
      LoadVariableData(var, timeStep);
      output->GetPointData()->AddArray(this->data[var]);
    }
  }
  return 1;
}

//----------------------------------------------------------------------------
// Load one variable data array of BLOCK structure into ParaView
//----------------------------------------------------------------------------
void vtkVPICReader::LoadVariableData(int var, int timeStep)
{
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
  float* block = new float[this->NumberOfTuples];
  float* varData = this->data[var]->GetPointer(0);
  int pos;

  for (int comp = 0; comp < numberOfComponents; comp++) {

    // Fetch the data for a single component into temporary storage
    this->vpicData->loadVariableData(block, timeStep, var, comp);

    if (this->VariableStruct[var] != TENSOR) {

      // Load into the data array by tuple so place data every comp'th spot
      pos = comp;
      for (int index = 0; index < this->NumberOfTuples; index++) {
         varData[pos] = block[index];
         pos += numberOfComponents;
      }
    }

    else {
      // Tensors are 6 point and must be written as 9 point
      // (0->0) (1->4) (2->8) (3->5,7) (4->2,6) (5->1,3)
      switch (comp) {
      case 0:
        pos = 0;
        for (int index = 0; index < this->NumberOfTuples; index++) {
           varData[pos] = block[index];
           pos += TENSOR9_DIMENSION;
        }
        break;
      case 1:
        pos = 4;
        for (int index = 0; index < this->NumberOfTuples; index++) {
           varData[pos] = block[index];
           pos += TENSOR9_DIMENSION;
        }
        break;
      case 2:
        pos = 8;
        for (int index = 0; index < this->NumberOfTuples; index++) {
           varData[pos] = block[index];
           pos += TENSOR9_DIMENSION;
        }
        break;
      case 3:
        pos = 5;
        for (int index = 0; index < this->NumberOfTuples; index++) {
           varData[pos] = block[index];
           pos += TENSOR9_DIMENSION;
        }
        pos = 7;
        for (int index = 0; index < this->NumberOfTuples; index++) {
           varData[pos] = block[index];
           pos += TENSOR9_DIMENSION;
        }
        break;
      case 4:
        pos = 2;
        for (int index = 0; index < this->NumberOfTuples; index++) {
           varData[pos] = block[index];
           pos += TENSOR9_DIMENSION;
        }
        pos = 6;
        for (int index = 0; index < this->NumberOfTuples; index++) {
           varData[pos] = block[index];
           pos += TENSOR9_DIMENSION;
        }
        break;
      case 5:
        pos = 1;
        for (int index = 0; index < this->NumberOfTuples; index++) {
           varData[pos] = block[index];
           pos += TENSOR9_DIMENSION;
        }
        pos = 3;
        for (int index = 0; index < this->NumberOfTuples; index++) {
           varData[pos] = block[index];
           pos += TENSOR9_DIMENSION;
        }
        break;
      }
    }
  }
  delete [] block;
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
