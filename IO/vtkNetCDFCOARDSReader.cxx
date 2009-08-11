// -*- c++ -*-
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkNetCDFCOARDSReader.cxx

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

#include "vtkNetCDFCOARDSReader.h"

#include "vtkDataArraySelection.h"
#include "vtkDoubleArray.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkIntArray.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPoints.h"
#include "vtkRectilinearGrid.h"
#include "vtkStdString.h"
#include "vtkStructuredGrid.h"

#include "vtkSmartPointer.h"
#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

#include <string.h>

#include <netcdf.h>

#define CALL_NETCDF_GENERIC(call, on_error) \
  { \
    int errorcode = call; \
    if (errorcode != NC_NOERR) \
      { \
      const char * errorstring = nc_strerror(errorcode); \
      on_error; \
      } \
  }

#define CALL_NETCDF(call) \
  CALL_NETCDF_GENERIC(call, vtkErrorMacro(<< "netCDF Error: " << errorstring); return 0;)

#define CALL_NETCDF_GW(call) \
  CALL_NETCDF_GENERIC(call, vtkGenericWarningMacro(<< "netCDF Error: " << errorstring); return 0;)

#include <vtkstd/algorithm>

#include <math.h>

//=============================================================================
vtkNetCDFCOARDSReader::vtkDimensionInfo::vtkDimensionInfo(int ncFD, int id)
{
  this->DimId = id;
  this->LoadMetaData(ncFD);
}

int vtkNetCDFCOARDSReader::vtkDimensionInfo::LoadMetaData(int ncFD)
{
  this->Units = UNDEFINED_UNITS;

  char name[NC_MAX_NAME+1];
  CALL_NETCDF_GW(nc_inq_dimname(ncFD, this->DimId, name));
  this->Name = name;

  size_t dimLen;
  CALL_NETCDF_GW(nc_inq_dimlen(ncFD, this->DimId, &dimLen));
  this->Coordinates = vtkSmartPointer<vtkDoubleArray>::New();
//   this->Coordinates->SetName((this->Name + "_Coordinates").c_str());
  this->Coordinates->SetNumberOfComponents(1);
  this->Coordinates->SetNumberOfTuples(dimLen);

  int varId;
  int varNumDims;
  int varDim;
  // By convention if there is a single dimension variable with the same name as
  // its dimension, then the data contains the coordinates for the dimension.
  if (   (nc_inq_varid(ncFD, name, &varId) == NC_NOERR)
      && (nc_inq_varndims(ncFD, varId, &varNumDims) == NC_NOERR)
      && (varNumDims == 1)
      && (nc_inq_vardimid(ncFD, varId, &varDim) == NC_NOERR)
      && (varDim == this->DimId) )
    {
    // Read coordinates
    CALL_NETCDF_GW(nc_get_var_double(ncFD, varId,
                                     this->Coordinates->GetPointer(0)));

    // Check to see if the spacing is regular.
    this->Origin = this->Coordinates->GetValue(0);
    this->Spacing
      = (this->Coordinates->GetValue(dimLen-1) - this->Origin)/(dimLen-1);
    this->HasRegularSpacing = true;     // Then check to see if it is false.
    double tolerance = 0.01*this->Spacing;
    for (size_t i = 1; i < dimLen; i++)
      {
      double expectedValue = this->Origin + i*this->Spacing;
      double actualValue = this->Coordinates->GetValue(i);
      if (   (actualValue < expectedValue-tolerance)
          || (actualValue > expectedValue+tolerance) )
        {
        this->HasRegularSpacing = false;
        break;
        }
      }

    // Check units.
    size_t unitsLength;
    if (nc_inq_attlen(ncFD, varId, "units", &unitsLength) == NC_NOERR)
      {
      vtkStdString units;
      units.resize(unitsLength);        // Note: don't need terminating char.
      CALL_NETCDF_GW(nc_get_att_text(ncFD, varId, "units", &units.at(0)));
      // Time, latitude, and longitude dimensions are those with units that
      // correspond to strings formatted with the Unidata udunits package.  I'm
      // not sure if these checks are complete, but they matches all of the
      // examples I have seen.
      if (units.find(" since ") != vtkStdString::npos)
        {
        this->Units = TIME_UNITS;
        }
      else if (units.find("degrees") != vtkStdString::npos)
        {
        this->Units = DEGREE_UNITS;
        }
      }
    }
  else
    {
    // Fake coordinates
    for (size_t i = 0; i < dimLen; i++)
      {
      this->Coordinates->SetValue(i, static_cast<double>(i));
      }
    this->HasRegularSpacing = true;
    this->Origin = 0.0;
    this->Spacing = 1.0;
    }
  return 1;
}


//=============================================================================
vtkCxxRevisionMacro(vtkNetCDFCOARDSReader, "1.3.2.1");
vtkStandardNewMacro(vtkNetCDFCOARDSReader);

//-----------------------------------------------------------------------------
vtkNetCDFCOARDSReader::vtkNetCDFCOARDSReader()
{
  this->SphericalCoordinates = 1;
}

vtkNetCDFCOARDSReader::~vtkNetCDFCOARDSReader()
{
}

void vtkNetCDFCOARDSReader::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "SphericalCoordinates: " << this->SphericalCoordinates <<endl;
}

//-----------------------------------------------------------------------------
int vtkNetCDFCOARDSReader::CanReadFile(const char *filename)
{
  // We really just read basic arrays from netCDF files.  If the netCDF library
  // says we can read it, then we can read it.
  int ncFD;
  int errorcode = nc_open(filename, NC_NOWRITE, &ncFD);
  if (errorcode == NC_NOERR)
    {
    nc_close(ncFD);
    return 1;
    }
  else
    {
    return 0;
    }
}

//-----------------------------------------------------------------------------
int vtkNetCDFCOARDSReader::RequestDataObject(
                                 vtkInformation *vtkNotUsed(request),
                                 vtkInformationVector **vtkNotUsed(inputVector),
                                 vtkInformationVector *outputVector)
{
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  vtkDataObject *output = vtkDataObject::GetData(outInfo);

  // This is really too early to know the appropriate data type.  We need to
  // have meta data and let the user select arrays.  We have to do part
  // of the RequestInformation to get the appropriate meta data.
  if (!this->UpdateMetaData()) return 0;

  int dataType = VTK_IMAGE_DATA;

  int ncFD;
  CALL_NETCDF(nc_open(this->FileName, NC_NOWRITE, &ncFD));

  int numArrays = this->VariableArraySelection->GetNumberOfArrays();
  for (int arrayIndex = 0; arrayIndex < numArrays; arrayIndex++)
    {
    if (!this->VariableArraySelection->GetArraySetting(arrayIndex)) continue;

    const char *name = this->VariableArraySelection->GetArrayName(arrayIndex);
    int varId;
    CALL_NETCDF(nc_inq_varid(ncFD, name, &varId));

    int currentNumDims;
    CALL_NETCDF(nc_inq_varndims(ncFD, varId, &currentNumDims));
    if (currentNumDims < 1) continue;
    VTK_CREATE(vtkIntArray, currentDimensions);
    currentDimensions->SetNumberOfComponents(1);
    currentDimensions->SetNumberOfTuples(currentNumDims);
    CALL_NETCDF(nc_inq_vardimid(ncFD, varId,
                                currentDimensions->GetPointer(0)));

    // Remove initial time dimension, which has no effect on data type.
    if (this->IsTimeDimension(ncFD, currentDimensions->GetValue(0)))
      {
      currentDimensions->RemoveTuple(0);
      currentNumDims--;
      if (currentNumDims < 1) continue;
      }

    // Check to see if the dimensions fit spherical coordinates.
    if (   this->SphericalCoordinates
        && (currentNumDims == 3)
        && (   this->DimensionInfo[currentDimensions->GetValue(1)].GetUnits()
            == vtkDimensionInfo::DEGREE_UNITS)
        && (   this->DimensionInfo[currentDimensions->GetValue(2)].GetUnits()
            == vtkDimensionInfo::DEGREE_UNITS) )
      {
      dataType = VTK_STRUCTURED_GRID;
      break;
      }

    // Check to see if any dimension as irregular spacing.
    for (int i = 0; i < currentNumDims; i++)
      {
      int dimId = currentDimensions->GetValue(i);
      if (!this->DimensionInfo[dimId].GetHasRegularSpacing())
        {
        dataType = VTK_RECTILINEAR_GRID;
        break;
        }
      }

    break;
    }

  if (dataType == VTK_IMAGE_DATA)
    {
    if (!output || !output->IsA("vtkImageData"))
      {
      output = vtkImageData::New();
      output->SetPipelineInformation(outInfo);
      output->Delete();   // Not really deleted.
      }
    }
  else if (dataType == VTK_RECTILINEAR_GRID)
    {
    if (!output || !output->IsA("vtkRectilinearGrid"))
      {
      output = vtkRectilinearGrid::New();
      output->SetPipelineInformation(outInfo);
      output->Delete();   // Not really deleted.
      }
    }
  else // dataType == VTK_STRUCTURED_GRID
    {
    if (!output || !output->IsA("vtkStructuredGrid"))
      {
      output = vtkStructuredGrid::New();
      output->SetPipelineInformation(outInfo);
      output->Delete();   // Not really deleted.
      }
    }

  return 1;
}

//-----------------------------------------------------------------------------
int vtkNetCDFCOARDSReader::RequestData(vtkInformation *request,
                                       vtkInformationVector **inputVector,
                                       vtkInformationVector *outputVector)
{
  // Let the superclass do the heavy lifting.
  if (!this->Superclass::RequestData(request, inputVector, outputVector))
    {
    return 0;
    }

  // Add spacing information defined by the COARDS conventions.

  vtkImageData *imageOutput = vtkImageData::GetData(outputVector);
  if (imageOutput)
    {
    double origin[3];
    origin[0] = origin[1] = origin[2] = 0.0;
    double spacing[3];
    spacing[0] = spacing[1] = spacing[2] = 1.0;

    int numDim = this->LoadingDimensions->GetNumberOfTuples();
    if (numDim >= 3) numDim = 3;

    for (int i = 0; i < numDim; i++)
      {
      // Remember that netCDF dimension ordering is backward from VTK.
      int dim = this->LoadingDimensions->GetValue(numDim-i-1);
      origin[i] = this->DimensionInfo[dim].GetOrigin();
      spacing[i] = this->DimensionInfo[dim].GetSpacing();
      }
    }

  vtkRectilinearGrid *rectOutput = vtkRectilinearGrid::GetData(outputVector);
  if (rectOutput)
    {
    int extent[6];
    rectOutput->GetExtent(extent);

    int numDim = this->LoadingDimensions->GetNumberOfTuples();
    for (int i = 0; i < 3; i++)
      {
      vtkSmartPointer<vtkDoubleArray> coords;
      if (i < numDim)
        {
        // Remember that netCDF dimension ordering is backward from VTK.
        int dim = this->LoadingDimensions->GetValue(numDim-i-1);
        coords = this->DimensionInfo[dim].GetCoordinates();
        int extLow = extent[2*i];
        int extHi = extent[2*i+1];
        if ((extLow != 0) || (extHi != coords->GetNumberOfTuples()-1))
          {
          // Getting a subset of this dimension.
          VTK_CREATE(vtkDoubleArray, newcoords);
          newcoords->SetNumberOfComponents(1);
          newcoords->SetNumberOfTuples(extHi-extLow+1);
          memcpy(newcoords->GetPointer(0), coords->GetPointer(extLow),
                 (extHi-extLow+1)*sizeof(double));
          coords = newcoords;
          }
        }
      else
        {
        coords = vtkSmartPointer<vtkDoubleArray>::New();
        coords->SetNumberOfTuples(1);
        coords->SetComponent(0, 0, 0.0);
        }
      switch (i)
        {
        case 0: rectOutput->SetXCoordinates(coords);  break;
        case 1: rectOutput->SetYCoordinates(coords);  break;
        case 2: rectOutput->SetZCoordinates(coords);  break;
        }
      }
    }

  vtkStructuredGrid *structOutput = vtkStructuredGrid::GetData(outputVector);
  if (structOutput)
    {
    int extent[6];
    structOutput->GetExtent(extent);

    vtkDoubleArray *longCoords = this->DimensionInfo[this->LoadingDimensions->GetValue(2)].GetCoordinates();
    vtkDoubleArray *latCoords = this->DimensionInfo[this->LoadingDimensions->GetValue(1)].GetCoordinates();
    vtkDoubleArray *heightCoords = this->DimensionInfo[this->LoadingDimensions->GetValue(0)].GetCoordinates();

    VTK_CREATE(vtkPoints, points);
    points->SetDataTypeToDouble();
    points->Allocate(  (extent[1]-extent[0]+1)
                     * (extent[3]-extent[2]+1)
                     * (extent[5]-extent[4]+1) );

    for (int k = extent[4]; k <= extent[5]; k++)
      {
      double height = heightCoords->GetValue(k);

      for (int j = extent[2]; j <= extent[3]; j++)
        {
        double phi    = vtkMath::RadiansFromDegrees(latCoords->GetValue(j));

        for (int i = extent[0]; i <= extent[1]; i++)
          {
          double theta  = vtkMath::RadiansFromDegrees(longCoords->GetValue(i));

          double cartesianCoord[3];
          cartesianCoord[0] = height*cos(theta)*cos(phi);
          cartesianCoord[1] = height*sin(theta)*cos(phi);
          cartesianCoord[2] = height*sin(phi);
          points->InsertNextPoint(cartesianCoord);
          }
        }
      }

    structOutput->SetPoints(points);
    }

  return 1;
}

//-----------------------------------------------------------------------------
int vtkNetCDFCOARDSReader::ReadMetaData(int ncFD)
{
  int i;

  vtkDebugMacro("ReadMetaData");

  int numDimensions;
  CALL_NETCDF(nc_inq_ndims(ncFD, &numDimensions));
  this->DimensionInfo.resize(numDimensions);

  for (i = 0; i < numDimensions; i++)
    {
    this->DimensionInfo[i] = vtkDimensionInfo(ncFD, i);
    }

  // Look at all variables and record them so that the user can select
  // which ones he wants.
  this->VariableArraySelection->RemoveAllArrays();

  int numVariables;
  CALL_NETCDF(nc_inq_nvars(ncFD, &numVariables));

  for (i = 0; i < numVariables; i++)
    {
    char name[NC_MAX_NAME+1];
    CALL_NETCDF(nc_inq_varname(ncFD, i, name));
    int dimId;
    if (nc_inq_dimid(ncFD, name, &dimId) == NC_NOERR)
      {
      // This is a special variable that just tells us information about
      // a particular dimension.
      }
    else
      {
      // This is a real variable we want to expose.
      this->VariableArraySelection->AddArray(name);
      }
    }

  return 1;
}

//-----------------------------------------------------------------------------
int vtkNetCDFCOARDSReader::IsTimeDimension(int vtkNotUsed(ncFD), int dimId)
{
  return this->DimensionInfo[dimId].GetUnits() == vtkDimensionInfo::TIME_UNITS;
}

//-----------------------------------------------------------------------------
vtkSmartPointer<vtkDoubleArray> vtkNetCDFCOARDSReader::GetTimeValues(
                                                int vtkNotUsed(ncFD), int dimId)
{
  return this->DimensionInfo[dimId].GetCoordinates();
}
