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
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStringArray.h"
#include "vtkToolkits.h"
#include "vtkUnstructuredGrid.h"

#include "vtk_netcdfcpp.h"

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <cstdarg>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

// In VTK commit 64cb89e3e6ae08f440eb6d4cbfb308c41ab7d258, a lot of signatures
// in the netcdf library changed size values from 'long' to 'size_t'. However,
// upstream versions of netcdf are using long. The following typedef resolves
// this issue.
// We cannot just check for VTK_USE_SYSTEM_NETCDF because ParaView's superbuild
// installs a "system" netcdf with the same modifications...
#if defined(VTK_NETCDFCPP_USE_SIZE_T) || defined(VTK_NETCDF_USE_SIZE_T)
typedef size_t nc_size_t;
#else
typedef long nc_size_t;
#endif


// Restricted to the supported NcType-convertible types.
#define vtkNcTemplateMacro(call)                                               \
  vtkTemplateMacroCase(VTK_DOUBLE, double, call);                              \
  vtkTemplateMacroCase(VTK_FLOAT, float, call);                                \
  vtkTemplateMacroCase(VTK_INT, int, call);                                    \
  vtkTemplateMacroCase(VTK_SHORT, short, call);                                \
  vtkTemplateMacroCase(VTK_CHAR, char, call);                                  \
  vtkTemplateMacroCase(VTK_SIGNED_CHAR, signed char, call) /* ncbyte */

#define vtkNcDispatch(type, call)                                              \
  switch (type)                                                                \
  {                                                                          \
    vtkNcTemplateMacro(call);                                                  \
    default:                                                                   \
      vtkErrorMacro(<<"Unsupported data type: " << (type));                    \
      abort();                                                                 \
  }

namespace {

std::string dimensionedArrayName(NcVar *var)
{
  std::ostringstream out;
  out << var->name() << "(";
  for (int dim = 0; dim < var->num_dims(); ++dim)
  {
    if (dim != 0)
    {
      out << ", ";
    }
    out << var->get_dim(dim)->name();
  }
  out << ")";
  return out.str();
}

struct DimMetaData
{
  long curIdx;
  long dimSize;
};

} // end anon namespace

//----------------------------------------------------------------------------
// Internal class to avoid name pollution
//----------------------------------------------------------------------------

class vtkMPASReader::Internal
{
public:
  // variableIndex --> vtkDataArray
  typedef std::map<int, vtkSmartPointer<vtkDataArray> > ArrayMap;
  typedef std::map<std::string, DimMetaData> DimMetaDataMap;
  Internal() : ncFile(NULL) {}
  ~Internal() { delete ncFile; }

  NcFile* ncFile;
  std::vector<NcVar*> pointVars;
  std::vector<NcVar*> cellVars;
  ArrayMap pointArrays;
  ArrayMap cellArrays;

  // Returns true if the dimension name is not nCells, nVertices, or Time.
  bool isExtraDim(const std::string &name);

  // Indices at which arbitrary trailing dimensions are fixed:
  DimMetaDataMap dimMetaDataMap;
  vtkTimeStamp dimMetaDataTime;
  // Set of dimensions currently used by the selected arrays:
  vtkNew<vtkStringArray> extraDims;
  vtkTimeStamp extraDimTime;
};

bool vtkMPASReader::Internal::isExtraDim(const string &name)
{
  return name != "nCells" && name != "nVertices" && name != "Time";
}

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

static bool isNcVar(NcFile *ncFile, NcToken name)
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

static bool isNcDim(NcFile *ncFile, NcToken name)
{
  int num_dims = ncFile->num_dims();
  //cerr << "looking for: " << name << endl;
  for (int i = 0; i < num_dims; i++)
  {
    NcDim* ncDim = ncFile->get_dim(i);
    //cerr << "checking " << ncDim->name() << endl;
    const char* ncDimName = ncDim->name();
    if ((ncDimName && strcmp(ncDimName, name)) == 0)
    {
      // we have a match, so return
      return true;
    }
  }
  return false;
}

//----------------------------------------------------------------------------
// Check if there is a NetCDF attribute by that name
//----------------------------------------------------------------------------

static bool isNcAtt(NcFile *ncFile, NcToken name)
{
  int num_atts = ncFile->num_atts();
  for (int i = 0; i < num_atts; i++)
  {
    NcAtt *ncAtt = ncFile->get_att(i);
    if ((strcmp(ncAtt->name(), name)) == 0)
    {
      return true;
    }
  }
  return false;
}

//-----------------------------------------------------------------------------
//  Function to convert cartesian coordinates to spherical, for use in
//  computing points in different layers of multilayer spherical view
//----------------------------------------------------------------------------

static int CartesianToSpherical(double x, double y, double z, double* rho,
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

static int SphericalToCartesian(double rho, double phi, double theta, double* x,
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

  vtkDebugMacro(<< "Created vtkMPASReader" << endl);
}

//----------------------------------------------------------------------------
//  Destroys data stored for variables, points, and cells, but
//  doesn't destroy the list of variables or toplevel cell/pointVarDataArray.
//----------------------------------------------------------------------------

void vtkMPASReader::DestroyData()

{
  vtkDebugMacro(<<"DestroyData...");

  this->Internals->cellArrays.clear();
  this->Internals->pointArrays.clear();

  free(this->CellMap);
  this->CellMap = NULL;

  free(this->PointMap);
  this->PointMap = NULL;

  free(this->MaximumLevelPoint);
  this->MaximumLevelPoint = NULL;
}

//----------------------------------------------------------------------------
// Destructor for MPAS Reader
//----------------------------------------------------------------------------

vtkMPASReader::~vtkMPASReader()
{
  vtkDebugMacro(<< "Destructing vtkMPASReader..." << endl);

  this->SetFileName(NULL);

  delete this->Internals->ncFile;
  this->Internals->ncFile = NULL;

  this->DestroyData();

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

  delete this->Internals;

  vtkDebugMacro(<< "Destructed vtkMPASReader" << endl);
}

//----------------------------------------------------------------------------
void vtkMPASReader::ReleaseNcData()
{
  this->Internals->pointVars.clear();
  this->Internals->pointArrays.clear();
  this->Internals->cellVars.clear();
  this->Internals->cellArrays.clear();

  this->PointDataArraySelection->RemoveAllArrays();
  this->CellDataArraySelection->RemoveAllArrays();
  this->UpdateDimensions(true); // Reset extra dimension list.

  free(this->PointX);
  this->PointX = NULL;
  free(this->PointY);
  this->PointY = NULL;
  free(this->PointZ);
  this->PointZ = NULL;

  free(this->OrigConnections);
  this->OrigConnections = NULL;
  free(this->ModConnections);
  this->ModConnections = NULL;
  free(this->CellMap);
  this->CellMap = NULL;
  free(this->PointMap);
  this->PointMap = NULL;
  free(this->MaximumLevelPoint);
  this->MaximumLevelPoint = NULL;

  delete this->Internals->ncFile;
  this->Internals->ncFile = NULL;
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

  this->ReleaseNcData();

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

  // Get ParaView information pointer
  vtkInformation* outInfo = outVector->GetInformationObject(0);

  this->Internals->ncFile = new NcFile(this->FileName);
  if (!this->Internals->ncFile->is_valid())
  {
    vtkErrorMacro( << "Couldn't open file: " << this->FileName << endl);
    this->ReleaseNcData();
    return 0;
  }

  if (!this->GetNcDims())
  {
    this->ReleaseNcData();
    return 0;
  }

  if (!this->GetNcAtts())
  {
    this->ReleaseNcData();
    return 0;
  }

  if (!this->CheckParams())
  {
    this->ReleaseNcData();
    return 0;
  }

  if (!this->BuildVarArrays())
  {
    this->ReleaseNcData();
    return 0;
  }

  // Collect temporal information

  // At this time, MPAS doesn't have fine-grained time value, just
  // the number of the step, so that is what I store here for TimeSteps.
  if (this->NumberOfTimeSteps > 0)
  {
    // Tell the pipeline what steps are available
    std::vector<double> timeSteps;
    for (int i = 0; i < this->NumberOfTimeSteps; ++i)
    {
      timeSteps.push_back(static_cast<double>(i));
    }
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(),
                 &timeSteps[0], static_cast<int>(timeSteps.size()));

    double tRange[2];
    tRange[0] = 0.;
    tRange[1] = static_cast<double>(this->NumberOfTimeSteps - 1);
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(), tRange, 2);
  }
  else
  {
    outInfo->Remove(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
    outInfo->Remove(vtkStreamingDemandDrivenPipeline::TIME_RANGE());
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

  this->DestroyData();
  if (!this->ReadAndOutputGrid())
  {
    this->DestroyData();
    return 0;
  }

  // Collect the time step requested
  this->DTime = 0;
  if (outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP()))
  {
    this->DTime =
        outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());
  }
  output->GetInformation()->Set(vtkDataObject::DATA_TIME_STEP(), this->DTime);

  // Examine each variable to see if it is selected
  int numPointVars = static_cast<int>(this->Internals->pointVars.size());
  for (int var = 0; var < numPointVars; var++)
  {
    // Is this variable requested
    if (this->PointDataArraySelection->GetArraySetting(var))
    {
      vtkDataArray *array = this->LoadPointVarData(var);
      if (!array)
      {
        vtkWarningMacro(<< "Error loading point variable '"
                        << this->Internals->pointVars[var]->name() << "'.");
        continue;
      }
      output->GetPointData()->AddArray(array);
    }
  }

  int numCellVars = static_cast<int>(this->Internals->cellVars.size());
  for (int var = 0; var < numCellVars; var++)
  {
    if (this->CellDataArraySelection->GetArraySetting(var))
    {
      vtkDataArray *array = this->LoadCellVarData(var);
      if (!array)
      {
        vtkWarningMacro(<< "Error loading point variable '"
                        << this->Internals->pointVars[var]->name() << "'.");
        continue;
      }
      output->GetCellData()->AddArray(array);
    }
  }

  this->LoadTimeFieldData(output);

  vtkDebugMacro( << "Returning from RequestData" << endl);
  return 1;
}

//----------------------------------------------------------------------------
// Set defaults for various parameters and initialize some variables
//----------------------------------------------------------------------------

void vtkMPASReader::SetDefaults() {

  // put in defaults
  this->VerticalDimension = "nVertLevels";
  this->VerticalLevelRange[0] = 0;
  this->VerticalLevelRange[1] = 1;

  this->LayerThicknessRange[0] = 0;
  this->LayerThicknessRange[1] = 200000;
  this->LayerThickness = 10000;
  vtkDebugMacro
    ( << "SetDefaults: LayerThickness set to " << LayerThickness << endl);

  this->CenterLonRange[0] = 0;
  this->CenterLonRange[1] = 360;
  this->CenterLon = 180;

  this->Geometry = Spherical;

  this->IsAtmosphere = false;
  this->ProjectLatLon = false;
  this->OnASphere = false;
  this->ShowMultilayerView = false;
  this->IsZeroCentered = false;

  this->IncludeTopography = false;
  this->DoBugFix = false;
  this->CenterRad = CenterLon * vtkMath::Pi() / 180.0;

  this->UseDimensionedArrayNames = false;

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

  if (isNcDim(pnf, this->VerticalDimension.c_str()))
  {
    NcDim* nVertLevels = pnf->get_dim(this->VerticalDimension.c_str());
    this->MaximumNVertLevels = nVertLevels->size();
  }
  else
  {
    this->MaximumNVertLevels = 0;
  }

  return 1;
}

//----------------------------------------------------------------------------
int vtkMPASReader::GetNcAtts()
{
  NcFile *pnf = this->Internals->ncFile;

  if (!isNcAtt(pnf, "on_a_sphere"))
  {
    vtkWarningMacro("Attribute 'on_a_sphere' missing in file " << this->FileName
                    << ". Assuming \"YES\".");
    this->OnASphere = true;
  }
  else
  {
    NcAtt *onASphere = pnf->get_att("on_a_sphere");
    char *onASphereString = onASphere->as_string(0);
    this->OnASphere = (strcmp(onASphereString, "YES") == 0);
    delete [] onASphereString;
    onASphereString = NULL;
  }

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

  this->VerticalLevelRange[0] = 0;
  this->VerticalLevelRange[1] = this->MaximumNVertLevels-1;

  if (this->OnASphere)
  {
    if (this->ProjectLatLon)
    {
      this->Geometry = Projected;
    }
    else
    {
      this->Geometry = Spherical;
    }
  }
  else
  {
    this->Geometry = Planar;
    if (this->ProjectLatLon)
    {
      vtkWarningMacro("Ignoring ProjectLatLong -- Data is not on_a_sphere.");
    }
  }

  return(1);
}

//----------------------------------------------------------------------------
// Get the NetCDF variables on cell or vertex
//----------------------------------------------------------------------------

int vtkMPASReader::GetNcVars (const char* cellDimName, const char* pointDimName)
{
  this->Internals->pointArrays.clear();
  this->Internals->pointVars.clear();
  this->Internals->cellArrays.clear();
  this->Internals->cellVars.clear();

  NcFile* ncFile =  this->Internals->ncFile;

  int numVars = ncFile->num_vars();
  for (int i = 0; i < numVars; i++)
  {
    NcVar* var = ncFile->get_var(i);

    // Variables must have the following dimension specification:
    // [Time, ] (nCells | nVertices), [arbitraryDim1, [arbitraryDim2, [...]]]

    bool isPointData = false;
    bool isCellData = false;
    int numDims = var->num_dims();
    std::vector<std::string> dimNames;
    for (int dim = 0; dim < std::min(numDims, 2); ++dim)
    {
      dimNames.push_back(var->get_dim(dim)->name());
    }

    if (numDims < 1)
    {
      vtkWarningMacro(<<"Variable '" << var->name()
                      << "' has invalid number of dimensions: " << numDims);
      continue;
    }

    if (dimNames[0] == "Time" && dimNames.size() >= 2)
    {
      if (dimNames[1] == pointDimName)
      {
        isPointData = true;
      }
      else if (dimNames[1] == cellDimName)
      {
        isCellData = true;
      }
    }
    else if (dimNames[0] == pointDimName)
    {
      isPointData = true;
    }
    else if (dimNames[0] == cellDimName)
    {
      isCellData = true;
    }

    // Add to cell or point var array
    if (isCellData)
    {
      this->Internals->cellVars.push_back(var);
    }
    else if (isPointData)
    {
      this->Internals->pointVars.push_back(var);
    }
  }

  return(1);
}


//----------------------------------------------------------------------------
// Build the selection Arrays for points and cells in the GUI.
//----------------------------------------------------------------------------

int vtkMPASReader::BuildVarArrays()
{
  // figure out what variables to visualize -
  if (!GetNcVars("nVertices", "nCells"))
  {
    return 0;
  }

  for (size_t v = 0; v < this->Internals->pointVars.size(); v++)
  {
    NcVar *var = this->Internals->pointVars[v];
    string name = this->UseDimensionedArrayNames ? dimensionedArrayName(var)
                                                 : var->name();
    this->PointDataArraySelection->EnableArray(name.c_str());
    // Register the dimensions:
    for (int d = 0; d < var->num_dims(); ++d)
    {
      this->InitializeDimension(var->get_dim(d));
    }
    vtkDebugMacro(<<"Adding point var: " << name);
  }

  for (size_t v = 0; v < this->Internals->cellVars.size(); v++)
  {
    NcVar *var = this->Internals->cellVars[v];
    string name = this->UseDimensionedArrayNames ? dimensionedArrayName(var)
                                                 : var->name();
    this->CellDataArraySelection->EnableArray(name.c_str());
    // Register the dimensions:
    for (int d = 0; d < var->num_dims(); ++d)
    {
      this->InitializeDimension(var->get_dim(d));
    }
    vtkDebugMacro(<<"Adding cell var: " << name);
  }

  return 1;
}


//----------------------------------------------------------------------------
//  Read the data from the ncfile, allocate the geometry and create the
//  vtk data structures for points and cells.
//----------------------------------------------------------------------------

int vtkMPASReader::ReadAndOutputGrid()
{
  switch (this->Geometry)
  {
    case vtkMPASReader::Spherical:
      if (!this->AllocSphericalGeometry())
      {
        return 0;
      }
      this->FixPoints();
      break;

    case vtkMPASReader::Projected:
      if (!this->AllocProjectedGeometry())
      {
        return 0;
      }
      this->ShiftLonData();
      this->FixPoints();
      if (!this->EliminateXWrap())
      {
        return 0;
      }
      break;

    case vtkMPASReader::Planar:
      if (!this->AllocPlanarGeometry())
      {
        return 0;
      }
      this->FixPoints();
      if (!this->EliminateXWrap())
      {
        return 0;
      }
      break;

    default:
      vtkErrorMacro("Invalid geometry type (" << this->Geometry << ").");
      return 0;
  }

  this->OutputPoints();
  this->OutputCells();

  return 1;
}

//----------------------------------------------------------------------------
// Allocate into sphere view of dual geometry
//----------------------------------------------------------------------------

int vtkMPASReader::AllocSphericalGeometry()
{
  NcFile* ncFile = this->Internals->ncFile;

  CHECK_VAR(ncFile, "xCell");
  this->PointX = (double*)malloc((this->NumberOfPoints+this->PointOffset) *
                                 sizeof(double));
  CHECK_MALLOC(this->PointX);
  NcVar*  xCellVar = ncFile->get_var("xCell");
  if (!this->ValidateDimensions(xCellVar, false, 1, "nCells"))
  {
    return 0;
  }
  xCellVar->get(this->PointX + this->PointOffset, this->NumberOfPoints);
  // point 0 is 0.0
  this->PointX[0] = 0.0;

  CHECK_VAR(ncFile, "yCell");
  this->PointY = (double*)malloc((this->NumberOfPoints+this->PointOffset) *
                                 sizeof(double));
  CHECK_MALLOC(this->PointY);
  NcVar*  yCellVar = ncFile->get_var("yCell");
  if (!this->ValidateDimensions(yCellVar, false, 1, "nCells"))
  {
    return 0;
  }
  yCellVar->get(this->PointY + this->PointOffset, this->NumberOfPoints);
  // point 0 is 0.0
  this->PointY[0] = 0.0;

  CHECK_VAR(ncFile, "zCell");
  this->PointZ = (double*)malloc((this->NumberOfPoints+this->PointOffset) *
                                 sizeof(double));
  CHECK_MALLOC(this->PointZ);
  NcVar*  zCellVar = ncFile->get_var("zCell");
  if (!this->ValidateDimensions(zCellVar, false, 1, "nCells"))
  {
    return 0;
  }
  zCellVar->get(this->PointZ + this->PointOffset, this->NumberOfPoints);
  // point 0 is 0.0
  this->PointZ[0] = 0.0;

  CHECK_VAR(ncFile, "cellsOnVertex");
  this->OrigConnections = (int *) malloc(this->NumberOfCells *
                                         this->PointsPerCell * sizeof(int));
  CHECK_MALLOC(this->OrigConnections);
  NcVar *connectionsVar = ncFile->get_var("cellsOnVertex");
  // TODO Spec says dims should be '3', 'nVertices', but my example files
  // use nVertices, vertexDegree...
  if (!this->ValidateDimensions(connectionsVar, false, 2,
                                "nVertices", "vertexDegree"))
  {
    return 0;
  }
  connectionsVar->get(this->OrigConnections, this->NumberOfCells,
                      this->PointsPerCell);

  if (isNcVar(ncFile, "maxLevelCell"))
  {
    this->IncludeTopography = true;
    this->MaximumLevelPoint = (int*)malloc((this->NumberOfPoints +
                                            this->PointOffset) * sizeof(int));
    CHECK_MALLOC(this->MaximumLevelPoint);
    NcVar *maxLevelPointVar = ncFile->get_var("maxLevelCell");
    if (!this->ValidateDimensions(maxLevelPointVar, false, 1, "nCells"))
    {
      return 0;
    }
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
  }

  return 1;
}


//----------------------------------------------------------------------------
// Allocate the lat/lon projection of dual geometry.
//----------------------------------------------------------------------------

int vtkMPASReader::AllocProjectedGeometry()
{
  NcFile* ncFile = this->Internals->ncFile;
  const float BLOATFACTOR = .5;
  this->ModNumPoints = (int)floor(this->NumberOfPoints*(1.0 + BLOATFACTOR));
  this->ModNumCells = (int)floor(this->NumberOfCells*(1.0 + BLOATFACTOR))+1;

  CHECK_VAR(ncFile, "lonCell");
  this->PointX = (double*)malloc(this->ModNumPoints * sizeof(double));
  CHECK_MALLOC(this->PointX);
  NcVar*  xCellVar = ncFile->get_var("lonCell");
  if (!this->ValidateDimensions(xCellVar, false, 1, "nCells"))
  {
    return 0;
  }
  xCellVar->get(this->PointX + this->PointOffset, this->NumberOfPoints);
  // point 0 is 0.0
  this->PointX[0] = 0.0;

  CHECK_VAR(ncFile, "latCell");
  this->PointY = (double*)malloc(this->ModNumPoints * sizeof(double));
  CHECK_MALLOC(this->PointY);
  NcVar*  yCellVar = ncFile->get_var("latCell");
  if (!this->ValidateDimensions(yCellVar, false, 1, "nCells"))
  {
    return 0;
  }
  yCellVar->get(this->PointY+this->PointOffset, this->NumberOfPoints);
  // point 0 is 0.0
  this->PointY[0] = 0.0;

  CHECK_VAR(ncFile, "cellsOnVertex");
  this->OrigConnections = (int *) malloc(this->NumberOfCells * this->PointsPerCell *
                                         sizeof(int));
  CHECK_MALLOC(this->OrigConnections);
  NcVar *connectionsVar = ncFile->get_var("cellsOnVertex");
  // TODO Spec says dims should be '3', 'nVertices', but my example files
  // use nVertices, vertexDegree...
  if (!this->ValidateDimensions(connectionsVar, false, 2,
                                "nVertices", "vertexDegree"))
  {
    return 0;
  }
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
    if (!this->ValidateDimensions(maxLevelPointVar, false, 1, "nCells"))
    {
      return 0;
    }
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
  }

  return 1;
}

int vtkMPASReader::AllocPlanarGeometry()
{
  NcFile* ncFile = this->Internals->ncFile;

  const float BLOATFACTOR = .5f;
  this->ModNumPoints = (int)floor(this->NumberOfPoints*(1.0 + BLOATFACTOR));
  this->ModNumCells = (int)floor(this->NumberOfCells*(1.0 + BLOATFACTOR))+1;

  CHECK_VAR(ncFile, "xCell");
  this->PointX = (double*)malloc(this->ModNumPoints * sizeof(double));
  CHECK_MALLOC(this->PointX);
  NcVar*  xCellVar = ncFile->get_var("xCell");
  if (!this->ValidateDimensions(xCellVar, false, 1, "nCells"))
  {
    return 0;
  }
  xCellVar->get(this->PointX + this->PointOffset, this->NumberOfPoints);
  // point 0 is 0.0
  this->PointX[0] = 0.0;

  CHECK_VAR(ncFile, "yCell");
  this->PointY = (double*)malloc(this->ModNumPoints * sizeof(double));
  CHECK_MALLOC(this->PointY);
  NcVar*  yCellVar = ncFile->get_var("yCell");
  if (!this->ValidateDimensions(yCellVar, false, 1, "nCells"))
  {
    return 0;
  }
  yCellVar->get(this->PointY + this->PointOffset, this->NumberOfPoints);
  // point 0 is 0.0
  this->PointY[0] = 0.0;

  CHECK_VAR(ncFile, "zCell");
  this->PointZ = (double*)malloc(this->ModNumPoints * sizeof(double));
  CHECK_MALLOC(this->PointZ);
  NcVar *zCellVar = ncFile->get_var("zCell");
  if (!this->ValidateDimensions(zCellVar, false, 1, "nCells"))
  {
    return 0;
  }
  zCellVar->get(this->PointZ + this->PointOffset, this->NumberOfPoints);
  // point 0 is 0.0
  this->PointZ[0] = 0.0;

  CHECK_VAR(ncFile, "cellsOnVertex");
  this->OrigConnections = (int *) malloc(this->NumberOfCells * this->PointsPerCell *
                                         sizeof(int));
  CHECK_MALLOC(this->OrigConnections);
  NcVar *connectionsVar = ncFile->get_var("cellsOnVertex");
  // TODO Spec says dims should be '3', 'nVertices', but my example files
  // use nVertices, vertexDegree...
  if (!this->ValidateDimensions(connectionsVar, false, 2,
                                "nVertices", "vertexDegree"))
  {
    return 0;
  }
  connectionsVar->get(this->OrigConnections, this->NumberOfCells,
                      this->PointsPerCell);

  // create list to include modified origConnections (due to eliminating
  // wraparound) plus additional cells added when mirroring cells that had
  // previously wrapped around

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
    this->MaximumLevelPoint = (int*)malloc(2 * this->NumberOfPoints *
                                           sizeof(int));
    CHECK_MALLOC(this->MaximumLevelPoint);
    NcVar *maxLevelPointVar = ncFile->get_var("maxLevelCell");
    if (!this->ValidateDimensions(maxLevelPointVar, false, 1, "nCells"))
    {
      return 0;
    }
    maxLevelPointVar->get(this->MaximumLevelPoint + this->PointOffset,
                          this->NumberOfPoints);
  }

  this->CurrentExtraPoint = this->NumberOfPoints + this->PointOffset;
  this->CurrentExtraCell = this->NumberOfCells + this->CellOffset;

  if (this->ShowMultilayerView)
  {
    this->MaximumCells = this->CurrentExtraCell * this->MaximumNVertLevels;
    this->MaximumPoints = this->CurrentExtraPoint *
        (this->MaximumNVertLevels + 1);
  }
  else
  {
    this->MaximumCells = this->CurrentExtraCell;
    this->MaximumPoints = this->CurrentExtraPoint;
  }

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

int vtkMPASReader::AddMirrorPoint(int index, double dividerX, double offset)
{
  double X = this->PointX[index];
  double Y = this->PointY[index];

  // add on east
  if (X < dividerX)
  {
    X += offset;
  }
  else
  {
    // add on west
    X -= offset;
  }

  assert(this->CurrentExtraPoint < this->ModNumPoints);
  this->PointX[this->CurrentExtraPoint] = X;
  this->PointY[this->CurrentExtraPoint] = Y;

  int mirrorPoint = this->CurrentExtraPoint;

  // record mapping
  *(this->PointMap + (this->CurrentExtraPoint - this->NumberOfPoints - this->PointOffset)) = index;
  this->CurrentExtraPoint++;

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

int vtkMPASReader::EliminateXWrap()
{
  if (this->NumberOfPoints == 0)
  {
    return 1;
  }

  double xLength;
  double xCenter;
  switch (this->Geometry)
  {
    case vtkMPASReader::Spherical:
      vtkErrorMacro("EliminateXWrap called for spherical geometry.");
      return 0;

    case vtkMPASReader::Projected:
      xLength = 2 * vtkMath::Pi();
      xCenter = this->CenterRad;
      break;

    case vtkMPASReader::Planar:
    {
      // Determine the bounds in the x-dimension
      double xRange[2] = { this->PointX[this->PointOffset],
                           this->PointX[this->PointOffset] };
      for (int i = 1; i < this->NumberOfPoints; ++i)
      {
        double x = this->PointX[this->PointOffset + i];
        xRange[0] = std::min(xRange[0], x);
        xRange[1] = std::max(xRange[1], x);
      }

      xLength = xRange[1] - xRange[0];
      xCenter = (xRange[0] + xRange[1]) * 0.5;
    }
      break;

    default:
      vtkErrorMacro("Unrecognized geometry type (" << this->Geometry << ").");
      return 0;
  }

  // Assume a point is wrapped if it is more than 3/4 across the dataset:
  double tolerance = xLength * 0.75;

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
      if (abs(this->PointX[conns[k]] - this->PointX[conns[lastk]]) > tolerance)
      {
        xWrap = true;
        break;
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
        if (abs(this->PointX[neigh] - anchorX) > tolerance)
        {
          modConns[k] = this->AddMirrorPoint(neigh, anchorX, xLength);
        }
        else
        {
          // use existing kth point
          modConns[k] = neigh;
        }
      }

      // move addedConns to this->ModConnections extra cells area
      int* addedConns = this->ModConnections
        + (this->CurrentExtraCell * this->PointsPerCell);

      // add a mirroring cell to other side

      // add mirrored anchor first
      addedConns[0] = this->AddMirrorPoint(conns[0], xCenter, xLength);
      anchorX = this->PointX[addedConns[0]];

      // add mirror cell points if needed
      for (int k = 1; k < this->PointsPerCell; k++)
      {
        int neigh = conns[k];

        // add a new point for neighbor, figure out east or west
        if (abs(this->PointX[neigh] - anchorX) > tolerance)
        {
          addedConns[k] = this->AddMirrorPoint(neigh, anchorX, xLength);
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

  return 1;
}


//----------------------------------------------------------------------------
//  Add points to vtk data structures
//----------------------------------------------------------------------------

void vtkMPASReader::OutputPoints()
{
  vtkUnstructuredGrid* output = this->GetOutput();

  double adjustedLayerThickness = this->IsAtmosphere
      ? static_cast<double>(-this->LayerThickness)
      : static_cast<double>(this->LayerThickness);


  vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
  points->Allocate(this->MaximumPoints);
  output->SetPoints(points);

  for (int j = 0; j < this->CurrentExtraPoint; j++ )
  {
    double x, y, z;

    switch (this->Geometry)
    {
      case vtkMPASReader::Planar:
      case vtkMPASReader::Spherical:
        x = this->PointX[j];
        y = this->PointY[j];
        z = this->PointZ[j];
        break;

      case vtkMPASReader::Projected:
        x = this->PointX[j] * 180.0 / vtkMath::Pi();
        y = this->PointY[j] * 180.0 / vtkMath::Pi();
        z = 0.0;
        break;

      default:
        vtkErrorMacro("Unrecognized geometry type (" << this->Geometry << ").");
        return;
    }

    if (!this->ShowMultilayerView)
    {
      points->InsertNextPoint(x, y, z);
    }
    else
    {
      double rho=0.0, rholevel=0.0, theta=0.0, phi=0.0;
      int retval = -1;

      if (this->Geometry == Spherical)
      {
        if ((x != 0.0) || (y != 0.0) || (z != 0.0))
        {
          retval = CartesianToSpherical(x, y, z, &rho, &phi, &theta);
          if (retval)
          {
            vtkWarningMacro("Can't create point for layered view.");
          }
        }
      }

      for (int levelNum = 0; levelNum < this->MaximumNVertLevels+1; levelNum++)
      {
        if (this->Geometry == Spherical)
        {
          if (!retval && ((x != 0.0) || (y != 0.0) || (z != 0.0)))
          {
            rholevel = rho - (adjustedLayerThickness * levelNum);
            retval = SphericalToCartesian(rholevel, phi, theta, &x, &y, &z);
            if (retval)
            {
              vtkWarningMacro("Can't create point for layered view.");
            }
          }
        }
        else
        {
          z = -levelNum * adjustedLayerThickness;
        }
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

//------------------------------------------------------------------------------
bool vtkMPASReader::ValidateDimensions(NcVar *var, bool silent, int ndims, ...)
{
  if (var->num_dims() != ndims)
  {
    if (!silent)
    {
      vtkWarningMacro(<< "Expected variable '" << var->name() << "' to have "
                      << ndims << "dimension(s), but it has " << var->num_dims()
                      << ".");
    }
    return false;
  }

  va_list args;
  va_start(args, ndims);

  for (int i = 0; i < ndims; ++i)
  {
    NcDim *dim = var->get_dim(i);
    std::string dimName(va_arg(args, const char *));
    if (dimName != dim->name())
    {
      if (!silent)
      {
        vtkWarningMacro(<< "Expected variable '" << var->name() << "' to have '"
                        << dimName << "' at dimension index " << i << ", not '"
                        << dim->name() << "'.");
      }
      va_end(args);
      return false;
    }
  }

  va_end(args);

  return true;
}

//------------------------------------------------------------------------------
long vtkMPASReader::GetCursorForDimension(const NcDim *dim)
{
  std::string dimName = dim->name();
  if (dimName == "nCells" || dimName == "nVertices")
  {
    return 0;
  }
  else if (dimName == "Time")
  {
    return std::min(static_cast<long>(std::floor(this->DTime)),
                    static_cast<long>(this->NumberOfTimeSteps-1));
  }
  else if (this->ShowMultilayerView &&
           dimName == this->VerticalDimension)
  {
    return 0;
  }
  else
  {
    return this->InitializeDimension(dim);
  }
}

//------------------------------------------------------------------------------
size_t vtkMPASReader::GetCountForDimension(const NcDim *dim)
{
  std::string dimName = dim->name();
  if (dimName == "nCells")
  {
    return this->NumberOfPoints;
  }
  else if (dimName == "nVertices")
  {
    return this->NumberOfCells;
  }
  else if (this->ShowMultilayerView && dimName == this->VerticalDimension)
  {
    return this->MaximumNVertLevels;
  }
  else
  {
    return 1;
  }
}

//------------------------------------------------------------------------------
long vtkMPASReader::InitializeDimension(const NcDim *dim)
{
  Internal::DimMetaDataMap::const_iterator match =
      this->Internals->dimMetaDataMap.find(dim->name());

  long result = 0;
  if (match == this->Internals->dimMetaDataMap.end())
  {
    DimMetaData metaData;
    metaData.curIdx = result;
    metaData.dimSize = dim->size();

    this->Internals->dimMetaDataMap.insert(
          std::make_pair(std::string(dim->name()), metaData));
    this->Internals->dimMetaDataTime.Modified();
  }
  else
  {
    result = match->second.curIdx;
  }

  return result;
}

//----------------------------------------------------------------------------
//  Add cells to vtk data structures
//----------------------------------------------------------------------------

void vtkMPASReader::OutputCells()
{
  vtkDebugMacro(<< "In OutputCells..." << endl);
  vtkUnstructuredGrid* output = GetOutput();

  output->Allocate(this->MaximumCells, this->MaximumCells);

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
    (<< "OutputCells: this->MaximumCells: " << this->MaximumCells
     << " cellType: " << cellType << " this->MaximumNVertLevels: " << this->MaximumNVertLevels
     << " LayerThickness: " << LayerThickness << " ProjectLatLon: "
     << ProjectLatLon << " ShowMultilayerView: " << ShowMultilayerView);

  std::vector<vtkIdType> polygon(pointsPerPolygon);

  for (int j = 0; j < this->CurrentExtraCell ; j++)
  {

    int* conns;
    if (this->Geometry == Spherical)
    {
      conns = this->OrigConnections + (j * this->PointsPerCell);
    }
    else
    {
      conns = this->ModConnections + (j * this->PointsPerCell);
    }

    int minLevel= 0;

    if (this->IncludeTopography)
    {
      int* connections;

      //check if it is a mirror cell, if so, get original
      if (j >= this->NumberOfCells + this->CellOffset)
      {
        int origCellNum = *(this->CellMap + (j - this->NumberOfCells - this->CellOffset));
        connections = this->OrigConnections + (origCellNum*this->PointsPerCell);
      }
      else
      {
        connections = this->OrigConnections + (j * this->PointsPerCell);
      }

      minLevel = this->MaximumLevelPoint[connections[0]];

      // Take the min of the this->MaximumLevelPoint of each point
      for (int k = 1; k < this->PointsPerCell; k++)
      {
        minLevel = min(minLevel, this->MaximumLevelPoint[connections[k]]);
      }
    }

    // singlelayer
    if (!this->ShowMultilayerView)
    {
      // If that min is greater than or equal to this output level,
      // include the cell, otherwise set all points to zero.

      if (this->IncludeTopography && ((minLevel-1) < this->GetVerticalLevel()))
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

  free(this->ModConnections); this->ModConnections = NULL;
  free(this->OrigConnections); this->OrigConnections = NULL;

  vtkDebugMacro(<< "Leaving OutputCells..." << endl);
}

//------------------------------------------------------------------------------
vtkDataArray *vtkMPASReader::CreateDataArray(int typeNc)
{
  int typeVtk = this->NcTypeToVtkType(static_cast<NcType>(typeNc));
  return vtkDataArray::CreateDataArray(typeVtk);
}

//------------------------------------------------------------------------------
vtkIdType vtkMPASReader::ComputeNumberOfTuples(NcVar *ncVar)
{
  int numDims = ncVar->num_dims();
  vtkIdType size = 0;
  for (int dim = 0; dim < numDims; ++dim)
  {
    vtkIdType count = static_cast<vtkIdType>(
          this->GetCountForDimension(ncVar->get_dim(dim)));
    if (size == 0)
    {
      size = count;
    }
    else
    {
      size *= count;
    }
  }
  return size;
}

//------------------------------------------------------------------------------
template <typename ValueType>
bool vtkMPASReader::LoadDataArray(NcVar *ncVar, vtkDataArray *array,
                                  bool resize)
{
  if (array->GetDataType() != this->NcTypeToVtkType(ncVar->type()))
  {
    vtkWarningMacro("Invalid array type.");
    return false;
  }

  int numDims = ncVar->num_dims();
  std::vector<long> cursor;
  std::vector<nc_size_t> counts;
  vtkIdType size = 0;

  for (int dim = 0; dim < numDims; ++dim)
  {
    cursor.push_back(this->GetCursorForDimension(ncVar->get_dim(dim)));
    counts.push_back(static_cast<nc_size_t>(
                       this->GetCountForDimension(ncVar->get_dim(dim))));
    if (size == 0)
    {
      size = counts.back();
    }
    else
    {
      size *= counts.back();
    }
  }

  if (resize)
  {
    array->SetNumberOfComponents(1);
    array->SetNumberOfTuples(size);
  }
  else
  {
    if (array->GetNumberOfComponents() != 1)
    {
      vtkWarningMacro("Invalid number of components: "
                      << array->GetNumberOfComponents() << ".");
      return false;
    }
    else if (array->GetNumberOfTuples() < size)
    {
      vtkWarningMacro("Array only has " << array->GetNumberOfTuples()
                      << " allocated, but we need " << size << ".");
      return false;
    }
  }

  ValueType *dataBlock = static_cast<ValueType*>(array->GetVoidPointer(0));
  if (!dataBlock)
  {
    vtkWarningMacro("GetVoidPointer returned NULL.");
    return false;
  }

  if (!ncVar->set_cur(&cursor[0]))
  {
    vtkWarningMacro("Setting cursor failed.");
    return false;
  }

  if (!ncVar->get(dataBlock, &counts[0]))
  {
    vtkWarningMacro("Reading " << size << " elements failed.");
    return false;
  }

  return true;
}

//------------------------------------------------------------------------------
template <typename ValueType>
int vtkMPASReader::LoadPointVarDataImpl(NcVar *ncVar, vtkDataArray *array)
{
  // Don't resize, we've preallocated extra room for multilayer (if needed):
  if (!this->LoadDataArray<ValueType>(ncVar, array, /*resize=*/false))
  {
    return 0;
  }

  // Check if this variable contains the vertical dimension:
  bool hasVerticalDimension = false;
  int numDims = ncVar->num_dims();
  if (this->ShowMultilayerView)
  {
    for (int d = 0; d < numDims; ++d)
    {
      if (this->VerticalDimension == ncVar->get_dim(d)->name())
      {
        hasVerticalDimension = true;
        break;
      }
    }
  }

  vtkIdType varSize = this->ComputeNumberOfTuples(ncVar);
  ValueType *dataBlock = static_cast<ValueType*>(array->GetVoidPointer(0));
  std::vector<ValueType> tempData; // Used for Multilayer

  // singlelayer
  if (!this->ShowMultilayerView)
  {
    // Account for point offset:
    if (this->PointOffset != 0)
    {
      assert(this->NumberOfPoints <= array->GetNumberOfTuples() &&
             "Source array too small.");
      assert(this->PointOffset + this->NumberOfPoints <=
             array->GetNumberOfTuples() && "Destination array too small.");
      if (this->PointOffset < this->NumberOfPoints)
      {
        std::copy_backward(dataBlock, dataBlock + this->NumberOfPoints,
                           dataBlock + this->PointOffset +
                           this->NumberOfPoints);
      }
      else
      {
        std::copy(dataBlock, dataBlock + this->NumberOfPoints,
                  dataBlock + this->PointOffset);
      }
    }
    dataBlock[0] = dataBlock[1];
    // data is all in place, don't need to do next step
  }
  else
  { // multilayer
    if (this->MaximumPoints == 0)
    {
      return 0; // No points
    }

    tempData.resize(this->MaximumPoints);
    size_t vertPointOffset = this->MaximumNVertLevels * this->PointOffset;
    ValueType *dataPtr = &tempData[0] + vertPointOffset;

    assert(varSize < array->GetNumberOfTuples());
    assert(varSize < static_cast<vtkIdType>(this->MaximumPoints -
                                            vertPointOffset));
    std::copy(dataBlock, dataBlock + varSize, dataPtr);

    if (!hasVerticalDimension)
    {
      // need to replicate data over all vertical layers
      // layout in memory needs to be:
      // pt1, pt1, ..., (VertLevels times), pt2, pt2, ..., (VertLevels times),
      // need to go backwards through the points in order to not overwrite
      // anything.
      for(int i = this->NumberOfPoints; i > 0; i--)
      {
        // point to copy
        ValueType pt = *(dataPtr + i - 1);

        // where to start copying
        ValueType *copyPtr = dataPtr + (i-1)*this->MaximumNVertLevels;

        std::fill(copyPtr, copyPtr + this->MaximumNVertLevels, pt);
      }
    }
  }

  vtkDebugMacro(<<"Got point data.");

  int i = 0;
  int k = 0;

  if (this->ShowMultilayerView)
  {
    // put in dummy points
    assert(this->MaximumNVertLevels * 2 <= this->MaximumPoints);
    assert(this->MaximumNVertLevels <= array->GetNumberOfTuples());
    std::copy(tempData.begin() + this->MaximumNVertLevels,
              tempData.begin() + (2*this->MaximumNVertLevels),
              dataBlock);

    // write highest level dummy point (duplicate of last level)
    assert(this->MaximumNVertLevels < array->GetNumberOfTuples());
    assert(2*this->MaximumNVertLevels - 1 < this->MaximumPoints);
    dataBlock[this->MaximumNVertLevels] =
        tempData[2*this->MaximumNVertLevels - 1];

    vtkDebugMacro(<<"Wrote dummy point data.");

    // put in other points
    for (int j = this->PointOffset;
         j < this->NumberOfPoints + this->PointOffset;
         j++)
    {

      i = j*(this->MaximumNVertLevels+1);
      k = j*(this->MaximumNVertLevels);

      // write data for one point -- lowest level to highest
      assert(k + this->MaximumNVertLevels <= this->MaximumPoints);
      assert(i + this->MaximumNVertLevels <= array->GetNumberOfTuples());
      std::copy(tempData.begin() + k,
                tempData.begin() + k + this->MaximumNVertLevels,
                dataBlock + i);

      // for last layer of points, repeat last level's values
      // Need Mark's input on this one
      dataBlock[i++] = tempData[--k];
      //vtkDebugMacro (<< "Wrote j:" << j << endl);
    }
  }

  vtkDebugMacro(<<"Wrote next points.");

  vtkDebugMacro(<<"this->NumberOfPoints: " << this->NumberOfPoints << " "
                <<"this->CurrentExtraPoint: " << this->CurrentExtraPoint);

  // put out data for extra points
  for (int j = this->PointOffset + this->NumberOfPoints;
       j < this->CurrentExtraPoint;
       j++)
  {
    // use map to find out what point data we are using
    if (!this->ShowMultilayerView)
    {
      k = this->PointMap[j - this->NumberOfPoints - this->PointOffset];
      assert(j < array->GetNumberOfTuples());
      assert(k < array->GetNumberOfTuples());
      dataBlock[j] = dataBlock[k];
    }
    else
    {
      k = this->PointMap[j - this->NumberOfPoints - this->PointOffset] *
          this->MaximumNVertLevels;
      // write data for one point -- lowest level to highest
      assert(k + this->MaximumNVertLevels <= this->MaximumPoints);
      assert(i + this->MaximumNVertLevels <= array->GetNumberOfTuples());
      std::copy(tempData.begin() + k,
                tempData.begin() + k + this->MaximumNVertLevels,
                dataBlock + i);

      // for last layer of points, repeat last level's values
      // Need Mark's input on this one
      dataBlock[i++] = tempData[--k];
    }
  }

  vtkDebugMacro(<<"wrote extra point data.");
  return 1;
}

//----------------------------------------------------------------------------
//  Load the data for a point variable
//----------------------------------------------------------------------------
vtkDataArray *vtkMPASReader::LoadPointVarData(int variableIndex)
{
  NcVar* ncVar = this->Internals->pointVars[variableIndex];
  if (ncVar == NULL)
  {
    vtkErrorMacro(<<"No NetCDF data for pointVar @ index " << variableIndex);
    return 0;
  }

  vtkDebugMacro(<<"Loading point data array named: " << ncVar->name());

  // Get data type:
  NcType typeNc = ncVar->type();
  int typeVtk = this->NcTypeToVtkType(typeNc);

  // Allocate data array pointer for this variable:
  vtkSmartPointer<vtkDataArray> array =
      this->LookupPointDataArray(variableIndex);
  if (array == NULL)
  {
    vtkDebugMacro(<<"Allocating data array.");
    array = vtkSmartPointer<vtkDataArray>::Take(
          vtkDataArray::CreateDataArray(typeVtk));
  }
  array->SetName(ncVar->name());
  array->SetNumberOfComponents(1);
  array->SetNumberOfTuples(this->MaximumPoints);

  int success = false;
  vtkNcDispatch(typeVtk,
                success = this->LoadPointVarDataImpl<VTK_TT>(ncVar, array););

  if (success)
  {
    this->Internals->pointArrays[variableIndex] = array;
    return array;
  }
  return NULL;
}

//------------------------------------------------------------------------------
template <typename ValueType>
int vtkMPASReader::LoadCellVarDataImpl(NcVar *ncVar, vtkDataArray *array)
{
  // Don't resize, we've preallocated extra room for multilayer (if needed):
  if (!this->LoadDataArray<ValueType>(ncVar, array, /*resize=*/false))
  {
    return 0;
  }

  ValueType *dataBlock = static_cast<ValueType*>(array->GetVoidPointer(0));

  // put out data for extra cells
  for (int j = this->CellOffset + this->NumberOfCells;
       j < this->CurrentExtraCell;
       j++)
  {
    // use map to find out what cell data we are using
    if (!this->ShowMultilayerView)
    {
      int k = this->CellMap[j - this->NumberOfCells - this->CellOffset];
      assert(j < array->GetNumberOfTuples());
      assert(k < array->GetNumberOfTuples());
      dataBlock[j] = dataBlock[k];
    }
    else
    {
      int i = j*this->MaximumNVertLevels;
      int k = this->CellMap[j - this->NumberOfCells - this->CellOffset] *
              this->MaximumNVertLevels;

      // write data for one cell -- lowest level to highest
      assert(i < array->GetNumberOfTuples());
      assert(k + this->MaximumNVertLevels <= array->GetNumberOfTuples());
      std::copy(dataBlock + k, dataBlock + k + this->MaximumNVertLevels,
                dataBlock + i);
    }
  }

  vtkDebugMacro(<<"Stored data.");

  return 1;
}

//----------------------------------------------------------------------------
//  Load the data for a cell variable
//----------------------------------------------------------------------------

vtkDataArray* vtkMPASReader::LoadCellVarData(int variableIndex)
{
  NcVar* ncVar = this->Internals->cellVars[variableIndex];
  if (ncVar == NULL)
  {
    vtkErrorMacro(<<"No NetCDF data for cellVar @ index " << variableIndex);
    return 0;
  }

  vtkDebugMacro(<<"Loading cell data array named: " << ncVar->name());

  // Get data type:
  NcType typeNc = ncVar->type();
  int typeVtk = this->NcTypeToVtkType(typeNc);

  // Allocate data array pointer for this variable:
  vtkSmartPointer<vtkDataArray> array =
      this->LookupCellDataArray(variableIndex);
  if (array == NULL)
  {
    vtkDebugMacro(<<"Allocating data array.");
    array = vtkSmartPointer<vtkDataArray>::Take(
          vtkDataArray::CreateDataArray(typeVtk));
  }
  array->SetName(ncVar->name());
  array->SetNumberOfComponents(1);
  array->SetNumberOfTuples(this->MaximumCells);

  int success = false;
  vtkNcDispatch(typeVtk,
                success = this->LoadCellVarDataImpl<VTK_TT>(ncVar, array););
  if (success)
  {
    this->Internals->cellArrays[variableIndex] = array;
    return array;
  }
  return NULL;
}

//------------------------------------------------------------------------------
vtkDataArray *vtkMPASReader::LookupPointDataArray(int varIdx)
{
  Internal::ArrayMap::iterator it = this->Internals->pointArrays.find(varIdx);
  return it != this->Internals->pointArrays.end() ? it->second : NULL;
}

//------------------------------------------------------------------------------
vtkDataArray *vtkMPASReader::LookupCellDataArray(int varIdx)
{
  Internal::ArrayMap::iterator it = this->Internals->cellArrays.find(varIdx);
  return it != this->Internals->cellArrays.end() ? it->second : NULL;
}

//------------------------------------------------------------------------------
void vtkMPASReader::LoadTimeFieldData(vtkUnstructuredGrid *dataset)
{
  vtkStringArray *array = NULL;
  vtkFieldData *fd = dataset->GetFieldData();
  if (!fd)
  {
    fd = vtkFieldData::New();
    dataset->SetFieldData(fd);
    fd->Delete();
  }

  if (vtkDataArray *da = fd->GetArray("Time"))
  {
    if (!(array = vtkArrayDownCast<vtkStringArray>(da)))
    {
      vtkWarningMacro("Not creating \"Time\" field data array: a data array "
                      "with this name already exists.");
      return;
    }
  }

  if (!array)
  {
    array = vtkStringArray::New();
    array->SetName("Time");
    fd->AddArray(array);
    array->Delete();
  }

  // If the xtime variable exists, use its value at the current timestep:
  std::string time;
  if (isNcVar(this->Internals->ncFile, "xtime"))
  {
    NcVar *var = this->Internals->ncFile->get_var("xtime");
    if (var && this->ValidateDimensions(var, false, 2, "Time", "StrLen"))
    {
      NcDim *strLenDim = this->Internals->ncFile->get_dim("StrLen");
      assert(strLenDim);
      long strLen = strLenDim->size();
      if (strLen > 0)
      {
        time.resize(strLen);
        var->set_cur(this->GetCursorForDimension(var->get_dim(0)), 0);
        if (var->get(&time[0], 1, strLen))
        {
          // Trim off trailing whitespace:
          size_t realLength = time.find_last_not_of(" ");
          if (realLength != vtkStdString::npos)
          {
            time.resize(realLength + 1);
          }
        }
        else
        {
          vtkWarningMacro("Error reading xtime variable from file.");
          time.clear();
        }
      }
    }
  }

  // If no string time is available or the read fails, just insert the timestep:
  if (time.empty())
  {
    std::ostringstream timeStr;
    timeStr << "Timestep " << std::floor(this->DTime)
            << "/" << this->NumberOfTimeSteps;
    time = timeStr.str();
  }

  assert(array != NULL);
  array->SetNumberOfComponents(1);
  array->SetNumberOfTuples(1);
  array->SetValue(0, time);
}

//------------------------------------------------------------------------------
inline int vtkMPASReader::NcTypeToVtkType(int ncType)
{
  switch (static_cast<NcType>(ncType))
  {
    case ncByte:
      return VTK_SIGNED_CHAR;
    case ncChar:
      return VTK_CHAR;
    case ncShort:
      return VTK_SHORT;
    case ncInt:
      return VTK_INT;
    case ncFloat:
      return VTK_FLOAT;
    case ncDouble:
      return VTK_DOUBLE;
    case ncNoType:
    default: // Shouldn't happen...
      vtkGenericWarningMacro(<<"Invalid NcType: " << ncType);
      return VTK_VOID;
  }
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
void vtkMPASReader::UpdateDimensions(bool force)
{
  if (!force &&
      this->Internals->dimMetaDataTime < this->Internals->extraDimTime)
  {
    return;
  }

  this->Internals->extraDims->Reset();

  if (!this->Internals->ncFile)
  {
    this->Internals->extraDimTime.Modified();
    return;
  }

  std::set<std::string> dimSet;

  typedef Internal::DimMetaDataMap::const_iterator Iter;
  const Internal::DimMetaDataMap &map = this->Internals->dimMetaDataMap;
  for (Iter it = map.begin(), itEnd = map.end(); it != itEnd; ++it)
  {
    if (this->Internals->isExtraDim(it->first))
    {
      dimSet.insert(it->first);
    }
  }

  typedef std::set<std::string>::const_iterator SetIter;
  this->Internals->extraDims->Allocate(static_cast<vtkIdType>(dimSet.size()));
  for (SetIter it = dimSet.begin(), itEnd = dimSet.end(); it != itEnd; ++it)
  {
    this->Internals->extraDims->InsertNextValue(it->c_str());
  }

  this->Internals->extraDimTime.Modified();
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
  return this->PointDataArraySelection->GetArrayName(index);
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
  return this->CellDataArraySelection->GetArrayName(index);
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
int vtkMPASReader::GetNumberOfDimensions()
{
  this->UpdateDimensions();
  return this->Internals->extraDims->GetNumberOfTuples();
}

//----------------------------------------------------------------------------
std::string vtkMPASReader::GetDimensionName(int idx)
{
  this->UpdateDimensions();
  return this->Internals->extraDims->GetValue(idx);
}

//----------------------------------------------------------------------------
vtkStringArray *vtkMPASReader::GetAllDimensions()
{
  this->UpdateDimensions();
  return this->Internals->extraDims.GetPointer();
}

//----------------------------------------------------------------------------
int vtkMPASReader::GetDimensionCurrentIndex(const std::string &dim)
{
  this->UpdateDimensions();

  typedef Internal::DimMetaDataMap::const_iterator Iter;
  Iter it = this->Internals->dimMetaDataMap.find(dim);
  if (it == this->Internals->dimMetaDataMap.end())
  {
    return -1;
  }
  return it->second.curIdx;
}

//----------------------------------------------------------------------------
void vtkMPASReader::SetDimensionCurrentIndex(const string &dim, int idx)
{
  this->UpdateDimensions();

  typedef Internal::DimMetaDataMap::iterator Iter;
  Iter it = this->Internals->dimMetaDataMap.find(dim);
  if (it != this->Internals->dimMetaDataMap.end() &&
      idx < it->second.dimSize)
  {
    it->second.curIdx = idx;
    this->Modified();
  }

}

//----------------------------------------------------------------------------
int vtkMPASReader::GetDimensionSize(const string &dim)
{
  this->UpdateDimensions();

  typedef Internal::DimMetaDataMap::const_iterator Iter;
  Iter it = this->Internals->dimMetaDataMap.find(dim);
  if (it == this->Internals->dimMetaDataMap.end())
  {
    return -1;
  }
  return it->second.dimSize;
}

//----------------------------------------------------------------------------
//  Set vertical level to be viewed.
//----------------------------------------------------------------------------

void vtkMPASReader::SetVerticalLevel(int level)
{
  this->SetDimensionCurrentIndex(this->VerticalDimension, level);
}

//------------------------------------------------------------------------------
int vtkMPASReader::GetVerticalLevel()
{
  return this->GetDimensionCurrentIndex(this->VerticalDimension);
}

//----------------------------------------------------------------------------
//  Set center longitude for lat/lon projection
//----------------------------------------------------------------------------

void vtkMPASReader::SetCenterLon(int val)
{
  vtkDebugMacro(<< "SetCenterLon: is " << this->CenterLon << endl);
  if (CenterLon != val)
  {
    this->CenterLon = val;
    this->CenterRad = val * vtkMath::Pi() / 180.0;
    this->Modified();

    vtkDebugMacro(<< "SetCenterLon: set to " << this->CenterLon << endl);
    vtkDebugMacro(<< "CenterRad set to " << this->CenterRad << endl);
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
  return ret;
}

//------------------------------------------------------------------------------
vtkMTimeType vtkMPASReader::GetMTime()
{
  vtkMTimeType result = this->Superclass::GetMTime();
  result = std::max(result, this->CellDataArraySelection->GetMTime());
  result = std::max(result, this->PointDataArraySelection->GetMTime());
  // Excluded, as this just manages a cache:
  //  result = std::max(result, this->Internals->extraDimTime.GetMTime());
  result = std::max(result, this->Internals->dimMetaDataTime.GetMTime());
  return result;
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
  os << indent << "this->MaximumPoints: " << this->MaximumPoints << "\n";
  os << indent << "this->MaximumCells: " << this->MaximumCells << "\n";
  os << indent << "ProjectLatLon: "
     << (this->ProjectLatLon?"ON":"OFF") << endl;
  os << indent << "OnASphere: "
     << (this->OnASphere?"ON":"OFF") << endl;
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

//------------------------------------------------------------------------------
int vtkMPASReader::GetNumberOfCellVars()
{
  return static_cast<int>(this->Internals->cellVars.size());
}

//------------------------------------------------------------------------------
int vtkMPASReader::GetNumberOfPointVars()
{
  return static_cast<int>(this->Internals->pointVars.size());
}
