/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkMPASReader.h

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*=========================================================================

  Copyright (c) 2002-2005 Los Alamos National Laboratory

  This software and ancillary information known as vtk_ext (and herein
  called "SOFTWARE") is made available under the terms described below.
  The SOFTWARE has been approved for release with associated LA_CC
  Number 99-44, granted by Los Alamos National Laboratory in July 1999.

  Unless otherwise indicated, this SOFTWARE has been authored by an
  employee or employees of the University of California, operator of the
  Los Alamos National Laboratory under Contract No. W-7405-ENG-36 with
  the United States Department of Energy.

  The United States Government has rights to use, reproduce, and
  distribute this SOFTWARE.  The public may copy, distribute, prepare
  derivative works and publicly display this SOFTWARE without charge,
  provided that this Notice and any statement of authorship are
  reproduced on all copies.

  Neither the U. S. Government, the University of California, nor the
  Advanced Computing Laboratory makes any warranty, either express or
  implied, nor assumes any liability or responsibility for the use of
  this SOFTWARE.

  If SOFTWARE is modified to produce derivative works, such modified
  SOFTWARE should be clearly marked, so as not to confuse it with the
  version available from Los Alamos National Laboratory.

  =========================================================================*/

// Christine Ahrens (cahrens@lanl.gov)
// Version 1.3

/*=========================================================================
  NOTES
  When using this reader, it is important that you remember to do the following:
  1.  When changing a selected variable, remember to select it also in the drop
  down box to "color by".  It doesn't color by that variable automatically.
  2.  When selecting multilayer sphere view, make layer thickness around
  100,000.
  3.  When selecting multilayer lat/lon view, make layer thickness around 10.
  4.  Always click the -Z orientation after making a switch from lat/lon to
  sphere, from single to multilayer or changing thickness.
  5.  Be conservative on the number of changes you make before hitting Apply,
  since there may be bugs in this reader.  Just make one change and then hit
  Apply.

  =========================================================================*/

#include "vtkMPASReader.h"

#include "vtkCallbackCommand.h"
#include "vtkCellData.h"
#include "vtkCellType.h"
#include "vtkCellArray.h"
#include "vtkDataArraySelection.h"
#include "vtkDataObject.h"
#include "vtkErrorCode.h"
#include "vtkDoubleArray.h"
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

#define MAX_VARS 100
#define DEFAULT_LAYER_THICKNESS 10
double PI = 3.141592;


//----------------------------------------------------------------------------
// Internal class to avoid name pollution
//----------------------------------------------------------------------------

class vtkMPASReader::Internal {
  public:
    Internal() :
      ncFile(NULL)
  {
    for (int i = 0; i < MAX_VARS; i++)
    {
      cellVars[i] = NULL;
      pointVars[i] = NULL;
    }
  };

    NcFile* ncFile;
    NcVar* cellVars[MAX_VARS];
    NcVar* pointVars[MAX_VARS];
};


//----------------------------------------------------------------------------
// Macro to check malloc didn't return an error
//----------------------------------------------------------------------------

#define CHECK_MALLOC(ptr) \
  if (ptr == NULL)                  \
{                               \
  vtkErrorMacro( << "malloc failed!" << endl);     \
  return(0);                      \
}


//----------------------------------------------------------------------------
//  Macro to check if the named NetCDF dimension exists
//----------------------------------------------------------------------------

#define CHECK_DIM(ncFile, name) \
  if (!isNcDim(ncFile, name))                                         \
{                                                                 \
  vtkErrorMacro( << "Cannot find dimension: " << name << endl);     \
  return 0;                                                         \
}


//----------------------------------------------------------------------------
// Macro to check if the named NetCDF variable exists
//----------------------------------------------------------------------------

#define CHECK_VAR(ncFile, name)                                      \
  if (!isNcVar(ncFile, name))                                        \
{                                                                \
  vtkErrorMacro( << "Cannot find variable: " << name << endl);     \
  return 0;                                                        \
}


//----------------------------------------------------------------------------
// Function to check if there is a NetCDF variable by that name
//-----------------------------------------------------------------------------

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


//----------------------------------------------------------------------------
// Check if there is a NetCDF dimension by that name
//----------------------------------------------------------------------------

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


//-----------------------------------------------------------------------------
//  Function to convert cartesian coordinates to spherical, for use in
//  computing points in different layers of multilayer spherical view
//----------------------------------------------------------------------------

int CartesianToSpherical(double x, double y, double z, double* rho,
    double* phi, double* theta)
{
  double trho, ttheta, tphi;

  trho = sqrt( (x*x) + (y*y) + (z*z));
  ttheta = atan2(y, x);
  tphi = acos(z/(trho));
  if (vtkMath::IsNan(trho) || vtkMath::IsNan(ttheta) || vtkMath::IsNan(tphi))
    {
    return -1;
    }
  *rho = trho;
  *theta = ttheta;
  *phi = tphi;
  return 0;

}


//----------------------------------------------------------------------------
//  Function to convert spherical coordinates to cartesian, for use in
//  computing points in different layers of multilayer spherical view
//----------------------------------------------------------------------------

int SphericalToCartesian(double rho, double phi, double theta, double* x,
    double* y, double* z)
{
  double tx, ty, tz;

  tx = rho* sin(phi) * cos(theta);
  ty = rho* sin(phi) * sin(theta);
  tz = rho* cos(phi);
  if (vtkMath::IsNan(tx) || vtkMath::IsNan(ty) || vtkMath::IsNan(tz))
    {
    return -1;
    }

  *x = tx;
  *y = ty;
  *z = tz;

  return 0;
}


vtkStandardNewMacro(vtkMPASReader);

//----------------------------------------------------------------------------
// Constructor for vtkMPASReader
//----------------------------------------------------------------------------

vtkMPASReader::vtkMPASReader()
{
  this->Internals = new vtkMPASReader::Internal;

  // Debugging
  //this->DebugOn();
  vtkDebugMacro(<< "Starting to create vtkMPASReader..." << endl);

  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);

  this->infoRequested = false;
  this->dataRequested = false;

  SetDefaults();

  // Setup selection callback to modify this object when array selection changes
  this->PointDataArraySelection = vtkDataArraySelection::New();
  this->CellDataArraySelection = vtkDataArraySelection::New();
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
//  Destroys data stored for variables, points, and cells, but
//  doesn't destroy the list of variables or toplevel cell/pointVarDataArray.
//----------------------------------------------------------------------------

void vtkMPASReader::DestroyData()
{
  vtkDebugMacro(<< "DestroyData..." << endl);
  // vars are okay, just delete var data storage

  vtkDebugMacro(<< "Destructing cell var data..." << endl);
  if (this->cellVarDataArray)
    {
    for (int i = 0; i < this->numCellVars; i++)
      {
      if (this->cellVarDataArray[i] != NULL)
        {
        this->cellVarDataArray[i]->Delete();
        this->cellVarDataArray[i] = NULL;
        }
      }
    }

  vtkDebugMacro(<< "Destructing point var array..." << endl);
  if (this->pointVarDataArray)
    {
    for (int i = 0; i < this->numPointVars; i++)
      {
      if (this->pointVarDataArray[i] != NULL)
        {
        this->pointVarDataArray[i]->Delete();
        this->pointVarDataArray[i] = NULL;
        }
      }
    }

  // delete old geometry and create new

  if (this->pointVarData)
    {
    free(this->pointVarData);
    this->pointVarData = NULL;
    }

  if (cellMap)
    {
    free(cellMap);
    cellMap = NULL;
    }

  if (pointMap)
    {
    free(pointMap);
    pointMap = NULL;
    }

  if (maxLevelPoint)
    {
    free (maxLevelPoint);
    maxLevelPoint = NULL;
    }
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

  DestroyData();

  if (this->cellVarDataArray)
    {
    delete [] this->cellVarDataArray;
    }

  if (this->pointVarDataArray)
    {
    delete [] this->pointVarDataArray;
    }

  vtkDebugMacro(<< "Destructing other stuff..." << endl);
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

    if (!GetNcDims())
      {
      return(0);
      }

    vtkDebugMacro
      (<< "In vtkMPASReader::RequestInformation setting VerticalLevelRange"
       << endl);

    if (!CheckParams())
      {
      return(0);
      }

    if (!BuildVarArrays())
      {
      return 0;
      }


    // Allocate the ParaView data arrays which will hold the variables
    this->pointVarDataArray = new vtkDoubleArray*[this->numPointVars];
    for (int i = 0; i < this->numPointVars; i++)
      {
      this->pointVarDataArray[i] = NULL;
      }
    this->cellVarDataArray = new vtkDoubleArray*[this->numCellVars];
    for (int i = 0; i < this->numCellVars; i++)
      {
      this->cellVarDataArray[i] = NULL;
      }

    // Start with no data loaded into ParaView
    DisableAllPointArrays();
    DisableAllCellArrays();

    // Collect temporal information

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

//----------------------------------------------------------------------------
// Data is read into a vtkUnstructuredGrid
//----------------------------------------------------------------------------

int vtkMPASReader::RequestData(vtkInformation *vtkNotUsed(reqInfo),
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
  if (this->dataRequested)
    {
    DestroyData();
    }

  if (!ReadAndOutputGrid(true))
    {
    return 0;
    }

  // Collect the time step requested
  double* requestedTimeSteps = NULL;
  int numRequestedTimeSteps = 0;
  vtkInformationDoubleVectorKey* timeKey =
    static_cast<vtkInformationDoubleVectorKey*>
    (vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEPS());
  if (outInfo->Has(timeKey))
    {
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
  for (int var = 0; var < this->numPointVars; var++)
    {

    // Is this variable requested
    if (this->PointDataArraySelection->GetArraySetting(var))
      {
      vtkDebugMacro( << "Loading Point Variable: " << var << endl);
      if (!LoadPointVarData(var, dTime))
        {
        return 0;
        }
      output->GetPointData()->AddArray(this->pointVarDataArray[var]);

    }
  }

  for (int var = 0; var < this->numCellVars; var++)
    {
    if (this->CellDataArraySelection->GetArraySetting(var))
      {
      vtkDebugMacro
        ( << "Loading Cell Variable: "
          << this->Internals->cellVars[var]->name() << endl);
      LoadCellVarData(var, dTime);
      output->GetCellData()->AddArray(this->cellVarDataArray[var]);
      }
    }

  this->dataRequested = true;

  vtkDebugMacro( << "Returning from RequestData" << endl);
  return 1;
}

//----------------------------------------------------------------------------
// Set defaults for various parameters and initialize some variables
//----------------------------------------------------------------------------

void vtkMPASReader::SetDefaults() {

  // put in defaults
  this->VerticalLevelRange[0] = 0;
  this->VerticalLevelRange[1] = 1;
  this->VerticalLevelSelected = 0;

  this->LayerThicknessRange[0] = 0;
  this->LayerThicknessRange[1] = 200000;
  this->LayerThickness = 10000;
  vtkDebugMacro
    ( << "SetDefaults: LayerThickness set to " << LayerThickness << endl);

  this->CenterLonRange[0] = 0;
  this->CenterLonRange[1] = 360;
  this->CenterLon = 180;

  this->IsAtmosphere = false;
  this->ProjectLatLon = false;
  this->ShowMultilayerView = false;
  this->IsZeroCentered = false;

  this->includeTopography = false;
  this->doBugFix = false;
  this->centerRad = CenterLon * PI / 180.0;

  this->pointX = NULL;
  this->pointY = NULL;
  this->pointZ = NULL;
  this->origConnections = NULL;
  this->modConnections = NULL;
  this->cellMap = NULL;
  this->pointMap = NULL;
  this->maxLevelPoint = NULL;

  this->FileName = NULL;
  this->dTime = 0;
  this->cellVarDataArray = NULL;
  this->pointVarDataArray = NULL;
  this->cellVarData = NULL;
  this->pointVarData = NULL;
  this->TimeSteps = NULL;

}

//----------------------------------------------------------------------------
// Get dimensions of key NetCDF variables
//----------------------------------------------------------------------------

int vtkMPASReader::GetNcDims()
{
  NcFile *pnf = this->Internals->ncFile;

  CHECK_DIM(pnf, "nCells");
  NcDim* nCells = pnf->get_dim("nCells");
  numPoints = nCells->size();
  pointOffset = 1;

  CHECK_DIM(pnf, "nVertices");
  NcDim* nVertices = pnf->get_dim("nVertices");
  numCells = nVertices->size();
  cellOffset = 0;

  CHECK_DIM(pnf, "vertexDegree");
  NcDim* vertexDegree = pnf->get_dim("vertexDegree");
  pointsPerCell = vertexDegree->size();

  CHECK_DIM(pnf, "Time");
  NcDim* Time = pnf->get_dim("Time");
  NumberOfTimeSteps = Time->size();

  CHECK_DIM(pnf, "nVertLevels");
  NcDim* nVertLevels = pnf->get_dim("nVertLevels");
  maxNVertLevels = nVertLevels->size();

  return 1;
}

//----------------------------------------------------------------------------
//  Check parameters are valid
//----------------------------------------------------------------------------

int vtkMPASReader::CheckParams()
{

  if ((pointsPerCell != 3) && (pointsPerCell != 4))
    {
    vtkErrorMacro
      ("This code is only for hexagonal or quad primal grids" << endl);
    return(0);
    }

  /*
  // double-check we can do multilayer
  if ((ShowMultilayerView) && (maxNVertLevels == 1))
  {
  ShowMultilayerView = false;
  }

  if (!ShowMultilayerView)
  {
  maxNVertLevels = 1;
  }

   */
  // check params make sense
  this->VerticalLevelRange[0] = 0;
  this->VerticalLevelRange[1] = this->maxNVertLevels-1;

  return(1);
}

//----------------------------------------------------------------------------
// Get the NetCDF variables on cell or vertex
//----------------------------------------------------------------------------

int vtkMPASReader::GetNcVars (const char* cellDimName, const char* pointDimName)
{
  int cellVarIndex = -1;
  int pointVarIndex = -1;

  NcFile* ncFile =  this->Internals->ncFile;

  int numVars = ncFile->num_vars();

  for (int i = 0; i < numVars; i++)
    {
    NcVar* aVar = ncFile->get_var(i);

    // must have 3 dims
    // (Time, nCells | nVertices, nVertLevels | nVertLevelsP1)

    int numDims = aVar->num_dims();
    //cout << "Num Dims of var: " << aVar->name() << " is " << numDims << endl;
    if (numDims != 3)
      {
      continue;
      }

    // TODO, check if it is a double
    // assume a double for now

    // check for Time dim 0
    NcToken dim0Name = aVar->get_dim(0)->name();
    if (strcmp(dim0Name, "Time"))
      {
      continue;
      }

    // check for dim 1 being cell or point
    bool isCellData = false;
    bool isPointData = false;
    NcToken dim1Name = aVar->get_dim(1)->name();
    if (!strcmp(dim1Name, cellDimName))
      {
      isCellData = true;
      }
    else if (!strcmp(dim1Name, pointDimName))
      {
      isPointData = true;
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

    // Add to cell or point var array
    if (isCellData)
      {  // means it is cell data
      cellVarIndex++;
      if (cellVarIndex > MAX_VARS-1)
        {
        vtkErrorMacro( << "Exceeded number of cell vars." << endl);
        return(0);
        }
      this->Internals->cellVars[cellVarIndex] = aVar;
      //cout << "Adding var " << aVar->name() << " to cellVars" << endl;
      }
    else if (isPointData)
      {

      pointVarIndex++;
      if (pointVarIndex > MAX_VARS-1)
        {
        vtkErrorMacro( << "Exceeded number of point vars." << endl);
        return(0);
        }
      this->Internals->pointVars[pointVarIndex] = aVar;
      //cout << "Adding var " << aVar->name() << " to pointVars" << endl;
      }
    }

  this->numPointVars = pointVarIndex+1;
  this->numCellVars = cellVarIndex+1;
  return(1);
}


//----------------------------------------------------------------------------
// Build the selection Arrays for points and cells in the GUI.
//----------------------------------------------------------------------------

int vtkMPASReader::BuildVarArrays()
{
  vtkDebugMacro(<< "In vtkMPASReader::BuildVarArrays" << endl);

  // figure out what variables to visualize -
  int retval = GetNcVars("nVertices", "nCells");

  if (!retval)
    {
    return 0;
    }

  vtkDebugMacro( << "numCellVars: " << this->numCellVars
      << " numPointVars: " << this->numPointVars << endl);

  for (int var = 0; var < this->numPointVars; var++)
    {
    this->PointDataArraySelection->
      EnableArray((const char*)(this->Internals->pointVars[var]->name()));
    vtkDebugMacro
      (<< "Adding point var: "
       << this->Internals->pointVars[var]->name() << endl);
    }

  for (int var = 0; var < this->numCellVars; var++)
    {
    vtkDebugMacro
      (<< "Adding cell var: "
       << this->Internals->cellVars[var]->name() << endl);
    this->CellDataArraySelection->
      EnableArray((const char*)(this->Internals->cellVars[var]->name()));
    }

  vtkDebugMacro(<< "Leaving vtkMPASReader::BuildVarArrays" << endl);

  return(1);
}


//----------------------------------------------------------------------------
//  Read the data from the ncfile, allocate the geometry and create the
//  vtk data structures for points and cells.
//----------------------------------------------------------------------------

int vtkMPASReader::ReadAndOutputGrid(bool init)
{

  vtkDebugMacro(<< "In vtkMPASReader::ReadAndOutputGrid" << endl);

  if (!ProjectLatLon)
    {
    if (!AllocSphereGeometry())
      {
      return 0;
      }
    FixPoints();
    }
  else
    {
    if (!AllocLatLonGeometry())
      {
      return 0;
      }
    ShiftLonData();
    FixPoints();
    if (!EliminateXWrap())
      {
      return 0;
      }
    }

  OutputPoints(init);
  OutputCells(init);

  // Allocate the data arrays which will hold the NetCDF var data
  //vtkDebugMacro(<<"cellVarData: Alloc " << maxCells << " doubles" << endl);
  //this->cellVarData = (double*) malloc(sizeof(double)*maxCells);
  //CHECK_MALLOC(this->cellVarData);
  vtkDebugMacro(<<"pointVarData: Alloc " << maxPoints << " doubles" << endl);
  this->pointVarData = (double*) malloc(sizeof(double)*maxPoints);
  CHECK_MALLOC(this->pointVarData);

  vtkDebugMacro(<< "Leaving vtkMPASReader::ReadAndOutputGrid" << endl);

  return(1);
}


//----------------------------------------------------------------------------
// Allocate into sphere view of dual geometry
//----------------------------------------------------------------------------

int vtkMPASReader::AllocSphereGeometry()
{
  vtkDebugMacro(<< "In AllocSphereGeometry..." << endl);
  NcFile* ncFile = this->Internals->ncFile;

  CHECK_VAR(ncFile, "xCell");
  pointX = (double*)malloc((numPoints+pointOffset) * sizeof(double));
  CHECK_MALLOC(pointX);
  NcVar*  xCellVar = ncFile->get_var("xCell");
  xCellVar->get(pointX + pointOffset, numPoints);
  // point 0 is 0.0
  *pointX = 0.0;

  CHECK_VAR(ncFile, "yCell");
  pointY = (double*)malloc((numPoints+pointOffset) * sizeof(double));
  CHECK_MALLOC(pointY);
  NcVar*  yCellVar = ncFile->get_var("yCell");
  yCellVar->get(pointY + pointOffset, numPoints);
  // point 0 is 0.0
  *pointY = 0.0;

  CHECK_VAR(ncFile, "zCell");
  pointZ = (double*)malloc((numPoints+pointOffset) * sizeof(double));
  CHECK_MALLOC(pointZ);
  NcVar*  zCellVar = ncFile->get_var("zCell");
  zCellVar->get(pointZ + pointOffset, numPoints);
  // point 0 is 0.0
  *pointZ = 0.0;

  CHECK_VAR(ncFile, "cellsOnVertex");
  origConnections = (int *) malloc(numCells * pointsPerCell *
      sizeof(int));
  CHECK_MALLOC(origConnections);
  NcVar *connectionsVar = ncFile->get_var("cellsOnVertex");
  connectionsVar->get(origConnections, numCells, pointsPerCell);

  if (isNcVar(ncFile, "maxLevelCell"))
    {
    includeTopography = true;
    maxLevelPoint = (int*)malloc((numPoints + pointOffset) * sizeof(int));
    CHECK_MALLOC(maxLevelPoint);
    NcVar *maxLevelPointVar = ncFile->get_var("maxLevelCell");
    maxLevelPointVar->get(maxLevelPoint + pointOffset, numPoints);
    }

  currentExtraPoint = numPoints + pointOffset;
  currentExtraCell = numCells + cellOffset;

  if (ShowMultilayerView)
    {
    maxCells = currentExtraCell*maxNVertLevels;
    vtkDebugMacro
      (<< "alloc sphere: multilayer: setting maxCells to " << maxCells << endl);
    maxPoints = currentExtraPoint*(maxNVertLevels+1);
    vtkDebugMacro
      (<< "alloc sphere: multilayer: setting maxPoints to " << maxPoints
       << endl);
    }
  else
    {
    maxCells = currentExtraCell;
    maxPoints = currentExtraPoint;
    vtkDebugMacro
      (<< "alloc sphere: singlelayer: setting maxPoints to " << maxPoints
       << endl);
    }
  vtkDebugMacro(<< "Leaving AllocSphereGeometry..." << endl);

  return 1;
}


//----------------------------------------------------------------------------
// Allocate the lat/lon projection of dual geometry.
//----------------------------------------------------------------------------

int vtkMPASReader::AllocLatLonGeometry()
{
  vtkDebugMacro(<< "In AllocLatLonGeometry..." << endl);
  NcFile* ncFile = this->Internals->ncFile;
  const float BLOATFACTOR = .5;
  modNumPoints = (int)floor(numPoints*(1.0 + BLOATFACTOR));
  modNumCells = (int)floor(numCells*(1.0 + BLOATFACTOR))+1;

  CHECK_VAR(ncFile, "lonCell");
  pointX = (double*)malloc(modNumPoints * sizeof(double));
  CHECK_MALLOC(pointX);
  NcVar*  xCellVar = ncFile->get_var("lonCell");
  xCellVar->get(pointX + pointOffset, numPoints);
  // point 0 is 0.0
  *pointX = 0.0;

  CHECK_VAR(ncFile, "latCell");
  pointY = (double*)malloc(modNumPoints * sizeof(double));
  CHECK_MALLOC(pointY);
  NcVar*  yCellVar = ncFile->get_var("latCell");
  yCellVar->get(pointY+pointOffset, numPoints);
  // point 0 is 0.0
  *pointY = 0.0;

  CHECK_VAR(ncFile, "cellsOnVertex");
  origConnections = (int *) malloc(numCells * pointsPerCell *
      sizeof(int));
  CHECK_MALLOC(origConnections);
  NcVar *connectionsVar = ncFile->get_var("cellsOnVertex");
  connectionsVar->get(origConnections, numCells, pointsPerCell);

  // create my own list to include modified origConnections (due to
  // eliminating wraparound in the lat/lon projection) plus additional
  // cells added when mirroring cells that had previously wrapped around

  modConnections = (int *) malloc(modNumCells * pointsPerCell
      * sizeof(int));
  CHECK_MALLOC(modConnections);


  // allocate an array to map the extra points and cells to the original
  // so that when obtaining data, we know where to get it
  pointMap = (int*)malloc((int)floor(numPoints*BLOATFACTOR)
      * sizeof(int));
  CHECK_MALLOC(pointMap);
  cellMap = (int*)malloc((int)floor(numCells*BLOATFACTOR)
      * sizeof(int));
  CHECK_MALLOC(cellMap);

  if (isNcVar(ncFile, "maxLevelCell"))
    {
    includeTopography = true;
    maxLevelPoint = (int*)malloc((numPoints + numPoints) * sizeof(int));
    CHECK_MALLOC(maxLevelPoint);
    NcVar *maxLevelPointVar = ncFile->get_var("maxLevelCell");
    maxLevelPointVar->get(maxLevelPoint + pointOffset, numPoints);
    }

  currentExtraPoint = numPoints + pointOffset;
  currentExtraCell = numCells + cellOffset;

  if (ShowMultilayerView)
    {
    maxCells = currentExtraCell*maxNVertLevels;
    maxPoints = currentExtraPoint*(maxNVertLevels+1);
    vtkDebugMacro
      (<< "alloc latlon: multilayer: setting maxPoints to " << maxPoints
       << endl);
    }
  else
    {
    maxCells = currentExtraCell;
    maxPoints = currentExtraPoint;
    vtkDebugMacro
      (<< "alloc latlon: singlelayer: setting maxPoints to " << maxPoints
       << endl);
    }
  vtkDebugMacro(<< "Leaving AllocLatLonGeometry..." << endl);

  return 1;
}


//----------------------------------------------------------------------------
//  Shift data if center longitude needs to change.
//----------------------------------------------------------------------------

void vtkMPASReader::ShiftLonData()
{
  vtkDebugMacro(<< "In ShiftLonData..." << endl);
  // if atmospheric data, or zero centered, set center to 180 instead of 0
  if (IsAtmosphere || IsZeroCentered)
    {
    for (int j = pointOffset; j < numPoints + pointOffset; j++)
      {
      // need to shift over the point so center is at PI
      if (pointX[j] < 0)
        {
        pointX[j] += 2*PI;
        }
      }
    }

  if (CenterLon != 180)
    {
    for (int j = pointOffset; j < numPoints + pointOffset; j++)
      {
      // need to shift over the point if centerLon dictates
      if (centerRad < PI)
        {
        if (pointX[j] > (centerRad + PI))
          {
          pointX[j] = -((2*PI) - pointX[j]);
          }
        }
      else if (centerRad > PI)
        {
        if (pointX[j] < (centerRad - PI))
          {
          pointX[j] += 2*PI;
          }
        }
      }
    }
  vtkDebugMacro(<< "Leaving ShiftLonData..." << endl);
}


//----------------------------------------------------------------------------
//  Add a "mirror point" -- a point on the opposite side of the lat/lon
// projection.
//----------------------------------------------------------------------------

int vtkMPASReader::AddMirrorPoint(int index, double dividerX)
{
  //vtkDebugMacro(<< "In AddMirrorPoint..." << endl);
  double X = pointX[index];
  double Y = pointY[index];

  // add on east
  if (X < dividerX)
    {
    X += 2*PI;
    }
  else
    {
    // add on west
    X -= 2*PI;
    }

  pointX[currentExtraPoint] = X;
  pointY[currentExtraPoint] = Y;

  int mirrorPoint = currentExtraPoint;

  // record mapping
  *(pointMap + (currentExtraPoint - numPoints - pointOffset)) = index;
  currentExtraPoint++;

  //vtkDebugMacro(<< "Leaving AddMirrorPoint..." << endl);
  return mirrorPoint;
}


//----------------------------------------------------------------------------
// Check for out-of-range values and do bugfix
//----------------------------------------------------------------------------

void vtkMPASReader::FixPoints()
{
  vtkDebugMacro(<< "In FixPoints..." << endl);

  for (int j = cellOffset; j < numCells + cellOffset; j++ )
    {
    int *conns = origConnections + (j * pointsPerCell);

    // go through and make sure none of the referenced points are
    // out of range
    // if so, set all to point 0
    for (int k = 0; k < pointsPerCell; k++)
      {
      if  ((conns[k] <= 0) || (conns[k] > numPoints))
        {
        for (int m = 0; m < pointsPerCell; m++)
          {
          conns[m] = 0;
          }
        break;
        }
      }

    if (doBugFix)
      {
      //BUG FIX for problem where cells are stretching to a faraway point
      int lastk = pointsPerCell-1;
      const double thresh = .06981317007977; // 4 degrees
      for (int k = 0; k < pointsPerCell; k++)
        {
        double ydiff = abs(pointY[conns[k]]
                           - pointY[conns[lastk]]);
        // Don't look at cells at map border
        if (ydiff > thresh)
          {
          for (int m = 0; m < pointsPerCell; m++)
            {
            conns[m] = 0;
            }
          break;
          }
        }
      }
    }
  vtkDebugMacro(<< "Leaving FixPoints..." << endl);
}


//----------------------------------------------------------------------------
// Eliminate wraparound at east/west edges of lat/lon projection
//----------------------------------------------------------------------------

int vtkMPASReader::EliminateXWrap ()
{
  vtkDebugMacro(<< "In EliminateXWrap..." << endl);

  // For each cell, examine vertices
  // Add new points and cells where needed to account for wraparound.
  for (int j = cellOffset; j < numCells + cellOffset; j++ )
    {
    int *conns = origConnections + (j * pointsPerCell);
    int *modConns = modConnections + (j * pointsPerCell);

    // Determine if we are wrapping in X direction
    int lastk = pointsPerCell-1;
    bool xWrap = false;
    for (int k = 0; k < pointsPerCell; k++)
      {
      if (abs(pointX[conns[k]]
              - pointX[conns[lastk]]) > 5.5)
        {
        xWrap = true;
        }
      lastk = k;
      }

    // If we wrapped in X direction, modify cell and add mirror cell
    if (xWrap)
      {

      // first point is anchor it doesn't move
      double anchorX = pointX[conns[0]];
      modConns[0] = conns[0];

      // modify existing cell, so it doesn't wrap
      // move points to one side
      for (int k = 1; k < pointsPerCell; k++)
        {
        int neigh = conns[k];

        // add a new point, figure out east or west

        if (abs(pointX[neigh] - anchorX) > 5.5)
          {
          modConns[k] = AddMirrorPoint(neigh, anchorX);
          }
        else
          {
          // use existing kth point
          modConns[k] = neigh;
          }
        }

      // move addedConns to modConnections extra cells area
      int* addedConns = modConnections
        + (currentExtraCell * pointsPerCell);

      // add a mirroring cell to other side

      // add mirrored anchor first
      addedConns[0] = AddMirrorPoint(conns[0], centerRad);
      anchorX = pointX[addedConns[0]];

      // add mirror cell points if needed
      for (int k = 1; k < pointsPerCell; k++)
        {
        int neigh = conns[k];

        // add a new point for neighbor, figure out east or west
        if (abs(pointX[neigh] - anchorX) > 5.5)
          {
          addedConns[k] = AddMirrorPoint(neigh, anchorX);
          }
        else
          {
          // use existing kth point
          addedConns[k] = neigh;
          }
        }
      *(cellMap + (currentExtraCell - numCells - cellOffset)) = j;
      currentExtraCell++;
      }
    else
      {

      // just add cell "as is" to modConnections
      for (int k=0; k< pointsPerCell; k++)
        {
        modConns[k] = conns[k];
        }
      }
    if (currentExtraCell > modNumCells)
      {
      vtkErrorMacro( << "Exceeded storage for extra cells!" << endl);
      return(0);
      }
    if (currentExtraPoint > modNumPoints)
      {
      vtkErrorMacro( << "Exceeded storage for extra points!" << endl);
      return(0);
      }
    }

  if (!ShowMultilayerView)
    {
    maxCells = currentExtraCell;
    maxPoints = currentExtraPoint;
    vtkDebugMacro
      (<< "elim xwrap: singlelayer: setting maxPoints to " << maxPoints
       << endl);
    }
  else
    {
    maxCells = currentExtraCell*maxNVertLevels;
    maxPoints = currentExtraPoint*(maxNVertLevels+1);
    vtkDebugMacro
      (<< "elim xwrap: multilayer: setting maxPoints to " <<
       maxPoints << endl);
    }
  vtkDebugMacro(<< "Leaving EliminateXWrap..." << endl);

  return 1;
}


//----------------------------------------------------------------------------
//  Add points to vtk data structures
//----------------------------------------------------------------------------

void vtkMPASReader::OutputPoints(bool init)
{
  vtkDebugMacro(<< "In OutputPoints..." << endl);

  vtkUnstructuredGrid* output = GetOutput();

  vtkSmartPointer<vtkPoints> points;

  float adjustedLayerThickness = LayerThickness;

  if (IsAtmosphere)
    {
    adjustedLayerThickness = -LayerThickness;
    }

  vtkDebugMacro
    (<< "OutputPoints: maxPoints: " << maxPoints << " maxNVertLevels: "
     << maxNVertLevels << " LayerThickness: " << LayerThickness
     <<"ProjectLatLon: " << ProjectLatLon << " ShowMultilayerView: "
     << ShowMultilayerView << endl);

  if (init)
    {
    points = vtkSmartPointer<vtkPoints>::New();
    points->Allocate(maxPoints, maxPoints);
    output->SetPoints(points);
    }
  else
    {
    points = output->GetPoints();
    points->Initialize();
    points->Allocate(maxPoints, maxPoints);
    }

  for (int j = 0; j < currentExtraPoint; j++ )
    {

    double x, y, z;

    if (ProjectLatLon)
      {
      x = pointX[j] * 180.0 / PI;
      y = pointY[j] * 180.0 / PI;
      z = 0.0;
      }
    else
      {
      x = pointX[j];
      y = pointY[j];
      z = pointZ[j];
      }

    if (!ShowMultilayerView)
      {
      points->InsertNextPoint(x, y, z);
      //    points->SetPoint(j, x, y, z);
      }
    else
      {
      double rho=0.0, rholevel=0.0, theta=0.0, phi=0.0;
      int retval = -1;

      if (!ProjectLatLon)
        {
        if ((x != 0.0) || (y != 0.0) || (z != 0.0))
          {
          retval = CartesianToSpherical(x, y, z, &rho, &phi, &theta);
          if (retval)
            {
            vtkDebugMacro("Can't create point for layered view." << endl);
            }
          }
        }

      for (int levelNum = 0; levelNum < maxNVertLevels+1; levelNum++)
        {

        if (ProjectLatLon)
          {
          z = -(double)((levelNum)*adjustedLayerThickness);
          }
        else
          {
          if (!retval && ((x != 0.0) || (y != 0.0) || (z != 0.0)))
            {
            rholevel = rho - (adjustedLayerThickness * levelNum);
            retval = SphericalToCartesian(rholevel, phi, theta, &x, &y, &z);
            if (retval)
              {
              vtkDebugMacro("Can't create point for layered view." << endl);
              }
            }
          }
        //   points->SetPoint(j*(maxNVertLevels+1) + levelNum, x, y, z);
        points->InsertNextPoint(x, y, z);
        }
      }
    }

  if (pointX)
    {
    free(pointX);
    pointX = NULL;
    }
  if (pointY)
    {
    free(pointY);
    pointY = NULL;
    }
  if (pointZ)
    {
    free(pointZ);
    pointZ = NULL;
    }
  vtkDebugMacro(<< "Leaving OutputPoints..." << endl);
}


//----------------------------------------------------------------------------
// Determine if cell is one of VTK_TRIANGLE, VTK_WEDGE, VTK_QUAD or
// VTK_HEXAHEDRON
//----------------------------------------------------------------------------

unsigned char vtkMPASReader::GetCellType()
{
  // write cell types
  unsigned char cellType = VTK_TRIANGLE;
  switch (pointsPerCell) {
    case 3:
      if (!ShowMultilayerView)
        {
        cellType = VTK_TRIANGLE;
        }
      else
        {
        cellType = VTK_WEDGE;
        }
      break;
    case 4:
      if (!ShowMultilayerView)
        {
        cellType = VTK_QUAD;
        }
      else
        {
        cellType = VTK_HEXAHEDRON;
        }
      break;
    default:
      break;
  }
  return cellType;
}


//----------------------------------------------------------------------------
//  Add cells to vtk data structures
//----------------------------------------------------------------------------

void vtkMPASReader::OutputCells(bool init)
{
  vtkDebugMacro(<< "In OutputCells..." << endl);


  vtkUnstructuredGrid* output = GetOutput();

  if (init)
    {
    output->Allocate(maxCells, maxCells);
    }
  else
    {
    vtkCellArray* cells = output->GetCells();
    cells->Initialize();
    output->Allocate(maxCells, maxCells);
    }

  int cellType = GetCellType();
  int val;

  int pointsPerPolygon;
  if (ShowMultilayerView)
    {
    pointsPerPolygon = 2 * pointsPerCell;
    }
  else
    {
    pointsPerPolygon = pointsPerCell;
    }

  vtkDebugMacro
    (<< "OutputCells: init: " << init << " maxCells: " << maxCells
     << " cellType: " << cellType << " maxNVertLevels: " << maxNVertLevels
     << " LayerThickness: " << LayerThickness << " ProjectLatLon: "
     << ProjectLatLon << " ShowMultilayerView: " << ShowMultilayerView << endl);

  vtkIdType* polygon = new vtkIdType[pointsPerPolygon];

  for (int j = 0; j < currentExtraCell ; j++)
    {

    int* conns;
    if (ProjectLatLon)
      {
      conns = modConnections + (j * pointsPerCell);
      }
    else
      {
      conns = origConnections + (j * pointsPerCell);
      }

    int minLevel= 0;

    if (includeTopography)
      {
      int* connections;

      //check if it is a mirror cell, if so, get original
      if (j >= numCells + cellOffset)
        {
        //cout << "setting origCell" << endl;
        int origCellNum = *(cellMap + (j - numCells - cellOffset));
        //cout << "mirror cell: " <<j<< " origCell: "<< origCell << endl;
        connections = origConnections + (origCellNum*pointsPerCell);
        }
      else
        {
        connections = origConnections + (j * pointsPerCell);
        }

      //cout << "calc minLevel" << endl;
      minLevel = maxLevelPoint[connections[0]];
      //cout << "initial minLevel:" << minLevel << endl;
      // Take the min of the maxLevelPoint of each point
      for (int k = 1; k < pointsPerCell; k++)
        {
        minLevel = min(minLevel, maxLevelPoint[connections[k]]);
        }
      //cout << endl;
      }

    // singlelayer
    if (!ShowMultilayerView)
      {
      // If that min is greater than or equal to this output level,
      // include the cell, otherwise set all points to zero.

      if (includeTopography && ((minLevel-1) < VerticalLevelSelected))
        {
        //cerr << "Setting all points to zero" << endl;
        val = 0;
        for (int k = 0; k < pointsPerCell; k++)
          {
          polygon[k] = val;
          }
        }
      else
        {
        for (int k = 0; k < pointsPerCell; k++)
          {
          polygon[k] = conns[k];
          }
        }
      output->InsertNextCell(cellType, pointsPerPolygon, polygon);

    }
    else
      { // multilayer
      // for each level, write the cell
      for (int levelNum = 0; levelNum < maxNVertLevels; levelNum++)
        {
        if (includeTopography && ((minLevel-1) < levelNum))
          {
          // setting all points to zero
          val = 0;
          for (int k = 0; k < pointsPerPolygon; k++)
            {
            polygon[k] = val;
            }
          }
        else
          {
          for (int k = 0; k < pointsPerCell; k++)
            {
            val = (conns[k]*(maxNVertLevels+1)) + levelNum;
            polygon[k] = val;
            }

          for (int k = 0; k < pointsPerCell; k++)
            {
            val = (conns[k]*(maxNVertLevels+1)) + levelNum + 1;
            polygon[k+pointsPerCell] = val;
            }
          }
        //vtkDebugMacro
        //("InsertingCell j: " << j << " level: " << levelNum << endl);
        output->InsertNextCell(cellType, pointsPerPolygon, polygon);
        }
      }
    }

  //cma check these frees
  free(modConnections); modConnections = NULL;
  free(origConnections); origConnections = NULL;
  free(polygon); polygon = NULL;

  vtkDebugMacro(<< "Leaving OutputCells..." << endl);
}


//----------------------------------------------------------------------------
//  Load the data for a point variable
//----------------------------------------------------------------------------

int vtkMPASReader::LoadPointVarData(int variableIndex, double dTimeStep)
{

  vtkDebugMacro(<< "In vtkMPASReader::LoadPointVarData" << endl);

  NcVar* ncVar = this->Internals->pointVars[variableIndex];

  vtkDebugMacro(<< "got ncVar in vtkMPASReader::LoadPointVarData" << endl);
  if (ncVar == NULL)
    {
    vtkErrorMacro( << "Can't find data for variable " << variableIndex << endl);
    return 0;
    }

  // Allocate data array for this variable

  if (this->pointVarDataArray[variableIndex] == NULL)
    {
    vtkDebugMacro
      (<< "allocating data array in vtkMPASReader::LoadPointVarData" << endl);
    this->pointVarDataArray[variableIndex] = vtkDoubleArray::New();
    this->pointVarDataArray[variableIndex]->SetName
      (this->Internals->pointVars[variableIndex]->name());
    this->pointVarDataArray[variableIndex]->SetNumberOfTuples
      (maxPoints);
    this->pointVarDataArray[variableIndex]->SetNumberOfComponents(1);
    }

  vtkDebugMacro(<< "getting pointer in vtkMPASReader::LoadPointVarData"
                << endl);
  double* dataBlock = this->pointVarDataArray[variableIndex]->GetPointer(0);

  vtkDebugMacro( << "dTimeStep requested: " << dTimeStep << endl);
  int timestep = min((int)floor(dTimeStep),
                     (int)(this->NumberOfTimeSteps-1));
  vtkDebugMacro( << "Time: " << timestep << endl);


  // singlelayer
  if (!ShowMultilayerView)
    {
    this->Internals->pointVars[variableIndex]->set_cur
      (timestep, 0, this->VerticalLevelSelected);

    // we can go ahead and read it into the datablock
    this->Internals->pointVars[variableIndex]->get
      (dataBlock+pointOffset, 1, numPoints, 1);
    dataBlock[0] = dataBlock[1];
    // data is all in place, don't need to do next step

    }
  else
    { // multilayer
    this->Internals->pointVars[variableIndex]->set_cur(timestep, 0, 0);
    this->Internals->pointVars[variableIndex]->get
      (pointVarData +
       (maxNVertLevels * pointOffset), 1, numPoints, maxNVertLevels);
    }

  vtkDebugMacro
    (<< "got point data in vtkMPASReader::LoadPointVarData" << endl);

  int i=0, k=0;

  if (ShowMultilayerView)
    {

    // put in dummy points
    for (int levelNum = 0; levelNum < maxNVertLevels; levelNum++)
      {
      dataBlock[levelNum] = pointVarData[maxNVertLevels + levelNum];
      }
    // write highest level dummy point (duplicate of last level)
    dataBlock[maxNVertLevels] = pointVarData[maxNVertLevels+maxNVertLevels-1];

    vtkDebugMacro (<< "Wrote dummy vtkMPASReader::LoadPointVarData" << endl);

    // put in other points
    for (int j = pointOffset; j < numPoints + pointOffset; j++)
      {

      i = j*(maxNVertLevels+1);
      k = j*(maxNVertLevels);

      // write data for one point -- lowest level to highest
      for (int levelNum = 0; levelNum < maxNVertLevels; levelNum++)
        {
        dataBlock[i++] = pointVarData[k++];
        }

      // for last layer of points, repeat last level's values
      // Need Mark's input on this one
      dataBlock[i++] = pointVarData[--k];
      //vtkDebugMacro (<< "Wrote j:" << j << endl);
      }

    }

  vtkDebugMacro (<< "Wrote next pts vtkMPASReader::LoadPointVarData" << endl);

  vtkDebugMacro
    (<< "numPoints: " << numPoints << " currentExtraPoint: "
     << currentExtraPoint << endl);

  // put out data for extra points
  for (int j = pointOffset + numPoints; j < currentExtraPoint; j++)
    {
    // use map to find out what point data we are using

    if (!ShowMultilayerView)
      {
      k = pointMap[j - numPoints - pointOffset];
      dataBlock[j] = dataBlock[k];
      }
    else
      {
      k = pointMap[j - numPoints - pointOffset]*maxNVertLevels;
      // write data for one point -- lowest level to highest
      for (int levelNum = 0; levelNum < maxNVertLevels; levelNum++)
        {
        dataBlock[i++] = pointVarData[k++];
        }

      // for last layer of points, repeat last level's values
      // Need Mark's input on this one
      dataBlock[i++] = pointVarData[--k];
      }
    }

  vtkDebugMacro
    (<< "wrote extra point data in vtkMPASReader::LoadPointVarData"
     << endl);
  return (1);
}


//----------------------------------------------------------------------------
//  Load the data for a cell variable
//----------------------------------------------------------------------------

int vtkMPASReader::LoadCellVarData(int variableIndex, double dTimeStep)
{
  vtkDebugMacro(<< "In vtkMPASReader::LoadCellVarData" << endl);

  // cma modify to use point map for lat/lon projection

  NcVar* ncVar = this->Internals->cellVars[variableIndex];

  if (ncVar == NULL)
    {
    vtkErrorMacro
      (<< "Can't find data for variable index:" << variableIndex << endl);
    return 0;
    }

  // Allocate data array for this variable
  if (this->cellVarDataArray[variableIndex] == NULL)
    {
    this->cellVarDataArray[variableIndex] = vtkDoubleArray::New();
    vtkDebugMacro
      ( << "Allocated cell var index: "
        << this->Internals->cellVars[variableIndex]->name() << endl);
    this->cellVarDataArray[variableIndex]->SetName
      (this->Internals->cellVars[variableIndex]->name());
    this->cellVarDataArray[variableIndex]->SetNumberOfTuples
      (this->maxCells);
    this->cellVarDataArray[variableIndex]->SetNumberOfComponents(1);
    }

  vtkDebugMacro(<< "getting pointer in vtkMPASReader::LoadCellVarData" << endl);

  double* dataBlock = this->cellVarDataArray[variableIndex]->GetPointer(0);

  vtkDebugMacro( << "dTimeStep requested: " << dTimeStep << endl);
  int timestep = min((int)floor(dTimeStep),
                     (int)(this->NumberOfTimeSteps-1));
  vtkDebugMacro( << "Time: " << timestep << endl);

  ncVar->set_cur(timestep, 0, this->VerticalLevelSelected);

  if (!ShowMultilayerView)
    {
    ncVar->get(dataBlock, 1, numCells, 1);
    }
  else
    {
    ncVar->get(dataBlock, 1, numCells, maxNVertLevels);
    }

  vtkDebugMacro(<< "Got data for cell var: "
                << this->Internals->cellVars[variableIndex]->name()
                << endl);

  // put out data for extra cells
  for (int j = cellOffset + numCells; j < currentExtraCell; j++)
    {
    // use map to find out what cell data we are using

    if (!ShowMultilayerView)
      {
      int k = cellMap[j - numCells - cellOffset];
      dataBlock[j] = dataBlock[k];
      }
    else
      {
      int i = j*maxNVertLevels;
      int k = cellMap[j - numCells - cellOffset]*maxNVertLevels;
      // write data for one cell -- lowest level to highest
      for (int levelNum = 0; levelNum < maxNVertLevels; levelNum++)
        {
        dataBlock[i++] = dataBlock[k++];
        }
      }
    }

  vtkDebugMacro( << "Stored data for cell var: "
      << this->Internals->cellVars[variableIndex]->name() << endl);

  return(1);
}


//----------------------------------------------------------------------------
// If the user changes parameters (lat/lon to spherical, or singlelayer to
// multilayer, regenerate the geometry.
//----------------------------------------------------------------------------

int vtkMPASReader::RegenerateGeometry()
{

  vtkUnstructuredGrid* output = GetOutput();

  vtkDebugMacro(<< "RegenerateGeometry ..." << endl);

  DestroyData();

  // Output the unstructured grid from the netCDF file
  if (!ReadAndOutputGrid(true))
    {
    return 0;
    }

  // fetch data selected using new geometry
  // Examine each variable to see if it is selected
  for (int var = 0; var < this->numPointVars; var++)
    {

    // Is this variable requested
    if (this->PointDataArraySelection->GetArraySetting(var))
      {
      vtkDebugMacro( << "Loading Point Variable: " << var << endl);
      if (!LoadPointVarData(var, dTime))
        {
        return 0;
        }
      output->GetPointData()->AddArray(this->pointVarDataArray[var]);
      }
    }

  for (int var = 0; var < this->numCellVars; var++)
    {
    if (this->CellDataArraySelection->GetArraySetting(var))
      {
      vtkDebugMacro
        ( << "Loading Cell Variable: "
          << this->Internals->cellVars[var]->name() << endl);
      if (!LoadCellVarData(var, dTime))
        {
        return 0;
        }
      output->GetCellData()->AddArray(this->cellVarDataArray[var]);
      }
    }

  this->PointDataArraySelection->Modified();
  this->CellDataArraySelection->Modified();

  this->Modified();

  return 1;
}


//----------------------------------------------------------------------------
//  Callback if the user selects a variable.
//----------------------------------------------------------------------------

void vtkMPASReader::SelectionCallback(vtkObject*,
    unsigned long vtkNotUsed(eventid),
    void* clientdata,
    void* vtkNotUsed(calldata))
{
  static_cast<vtkMPASReader*>(clientdata)->Modified();
}


//----------------------------------------------------------------------------
//  Return the output.
//----------------------------------------------------------------------------

vtkUnstructuredGrid* vtkMPASReader::GetOutput()
{
  return this->GetOutput(0);
}


//----------------------------------------------------------------------------
//  Returns the output given an id.
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
//  Get number of point arrays.
//----------------------------------------------------------------------------

int vtkMPASReader::GetNumberOfPointArrays()
{
  return this->PointDataArraySelection->GetNumberOfArrays();
}


//----------------------------------------------------------------------------
// Get number of cell arrays.
//----------------------------------------------------------------------------

int vtkMPASReader::GetNumberOfCellArrays()
{
  return this->CellDataArraySelection->GetNumberOfArrays();
}


//----------------------------------------------------------------------------
// Make all point selections available.
//----------------------------------------------------------------------------
void vtkMPASReader::EnableAllPointArrays()
{
  this->PointDataArraySelection->EnableAllArrays();
}


//----------------------------------------------------------------------------
// Make all point selections unavailable.
//----------------------------------------------------------------------------

void vtkMPASReader::DisableAllPointArrays()
{
  this->PointDataArraySelection->DisableAllArrays();
}

//----------------------------------------------------------------------------
// Make all cell selections available.
//----------------------------------------------------------------------------

void vtkMPASReader::EnableAllCellArrays()
{
  this->CellDataArraySelection->EnableAllArrays();
}


//----------------------------------------------------------------------------
// Make all cell selections unavailable.
//----------------------------------------------------------------------------

void vtkMPASReader::DisableAllCellArrays()
{
  this->CellDataArraySelection->DisableAllArrays();
}


//----------------------------------------------------------------------------
// Get name of indexed point variable
//----------------------------------------------------------------------------

const char* vtkMPASReader::GetPointArrayName(int index)
{
  return (const char*)(this->Internals->pointVars[index]->name());

}

//----------------------------------------------------------------------------
// Get status of named point variable selection
//----------------------------------------------------------------------------

int vtkMPASReader::GetPointArrayStatus(const char* name)
{
  return this->PointDataArraySelection->ArrayIsEnabled(name);
}


//----------------------------------------------------------------------------
// Set status of named point variable selection.
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
// Get name of indexed cell variable
//----------------------------------------------------------------------------

const char* vtkMPASReader::GetCellArrayName(int index)
{
  return (const char*)(this->Internals->cellVars[index]->name());
}


//----------------------------------------------------------------------------
// Get status of named cell variable selection.
//----------------------------------------------------------------------------

int vtkMPASReader::GetCellArrayStatus(const char* name)
{
  return this->CellDataArraySelection->ArrayIsEnabled(name);
}


//----------------------------------------------------------------------------
// Set status of named cell variable selection.
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


//----------------------------------------------------------------------------
//  Set vertical level to be viewed.
//----------------------------------------------------------------------------

void vtkMPASReader::SetVerticalLevel(int level)
{
  this->VerticalLevelSelected = level;
  vtkDebugMacro( << "Set VerticalLevelSelected to: " << level << endl);

  vtkDebugMacro( << "infoRequested?: " << infoRequested << endl);

  if (!this->infoRequested)
    {
    return;
    }
  if (!this->dataRequested)
    {
    return;
    }

  // Examine each variable to see if it is selected
  for (int var = 0; var < this->numPointVars; var++)
    {
    // Is this variable requested
    if (this->PointDataArraySelection->GetArraySetting(var))
      {
      vtkDebugMacro( << "Loading Point Variable: "
                     << this->Internals->pointVars[var]->name() << endl);
      LoadPointVarData(var, dTime);
      }
    }

  for (int var = 0; var < this->numCellVars; var++)
    {
    if (this->CellDataArraySelection->GetArraySetting(var))
      {
      vtkDebugMacro( << "Loading Cell Variable: "
                     << this->Internals->cellVars[var]->name() << endl);
      LoadCellVarData(var, dTime);
      }
    }

  this->PointDataArraySelection->Modified();
  this->CellDataArraySelection->Modified();
}


//----------------------------------------------------------------------------
//  Set layer thickness for multilayer view.
//----------------------------------------------------------------------------

void vtkMPASReader::SetLayerThickness(int val)
{
  if (LayerThickness != val)
    {
    LayerThickness = val;
    vtkDebugMacro
      ( << "SetLayerThickness: LayerThickness set to " << LayerThickness
        << endl);
    if (ShowMultilayerView)
      {
      // Don't regenerate if we've never done an initial read
      if (!this->infoRequested)
        {
        return;
        }
      if (!this->dataRequested)
        {
        return;
        }
      RegenerateGeometry();
      }
    }
}


//----------------------------------------------------------------------------
//  Set center longitude for lat/lon projection
//----------------------------------------------------------------------------

void vtkMPASReader::SetCenterLon(int val)
{
  vtkDebugMacro( << "SetCenterLon: is " << CenterLon << endl);
  if (CenterLon != val)
    {
    vtkDebugMacro( << "SetCenterLon: set to " << CenterLon << endl);
    CenterLon = val;
    this->centerRad = CenterLon * PI / 180.0;
    vtkDebugMacro( << "centerRad set to " << centerRad << endl);
    if (ProjectLatLon)
      {
      // Don't regenerate if we've never done an initial read
      if (!this->infoRequested)
        {
        return;
        }
      if (!this->dataRequested)
        {
        return;
        }
      RegenerateGeometry();
      }
    }
}


//----------------------------------------------------------------------------
// Set view to be the lat/lon projection.
//----------------------------------------------------------------------------

void vtkMPASReader::SetProjectLatLon(bool val)
{
  if (ProjectLatLon != val)
    {
    ProjectLatLon = val;
    // Don't regenerate if we've never done an initial read
    if (!this->infoRequested)
      {
      return;
      }
    if (!this->dataRequested)
      {
      return;
      }
    RegenerateGeometry();
  }
}


//----------------------------------------------------------------------------
// Set the view to be of atmosphere (vertical levels go up)
//----------------------------------------------------------------------------

void vtkMPASReader::SetIsAtmosphere(bool val)
{
  if (IsAtmosphere != val)
    {
    IsAtmosphere = val;
    // Don't regenerate if we've never done an initial read
    if (!this->infoRequested)
      {
      return;
      }
    if (!this->dataRequested)
      {
      return;
      }
    RegenerateGeometry();
  }
}


//----------------------------------------------------------------------------
//  Set lat/lon projection to be centered at zero longitude
//----------------------------------------------------------------------------

void vtkMPASReader::SetIsZeroCentered(bool val)
{
  if (IsZeroCentered != val)
    {
    IsZeroCentered = val;
    // Don't regenerate if we've never done an initial read
    if (!this->infoRequested)
      {
      return;
      }
    if (!this->dataRequested)
      {
      return;
      }
    RegenerateGeometry();
  }
}


//----------------------------------------------------------------------------
//  Set view to be multilayered view.
//----------------------------------------------------------------------------

void vtkMPASReader::SetShowMultilayerView(bool val)
{
  if (ShowMultilayerView != val)
    {
    ShowMultilayerView= val;
    // Don't regenerate if we've never done an initial read
    if (!this->infoRequested)
      {
      return;
      }
    if (!this->dataRequested)
      {
      return;
      }
    RegenerateGeometry();
  }
}


//----------------------------------------------------------------------------
//  Determine if this reader can read the given file (if it is an MPAS format)
// NetCDF file
//----------------------------------------------------------------------------

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


//----------------------------------------------------------------------------
//  Print self.
//----------------------------------------------------------------------------

void vtkMPASReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FileName: "
     << (this->FileName?this->FileName:"NULL") << "\n";
  os << indent << "VerticalLevelRange: " << this->VerticalLevelRange << "\n";
  os << indent << "numPointVars: " << this->numPointVars << "\n";
  os << indent << "numCellVars: " << this->numCellVars << "\n";
  os << indent << "maxPoints: " << this->maxPoints << "\n";
  os << indent << "maxCells: " << this->maxCells << "\n";
  os << indent << "ProjectLatLon: "
     << (this->ProjectLatLon?"ON":"OFF") << endl;
  os << indent << "ShowMultilayerView: "
     << (this->ShowMultilayerView?"ON":"OFF") << endl;
  os << indent << "CenterLonRange: "
     << this->CenterLonRange[0] << "," << this->CenterLonRange[1] << endl;
  os << indent << "IsAtmosphere: "
     << (this->IsAtmosphere?"ON":"OFF") << endl;
  os << indent << "IsZeroCentered: "
     << (this->IsZeroCentered?"ON":"OFF") << endl;
  os << indent << "LayerThicknessRange: "
     << this->LayerThicknessRange[0] << "," << this->LayerThicknessRange[1] << endl;
}
