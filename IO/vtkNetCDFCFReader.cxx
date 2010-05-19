// -*- c++ -*-
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkNetCDFCFReader.cxx

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

#include "vtkNetCDFCFReader.h"

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
#include "vtkStringArray.h"
#include "vtkStructuredGrid.h"

#include "vtkSmartPointer.h"
#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

#include <vtkstd/set>
#include <vtksys/RegularExpression.hxx>
#include <vtksys/SystemTools.hxx>

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
// Convenience function for getting the text attribute on a variable.  Returns
// true if the attribute exists, false otherwise.
static bool ReadTextAttribute(int ncFD, int varId, const char *name,
                              vtkStdString &result)
{
  size_t length;
  if (nc_inq_attlen(ncFD, varId, name, &length) != NC_NOERR) return false;

  result.resize(length);
  if (nc_get_att_text(ncFD,varId,name,&result.at(0)) != NC_NOERR) return false;

  // The line below seems weird, but it is here for a good reason.  In general,
  // text attributes are not null terminated, so you have to add your own (which
  // the vtkStdString will do for us).  However, sometimes a null terminating
  // character is written in the attribute anyway.  In a C string this is no big
  // deal.  But it means that the vtkStdString has a null character in it and it
  // is technically different than its own C string.  This line corrects that
  // regardless of whether the null string was written we will get the right
  // string.
  result = result.c_str();

  return true;
}

//=============================================================================
vtkNetCDFCFReader::vtkDimensionInfo::vtkDimensionInfo(int ncFD, int id)
{
  this->DimId = id;
  this->LoadMetaData(ncFD);
}

int vtkNetCDFCFReader::vtkDimensionInfo::LoadMetaData(int ncFD)
{
  this->Units = UNDEFINED_UNITS;

  char name[NC_MAX_NAME+1];
  CALL_NETCDF_GW(nc_inq_dimname(ncFD, this->DimId, name));
  this->Name = name;

  size_t dimLen;
  CALL_NETCDF_GW(nc_inq_dimlen(ncFD, this->DimId, &dimLen));
  this->Coordinates = vtkSmartPointer<vtkDoubleArray>::New();
  this->Coordinates->SetName((this->Name + "_Coordinates").c_str());
  this->Coordinates->SetNumberOfComponents(1);
  this->Coordinates->SetNumberOfTuples(dimLen);

  this->Bounds = vtkSmartPointer<vtkDoubleArray>::New();
  this->Bounds->SetName((this->Name + "_Bounds").c_str());
  this->Bounds->SetNumberOfComponents(1);
  this->Bounds->SetNumberOfTuples(dimLen+1);

  this->SpecialVariables = vtkSmartPointer<vtkStringArray>::New();

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
    this->SpecialVariables->InsertNextValue(name);

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
    vtkStdString units;
    if (ReadTextAttribute(ncFD, varId, "units", units))
      {
      units = vtksys::SystemTools::LowerCase(units);
      // Time, latitude, and longitude dimensions are those with units that
      // correspond to strings formatted with the Unidata udunits package.  I'm
      // not sure if these checks are complete, but they matches all of the
      // examples I have seen.
      if (units.find(" since ") != vtkStdString::npos)
        {
        this->Units = TIME_UNITS;
        }
      else if (vtksys::RegularExpression("degrees?_?n").find(units))
        {
        this->Units = LATITUDE_UNITS;
        }
      else if (vtksys::RegularExpression("degrees?_?e").find(units))
        {
        this->Units = LONGITUDE_UNITS;
        }
      }

    // Check axis.
    vtkStdString axis;
    if (ReadTextAttribute(ncFD, varId, "axis", axis))
      {
      // The axis attribute is an alternate way of defining the coordinate type.
      // The string can be "X", "Y", "Z", or "T" which mean longitude, latitude,
      // vertical, and time, respectively.
      if (axis == "X")
        {
        this->Units = LONGITUDE_UNITS;
        }
      else if (axis == "Y")
        {
        this->Units = LATITUDE_UNITS;
        }
      else if (axis == "Z")
        {
        this->Units = VERTICAL_UNITS;
        }
      else if (axis == "T")
        {
        this->Units = TIME_UNITS;
        }
      }

    // Check positive.
    vtkStdString positive;
    if (ReadTextAttribute(ncFD, varId, "positive", positive))
      {
      positive = vtksys::SystemTools::LowerCase(positive);
      if (positive.find("down") != vtkStdString::npos)
        {
        // Flip the values of the coordinates.
        for (vtkIdType i = 0; i < this->Coordinates->GetNumberOfTuples(); i++)
          {
          this->Coordinates->SetValue(i, -(this->Coordinates->GetValue(i)));
          }
        this->Spacing = -this->Spacing;
        }
      }

    // Create the bounds array, which is used in place of the coordinates when
    // loading as cell data.  We will look for the bounds attribute on the
    // description variable to see if cell bounds have been written out.  This
    // code assumes that if such an attribute exists, it is the name of another
    // variable that is of dimensions of size dimLen X 2.  There are no checks
    // for this (other than the existence of the attribute), so if this is not
    // the case then the code could fail.
    vtkStdString boundsName;
    if (ReadTextAttribute(ncFD, varId, "bounds", boundsName))
      {
      this->SpecialVariables->InsertNextValue(boundsName);

      int boundsVarId;
      CALL_NETCDF_GW(nc_inq_varid(ncFD, boundsName.c_str(), &boundsVarId));

      // Read in the first bound value for each entry as a point bound.  If the
      // cells are connected, the second bound value should equal the first
      // bound value of the next entry anyway.
      size_t start[2];  start[0] = start[1] = 0;
      size_t count[2];  count[0] = dimLen;  count[1] = 1;
      CALL_NETCDF_GW(nc_get_vars_double(ncFD, boundsVarId, start, count, NULL,
                                        this->Bounds->GetPointer(0)));

      // Read in the last value for the bounds array.  It will be the second
      // bound in the last entry.  This will not be replicated unless the
      // dimension is a longitudinal one that wraps all the way around.
      start[0] = dimLen-1;  start[1] = 1;
      count[0] = 1;  count[1] = 1;
      CALL_NETCDF_GW(nc_get_vars_double(ncFD, boundsVarId, start, count, NULL,
                                        this->Bounds->GetPointer(dimLen)));
      }
    else
      {
      // Bounds not given.  Set them based on the coordinates.
      this->Bounds->SetValue(
                         0, this->Coordinates->GetValue(0) - 0.5*this->Spacing);
      for (vtkIdType i = 1; i < static_cast<vtkIdType>(dimLen); i++)
        {
        double v0 = this->Coordinates->GetValue(i-1);
        double v1 = this->Coordinates->GetValue(i);
        this->Bounds->SetValue(i, 0.5*(v0+v1));
        }
      this->Bounds->SetValue(dimLen,
                       this->Coordinates->GetValue(dimLen-1)+0.5*this->Spacing);
      }
    }
  else
    {
    // Fake coordinates
    for (size_t i = 0; i < dimLen; i++)
      {
      this->Coordinates->SetValue(i, static_cast<double>(i));
      this->Bounds->SetValue(i, static_cast<double>(i) - 0.5);
      }
    this->Bounds->SetValue(dimLen, static_cast<double>(dimLen) - 0.5);
    this->HasRegularSpacing = true;
    this->Origin = 0.0;
    this->Spacing = 1.0;
    }

  return 1;
}

//-----------------------------------------------------------------------------
class vtkNetCDFCFReader::vtkDimensionInfoVector
{
public:
  vtkstd::vector<vtkDimensionInfo> v;
};

//=============================================================================
vtkNetCDFCFReader::vtkDependentDimensionInfo::vtkDependentDimensionInfo(
                                                  int ncFD, int varId,
                                                  vtkNetCDFCFReader *parent)
{
  if (this->LoadMetaData(ncFD, varId, parent))
    {
    this->Valid = true;
    }
  else
    {
    this->Valid = false;
    }
}

//-----------------------------------------------------------------------------
int vtkNetCDFCFReader::vtkDependentDimensionInfo::LoadMetaData(
                                                  int ncFD, int varId,
                                                  vtkNetCDFCFReader *parent)
{
  int longitudeCoordVarId, latitudeCoordVarId;
  int longitudeBoundsVarId, latitudeBoundsVarId;
  longitudeCoordVarId = latitudeCoordVarId = -1;
  longitudeBoundsVarId = latitudeBoundsVarId = -1;
  this->GridDimensions = vtkSmartPointer<vtkIntArray>::New();
  this->SpecialVariables = vtkSmartPointer<vtkStringArray>::New();

  // The grid dimensions are the dimensions on the variable.  Since multiple
  // variables can be put on the same grid and this class identifies grids by
  // their variables, I group all of the dimension combinations together for 2D
  // coordinate lookup.  Technically, the CF specification allows you to have
  // specify different coordinate variables, but we do not support that because
  // there is no easy way to differentiate grids with the same dimensions.  If
  // different coordinates are needed, then duplicate dimensions should be
  // created.  Anyone who disagrees should write their own class.
  int numGridDimensions;
  CALL_NETCDF_GW(nc_inq_varndims(ncFD, varId, &numGridDimensions));
  if (numGridDimensions < 2) return 0;
  this->GridDimensions->SetNumberOfTuples(numGridDimensions);
  CALL_NETCDF_GW(nc_inq_vardimid(ncFD, varId,
                                 this->GridDimensions->GetPointer(0)));

  // Remove initial time dimension, which has no effect on data type.
  if (parent->IsTimeDimension(ncFD, this->GridDimensions->GetValue(0)))
    {
    this->GridDimensions->RemoveTuple(0);
    numGridDimensions--;
    if (numGridDimensions < 2) return 0;
    }

  vtkStdString coordinates;
  if (!ReadTextAttribute(ncFD, varId, "coordinates", coordinates)) return 0;

  vtkstd::vector<vtkstd::string> coordName;
  vtksys::SystemTools::Split(coordinates, coordName, ' ');

  for (vtkstd::vector<vtkstd::string>::iterator iter = coordName.begin();
       iter != coordName.end(); iter++)
    {
    int auxCoordVarId;
    if (nc_inq_varid(ncFD, iter->c_str(), &auxCoordVarId) != NC_NOERR) continue;

    // I am only interested in 2D variables.
    int numDims;
    CALL_NETCDF_GW(nc_inq_varndims(ncFD, auxCoordVarId, &numDims));
    if (numDims != 2) continue;

    // Make sure that the coordinate variables have the same dimensions and that
    // those dimensions are the same as the last two dimensions on the grid.
    // Not sure if that is enforced by the specification, but I am going to make
    // that assumption.
    int auxCoordDims[2];
    CALL_NETCDF_GW(nc_inq_vardimid(ncFD, auxCoordVarId, auxCoordDims));
    int *gridDims = this->GridDimensions->GetPointer(numGridDimensions - 2);
    if ((auxCoordDims[0] != gridDims[0]) || (auxCoordDims[1] != gridDims[1]))
      {
      continue;
      }

    // The variable is no use to me unless it is identified as either longitude
    // or latitude.
    vtkStdString units;
    if (!ReadTextAttribute(ncFD, auxCoordVarId, "units", units)) continue;
    units = vtksys::SystemTools::LowerCase(units);
    if (vtksys::RegularExpression("degrees?_?n").find(units))
      {
      latitudeCoordVarId = auxCoordVarId;
      }
    else if (vtksys::RegularExpression("degrees?_?e").find(units))
      {
      longitudeCoordVarId = auxCoordVarId;
      }
    else
      {
      continue;
      }
    this->SpecialVariables->InsertNextValue(*iter);
    }

  if ((longitudeCoordVarId == -1) || (latitudeCoordVarId == -1))
    {
    // Did not find coordinate variables.
    return 0;
    }

  vtkStdString bounds;
  if (ReadTextAttribute(ncFD, longitudeCoordVarId, "bounds", bounds))
    {
    // The bounds is supposed to point to an array with 3 dimensions.  The first
    // two should be the same as the coord array, the third with 4 entries.
    // Maybe I should check this, but I'm not.
    CALL_NETCDF_GW(nc_inq_varid(ncFD, bounds.c_str(), &longitudeBoundsVarId));
    this->SpecialVariables->InsertNextValue(bounds);
    }
  if (ReadTextAttribute(ncFD, latitudeCoordVarId, "bounds", bounds))
    {
    // The bounds is supposed to point to an array with 3 dimensions.  The first
    // two should be the same as the coord array, the third with 4 entries.
    // Maybe I should check this, but I'm not.
    CALL_NETCDF_GW(nc_inq_varid(ncFD, bounds.c_str(), &latitudeBoundsVarId));
    this->SpecialVariables->InsertNextValue(bounds);
    }

  this->HasBounds = ((longitudeBoundsVarId != -1)&&(latitudeBoundsVarId != -1));

  // Load in all the longitude and latitude coordinates.  Maybe not the most
  // efficient thing to do for large data, but it is just a 2D plane, so it
  // should be OK for most things.
  this->LongitudeCoordinates = vtkSmartPointer<vtkDoubleArray>::New();
  this->LatitudeCoordinates = vtkSmartPointer<vtkDoubleArray>::New();
  if (this->HasBounds)
    {
    if (!this->LoadBoundsVariable(ncFD, longitudeBoundsVarId,
                                  this->LongitudeCoordinates)) return 0;
    if (!this->LoadBoundsVariable(ncFD, latitudeBoundsVarId,
                                  this->LatitudeCoordinates)) return 0;
    }
  else
    {
    if (!this->LoadCoordinateVariable(ncFD, longitudeCoordVarId,
                                      this->LongitudeCoordinates)) return 0;
    if (!this->LoadCoordinateVariable(ncFD, latitudeCoordVarId,
                                      this->LatitudeCoordinates)) return 0;
    }

  return 1;
}

//-----------------------------------------------------------------------------
int vtkNetCDFCFReader::vtkDependentDimensionInfo::LoadCoordinateVariable(
                                                         int ncFD, int varId,
                                                         vtkDoubleArray *coords)
{
  int dimIds[2];
  CALL_NETCDF_GW(nc_inq_vardimid(ncFD, varId, dimIds));

  size_t dimSizes[2];
  for (int i = 0; i < 2; i++)
    {
    CALL_NETCDF_GW(nc_inq_dimlen(ncFD, dimIds[i], &dimSizes[i]));
    }

  coords->SetNumberOfComponents(static_cast<int>(dimSizes[1]));
  coords->SetNumberOfTuples(static_cast<vtkIdType>(dimSizes[0]));
  CALL_NETCDF_GW(nc_get_var_double(ncFD, varId, coords->GetPointer(0)));

  return 1;
}

//-----------------------------------------------------------------------------
int vtkNetCDFCFReader::vtkDependentDimensionInfo::LoadBoundsVariable(
                                                         int ncFD, int varId,
                                                         vtkDoubleArray *coords)
{
  int dimIds[3];
  CALL_NETCDF_GW(nc_inq_vardimid(ncFD, varId, dimIds));

  size_t dimSizes[3];
  for (int i = 0; i < 3; i++)
    {
    CALL_NETCDF_GW(nc_inq_dimlen(ncFD, dimIds[i], &dimSizes[i]));
    }

  if (dimSizes[2] != 4)
    {
    vtkGenericWarningMacro(<< "Expected 2D dependent coordinate bounds to have"
                           << " 4 entries in final dimension.  Instead has "
                           << dimSizes[2]);
    return 0;
    }

  // Bounds are stored as 4-tuples for every cell.  Tuple entries 0 and 1
  // connect to the cell in the -i topological direction.  Tuple entries 0 and 3
  // connect to the cell in the -j topological direction.
  vtkstd::vector<double> boundsData(dimSizes[0]*dimSizes[1]*4);
  CALL_NETCDF_GW(nc_get_var_double(ncFD, varId, &boundsData.at(0)));

  // The coords array are the coords at the points.  There is one more point
  // than cell in each topological direction.
  int numComponents = static_cast<int>(dimSizes[1]);
  vtkIdType numTuples = static_cast<vtkIdType>(dimSizes[0]);
  coords->SetNumberOfComponents(numComponents+1);
  coords->SetNumberOfTuples(numTuples+1);

  // Copy from the bounds data to the coordinates data.  Most values will
  // be copied from the bound's 0'th tuple entry.  Values at the extremes
  // will be copied from other entries.
  for (vtkIdType j = 0; j < numTuples; j++)
    {
    for (int i = 0; i < numComponents; i++)
      {
      coords->SetComponent(j, i, boundsData[(j*numComponents + i)*4 + 0]);
      }
    coords->SetComponent(j, numComponents,
                         boundsData[((j+1)*numComponents-1)*4 + 1]);
    }
  for (int i = 0; i < numComponents; i++)
    {
    coords->SetComponent(numTuples, i,
                         boundsData[((numTuples-1)*numComponents)*4 + 2]);
    }
  coords->SetComponent(numTuples, numComponents,
                       boundsData[(numTuples*numComponents-1)*4 + 3]);

  return 1;
}

//-----------------------------------------------------------------------------
class vtkNetCDFCFReader::vtkDependentDimensionInfoVector
{
public:
  vtkstd::vector<vtkNetCDFCFReader::vtkDependentDimensionInfo> v;
};

//=============================================================================
vtkStandardNewMacro(vtkNetCDFCFReader);

//-----------------------------------------------------------------------------
vtkNetCDFCFReader::vtkNetCDFCFReader()
{
  this->SphericalCoordinates = 1;
  this->VerticalScale = 1.0;
  this->VerticalBias = 0.0;

  this->DimensionInfo = new vtkDimensionInfoVector;
  this->DependentDimensionInfo = new vtkDependentDimensionInfoVector;
}

vtkNetCDFCFReader::~vtkNetCDFCFReader()
{
  delete this->DimensionInfo;
  delete this->DependentDimensionInfo;
}

void vtkNetCDFCFReader::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "SphericalCoordinates: " << this->SphericalCoordinates <<endl;
  os << indent << "VerticalScale: " << this->VerticalScale <<endl;
  os << indent << "VerticalBias: " << this->VerticalBias <<endl;
}

//-----------------------------------------------------------------------------
int vtkNetCDFCFReader::CanReadFile(const char *filename)
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
int vtkNetCDFCFReader::RequestDataObject(
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
    if (this->SphericalCoordinates)
      {
      if (this->CoordinatesAreSpherical(currentDimensions->GetPointer(0),
                                        currentNumDims))
        {
        dataType = VTK_STRUCTURED_GRID;
        break;
        }
      }

    // Check to see if any dimension has irregular spacing.
    for (int i = 0; i < currentNumDims; i++)
      {
      int dimId = currentDimensions->GetValue(i);
      if (!this->GetDimensionInfo(dimId)->GetHasRegularSpacing())
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
int vtkNetCDFCFReader::RequestData(vtkInformation *request,
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
      origin[i] = this->GetDimensionInfo(dim)->GetOrigin();
      spacing[i] = this->GetDimensionInfo(dim)->GetSpacing();
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
        coords = this->GetDimensionInfo(dim)->GetCoordinates();
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
    if (this->FindDependentDimensionInfo(this->LoadingDimensions))
      {
      this->Add2DSphericalCoordinates(structOutput);
      }
    else
      {
      this->Add1DSphericalCoordinates(structOutput);
      }
    }

  return 1;
}

//-----------------------------------------------------------------------------
void vtkNetCDFCFReader::Add1DSphericalCoordinates(
                                                vtkStructuredGrid *structOutput)
{
  int extent[6];
  structOutput->GetExtent(extent);

  vtkDoubleArray *coordArrays[3];
  for (vtkIdType i = 0; i < this->LoadingDimensions->GetNumberOfTuples(); i++)
    {
    int dim = this->LoadingDimensions->GetValue(i);
    coordArrays[i] = this->GetDimensionInfo(dim)->GetBounds();
    }

  int longitudeDim, latitudeDim, verticalDim;
  this->IdentifySphericalCoordinates(
                                   this->LoadingDimensions->GetPointer(0),
                                   this->LoadingDimensions->GetNumberOfTuples(),
                                   longitudeDim, latitudeDim, verticalDim);

  VTK_CREATE(vtkPoints, points);
  points->SetDataTypeToDouble();
  points->Allocate(  (extent[1]-extent[0]+1)
                   * (extent[3]-extent[2]+1)
                   * (extent[5]-extent[4]+1) );

  // Check the height scale and bias.
  double vertScale = this->VerticalScale;
  double vertBias = this->VerticalBias;
  if (verticalDim >= 0)
    {
    double *verticalRange = coordArrays[verticalDim]->GetRange();
    if (   (verticalRange[0]*vertScale + vertBias < 0)
        || (verticalRange[1]*vertScale + vertBias < 0) )
      {
      vertBias = -vtkstd::min(verticalRange[0], verticalRange[1])*vertScale;
      }
    }
  else
    {
    if (vertScale + vertBias <= 0)
      {
      vertScale = 1.0;
      vertBias = 0.0;
      }
    }

  int ijk[3];
  for (ijk[0] = extent[4]; ijk[0] <= extent[5]; ijk[0]++)
    {
    for (ijk[1] = extent[2]; ijk[1] <= extent[3]; ijk[1]++)
      {
      for (ijk[2] = extent[0]; ijk[2] <= extent[1]; ijk[2]++)
        {
        double lon, lat, h;
        if (verticalDim >= 0)
          {
          lon = coordArrays[longitudeDim]->GetValue(ijk[longitudeDim]);
          lat = coordArrays[latitudeDim]->GetValue(ijk[latitudeDim]);
          h = coordArrays[verticalDim]->GetValue(ijk[verticalDim]);
          }
        else
          {
          lon = coordArrays[longitudeDim]->GetValue(ijk[longitudeDim+1]);
          lat = coordArrays[latitudeDim]->GetValue(ijk[latitudeDim+1]);
          h = 1.0;
          }
        lon = vtkMath::RadiansFromDegrees(lon);
        lat = vtkMath::RadiansFromDegrees(lat);
        h = h*vertScale + vertBias;

        double cartesianCoord[3];
        cartesianCoord[0] = h*cos(lon)*cos(lat);
        cartesianCoord[1] = h*sin(lon)*cos(lat);
        cartesianCoord[2] = h*sin(lat);
        points->InsertNextPoint(cartesianCoord);
        }
      }
    }

  structOutput->SetPoints(points);
}

//-----------------------------------------------------------------------------
void vtkNetCDFCFReader::Add2DSphericalCoordinates(
                                                vtkStructuredGrid *structOutput)
{
  vtkDependentDimensionInfo *info
    = this->FindDependentDimensionInfo(this->LoadingDimensions);

  int extent[6];
  structOutput->GetExtent(extent);

  VTK_CREATE(vtkPoints, points);
  points->SetDataTypeToDouble();
  points->Allocate(  (extent[1]-extent[0]+1)
                   * (extent[3]-extent[2]+1)
                   * (extent[5]-extent[4]+1) );

  vtkDoubleArray *longitudeCoordinates = info->GetLongitudeCoordinates();
  vtkDoubleArray *latitudeCoordinates = info->GetLatitudeCoordinates();

  vtkDoubleArray *verticalCoordinates = NULL;
  if (this->LoadingDimensions->GetNumberOfTuples() == 3)
    {
    int vertDim = this->LoadingDimensions->GetValue(0);
    if (info->GetHasBounds())
      {
      verticalCoordinates = this->GetDimensionInfo(vertDim)->GetBounds();
      }
    else
      {
      verticalCoordinates = this->GetDimensionInfo(vertDim)->GetCoordinates();
      }
    }

  // Check the height scale and bias.
  double vertScale = this->VerticalScale;
  double vertBias = this->VerticalBias;
  if (verticalCoordinates)
    {
    double *verticalRange = verticalCoordinates->GetRange();
    if (   (verticalRange[0]*vertScale + vertBias < 0)
        || (verticalRange[1]*vertScale + vertBias < 0) )
      {
      vertBias = -vtkstd::min(verticalRange[0], verticalRange[1])*vertScale;
      }
    }
  else
    {
    if (vertScale + vertBias <= 0)
      {
      vertScale = 1.0;
      vertBias = 0.0;
      }
    }

  for (int k = extent[4]; k <= extent[5]; k++)
    {
    double h;
    if (verticalCoordinates)
      {
      h = verticalCoordinates->GetValue(k)*vertScale + vertBias;
      }
    else
      {
      h = vertScale + vertBias;
      }
    for (int j = extent[2]; j <= extent[3]; j++)
      {
      for (int i = extent[0]; i <= extent[1]; i++)
        {
        double lon = longitudeCoordinates->GetComponent(j, i);
        double lat = latitudeCoordinates->GetComponent(j, i);
        lon = vtkMath::RadiansFromDegrees(lon);
        lat = vtkMath::RadiansFromDegrees(lat);

        double cartesianCoord[3];
        cartesianCoord[0] = h*cos(lon)*cos(lat);
        cartesianCoord[1] = h*sin(lon)*cos(lat);
        cartesianCoord[2] = h*sin(lat);
        points->InsertNextPoint(cartesianCoord);
        }
      }
    }

  structOutput->SetPoints(points);
}

//-----------------------------------------------------------------------------
int vtkNetCDFCFReader::ReadMetaData(int ncFD)
{
  int i;

  vtkDebugMacro("ReadMetaData");

  int numDimensions;
  CALL_NETCDF(nc_inq_ndims(ncFD, &numDimensions));
  this->DimensionInfo->v.resize(numDimensions);

  vtkstd::set<vtkStdString> specialVariables;

  for (i = 0; i < numDimensions; i++)
    {
    this->DimensionInfo->v[i] = vtkDimensionInfo(ncFD, i);

    // Record any special variables for this dimension.
    vtkStringArray* dimensionVariables
      = this->DimensionInfo->v[i].GetSpecialVariables();
    for (vtkIdType j = 0; j < dimensionVariables->GetNumberOfValues(); j++)
      {
      specialVariables.insert(dimensionVariables->GetValue(j));
      }
    }

  int numVariables;
  CALL_NETCDF(nc_inq_nvars(ncFD, &numVariables));

  // Check all variables for special 2D coordinates.
  for (i = 0; i < numVariables; i++)
    {
    vtkDependentDimensionInfo info(ncFD, i, this);
    if (!info.GetValid()) continue;
    if (this->FindDependentDimensionInfo(info.GetGridDimensions()) != NULL)
      {
      continue;
      }

    this->DependentDimensionInfo->v.push_back(info);

    // Record any special variables.
    vtkStringArray* dimensionVariables = info.GetSpecialVariables();
    for (vtkIdType j = 0; j < dimensionVariables->GetNumberOfValues(); j++)
      {
      specialVariables.insert(dimensionVariables->GetValue(j));
      }
    }

  // Look at all variables and record them so that the user can select
  // which ones he wants.
  this->VariableArraySelection->RemoveAllArrays();

  for (i = 0; i < numVariables; i++)
    {
    char name[NC_MAX_NAME+1];
    CALL_NETCDF(nc_inq_varname(ncFD, i, name));
    // Make sure this is not a special variable that describes a dimension
    // before exposing it.
    if (specialVariables.find(name) == specialVariables.end())
      {
      this->VariableArraySelection->AddArray(name);
      }
    }

  return 1;
}

//-----------------------------------------------------------------------------
int vtkNetCDFCFReader::IsTimeDimension(int vtkNotUsed(ncFD), int dimId)
{
  return (   this->GetDimensionInfo(dimId)->GetUnits()
          == vtkDimensionInfo::TIME_UNITS );
}

//-----------------------------------------------------------------------------
vtkSmartPointer<vtkDoubleArray> vtkNetCDFCFReader::GetTimeValues(
                                                int vtkNotUsed(ncFD), int dimId)
{
  return this->GetDimensionInfo(dimId)->GetCoordinates();
}

//-----------------------------------------------------------------------------
inline vtkNetCDFCFReader::vtkDimensionInfo *
vtkNetCDFCFReader::GetDimensionInfo(int dimension)
{
  return &(this->DimensionInfo->v.at(dimension));
}

//-----------------------------------------------------------------------------
vtkNetCDFCFReader::vtkDependentDimensionInfo *
vtkNetCDFCFReader::FindDependentDimensionInfo(vtkIntArray *dims)
{
  return this->FindDependentDimensionInfo(dims->GetPointer(0),
                                          dims->GetNumberOfTuples());
}

vtkNetCDFCFReader::vtkDependentDimensionInfo *
vtkNetCDFCFReader::FindDependentDimensionInfo(const int *dims, int numDims)
{
  for (size_t i = 0; i < this->DependentDimensionInfo->v.size(); i++)
    {
    vtkIntArray *dependentDims
      = this->DependentDimensionInfo->v[i].GetGridDimensions();
    if (numDims == dependentDims->GetNumberOfTuples())
      {
      bool same = true;
      for (vtkIdType j = 0; j < numDims; j++)
        {
        if (dims[j] != dependentDims->GetValue(j))
          {
          same = false;
          break;
          }
        }
      if (same) return &(this->DependentDimensionInfo->v[i]);
      }
    }
  return NULL;
}

//-----------------------------------------------------------------------------
void vtkNetCDFCFReader::IdentifySphericalCoordinates(const int *dimensions,
                                                     int numDimensions,
                                                     int &longitudeDim,
                                                     int &latitudeDim,
                                                     int &verticalDim)
{
  longitudeDim = latitudeDim = verticalDim = -1;
  for (int i = 0; i < numDimensions; i++)
    {
    switch (this->GetDimensionInfo(dimensions[i])->GetUnits())
      {
      case vtkDimensionInfo::LONGITUDE_UNITS:
        longitudeDim = i;
        break;
      case vtkDimensionInfo::LATITUDE_UNITS:
        latitudeDim = i;
        break;
      default:
        verticalDim = i;
        break;
      }
    }
}

