#include "vtkMPASReader.h"

#include "vtkCallbackCommand.h"
#include "vtkCellData.h"
#include "vtkCellType.h"
#include "vtkDataArraySelection.h"
#include "vtkDataObject.h"
#include "vtkErrorCode.h"
#include "vtkFloatArray.h"
#include "vtkInformation.h"
#include "vtkInformationDoubleVectorKey.h"
#include "vtkInformationVector.h"
#include "vtkIntArray.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStringArray.h"
#include "vtkTableExtentTranslator.h"
#include "vtkToolkits.h"
#include "vtkUnstructuredGrid.h"

#include "vtk_netcdfcpp.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include "stdlib.h"
#include <string>
#include <cmath>
#include <cfloat>
#include <algorithm>

using namespace std;

class vtkInternals {
public:
  vtkInternals() :
    ncFile(NULL),
    Time(NULL),
    nCells(NULL),
    nVertices(NULL), vertexDegree(NULL),
    nVertLevels(NULL)
  {
    for (int i = 0; i < MAX_VARS; i++)
      {
      dualCellVars[i] = NULL;
      dualPointVars[i] = NULL;
      }
  };

  NcFile* ncFile;
  NcDim* Time;
  NcDim* nCells;
  NcDim* nVertices;
  NcDim* vertexDegree;
  NcDim* nVertLevels;
  NcVar* dualCellVars[MAX_VARS];
  NcVar* dualPointVars[MAX_VARS];
};

#define CHECK_MALLOC(ptr) \
  if (ptr == NULL)                  \
    {                               \
    cerr << "malloc failed!\n";     \
    return(0);                      \
    }

#define CHECK_DIM(ncFile, name) \
  if (!isNcDim(ncFile, name))                                         \
    {                                                                 \
    vtkErrorMacro( << "Cannot find dimension: " << name << endl);     \
    return 0;                                                         \
    }

#define CHECK_VAR(ncFile, name)                                      \
  if (!isNcVar(ncFile, name))                                        \
    {                                                                \
    vtkErrorMacro( << "Cannot find variable: " << name << endl);     \
    return 0;                                                        \
    }

// Check if there is a variable by that name

bool isNcVar(NcFile *ncFile, NcToken name)
{
  int num_vars = ncFile->num_vars();
  for (int i = 0; i < num_vars; i++)
    {
    NcVar* ncVar = ncFile->get_var(i);
    if ((strcmp(ncVar->name(), name)) == 0)
      {
      // we have a match, so return
      return true;
      }
    }
  return false;
}

// Check if there is a dimension by that name

bool isNcDim(NcFile *ncFile, NcToken name)
{
  int num_dims = ncFile->num_dims();
  //cerr << "looking for: " << name << endl;
  for (int i = 0; i < num_dims; i++)
    {
    NcDim* ncDim = ncFile->get_dim(i);
    //cerr << "checking " << ncDim->name() << endl;
    if ((strcmp(ncDim->name(), name)) == 0)
      {
      // we have a match, so return
      return true;
      }
    }
  return false;
}


int my_isinf_f(float x)
{
  if (vtkMath::IsInf((double)x))
    {
    return (x < 0.0 ? -1 : 1);
    }

  return 0;
}

float convertDouble2ValidFloat(double inputData)
{

  // check for NaN
  if (inputData != inputData)
    {
    cerr << "found NaN!" << endl;
    return -FLT_MAX;
    }

  // check for infinity
  int retval = my_isinf_f((float)inputData);
  if (retval < 0)
    {
    return -FLT_MAX;
    }
  else if (retval > 0)
    {
    return FLT_MAX;
    }

  // check number too small for float
  if (abs(inputData) < 1e-126)
    {
    return 0.0;
    }

  if ((float)inputData == 0)
    {
    return 0.0;
    }

  if ((abs(inputData) > 0) && (abs(inputData) < FLT_MIN))
    {
    if (inputData < 0)
      {
      return -FLT_MIN;
      }
    else
      {
      return FLT_MIN;
      }
    }

  if (abs(inputData) > FLT_MAX)
    {
    if (inputData < 0)
      {
      return -FLT_MAX;
      }
    else
      {
      return FLT_MAX;
      }
    }

  return (float)inputData;
}

vtkStandardNewMacro(vtkMPASReader);

//----------------------------------------------------------------------------
// Constructor for vtkMPASReader
//----------------------------------------------------------------------------

vtkMPASReader::vtkMPASReader()
{
  this->Internals = new vtkInternals;

  // Debugging
  //this->DebugOn();
  vtkDebugMacro(<< "Starting to create vtkMPASReader..." << endl);

  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);

  this->FileName = NULL;

  this->infoRequested = false;
  this->dataRequested = false;

  this->dTime = 0;

  this->PointDataArraySelection = vtkDataArraySelection::New();
  this->CellDataArraySelection = vtkDataArraySelection::New();

  this->NumberOfDualPoints = 0;
  this->NumberOfDualCells = 0;
  this->NumberOfVariables = 0;

  this->primalPointVarData = NULL;
  this->primalCellVarData = NULL;

  this->dualCellVarData = NULL;
  this->dualPointVarData = NULL;

  // put in defaults
  this->VerticalLevelRange[0] = 0;
  this->VerticalLevelRange[1] = 1;
  this->VerticalLevelSelected = 0;

  this->TimeSteps = NULL;

  // Setup selection callback to modify this object when array selection changes
  this->SelectionObserver = vtkCallbackCommand::New();
  this->SelectionObserver->SetCallback(&vtkMPASReader::SelectionCallback);
  this->SelectionObserver->SetClientData(this);
  this->CellDataArraySelection->AddObserver(vtkCommand::ModifiedEvent,
                                            this->SelectionObserver);
  this->PointDataArraySelection->AddObserver(vtkCommand::ModifiedEvent,
                                             this->SelectionObserver);

  vtkDebugMacro(<< "MAX_VARS:" << MAX_VARS << endl);

  vtkDebugMacro(<< "Created vtkMPASReader" << endl);
}

//----------------------------------------------------------------------------
// Destructor for MPAS Reader
//----------------------------------------------------------------------------

vtkMPASReader::~vtkMPASReader()
{
  vtkDebugMacro(<< "Destructing vtkMPASReader..." << endl);

  if (this->FileName)
    {
    delete [] this->FileName;
    }

  if (this->Internals->ncFile)
    {
    delete this->Internals->ncFile;
    }

  vtkDebugMacro(<< "Destructing cell var array..." << endl);
  if (this->dualCellVarData)
    {
    for (int i = 0; i < this->numDualCellVars; i++)
      {
      if (this->dualCellVarData[i] != NULL)
        {
        this->dualCellVarData[i]->Delete();
        }
      }
    delete [] this->dualCellVarData;
    }

  vtkDebugMacro(<< "Destructing point var array..." << endl);
  if (this->dualPointVarData)
    {
    for (int i = 0; i < this->numDualPointVars; i++)
      {
      if (this->dualPointVarData[i] != NULL)
        {
        this->dualPointVarData[i]->Delete();
        }
      }
    delete [] this->dualPointVarData;
    }

  vtkDebugMacro(<< "Destructing other stuff..." << endl);
  if (this->primalCellVarData)
    {
    free (this->primalCellVarData);
    }
  if (this->primalPointVarData)
    {
    free (this->primalPointVarData);
    }
  if (this->PointDataArraySelection)
    {
    this->PointDataArraySelection->Delete();
    }
  if (this->CellDataArraySelection)
    {
    this->CellDataArraySelection->Delete();
    }
  if (this->SelectionObserver)
    {
    this->SelectionObserver->Delete();
    }

  delete this->Internals;

  vtkDebugMacro(<< "Destructed vtkMPASReader" << endl);
}

//----------------------------------------------------------------------------
// Verify that the file exists, get dimension sizes and variables
//----------------------------------------------------------------------------

int vtkMPASReader::RequestInformation(
                                      vtkInformation *reqInfo,
                                      vtkInformationVector **inVector,
                                      vtkInformationVector *outVector)
{
  vtkDebugMacro(<< "In vtkMPASReader::RequestInformation" << endl);

  if (!this->Superclass::RequestInformation(reqInfo, inVector, outVector))
    {
    return 0;
    }

  // Verify that file exists
  if ( !this->FileName )
    {
    vtkErrorMacro("No filename specified");
    return 0;
    }

  vtkDebugMacro
    (<< "In vtkMPASReader::RequestInformation read filename okay" << endl);
  // Get ParaView information pointer
  vtkInformation* outInfo = outVector->GetInformationObject(0);

  // RequestInformation() is called for every Modified() event which means
  // when more variable data is selected it will be called again

  if (!this->infoRequested)
    {
    this->infoRequested = true;

    vtkDebugMacro(<< "FileName: " << this->FileName << endl);
    this->Internals->ncFile = new NcFile(this->FileName);

    if (!this->Internals->ncFile->is_valid())
      {
      vtkErrorMacro( << "Couldn't open file: " << this->FileName << endl);
      return 0;
      }

    vtkDebugMacro
      (<< "In vtkMPASReader::RequestInformation read file okay" << endl);

    CHECK_DIM(this->Internals->ncFile, "nCells");
    CHECK_DIM(this->Internals->ncFile, "nVertices");
    CHECK_DIM(this->Internals->ncFile, "vertexDegree");
    CHECK_DIM(this->Internals->ncFile, "Time");
    CHECK_DIM(this->Internals->ncFile, "nVertLevels");

    this->Internals->nCells = this->Internals->ncFile->get_dim("nCells");
    this->Internals->nVertices = this->Internals->ncFile->get_dim("nVertices");
    this->Internals->vertexDegree =
      this->Internals->ncFile->get_dim("vertexDegree");
    this->Internals->Time = this->Internals->ncFile->get_dim("Time");
    this->Internals->nVertLevels =
      this->Internals->ncFile->get_dim("nVertLevels");

    vtkDebugMacro
      (<< "In vtkMPASReader::RequestInformation setting VerticalLevelRange"
       << endl);

    this->VerticalLevelRange[0] = 0;
    if (this->Internals->nVertLevels != NULL)
      {
      this->VerticalLevelRange[1] = this->Internals->nVertLevels->size()-1;
      }
    else
      {
      this->VerticalLevelRange[1] = 1;
      }

    if (!BuildVarArrays())
      {
      return 0;
      }

    // Allocate the data arrays which will hold the NetCDF var data

    this->primalCellVarData = (double*)
      malloc(sizeof(double)*this->Internals->nCells->size());
    CHECK_MALLOC(this->primalCellVarData);
    this->primalPointVarData = (double*)
      malloc(sizeof(double)*this->Internals->nVertices->size());
    CHECK_MALLOC(this->primalPointVarData);

    // Allocate the ParaView data arrays which will hold the variables
    this->dualPointVarData = new vtkFloatArray*[this->numDualPointVars];
    for (int i = 0; i < this->numDualPointVars; i++)
      {
      this->dualPointVarData[i] = NULL;
      }
    this->dualCellVarData = new vtkFloatArray*[this->numDualCellVars];
    for (int i = 0; i < this->numDualCellVars; i++)
      {
      this->dualCellVarData[i] = NULL;
      }

    // Start with no data loaded into ParaView
    DisableAllPointArrays();
    DisableAllCellArrays();

    this->NumberOfDualCells = this->Internals->nVertices->size();
    this->NumberOfDualPoints = this->Internals->nCells->size() + 1;

    // Collect temporal information
    this->NumberOfTimeSteps = this->Internals->Time->size();

    // At this time, MPAS doesn't have fine-grained time value, just
    // the number of the step, so that is what I store here for TimeSteps.
    if (this->TimeSteps != NULL)
      {
      delete[] this->TimeSteps;
      }
    this->TimeSteps = new double[this->NumberOfTimeSteps];
    for (int step = 0; step < this->NumberOfTimeSteps; step++)
      {
      this->TimeSteps[step] = (double) step;
      }
    // Tell the pipeline what steps are available
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(),
                 this->TimeSteps, this->NumberOfTimeSteps);

    double tRange[2];
    tRange[0] = this->TimeSteps[0];
    tRange[1] = this->TimeSteps[this->NumberOfTimeSteps-1];
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(),
                 tRange,
                 2);
    }

  return 1;
}

int vtkMPASReader::BuildVarArrays()
{
  vtkDebugMacro(<< "In vtkMPASReader::BuildVarArrays" << endl);

  // figure out what variables to visualize -
  int dualCellVarIndex = -1;
  int dualPointVarIndex = -1;

  int numVars = this->Internals->ncFile->num_vars();

  bool tracersExist = false;

  for (int i = 0; i < numVars; i++)
    {
    NcVar* aVar = this->Internals->ncFile->get_var(i);

    // must have 3 dims (Time, nCells|nVertices, nVertLevels)

    int numDims = aVar->num_dims();
    vtkDebugMacro
      ( << "Num Dims of var: " << aVar->name() << " is " << numDims << endl);
    if ((numDims != 3) && (strcmp(aVar->name(), "tracers")))
      {
      continue; // try the next var
      }
    else
      {
      // TODO, check if it is a double
      // assume a double for now

      // check for Time dim
      NcToken dim0Name = aVar->get_dim(0)->name();
      if (strcmp(dim0Name, "Time"))
        {
        continue;
        }

      // check for dim 1 being num vertices or cells
      bool isVertexData = false;
      bool isCellData = false;
      NcToken dim1Name = aVar->get_dim(1)->name();
      if (!strcmp(dim1Name, "nVertices"))
        {
        isVertexData = true;
        }
      else if (!strcmp(dim1Name, "nCells"))
        {
        isCellData = true;
        }
      else
        {
        continue;
        }

      // check if dim 2 is nVertLevels or nVertLevelsP1, too
      NcToken dim2Name = aVar->get_dim(2)->name();
      if ((strcmp(dim2Name, "nVertLevels"))
          && (strcmp(dim2Name, "nVertLevelsP1")))
        {
        continue;
        }

      /*
      // check if we have data for the selected level
      if ((aVar->get_dim(2)->size()-1 < this->VerticalLevelSelected)
      || (this->VerticalLevelSelected < 0)) {
      //cout << "No data found for level ";
      //cout << outputVertLevel << " for variable ";
      //cout << aVar->name() << endl;
      continue;
      }
      */

      // Add to cell or point var array
      if (isVertexData)
        {  // means it is dual cell data
        dualCellVarIndex++;
        if (dualCellVarIndex > (MAX_VARS-1))
          {
          vtkDebugMacro(<< "too many dual cell vars!" << endl);
          exit(0);
          }
        this->Internals->dualCellVars[dualCellVarIndex] = aVar;
        //cout << "Adding var " << aVar->name() << " to dualCellVars" << endl;
        }
      else if (isCellData)
        { // means it is dual vertex data
        if (strcmp(aVar->name(), "tracers"))
          {
          dualPointVarIndex++;
          if (dualPointVarIndex > MAX_VARS-1)
            {
            cerr << "Exceeded number of point vars." << endl;
            exit(1);
            }
          this->Internals->dualPointVars[dualPointVarIndex] = aVar;
          //cout<< "Adding var " << aVar->name() << " to dualPointVars" << endl;
          }
        else
          { // case of tracers, add each as "tracer0", "tracer1", etc.
          tracersExist = true;
          int numTracers = aVar->get_dim(3)->size();
          for (int t = 0; t < numTracers; t++)
            {
            dualPointVarIndex++;
            if (dualPointVarIndex > MAX_VARS-1)
              {
              vtkDebugMacro(<< "too many point vars!" << endl);
              exit(1);
              }
            this->Internals->dualPointVars[dualPointVarIndex] = aVar;
            //cout<<"Adding var "<< aVar->name() << " to dualPointVars" << endl;
            ostringstream tracerName;
            tracerName << "tracer" << t+1;
            strcpy(tracerNames[t], tracerName.str().c_str());
            }
          }
        }
      }
    }

  this->numDualCellVars = dualCellVarIndex+1;
  this->numDualPointVars = dualPointVarIndex+1;

  vtkDebugMacro( << "numDualCellVars: " << this->numDualCellVars
                 << " numDualPointVars: " << this->numDualPointVars << endl);

  int tracerNum = 0;

  for (int var = 0; var < this->numDualPointVars; var++)
    {
    if (!strcmp(this->Internals->dualPointVars[var]->name(), "tracers") )
      {
      this->PointDataArraySelection->
        EnableArray((const char*)(this->tracerNames[tracerNum]));
      vtkDebugMacro(<< "Adding point var: " << tracerNames[tracerNum] << endl);
      tracerNum++;
      }
    else
      {
      this->PointDataArraySelection->
        EnableArray((const char*)(this->Internals->dualPointVars[var]->name()));
      vtkDebugMacro
        (<< "Adding point var: "
         << this->Internals->dualPointVars[var]->name() << endl);
      }
    }

  for (int var = 0; var < this->numDualCellVars; var++)
    {
    vtkDebugMacro
      (<< "Adding cell var: "
       << this->Internals->dualCellVars[var]->name() << endl);
    this->CellDataArraySelection->
      EnableArray((const char*)(this->Internals->dualCellVars[var]->name()));
    }

  vtkDebugMacro(<< "Leaving vtkMPASReader::BuildVarArrays" << endl);

  return(1);
}


//----------------------------------------------------------------------------
// Data is read into a vtkUnstructuredGrid
//----------------------------------------------------------------------------

    int vtkMPASReader::RequestData(
            vtkInformation *vtkNotUsed(reqInfo),
            vtkInformationVector **vtkNotUsed(inVector),
            vtkInformationVector *outVector)
{
    vtkDebugMacro(<< "In vtkMPASReader::RequestData" << endl);


    // get the info object
    vtkInformation *outInfo = outVector->GetInformationObject(0);

    // Output will be an ImageData
    vtkUnstructuredGrid *output = vtkUnstructuredGrid::SafeDownCast(
            outInfo->Get(vtkDataObject::DATA_OBJECT()));

    // Output the unstructured grid from the netCDF file
    if (!ReadAndOutputDualGrid()) return 0;

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

    // print out how many steps are requested, just for my information
    vtkDebugMacro
      ( << "Num Time steps requested: " << numRequestedTimeSteps << endl);

    // At this time, it seems to only get one timestep of info, why?

    this->dTime = requestedTimeSteps[0];
    vtkDebugMacro(<< "this->dTime: " << this->dTime << endl);
    double dTimeTemp = this->dTime;
    output->GetInformation()->Set
      (vtkDataObject::DATA_TIME_STEPS(), &dTimeTemp, 1);
    vtkDebugMacro(<< "dTimeTemp: " << dTimeTemp << endl);
    this->dTime = dTimeTemp;

    // Examine each variable to see if it is selected
    for (int var = 0; var < this->numDualPointVars; var++) {

        // Is this variable requested
        if (this->PointDataArraySelection->GetArraySetting(var)) {
            vtkDebugMacro( << "Loading Point Variable: " << var << endl);
            LoadPointVarData(var, dTime);
            output->GetPointData()->AddArray(this->dualPointVarData[var]);

        }
    }

    for (int var = 0; var < this->numDualCellVars; var++) {
        if (this->CellDataArraySelection->GetArraySetting(var)) {
            vtkDebugMacro
              ( << "Loading Cell Variable: "
                << this->Internals->dualCellVars[var]->name() << endl);
            LoadCellVarData(var, dTime);
            output->GetCellData()->AddArray(this->dualCellVarData[var]);

        }
    }

    this->dataRequested = true;

    vtkDebugMacro( << "Returning from RequestData" << endl);
    return 1;
}

int vtkMPASReader::ReadAndOutputDualGrid()
{

    vtkDebugMacro(<< "In vtkMPASReader::ReadAndOutputDualGrid" << endl);

    // read points  (centers of primal-mesh cells)

    CHECK_VAR(this->Internals->ncFile, "xCell");
    double *xCellData = (double*)malloc(this->Internals->nCells->size()
            * sizeof(double));
    CHECK_MALLOC(xCellData);
    NcVar *xCellVar = this->Internals->ncFile->get_var("xCell");
    xCellVar->get(xCellData, this->Internals->nCells->size());

    CHECK_VAR(this->Internals->ncFile, "yCell");
    double *yCellData = (double*)malloc(this->Internals->nCells->size()
            * sizeof(double));
    CHECK_MALLOC(yCellData);
    NcVar *yCellVar = this->Internals->ncFile->get_var("yCell");
    yCellVar->get(yCellData, this->Internals->nCells->size());

    CHECK_VAR(this->Internals->ncFile, "zCell");
    double *zCellData = (double*)malloc(this->Internals->nCells->size()
            * sizeof(double));
    //cout << "ptr for zCellData"  << zCellData << endl;
    CHECK_MALLOC(zCellData);
    NcVar *zCellVar = this->Internals->ncFile->get_var("zCell");
    zCellVar->get(zCellData, this->Internals->nCells->size());

    // read dual-mesh cells  (triangles formed by primal-mesh cell centers)

    // cellsOnVertex is a 2D array of cell numbers
    // of dimensions (nVertices X vertexDegree)
    CHECK_VAR(this->Internals->ncFile, "cellsOnVertex");
    int *cellsOnVertex = (int *) malloc((this->Internals->nVertices->size()) *
            this->Internals->vertexDegree->size() * sizeof(int));
    //cout << "ptr for cellsOnVertex"  << cellsOnVertex << endl;
    CHECK_MALLOC(cellsOnVertex);
    NcVar *cellsOnVertexVar = this->Internals->ncFile->get_var("cellsOnVertex");
    //cout << "getting cellsOnVertexVar\n";
    cellsOnVertexVar->get(cellsOnVertex, this->Internals->nVertices->size(),
            this->Internals->vertexDegree->size());


    vtkUnstructuredGrid* output = GetOutput();

    vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
    points->Allocate(this->NumberOfDualPoints, this->NumberOfDualPoints);

    // first write a dummy point, because the climate code
    // starts their cell numbering at 1 and VTK starts it at
    // 0
    points->InsertNextPoint(0, 0, 0);

    for (int i = 0; i < this->Internals->nCells->size(); i++) {
        points->InsertNextPoint(xCellData[i], yCellData[i], zCellData[i]);
    }

    output->SetPoints(points);

    free(xCellData);
    free(yCellData);
    free(zCellData);

    // Write dual-mesh cells
    // Dual-mesh cells are triangles with primal-mesh cell
    // centers as the vertices.
    // The number of dual-mesh cells is the number of vertices in the
    // primal mesh.

    // for each dual-mesh cell

    // write cell types
    int cellType;
    if (this->Internals->vertexDegree->size() == 3)
        cellType = VTK_TRIANGLE;
    else if (this->Internals->vertexDegree->size() == 4)
        cellType = VTK_QUAD;
    else cellType = VTK_POLYGON;

    output->Allocate(this->NumberOfDualCells, this->NumberOfDualCells);

    vtkIdType* polygon = new vtkIdType[this->Internals->vertexDegree->size()];
    for (int i = 0; i < this->Internals->nVertices->size(); i++)
      {

      // since primal vertex(pt) numbers  == dual cell numbers
      // we go through the primal vertices, find the cells around
      // them, and since those primal cell numbers are dual pt numbers
      // we can write the cell numbers for the cellsOnVertex
      // and those will be the numbers of the dual vertices (pts).

      int* dualMeshCells = cellsOnVertex +
        (i * this->Internals->vertexDegree->size());
      for (int j = 0; j < this->Internals->vertexDegree->size(); j++)
        {
        polygon[j] = dualMeshCells[j];
        }

      // InsertNextCell(type, number of points, array of points)
      output->InsertNextCell(cellType,
                             this->Internals->vertexDegree->size(), polygon);
      }
    free(cellsOnVertex);
    free(polygon);

    vtkDebugMacro(<< "Leaving vtkMPASReader::ReadAndOutputDualGrid" << endl);

    return(1);
}

int vtkMPASReader::LoadPointVarData(int variableIndex, double dTimeStep)
{

    vtkDebugMacro(<< "In vtkMPASReader::LoadPointVarData" << endl);

    NcVar* ncVar = this->Internals->dualPointVars[variableIndex];

    vtkDebugMacro(<< "got ncVar in vtkMPASReader::LoadPointVarData" << endl);
    if (ncVar == NULL) {
        cerr << "Can't find data for variable " << variableIndex << endl;
        return 0;
    }

    // Allocate data array for this variable

    bool isTracer = false;
    int tracerNum = 0;

    // if it is a tracer var
    if (!strcmp(this->Internals->dualPointVars[variableIndex]->name(),
                "tracers"))
      {
      isTracer = true;

      // find its tracer number
      int firstTracerIndex = 0;
      for (int v = 0; v < numDualPointVars; v++)
        {
        if (!strcmp(this->Internals->dualPointVars[v]->name(), "tracers"))
          {
          firstTracerIndex = v;
          break;
          }
        }
      tracerNum = variableIndex - firstTracerIndex;
      }

    vtkDebugMacro
      (<< "isTracer: " << isTracer << " and tracerNum: " << tracerNum << endl);

    if (this->dualPointVarData[variableIndex] == NULL)
      {
      vtkDebugMacro
        (<< "allocating data array in vtkMPASReader::LoadPointVarData" << endl);
      this->dualPointVarData[variableIndex] = vtkFloatArray::New();
      if (isTracer)
        {
        this->dualPointVarData[variableIndex]->SetName(tracerNames[tracerNum]);
        vtkDebugMacro(<< "set name to : " << (tracerNames[tracerNum]) << endl);
        }
      else
        {
        this->dualPointVarData[variableIndex]->SetName
          (this->Internals->dualPointVars[variableIndex]->name());
        }
      this->dualPointVarData[variableIndex]->SetNumberOfTuples
        (this->NumberOfDualPoints);
      this->dualPointVarData[variableIndex]->SetNumberOfComponents(1);
      }

    vtkDebugMacro(<< "getting pointer in vtkMPASReader::LoadPointVarData"
                  << endl);
    float* dataBlock = this->dualPointVarData[variableIndex]->GetPointer(0);

    // allocate for doubles, will convert to vtkFloatArray
    vtkDebugMacro( << "dTimeStep requested: " << dTimeStep << endl);
    int timestep = min((int)floor(dTimeStep),
                       (int)(this->Internals->Time->size()-1));
    vtkDebugMacro( << "Time: " << timestep << endl);

    if (isTracer)
      {
      this->Internals->dualPointVars[variableIndex]->set_cur
        (timestep, 0, this->VerticalLevelSelected, tracerNum);
      this->Internals->dualPointVars[variableIndex]->get
        (primalCellVarData, 1, this->Internals->nCells->size(), 1, 1);
      }
    else
      {
      this->Internals->dualPointVars[variableIndex]->set_cur
        (timestep, 0, this->VerticalLevelSelected);
      this->Internals->dualPointVars[variableIndex]->get
        (primalCellVarData, 1, this->Internals->nCells->size(), 1);
      }

    vtkDebugMacro
      (<< "got point data in vtkMPASReader::LoadPointVarData" << endl);

    double *var_target = this->primalCellVarData;

    // write dummy, just make it the same as the first elt, so we stay in range.
    float validData;
    validData = convertDouble2ValidFloat (*var_target);

    dataBlock[0] = validData;

    for (int j = 0; j < this->Internals->nCells->size(); j++)
      {
      validData = convertDouble2ValidFloat (*var_target);
      dataBlock[j+1] =  validData;
      var_target ++;
      }

    vtkDebugMacro
      (<< "converted and stored point data in vtkMPASReader::LoadPointVarData"
       << endl);
    return (1);
}


int vtkMPASReader::LoadCellVarData(int variableIndex, double dTimeStep)
{
  vtkDebugMacro(<< "In vtkMPASReader::LoadCellVarData" << endl);

  NcVar* ncVar = this->Internals->dualCellVars[variableIndex];

  if (ncVar == NULL)
    {
    cerr << "Can't find data for variable index:" << variableIndex << endl;
    return 0;
    }

  // Allocate data array for this variable
  if (this->dualCellVarData[variableIndex] == NULL)
    {
    this->dualCellVarData[variableIndex] = vtkFloatArray::New();
    vtkDebugMacro
      ( << "Allocated cell var index: "
        << this->Internals->dualCellVars[variableIndex]->name() << endl);
    this->dualCellVarData[variableIndex]->SetName
      (this->Internals->dualCellVars[variableIndex]->name());
    this->dualCellVarData[variableIndex]->SetNumberOfTuples
      (this->NumberOfDualCells);
    this->dualCellVarData[variableIndex]->SetNumberOfComponents(1);
    }

  vtkDebugMacro(<< "getting pointer in vtkMPASReader::LoadCellVarData" << endl);

  float* dataBlock = this->dualCellVarData[variableIndex]->GetPointer(0);

  vtkDebugMacro( << "dTimeStep requested: " << dTimeStep << endl);
  int timestep = min((int)floor(dTimeStep),
                     (int)(this->Internals->Time->size()-1));
  vtkDebugMacro( << "Time: " << timestep << endl);

  ncVar->set_cur(timestep, 0, this->VerticalLevelSelected);

  ncVar->get(this->primalPointVarData, 1, this->Internals->nVertices->size(), 1);

  vtkDebugMacro(<< "Got data for cell var: "
                << this->Internals->dualCellVars[variableIndex]->name()
                << endl);

  double *var_target = this->primalPointVarData;

  for (int j = 0; j < this->Internals->nVertices->size(); j++)
    {
    float validData = convertDouble2ValidFloat (*var_target);
    dataBlock[j] =  validData;
    var_target++;
    }

  vtkDebugMacro( << "Converted and stored data for cell var: "
                 << this->Internals->dualCellVars[variableIndex]->name() << endl);

  return(1);
}

//----------------------------------------------------------------------------
void vtkMPASReader::SelectionCallback(vtkObject*,
                                      unsigned long vtkNotUsed(eventid),
                                      void* clientdata,
                                      void* vtkNotUsed(calldata))
{
  static_cast<vtkMPASReader*>(clientdata)->Modified();
}

//----------------------------------------------------------------------------
vtkUnstructuredGrid* vtkMPASReader::GetOutput()
{
  return this->GetOutput(0);
}

//----------------------------------------------------------------------------
vtkUnstructuredGrid* vtkMPASReader::GetOutput(int idx)
{
  if (idx)
    {
    return NULL;
    }
  else
    {
    return vtkUnstructuredGrid::SafeDownCast( this->GetOutputDataObject(idx) );
    }
}

//----------------------------------------------------------------------------
int vtkMPASReader::GetNumberOfPointArrays()
{
  return this->PointDataArraySelection->GetNumberOfArrays();
}

//----------------------------------------------------------------------------
int vtkMPASReader::GetNumberOfCellArrays()
{
  return this->CellDataArraySelection->GetNumberOfArrays();
}

//----------------------------------------------------------------------------
void vtkMPASReader::EnableAllPointArrays()
{
  this->PointDataArraySelection->EnableAllArrays();
}

//----------------------------------------------------------------------------
void vtkMPASReader::DisableAllPointArrays()
{
  this->PointDataArraySelection->DisableAllArrays();
}

//----------------------------------------------------------------------------
void vtkMPASReader::EnableAllCellArrays()
{
  this->CellDataArraySelection->EnableAllArrays();
}

//----------------------------------------------------------------------------
void vtkMPASReader::DisableAllCellArrays()
{
  this->CellDataArraySelection->DisableAllArrays();
}

//----------------------------------------------------------------------------
const char* vtkMPASReader::GetPointArrayName(int index)
{

  // if it is a tracer var
  if (!strcmp(this->Internals->dualPointVars[index]->name(), "tracers"))
    {
    // find its tracer number
    int firstTracerIndex = 0;
    for (int v = 0; v < numDualPointVars; v++)
      {
      if (!strcmp(this->Internals->dualPointVars[v]->name(), "tracers"))
        {
        firstTracerIndex = v;
        break;
        }
      }
    int tracerNum = index - firstTracerIndex;
    vtkDebugMacro
      ( << "GetPointArrayName: " << tracerNames[tracerNum] << endl);
    return (const char*) tracerNames[tracerNum];
    }
  else
    {
    vtkDebugMacro( << "GetPointArrayName: "
                   << this->Internals->dualPointVars[index]->name() << endl);
    return (const char*)(this->Internals->dualPointVars[index]->name());
    }

}

//----------------------------------------------------------------------------
int vtkMPASReader::GetPointArrayStatus(const char* name)
{
  return this->PointDataArraySelection->ArrayIsEnabled(name);
}

//----------------------------------------------------------------------------
void vtkMPASReader::SetPointArrayStatus(const char* name, int status)
{
  if (status)
    {
    this->PointDataArraySelection->EnableArray(name);
    }
  else
    {
    this->PointDataArraySelection->DisableArray(name);
    }
}

//----------------------------------------------------------------------------
const char* vtkMPASReader::GetCellArrayName(int index)
{
  return (const char*)(this->Internals->dualCellVars[index]->name());
}

//----------------------------------------------------------------------------
int vtkMPASReader::GetCellArrayStatus(const char* name)
{
  return this->CellDataArraySelection->ArrayIsEnabled(name);
}

//----------------------------------------------------------------------------
void vtkMPASReader::SetCellArrayStatus(const char* name, int status)
{
  if (status)
    {
    this->CellDataArraySelection->EnableArray(name);
    }
  else
    {
    this->CellDataArraySelection->DisableArray(name);
    }
}


void vtkMPASReader::SetVerticalLevel(int level)
{
  this->VerticalLevelSelected = level;
  vtkDebugMacro( << "Set VerticalLevelSelected to: " << level << endl);

  vtkDebugMacro( << "infoRequested?: " << infoRequested << endl);

  if (!this->infoRequested) { return; }
  if (!this->dataRequested) { return; }

  // Examine each variable to see if it is selected
  for (int var = 0; var < this->numDualPointVars; var++)
    {
    // Is this variable requested
    if (this->PointDataArraySelection->GetArraySetting(var))
      {
      vtkDebugMacro( << "Loading Point Variable: "
                     << this->Internals->dualPointVars[var]->name() << endl);
      LoadPointVarData(var, dTime);
      }
    }

  for (int var = 0; var < this->numDualCellVars; var++)
    {
    if (this->CellDataArraySelection->GetArraySetting(var))
      {
      vtkDebugMacro( << "Loading Cell Variable: "
                     << this->Internals->dualCellVars[var]->name() << endl);
      LoadCellVarData(var, dTime);
      }
    }

  this->PointDataArraySelection->Modified();
  this->CellDataArraySelection->Modified();
}

//-----------------------------------------------------------------------------
int vtkMPASReader::CanReadFile(const char *filename)
{
  NcFile* ncFile = new NcFile(filename);
  if (!ncFile->is_valid())
    {
    return 0;
    }
  bool ret = true;
  ret &= isNcDim(ncFile, "nCells");
  ret &= isNcDim(ncFile, "nVertices");
  ret &= isNcDim(ncFile, "vertexDegree");
  ret &= isNcDim(ncFile, "Time");
  ret &= isNcDim(ncFile, "nVertLevels");
  return ret;
}

//-----------------------------------------------------------------------------
void vtkMPASReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FileName: " << (this->FileName?this->FileName:"NULL") << "\n";
  os << indent << "VerticalLevelRange: " << this->VerticalLevelRange << "\n";
  os << indent << "NumberOfVariables: " << this->NumberOfVariables << "\n";
  os << indent << "NumberOfDualPoints: " << this->NumberOfDualPoints << "\n";
  os << indent << "NumberOfDualCells: " << this->NumberOfDualCells << "\n";
}
