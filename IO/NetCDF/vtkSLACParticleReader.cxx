// -*- c++ -*-
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSLACParticleReader.cxx

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

#include "vtkSLACParticleReader.h"

#include "vtkCallbackCommand.h"
#include "vtkCellArray.h"
#include "vtkCompositeDataIterator.h"
#include "vtkDataArraySelection.h"
#include "vtkDoubleArray.h"
#include "vtkIdTypeArray.h"
#include "vtkIntArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnstructuredGrid.h"

#include "vtkSmartPointer.h"
#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

#include "vtk_netcdf.h"

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

#ifdef VTK_USE_64BIT_IDS
#ifdef NC_INT64
// This may or may not work with the netCDF 4 library reading in netCDF 3 files.
#define nc_get_vars_vtkIdType nc_get_vars_longlong
#else // NC_INT64
static int nc_get_vars_vtkIdType(int ncid, int varid, const size_t start[],
                                 const size_t count[], const ptrdiff_t stride[],
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
#define nc_get_vars_vtkIdType nc_get_vars_int
#endif // VTK_USE_64BIT_IDS

//=============================================================================
#define MY_MIN(x, y)    ((x) < (y) ? (x) : (y))
#define MY_MAX(x, y)    ((x) < (y) ? (y) : (x))

// //=============================================================================
// static int NetCDFTypeToVTKType(nc_type type)
// {
//   switch (type)
//     {
//     case NC_BYTE: return VTK_UNSIGNED_CHAR;
//     case NC_CHAR: return VTK_CHAR;
//     case NC_SHORT: return VTK_SHORT;
//     case NC_INT: return VTK_INT;
//     case NC_FLOAT: return VTK_FLOAT;
//     case NC_DOUBLE: return VTK_DOUBLE;
//     default:
//       vtkGenericWarningMacro(<< "Unknown netCDF variable type "
//                              << type);
//       return -1;
//     }
// }

//=============================================================================
// This class automatically closes a netCDF file descripter when it goes out
// of scope.  This allows us to exit on error without having to close the
// file at every instance.
class vtkSLACParticleReaderAutoCloseNetCDF
{
public:
  vtkSLACParticleReaderAutoCloseNetCDF(const char *filename, int omode,
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
  ~vtkSLACParticleReaderAutoCloseNetCDF() {
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
  vtkSLACParticleReaderAutoCloseNetCDF();       // Not implemented
  vtkSLACParticleReaderAutoCloseNetCDF(const vtkSLACParticleReaderAutoCloseNetCDF &); // Not implemented
  void operator=(const vtkSLACParticleReaderAutoCloseNetCDF &); // Not implemented
};

//=============================================================================
vtkStandardNewMacro(vtkSLACParticleReader);

//-----------------------------------------------------------------------------
vtkSLACParticleReader::vtkSLACParticleReader()
{
  this->SetNumberOfInputPorts(0);

  this->FileName = NULL;
}

vtkSLACParticleReader::~vtkSLACParticleReader()
{
  this->SetFileName(NULL);
}

void vtkSLACParticleReader::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  if (this->FileName)
  {
    os << indent << "FileName: " << this->FileName << endl;
  }
  else
  {
    os << indent << "FileName: (null)\n";
  }
}

//-----------------------------------------------------------------------------
int vtkSLACParticleReader::CanReadFile(const char *filename)
{
  vtkSLACParticleReaderAutoCloseNetCDF ncFD(filename, NC_NOWRITE, true);
  if (!ncFD.Valid()) return 0;

  // Check for the existence of several arrays we know should be in the file.
  int dummy;
  if (nc_inq_varid(ncFD(), "particlePos",&dummy) != NC_NOERR) return 0;
  if (nc_inq_varid(ncFD(), "particleInfo", &dummy) != NC_NOERR) return 0;
  if (nc_inq_varid(ncFD(), "time", &dummy) != NC_NOERR) return 0;

  return 1;
}

//-----------------------------------------------------------------------------
vtkIdType vtkSLACParticleReader::GetNumTuplesInVariable(int ncFD, int varId,
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
int vtkSLACParticleReader::RequestInformation(
                                 vtkInformation *vtkNotUsed(request),
                                 vtkInformationVector **vtkNotUsed(inputVector),
                                 vtkInformationVector *outputVector)
{
  if (!this->FileName)
    {
    vtkErrorMacro("No filename specified.");
    return 0;
    }

  vtkSLACParticleReaderAutoCloseNetCDF ncFD(this->FileName, NC_NOWRITE);
  if (!ncFD.Valid()) return 0;

  int timeVar;
  CALL_NETCDF(nc_inq_varid(ncFD(), "time", &timeVar));
  double timeValue;
  CALL_NETCDF(nc_get_var_double(ncFD(), timeVar, &timeValue));

  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(), &timeValue, 1);
  double timeRange[2];
  timeRange[0] = timeRange[1] = timeValue;
  outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(), timeRange, 2);

  // Report that we support any number of pieces (but we are only really going
  // to load anything for piece 0).
  outInfo->Set(vtkStreamingDemandDrivenPipeline::MAXIMUM_NUMBER_OF_PIECES(), -1);

  return 1;
}

//-----------------------------------------------------------------------------
int vtkSLACParticleReader::RequestData(vtkInformation *vtkNotUsed(request),
                               vtkInformationVector **vtkNotUsed(inputVector),
                               vtkInformationVector *outputVector)
{
  vtkPolyData *output = vtkPolyData::GetData(outputVector);

  if (!this->FileName)
    {
    vtkErrorMacro("No filename specified.");
    return 0;
    }

  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  int requestedPiece = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  if (requestedPiece != 0)
    {
    // Return empty data for all but piece 0.
    return 1;
    }

  vtkSLACParticleReaderAutoCloseNetCDF ncFD(this->FileName, NC_NOWRITE);
  if (!ncFD.Valid()) return 0;

  VTK_CREATE(vtkPoints, points);

  int particlePosVar;
  CALL_NETCDF(nc_inq_varid(ncFD(), "particlePos", &particlePosVar));
  vtkIdType numParticles = this->GetNumTuplesInVariable(ncFD(), particlePosVar, 6);

  size_t start[2], count[2];
  start[0] = 0; count[0] = numParticles;
  start[1] = 0; count[1] = 3;

  VTK_CREATE(vtkDoubleArray, coords);
  coords->SetNumberOfComponents(3);
  coords->SetNumberOfTuples(numParticles);
  CALL_NETCDF(nc_get_vars_double(ncFD(), particlePosVar, start, count, NULL,
              coords->GetPointer(0)));
  points->SetData(coords);
  output->SetPoints(points);

  VTK_CREATE(vtkDoubleArray, momentum);
  momentum->SetName("Momentum");
  momentum->SetNumberOfComponents(3);
  momentum->SetNumberOfTuples(numParticles);
  start[1] = 3;
  CALL_NETCDF(nc_get_vars_double(ncFD(), particlePosVar, start, count, NULL,
              momentum->GetPointer(0)));
  output->GetPointData()->AddArray(momentum);

  int particleInfoVar;
  CALL_NETCDF(nc_inq_varid(ncFD(), "particleInfo", &particleInfoVar));
  start[1] = 0;  count[1] = 1;

  VTK_CREATE(vtkIdTypeArray, ids);
  ids->SetName("ParticleIds");
  ids->SetNumberOfComponents(1);
  ids->SetNumberOfTuples(numParticles);
  CALL_NETCDF(nc_get_vars_vtkIdType(ncFD(), particleInfoVar, start, count, NULL,
              ids->GetPointer(0)));
  output->GetPointData()->SetGlobalIds(ids);

  VTK_CREATE(vtkIntArray, emissionType);
  emissionType->SetName("EmissionType");
  emissionType->SetNumberOfComponents(1);
  emissionType->SetNumberOfTuples(numParticles);
  start[1] = 1;
  CALL_NETCDF(nc_get_vars_int(ncFD(), particleInfoVar, start, count, NULL,
              emissionType->GetPointer(0)));
  output->GetPointData()->AddArray(emissionType);

  VTK_CREATE(vtkCellArray, verts);
  verts->Allocate(verts->EstimateSize(numParticles, 1));
  for (vtkIdType i = 0; i < numParticles; i++)
    {
    verts->InsertNextCell(1, &i);
    }
  output->SetVerts(verts);

  int timeVar;
  CALL_NETCDF(nc_inq_varid(ncFD(), "time", &timeVar));
  double timeValue;
  CALL_NETCDF(nc_get_var_double(ncFD(), timeVar, &timeValue));
  output->GetInformation()->Set(vtkDataObject::DATA_TIME_STEP(),  timeValue);

  return 1;
}

