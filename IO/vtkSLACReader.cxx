// -*- c++ -*-
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSLACReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

#include "vtkSLACReader.h"

#include "vtkCallbackCommand.h"
#include "vtkCellArray.h"
#include "vtkCompositeDataIterator.h"
#include "vtkDataArraySelection.h"
#include "vtkDoubleArray.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationIntegerKey.h"
#include "vtkInformationVector.h"
#include "vtkInformationObjectBaseKey.h"
#include "vtkMath.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkStdString.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnstructuredGrid.h"

#include "vtkSmartPointer.h"
#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

#include <netcdf.h>

#include <vtkstd/algorithm>
#include <vtkstd/map>
#include <vtkstd/vector>
#include <vtksys/hash_map.hxx>
#include <vtksys/RegularExpression.hxx>

#include <math.h>

//=============================================================================
#define CALL_NETCDF(call)                       \
  { \
    int errorcode = call; \
    if (errorcode != NC_NOERR) \
      { \
      vtkErrorMacro(<< "netCDF Error: " << nc_strerror(errorcode)); \
      return 0; \
      } \
  }

#define WRAP_NETCDF(call) \
  { \
    int errorcode = call; \
    if (errorcode != NC_NOERR) return errorcode; \
  }

//-----------------------------------------------------------------------------
#ifdef VTK_USE_64BIT_IDS
#ifdef NC_INT64
// This may or may not work with the netCDF 4 library reading in netCDF 3 files.
#define nc_get_var_vtkIdType nc_get_var_longlong
#define nc_get_vars_vtkIdType nc_get_vars_longlong
#else // NC_INT64
static int nc_get_var_vtkIdType(int ncid, int varid, vtkIdType *ip)
{
  // Step 1, figure out how many entries in the given variable.
  int numdims, dimids[NC_MAX_VAR_DIMS];
  WRAP_NETCDF(nc_inq_varndims(ncid, varid, &numdims));
  WRAP_NETCDF(nc_inq_vardimid(ncid, varid, dimids));
  vtkIdType numValues = 1;
  for (int dim = 0; dim < numdims; dim++)
    {
    size_t dimlen;
    WRAP_NETCDF(nc_inq_dimlen(ncid, dimids[dim], &dimlen));
    numValues *= dimlen;
    }

  // Step 2, read the data in as 32 bit integers.  Recast the input buffer
  // so we do not have to create a new one.
  long *smallIp = reinterpret_cast<long*>(ip);
  WRAP_NETCDF(nc_get_var_long(ncid, varid, smallIp));

  // Step 3, recast the data from 32 bit integers to 64 bit integers.  Since we
  // are storing both in the same buffer, we need to be careful to not overwrite
  // uncopied 32 bit numbers with 64 bit numbers.  We can do that by copying
  // backwards.
  for (vtkIdType i = numValues-1; i >= 0; i--)
    {
    ip[i] = static_cast<vtkIdType>(smallIp[i]);
    }

  return NC_NOERR;
}
static int nc_get_vars_vtkIdType(int ncid, int varid,
                                 const size_t start[], const size_t count[],
                                 const ptrdiff_t stride[],
                                 vtkIdType *ip)
{
  // Step 1, figure out how many entries in the given variable.
  int numdims;
  WRAP_NETCDF(nc_inq_varndims(ncid, varid, &numdims));
  vtkIdType numValues = 1;
  for (int dim = 0; dim < numdims; dim++)
    {
    numValues *= count[dim];
    }

  // Step 2, read the data in as 32 bit integers.  Recast the input buffer
  // so we do not have to create a new one.
  long *smallIp = reinterpret_cast<long*>(ip);
  WRAP_NETCDF(nc_get_vars_long(ncid, varid, start, count, stride, smallIp));

  // Step 3, recast the data from 32 bit integers to 64 bit integers.  Since we
  // are storing both in the same buffer, we need to be careful to not overwrite
  // uncopied 32 bit numbers with 64 bit numbers.  We can do that by copying
  // backwards.
  for (vtkIdType i = numValues-1; i >= 0; i--)
    {
    ip[i] = static_cast<vtkIdType>(smallIp[i]);
    }

  return NC_NOERR;
}
#endif // NC_INT64
#else // VTK_USE_64_BIT_IDS
#define nc_get_var_vtkIdType nc_get_var_int
#define nc_get_vars_vtkIdType nc_get_vars_int
#endif // VTK_USE_64BIT_IDS

//-----------------------------------------------------------------------------
// This convenience function gets a scalar variable as a double, doing the
// appropriate checks.
static int nc_get_scalar_double(int ncid, const char *name, double *dp)
{
  int varid;
  WRAP_NETCDF(nc_inq_varid(ncid, name, &varid));
  int numdims;
  WRAP_NETCDF(nc_inq_varndims(ncid, varid, &numdims));
  if (numdims != 0)
    {
    // Not a great error to return, but better than nothing.
    return NC_EVARSIZE;
    }
  WRAP_NETCDF(nc_get_var_double(ncid, varid, dp));

  return NC_NOERR;
}

//=============================================================================
// Describes how faces are defined in a tetrahedra in the files.
const int tetFaces[4][3] = {
  { 0, 2, 1 },
  { 0, 3, 2 },
  { 0, 1, 3 },
  { 1, 2, 3 }
};

// Describes the points on each edge of a VTK triangle.  The edges are in the
// same order as the midpoints are defined in a VTK quadratic triangle.
const int triEdges[3][2] = {
  { 0, 1 },
  { 1, 2 },
  { 0, 2 }
};

//=============================================================================
#define MY_MIN(x, y)    ((x) < (y) ? (x) : (y))
#define MY_MAX(x, y)    ((x) < (y) ? (y) : (x))

//=============================================================================
static int NetCDFTypeToVTKType(nc_type type)
{
  switch (type)
    {
    case NC_BYTE: return VTK_UNSIGNED_CHAR;
    case NC_CHAR: return VTK_CHAR;
    case NC_SHORT: return VTK_SHORT;
    case NC_INT: return VTK_INT;
    case NC_FLOAT: return VTK_FLOAT;
    case NC_DOUBLE: return VTK_DOUBLE;
    default:
      vtkGenericWarningMacro(<< "Unknown netCDF variable type "
                             << type);
      return -1;
    }
}

//=============================================================================
// This class automatically closes a netCDF file descripter when it goes out
// of scope.  This allows us to exit on error without having to close the
// file at every instance.
class vtkSLACReaderAutoCloseNetCDF
{
public:
  vtkSLACReaderAutoCloseNetCDF(const char *filename, int omode,
                               bool quiet=false) {
    int errorcode = nc_open(filename, omode, &this->fd);
    if (errorcode != NC_NOERR)
      {
      if (!quiet)
        {
        vtkGenericWarningMacro(<< "Could not open " << filename << endl
                               << nc_strerror(errorcode));
        }
      this->fd = -1;
      }
  }
  ~vtkSLACReaderAutoCloseNetCDF() {
    if (this->fd != -1)
      {
      nc_close(this->fd);
      }
  }
  int operator()() const { return this->fd; }
  bool Valid() const { return this->fd != -1; }
protected:
  int fd;
private:
  vtkSLACReaderAutoCloseNetCDF();       // Not implemented
  vtkSLACReaderAutoCloseNetCDF(const vtkSLACReaderAutoCloseNetCDF &); // Not implemented
  void operator=(const vtkSLACReaderAutoCloseNetCDF &); // Not implemented
};

//=============================================================================
// A convenience function that gets a block from a multiblock data set,
// performing allocation if necessary.
static vtkUnstructuredGrid *AllocateGetBlock(vtkMultiBlockDataSet *blocks,
                                             unsigned int blockno,
                                             vtkInformationIntegerKey *typeKey)
{
  if (blockno > 1000)
    {
    vtkGenericWarningMacro(<< "Unexpected block number: " << blockno);
    blockno = 0;
    }

  if (blocks->GetNumberOfBlocks() <= blockno)
    {
    blocks->SetNumberOfBlocks(blockno+1);
    }

  vtkUnstructuredGrid *grid
    = vtkUnstructuredGrid::SafeDownCast(blocks->GetBlock(blockno));
  if (!grid)
    {
    grid = vtkUnstructuredGrid::New();
    blocks->SetBlock(blockno, grid);
    blocks->GetMetaData(blockno)->Set(typeKey, 1);
    grid->Delete();     // Not really deleted.
    }

  return grid;
}

//=============================================================================
// Classes for storing midpoint maps.  These are basically wrappers around STL
// maps.

// I originally had this placed inside of the vtkSLACReader declaration, where
// it makes much more sense.  However, MSVC6 seems to have a problem with this.
struct vtkSLACReaderEdgeEndpointsHash {
public:
  size_t operator()(const vtkSLACReader::EdgeEndpoints &edge) const {
    return static_cast<size_t>(edge.GetMinEndPoint() + edge.GetMaxEndPoint());
  }
};

//-----------------------------------------------------------------------------
class vtkSLACReader::MidpointCoordinateMap::vtkInternal
{
public:
  typedef vtksys::hash_map<vtkSLACReader::EdgeEndpoints,
                           vtkSLACReader::MidpointCoordinates,
                           vtkSLACReaderEdgeEndpointsHash> MapType;
  MapType Map;
};

vtkSLACReader::MidpointCoordinateMap::MidpointCoordinateMap()
{
  this->Internal = new vtkSLACReader::MidpointCoordinateMap::vtkInternal;
}

vtkSLACReader::MidpointCoordinateMap::~MidpointCoordinateMap()
{
  delete this->Internal;
}

void vtkSLACReader::MidpointCoordinateMap::AddMidpoint(
                                            const EdgeEndpoints &edge,
                                            const MidpointCoordinates &midpoint)
{
  this->Internal->Map[edge] = midpoint;
}

void vtkSLACReader::MidpointCoordinateMap::RemoveMidpoint(
                                                      const EdgeEndpoints &edge)
{
  vtkInternal::MapType::iterator iter = this->Internal->Map.find(edge);
  if (iter != this->Internal->Map.end())
    {
    this->Internal->Map.erase(iter);
    }
}

void vtkSLACReader::MidpointCoordinateMap::RemoveAllMidpoints()
{
  this->Internal->Map.clear();
}

vtkIdType vtkSLACReader::MidpointCoordinateMap::GetNumberOfMidpoints() const
{
  return static_cast<vtkIdType>(this->Internal->Map.size());
}

vtkSLACReader::MidpointCoordinates *
vtkSLACReader::MidpointCoordinateMap::FindMidpoint(const EdgeEndpoints &edge)
{
  vtkInternal::MapType::iterator iter = this->Internal->Map.find(edge);
  if (iter != this->Internal->Map.end())
    {
    return &iter->second;
    }
  else
    {
    return NULL;
    }
}

//-----------------------------------------------------------------------------
class vtkSLACReader::MidpointIdMap::vtkInternal
{
public:
  typedef vtksys::hash_map<vtkSLACReader::EdgeEndpoints, vtkIdType,
                           vtkSLACReaderEdgeEndpointsHash> MapType;
  MapType Map;
  MapType::iterator Iterator;
};

vtkSLACReader::MidpointIdMap::MidpointIdMap()
{
  this->Internal = new vtkSLACReader::MidpointIdMap::vtkInternal;
}

vtkSLACReader::MidpointIdMap::~MidpointIdMap()
{
  delete this->Internal;
}

void vtkSLACReader::MidpointIdMap::AddMidpoint(const EdgeEndpoints &edge,
                                               vtkIdType midpoint)
{
  this->Internal->Map[edge] = midpoint;
}

void vtkSLACReader::MidpointIdMap::RemoveMidpoint(const EdgeEndpoints &edge)
{
  vtkInternal::MapType::iterator iter = this->Internal->Map.find(edge);
  if (iter != this->Internal->Map.end())
    {
    this->Internal->Map.erase(iter);
    }
}

void vtkSLACReader::MidpointIdMap::RemoveAllMidpoints()
{
  this->Internal->Map.clear();
}

vtkIdType vtkSLACReader::MidpointIdMap::GetNumberOfMidpoints() const
{
  return static_cast<vtkIdType>(this->Internal->Map.size());
}

vtkIdType *vtkSLACReader::MidpointIdMap::FindMidpoint(const EdgeEndpoints &edge)
{
  vtkInternal::MapType::iterator iter = this->Internal->Map.find(edge);
  if (iter != this->Internal->Map.end())
    {
    return &iter->second;
    }
  else
    {
    return NULL;
    }
}

void vtkSLACReader::MidpointIdMap::InitTraversal()
{
  this->Internal->Iterator = this->Internal->Map.begin();
}

bool vtkSLACReader::MidpointIdMap::GetNextMidpoint(EdgeEndpoints &edge,
                                                   vtkIdType &midpoint)
{
  if (this->Internal->Iterator == this->Internal->Map.end()) return false;

  edge = this->Internal->Iterator->first;
  midpoint = this->Internal->Iterator->second;

  this->Internal->Iterator++;
  return true;
}

//=============================================================================
vtkStandardNewMacro(vtkSLACReader);

vtkInformationKeyMacro(vtkSLACReader, IS_INTERNAL_VOLUME, Integer);
vtkInformationKeyMacro(vtkSLACReader, IS_EXTERNAL_SURFACE, Integer);
vtkInformationKeyMacro(vtkSLACReader, POINTS, ObjectBase);
vtkInformationKeyMacro(vtkSLACReader, POINT_DATA, ObjectBase);

//-----------------------------------------------------------------------------
// The internals class mostly holds templated ivars that we don't want to
// expose in the header file.
class vtkSLACReader::vtkInternal
{
public:
  vtkstd::vector<vtkStdString> ModeFileNames;

  vtkSmartPointer<vtkDataArraySelection> VariableArraySelection;

  // Description:
  // A quick lookup to find the correct mode file name given a time value.
  // Only valid when TimeStepModes is true.
  vtkstd::map<double, vtkStdString> TimeStepToFile;

  // Description:
  // References and shallow copies to the last output data.  We keep this
  // arround in case we do not have to read everything in again.
  vtkSmartPointer<vtkPoints> PointCache;
  vtkSmartPointer<vtkMultiBlockDataSet> MeshCache;
  MidpointIdMap MidpointIdCache;
};

//-----------------------------------------------------------------------------
vtkSLACReader::vtkSLACReader()
{
  this->Internal = new vtkSLACReader::vtkInternal;

  this->SetNumberOfInputPorts(0);

  this->MeshFileName = NULL;

  this->ReadInternalVolume = 0;
  this->ReadExternalSurface = 1;
  this->ReadMidpoints = 1;

  this->Internal->VariableArraySelection
    = vtkSmartPointer<vtkDataArraySelection>::New();
  VTK_CREATE(vtkCallbackCommand, cbc);
  cbc->SetCallback(&vtkSLACReader::SelectionModifiedCallback);
  cbc->SetClientData(this);
  this->Internal->VariableArraySelection->AddObserver(vtkCommand::ModifiedEvent,
                                                      cbc);

  this->ReadModeData = false;
  this->TimeStepModes = false;
  this->FrequencyModes = false;

  this->SetNumberOfOutputPorts(NUM_OUTPUTS);
}

vtkSLACReader::~vtkSLACReader()
{
  this->SetMeshFileName(NULL);

  delete this->Internal;
}

void vtkSLACReader::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  if (this->MeshFileName)
    {
    os << indent << "MeshFileName: " << this->MeshFileName << endl;
    }
  else
    {
    os << indent << "MeshFileName: (null)\n";
    }

  for (unsigned int i = 0; i < this->Internal->ModeFileNames.size(); i++)
    {
    os << indent << "ModeFileName[" << i << "]: "
       << this->Internal->ModeFileNames[i] << endl;
    }

  os << indent << "ReadInternalVolume: " << this->ReadInternalVolume << endl;
  os << indent << "ReadExternalSurface: " << this->ReadExternalSurface << endl;
  os << indent << "ReadMidpoints: " << this->ReadMidpoints << endl;

  os << indent << "VariableArraySelection:" << endl;
  this->Internal->VariableArraySelection->PrintSelf(os, indent.GetNextIndent());
}

//-----------------------------------------------------------------------------
int vtkSLACReader::CanReadFile(const char *filename)
{
  vtkSLACReaderAutoCloseNetCDF ncFD(filename, NC_NOWRITE, true);
  if (!ncFD.Valid()) return 0;

  // Check for the existence of several arrays we know should be in the file.
  int dummy;
  if (nc_inq_varid(ncFD(), "coords", &dummy) != NC_NOERR) return 0;
  if (nc_inq_varid(ncFD(), "tetrahedron_interior",&dummy) != NC_NOERR) return 0;
  if (nc_inq_varid(ncFD(), "tetrahedron_exterior",&dummy) != NC_NOERR) return 0;

  return 1;
}

//-----------------------------------------------------------------------------
void vtkSLACReader::AddModeFileName(const char *fname)
{
  this->Internal->ModeFileNames.push_back(fname);
  this->Modified();
}

void vtkSLACReader::RemoveAllModeFileNames()
{
  this->Internal->ModeFileNames.clear();
  this->Modified();
}

unsigned int vtkSLACReader::GetNumberOfModeFileNames()
{
  return static_cast<unsigned int>(this->Internal->ModeFileNames.size());
}

const char *vtkSLACReader::GetModeFileName(unsigned int idx)
{
  return this->Internal->ModeFileNames[idx].c_str();
}

//-----------------------------------------------------------------------------
vtkIdType vtkSLACReader::GetNumTuplesInVariable(int ncFD, int varId,
                                                int expectedNumComponents)
{
  int numDims;
  CALL_NETCDF(nc_inq_varndims(ncFD, varId, &numDims));
  if (numDims != 2)
    {
    char name[NC_MAX_NAME+1];
    CALL_NETCDF(nc_inq_varname(ncFD, varId, name));
    vtkErrorMacro(<< "Wrong dimensions on " << name);
    return 0;
    }

  int dimIds[2];
  CALL_NETCDF(nc_inq_vardimid(ncFD, varId, dimIds));

  size_t dimLength;
  CALL_NETCDF(nc_inq_dimlen(ncFD, dimIds[1], &dimLength));
  if (static_cast<int>(dimLength) != expectedNumComponents)
    {
    char name[NC_MAX_NAME+1];
    CALL_NETCDF(nc_inq_varname(ncFD, varId, name));
    vtkErrorMacro(<< "Unexpected tuple size on " << name);
    return 0;
    }

  CALL_NETCDF(nc_inq_dimlen(ncFD, dimIds[0], &dimLength));
  return static_cast<vtkIdType>(dimLength);
}

//-----------------------------------------------------------------------------
int vtkSLACReader::RequestInformation(
                                 vtkInformation *vtkNotUsed(request),
                                 vtkInformationVector **vtkNotUsed(inputVector),
                                 vtkInformationVector *outputVector)
{
  vtkInformation *surfaceOutInfo
    = outputVector->GetInformationObject(SURFACE_OUTPUT);
  surfaceOutInfo->Remove(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
  surfaceOutInfo->Remove(vtkStreamingDemandDrivenPipeline::TIME_RANGE());

  vtkInformation *volumeOutInfo
    = outputVector->GetInformationObject(VOLUME_OUTPUT);
  volumeOutInfo->Remove(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
  volumeOutInfo->Remove(vtkStreamingDemandDrivenPipeline::TIME_RANGE());

  if (!this->MeshFileName)
    {
    vtkErrorMacro("No filename specified.");
    return 0;
    }

  this->Internal->VariableArraySelection->RemoveAllArrays();

  vtkSLACReaderAutoCloseNetCDF meshFD(this->MeshFileName, NC_NOWRITE);
  if (!meshFD.Valid()) return 0;

  this->ReadModeData = false;   // Assume false until everything checks out.
  this->TimeStepModes = false;
  this->Internal->TimeStepToFile.clear();
  this->FrequencyModes = false;
  this->Frequency = 0.0;
  if (!this->Internal->ModeFileNames.empty())
    {
    // Check the first mode file, assume that the rest follow.
    vtkSLACReaderAutoCloseNetCDF modeFD(this->Internal->ModeFileNames[0],
                                        NC_NOWRITE);
    if (!modeFD.Valid()) return 0;

    int meshCoordsVarId, modeCoordsVarId;
    CALL_NETCDF(nc_inq_varid(meshFD(), "coords", &meshCoordsVarId));
    CALL_NETCDF(nc_inq_varid(modeFD(), "coords", &modeCoordsVarId));

    if (   this->GetNumTuplesInVariable(meshFD(), meshCoordsVarId, 3)
        != this->GetNumTuplesInVariable(modeFD(), modeCoordsVarId, 3) )
      {
      vtkWarningMacro(<< "Mode file "
                      << this->Internal->ModeFileNames[0].c_str()
                      << " invalid for mesh file " << this->MeshFileName
                      << "; the number of coordinates do not match.");
      }
    else
      {
      this->ReadModeData = true;

      // Read the "frequency".  When a time series is written, the frequency
      // variable is overloaded to mean time.  There is no direct way to tell
      // the difference, but things happen very quickly (less than nanoseconds)
      // in simulations that write out this data.  Thus, we expect large numbers
      // to be frequency (in Hz) and small numbers to be time (in seconds).
      if (   (nc_get_scalar_double(modeFD(), "frequency", &this->Frequency) != NC_NOERR)
          && (nc_get_scalar_double(modeFD(), "frequencyreal", &this->Frequency) != NC_NOERR) )
        {
        vtkWarningMacro(<< "Could not find frequency in mode data.");
        return 0;
        }
      if (this->Frequency < 100)
        {
        this->TimeStepModes = true;
        this->Internal->TimeStepToFile[this->Frequency]
          = this->Internal->ModeFileNames[0];
        }
      else
        {
        this->FrequencyModes = true;
        }

      //vtksys::RegularExpression imaginaryVar("_imag$");

      int ncoordDim;
      CALL_NETCDF(nc_inq_dimid(modeFD(), "ncoord", &ncoordDim));

      int numVariables;
      CALL_NETCDF(nc_inq_nvars(modeFD(), &numVariables));

      for (int i = 0; i < numVariables; i++)
        {
        int numDims;
        CALL_NETCDF(nc_inq_varndims(modeFD(), i, &numDims));
        if ((numDims < 1) || (numDims > 2)) continue;

        int dimIds[2];
        CALL_NETCDF(nc_inq_vardimid(modeFD(), i, dimIds));
        if (dimIds[0] != ncoordDim) continue;

        char name[NC_MAX_NAME+1];
        CALL_NETCDF(nc_inq_varname(modeFD(), i, name));
        if (strcmp(name, "coords") == 0) continue;
        //if (this->FrequencyModes && imaginaryVar.find(name)) continue;

        this->Internal->VariableArraySelection->AddArray(name);
        }
      }
    }

  if (this->TimeStepModes)
    {
    // If we are in time steps modes, we need to read in the time values from
    // all the files (and we have already read the first one).  We then report
    // the time steps we have.
    vtkstd::vector<vtkStdString>::iterator fileitr
      = this->Internal->ModeFileNames.begin();
    fileitr++;
    for ( ; fileitr != this->Internal->ModeFileNames.end(); fileitr++)
      {
      vtkSLACReaderAutoCloseNetCDF modeFD(*fileitr, NC_NOWRITE);
      if (!modeFD.Valid()) return 0;

      if (   (nc_get_scalar_double(modeFD(), "frequency", &this->Frequency) != NC_NOERR)
          && (nc_get_scalar_double(modeFD(), "frequencyreal", &this->Frequency) != NC_NOERR) )
        {
        vtkWarningMacro(<< "Could not find frequency in mode data.");
        return 0;
        }
      this->Internal->TimeStepToFile[this->Frequency] = *fileitr;
      }

    double range[2];
    surfaceOutInfo->Remove(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
    volumeOutInfo->Remove(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
    vtkstd::map<double, vtkStdString>::iterator timeitr
      = this->Internal->TimeStepToFile.begin();
    range[0] = timeitr->first;
    for ( ; timeitr != this->Internal->TimeStepToFile.end(); timeitr++)
      {
      range[1] = timeitr->first;        // Eventually set to last value.
      surfaceOutInfo->Append(vtkStreamingDemandDrivenPipeline::TIME_STEPS(),
                             timeitr->first);
      volumeOutInfo->Append(vtkStreamingDemandDrivenPipeline::TIME_STEPS(),
                            timeitr->first);
      }
    surfaceOutInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(),range,2);
    volumeOutInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(),range,2);
    }
  else if (this->FrequencyModes)
    {
    double range[2];
    range[0] = 0;
    range[1] = 1.0/this->Frequency;
    surfaceOutInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(),range,2);
    volumeOutInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(),range,2);
    }

  return 1;
}

//-----------------------------------------------------------------------------
int vtkSLACReader::RequestData(vtkInformation *request,
                               vtkInformationVector **vtkNotUsed(inputVector),
                               vtkInformationVector *outputVector)
{
  vtkInformation *outInfo[NUM_OUTPUTS];
  for (int i = 0; i < NUM_OUTPUTS; i++)
    {
    outInfo[i] = outputVector->GetInformationObject(i);
    }

  vtkMultiBlockDataSet *surfaceOutput
    = vtkMultiBlockDataSet::GetData(outInfo[SURFACE_OUTPUT]);
  vtkMultiBlockDataSet *volumeOutput
    = vtkMultiBlockDataSet::GetData(outInfo[VOLUME_OUTPUT]);

  if (!this->MeshFileName)
    {
    vtkErrorMacro("No filename specified.");
    return 0;
    }

  double time = 0.0;
  bool timeValid = false;
  int fromPort = request->Get(vtkExecutive::FROM_OUTPUT_PORT());
  if (outInfo[fromPort]->Has(
                         vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEPS()))
    {
    time = outInfo[fromPort]->Get(
                       vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEPS(),0);
    timeValid = true;
    }

  if (this->FrequencyModes)
    {
    this->Phase = vtkMath::DoubleTwoPi()*(time*this->Frequency);
    }

  int readMesh = !this->MeshUpToDate();

  // This convenience object holds the composite of the surface and volume
  // outputs.  Since each of these outputs is multiblock (and needs iterators)
  // anyway, then subroutines can just iterate over everything once.
  VTK_CREATE(vtkMultiBlockDataSet, compositeOutput);

  if (readMesh)
    {
    this->Internal->MidpointIdCache.RemoveAllMidpoints();
    this->Internal->MeshCache = vtkSmartPointer<vtkMultiBlockDataSet>::New();

    vtkSLACReaderAutoCloseNetCDF meshFD(this->MeshFileName, NC_NOWRITE);
    if (!meshFD.Valid()) return 0;

    if (!this->ReadInternalVolume && !this->ReadExternalSurface) return 1;

    if (!this->ReadConnectivity(meshFD(),surfaceOutput,volumeOutput)) return 0;

    this->UpdateProgress(0.25);

    // Shove two outputs in composite output.
    compositeOutput->SetNumberOfBlocks(2);
    compositeOutput->SetBlock(SURFACE_OUTPUT, surfaceOutput);
    compositeOutput->SetBlock(VOLUME_OUTPUT,  volumeOutput);
    compositeOutput->GetMetaData(static_cast<unsigned int>(SURFACE_OUTPUT))
      ->Set(vtkCompositeDataSet::NAME(), "Internal Volume");
    compositeOutput->GetMetaData(static_cast<unsigned int>(VOLUME_OUTPUT))
      ->Set(vtkCompositeDataSet::NAME(), "External Surface");

    // Set up point data.
    VTK_CREATE(vtkPoints, points);
    VTK_CREATE(vtkPointData, pd);
    compositeOutput->GetInformation()->Set(vtkSLACReader::POINTS(), points);
    compositeOutput->GetInformation()->Set(vtkSLACReader::POINT_DATA(), pd);

    if (!this->ReadCoordinates(meshFD(), compositeOutput)) return 0;

    this->UpdateProgress(0.5);

    // if surface_midpoint requested
    if (this->ReadMidpoints)
      {
      // if midpoints present in file 
      int dummy;
      if (nc_inq_varid(meshFD(), "surface_midpoint", &dummy) == NC_NOERR) 
        {
        if (!this->ReadMidpointData(meshFD(), compositeOutput,
                                    this->Internal->MidpointIdCache))
          {
          return 0;
          }
        }
      else // midpoints requested, but not in file
        {
        //   spit out warning and ignore the midpoint read request.
        vtkWarningMacro(<< "Midpoints requested, but not present in the mesh file.  Igoring the request.");
        }
      }

    this->Internal->MeshCache->ShallowCopy(compositeOutput);
    this->Internal->PointCache = points;
    this->MeshReadTime.Modified();
    }
  else
    {
    if (!this->RestoreMeshCache(surfaceOutput, volumeOutput, compositeOutput))
      {
      return 0;
      }
    }

  this->UpdateProgress(0.75);

  if (this->ReadModeData)
    {
    vtkStdString modeFileName;
    if (this->TimeStepModes && timeValid)
      {
      modeFileName = this->Internal->TimeStepToFile.lower_bound(time)->second;
      }
    else
      {
      modeFileName = this->Internal->ModeFileNames[0];
      }
    vtkSLACReaderAutoCloseNetCDF modeFD(modeFileName, NC_NOWRITE);
    if (!modeFD.Valid()) return 0;

    if (!this->ReadFieldData(modeFD(), compositeOutput)) return 0;

    this->UpdateProgress(0.875);

    if (!this->InterpolateMidpointData(compositeOutput,
                                       this->Internal->MidpointIdCache))
      {
      return 0;
      }

    if (timeValid)
      {
      surfaceOutput->GetInformation()->Set(vtkDataObject::DATA_TIME_STEPS(),
                                           &time, 1);
      volumeOutput->GetInformation()->Set(vtkDataObject::DATA_TIME_STEPS(),
                                          &time, 1);
      }
    }

  // Push points to output.
  vtkPoints *points = vtkPoints::SafeDownCast(
               compositeOutput->GetInformation()->Get(vtkSLACReader::POINTS()));
  VTK_CREATE(vtkCompositeDataIterator, outputIter);
  for (outputIter.TakeReference(compositeOutput->NewIterator());
       !outputIter->IsDoneWithTraversal(); outputIter->GoToNextItem())
    {
    vtkUnstructuredGrid *ugrid = vtkUnstructuredGrid::SafeDownCast(
                                       compositeOutput->GetDataSet(outputIter));
    ugrid->SetPoints(points);
    }

  // Push point field data to output.
  vtkPointData *pd = vtkPointData::SafeDownCast(
           compositeOutput->GetInformation()->Get(vtkSLACReader::POINT_DATA()));
  for (outputIter.TakeReference(compositeOutput->NewIterator());
       !outputIter->IsDoneWithTraversal(); outputIter->GoToNextItem())
    {
    vtkUnstructuredGrid *ugrid = vtkUnstructuredGrid::SafeDownCast(
                                       compositeOutput->GetDataSet(outputIter));
    ugrid->GetPointData()->ShallowCopy(pd);
    }

  return 1;
}

//----------------------------------------------------------------------------
void vtkSLACReader::SelectionModifiedCallback(vtkObject*, unsigned long,
                                              void* clientdata, void*)
{
  static_cast<vtkSLACReader*>(clientdata)->Modified();
}

//-----------------------------------------------------------------------------
int vtkSLACReader::GetNumberOfVariableArrays()
{
  return this->Internal->VariableArraySelection->GetNumberOfArrays();
}

//-----------------------------------------------------------------------------
const char* vtkSLACReader::GetVariableArrayName(int index)
{
  return this->Internal->VariableArraySelection->GetArrayName(index);
}

//-----------------------------------------------------------------------------
int vtkSLACReader::GetVariableArrayStatus(const char* name)
{
  return this->Internal->VariableArraySelection->ArrayIsEnabled(name);
}

//-----------------------------------------------------------------------------
void vtkSLACReader::SetVariableArrayStatus(const char* name, int status)
{
  vtkDebugMacro("Set cell array \"" << name << "\" status to: " << status);
  if(status)
    {
    this->Internal->VariableArraySelection->EnableArray(name);
    }
  else
    {
    this->Internal->VariableArraySelection->DisableArray(name);
    }
}

//-----------------------------------------------------------------------------
int vtkSLACReader::ReadTetrahedronInteriorArray(int meshFD,
                                                vtkIdTypeArray *connectivity)
{
  int tetInteriorVarId;
  CALL_NETCDF(nc_inq_varid(meshFD, "tetrahedron_interior", &tetInteriorVarId));
  vtkIdType numTetsInterior
    = this->GetNumTuplesInVariable(meshFD, tetInteriorVarId, NumPerTetInt);

  connectivity->Initialize();
  connectivity->SetNumberOfComponents(NumPerTetInt);
  connectivity->SetNumberOfTuples(numTetsInterior);
  CALL_NETCDF(nc_get_var_vtkIdType(meshFD, tetInteriorVarId,
                                   connectivity->GetPointer(0)));

  return 1;
}

//-----------------------------------------------------------------------------
int vtkSLACReader::ReadTetrahedronExteriorArray(int meshFD,
                                                vtkIdTypeArray *connectivity)
{
  int tetExteriorVarId;
  CALL_NETCDF(nc_inq_varid(meshFD, "tetrahedron_exterior", &tetExteriorVarId));
  vtkIdType numTetsExterior
    = this->GetNumTuplesInVariable(meshFD, tetExteriorVarId, NumPerTetExt);

  connectivity->Initialize();
  connectivity->SetNumberOfComponents(NumPerTetExt);
  connectivity->SetNumberOfTuples(numTetsExterior);
  CALL_NETCDF(nc_get_var_vtkIdType(meshFD, tetExteriorVarId,
                                   connectivity->GetPointer(0)));

  return 1;
}

//-----------------------------------------------------------------------------
int vtkSLACReader::CheckTetrahedraWinding(int meshFD)
{
  int i;

  // Read in the first interior tetrahedron topology.
  int tetInteriorVarId;
  CALL_NETCDF(nc_inq_varid(meshFD, "tetrahedron_interior", &tetInteriorVarId));

  size_t start[2], count[2];
  start[0] = 0;  count[0] = 1;
  start[1] = 0;  count[1] = NumPerTetInt;

  vtkIdType tetTopology[NumPerTetInt];
  CALL_NETCDF(nc_get_vars_vtkIdType(meshFD, tetInteriorVarId, start, count,
                                    NULL, tetTopology));

  // Read in the point coordinates for the tetrahedron.  The indices for the
  // points are stored in values 1-4 of tetTopology.
  int coordsVarId;
  CALL_NETCDF(nc_inq_varid(meshFD, "coords", &coordsVarId));

  double pts[4][3];
  for (i = 0; i < 4; i++)
    {
    start[0] = tetTopology[i+1];  count[0] = 1;
    start[1] = 0;                 count[1] = 3;
    CALL_NETCDF(nc_get_vars_double(meshFD, coordsVarId, start, count,
                                   NULL, pts[i]));
    }

  // Given the coordinates of the tetrahedron points, determine the direction of
  // the winding.  Note that this test will fail if the tetrahedron is
  // degenerate.  The first step is finding the normal of the triangle (0,1,2).
  double v1[3], v2[3], n[3];
  for (i = 0; i < 3; i++)
    {
    v1[i] = pts[1][i] - pts[0][i];
    v2[i] = pts[2][i] - pts[0][i];
    }
  vtkMath::Cross(v1, v2, n);

  // For the VTK winding, the normal, n, should point toward the fourth point
  // of the tetrahedron.
  double v3[3];
  for (i = 0; i < 3; i++)  v3[i] = pts[3][i] - pts[0][i];
  double dir = vtkMath::Dot(v3, n);
  return (dir >= 0.0);
}

//-----------------------------------------------------------------------------
int vtkSLACReader::ReadConnectivity(int meshFD,
                                    vtkMultiBlockDataSet *surfaceOutput,
                                    vtkMultiBlockDataSet *volumeOutput)
{
  // Decide if we need to invert the tetrahedra to make them compatible
  // with VTK winding.
  int invertTets = !this->CheckTetrahedraWinding(meshFD);

  // Read in interior tetrahedra.
  VTK_CREATE(vtkIdTypeArray, connectivity);
  if (this->ReadInternalVolume)
    {
    if (!this->ReadTetrahedronInteriorArray(meshFD, connectivity)) return 0;
    vtkIdType numTetsInterior = connectivity->GetNumberOfTuples();
    for (vtkIdType i = 0; i < numTetsInterior; i++)
      {
      // Interior tetrahedra are defined with 5 integers.  The first is an
      // element attribute (which we will use to separate into multiple blocks)
      // and the other four are ids for the 4 points of the tetrahedra.  The
      // faces of the tetrahedra are the following:
      // Face 0:  0,  2,  1
      // Face 1:  0,  3,  2
      // Face 2:  0,  1,  3
      // Face 3:  1,  2,  3
      // There are two possible "windings," the direction in which the normals
      // face, for any given tetrahedra.  SLAC files might support either
      // winding, but it should be consistent through the mesh.  The invertTets
      // flag set earlier indicates whether we need to invert the tetrahedra.
      vtkIdType tetInfo[NumPerTetInt];
      connectivity->GetTupleValue(i, tetInfo);
      if (invertTets) vtkstd::swap(tetInfo[1], tetInfo[2]);
      vtkUnstructuredGrid *ugrid = AllocateGetBlock(volumeOutput, tetInfo[0],
                                                    IS_INTERNAL_VOLUME());
      ugrid->InsertNextCell(VTK_TETRA, 4, tetInfo+1);
      }
    }

  // Read in exterior tetrahedra.
  if (!this->ReadTetrahedronExteriorArray(meshFD, connectivity)) return 0;
  vtkIdType numTetsExterior = connectivity->GetNumberOfTuples();
  for (vtkIdType i = 0; i < numTetsExterior; i++)
    {
    // Exterior tetrahedra are defined with 9 integers.  The first is an element
    // attribute and the next 4 are point ids, which is the same as interior
    // tetrahedra (see above).  The last 4 define the boundary condition of
    // each face (see above for the order of faces).  A flag of -1 is used
    // when the face is internal.  Other flags separate faces in a multiblock
    // data set.
    vtkIdType tetInfo[NumPerTetExt];
    connectivity->GetTupleValue(i, tetInfo);
    if (invertTets)
      {
      vtkstd::swap(tetInfo[1], tetInfo[2]); // Invert point indices
      vtkstd::swap(tetInfo[6], tetInfo[8]); // Correct faces for inversion
      }
    if (this->ReadInternalVolume)
      {
      vtkUnstructuredGrid *ugrid = AllocateGetBlock(volumeOutput, tetInfo[0],
                                                    IS_INTERNAL_VOLUME());
      ugrid->InsertNextCell(VTK_TETRA, 4, tetInfo+1);
      }

    if (this->ReadExternalSurface)
      {
      for (int face = 0; face < 4; face++)
        {
        int boundaryCondition = tetInfo[5+face];
        if (boundaryCondition >= 0)
          {
          vtkUnstructuredGrid *ugrid = AllocateGetBlock(surfaceOutput,
                                                        boundaryCondition,
                                                        IS_EXTERNAL_SURFACE());
          vtkIdType ptids[3];
          ptids[0] = tetInfo[1+tetFaces[face][0]];
          ptids[1] = tetInfo[1+tetFaces[face][1]];
          ptids[2] = tetInfo[1+tetFaces[face][2]];
          ugrid->InsertNextCell(VTK_TRIANGLE, 3, ptids);
          }
        }
      }
    }

  return 1;
}

//-----------------------------------------------------------------------------
vtkSmartPointer<vtkDataArray> vtkSLACReader::ReadPointDataArray(int ncFD,
                                                                int varId)
{
  // Get the dimension info.  We should only need to worry about 1 or 2D arrays.
  int numDims;
  CALL_NETCDF(nc_inq_varndims(ncFD, varId, &numDims));
  if (numDims > 2) // don't support 3d or higher arrays
    {
    vtkErrorMacro(<< "Sanity check failed.  "
                  << "Encountered array with too many dimensions.");
    return 0;
    }
  if (numDims < 1) // don't support 0d arrays
    {
    vtkErrorMacro(<< "Sanity check failed.  "
                  << "Encountered array no dimensions.");
    return 0;
    }
  int dimIds[2];
  CALL_NETCDF(nc_inq_vardimid(ncFD, varId, dimIds));
  size_t numCoords;
  CALL_NETCDF(nc_inq_dimlen(ncFD, dimIds[0], &numCoords));
  size_t numComponents = 1;
  if (numDims > 1)
    {
    CALL_NETCDF(nc_inq_dimlen(ncFD, dimIds[1], &numComponents));
    }

  // Allocate an array of the right type.
  nc_type ncType;
  CALL_NETCDF(nc_inq_vartype(ncFD, varId, &ncType));
  int vtkType = NetCDFTypeToVTKType(ncType);
  if (vtkType < 1) return 0;
  vtkSmartPointer<vtkDataArray> dataArray;
  dataArray.TakeReference(vtkDataArray::CreateDataArray(vtkType));
  dataArray->SetNumberOfComponents(static_cast<int>(numComponents));
  dataArray->SetNumberOfTuples(static_cast<vtkIdType>(numCoords));

  // Read the data from the file.
  size_t start[2], count[2];
  start[0] = start[1] = 0;
  count[0] = numCoords;  count[1] = numComponents;
  CALL_NETCDF(nc_get_vars(ncFD, varId, start, count, NULL,
                          dataArray->GetVoidPointer(0)));

  return dataArray;
}

//-----------------------------------------------------------------------------
int vtkSLACReader::ReadCoordinates(int meshFD, vtkMultiBlockDataSet *output)
{
  // Read in the point coordinates.  The coordinates are 3-tuples in an array
  // named "coords".
  int coordsVarId;
  CALL_NETCDF(nc_inq_varid(meshFD, "coords", &coordsVarId));

  vtkSmartPointer<vtkDataArray> coordData
    = this->ReadPointDataArray(meshFD, coordsVarId);
  if (!coordData) return 0;
  if (coordData->GetNumberOfComponents() != 3)
    {
    vtkErrorMacro(<< "Failed sanity check!  Coords have wrong dimensions.");
    return 0;
    }
  coordData->SetName("coords");

  vtkPoints *points = vtkPoints::SafeDownCast(
                        output->GetInformation()->Get(vtkSLACReader::POINTS()));
  points->SetData(coordData);

  return 1;
}

//-----------------------------------------------------------------------------
int vtkSLACReader::ReadFieldData(int modeFD, vtkMultiBlockDataSet *output)
{
  vtkPointData *pd = vtkPointData::SafeDownCast(
                    output->GetInformation()->Get(vtkSLACReader::POINT_DATA()));

  // Get the number of coordinates (which determines how many items are read
  // per variable).
  int ncoordDim;
  CALL_NETCDF(nc_inq_dimid(modeFD, "ncoord", &ncoordDim));
  size_t numCoords;
  CALL_NETCDF(nc_inq_dimlen(modeFD, ncoordDim, &numCoords));

  int numArrays = this->Internal->VariableArraySelection->GetNumberOfArrays();
  for (int arrayIndex = 0; arrayIndex < numArrays; arrayIndex++)
    {
    // skip array if not enabled
    if (!this->Internal->VariableArraySelection->GetArraySetting(arrayIndex))
      {
      continue;
      }

    // from the variable name, get the variable id
    const char * cname
      = this->Internal->VariableArraySelection->GetArrayName(arrayIndex);
    int varId;
    CALL_NETCDF(nc_inq_varid(modeFD, cname, &varId));

    vtkStdString name(cname);

    // if this variable isn't 1d or 2d array, skip it.
    int numDims;
    CALL_NETCDF(nc_inq_varndims(modeFD, varId, &numDims));
    if (numDims < 1 || numDims > 2)
      {
      vtkWarningMacro(<< "Encountered invalid variable dimensions.")
      continue;
      }

    // Read in the array data.
    vtkSmartPointer<vtkDataArray> dataArray
      = this->ReadPointDataArray(modeFD, varId);
    if (!dataArray) continue;

    // Check for imaginary component of mode data.
    if (this->FrequencyModes)
      {
      if (nc_inq_varid(modeFD, (name+"_imag").c_str(), &varId) == NC_NOERR)
        {
        // I am assuming here that the imaginary data (if it exists) has the
        // same dimensions as the real data.
        vtkSmartPointer<vtkDataArray> imagDataArray
          = this->ReadPointDataArray(modeFD, varId);
        if (imagDataArray)
          {
          // allocate space for complex magnitude data
          vtkSmartPointer<vtkDataArray> cplxMagArray; 
          cplxMagArray.TakeReference(vtkDataArray::CreateDataArray(VTK_DOUBLE));
          cplxMagArray->SetNumberOfComponents(1);
          cplxMagArray->SetNumberOfTuples(static_cast<vtkIdType>(numCoords));

          // allocate space for phase data
          vtkSmartPointer<vtkDataArray> phaseArray; 
          phaseArray.TakeReference(vtkDataArray::CreateDataArray(VTK_DOUBLE));
          phaseArray->SetNumberOfComponents(3);
          phaseArray->SetNumberOfTuples(static_cast<vtkIdType>(numCoords));

          int numComponents = dataArray->GetNumberOfComponents();
          vtkIdType numTuples = dataArray->GetNumberOfTuples();
          for (vtkIdType i = 0; i < numTuples; i++)
            {
            double accum_mag= 0.0;
            for (int j = 0; j < numComponents; j++)
              {
              double real = dataArray->GetComponent(i, j);
              double imag = imagDataArray->GetComponent(i, j);

              double mag2 = real*real + imag*imag;
              accum_mag += mag2;
              double mag= sqrt(mag2);

              double startphase = atan2(imag, real);
              dataArray->SetComponent(i, j, mag*cos(startphase + this->Phase));
              phaseArray->SetComponent(i, j, startphase);
              }
            cplxMagArray->SetComponent(i, 0, sqrt(accum_mag));
            phaseArray->SetComponent(i, 0, sqrt(accum_mag));
            }

          // add complex magnitude data to the point data
          vtkStdString cplxMagName= name + "_cplx_mag";
          cplxMagArray->SetName(cplxMagName);
          pd->AddArray(cplxMagArray);

          vtkStdString phaseName= name + "_phase";
          phaseArray->SetName(phaseName);
          pd->AddArray(phaseArray);
          }
        }
      }

    // Add the data to the point data.
    dataArray->SetName(name);
    pd->AddArray(dataArray);
    }

  return 1;
}

//-----------------------------------------------------------------------------
int vtkSLACReader::ReadMidpointCoordinates(
                                   int meshFD,
                                   vtkMultiBlockDataSet *output,
                                   vtkSLACReader::MidpointCoordinateMap &map)
{
  // Get the number of midpoints.
  int midpointsVar;
  CALL_NETCDF(nc_inq_varid(meshFD, "surface_midpoint", &midpointsVar));
  vtkIdType numMidpoints = this->GetNumTuplesInVariable(meshFD,midpointsVar,5);
  if (numMidpoints < 1) return 0;

  // Read in the raw data.
  VTK_CREATE(vtkDoubleArray, midpointData);
  midpointData->SetNumberOfComponents(5);
  midpointData->SetNumberOfTuples(numMidpoints);
  CALL_NETCDF(nc_get_var_double(meshFD, midpointsVar,
                                midpointData->GetPointer(0)));

  vtkPoints *points = vtkPoints::SafeDownCast(
                        output->GetInformation()->Get(vtkSLACReader::POINTS()));
  vtkIdType pointTotal = points->GetNumberOfPoints ();
  // Create a searchable structure.
  for (vtkIdType i = 0; i < numMidpoints; i++)
    {
    double *mp = midpointData->GetPointer(i*5);

    EdgeEndpoints edge(static_cast<vtkIdType>(mp[0]),
                       static_cast<vtkIdType>(mp[1]));
    MidpointCoordinates midpoint(mp+2, i+pointTotal);
    map.AddMidpoint(edge, midpoint);
    }

  return 1;
}

//-----------------------------------------------------------------------------
int vtkSLACReader::ReadMidpointData(int meshFD, vtkMultiBlockDataSet *output,
                                    MidpointIdMap &midpointIds)
{
  static bool GaveMidpointWarning = false;
  if (!GaveMidpointWarning)
    {
    vtkWarningMacro(<< "Quadratic elements not displayed entirely correctly yet.  Quadratic triangles are drawn as 4 linear triangles.");
    GaveMidpointWarning = true;
    }

  // Get the point information from the data.
  vtkPoints *points = vtkPoints::SafeDownCast(
                        output->GetInformation()->Get(vtkSLACReader::POINTS()));

  // Read in the midpoint coordinates.
  MidpointCoordinateMap midpointCoords;
  if (!this->ReadMidpointCoordinates(meshFD, output, midpointCoords)) return 0;

  vtkIdType newPointTotal
    = points->GetNumberOfPoints() + midpointCoords.GetNumberOfMidpoints();

  // Iterate over all of the parts in the output and visit the ones for the
  // external surface.
  VTK_CREATE(vtkCompositeDataIterator, outputIter);
  for (outputIter.TakeReference(output->NewIterator());
       !outputIter->IsDoneWithTraversal(); outputIter->GoToNextItem())
    {
    if (!output->GetMetaData(outputIter)->Get(IS_EXTERNAL_SURFACE())) continue;

    // Create a new cell array so that we can convert all the cells from
    // triangles to quadratic triangles.
    vtkUnstructuredGrid *ugrid
      = vtkUnstructuredGrid::SafeDownCast(output->GetDataSet(outputIter));
    vtkCellArray *oldCells = ugrid->GetCells();
    VTK_CREATE(vtkCellArray, newCells);
    newCells->Allocate(newCells->EstimateSize(oldCells->GetNumberOfCells(), 6));

    // Iterate over all of the cells.
    vtkIdType npts, *pts;
    for (oldCells->InitTraversal(); oldCells->GetNextCell(npts, pts); )
      {
      newCells->InsertNextCell(6);

      // Copy corner points.
      newCells->InsertCellPoint(pts[0]);
      newCells->InsertCellPoint(pts[1]);
      newCells->InsertCellPoint(pts[2]);

      // Add edge midpoints.
      for (int edgeInc = 0; edgeInc < 3; edgeInc++)
        {
        // Get the points defining the edge.
        vtkIdType p0 = pts[triEdges[edgeInc][0]];
        vtkIdType p1 = pts[triEdges[edgeInc][1]];
        EdgeEndpoints edge(p0, p1);

        // See if we have already copied this midpoint.
        vtkIdType midId;
        vtkIdType *midIdPointer = midpointIds.FindMidpoint(edge);
        if (midIdPointer != NULL)
          {
          midId = *midIdPointer;
          }
        else
          {
          // Check to see if the midpoint was read from the file.  If not,
          // then interpolate linearly between the two edge points.
          MidpointCoordinates midpoint;
          MidpointCoordinates *midpointPointer
            = midpointCoords.FindMidpoint(edge);
          if (midpointPointer == NULL)
            {
            double coord0[3], coord1[3], coordMid[3];
            points->GetPoint(p0, coord0);
            points->GetPoint(p1, coord1);
            coordMid[0] = 0.5*(coord0[0] + coord1[0]);
            coordMid[1] = 0.5*(coord0[1] + coord1[1]);
            coordMid[2] = 0.5*(coord0[2] + coord1[2]);
            midpoint = MidpointCoordinates(coordMid, newPointTotal);
            newPointTotal ++;
            }
          else
            {
            midpoint = *midpointPointer;
            // Erase the midpoint from the map.  We don't need it anymore since
            // we will insert a point id in the midpointIds map (see below).
            midpointCoords.RemoveMidpoint(edge);
            }

          // Add the new point to the point data.
          points->InsertPoint(midpoint.ID, midpoint.Coordinate);

          // Add the new point to the id map.
          midpointIds.AddMidpoint(edge, midpoint.ID);
          midId = midpoint.ID;
          }

        // Record the midpoint in the quadratic cell.
        newCells->InsertCellPoint(midId);
        }
      }

    // Save the new cells in the data.
    ugrid->SetCells(VTK_QUADRATIC_TRIANGLE, newCells);
    }

  return 1;
}

//-----------------------------------------------------------------------------
int vtkSLACReader::InterpolateMidpointData(vtkMultiBlockDataSet *output,
                                           vtkSLACReader::MidpointIdMap &map)
{
  // Get the point information from the output data (where it was placed
  // earlier).
  vtkPoints *points = vtkPoints::SafeDownCast(
                        output->GetInformation()->Get(vtkSLACReader::POINTS()));
  vtkPointData *pd = vtkPointData::SafeDownCast(
                    output->GetInformation()->Get(vtkSLACReader::POINT_DATA()));
  if (!pd)
    {
    vtkWarningMacro(<< "Missing point data.");
    return 0;
    }

  // Set up the point data for adding new points and interpolating their values.
  pd->InterpolateAllocate(pd, points->GetNumberOfPoints());

  EdgeEndpoints edge;
  vtkIdType midpoint;
  for (map.InitTraversal(); map.GetNextMidpoint(edge, midpoint); )
    {
    pd->InterpolateEdge(pd, midpoint,
                        edge.GetMinEndPoint(), edge.GetMaxEndPoint(), 0.5);
    }

  return 1;
}

//-----------------------------------------------------------------------------
int vtkSLACReader::MeshUpToDate()
{
  if (this->MeshReadTime < this->GetMTime())
    {
    return 0;
    }
  if (this->MeshReadTime < this->Internal->VariableArraySelection->GetMTime())
    {
    return 0;
    }
  return 1;
}

//-----------------------------------------------------------------------------
int vtkSLACReader::RestoreMeshCache(vtkMultiBlockDataSet *surfaceOutput,
                                    vtkMultiBlockDataSet *volumeOutput,
                                    vtkMultiBlockDataSet *compositeOutput)
{
  surfaceOutput->ShallowCopy(
                           this->Internal->MeshCache->GetBlock(SURFACE_OUTPUT));
  volumeOutput->ShallowCopy(this->Internal->MeshCache->GetBlock(VOLUME_OUTPUT));

  // Shove two outputs in composite output.
  compositeOutput->SetNumberOfBlocks(2);
  compositeOutput->SetBlock(SURFACE_OUTPUT, surfaceOutput);
  compositeOutput->SetBlock(VOLUME_OUTPUT,  volumeOutput);
  compositeOutput->GetMetaData(static_cast<unsigned int>(SURFACE_OUTPUT))
    ->Set(vtkCompositeDataSet::NAME(), "Internal Volume");
  compositeOutput->GetMetaData(static_cast<unsigned int>(VOLUME_OUTPUT))
    ->Set(vtkCompositeDataSet::NAME(), "External Surface");

  compositeOutput->GetInformation()->Set(vtkSLACReader::POINTS(),
                                         this->Internal->PointCache);

  VTK_CREATE(vtkPointData, pd);
  compositeOutput->GetInformation()->Set(vtkSLACReader::POINT_DATA(), pd);

  return 1;
}
