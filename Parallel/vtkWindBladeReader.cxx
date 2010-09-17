/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkWindBladeReader.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkWindBladeReader.h"

#include "vtkCallbackCommand.h"
#include "vtkCellData.h"
#include "vtkCellType.h"
#include "vtkDataArraySelection.h"
#include "vtkFloatArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkStructuredGrid.h"
#include "vtkUnstructuredGrid.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include "vtkStringArray.h"
#include "vtkFloatArray.h"
#include "vtkIntArray.h"
#include "vtkPoints.h"
#include "vtkStructuredGrid.h"
#include "vtkUnstructuredGrid.h"
#include "vtkMultiBlockDataSetAlgorithm.h"

#include <vtkstd/string>
#include <vtksys/ios/sstream>
#include <vtksys/ios/iostream>
#include <cstring>
#include <cmath>

#include "vtkMultiProcessController.h"

using namespace vtkstd;

#ifdef WIN32
static const char * Slash = "\\";
#else
static const char * Slash = "/";
#endif

vtkStandardNewMacro(vtkWindBladeReader);

//----------------------------------------------------------------------------
// Constructor for WindBlade Reader
//----------------------------------------------------------------------------
vtkWindBladeReader::vtkWindBladeReader()
{
  this->Filename = NULL;
  this->SetNumberOfInputPorts(0);

  // Set up two output ports, one for fields, one for blades
  this->SetNumberOfOutputPorts(2);

  // Irregularly spaced grid description for entire problem
  this->Points = vtkPoints::New();
  this->xSpacing = vtkFloatArray::New();
  this->ySpacing = vtkFloatArray::New();
  this->zSpacing = vtkFloatArray::New();
  this->zTopographicValues = 0;

  // Blade geometry
  this->BPoints = vtkPoints::New();
  this->NumberOfBladePoints = 0;
  this->NumberOfBladeCells = 0;

  // Static tower information
  this->NumberOfBladeTowers = 0;
  this->XPosition = vtkFloatArray::New();
  this->YPosition = vtkFloatArray::New();
  this->HubHeight = vtkFloatArray::New();
  this->BladeCount = vtkIntArray::New();

  // Options to include extra files for topography and turbines
  this->UseTopographyFile = 0;
  this->UseTurbineFile = 0;

  // Setup selection callback to modify this object when array selection changes
  this->SelectionObserver = vtkCallbackCommand::New();
  this->SelectionObserver->SetCallback(&vtkWindBladeReader::SelectionCallback);
  this->SelectionObserver->SetClientData(this);

  this->PointDataArraySelection = vtkDataArraySelection::New();
  this->PointDataArraySelection->AddObserver(vtkCommand::ModifiedEvent,
                                             this->SelectionObserver);

  // Variables need to be divided by density
  this->NumberOfTimeSteps = 1;
  this->NumberOfVariables = 0;
  this->DivideVariables = vtkStringArray::New();
  this->DivideVariables->InsertNextValue("UVW");
  this->DivideVariables->InsertNextValue("A-scale turbulence");
  this->DivideVariables->InsertNextValue("B-scale turbulence");
  this->DivideVariables->InsertNextValue("Oxygen");

  this->data = 0;

  // Set rank and total number of processors
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

  // by default don't skip any lines because normal wind files do not
  // have a header
  this->NumberLinesToSkip = 0;
}

//----------------------------------------------------------------------------
// Destructor for WindBlade Reader
//----------------------------------------------------------------------------
vtkWindBladeReader::~vtkWindBladeReader()
{
  if (this->Filename)
    {
      delete[] this->Filename;
    }
  this->PointDataArraySelection->Delete();
  this->DivideVariables->Delete();

  this->XPosition->Delete();
  this->YPosition->Delete();
  this->HubHeight->Delete();
  this->BladeCount->Delete();
  this->xSpacing->Delete();
  this->ySpacing->Delete();
  this->zSpacing->Delete();

  if (this->zTopographicValues != 0)
    delete [] zTopographicValues;

  this->Points->Delete();
  this->BPoints->Delete();

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
// Print information about WindBlade Reader
//----------------------------------------------------------------------------
void vtkWindBladeReader::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "FileName: "
     << (this->Filename ? this->Filename : "(NULL)") << endl;

  os << indent << "WholeExent: {" << this->WholeExtent[0] << ", "
     << this->WholeExtent[1] << ", " << this->WholeExtent[2] << ", "
     << this->WholeExtent[3] << ", " << this->WholeExtent[4] << ", "
     << this->WholeExtent[5] << "}" << endl;
  os << indent << "SubExtent: {" << this->SubExtent[0] << ", "
     << this->SubExtent[1] << ", " << this->SubExtent[2] << ", "
     << this->SubExtent[3] << ", " << this->SubExtent[4] << ", "
     << this->SubExtent[5] << "}" << endl;
  os << indent << "VariableArraySelection:" << endl;
  this->PointDataArraySelection->PrintSelf(os, indent.GetNextIndent());
}

//----------------------------------------------------------------------------
// RequestInformation supplies global meta information
//----------------------------------------------------------------------------
int vtkWindBladeReader::RequestInformation(
      vtkInformation* vtkNotUsed(request),
      vtkInformationVector** vtkNotUsed(inputVector),
      vtkInformationVector* outputVector)
{
  // Verify that file exists
  if ( !this->Filename ) {
    vtkErrorMacro("No filename specified");
    return 0;
  }

  // Get ParaView information and output pointers
  vtkInformation* fieldInfo = outputVector->GetInformationObject(0);
  vtkStructuredGrid *field = vtkStructuredGrid::SafeDownCast(
                             fieldInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkInformation* bladeInfo = outputVector->GetInformationObject(1);
  vtkUnstructuredGrid* blade = GetBladeOutput();

  // Read global size and variable information from input file one time
  if (this->NumberOfVariables == 0) {

    // Read the size of the problem and variables in data set
    ReadGlobalData();

    // If turbine file exists setup number of cells and points in blades, towers
    if (this->UseTurbineFile == 1)
      SetupBladeData();

    // Allocate the ParaView data arrays which will hold the variable data
    this->data = new vtkFloatArray*[this->NumberOfVariables];
    for (int var = 0; var < this->NumberOfVariables; var++) {
      this->data[var] = vtkFloatArray::New();
      this->data[var]->SetName(VariableName[var].c_str());
      this->PointDataArraySelection->AddArray(this->VariableName[var].c_str());
    }

    // Set up extent information manually for now
    this->WholeExtent[0] = this->WholeExtent[2] = this->WholeExtent[4] = 0;
    this->WholeExtent[1] = this->Dimension[0] - 1;
    this->WholeExtent[3] = this->Dimension[1] - 1;
    this->WholeExtent[5] = this->Dimension[2] - 1;

    field->SetWholeExtent(this->WholeExtent);
    field->SetDimensions(this->Dimension);
    fieldInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),
                   this->WholeExtent, 6);
    blade->SetWholeExtent(this->WholeExtent);

    // Create the rectilinear coordinate spacing for entire problem
    CreateCoordinates();

    // Collect temporal information and attach to both output ports
    this->TimeSteps = NULL;

    if (this->NumberOfTimeSteps > 0) {
      this->TimeSteps = new double[this->NumberOfTimeSteps];

      this->TimeSteps[0] = (double) this->TimeStepFirst;
      for (int step = 1; step < this->NumberOfTimeSteps; step++)
        this->TimeSteps[step] = this->TimeSteps[step-1] +
                                (double) this->TimeStepDelta;

      // Tell the pipeline what steps are available
      fieldInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(),
                     this->TimeSteps, this->NumberOfTimeSteps);
      bladeInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(),
                     this->TimeSteps, this->NumberOfTimeSteps);

      // Range is required to get GUI to show things
      double tRange[2];
      tRange[0] = this->TimeSteps[0];
      tRange[1] = this->TimeSteps[this->NumberOfTimeSteps - 1];
      fieldInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(), tRange, 2);
      bladeInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(), tRange, 2);
    } else {
      fieldInfo->Remove(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
      fieldInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(),
                   this->TimeSteps, this->NumberOfTimeSteps);
      bladeInfo->Remove(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
      bladeInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(),
                   this->TimeSteps, this->NumberOfTimeSteps);
    }
  }
  return 1;
}

//----------------------------------------------------------------------------
// RequestData populates the output object with data for rendering
// Uses two output ports (one for fields and one for turbine blades).
//----------------------------------------------------------------------------
int vtkWindBladeReader::RequestData(
      vtkInformation *reqInfo,
      vtkInformationVector **vtkNotUsed(inVector),
      vtkInformationVector *outVector)
{
  int port = reqInfo->Get(vtkDemandDrivenPipeline::FROM_OUTPUT_PORT());

  // Request data for field port
  if (port == 0) {

    // Get the information and output pointers
    vtkInformation* fieldInfo = outVector->GetInformationObject(0);
    vtkStructuredGrid *field = GetFieldOutput();

    // Set the extent info for this processor
    fieldInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),
                   this->SubExtent);
    field->SetExtent(this->SubExtent);

    // Set the rectilinear coordinates matching the requested subextents
    // Extents may include ghost cells for filters that require them
    FillCoordinates();
    field->SetPoints(this->Points);

    this->SubDimension[0] = this->SubExtent[1] - this->SubExtent[0] + 1;
    this->SubDimension[1] = this->SubExtent[3] - this->SubExtent[2] + 1;
    this->SubDimension[2] = this->SubExtent[5] - this->SubExtent[4] + 1;

    this->NumberOfTuples = 1;
    for (int dim = 0; dim < DIMENSION; dim++)
      this->NumberOfTuples *= this->SubDimension[dim];

    // Collect the time step requested
    double* requestedTimeSteps = NULL;
    int numRequestedTimeSteps = 0;
    vtkInformationDoubleVectorKey* timeKey =
      static_cast<vtkInformationDoubleVectorKey*>
        (vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEPS());

    if (fieldInfo->Has(timeKey)) {
      numRequestedTimeSteps = fieldInfo->Length(timeKey);
      requestedTimeSteps = fieldInfo->Get(timeKey);
    }

    // Actual time for the time step
    double dTime = requestedTimeSteps[0];
    field->GetInformation()->Set(vtkDataObject::DATA_TIME_STEPS(), &dTime, 1);

    // Index of the time step to request
    int timeStep = 0;
    while (timeStep < this->NumberOfTimeSteps &&
           this->TimeSteps[timeStep] < dTime)
      timeStep++;

    // Open the data file for time step if needed
    ostringstream fileName;
    fileName << this->RootDirectory << Slash
             << this->DataDirectory << Slash << this->DataBaseName
             << this->TimeSteps[timeStep];
    this->FilePtr = fopen(fileName.str().c_str(), "r");
    if (this->FilePtr == NULL)
      cout << "Could not open file " << fileName.str() << endl;
    if (this->Rank == 0)
      cout << "Load file " << fileName.str() << endl;

    // Some variables depend on others, so force their loading
    for (int i = 0; i < this->DivideVariables->GetNumberOfTuples(); i++)
      if (GetPointArrayStatus(this->DivideVariables->GetValue(i)))
        SetPointArrayStatus("Density", 1);

    // Examine each file variable to see if it is selected and load
    for (int var = 0; var < this->NumberOfFileVariables; var++) {
      if (this->PointDataArraySelection->GetArraySetting(var)) {
        LoadVariableData(var);
        field->GetPointData()->AddArray(this->data[var]);
      }
    }

    // Divide variables by Density if required
    for (int i = 0; i < this->DivideVariables->GetNumberOfTuples(); i++)
      if (GetPointArrayStatus(this->DivideVariables->GetValue(i)))
        DivideByDensity(this->DivideVariables->GetValue(i));

    // Calculate pressure if requested
    if (GetPointArrayStatus("Pressure")) {
      int pressure = this->PointDataArraySelection->GetArrayIndex("Pressure");
      int pre = this->PointDataArraySelection->GetArrayIndex("Pressure-Pre");
      int tempg = this->PointDataArraySelection->GetArrayIndex("tempg");
      int density = this->PointDataArraySelection->GetArrayIndex("Density");

      CalculatePressure(pressure, pre, tempg, density);
      field->GetPointData()->AddArray(this->data[pressure]);
      field->GetPointData()->AddArray(this->data[pressure + 1]);
    }

    // Calculate vorticity if requested
    if (GetPointArrayStatus("Vorticity")) {
      int vort = this->PointDataArraySelection->GetArrayIndex("Vorticity");
      int uvw = this->PointDataArraySelection->GetArrayIndex("UVW");
      int density = this->PointDataArraySelection->GetArrayIndex("Density");

      CalculateVorticity(vort, uvw, density);
      field->GetPointData()->AddArray(this->data[vort]);
    }
    // Close file after all data is read
    fclose(this->FilePtr);
  }

  // Request data is on blade
  // hackish: answer regardless of port just to keep the temporal
  // pipeline happy see SDDP::PREVIOUS_UPDATE_TIME_STEPS()
  if (port == 0 || port == 1) {
    if (this->UseTurbineFile == 1 && this->Rank == 0) {

      vtkInformation* bladeInfo = outVector->GetInformationObject(1);
      vtkUnstructuredGrid* blade = GetBladeOutput();

      // Collect the time step requested
      double* requestedTimeSteps = NULL;
      int numRequestedTimeSteps = 0;
      vtkInformationDoubleVectorKey* timeKey =
        static_cast<vtkInformationDoubleVectorKey*>
          (vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEPS());

      double dTime = 0.0;
      if (bladeInfo->Has(timeKey)) {
        numRequestedTimeSteps = bladeInfo->Length(timeKey);
        requestedTimeSteps = bladeInfo->Get(timeKey);
        dTime = requestedTimeSteps[0];
      }

      // Actual time for the time step
      blade->GetInformation()->Set(vtkDataObject::DATA_TIME_STEPS(), &dTime, 1);

      // Index of the time step to request
      int timeStep = 0;
      while (timeStep < this->NumberOfTimeSteps &&
             this->TimeSteps[timeStep] < dTime)
        timeStep++;

      LoadBladeData(timeStep);
    }
  }
  return 1;
}

//----------------------------------------------------------------------------
// Divide data variable by density for display
//----------------------------------------------------------------------------
void vtkWindBladeReader::DivideByDensity(const char* varName)
{
  int var = this->PointDataArraySelection->GetArrayIndex(varName);
  int density = this->PointDataArraySelection->GetArrayIndex("Density");

  float* varData = this->data[var]->GetPointer(0);
  float* densityData = this->data[density]->GetPointer(0);

  int numberOfTuples = this->data[var]->GetNumberOfTuples();
  int numberOfComponents = this->data[var]->GetNumberOfComponents();

  int index = 0;
  for (int i = 0; i < numberOfTuples; i++) {
    for (int j = 0; j < numberOfComponents; j++) {
      varData[index++] /= densityData[i];
    }
  }
}

//----------------------------------------------------------------------------
// Calculate pressure from tempg and density
// Calculate pressure - pre from pressure in first z position
// Requires that all data be present
//----------------------------------------------------------------------------
void vtkWindBladeReader::CalculatePressure(int pressure, int prespre,
                                           int tempg, int density)
{
  // Set the number of components and tuples for the requested data
  this->data[pressure]->SetNumberOfComponents(1);
  this->data[pressure]->SetNumberOfTuples(this->NumberOfTuples);
  float* pressureData = this->data[pressure]->GetPointer(0);

  this->data[prespre]->SetNumberOfComponents(1);
  this->data[prespre]->SetNumberOfTuples(this->NumberOfTuples);
  float* prespreData = this->data[prespre]->GetPointer(0);

  // Read tempg and Density components from file
  float* tempgData = new float[this->BlockSize];
  float* densityData = new float[this->BlockSize];
  fseek(this->FilePtr, this->VariableOffset[tempg], SEEK_SET);
  fread(tempgData, sizeof(float), this->BlockSize, this->FilePtr);
  fseek(this->FilePtr, this->VariableOffset[density], SEEK_SET);
  fread(densityData, sizeof(float), this->BlockSize, this->FilePtr);

  // Entire block of data is read so to calculate index into that data we
  // must use the entire Dimension and not the SubDimension
  int planeSize = this->Dimension[0] * this->Dimension[1];
  int rowSize = this->Dimension[0];

  // Pressure - pre needs the first XY plane pressure values
  float* firstPressure = new float[this->Dimension[2]];
  for (int k = 0; k < this->Dimension[2]; k++) {
    int index = k * planeSize;
    firstPressure[k] = densityData[index] * DRY_AIR_CONSTANT * tempgData[index];
  }

  // Only the requested subextents are stored on this processor
  int pos = 0;
  for (int k = this->SubExtent[4]; k <= this->SubExtent[5]; k++) {
    for (int j = this->SubExtent[2]; j <= this->SubExtent[3]; j++) {
      for (int i = this->SubExtent[0]; i <= this->SubExtent[1]; i++) {
        int index = (k * planeSize) + (j * rowSize) + i;

        // Pressure is function of density and tempg for the same position
        // Pressure - pre is the pressure at a position minus the pressure
        // from the first value in the z plane

        pressureData[pos] = densityData[index] *
                            DRY_AIR_CONSTANT * tempgData[index];
        prespreData[pos] = pressureData[pos] - firstPressure[k];
        pos++;
      }
    }
  }
  delete [] tempgData;
  delete [] densityData;
  delete [] firstPressure;
}

//----------------------------------------------------------------------------
// Calculate vorticity from UVW
// Requires ghost cell information so fetch all data from files for now
//----------------------------------------------------------------------------
void vtkWindBladeReader::CalculateVorticity(int vort, int uvw, int density)
{
  // Set the number of components and tuples for the requested data
  this->data[vort]->SetNumberOfComponents(1);
  this->data[vort]->SetNumberOfTuples(this->NumberOfTuples);
  float* vortData = this->data[vort]->GetPointer(0);

  // Read U and V components (two int block sizes in between)
  float* uData = new float[this->BlockSize];
  float* vData = new float[this->BlockSize];
  fseek(this->FilePtr, this->VariableOffset[uvw], SEEK_SET);
  fread(uData, sizeof(float), this->BlockSize, this->FilePtr);
  fseek(this->FilePtr, (2 * sizeof(int)), SEEK_SET);
  fread(vData, sizeof(float), this->BlockSize, this->FilePtr);

  // Read Density component
  float* densityData = new float[this->BlockSize];
  fseek(this->FilePtr, this->VariableOffset[density], SEEK_SET);
  fread(densityData, sizeof(float), this->BlockSize, this->FilePtr);

  // Divide U and V components by Density
  for (int i = 0; i < this->BlockSize; i++) {
    uData[i] /= densityData[i];
    vData[i] /= densityData[i];
  }

  // Entire block of data is read so to calculate index into that data we
  // must use the entire Dimension and not the SubDimension
  // Only the requested subextents are stored on this processor
  int planeSize = this->Dimension[0] * this->Dimension[1];
  int rowSize = this->Dimension[0];

  // Initialize to 0.0 because edges have no values
  int pos = 0;
  for (int k = this->SubExtent[4]; k <= this->SubExtent[5]; k++)
    for (int j = this->SubExtent[2]; j <= this->SubExtent[3]; j++)
      for (int i = this->SubExtent[0]; i <= this->SubExtent[1]; i++)
        vortData[pos++] = 0.0;

  // For inner positions calculate vorticity
  pos = 0;
  float ddx = this->Step[0];
  float ddy = this->Step[1];

  for (int k = this->SubExtent[4]; k <= this->SubExtent[5]; k++) {
    for (int j = this->SubExtent[2]; j <= this->SubExtent[3]; j++) {
      for (int i = this->SubExtent[0]; i <= this->SubExtent[1]; i++) {

        // Edges are initialized to 0
        if (j == this->SubExtent[2] || j == this->SubExtent[3] ||
            i == this->SubExtent[0] || i == this->SubExtent[1]) {
              pos++;
        } else {
          // Vorticity depends on four cells surrounding this cell
          int index_vp = (k * planeSize) + (j * rowSize) + (i + 1);
          int index_vm = (k * planeSize) + (j * rowSize) + (i - 1);
          int index_up = (k * planeSize) + ((j + 1) * rowSize) + i;
          int index_um = (k * planeSize) + ((j - 1) * rowSize) + i;

          vortData[pos++] = ((vData[index_vp] - vData[index_vm]) / ddx) -
                            ((uData[index_up] - uData[index_um]) / ddy);
        }
      }
    }
  }
  delete [] uData;
  delete [] vData;
  delete [] densityData;
}

//----------------------------------------------------------------------------
// Load one variable data array of BLOCK structure into ParaView
//----------------------------------------------------------------------------
void vtkWindBladeReader::LoadVariableData(int var)
{
  this->data[var]->Delete();
  this->data[var] = vtkFloatArray::New();
  this->data[var]->SetName(VariableName[var].c_str());

  // Skip to the appropriate variable block and read byte count
  // not used? int byteCount;
  fseek(this->FilePtr, this->VariableOffset[var], SEEK_SET);

  // Set the number of components for this variable
  int numberOfComponents = 0;
  if (this->VariableStruct[var] == SCALAR) {
    numberOfComponents = 1;
    this->data[var]->SetNumberOfComponents(numberOfComponents);
  }
  else if (this->VariableStruct[var] == VECTOR) {
    numberOfComponents = DIMENSION;
    this->data[var]->SetNumberOfComponents(numberOfComponents);
  }

  // Set the number of tuples which will allocate all tuples
  this->data[var]->SetNumberOfTuples(this->NumberOfTuples);

  // For each component of the requested variable load data
  float* block = new float[this->BlockSize];
  float* varData = this->data[var]->GetPointer(0);

  // Entire block of data is read so to calculate index into that data we
  // must use the entire Dimension and not the SubDimension
  // Only the requested subextents are stored on this processor
  int planeSize = this->Dimension[0] * this->Dimension[1];
  int rowSize = this->Dimension[0];

  for (int comp = 0; comp < numberOfComponents; comp++) {

    // Read the block of data
    fread(block, sizeof(float), this->BlockSize, this->FilePtr);

    int pos = comp;
    for (int k = this->SubExtent[4]; k <= this->SubExtent[5]; k++) {
      for (int j = this->SubExtent[2]; j <= this->SubExtent[3]; j++) {
        for (int i = this->SubExtent[0]; i <= this->SubExtent[1]; i++) {
          int index = (k * planeSize) + (j * rowSize) + i;
          varData[pos] = block[index];
          pos += numberOfComponents;
        }
      }
    }

    // Skip closing and opening byte sizes
    fseek(this->FilePtr, (2 * sizeof(int)), SEEK_CUR);
  }
  delete [] block;
}

//----------------------------------------------------------------------------
// Load one variable data array of BLOCK structure into ParaView
//----------------------------------------------------------------------------
void vtkWindBladeReader::ReadGlobalData()
{
  ifstream inStr(this->Filename);
  if (!inStr) {
    cout << "Could not open the global .wind file " << this->Filename << endl;
  }

  string::size_type dirPos = string(this->Filename).rfind(Slash);
   if (dirPos == string::npos) {
      cout << "Bad input file name " << this->Filename << endl;
   }
  this->RootDirectory = string(this->Filename).substr(0, dirPos);

  char inBuf[LINE_SIZE];
  string keyword;
  string rest;
  string headerVersion;

  while (inStr.getline(inBuf, LINE_SIZE)) {
    if (inBuf[0] != '#' && inStr.gcount() > 1) {

      string line(inBuf);
      string::size_type keyPos = line.find(' ');
      keyword = line.substr(0, keyPos);
      rest = line.substr(keyPos + 1);
      istringstream lineStr(rest.c_str());

      // Header information
      if (keyword == "WIND_HEADER_VERSION")
        lineStr >> headerVersion;

      // Topology variables
      else if (keyword == "GRID_SIZE_X")
        lineStr >> this->Dimension[0];
      else if (keyword == "GRID_SIZE_Y")
        lineStr >> this->Dimension[1];
      else if (keyword == "GRID_SIZE_Z")
        lineStr >> this->Dimension[2];
      else if (keyword == "GRID_DELTA_X")
        lineStr >> this->Step[0];
      else if (keyword == "GRID_DELTA_Y")
        lineStr >> this->Step[1];
      else if (keyword == "GRID_DELTA_Z")
        lineStr >> this->Step[2];

      // Geometry variables
      else if (keyword == "USE_TOPOGRAPHY_FILE")
        lineStr >> this->UseTopographyFile;
      else if (keyword == "TOPOGRAPHY_FILE")
        this->TopographyFile = rest;
      else if (keyword == "COMPRESSION")
        lineStr >> this->Compression;
      else if (keyword == "FIT")
        lineStr >> this->Fit;

      // Time variables
      else if (keyword == "TIME_STEP_FIRST")
        lineStr >> this->TimeStepFirst;
      else if (keyword == "TIME_STEP_LAST")
        lineStr >> this->TimeStepLast;
      else if (keyword == "TIME_STEP_DELTA")
        lineStr >> this->TimeStepDelta;

      // Turbine variables
      else if (keyword == "USE_TURBINE_FILE")
        lineStr >> this->UseTurbineFile;
      else if (keyword == "TURBINE_DIRECTORY")
        this->TurbineDirectory = rest;
      else if (keyword == "TURBINE_TOWER")
        this->TurbineTowerName = rest;
      else if (keyword == "TURBINE_BLADE")
        this->TurbineBladeName = rest;

      // Data variables
      else if (keyword == "DATA_DIRECTORY")
        this->DataDirectory = rest;
      else if (keyword == "DATA_BASE_FILENAME")
        this->DataBaseName = rest;
      else if (keyword == "DATA_VARIABLES") {
        lineStr >> this->NumberOfFileVariables;
        ReadDataVariables(inStr);
        FindVariableOffsets();
      }
    }
  }
  if (this->TimeStepFirst < this->TimeStepLast)
    this->NumberOfTimeSteps = ((this->TimeStepLast - this->TimeStepFirst) /
                                this->TimeStepDelta) + 1;
}

//----------------------------------------------------------------------------
//
// Read the field variable information
//
//----------------------------------------------------------------------------
void vtkWindBladeReader::ReadDataVariables(ifstream& inStr)
{
  char inBuf[LINE_SIZE];
  string structType, basicType;

  // Derive Vorticity = f(UVW, Density)
  // Derive Pressure = f(tempg, Density)
  // Derive Pressure - pre = f(Pressure)
  this->NumberOfDerivedVariables = 3;
  this->NumberOfVariables = this->NumberOfFileVariables;
  int totalVariables = this->NumberOfFileVariables +
                       this->NumberOfDerivedVariables;

  this->VariableName = new vtkStdString[totalVariables];
  this->VariableStruct = new int[totalVariables];
  this->VariableCompSize = new int[totalVariables];
  this->VariableBasicType = new int[totalVariables];
  this->VariableByteCount = new int[totalVariables];
  this->VariableOffset = new long int[totalVariables];

  bool hasUVW = false;
  bool hasDensity = false;
  bool hasTempg = false;

  for (int i = 0; i < this->NumberOfFileVariables; i++) {
    inStr.getline(inBuf, LINE_SIZE);

    // Variable name
    string varLine(inBuf);
    string::size_type lastPos = varLine.rfind('"');
    this->VariableName[i] = varLine.substr(1, lastPos-1);

    if (this->VariableName[i] == "UVW") hasUVW = true;
    if (this->VariableName[i] == "Density") hasDensity = true;
    if (this->VariableName[i] == "tempg") hasTempg = true;

    // Structure, number of components, type, number of bytes
    string rest = varLine.substr(lastPos+1);
    istringstream line(rest);

    line >> structType;
    line >> this->VariableCompSize[i];

    if (structType == "SCALAR")
      this->VariableStruct[i] = SCALAR;
    else if (structType == "VECTOR")
      this->VariableStruct[i] = VECTOR;
    else
      cout << "Error in structure type " << structType << endl;

    line >> basicType;
    line >> this->VariableByteCount[i];

    if (basicType == "FLOAT")
      this->VariableBasicType[i] = FLOAT;
    else if (basicType == "INTEGER")
      this->VariableBasicType[i] = INTEGER;
    else
      cout << "Error in basic type " << basicType << endl;
  }

  // Add any derived variables
  if (hasUVW && hasDensity) {
    this->VariableName[this->NumberOfVariables] = "Vorticity";
    this->NumberOfVariables++;
  }
  if (hasTempg && hasDensity) {
    this->VariableName[this->NumberOfVariables] = "Pressure";
    this->NumberOfVariables++;
    this->VariableName[this->NumberOfVariables] = "Pressure-Pre";
    this->NumberOfVariables++;
  }
}

//----------------------------------------------------------------------------
//
// Open the first data file and verify that the data is where is should be
// Each data block is enclosed by two ints which record the number of bytes
// Save the file offset for each varible
//
//----------------------------------------------------------------------------
void vtkWindBladeReader::FindVariableOffsets()
{
  // Open the first data file
  ostringstream fileName;
  fileName << this->RootDirectory << Slash
           << this->DataDirectory << Slash
           << this->DataBaseName << this->TimeStepFirst;
  this->FilePtr = fopen(fileName.str().c_str(), "r");
  if (this->FilePtr == NULL) {
    cout << "Could not open file " << fileName.str() << endl;
    exit(1);
  }

  // Scan file recording offsets which points to the first data value
  int byteCount;
  fread(&byteCount, sizeof(int), 1, this->FilePtr);
  this->BlockSize = byteCount / BYTES_PER_DATA;

  for (int var = 0; var < this->NumberOfFileVariables; var++) {
    this->VariableOffset[var] = ftell(this->FilePtr);

    // Skip over the SCALAR or VECTOR components for this variable
    int numberOfComponents = 1;
    if (this->VariableStruct[var] == VECTOR)
      numberOfComponents = DIMENSION;

    for (int comp = 0; comp < numberOfComponents; comp++) {
      // Skip data plus two integer byte counts
      fseek(this->FilePtr, (byteCount+(2 * sizeof(int))), SEEK_CUR);
    }
  }
  fclose(this->FilePtr);
}

//----------------------------------------------------------------------------
// Fill in the rectilinear points for the requested subextents
//----------------------------------------------------------------------------
void vtkWindBladeReader::FillCoordinates()
{
  this->Points->Delete();
  this->Points = vtkPoints::New();

  // If dataset is flat, x and y are constant spacing, z is stretched
  if (this->UseTopographyFile == 0) {

    // Save vtkPoints instead of spacing coordinates because topography file
    // requires this to be vtkStructuredGrid and not vtkRectilinearGrid
    for (int k = this->SubExtent[4]; k <= this->SubExtent[5]; k++) {
      float z = this->zSpacing->GetValue(k);
      for (int j = this->SubExtent[2]; j <= this->SubExtent[3]; j++) {
        float y = this->ySpacing->GetValue(j);
        for (int i = this->SubExtent[0]; i <= this->SubExtent[1]; i++) {
          float x = this->xSpacing->GetValue(i);
          this->Points->InsertNextPoint(x, y, z);
        }
      }
    }
  }

  // If dataset is topographic, x and y are constant spacing center on (0,0)
  // Z data is calculated from an x by y topographic data file
  else {
    int planeSize = this->Dimension[0] * this->Dimension[1];
    int rowSize = this->Dimension[0];

    for (int k = this->SubExtent[4]; k <= this->SubExtent[5]; k++) {
      for (int j = this->SubExtent[2]; j <= this->SubExtent[3]; j++) {
        float y = this->ySpacing->GetValue(j);
        for (int i = this->SubExtent[0]; i <= this->SubExtent[1]; i++) {
          float x = this->xSpacing->GetValue(i);
          int index = (k * planeSize) + (j * rowSize) + i;
          this->Points->InsertNextPoint(x, y, this->zTopographicValues[index]);
        }
      }
    }
  }
}

//----------------------------------------------------------------------------
// Calculate the Points for flat Rectilinear type grid or topographic
// generalized StructuredGrid which is what is being created here
//----------------------------------------------------------------------------
void vtkWindBladeReader::CreateCoordinates()
{
  // If dataset is flat, x and y are constant spacing, z is stretched
  if (this->UseTopographyFile == 0) {
    float value = 0.0;
    for (int i = 0; i < this->Dimension[0]; i++) {
      this->xSpacing->InsertNextValue(value);
      value += this->Step[0];
    }

    value = 0.0;
    for (int j = 0; j < this->Dimension[1]; j++) {
      this->ySpacing->InsertNextValue(value);
      value += this->Step[1];
    }

    double maxZ = this->Step[2] * this->Dimension[2];
    for (int k = 0; k < this->Dimension[2]; k++) {
      double zcoord = (k * this->Step[2]) + (0.5 * this->Step[2]);
      double zcartesian = GDeform(zcoord, maxZ, 0);
      this->zSpacing->InsertNextValue(zcartesian);
    }
  }

  // If dataset is topographic, x and y are constant spacing center on (0,0)
  // Z data is calculated from an x by y topographic data file
  else {
    float xHalf = (((this->Dimension[0] + 1.0) / 2.0) - 1.0) * this->Step[0];
    for (int i = 0; i < this->Dimension[0]; i++)
      this->xSpacing->InsertNextValue((i * this->Step[0]) - xHalf);

    float yHalf = (((this->Dimension[1] + 1.0) / 2.0) - 1.0) * this->Step[1];
    for (int j = 0; j < this->Dimension[1]; j++)
      this->ySpacing->InsertNextValue((j * this->Step[1]) - yHalf);

    this->zTopographicValues = new float[this->BlockSize];
    CreateZTopography(this->zTopographicValues);
  }
}

//----------------------------------------------------------------------------
// Create the z topography from 2D (x,y) elevations and return in zData
//----------------------------------------------------------------------------
void vtkWindBladeReader::CreateZTopography(float* zValues)
{
  // Read the x,y topography data file
  ostringstream fileName;
  fileName << this->RootDirectory << Slash
           << this->TopographyFile;
  FILE* filePtr = fopen(fileName.str().c_str(), "r");
  int blockSize = this->Dimension[0] * this->Dimension[1];
  float* topoData = new float[blockSize];

  fseek(filePtr, BYTES_PER_DATA, SEEK_SET);  // Fortran byte count
  fread(topoData, sizeof(float), blockSize, filePtr);

  // Initial z coordinate processing
  float* zedge = new float[this->Dimension[2] + 1];
  float* z = new float[this->Dimension[2]];
  float zb;
  int ibctopbot = 1;

  if (ibctopbot == 1) {
    for (int k = 0; k <= this->Dimension[2]; k++)
      zedge[k] = k * this->Step[2];
    zb = zedge[this->Dimension[2]];
    for (int k = 0; k < this->Dimension[2]; k++)
      z[k] = k * this->Step[2] + 0.5 * this->Step[2];
  }

  else {
    for (int k = 0; k < this->Dimension[2]; k++)
      z[k] = k * this->Step[2];
    zb = z[this->Dimension[2] - 1];
  }

  // Use cubic spline or deformation to calculate z values
  int npoints = 31;
  float* zdata = new float[npoints];
  float* zcoeff = new float[npoints];
  float zcrdata[] = {
        0.0 ,    2.00,    4.00,     6.00,      8.00,
       10.00,   14.00,   18.00,    22.00,     26.00,
       30.00,   34.00,   40.00,    50.00,     70.00,
      100.00,  130.00,  160.00,   200.00,    250.00,
      300.00,  350.00,  450.00,   550.00,    750.00,
      950.00, 1150.00, 1400.00,  1700.00,   2000.00,   2400.00 };

  // No deformation, use spline to define z coefficients
  if (this->Compression == 0.0) {
    for (int i = 0; i < npoints; i++)
      zdata[i] = (z[i] * zb) / z[npoints - 1];

    // Call spline with zcoeff being the answer
    spline(zdata, zcrdata, npoints, 99.0e31, 99.0e31, zcoeff);
  }

  // Fill the zValues array depending on compression
  int planeSize = this->Dimension[0] * this->Dimension[1];
  int rowSize = this->Dimension[0];
  int flag = 0;

  for (int k = 0; k < this->Dimension[2]; k++) {
    for (int j = 0; j < this->Dimension[1]; j++) {
      for (int i = 0; i < this->Dimension[0]; i++) {
        int index = (k * planeSize) + (j * rowSize) + i;
        int tIndex = (j * rowSize) + i;

        if (this->Compression == 0.0) {
          // Use spline interpolation
          float zinterp;
          splint(zdata, zcrdata, zcoeff, npoints, z[k], &zinterp, flag);
          zValues[index] = zinterp;
        } else {
          // Use deformation
          zValues[index] = GDeform(z[k], zb, flag) *
                                (zb - topoData[tIndex]) / zb + topoData[tIndex];
        }
      }
    }
  }

  delete [] topoData;
  delete [] zedge;
  delete [] z;
  delete [] zdata;
  delete [] zcoeff;
}

//----------------------------------------------------------------------------
//
// Stretch the Z coordinate for flat topography
// If flag = 0 compute gdeform(z)
// If flag = 1 compute derivative of gdeform(z)
// Return cubic polynomial fit
//
//----------------------------------------------------------------------------
float vtkWindBladeReader::GDeform(float sigma, float sigmaMax, int flag)
{
  float sigma_2 = sigma * sigma;
  float sigma_3 = sigma_2 * sigma;

  float f = this->Fit;
  float aa1 = this->Compression;

  float aa2 = (f * (1.0 - aa1)) / sigmaMax;
  float aa3 = (1.0 - (aa2 * sigmaMax) - aa1) / (sigmaMax * sigmaMax);

  float zcoord = 0.0;
  if (flag == 0)
    zcoord = (aa3 * sigma_3) + (aa2 * sigma_2) + (aa1 * sigma);
  else if (flag == 1)
    zcoord = (3.0 * aa3 * sigma_2) + (2.0 * aa2 * sigma) + aa1;

  return zcoord;
}

//----------------------------------------------------------------------------
// Cubic spline from Numerical Recipes (altered for zero based arrays)
// Called only once to process entire tabulated function
//
// Given arrays x[0..n-1] and y[0..n-1] containing a tabulated function
// with x0 < x1 < .. < xn-1, and given values yp1 and ypn for the
// first derivative of the interpolating function at points 0 and n-1,
// this routine returns an array y2[0..n-1] that contains the second
// derivatives of the interpolating function.  If yp1 or ypn > e30
// the rougine is signaled to set the corresponding boundary condition
// for a natural spline, with zero second derivative on that boundary.
//----------------------------------------------------------------------------
void vtkWindBladeReader::spline(
      float* x, float* y,  // arrays
      int n,      // size of arrays
      float yp1, float ypn,  // boundary condition
      float* y2)    // return array
{
  float qn, un;
  float* u = new float[n];

  // Lower boundary condition set to natural spline
  if (yp1 > 0.99e30)
    y2[0] = u[0] = 0.0;

  // Lower boundary condition set to specified first derivative
  else {
    y2[0] = -0.5;
    u[0]=(3.0/(x[1]-x[0]))*((y[1]-y[0])/(x[1]-x[0])-yp1);
  }

  // Decomposition loop of tridiagonal algorithm
  for (int i = 1; i < n-1; i++) {
    float sig = (x[i] - x[i-1]) / (x[i+1] - x[i-1]);
    float p = sig * y2[i-1] + 2.0;
    y2[i] = (sig - 1.0) / p;
    u[i] = (y[i+1] - y[i]) / (x[i+1] - x[i]) -
           (y[i] - y[i-1]) / (x[i] - x[i-1]);
    u[i] = (6.0 * u[i] / (x[i+1] - x[i-1]) - sig * u[i-1]) / p;
  }

  // Upper boundary condition set to natural spline
  if (ypn > 0.99e30)
    qn = un = 0.0;

  // Upper boundary condition set to specified first derivative
  else {
    qn = 0.5;
    un = (3.0 / (x[n-1] - x[n-2])) *
         (ypn - (y[n-1] - y[n-2]) / (x[n-1] -x [n-2]));
  }

  // Back substitution loop of tridiagonal algorithm
  y2[n-1] = (un - qn * u[n-2]) / (qn * y2[n-2] + 1.0);
  for (int k = n - 2; k >= 0; k--)
    y2[k] = y2[k] * y2[k+1] + u[k];

  delete [] u;
}

//----------------------------------------------------------------------------
// Cubic spline interpolation from Numerical Recipes
// Called succeeding times after spline is called once
// Given x, y and y2 arrays from spline return cubic spline interpolated
//----------------------------------------------------------------------------
void vtkWindBladeReader::splint(
      float* xa, float* ya,   // arrays sent to spline()
      float* y2a,     // result from spline()
      int n,      // size of arrays
      float x,    //
      float* y,    // interpolated value
      int kderivative)
{
  // Find the right place in the table by means of bisection
  // Optimal is sequential calls are at random values of x
  int klo = 0;
  int khi = n - 1;
  while (khi - klo > 1) {
    int k = (khi + klo) / 2;
    if (xa[k] > x)
      khi = k;
    else
      klo = k;
  }

  float h = xa[khi] - xa[klo];
  float a = (xa[khi] - x) / h;
  float b = (x - xa[klo]) / h;
  if (kderivative == 0)
    *y = a * ya[klo] + b * ya[khi] +
         ((a * a * a - a) * y2a[klo] +
         (b * b * b - b) * y2a[khi]) * (h * h) / 6.0;
  else
    *y = ((ya[khi] - ya[klo]) / h) -
         ((((((3.0 * a * a) - 1.0) * y2a[klo]) -
            (((3.0 * b * b) - 1.0) * y2a[khi])) * h) / 6.0);
}

//----------------------------------------------------------------------------
// Build the turbine towers
// Parse a blade file to set the number of cells and points in blades
//----------------------------------------------------------------------------
void vtkWindBladeReader::SetupBladeData()
{
  // Load the tower information
  ostringstream fileName;
  fileName << this->RootDirectory << Slash
           << this->TurbineDirectory << Slash
           << this->TurbineTowerName;
  ifstream inStr(fileName.str().c_str());
  if (!inStr)
    cout << "Could not open " << fileName.str().c_str() << endl;

  // File is ASCII text so read until EOF
  char inBuf[LINE_SIZE];
  float hubHeight, bladeLength, maxRPM, xPos, yPos, yawAngle;
  float angularVelocity, angleBlade1;
  int numberOfBlades;
  int towerID;
  // all header stuff is here to deal with wind data format changes
  // number of columns tells us if the turbine tower file has at least 13
  // columns. if so then we are dealing with a wind data format that has
  // an extra header in the turbine blade files
  int numColumns = 0;

  // test first line in turbine tower file to see if it has at least 13th column
  // if so then this is indication of "new" format
  if (inStr.getline(inBuf, LINE_SIZE)) {
    size_t len = strlen(inBuf);
    // number of lines corresponds to number of spaces
    for (size_t i = 0; i < len; i++)
      if (inBuf[i] == ' ')
        numColumns++;
  }
  else
    std::cout << fileName.str().c_str() << " is empty!\n";
  // reset seek position
  inStr.seekg(ios_base::beg);

  while (inStr.getline(inBuf, LINE_SIZE)) {

    istringstream line(inBuf);
    line >> towerID >> hubHeight >> bladeLength >> numberOfBlades >> maxRPM;
    line >> xPos >> yPos;
    line >> yawAngle >> angularVelocity >> angleBlade1;

    this->XPosition->InsertNextValue(xPos);
    this->YPosition->InsertNextValue(yPos);
    this->HubHeight->InsertNextValue(hubHeight);
    this->BladeCount->InsertNextValue(numberOfBlades);
  }
  this->NumberOfBladeTowers = XPosition->GetNumberOfTuples();
  inStr.close();

  // Calculate the number of cells in unstructured turbine blades
  ostringstream fileName2;
  fileName2 << this->RootDirectory << Slash
            << this->TurbineDirectory << Slash
            << this->TurbineBladeName << this->TimeStepFirst;
  ifstream inStr2(fileName2.str().c_str());
  if (!inStr2) {
    cout << "Could not open blade file: " << fileName2.str().c_str() <<
            " to calculate blade cells...\n";
    for (int i = this->TimeStepFirst + this->TimeStepDelta; i <= this->TimeStepLast;
         i += this->TimeStepDelta) {
      ostringstream fileName3;
      fileName3 << this->RootDirectory << Slash
                  << this->TurbineDirectory << Slash
                  << this->TurbineBladeName << i;
      std::cout << "Trying " << fileName3.str().c_str() << "...";
      inStr2.open(fileName3.str().c_str());
      if(inStr2.good()) {
        std::cout << "success.\n";
        break;
      }
      else
        std::cout << "failure.\n";
    }
  }

  this->NumberOfBladeCells = 0;
  // if we have at least 13 columns, then this is the new format with a header in the
  // turbine blade file
  if (numColumns >= 13 && inStr2) {
    int linesSkipped = 0;
    // each blade tower tries to split the columns such that there are
    // five items per line in header, so skip those lines
    this->NumberLinesToSkip = this->NumberOfBladeTowers*(int)ceil(numColumns/5.0);
    // now skip the first few lines based on header, if that applies
    while(inStr2.getline(inBuf, LINE_SIZE) && linesSkipped < NumberLinesToSkip)
      linesSkipped++;
  }
  while (inStr2.getline(inBuf, LINE_SIZE))
    this->NumberOfBladeCells++;
  inStr2.close();
  this->NumberOfBladePoints = this->NumberOfBladeCells * NUM_PART_SIDES;

  // Points and cells needed for constant towers
  this->NumberOfBladePoints += this->NumberOfBladeTowers * NUM_BASE_SIDES;
  this->NumberOfBladeCells += this->NumberOfBladeTowers;
}

//----------------------------------------------------------------------------
// Build the turbine blades
//----------------------------------------------------------------------------
void vtkWindBladeReader::LoadBladeData(int timeStep)
{
  this->BPoints->Delete();
  this->BPoints = vtkPoints::New();

  // Open the file for this time step
  ostringstream fileName;
  fileName << this->RootDirectory << Slash
           << this->TurbineDirectory << Slash
           << this->TurbineBladeName
           << this->TimeSteps[timeStep];
  ifstream inStr(fileName.str().c_str());
  char inBuf[LINE_SIZE];

  // Allocate space for points and cells
  this->BPoints->Allocate(this->NumberOfBladePoints, this->NumberOfBladePoints);
  vtkUnstructuredGrid* blade = GetBladeOutput();
  blade->Allocate(this->NumberOfBladeCells, this->NumberOfBladeCells);
  blade->SetPoints(this->BPoints);

  // Allocate space for data
  vtkFloatArray* axialForce = vtkFloatArray::New();
  axialForce->SetName("Axial Force");
  axialForce->SetNumberOfTuples(this->NumberOfBladeCells);
  axialForce->SetNumberOfComponents(1);
  blade->GetCellData()->AddArray(axialForce);
  float* aBlock = axialForce->GetPointer(0);

  vtkFloatArray* radialForce = vtkFloatArray::New();
  radialForce->SetName("Radial Force");
  radialForce->SetNumberOfTuples(this->NumberOfBladeCells);
  radialForce->SetNumberOfComponents(1);
  blade->GetCellData()->AddArray(radialForce);
  float* rBlock = radialForce->GetPointer(0);

  vtkFloatArray* test = vtkFloatArray::New();
  test->SetName("Test");
  test->SetNumberOfTuples(this->NumberOfBladeCells);
  test->SetNumberOfComponents(1);
  blade->GetCellData()->AddArray(test);
  float* tBlock = test->GetPointer(0);

  // File is ASCII text so read until EOF
  int index = 0;
  int indx = 0;
  int firstPoint;
  int turbineID, bladeID, partID;
  float x, y, z;
  vtkIdType cell[NUM_BASE_SIDES];

  int linesRead = 0;
  while (inStr.getline(inBuf, LINE_SIZE)) {
    // continue around header if need be
    linesRead++;
    if (linesRead <= this->NumberLinesToSkip) continue;
    istringstream line(inBuf);
    line >> turbineID >> bladeID >> partID;

    firstPoint = index;
    for (int side = 0; side < NUM_PART_SIDES; side++) {
      line >> x >> y >> z;
      this->BPoints->InsertNextPoint(x, y, z);
    }

    // Polygon points are leading edge then trailing edge so points are 0-1-3-2
    cell[0] = firstPoint;
    cell[1] = firstPoint + 1;
    cell[2] = firstPoint + 3;
    cell[3] = firstPoint + 2;
    index += NUM_PART_SIDES;
    blade->InsertNextCell(VTK_POLYGON, NUM_PART_SIDES, cell);

    line >> aBlock[indx] >> rBlock[indx];
    tBlock[indx] = turbineID * bladeID;
    indx++;
  }

  // Add the towers to the geometry
  for (int i = 0; i < this->NumberOfBladeTowers; i++) {
    x = this->XPosition->GetValue(i);
    y = this->YPosition->GetValue(i);
    z = this->HubHeight->GetValue(i);

    this->BPoints->InsertNextPoint(x-2, y-2, 0.0);
    this->BPoints->InsertNextPoint(x+2, y-2, 0.0);
    this->BPoints->InsertNextPoint(x+2, y+2, 0.0);
    this->BPoints->InsertNextPoint(x-2, y+2, 0.0);
    this->BPoints->InsertNextPoint(x, y, z);
    firstPoint = index;
    cell[0] = firstPoint;
    cell[1] = firstPoint + 1;
    cell[2] = firstPoint + 2;
    cell[3] = firstPoint + 3;
    cell[4] = firstPoint + 4;
    index += NUM_BASE_SIDES;
    blade->InsertNextCell(VTK_PYRAMID, NUM_BASE_SIDES, cell);

    aBlock[indx] = 0.0;
    rBlock[indx] = 0.0;
    tBlock[indx] = 0.0;
    indx++;
  }
  axialForce->Delete();
  radialForce->Delete();
  test->Delete();
}

//----------------------------------------------------------------------------
void vtkWindBladeReader::SelectionCallback(
  vtkObject*, unsigned long vtkNotUsed(eventid), void* clientdata, void* vtkNotUsed(calldata))
{
  static_cast<vtkWindBladeReader*>(clientdata)->Modified();
}

//----------------------------------------------------------------------------
vtkStructuredGrid* vtkWindBladeReader::GetFieldOutput()
{
  return vtkStructuredGrid::SafeDownCast(
    this->GetExecutive()->GetOutputData(0));
}

//----------------------------------------------------------------------------
int vtkWindBladeReader::GetNumberOfPointArrays()
{
  return this->PointDataArraySelection->GetNumberOfArrays();
}

//----------------------------------------------------------------------------
void vtkWindBladeReader::EnableAllPointArrays()
{
    this->PointDataArraySelection->EnableAllArrays();
}

//----------------------------------------------------------------------------
void vtkWindBladeReader::DisableAllPointArrays()
{
    this->PointDataArraySelection->DisableAllArrays();
}

//----------------------------------------------------------------------------
const char* vtkWindBladeReader::GetPointArrayName(int index)
{
  return this->VariableName[index].c_str();
}

//----------------------------------------------------------------------------
int vtkWindBladeReader::GetPointArrayStatus(const char* name)
{
  return this->PointDataArraySelection->ArrayIsEnabled(name);
}

//----------------------------------------------------------------------------
void vtkWindBladeReader::SetPointArrayStatus(const char* name, int status)
{
  if (status)
    this->PointDataArraySelection->EnableArray(name);
  else
    this->PointDataArraySelection->DisableArray(name);
}

vtkUnstructuredGrid *vtkWindBladeReader::GetBladeOutput()
{
  if (this->GetNumberOfOutputPorts() < 2)
    {
    return NULL;
    }
  return vtkUnstructuredGrid::SafeDownCast(
    this->GetExecutive()->GetOutputData(1));
}

//----------------------------------------------------------------------------
int vtkWindBladeReader::FillOutputPortInformation(int port,
                                                  vtkInformation* info)
{
  if(port == 0)
    {
    return this->Superclass::FillOutputPortInformation(port, info);
    }
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkUnstructuredGrid");
  return 1;
}
