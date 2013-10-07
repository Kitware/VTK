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
      this->cellVars[i] = NULL;
      this->pointVars[i] = NULL;
      }
  };
  ~Internal()
  {
    if(this->ncFile)
      {
      delete ncFile;
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

  this->CellMask = 0;

  // Debugging
  //this->DebugOn();
  vtkDebugMacro(<< "Starting to create vtkMPASReader..." << endl);

  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);

  this->InfoRequested = false;
  this->DataRequested = false;

  this->SetDefaults();

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
  if (this->CellVarDataArray)
    {
    for (int i = 0; i < this->NumberOfCellVars; i++)
      {
      if (this->CellVarDataArray[i] != NULL)
        {
        this->CellVarDataArray[i]->Delete();
        this->CellVarDataArray[i] = NULL;
        }
      }
    }

  vtkDebugMacro(<< "Destructing point var array..." << endl);
  if (this->PointVarDataArray)
    {
    for (int i = 0; i < this->NumberOfPointVars; i++)
      {
      if (this->PointVarDataArray[i] != NULL)
        {
        this->PointVarDataArray[i]->Delete();
        this->PointVarDataArray[i] = NULL;
        }
      }
    }

  // delete old geometry and create new

  if (this->PointVarData)
    {
    delete []this->PointVarData;
    this->PointVarData = NULL;
    }

  if (this->CellMap)
    {
    free(this->CellMap);
    this->CellMap = NULL;
    }

  if (this->PointMap)
    {
    free(this->PointMap);
    this->PointMap = NULL;
    }

  if (this->MaximumLevelPoint)
    {
    free(this->MaximumLevelPoint);
    this->MaximumLevelPoint = NULL;
    }
}

//----------------------------------------------------------------------------
// Destructor for MPAS Reader
//----------------------------------------------------------------------------

vtkMPASReader::~vtkMPASReader()
{
  vtkDebugMacro(<< "Destructing vtkMPASReader..." << endl);

  this->SetFileName(NULL);

  if (this->Internals->ncFile)
    {
    delete this->Internals->ncFile;
    this->Internals->ncFile = NULL;
    }

  this->DestroyData();

  if (this->CellVarDataArray)
    {
    delete [] this->CellVarDataArray;
    this->CellVarDataArray = NULL;
    }

  if (this->PointVarDataArray)
    {
    delete [] this->PointVarDataArray;
    this->PointVarDataArray = NULL;
    }

  vtkDebugMacro(<< "Destructing other stuff..." << endl);
  if (this->PointDataArraySelection)
    {
    this->PointDataArraySelection->Delete();
    this->PointDataArraySelection = NULL;
    }
  if (this->CellDataArraySelection)
    {
    this->CellDataArraySelection->Delete();
    this->CellDataArraySelection = NULL;
    }
  if (this->SelectionObserver)
    {
    this->SelectionObserver->Delete();
    this->SelectionObserver = NULL;
    }
  if(this->TimeSteps)
    {
    delete []this->TimeSteps;
    this->TimeSteps = NULL;
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

  if (!this->InfoRequested)
    {
    this->InfoRequested = true;

    vtkDebugMacro(<< "FileName: " << this->FileName << endl);
    if(this->Internals->ncFile)
      {
      delete this->Internals->ncFile;
      }
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
    if(this->PointVarDataArray)
      {
      delete []this->PointVarDataArray;
      }
    this->PointVarDataArray = new vtkDoubleArray*[this->NumberOfPointVars];
    for (int i = 0; i < this->NumberOfPointVars; i++)
      {
      this->PointVarDataArray[i] = NULL;
      }
    if(this->CellVarDataArray)
      {
      delete []this->CellVarDataArray;
      }
    this->CellVarDataArray = new vtkDoubleArray*[this->NumberOfCellVars];
    for (int i = 0; i < this->NumberOfCellVars; i++)
      {
      this->CellVarDataArray[i] = NULL;
      }

    // Start with no data loaded into ParaView
    this->DisableAllPointArrays();
    this->DisableAllCellArrays();

    // Collect temporal information

    // At this time, MPAS doesn't have fine-grained time value, just
    // the number of the step, so that is what I store here for TimeSteps.
    if (this->TimeSteps != NULL)
      {
      delete []this->TimeSteps;
      this->TimeSteps = NULL;
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
  if (this->DataRequested)
    {
    this->DestroyData();
    }

  if (!this->ReadAndOutputGrid(true))
    {
    return 0;
    }

  // Collect the time step requested
  double requestedTimeStep(0);
#ifndef NDEBUG
  int numRequestedTimeSteps = 0;
#endif
  vtkInformationDoubleKey* timeKey =
    static_cast<vtkInformationDoubleKey*>
    (vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());
  if (outInfo->Has(timeKey))
    {
#ifndef NDEBUG
    numRequestedTimeSteps = 1;
#endif
    requestedTimeStep = outInfo->Get(timeKey);
    }

  // print out how many steps are requested, just for my information
  vtkDebugMacro
    ( << "Num Time steps requested: " << numRequestedTimeSteps << endl);

  // At this time, it seems to only get one timestep of info, why?

  this->DTime = requestedTimeStep;
  vtkDebugMacro(<< "this->DTime: " << this->DTime << endl);
  double dTimeTemp = this->DTime;
  output->GetInformation()->Set
    (vtkDataObject::DATA_TIME_STEP(), dTimeTemp);
  vtkDebugMacro(<< "dTimeTemp: " << dTimeTemp << endl);
  this->DTime = dTimeTemp;

  // Examine each variable to see if it is selected
  for (int var = 0; var < this->NumberOfPointVars; var++)
    {

    // Is this variable requested
    if (this->PointDataArraySelection->GetArraySetting(var))
      {
      vtkDebugMacro( << "Loading Point Variable: " << var << endl);
      if (!this->LoadPointVarData(var, this->DTime))
        {
        return 0;
        }
      output->GetPointData()->AddArray(this->PointVarDataArray[var]);

    }
  }

  for (int var = 0; var < this->NumberOfCellVars; var++)
    {
    if (this->CellDataArraySelection->GetArraySetting(var))
      {
      vtkDebugMacro
        ( << "Loading Cell Variable: "
          << this->Internals->cellVars[var]->name() << endl);
      this->LoadCellVarData(var, this->DTime);
      output->GetCellData()->AddArray(this->CellVarDataArray[var]);
      }
    }

  this->DataRequested = true;

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

  this->IncludeTopography = false;
  this->DoBugFix = false;
  this->CenterRad = CenterLon * vtkMath::Pi() / 180.0;

  this->PointX = NULL;
  this->PointY = NULL;
  this->PointZ = NULL;
  this->OrigConnections = NULL;
  this->ModConnections = NULL;
  this->CellMap = NULL;
  this->PointMap = NULL;
  this->MaximumLevelPoint = NULL;

  this->FileName = NULL;
  this->DTime = 0;
  this->CellVarDataArray = NULL;
  this->PointVarDataArray = NULL;
  this->PointVarData = NULL;
  this->TimeSteps = NULL;

  this->NumberOfPointVars = 0;
  this->NumberOfCellVars = 0;
  this->MaximumPoints = 0;
  this->MaximumCells = 0;
}

//----------------------------------------------------------------------------
// Get dimensions of key NetCDF variables
//----------------------------------------------------------------------------

int vtkMPASReader::GetNcDims()
{
  NcFile *pnf = this->Internals->ncFile;

  CHECK_DIM(pnf, "nCells");
  NcDim* nCells = pnf->get_dim("nCells");
  this->NumberOfPoints = nCells->size();
  this->PointOffset = 1;

  CHECK_DIM(pnf, "nVertices");
  NcDim* nVertices = pnf->get_dim("nVertices");
  this->NumberOfCells = nVertices->size();
  this->CellOffset = 0;

  CHECK_DIM(pnf, "vertexDegree");
  NcDim* vertexDegree = pnf->get_dim("vertexDegree");
  this->PointsPerCell = vertexDegree->size();

  CHECK_DIM(pnf, "Time");
  NcDim* Time = pnf->get_dim("Time");
  this->NumberOfTimeSteps = Time->size();

  CHECK_DIM(pnf, "nVertLevels");
  NcDim* nVertLevels = pnf->get_dim("nVertLevels");
  this->MaximumNVertLevels = nVertLevels->size();

  return 1;
}

//----------------------------------------------------------------------------
//  Check parameters are valid
//----------------------------------------------------------------------------

int vtkMPASReader::CheckParams()
{

  if ((this->PointsPerCell != 3) && (this->PointsPerCell != 4))
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
  this->VerticalLevelRange[1] = this->MaximumNVertLevels-1;

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

    // for 1 dimension, must have format:
    // (nCells)
    //
    // for 2 dimensions, must have format:
    // (Time, nCells | nVertices, nVertLevels | nVertLevelsP1)
    //
    // for 3 dimensions, must have format:
    // (Time, nCells | nVertices, nVertLevels | nVertLevelsP1)

    int numDims = aVar->num_dims();
    //cout << "Num Dims of var: " << aVar->name() << " is " << numDims << endl;
    if (numDims > 3)
      {
      continue;
      }

    // TODO, check if it is a double
    // assume a double for now

    // check for Time dim 0
    NcToken dim0Name = aVar->get_dim(0)->name();
    if (numDims == 1)
      {
      if (strcmp(dim0Name, "nCells"))
        {
        continue;
        }
      }
    else
      {
      if (strcmp(dim0Name, "Time"))
        {
        continue;
        }
      }

    // check for dim 1 being cell or point
    bool isCellData = false;
    bool isPointData = false;
    if (numDims == 1)
      {
      isPointData = true;
      }
    else if (numDims == 2 || numDims == 3)
      {
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
      }

    // 3D fields are defined over nVertLevels.
    if (numDims == 3)
      {
      // check if dim 2 is nVertLevels or nVertLevelsP1, too
      NcToken dim2Name = aVar->get_dim(2)->name();
      if ((strcmp(dim2Name, "nVertLevels"))
          && (strcmp(dim2Name, "nVertLevelsP1")))
        {
        continue;
        }
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

  this->NumberOfPointVars = pointVarIndex+1;
  this->NumberOfCellVars = cellVarIndex+1;
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

  vtkDebugMacro( << "NumberOfCellVars: " << this->NumberOfCellVars
      << " NumberOfPointVars: " << this->NumberOfPointVars << endl);

  for (int var = 0; var < this->NumberOfPointVars; var++)
    {
    this->PointDataArraySelection->
      EnableArray((const char*)(this->Internals->pointVars[var]->name()));
    vtkDebugMacro
      (<< "Adding point var: "
       << this->Internals->pointVars[var]->name() << endl);
    }

  for (int var = 0; var < this->NumberOfCellVars; var++)
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
  vtkDebugMacro(<<"pointVarData: Alloc " << this->MaximumPoints << " doubles" << endl);
  if(this->PointVarData)
    {
    delete []this->PointVarData;
    }
  this->PointVarData = new double[this->MaximumPoints];

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
  this->PointX = (double*)malloc((this->NumberOfPoints+this->PointOffset) *
                                 sizeof(double));
  CHECK_MALLOC(this->PointX);
  NcVar*  xCellVar = ncFile->get_var("xCell");
  xCellVar->get(this->PointX + this->PointOffset, this->NumberOfPoints);
  // point 0 is 0.0
  this->PointX[0] = 0.0;

  CHECK_VAR(ncFile, "yCell");
  this->PointY = (double*)malloc((this->NumberOfPoints+this->PointOffset) *
                                 sizeof(double));
  CHECK_MALLOC(this->PointY);
  NcVar*  yCellVar = ncFile->get_var("yCell");
  yCellVar->get(this->PointY + this->PointOffset, this->NumberOfPoints);
  // point 0 is 0.0
  this->PointY[0] = 0.0;

  CHECK_VAR(ncFile, "zCell");
  this->PointZ = (double*)malloc((this->NumberOfPoints+this->PointOffset) *
                                 sizeof(double));
  CHECK_MALLOC(this->PointZ);
  NcVar*  zCellVar = ncFile->get_var("zCell");
  zCellVar->get(this->PointZ + this->PointOffset, this->NumberOfPoints);
  // point 0 is 0.0
  this->PointZ[0] = 0.0;

  CHECK_VAR(ncFile, "cellsOnVertex");
  this->OrigConnections = (int *) malloc(this->NumberOfCells *
                                         this->PointsPerCell * sizeof(int));
  CHECK_MALLOC(this->OrigConnections);
  NcVar *connectionsVar = ncFile->get_var("cellsOnVertex");
  connectionsVar->get(this->OrigConnections, this->NumberOfCells,
                      this->PointsPerCell);

  if (isNcVar(ncFile, "maxLevelCell"))
    {
    this->IncludeTopography = true;
    this->MaximumLevelPoint = (int*)malloc((this->NumberOfPoints +
                                            this->PointOffset) * sizeof(int));
    CHECK_MALLOC(this->MaximumLevelPoint);
    NcVar *maxLevelPointVar = ncFile->get_var("maxLevelCell");
    maxLevelPointVar->get(this->MaximumLevelPoint + this->PointOffset,
                          this->NumberOfPoints);
    }

  this->CurrentExtraPoint = this->NumberOfPoints + this->PointOffset;
  this->CurrentExtraCell = this->NumberOfCells + this->CellOffset;

  if (this->ShowMultilayerView)
    {
    this->MaximumCells = this->CurrentExtraCell*this->MaximumNVertLevels;
    vtkDebugMacro
      (<< "alloc sphere: multilayer: setting MaximumCells to " << this->MaximumCells);
    this->MaximumPoints = this->CurrentExtraPoint*(this->MaximumNVertLevels+1);
    vtkDebugMacro
      (<< "alloc sphere: multilayer: setting MaximumPoints to " << this->MaximumPoints);
    }
  else
    {
    this->MaximumCells = this->CurrentExtraCell;
    this->MaximumPoints = this->CurrentExtraPoint;
    vtkDebugMacro
      (<< "alloc sphere: singlelayer: setting MaximumPoints to " << this->MaximumPoints);

    if (isNcVar(ncFile, "vertexMask"))
      {
      this->CellMask = (int*)malloc(this->NumberOfCells*sizeof(int));
      CHECK_MALLOC(this->CellMask);
      NcVar*  cellMask = ncFile->get_var("vertexMask");
      cellMask->set_cur(0, this->VerticalLevelSelected);
      cellMask->get(this->CellMask, this->NumberOfCells, 1);
      }
    else
      {
      free(this->CellMask);
      this->CellMask = 0;
      }
    }
  vtkDebugMacro(<< "Leaving AllocSphereGeometry...");

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
  this->ModNumPoints = (int)floor(this->NumberOfPoints*(1.0 + BLOATFACTOR));
  this->ModNumCells = (int)floor(this->NumberOfCells*(1.0 + BLOATFACTOR))+1;

  CHECK_VAR(ncFile, "lonCell");
  this->PointX = (double*)malloc(this->ModNumPoints * sizeof(double));
  CHECK_MALLOC(this->PointX);
  NcVar*  xCellVar = ncFile->get_var("lonCell");
  xCellVar->get(this->PointX + this->PointOffset, this->NumberOfPoints);
  // point 0 is 0.0
  this->PointX[0] = 0.0;

  CHECK_VAR(ncFile, "latCell");
  this->PointY = (double*)malloc(this->ModNumPoints * sizeof(double));
  CHECK_MALLOC(this->PointY);
  NcVar*  yCellVar = ncFile->get_var("latCell");
  yCellVar->get(this->PointY+this->PointOffset, this->NumberOfPoints);
  // point 0 is 0.0
  this->PointY[0] = 0.0;

  CHECK_VAR(ncFile, "cellsOnVertex");
  this->OrigConnections = (int *) malloc(this->NumberOfCells * this->PointsPerCell *
                                         sizeof(int));
  CHECK_MALLOC(this->OrigConnections);
  NcVar *connectionsVar = ncFile->get_var("cellsOnVertex");
  connectionsVar->get(this->OrigConnections, this->NumberOfCells,
                      this->PointsPerCell);

  // create my own list to include modified origConnections (due to
  // eliminating wraparound in the lat/lon projection) plus additional
  // cells added when mirroring cells that had previously wrapped around

  this->ModConnections = (int *) malloc(this->ModNumCells * this->PointsPerCell
                                        * sizeof(int));
  CHECK_MALLOC(this->ModConnections);

  // allocate an array to map the extra points and cells to the original
  // so that when obtaining data, we know where to get it
  this->PointMap = (int*)malloc((int)floor(this->NumberOfPoints*BLOATFACTOR)
                                * sizeof(int));
  CHECK_MALLOC(this->PointMap);
  this->CellMap = (int*)malloc((int)floor(this->NumberOfCells*BLOATFACTOR)
                               * sizeof(int));
  CHECK_MALLOC(this->CellMap);

  if (isNcVar(ncFile, "maxLevelCell"))
    {
    this->IncludeTopography = true;
    this->MaximumLevelPoint = (int*)malloc((this->NumberOfPoints + this->NumberOfPoints) * sizeof(int));
    CHECK_MALLOC(this->MaximumLevelPoint);
    NcVar *maxLevelPointVar = ncFile->get_var("maxLevelCell");
    maxLevelPointVar->get(this->MaximumLevelPoint + this->PointOffset,
                          this->NumberOfPoints);
    }

  this->CurrentExtraPoint = this->NumberOfPoints + this->PointOffset;
  this->CurrentExtraCell = this->NumberOfCells + this->CellOffset;

  if (ShowMultilayerView)
    {
    this->MaximumCells = this->CurrentExtraCell*this->MaximumNVertLevels;
    this->MaximumPoints = this->CurrentExtraPoint*(this->MaximumNVertLevels+1);
    vtkDebugMacro
      (<< "alloc latlon: multilayer: setting this->MaximumPoints to " << this->MaximumPoints
       << endl);
    }
  else
    {
    this->MaximumCells = this->CurrentExtraCell;
    this->MaximumPoints = this->CurrentExtraPoint;
    vtkDebugMacro
      (<< "alloc latlon: singlelayer: setting this->MaximumPoints to " << this->MaximumPoints
       << endl);

    if (isNcVar(ncFile, "vertexMask"))
      {
      CHECK_VAR(ncFile, "vertexMask");
      this->CellMask = (int*)malloc(this->ModNumCells*sizeof(int));
      CHECK_MALLOC(this->CellMask);
      NcVar*  cellMask = ncFile->get_var("vertexMask");
      cellMask->set_cur(0, this->VerticalLevelSelected);
      cellMask->get(this->CellMask, this->NumberOfCells, 1);
      }
    else
      {
      free(this->CellMask);
      this->CellMask = 0;
      }
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
    for (int j = this->PointOffset; j < this->NumberOfPoints + this->PointOffset; j++)
      {
      // need to shift over the point so center is at PI
      if (this->PointX[j] < 0)
        {
        this->PointX[j] += 2*vtkMath::Pi();
        }
      }
    }

  if (CenterLon != 180)
    {
    for (int j = this->PointOffset; j < this->NumberOfPoints + this->PointOffset; j++)
      {
      // need to shift over the point if centerLon dictates
      if (this->CenterRad < vtkMath::Pi())
        {
        if (this->PointX[j] > (this->CenterRad + vtkMath::Pi()))
          {
          this->PointX[j] = -((2*vtkMath::Pi()) - this->PointX[j]);
          }
        }
      else if (this->CenterRad > vtkMath::Pi())
        {
        if (this->PointX[j] < (this->CenterRad - vtkMath::Pi()))
          {
          this->PointX[j] += 2*vtkMath::Pi();
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
  double X = this->PointX[index];
  double Y = this->PointY[index];

  // add on east
  if (X < dividerX)
    {
    X += 2*vtkMath::Pi();
    }
  else
    {
    // add on west
    X -= 2*vtkMath::Pi();
    }

  this->PointX[this->CurrentExtraPoint] = X;
  this->PointY[this->CurrentExtraPoint] = Y;

  int mirrorPoint = this->CurrentExtraPoint;

  // record mapping
  *(this->PointMap + (this->CurrentExtraPoint - this->NumberOfPoints - this->PointOffset)) = index;
  this->CurrentExtraPoint++;

  //vtkDebugMacro(<< "Leaving AddMirrorPoint..." << endl);
  return mirrorPoint;
}


//----------------------------------------------------------------------------
// Check for out-of-range values and do bugfix
//----------------------------------------------------------------------------

void vtkMPASReader::FixPoints()
{
  vtkDebugMacro(<< "In FixPoints..." << endl);

  for (int j = this->CellOffset; j < this->NumberOfCells + this->CellOffset; j++ )
    {
    int *conns = this->OrigConnections + (j * this->PointsPerCell);

    // go through and make sure none of the referenced points are
    // out of range
    // if so, set all to point 0
    for (int k = 0; k < this->PointsPerCell; k++)
      {
      if  ((conns[k] <= 0) || (conns[k] > this->NumberOfPoints))
        {
        for (int m = 0; m < this->PointsPerCell; m++)
          {
          conns[m] = 0;
          }
        break;
        }
      }

    if (this->DoBugFix)
      {
      //BUG FIX for problem where cells are stretching to a faraway point
      int lastk = this->PointsPerCell-1;
      const double thresh = .06981317007977; // 4 degrees
      for (int k = 0; k < this->PointsPerCell; k++)
        {
        double ydiff = abs(this->PointY[conns[k]]
                           - this->PointY[conns[lastk]]);
        // Don't look at cells at map border
        if (ydiff > thresh)
          {
          for (int m = 0; m < this->PointsPerCell; m++)
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
  for (int j = this->CellOffset; j < this->NumberOfCells + this->CellOffset; j++ )
    {
    int *conns = this->OrigConnections + (j * this->PointsPerCell);
    int *modConns = this->ModConnections + (j * this->PointsPerCell);

    // Determine if we are wrapping in X direction
    int lastk = this->PointsPerCell-1;
    bool xWrap = false;
    for (int k = 0; k < this->PointsPerCell; k++)
      {
      if (abs(this->PointX[conns[k]]
              - this->PointX[conns[lastk]]) > 5.5)
        {
        xWrap = true;
        }
      lastk = k;
      }

    // If we wrapped in X direction, modify cell and add mirror cell
    if (xWrap)
      {

      // first point is anchor it doesn't move
      double anchorX = this->PointX[conns[0]];
      modConns[0] = conns[0];

      // modify existing cell, so it doesn't wrap
      // move points to one side
      for (int k = 1; k < this->PointsPerCell; k++)
        {
        int neigh = conns[k];

        // add a new point, figure out east or west

        if (abs(this->PointX[neigh] - anchorX) > 5.5)
          {
          modConns[k] = AddMirrorPoint(neigh, anchorX);
          }
        else
          {
          // use existing kth point
          modConns[k] = neigh;
          }
        }

      if (this->CellMask)
        {
        this->CellMask[this->CurrentExtraCell] = this->CellMask[j];
        }

      // move addedConns to this->ModConnections extra cells area
      int* addedConns = this->ModConnections
        + (this->CurrentExtraCell * this->PointsPerCell);

      // add a mirroring cell to other side

      // add mirrored anchor first
      addedConns[0] = AddMirrorPoint(conns[0], this->CenterRad);
      anchorX = this->PointX[addedConns[0]];

      // add mirror cell points if needed
      for (int k = 1; k < this->PointsPerCell; k++)
        {
        int neigh = conns[k];

        // add a new point for neighbor, figure out east or west
        if (abs(this->PointX[neigh] - anchorX) > 5.5)
          {
          addedConns[k] = AddMirrorPoint(neigh, anchorX);
          }
        else
          {
          // use existing kth point
          addedConns[k] = neigh;
          }
        }
      *(this->CellMap + (this->CurrentExtraCell - this->NumberOfCells - this->CellOffset)) = j;
      this->CurrentExtraCell++;
      }
    else
      {

      // just add cell "as is" to this->ModConnections
      for (int k=0; k< this->PointsPerCell; k++)
        {
        modConns[k] = conns[k];
        }
      }
    if (this->CurrentExtraCell > this->ModNumCells)
      {
      vtkErrorMacro( << "Exceeded storage for extra cells!" << endl);
      return(0);
      }
    if (this->CurrentExtraPoint > this->ModNumPoints)
      {
      vtkErrorMacro( << "Exceeded storage for extra points!" << endl);
      return(0);
      }
    }

  if (!ShowMultilayerView)
    {
    this->MaximumCells = this->CurrentExtraCell;
    this->MaximumPoints = this->CurrentExtraPoint;
    vtkDebugMacro
      (<< "elim xwrap: singlelayer: setting this->MaximumPoints to " << this->MaximumPoints
       << endl);
    }
  else
    {
    this->MaximumCells = this->CurrentExtraCell*this->MaximumNVertLevels;
    this->MaximumPoints = this->CurrentExtraPoint*(this->MaximumNVertLevels+1);
    vtkDebugMacro
      (<< "elim xwrap: multilayer: setting this->MaximumPoints to " <<
       this->MaximumPoints << endl);
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
    (<< "OutputPoints: this->MaximumPoints: " << this->MaximumPoints << " this->MaximumNVertLevels: "
     << this->MaximumNVertLevels << " LayerThickness: " << LayerThickness
     <<"ProjectLatLon: " << ProjectLatLon << " ShowMultilayerView: "
     << ShowMultilayerView << endl);

  if (init)
    {
    points = vtkSmartPointer<vtkPoints>::New();
    points->Allocate(this->MaximumPoints, this->MaximumPoints);
    output->SetPoints(points);
    }
  else
    {
    points = output->GetPoints();
    points->Initialize();
    points->Allocate(this->MaximumPoints, this->MaximumPoints);
    }

  for (int j = 0; j < this->CurrentExtraPoint; j++ )
    {

    double x, y, z;

    if (ProjectLatLon)
      {
      x = this->PointX[j] * 180.0 / vtkMath::Pi();
      y = this->PointY[j] * 180.0 / vtkMath::Pi();
      z = 0.0;
      }
    else
      {
      x = this->PointX[j];
      y = this->PointY[j];
      z = this->PointZ[j];
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

      for (int levelNum = 0; levelNum < this->MaximumNVertLevels+1; levelNum++)
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
        //   points->SetPoint(j*(this->MaximumNVertLevels+1) + levelNum, x, y, z);
        points->InsertNextPoint(x, y, z);
        }
      }
    }

  if (this->PointX)
    {
    free(this->PointX);
    this->PointX = NULL;
    }
  if (this->PointY)
    {
    free(this->PointY);
    this->PointY = NULL;
    }
  if (this->PointZ)
    {
    free(this->PointZ);
    this->PointZ = NULL;
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
  switch (this->PointsPerCell) {
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
    output->Allocate(this->MaximumCells, this->MaximumCells);
    }
  else
    {
    vtkCellArray* cells = output->GetCells();
    cells->Initialize();
    output->Allocate(this->MaximumCells, this->MaximumCells);
    }

  int cellType = GetCellType();
  int val;

  int pointsPerPolygon;
  if (this->ShowMultilayerView)
    {
    pointsPerPolygon = 2 * this->PointsPerCell;
    }
  else
    {
    pointsPerPolygon = this->PointsPerCell;
    }

  vtkDebugMacro
    (<< "OutputCells: init: " << init << " this->MaximumCells: " << this->MaximumCells
     << " cellType: " << cellType << " this->MaximumNVertLevels: " << this->MaximumNVertLevels
     << " LayerThickness: " << LayerThickness << " ProjectLatLon: "
     << ProjectLatLon << " ShowMultilayerView: " << ShowMultilayerView);

  std::vector<vtkIdType> polygon(pointsPerPolygon);

  for (int j = 0; j < this->CurrentExtraCell ; j++)
    {

    int* conns;
    if (this->ProjectLatLon)
      {
      conns = this->ModConnections + (j * this->PointsPerCell);
      }
    else
      {
      conns = this->OrigConnections + (j * this->PointsPerCell);
      }

    int minLevel= 0;

    if (this->IncludeTopography)
      {
      int* connections;

      //check if it is a mirror cell, if so, get original
      if (j >= this->NumberOfCells + this->CellOffset)
        {
        //cout << "setting origCell" << endl;
        int origCellNum = *(this->CellMap + (j - this->NumberOfCells - this->CellOffset));
        //cout << "mirror cell: " <<j<< " origCell: "<< origCell << endl;
        connections = this->OrigConnections + (origCellNum*this->PointsPerCell);
        }
      else
        {
        connections = this->OrigConnections + (j * this->PointsPerCell);
        }

      //cout << "calc minLevel" << endl;
      minLevel = this->MaximumLevelPoint[connections[0]];
      //cout << "initial minLevel:" << minLevel << endl;
      // Take the min of the this->MaximumLevelPoint of each point
      for (int k = 1; k < this->PointsPerCell; k++)
        {
        minLevel = min(minLevel, this->MaximumLevelPoint[connections[k]]);
        }
      //cout << endl;
      }

    // singlelayer
    if (!this->ShowMultilayerView)
      {
      // If that min is greater than or equal to this output level,
      // include the cell, otherwise set all points to zero.

      if (this->IncludeTopography && ((minLevel-1) < this->VerticalLevelSelected))
        {
        //cerr << "Setting all points to zero" << endl;
        val = 0;
        for (int k = 0; k < this->PointsPerCell; k++)
          {
          polygon[k] = val;
          }
        }
      else
        {
        for (int k = 0; k < this->PointsPerCell; k++)
          {
          polygon[k] = conns[k];
          }
        }
      output->InsertNextCell(cellType, pointsPerPolygon, &polygon[0]);

    }
    else
      { // multilayer
      // for each level, write the cell
      for (int levelNum = 0; levelNum < this->MaximumNVertLevels; levelNum++)
        {
        if (this->IncludeTopography && ((minLevel-1) < levelNum))
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
          for (int k = 0; k < this->PointsPerCell; k++)
            {
            val = (conns[k]*(this->MaximumNVertLevels+1)) + levelNum;
            polygon[k] = val;
            }

          for (int k = 0; k < this->PointsPerCell; k++)
            {
            val = (conns[k]*(this->MaximumNVertLevels+1)) + levelNum + 1;
            polygon[k+this->PointsPerCell] = val;
            }
          }
        //vtkDebugMacro
        //("InsertingCell j: " << j << " level: " << levelNum << endl);
        output->InsertNextCell(cellType, pointsPerPolygon, &polygon[0]);
        }
      }
    }

  if (this->CellMask)
    {
    vtkIntArray* cellMask = vtkIntArray::New();
    cellMask->SetArray(this->CellMask,
                       this->CurrentExtraCell,
                       0,
                       vtkIntArray::VTK_DATA_ARRAY_FREE);
    cellMask->SetName("Mask");
    output->GetCellData()->AddArray(cellMask);
    this->CellMask = NULL;
    }

  free(this->ModConnections); this->ModConnections = NULL;
  free(this->OrigConnections); this->OrigConnections = NULL;

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

  if (this->PointVarDataArray[variableIndex] == NULL)
    {
    vtkDebugMacro
      (<< "allocating data array in vtkMPASReader::LoadPointVarData" << endl);
    this->PointVarDataArray[variableIndex] = vtkDoubleArray::New();
    this->PointVarDataArray[variableIndex]->SetName
      (this->Internals->pointVars[variableIndex]->name());
    this->PointVarDataArray[variableIndex]->SetNumberOfTuples
      (this->MaximumPoints);
    this->PointVarDataArray[variableIndex]->SetNumberOfComponents(1);
    }

  vtkDebugMacro(<< "getting pointer in vtkMPASReader::LoadPointVarData"
                << endl);
  double* dataBlock = this->PointVarDataArray[variableIndex]->GetPointer(0);

  vtkDebugMacro( << "dTimeStep requested: " << dTimeStep << endl);
  int timestep = min((int)floor(dTimeStep),
                     (int)(this->NumberOfTimeSteps-1));
  vtkDebugMacro( << "Time: " << timestep << endl);


  int numDims = ncVar->num_dims();

  // singlelayer
  if (!ShowMultilayerView)
    {
    // we can go ahead and read it into the datablock
    if (numDims == 1)
      {
      ncVar->set_cur((long)0);
      ncVar->get(dataBlock+this->PointOffset, this->NumberOfPoints);
      }
    else if (numDims == 2)
      {
      ncVar->set_cur(timestep, 0);
      ncVar->get(dataBlock+this->PointOffset, 1, this->NumberOfPoints);
      }
    else if (numDims == 3)
      {
      ncVar->set_cur(timestep, 0, this->VerticalLevelSelected);
      ncVar->get(dataBlock+this->PointOffset, 1, this->NumberOfPoints, 1);
      }

    dataBlock[0] = dataBlock[1];
    // data is all in place, don't need to do next step
    }
  else
    { // multilayer
    double* dataPtr = this->PointVarData +
                      (this->MaximumNVertLevels * this->PointOffset);
    if (numDims == 1)
      {
      ncVar->set_cur((long)0);
      ncVar->get(dataPtr, this->NumberOfPoints);
      }
    else if (numDims == 2)
      {
      ncVar->set_cur(timestep, 0);
      ncVar->get(dataPtr, 1, this->NumberOfPoints);
      }
    else if (numDims == 3)
      {
      ncVar->set_cur(timestep, 0, 0);
      ncVar->get(dataPtr, 1, this->NumberOfPoints, this->MaximumNVertLevels);
      }
    if (numDims == 1 || numDims == 2)
      {
      // need to replicate data over all vertical layers
      // layout in memory needs to be:
      // pt1, pt1, ..., (VertLevels times), pt2, pt2, ..., (VertLevels times),
      // need to go backwards through the points in order to not overwrite
      // anything.
      for(int i=this->NumberOfPoints; i>0; i--)
        {
        // point to copy
        double pt = *(dataPtr + i - 1);

        // where to start copying
        double* copyPtr = dataPtr + (i-1)*this->MaximumNVertLevels;

        for(int j=0; j<this->MaximumNVertLevels; j++)
          {
          *copyPtr = pt;
          copyPtr++;
          }
        }
      }
    }

  vtkDebugMacro
    (<< "got point data in vtkMPASReader::LoadPointVarData" << endl);

  int i=0, k=0;

  if (ShowMultilayerView)
    {

    // put in dummy points
    for (int levelNum = 0; levelNum < this->MaximumNVertLevels; levelNum++)
      {
      dataBlock[levelNum] = this->PointVarData[this->MaximumNVertLevels + levelNum];
      }
    // write highest level dummy point (duplicate of last level)
    dataBlock[this->MaximumNVertLevels] = this->PointVarData[this->MaximumNVertLevels+this->MaximumNVertLevels-1];

    vtkDebugMacro (<< "Wrote dummy vtkMPASReader::LoadPointVarData" << endl);

    // put in other points
    for (int j = this->PointOffset; j < this->NumberOfPoints + this->PointOffset; j++)
      {

      i = j*(this->MaximumNVertLevels+1);
      k = j*(this->MaximumNVertLevels);

      // write data for one point -- lowest level to highest
      for (int levelNum = 0; levelNum < this->MaximumNVertLevels; levelNum++)
        {
        dataBlock[i++] = this->PointVarData[k++];
        }

      // for last layer of points, repeat last level's values
      // Need Mark's input on this one
      dataBlock[i++] = this->PointVarData[--k];
      //vtkDebugMacro (<< "Wrote j:" << j << endl);
      }

    }

  vtkDebugMacro (<< "Wrote next pts vtkMPASReader::LoadPointVarData" << endl);

  vtkDebugMacro
    (<< "this->NumberOfPoints: " << this->NumberOfPoints << " this->CurrentExtraPoint: "
     << this->CurrentExtraPoint << endl);

  // put out data for extra points
  for (int j = this->PointOffset + this->NumberOfPoints; j < this->CurrentExtraPoint; j++)
    {
    // use map to find out what point data we are using

    if (!ShowMultilayerView)
      {
      k = this->PointMap[j - this->NumberOfPoints - this->PointOffset];
      dataBlock[j] = dataBlock[k];
      }
    else
      {
      k = this->PointMap[j - this->NumberOfPoints - this->PointOffset]*this->MaximumNVertLevels;
      // write data for one point -- lowest level to highest
      for (int levelNum = 0; levelNum < this->MaximumNVertLevels; levelNum++)
        {
        dataBlock[i++] = this->PointVarData[k++];
        }

      // for last layer of points, repeat last level's values
      // Need Mark's input on this one
      dataBlock[i++] = this->PointVarData[--k];
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
  if (this->CellVarDataArray[variableIndex] == NULL)
    {
    this->CellVarDataArray[variableIndex] = vtkDoubleArray::New();
    vtkDebugMacro
      ( << "Allocated cell var index: "
        << this->Internals->cellVars[variableIndex]->name() << endl);
    this->CellVarDataArray[variableIndex]->SetName
      (this->Internals->cellVars[variableIndex]->name());
    this->CellVarDataArray[variableIndex]->SetNumberOfTuples
      (this->MaximumCells);
    this->CellVarDataArray[variableIndex]->SetNumberOfComponents(1);
    }

  vtkDebugMacro(<< "getting pointer in vtkMPASReader::LoadCellVarData" << endl);

  double* dataBlock = this->CellVarDataArray[variableIndex]->GetPointer(0);

  vtkDebugMacro( << "dTimeStep requested: " << dTimeStep << endl);
  int timestep = min((int)floor(dTimeStep),
                     (int)(this->NumberOfTimeSteps-1));
  vtkDebugMacro( << "Time: " << timestep << endl);

  ncVar->set_cur(timestep, 0, this->VerticalLevelSelected);

  if (!ShowMultilayerView)
    {
    ncVar->get(dataBlock, 1, this->NumberOfCells, 1);
    }
  else
    {
    ncVar->get(dataBlock, 1, this->NumberOfCells, this->MaximumNVertLevels);
    }

  vtkDebugMacro(<< "Got data for cell var: "
                << this->Internals->cellVars[variableIndex]->name()
                << endl);

  // put out data for extra cells
  for (int j = this->CellOffset + this->NumberOfCells; j < this->CurrentExtraCell; j++)
    {
    // use map to find out what cell data we are using

    if (!ShowMultilayerView)
      {
      int k = this->CellMap[j - this->NumberOfCells - this->CellOffset];
      dataBlock[j] = dataBlock[k];
      }
    else
      {
      int i = j*this->MaximumNVertLevels;
      int k = this->CellMap[j - this->NumberOfCells - this->CellOffset]*this->MaximumNVertLevels;
      // write data for one cell -- lowest level to highest
      for (int levelNum = 0; levelNum < this->MaximumNVertLevels; levelNum++)
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
  for (int var = 0; var < this->NumberOfPointVars; var++)
    {

    // Is this variable requested
    if (this->PointDataArraySelection->GetArraySetting(var))
      {
      vtkDebugMacro( << "Loading Point Variable: " << var << endl);
      if (!LoadPointVarData(var, this->DTime))
        {
        return 0;
        }
      output->GetPointData()->AddArray(this->PointVarDataArray[var]);
      }
    }

  for (int var = 0; var < this->NumberOfCellVars; var++)
    {
    if (this->CellDataArraySelection->GetArraySetting(var))
      {
      vtkDebugMacro
        ( << "Loading Cell Variable: "
          << this->Internals->cellVars[var]->name() << endl);
      if (!LoadCellVarData(var, this->DTime))
        {
        return 0;
        }
      output->GetCellData()->AddArray(this->CellVarDataArray[var]);
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
  vtkDebugMacro( << "Set VerticalLevelSelected to: " << level);

  vtkDebugMacro( << "InfoRequested?: " << this->InfoRequested);

  if (!this->InfoRequested)
    {
    return;
    }
  if (!this->DataRequested)
    {
    return;
    }

  // Examine each variable to see if it is selected
  for (int var = 0; var < this->NumberOfPointVars; var++)
    {
    // Is this variable requested
    if (this->PointDataArraySelection->GetArraySetting(var))
      {
      vtkDebugMacro( << "Loading Point Variable: "
                     << this->Internals->pointVars[var]->name() << endl);
      LoadPointVarData(var, this->DTime);
      }
    }

  for (int var = 0; var < this->NumberOfCellVars; var++)
    {
    if (this->CellDataArraySelection->GetArraySetting(var))
      {
      vtkDebugMacro( << "Loading Cell Variable: "
                     << this->Internals->cellVars[var]->name() << endl);
      LoadCellVarData(var, this->DTime);
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
      if (!this->InfoRequested)
        {
        return;
        }
      if (!this->DataRequested)
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
    this->CenterRad = CenterLon * vtkMath::Pi() / 180.0;
    vtkDebugMacro( << "this->CenterRad set to " << this->CenterRad << endl);
    if (ProjectLatLon)
      {
      // Don't regenerate if we've never done an initial read
      if (!this->InfoRequested)
        {
        return;
        }
      if (!this->DataRequested)
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
    if (!this->InfoRequested)
      {
      return;
      }
    if (!this->DataRequested)
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
    if (!this->InfoRequested)
      {
      return;
      }
    if (!this->DataRequested)
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
    if (!this->InfoRequested)
      {
      return;
      }
    if (!this->DataRequested)
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
    if (!this->InfoRequested)
      {
      return;
      }
    if (!this->DataRequested)
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
  NcFile ncFile(filename);
  if (!ncFile.is_valid())
    {
    return 0;
    }
  bool ret = true;
  ret &= isNcDim(&ncFile, "nCells");
  ret &= isNcDim(&ncFile, "nVertices");
  ret &= isNcDim(&ncFile, "vertexDegree");
  ret &= isNcDim(&ncFile, "Time");
  ret &= isNcDim(&ncFile, "nVertLevels");
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
  os << indent << "VerticalLevelRange: " 
     << this->VerticalLevelRange[0] << ","
     << this->VerticalLevelRange[1] << "\n";
  os << indent << "this->NumberOfPointVars: " << this->NumberOfPointVars << "\n";
  os << indent << "this->NumberOfCellVars: " << this->NumberOfCellVars << "\n";
  os << indent << "this->MaximumPoints: " << this->MaximumPoints << "\n";
  os << indent << "this->MaximumCells: " << this->MaximumCells << "\n";
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
