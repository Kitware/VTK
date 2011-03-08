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

#include "vtkCellArray.h"
#include "vtkDataArraySelection.h"
#include "vtkDoubleArray.h"
#include "vtkExtentTranslator.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkIntArray.h"
#include "vtkMath.h"
#include "vtkMergePoints.h"
#include "vtkObjectFactory.h"
#include "vtkPoints.h"
#include "vtkRectilinearGrid.h"
#include "vtkStdString.h"
#include "vtkStringArray.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStructuredGrid.h"
#include "vtkUnstructuredGrid.h"

#include "vtkSmartPointer.h"
#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

#include <vtkstd/set>
#include <vtksys/RegularExpression.hxx>
#include <vtksys/SystemTools.hxx>

#include <string.h>

#include "vtk_netcdf.h"

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

//-----------------------------------------------------------------------------
// Convenience function for getting the range of all values in all components of
// a vtkDoubleArray.
static void GetRangeOfAllComponents(vtkDoubleArray *array, double range[2])
{
  range[0] = VTK_DOUBLE_MAX;
  range[1] = VTK_DOUBLE_MIN;
  for (int component = 0; component<array->GetNumberOfComponents(); component++)
    {
    double componentRange[2];
    array->GetRange(componentRange, component);
    if (componentRange[0] < range[0]) range[0] = componentRange[0];
    if (componentRange[1] > range[1]) range[1] = componentRange[1];
    }
}

//=============================================================================
vtkNetCDFCFReader::vtkDimensionInfo::vtkDimensionInfo(int ncFD, int id)
{
  this->DimId = id;

  this->Units = UNDEFINED_UNITS;
  this->HasRegularSpacing = true;
  this->Origin = 0.0;
  this->Spacing = 1.0;

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
      if (   (units.find(" since ") != vtkStdString::npos)
          || (units.find(" after ") != vtkStdString::npos) )
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
  // coordinate lookup.  Technically, the CF specification allows you to specify
  // different coordinate variables, but we do not support that because there is
  // no easy way to differentiate grids with the same dimensions.  If different
  // coordinates are needed, then duplicate dimensions should be created.
  // Anyone who disagrees should write their own reader class.
  int numGridDimensions;
  CALL_NETCDF_GW(nc_inq_varndims(ncFD, varId, &numGridDimensions));
  this->GridDimensions->SetNumberOfTuples(numGridDimensions);
  CALL_NETCDF_GW(nc_inq_vardimid(ncFD, varId,
                                 this->GridDimensions->GetPointer(0)));

  // Remove initial time dimension, which has no effect on data type.
  if (parent->IsTimeDimension(ncFD, this->GridDimensions->GetValue(0)))
    {
    this->GridDimensions->RemoveTuple(0);
    numGridDimensions--;
    }

  // Most coordinate variables are defined by a variable the same name as the
  // dimension they describe.  Those are handled elsewhere.  This class handles
  // dependent variables that define coordinates that are not the same name as
  // any dimension.  This is only done when the coordinates cannot be expressed
  // as a 1D table lookup from dimension index.  This occurs in only two places
  // in the CF convention.  First, it happens for 2D coordinate variables with
  // 4-sided cells.  This is basically when the grid is a 2D curvilinear grid.
  // Each i,j topological point can be placed anywhere in space.  Second, it
  // happens for multi-dimensional coordinate variables with p-sided cells.
  // These are unstructured collections of polygons.

  vtkStdString coordinates;
  if (!ReadTextAttribute(ncFD, varId, "coordinates", coordinates)) return 0;

  vtkstd::vector<vtkstd::string> coordName;
  vtksys::SystemTools::Split(coordinates, coordName, ' ');

  int numAuxCoordDims = -1;

  for (vtkstd::vector<vtkstd::string>::iterator iter = coordName.begin();
       iter != coordName.end(); iter++)
    {
    int auxCoordVarId;
    if (nc_inq_varid(ncFD, iter->c_str(), &auxCoordVarId) != NC_NOERR) continue;

    // Make sure that the coordinate variables have the same dimensions and that
    // those dimensions are the same as the last two dimensions on the grid.
    // Not sure if that is enforced by the specification, but I am going to make
    // that assumption.
    int numDims;
    CALL_NETCDF_GW(nc_inq_varndims(ncFD, auxCoordVarId, &numDims));
    // I am only supporting either 1 or 2 dimensions in the coordinate
    // variables.  See the comment below regarding identifying the
    // CellsUnstructured flag.
    if (numDims > 2) continue;

    int auxCoordDims[2];
    CALL_NETCDF_GW(nc_inq_vardimid(ncFD, auxCoordVarId, auxCoordDims));
    int *gridDims = this->GridDimensions->GetPointer(numGridDimensions-numDims);
    bool auxCoordDimsValid = true;
    for (int dimId = 0; dimId < numDims; dimId++)
      {
      if (auxCoordDims[dimId] != gridDims[dimId])
        {
        auxCoordDimsValid = false;
        break;
        }
      }
    if (!auxCoordDimsValid) continue;

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

    if (numAuxCoordDims < 0)
      {
      // Record the number of dimensions in the coordinate arrays.
      numAuxCoordDims = numDims;
      }
    else if (numAuxCoordDims != numDims)
      {
      // Number of dimensions in different coordinate arrays do not match.
      return 0;
      }
    }

  if ((longitudeCoordVarId == -1) || (latitudeCoordVarId == -1))
    {
    // Did not find all coordinate variables.
    return 0;
    }

  // Technically, p-sided cells can be accessed with any number of dimensions.
  // However, it makes little sense to have more than a 1D array of cell ids.
  // Supporting more than this is a pain because it exceeds the dimensionality
  // supported by vtkDataArray.  It also makes it exceedingly difficult to
  // determine whether the topology is implicitly defined by 2D 4-sided cells or
  // explicitly with p-sided cells.  Thus, I am only implementing p-sided cells
  // with 1D coordinate variables.
  if (numAuxCoordDims == 1)
    {
    this->CellsUnstructured = true;
    }
  else if (numAuxCoordDims == 2)
    {
    this->CellsUnstructured = false;
    }
  else
    {
    // Not supporting this.
    return 0;
    }

  vtkStdString bounds;
  if (ReadTextAttribute(ncFD, longitudeCoordVarId, "bounds", bounds))
    {
    // The bounds is supposed to point to an array with numAuxCoordDims+1
    // dimensions.  The first numAuxCoordDims should be the same as the coord
    // arrays.  The last dimension has the number of vertices in each cell.
    // Maybe I should check this, but I'm not.
    CALL_NETCDF_GW(nc_inq_varid(ncFD, bounds.c_str(), &longitudeBoundsVarId));
    this->SpecialVariables->InsertNextValue(bounds);
    }
  if (ReadTextAttribute(ncFD, latitudeCoordVarId, "bounds", bounds))
    {
    // The bounds is supposed to point to an array with numAuxCoordDims+1
    // dimensions.  The first numAuxCoordDims should be the same as the coord
    // arrays.  The last dimension has the number of vertices in each cell.
    // Maybe I should check this, but I'm not.
    CALL_NETCDF_GW(nc_inq_varid(ncFD, bounds.c_str(), &latitudeBoundsVarId));
    this->SpecialVariables->InsertNextValue(bounds);
    }

  this->HasBounds = ((longitudeBoundsVarId != -1)&&(latitudeBoundsVarId != -1));

  // Load in all the longitude and latitude coordinates.  Maybe not the most
  // efficient thing to do for large data, but it is just a 2D surface, so it
  // should be OK for most things.
  this->LongitudeCoordinates = vtkSmartPointer<vtkDoubleArray>::New();
  this->LatitudeCoordinates = vtkSmartPointer<vtkDoubleArray>::New();
  if (this->CellsUnstructured)
    {
    if (this->HasBounds)
      {
      int ok;
      ok = this->LoadUnstructuredBoundsVariable(ncFD, longitudeBoundsVarId,
                                                this->LongitudeCoordinates);
      if (!ok) return 0;
      ok = this->LoadUnstructuredBoundsVariable(ncFD, latitudeBoundsVarId,
                                                this->LatitudeCoordinates);
      if (!ok) return 0;
      }
    else
      {
      // Unstructured cells need bounds to define the topology.
      return 0;
      }
    }
  else
    {
    if (this->HasBounds)
      {
      int ok;
      ok = this->LoadBoundsVariable(ncFD, longitudeBoundsVarId,
                                    this->LongitudeCoordinates);
      if (!ok) return 0;
      ok = this->LoadBoundsVariable(ncFD, latitudeBoundsVarId,
                                    this->LatitudeCoordinates);
      if (!ok) return 0;
    }
    else
      {
      int ok;
      ok = this->LoadCoordinateVariable(ncFD, longitudeCoordVarId,
                                        this->LongitudeCoordinates);
      if (!ok) return 0;
      ok = this->LoadCoordinateVariable(ncFD, latitudeCoordVarId,
                                        this->LatitudeCoordinates);
      if (!ok) return 0;
      }
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
int vtkNetCDFCFReader::vtkDependentDimensionInfo::LoadUnstructuredBoundsVariable(
                                    int ncFD, int varId, vtkDoubleArray *coords)
{
  int dimIds[2];
  CALL_NETCDF_GW(nc_inq_vardimid(ncFD, varId, dimIds));

  size_t dimSizes[2];
  for (int i = 0; i < 2; i++)
    {
    CALL_NETCDF_GW(nc_inq_dimlen(ncFD, dimIds[i], &dimSizes[i]));
    }

  int numVertPerCell = static_cast<int>(dimSizes[1]);
  vtkIdType numCells = static_cast<vtkIdType>(dimSizes[0]);

  coords->SetNumberOfComponents(numVertPerCell);
  coords->SetNumberOfTuples(numCells);
  CALL_NETCDF_GW(nc_get_var_double(ncFD, varId, coords->GetPointer(0)));

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
  this->OutputType = -1;

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
  os << indent << "OutputType: " << this->OutputType <<endl;
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
void vtkNetCDFCFReader::SetOutputType(int type)
{
  vtkDebugMacro(<< this->GetClassName() << " (" << this << "):"
                " setting OutputType to " << type);
  if (this->OutputType != type)
    {
    bool typeValid = (   (type == -1)
                      || (type == VTK_IMAGE_DATA)
                      || (type == VTK_RECTILINEAR_GRID)
                      || (type == VTK_STRUCTURED_GRID)
                      || (type == VTK_UNSTRUCTURED_GRID) );
    if (typeValid)
      {
      this->OutputType = type;
      this->Modified();
      }
    else
      {
      vtkErrorMacro(<< "Invalid OutputType: " << type);
      }
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

  // Check that the dataType is correct or automatically set it if it is set to
  // -1.
  int dataType = this->OutputType;

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

    CoordinateTypesEnum coordType = this->CoordinateType(currentDimensions);

    int preferredDataType;
    switch(coordType)
      {
      case COORDS_UNIFORM_RECTILINEAR:
        preferredDataType = VTK_IMAGE_DATA;
        break;
      case COORDS_NONUNIFORM_RECTILINEAR:
        preferredDataType = VTK_RECTILINEAR_GRID;
        break;
      case COORDS_REGULAR_SPHERICAL:
      case COORDS_2D_EUCLIDEAN:
      case COORDS_2D_SPHERICAL:
      case COORDS_EUCLIDEAN_4SIDED_CELLS:
      case COORDS_SPHERICAL_4SIDED_CELLS:
        preferredDataType = VTK_STRUCTURED_GRID;
        break;
      case COORDS_EUCLIDEAN_PSIDED_CELLS:
      case COORDS_SPHERICAL_PSIDED_CELLS:
        preferredDataType = VTK_UNSTRUCTURED_GRID;
        break;
      default:
        vtkErrorMacro(<< "Internal error: unknown coordinate type.");
        return 0;
      }

    // Check the data type.
    if (dataType == -1)
      {
      dataType = preferredDataType;
      }
    else
      {
      switch (dataType)
        {
        case VTK_IMAGE_DATA:
          if (preferredDataType != VTK_IMAGE_DATA)
            {
            vtkWarningMacro("You have set the OutputType to a data type that"
                            " cannot fully represent the topology of the data."
                            " Some of the topology will be ignored.");
            }
          break;
        case VTK_RECTILINEAR_GRID:
          if (   (preferredDataType != VTK_IMAGE_DATA)
              || (preferredDataType != VTK_RECTILINEAR_GRID) )
            {
            vtkWarningMacro("You have set the OutputType to a data type that"
                            " cannot fully represent the topology of the data."
                            " Some of the topology will be ignored.");
            }
          break;
        case VTK_STRUCTURED_GRID:
          if (   (preferredDataType != VTK_IMAGE_DATA)
              || (preferredDataType != VTK_RECTILINEAR_GRID)
              || (preferredDataType != VTK_STRUCTURED_GRID) )
            {
            vtkWarningMacro("You have set the OutputType to a data type that"
                            " cannot fully represent the topology of the data."
                            " Some of the topology will be ignored.");
            }
          break;
        case VTK_UNSTRUCTURED_GRID:
          // Unstructured grid supports all topologies.
          break;
        default:
          vtkErrorMacro(<< "Sanity check failed: bad internal type.");
          return 0;
        }
      }

    // That's right, break.  We really only want to look at the dimensions of
    // the first valid loaded variable.  If we got here, we found a variable.
    // The loop is really only for the continues at the top.
    break;
    }

  if (dataType == -1)
    {
    // Either no variables are selected or only variables with no dimensions.
    // In either case, an image data should be fine.
    dataType = VTK_IMAGE_DATA;
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
  else if (dataType == VTK_STRUCTURED_GRID)
    {
    if (!output || !output->IsA("vtkStructuredGrid"))
      {
      output = vtkStructuredGrid::New();
      output->SetPipelineInformation(outInfo);
      output->Delete();   // Not really deleted.
      }
    }
  else if (dataType == VTK_UNSTRUCTURED_GRID)
    {
    if (!output || !output->IsA("vtkUnstructuredGrid"))
      {
      output = vtkUnstructuredGrid::New();
      output->SetPipelineInformation(outInfo);
      output->Delete();   // Not really deleted.
      }
    }
  else
    {
    vtkErrorMacro(<< "Sanity check failed: bad internal type.");
    return 0;
    }

  return 1;
}

//-----------------------------------------------------------------------------
int vtkNetCDFCFReader::RequestInformation(vtkInformation *request,
                                          vtkInformationVector **inputVector,
                                          vtkInformationVector *outputVector)
{
  // Let the superclass do the heavy lifting.
  if (!this->Superclass::RequestInformation(request, inputVector, outputVector))
    {
    return 0;
    }

  // Superclass understands structured data, but we have to handle unstructured
  // "extents" (pieces).
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  vtkDataObject *output = vtkDataObject::GetData(outInfo);
  if (output && (output->GetExtentType() != VTK_3D_EXTENT))
    {
    outInfo->Set(
              vtkStreamingDemandDrivenPipeline::MAXIMUM_NUMBER_OF_PIECES(), -1);
    }

  return 1;
}

//-----------------------------------------------------------------------------
int vtkNetCDFCFReader::RequestData(vtkInformation *request,
                                   vtkInformationVector **inputVector,
                                   vtkInformationVector *outputVector)
{
  // If the output does not directly support 3D extents, then we have to make
  // some from the piece information so the superclass knows what portion of
  // arrays to load.
  vtkDataObject *output = vtkDataObject::GetData(outputVector);
  if (output)
    {
    if (output->GetExtentType() == VTK_3D_EXTENT)
      {
      // Do nothing.  3D extents already set.
      }
    else if (output->GetExtentType() == VTK_PIECES_EXTENT)
      {
      int pieceNumber, numberOfPieces, ghostLevels;
      vtkInformation *outInfo = outputVector->GetInformationObject(0);
      pieceNumber = outInfo->Get(
                       vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
      numberOfPieces = outInfo->Get(
                   vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
      ghostLevels = outInfo->Get(
             vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS());

      int extent[6];
      this->ExtentForDimensionsAndPiece(pieceNumber,
                                        numberOfPieces,
                                        ghostLevels,
                                        extent);

      // Store the update extent in the output's information object to make it
      // easy to find whenever loading data for this object.
      output->GetInformation()->Set(
                  vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), extent, 6);
      }
    else
      {
      vtkWarningMacro(<< "Invalid extent type encountered.  Data arrays may"
                      << " be loaded incorrectly.");
      }
    }
  else // output == NULL
    {
    vtkErrorMacro(<< "No output object.");
    return 0;
    }

  // Let the superclass do the heavy lifting.
  if (!this->Superclass::RequestData(request, inputVector, outputVector))
    {
    return 0;
    }

  // Add spacing information defined by the COARDS conventions.

  vtkImageData *imageOutput = vtkImageData::GetData(outputVector);
  if (imageOutput)
    {
    this->AddRectilinearCoordinates(imageOutput);
    }

  vtkRectilinearGrid *rectilinearOutput
    = vtkRectilinearGrid::GetData(outputVector);
  if (rectilinearOutput)
    {
    this->AddRectilinearCoordinates(rectilinearOutput);
    }

  vtkStructuredGrid *structuredOutput
    = vtkStructuredGrid::GetData(outputVector);
  if (structuredOutput)
    {
    switch (this->CoordinateType(this->LoadingDimensions))
      {
      case COORDS_UNIFORM_RECTILINEAR:
      case COORDS_NONUNIFORM_RECTILINEAR:
        this->Add1DRectilinearCoordinates(structuredOutput);
        break;
      case COORDS_REGULAR_SPHERICAL:
        this->Add1DSphericalCoordinates(structuredOutput);
        break;
      case COORDS_2D_EUCLIDEAN:
      case COORDS_EUCLIDEAN_4SIDED_CELLS:
        this->Add2DRectilinearCoordinates(structuredOutput);
        break;
      case COORDS_2D_SPHERICAL:
      case COORDS_SPHERICAL_4SIDED_CELLS:
        this->Add2DSphericalCoordinates(structuredOutput);
        break;
      case COORDS_EUCLIDEAN_PSIDED_CELLS:
      case COORDS_SPHERICAL_PSIDED_CELLS:
        // There is no sensible way to store p-sided cells in a structured grid.
        // Just store them as a rectilinear grid, which should at least not
        // crash (bug #11543).
        this->Add1DRectilinearCoordinates(structuredOutput);
        break;
      default:
        vtkErrorMacro("Internal error: unknown coordinate type.");
        return 0;
      }
    }

  vtkUnstructuredGrid *unstructuredOutput
    = vtkUnstructuredGrid::GetData(outputVector);
  if (unstructuredOutput)
    {
    int extent[6];
    this->GetUpdateExtentForOutput(unstructuredOutput, extent);

    switch (this->CoordinateType(this->LoadingDimensions))
      {
      case COORDS_UNIFORM_RECTILINEAR:
      case COORDS_NONUNIFORM_RECTILINEAR:
        this->Add1DRectilinearCoordinates(unstructuredOutput, extent);
        break;
      case COORDS_REGULAR_SPHERICAL:
        this->Add1DSphericalCoordinates(unstructuredOutput, extent);
        break;
      case COORDS_2D_EUCLIDEAN:
      case COORDS_EUCLIDEAN_4SIDED_CELLS:
        this->Add2DRectilinearCoordinates(unstructuredOutput, extent);
        break;
      case COORDS_2D_SPHERICAL:
      case COORDS_SPHERICAL_4SIDED_CELLS:
        this->Add2DSphericalCoordinates(unstructuredOutput, extent);
        break;
      case COORDS_EUCLIDEAN_PSIDED_CELLS:
        this->AddUnstructuredRectilinearCoordinates(unstructuredOutput, extent);
        break;
      case COORDS_SPHERICAL_PSIDED_CELLS:
        this->AddUnstructuredSphericalCoordinates(unstructuredOutput, extent);
        break;
      default:
        vtkErrorMacro("Internal error: unknown coordinate type.");
        return 0;
      }
    }

  return 1;
}

//-----------------------------------------------------------------------------
void vtkNetCDFCFReader::ExtentForDimensionsAndPiece(int pieceNumber,
                                                    int numberOfPieces,
                                                    int ghostLevels,
                                                    int extent[6])
{
  VTK_CREATE(vtkExtentTranslator, extentTranslator);

  extentTranslator->SetWholeExtent(this->WholeExtent);
  extentTranslator->SetPiece(pieceNumber);
  extentTranslator->SetNumberOfPieces(numberOfPieces);
  extentTranslator->SetGhostLevel(ghostLevels);

  extentTranslator->PieceToExtent();

  extentTranslator->GetExtent(extent);
}

//-----------------------------------------------------------------------------
void vtkNetCDFCFReader::GetUpdateExtentForOutput(vtkDataSet *output,
                                                 int extent[6])
{
  vtkInformation *info = output->GetInformation();
  if (info->Has(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT()))
    {
    info->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), extent);
    }
  else
    {
    return this->Superclass::GetUpdateExtentForOutput(output, extent);
    }
}

//-----------------------------------------------------------------------------
void vtkNetCDFCFReader::AddRectilinearCoordinates(vtkImageData *imageOutput)
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
    vtkDimensionInfo *dimInfo = this->GetDimensionInfo(dim);
    origin[i] = dimInfo->GetOrigin();
    spacing[i] = dimInfo->GetSpacing();
    }

  imageOutput->SetOrigin(origin);
  imageOutput->SetSpacing(spacing);
}

//-----------------------------------------------------------------------------
void vtkNetCDFCFReader::AddRectilinearCoordinates(
                                          vtkRectilinearGrid *rectilinearOutput)
{
  int extent[6];
  rectilinearOutput->GetExtent(extent);

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
      case 0: rectilinearOutput->SetXCoordinates(coords);  break;
      case 1: rectilinearOutput->SetYCoordinates(coords);  break;
      case 2: rectilinearOutput->SetZCoordinates(coords);  break;
      }
    }
}

//-----------------------------------------------------------------------------
void vtkNetCDFCFReader::Add1DRectilinearCoordinates(vtkPoints *points,
                                                    const int extent[6])
{
  points->SetDataTypeToDouble();
  points->SetNumberOfPoints(  (extent[1]-extent[0]+1)
                            * (extent[3]-extent[2]+1)
                            * (extent[5]-extent[4]+1) );
  vtkDataArray *pointData = points->GetData();

  int numDimNetCDF = this->LoadingDimensions->GetNumberOfTuples();
  for (int dimVTK = 0; dimVTK < 3; dimVTK++)
    {
    vtkSmartPointer<vtkDoubleArray> coords;
    if (dimVTK < numDimNetCDF)
      {
      // Remember that netCDF dimension ordering is backward from VTK.
      int dimNetCDF = this->LoadingDimensions->GetValue(numDimNetCDF-dimVTK-1);
      coords = this->GetDimensionInfo(dimNetCDF)->GetCoordinates();

      int ijk[3];
      vtkIdType pointIdx = 0;
      for (ijk[2] = extent[2*2]; ijk[2] <= extent[2*2+1]; ijk[2]++)
        {
        for (ijk[1] = extent[1*2]; ijk[1] <= extent[1*2+1]; ijk[1]++)
          {
          for (ijk[0] = extent[0*2]; ijk[0] <= extent[0*2+1]; ijk[0]++)
            {
            pointData->SetComponent(pointIdx, dimVTK,
                                    coords->GetValue(ijk[dimVTK]));
            pointIdx++;
            }
          }
        }
      }
    else
      {
      int ijk[3];
      vtkIdType pointIdx = 0;
      for (ijk[2] = extent[2*2]; ijk[2] <= extent[2*2+1]; ijk[2]++)
        {
        for (ijk[1] = extent[1*2]; ijk[1] <= extent[1*2+1]; ijk[1]++)
          {
          for (ijk[0] = extent[0*2]; ijk[0] <= extent[0*2+1]; ijk[0]++)
            {
            pointData->SetComponent(pointIdx, dimVTK, 0.0);
            pointIdx++;
            }
          }
        }
      }
    }
}

//-----------------------------------------------------------------------------
void vtkNetCDFCFReader::Add2DRectilinearCoordinates(vtkPoints *points,
                                                    const int extent[6])
{
  points->SetDataTypeToDouble();
  points->Allocate(  (extent[1]-extent[0]+1)
                   * (extent[3]-extent[2]+1)
                   * (extent[5]-extent[4]+1) );

  vtkDependentDimensionInfo *info
    = this->FindDependentDimensionInfo(this->LoadingDimensions);

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

  for (int k = extent[4]; k <= extent[5]; k++)
    {
    double h;
    if (verticalCoordinates)
      {
      h = verticalCoordinates->GetValue(k);
      }
    else
      {
      h = 0.0;
      }
    for (int j = extent[2]; j <= extent[3]; j++)
      {
      for (int i = extent[0]; i <= extent[1]; i++)
        {
        double lon = longitudeCoordinates->GetComponent(j, i);
        double lat = latitudeCoordinates->GetComponent(j, i);
        points->InsertNextPoint(lon, lat, h);
        }
      }
    }
}

//-----------------------------------------------------------------------------
void vtkNetCDFCFReader::Add1DRectilinearCoordinates(
                                            vtkStructuredGrid *structuredOutput)
{
  int extent[6];
  structuredOutput->GetExtent(extent);

  VTK_CREATE(vtkPoints, points);
  this->Add1DRectilinearCoordinates(points, extent);
  structuredOutput->SetPoints(points);
}

//-----------------------------------------------------------------------------
void vtkNetCDFCFReader::Add2DRectilinearCoordinates(
                                            vtkStructuredGrid *structuredOutput)
{
  int extent[6];
  structuredOutput->GetExtent(extent);

  VTK_CREATE(vtkPoints, points);
  this->Add2DRectilinearCoordinates(points, extent);
  structuredOutput->SetPoints(points);
}

//-----------------------------------------------------------------------------
void vtkNetCDFCFReader::Add1DRectilinearCoordinates(
                                        vtkUnstructuredGrid *unstructuredOutput,
                                        const int extent[6])
{
  VTK_CREATE(vtkPoints, points);
  this->Add1DRectilinearCoordinates(points, extent);
  unstructuredOutput->SetPoints(points);

  this->AddStructuredCells(unstructuredOutput, extent);
}

//-----------------------------------------------------------------------------
void vtkNetCDFCFReader::Add2DRectilinearCoordinates(
                                        vtkUnstructuredGrid *unstructuredOutput,
                                        const int extent[6])
{
  VTK_CREATE(vtkPoints, points);
  this->Add2DRectilinearCoordinates(points, extent);
  unstructuredOutput->SetPoints(points);

  this->AddStructuredCells(unstructuredOutput, extent);
}

//-----------------------------------------------------------------------------
void vtkNetCDFCFReader::Add1DSphericalCoordinates(vtkPoints *points,
                                                  const int extent[6])
{
  points->SetDataTypeToDouble();
  points->Allocate(  (extent[1]-extent[0]+1)
                   * (extent[3]-extent[2]+1)
                   * (extent[5]-extent[4]+1) );

  vtkDoubleArray *coordArrays[3];
  for (vtkIdType i = 0; i < this->LoadingDimensions->GetNumberOfTuples(); i++)
    {
    int dim = this->LoadingDimensions->GetValue(i);
    coordArrays[i] = this->GetDimensionInfo(dim)->GetBounds();
    }

  int longitudeDim, latitudeDim, verticalDim;
  this->IdentifySphericalCoordinates(this->LoadingDimensions,
                                     longitudeDim, latitudeDim, verticalDim);

  if ((longitudeDim < 0) || (latitudeDim < 0))
    {
    vtkErrorMacro(<< "Internal error: treating non spherical coordinates as if"
                  << " they were spherical.");
    return;
    }

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
}

//-----------------------------------------------------------------------------
void vtkNetCDFCFReader::Add2DSphericalCoordinates(vtkPoints *points,
                                                  const int extent[6])
{
  points->SetDataTypeToDouble();
  points->Allocate(  (extent[1]-extent[0]+1)
                   * (extent[3]-extent[2]+1)
                   * (extent[5]-extent[4]+1) );

  vtkDependentDimensionInfo *info
    = this->FindDependentDimensionInfo(this->LoadingDimensions);

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
}

//-----------------------------------------------------------------------------
void vtkNetCDFCFReader::Add1DSphericalCoordinates(
                                            vtkStructuredGrid *structuredOutput)
{
  int extent[6];
  structuredOutput->GetExtent(extent);

  VTK_CREATE(vtkPoints, points);
  this->Add1DSphericalCoordinates(points, extent);
  structuredOutput->SetPoints(points);
}

//-----------------------------------------------------------------------------
void vtkNetCDFCFReader::Add2DSphericalCoordinates(
                                            vtkStructuredGrid *structuredOutput)
{
  int extent[6];
  structuredOutput->GetExtent(extent);

  VTK_CREATE(vtkPoints, points);
  this->Add2DSphericalCoordinates(points, extent);
  structuredOutput->SetPoints(points);
}

//-----------------------------------------------------------------------------
void vtkNetCDFCFReader::Add1DSphericalCoordinates(
                                        vtkUnstructuredGrid *unstructuredOutput,
                                        const int extent[6])
{
  VTK_CREATE(vtkPoints, points);
  this->Add1DSphericalCoordinates(points, extent);
  unstructuredOutput->SetPoints(points);

  this->AddStructuredCells(unstructuredOutput, extent);
}

//-----------------------------------------------------------------------------
void vtkNetCDFCFReader::Add2DSphericalCoordinates(
                                        vtkUnstructuredGrid *unstructuredOutput,
                                        const int extent[6])
{
  VTK_CREATE(vtkPoints, points);
  this->Add2DSphericalCoordinates(points, extent);
  unstructuredOutput->SetPoints(points);

  this->AddStructuredCells(unstructuredOutput, extent);
}

//-----------------------------------------------------------------------------
void vtkNetCDFCFReader::AddStructuredCells(
                                        vtkUnstructuredGrid *unstructuredOutput,
                                        const int extent[6])
{
  vtkIdType numPoints[3];
  numPoints[0] = extent[1] - extent[0] + 1;
  numPoints[1] = extent[3] - extent[2] + 1;
  numPoints[2] = extent[5] - extent[4] + 1;

  vtkIdType numCells[3];
  numCells[0] = numPoints[0] - 1;
  numCells[1] = numPoints[1] - 1;
  numCells[2] = numPoints[2] - 1;

  vtkIdType nextPointRow = numPoints[0];
  vtkIdType nextPointSlab = nextPointRow*numPoints[1];

  bool extentIs2D = (numCells[2] < 1);

  if (extentIs2D)
    {
    vtkIdType totalNumCells = numCells[0]*numCells[1];
    unstructuredOutput->Allocate(totalNumCells);
    vtkCellArray *cells = unstructuredOutput->GetCells();
    cells->Allocate(cells->EstimateSize(totalNumCells, 4));

    for (int j = 0; j < numCells[1]; j++)
      {
      vtkIdType rowStart = j*nextPointRow;
      for (int i = 0; i < numCells[0]; i++)
        {
        vtkIdType lowCellPoint = rowStart + i;

        vtkIdType pointIds[4];
        pointIds[0] = lowCellPoint;
        pointIds[1] = lowCellPoint + 1;
        pointIds[2] = lowCellPoint + nextPointRow + 1;
        pointIds[3] = lowCellPoint + nextPointRow;

        unstructuredOutput->InsertNextCell(VTK_QUAD, 4, pointIds);
        }
      }
    }
  else // !extentIs2D
    {
    vtkIdType totalNumCells = numCells[0]*numCells[1]*numCells[2];
    unstructuredOutput->Allocate(totalNumCells);
    vtkCellArray *cells = unstructuredOutput->GetCells();
    cells->Allocate(cells->EstimateSize(totalNumCells, 8));

    for (int k = 0; k < numCells[2]; k++)
      {
      vtkIdType slabStart = k*nextPointSlab;
      for (int j = 0; j < numCells[1]; j++)
        {
        vtkIdType rowStart = slabStart + j*nextPointRow;
        for (int i = 0; i < numCells[0]; i++)
          {
          vtkIdType lowCellPoint = rowStart + i;

          // This code is assuming that all axis are scaling up.  If that is
          // not the case, this will probably make inverted hexahedra.
          vtkIdType pointIds[8];
          pointIds[0] = lowCellPoint;
          pointIds[1] = lowCellPoint + 1;
          pointIds[2] = lowCellPoint + nextPointRow + 1;
          pointIds[3] = lowCellPoint + nextPointRow;
          pointIds[4] = lowCellPoint + nextPointSlab;
          pointIds[5] = lowCellPoint + nextPointSlab + 1;
          pointIds[6] = lowCellPoint + nextPointSlab + nextPointRow + 1;
          pointIds[7] = lowCellPoint + nextPointSlab + nextPointRow;

          unstructuredOutput->InsertNextCell(VTK_HEXAHEDRON, 8, pointIds);
          }
        }
      }
    }
}

//-----------------------------------------------------------------------------
void vtkNetCDFCFReader::AddUnstructuredRectilinearCoordinates(
                                        vtkUnstructuredGrid *unstructuredOutput,
                                        const int extent[6])
{
  vtkDependentDimensionInfo *info
    = this->FindDependentDimensionInfo(this->LoadingDimensions);

  vtkDoubleArray *longitudeCoordinates = info->GetLongitudeCoordinates();
  vtkDoubleArray *latitudeCoordinates = info->GetLatitudeCoordinates();

  int numPointsPerCell = longitudeCoordinates->GetNumberOfComponents();
  vtkIdType totalNumCells = longitudeCoordinates->GetNumberOfTuples();

  double bounds[6];
  GetRangeOfAllComponents(longitudeCoordinates, bounds+0);
  GetRangeOfAllComponents(latitudeCoordinates, bounds+2);
  bounds[4] = bounds[5] = 0.0;

  VTK_CREATE(vtkPoints, points);
  points->SetDataTypeToDouble();
  points->Allocate(totalNumCells);

  VTK_CREATE(vtkMergePoints, locator);
  locator->InitPointInsertion(points, bounds);

  // Make space in output unstructured grid.
  unstructuredOutput->Allocate(extent[1] - extent[0]);
  vtkCellArray *cells = unstructuredOutput->GetCells();
  cells->Allocate(cells->EstimateSize(extent[1]-extent[0], numPointsPerCell));

  vtkstd::vector<vtkIdType> cellPoints(numPointsPerCell);

  // This is a rather lame way to break up cells amongst processes.  It will be
  // slow and ghost cells are totally screwed up.
  for (int cellId = extent[0]; cellId < extent[1]; cellId++)
    {
    for (int cellPointId = 0; cellPointId < numPointsPerCell; cellPointId++)
      {
      double coord[3];
      coord[0] = longitudeCoordinates->GetComponent(cellId, cellPointId);
      coord[1] = latitudeCoordinates->GetComponent(cellId, cellPointId);
      coord[2] = 0.0;

      vtkIdType pointId;
      locator->InsertUniquePoint(coord, pointId);

      cellPoints[cellPointId] = pointId;
      }
    unstructuredOutput
      ->InsertNextCell(VTK_POLYGON, numPointsPerCell, &cellPoints.at(0));
    }

  points->Squeeze();
  unstructuredOutput->SetPoints(points);
}

//-----------------------------------------------------------------------------
void vtkNetCDFCFReader::AddUnstructuredSphericalCoordinates(
                                        vtkUnstructuredGrid *unstructuredOutput,
                                        const int extent[6])
{
  // First load the data as rectilinear coordinates, and then convert them
  // to spherical coordinates.  Not only does this reuse code, but it also
  // probably makes the locator more efficient this way.
  this->AddUnstructuredRectilinearCoordinates(unstructuredOutput, extent);

  double height = 1.0*this->VerticalScale + this->VerticalBias;
  if (height <= 0.0)
    {
    height = 1.0;
    }

  vtkPoints *points = unstructuredOutput->GetPoints();
  vtkIdType numPoints = points->GetNumberOfPoints();
  for (vtkIdType pointId = 0; pointId < numPoints; pointId++)
    {
    double lonLat[3];
    points->GetPoint(pointId, lonLat);
    double lon = vtkMath::RadiansFromDegrees(lonLat[0]);
    double lat = vtkMath::RadiansFromDegrees(lonLat[1]);

    double cartesianCoord[3];
    cartesianCoord[0] = height*cos(lon)*cos(lat);
    cartesianCoord[1] = height*sin(lon)*cos(lat);
    cartesianCoord[2] = height*sin(lat);
    points->SetPoint(pointId, cartesianCoord);
    }
}

//-----------------------------------------------------------------------------
int vtkNetCDFCFReader::ReadMetaData(int ncFD)
{
  vtkDebugMacro("ReadMetaData");

  int numDimensions;
  CALL_NETCDF(nc_inq_ndims(ncFD, &numDimensions));
  this->DimensionInfo->v.resize(numDimensions);

  vtkstd::set<vtkStdString> specialVariables;

  for (int i = 0; i < numDimensions; i++)
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
  for (int i = 0; i < numVariables; i++)
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

  // Look at all variables and record them so that the user can select which
  // ones he wants.  This oddness of adding and removing from
  // VariableArraySelection is to preserve any current settings for variables.
  typedef vtkstd::set<vtkStdString> stringSet;
  stringSet variablesToAdd;
  stringSet variablesToRemove;

  // Initialize variablesToRemove with all the variables.  Then remove them from
  // the list as we find them.
  for (int i = 0; i < this->VariableArraySelection->GetNumberOfArrays(); i++)
    {
    variablesToRemove.insert(this->VariableArraySelection->GetArrayName(i));
    }

  for (int i = 0; i < numVariables; i++)
    {
    char name[NC_MAX_NAME+1];
    CALL_NETCDF(nc_inq_varname(ncFD, i, name));
    if (specialVariables.find(name) == specialVariables.end())
      {
      if (variablesToRemove.find(name) == variablesToRemove.end())
        {
        // Variable not already here.  Insert it in the variables to add.
        variablesToAdd.insert(name);
        }
      else
        {
        // Variable already exists.  Leave it be.  Remove it from the
        // variablesToRemove list.
        variablesToRemove.erase(name);
        }
      }
    }

  // Add and remove variables.  This will be a no-op if the variables have not
  // changed.
  for (stringSet::iterator removeItr = variablesToRemove.begin();
       removeItr != variablesToRemove.end(); removeItr++)
    {
    this->VariableArraySelection->RemoveArrayByName(removeItr->c_str());
    }
  for (stringSet::iterator addItr = variablesToAdd.begin();
       addItr != variablesToAdd.end(); addItr++)
    {
    this->VariableArraySelection->AddArray(addItr->c_str());
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
  for (size_t i = 0; i < this->DependentDimensionInfo->v.size(); i++)
    {
    vtkIntArray *dependentDims
      = this->DependentDimensionInfo->v[i].GetGridDimensions();
    if (dims->GetNumberOfTuples() == dependentDims->GetNumberOfTuples())
      {
      bool same = true;
      for (vtkIdType j = 0; j < dims->GetNumberOfTuples(); j++)
        {
        if (dims->GetValue(j) != dependentDims->GetValue(j))
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
void vtkNetCDFCFReader::IdentifySphericalCoordinates(vtkIntArray *dimensions,
                                                     int &longitudeDim,
                                                     int &latitudeDim,
                                                     int &verticalDim)
{
  longitudeDim = latitudeDim = verticalDim = -1;
  for (int i = 0; i < dimensions->GetNumberOfTuples(); i++)
    {
    switch (this->GetDimensionInfo(dimensions->GetValue(i))->GetUnits())
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

//-----------------------------------------------------------------------------
vtkNetCDFCFReader::CoordinateTypesEnum
vtkNetCDFCFReader::CoordinateType(vtkIntArray *dimensions)
{
  vtkDependentDimensionInfo *dependentDimInfo
    = this->FindDependentDimensionInfo(dimensions);

  // Check to see if using p-sided cells.
  if (dependentDimInfo && dependentDimInfo->GetCellsUnstructured())
    {
    if (this->SphericalCoordinates)
      {
      return COORDS_SPHERICAL_PSIDED_CELLS;
      }
    else
      {
      return COORDS_EUCLIDEAN_PSIDED_CELLS;
      }
    }

  // Check to see if using 4-sided cells.
  if (   dependentDimInfo
      && !dependentDimInfo->GetCellsUnstructured()
      && dependentDimInfo->GetHasBounds() )
    {
    if (this->SphericalCoordinates)
      {
      return COORDS_SPHERICAL_4SIDED_CELLS;
      }
    else
      {
      return COORDS_EUCLIDEAN_4SIDED_CELLS;
      }
    }

  // Check to see if using 2D coordinate lookup.
  if (   dependentDimInfo
      && !dependentDimInfo->GetCellsUnstructured()
      && !dependentDimInfo->GetHasBounds() )
    {
    if (this->SphericalCoordinates)
      {
      return COORDS_2D_SPHERICAL;
      }
    else
      {
      return COORDS_2D_EUCLIDEAN;
      }
    }

  // Check to see if we should (otherwise) be using spherical coordinates.
  if (this->SphericalCoordinates)
    {
    int longitudeDim, latitudeDim, verticalDim;
    this->IdentifySphericalCoordinates(dimensions,
                                       longitudeDim,
                                       latitudeDim,
                                       verticalDim);
    if (   (longitudeDim != -1) && (latitudeDim != -1)
        && ((dimensions->GetNumberOfTuples() == 2) || (verticalDim != -1)) )
      {
      return COORDS_REGULAR_SPHERICAL;
      }
    }

  // Check to see if any dimension as irregular spacing.
  for (int i = 0; i < dimensions->GetNumberOfTuples(); i++)
    {
    int dimId = dimensions->GetValue(i);
    if (!this->GetDimensionInfo(dimId)->GetHasRegularSpacing())
      {
      return COORDS_NONUNIFORM_RECTILINEAR;
      }
    }

  // All dimensions appear to be uniform rectilinear.
  return COORDS_UNIFORM_RECTILINEAR;
}

//-----------------------------------------------------------------------------
bool vtkNetCDFCFReader::DimensionsAreForPointData(vtkIntArray *dimensions)
{
  switch (this->CoordinateType(dimensions))
    {
    case COORDS_UNIFORM_RECTILINEAR:    return true;
    case COORDS_NONUNIFORM_RECTILINEAR: return true;
    case COORDS_REGULAR_SPHERICAL:      return false;
    case COORDS_2D_EUCLIDEAN:           return true;
    case COORDS_2D_SPHERICAL:           return true;
    case COORDS_EUCLIDEAN_4SIDED_CELLS: return false;
    case COORDS_SPHERICAL_4SIDED_CELLS: return false;
    case COORDS_EUCLIDEAN_PSIDED_CELLS: return false;
    case COORDS_SPHERICAL_PSIDED_CELLS: return false;
    default:
      vtkErrorMacro("Internal error: unknown coordinate type.");
      return true;
    }
}
