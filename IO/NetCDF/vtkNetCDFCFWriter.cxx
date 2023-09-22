// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkNetCDFCFWriter.h"

#include "vtkArrayDispatch.h"
#include "vtkAssume.h"
#include "vtkCellData.h"
#include "vtkCellTypes.h"
#include "vtkCharArray.h"
#include "vtkDataArrayAccessor.h"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkIntArray.h"
#include "vtkLogger.h"
#include "vtkLongArray.h"
#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkShortArray.h"
#include "vtkSignedCharArray.h"
#include "vtkSmartPointer.h"
#include "vtkStringArray.h"
#include "vtkUnsignedIntArray.h"
#include "vtkUnsignedLongArray.h"
#include "vtkUnsignedShortArray.h"

#include <algorithm>
#include <array>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <vector>

#include "vtk_libproj.h"
#include "vtk_netcdf.h"

VTK_ABI_NAMESPACE_BEGIN
namespace
{
std::array<std::string, 3> COORD_NAME = { { "x", "y", "z" } };
std::array<std::string, 3> BOUNDS_NAME = { { "x_bounds", "y_bounds", "z_bounds" } };

std::set<std::string> GridMappingName = { "albers_conical_equal_area", "azimuthal_equidistant",
  "geostationary", "lambert_azimuthal_equal_area", "lambert_conformal_conic",
  "lambert_cylindrical_equal_area", "latitude_longitude", "mercator", "oblique_mercator",
  "orthographic", "polar_stereographic", "rotated_latitude_longitude", "sinusoidal",
  "stereographic", "tranverse_mercator", "vertical_perspective" };

enum AttributeType
{
  Double,
  String
};
std::map<std::string, AttributeType> GridMappingAttribute = { { "azimuth_of_central_line", Double },
  { "crs_wkt", String }, { "earth_radius", Double }, { "false_easting", Double },
  { "false_northing", Double }, { "geographic_crs_name", String }, { "geoid_name", String },
  { "geopotential_datum_name", String }, { "grid_mapping_name", String },
  { "grid_north_pole_latitude", Double }, { "grid_north_pole_longitude", Double },
  { "horizontal_datum_name", String }, { "inverse_flattening", Double },
  { "latitude_of_projection_origin", Double }, { "longitude_of_central_meridian", Double },
  { "longitude_of_prime_meridian", Double }, { "longitude_of_projection_origin", Double },
  { "north_pole_grid_longitude", Double }, { "perspective_point_height", Double },
  { "prime_meridian_name", String }, { "projection_crs_name", String },
  { "reference_ellipsoid_name", String }, { "scale_factor_at_central_meridian", Double },
  { "scale_factor_at_projection_origin", Double }, { "semi_major_axis", Double },
  { "semi_minor_axis", Double }, { "standard_parallel", Double },
  { "straight_vertical_longitude_from_pole", Double }, { "towgs84", Double } };

//=============================================================================
nc_type VTKTypeToNetCDFType(int type)
{
  switch (type)
  {
      // we use BYTE for char and signed char because NC_CHAR is an ascii character.
      // and NetCDF reports an error if you store something else.
    case VTK_CHAR:
    case VTK_SIGNED_CHAR:
    case VTK_UNSIGNED_CHAR:
      return NC_BYTE;
    case VTK_SHORT:
      return NC_SHORT;
    case VTK_INT:
      return NC_INT;
    case VTK_LONG:
      return NC_INT;
    case VTK_FLOAT:
      return NC_FLOAT;
    case VTK_DOUBLE:
      return NC_DOUBLE;
    default:
      return -1;
  }
}

//------------------------------------------------------------------------------
void SaveCoords(int ncid, int attributeType, const std::array<int, 3>& coordid,
  const std::array<std::vector<double>, 3>& coord, const std::array<int, 3>& boundsid,
  std::array<std::vector<std::array<double, 2>>, 3> bounds)
{
  int status;
  for (int i = 0; i < 3; ++i)
  {
    if ((status = nc_put_var_double(ncid, coordid[i], coord[i].data())))
    {
      std::ostringstream ostr;
      ostr << "Error nc_put_var_double " << COORD_NAME[attributeType][i] << ": "
           << nc_strerror(status);
      throw std::runtime_error(ostr.str());
    }
  }
  if (attributeType == vtkDataObject::CELL)
  {
    for (int i = 0; i < 3; ++i)
    {
      if ((status = nc_put_var_double(ncid, boundsid[i], bounds[i][0].data())))
      {
        std::ostringstream ostr;
        ostr << "Error nc_put_var_double " << BOUNDS_NAME[attributeType][i] << ": "
             << nc_strerror(status);
        throw std::runtime_error(ostr.str());
      }
    }
  }
}

//------------------------------------------------------------------------------
void GetCoords(vtkImageData* id, int attributeType, std::array<std::vector<double>, 3>* coord,
  std::array<std::vector<std::array<double, 2>>, 3>* bounds)
{
  double* origin = id->GetOrigin();
  int* dims = id->GetDimensions();
  double* spacing = id->GetSpacing();
  if (attributeType == vtkDataObject::POINT)
  {
    // no bounds for point grid
    for (int axis = 0; axis < 3; ++axis)
    {
      (*coord)[axis].resize(dims[axis]);
      for (int i = 0; i < dims[axis]; ++i)
      {
        (*coord)[axis][i] = origin[axis] + spacing[axis] * i;
      }
    }
  }
  else if (attributeType == vtkDataObject::CELL)
  {
    for (int axis = 0; axis < 3; ++axis)
    {
      (*coord)[axis].resize(dims[axis] - 1);
      (*bounds)[axis].resize(dims[axis] - 1);
      // number of cells are number of points - 1.
      for (int i = 0; i < dims[axis] - 1; ++i)
      {
        (*coord)[axis][i] = origin[axis] + spacing[axis] * (i + 0.5);

        (*bounds)[axis][i][0] = origin[axis] + spacing[axis] * i;
        (*bounds)[axis][i][1] = origin[axis] + spacing[axis] * (i + 1);
      }
    }
  }
  else
  {
    std::ostringstream ostr;
    ostr << "Invalid attribute type: " << attributeType;
    throw std::runtime_error(ostr.str());
  }
}

struct BlankToFillValueWorker
{
  BlankToFillValueWorker(vtkUnsignedCharArray* ghostType, int attributeType, int fillValue)
    : GhostType(ghostType)
    , FillValue(fillValue)
  {
    if (attributeType == vtkDataObject::POINT)
    {
      this->Hidden = vtkDataSetAttributes::HIDDENPOINT;
    }
    else
    {
      this->Hidden = vtkDataSetAttributes::HIDDENCELL;
    }
  }

  template <typename DataArray1, typename DataArray2>
  void operator()(DataArray1* array, DataArray2* arrayFillValue)
  {
    // This allows the compiler to optimize for the AOS array stride.
    VTK_ASSUME(array->GetNumberOfComponents() == 1);
    VTK_ASSUME(arrayFillValue->GetNumberOfComponents() == 1);

    // These allow this single worker function to be used with both
    // the vtkDataArray 'double' API and the more efficient
    // vtkGenericDataArray APIs, depending on the template parameters:
    vtkDataArrayAccessor<DataArray1> a(array);
    vtkDataArrayAccessor<DataArray2> an(arrayFillValue);

    vtkIdType numTuples = array->GetNumberOfTuples();
    for (vtkIdType tupleIdx = 0; tupleIdx < numTuples; ++tupleIdx)
    {
      // Set and Get compile to inlined optimizable raw memory accesses for
      // vtkGenericDataArray subclasses.
      if (this->GhostType->GetValue(tupleIdx) & this->Hidden)
      {
        an.Set(tupleIdx, 0, this->FillValue);
      }
      else
      {
        an.Set(tupleIdx, 0, a.Get(tupleIdx, 0));
      }
    }
  }

private:
  vtkUnsignedCharArray* GhostType;
  unsigned char Hidden;
  int FillValue;
};

void BlankToFillValue(vtkUnsignedCharArray* ghostType, vtkDataArray* array,
  vtkDataArray* arrayFillValue, int attributeType, int fillValue)
{
  // Create our worker functor:
  BlankToFillValueWorker worker(ghostType, attributeType, fillValue);

  // Define our dispatcher. We'll let vectors have any ValueType, but only
  // consider float/double arrays for magnitudes. These combinations will
  // use a 'fast-path' implementation generated by the dispatcher:
  typedef vtkArrayDispatch::Dispatch2ByValueType<vtkArrayDispatch::AllTypes, // ValueTypes allowed
                                                                             // by first array
    vtkArrayDispatch::AllTypes>
    Dispatcher;

  // Execute the dispatcher:
  if (!Dispatcher::Execute(array, arrayFillValue, worker))
  {
    // If Execute() fails, it means the dispatch failed due to an
    // unsupported array type. In this case, it's likely that the magnitude
    // array is using an integral type. This is an uncommon case, so we won't
    // generate a fast path for these, but instead call an instantiation of
    // CalcMagnitudeWorker::operator()<vtkDataArray, vtkDataArray>.
    // Through the use of vtkDataArrayAccessor, this falls back to using the
    // vtkDataArray double API:
    worker(array, arrayFillValue);
  }
}

}

class vtkNetCDFCFWriter::Implementation
{
public:
  Implementation(vtkNetCDFCFWriter* obj)
    : Obj(obj)
  {
  }

  int CreateFile()
  {
    int ncid;
    int status, crs_var;

    // Create the netCDF file
    if ((status = nc_create(this->Obj->GetFileName(), NC_NETCDF4 | NC_CLOBBER, &ncid)))
    {
      std::ostringstream ostr;
      ostr << "Error nc_create " << this->Obj->GetFileName() << ": " << nc_strerror(status);
      throw std::runtime_error(ostr.str());
    }
    if (this->StringAttributes.find("grid_mapping_name") != this->StringAttributes.end())
    {
      // create the crs variable to store the transform
      if ((status = nc_def_var(ncid, "crs", NC_INT, 0, nullptr, &crs_var)))
      {
        std::ostringstream ostr;
        ostr << "Error nc_def_var crs: " << nc_strerror(status);
        throw std::runtime_error(ostr.str());
      }
      // add all string attributes
      for (auto it = this->StringAttributes.begin(); it != this->StringAttributes.end(); ++it)
      {
        if ((status = nc_put_att_text(
               ncid, crs_var, it->first.c_str(), it->second.length(), it->second.c_str())))
        {
          std::ostringstream ostr;
          ostr << "Error nc_put_att_text crs:" << it->first << ":" << nc_strerror(status);
          throw std::runtime_error(ostr.str());
        }
      }
      // add all double attributes
      for (auto it = this->DoubleAttributes.begin(); it != this->DoubleAttributes.end(); ++it)
      {
        if ((status =
                nc_put_att_double(ncid, crs_var, it->first.c_str(), NC_DOUBLE, 1, &it->second)))
        {
          std::ostringstream ostr;
          ostr << "Error nc_put_att_double crs:" << it->first << ":" << nc_strerror(status);
          throw std::runtime_error(ostr.str());
        }
      }
    }
    return ncid;
  }

  void DefineCoords(int ncid, int attributeType, const std::array<std::vector<double>, 3>& coords,
    std::array<int, 3>& dimid, std::array<int, 3>& coordid, std::array<int, 3>& boundsid)
  {
    int nvid = -1;
    int status;
    std::array<std::string, 3> coordNameUpper = { { "X", "Y", "Z" } };
    std::array<std::string, 3> standardName = { { "projection_x_coordinate",
      "projection_y_coordinate", "depth" } };
    std::array<std::string, 3> gridMapping = { { "crs", "crs", "" } };

    // Create the dimensions
    for (int i = 0; i < 3; ++i)
    {
      if ((status = nc_def_dim(ncid, COORD_NAME[i].c_str(), coords[i].size(), &dimid[i])))
      {
        std::ostringstream ostr;
        ostr << "Error nc_def_dim " << COORD_NAME[attributeType][i] << ": " << nc_strerror(status);
        throw std::runtime_error(ostr.str());
      }
    }
    if (attributeType == vtkDataObject::CELL)
    {
      // number of vertices for a cell along an axis
      const char* nvName = "nv";
      if ((status = nc_def_dim(ncid, nvName, 2, &nvid)))
      {
        std::ostringstream ostr;
        ostr << "Error nc_def_dim " << nvName << ": " << nc_strerror(status);
        throw std::runtime_error(ostr.str());
      }
    }

    // Create the coord variables to store the data.
    for (int i = 0; i < 3; ++i)
    {
      if ((status = nc_def_var(ncid, COORD_NAME[i].c_str(), NC_DOUBLE, 1, &dimid[i], &coordid[i])))
      {
        std::ostringstream ostr;
        ostr << "Error nc_def_var " << COORD_NAME[attributeType][i] << ": " << nc_strerror(status);
        throw std::runtime_error(ostr.str());
      }
    }

    // create attributes for x, y, z
    for (int i = 0; i < 3; ++i)
    {
      if ((status = nc_put_att_text(
             ncid, coordid[i], "standard_name", standardName[i].length(), standardName[i].c_str())))
      {
        std::ostringstream ostr;
        ostr << "Error nc_put_att_text " << COORD_NAME[attributeType][i]
             << ":standard_name: " << nc_strerror(status);
        throw std::runtime_error(ostr.str());
      }
      if (i < 2) // X and Y
      {
        if ((status = nc_put_att_text(
               ncid, coordid[i], "grid_mapping", gridMapping[i].length(), gridMapping[i].c_str())))
        {
          std::ostringstream ostr;
          ostr << "Error nc_put_att_text " << COORD_NAME[attributeType][i]
               << ":grid_mapping: " << nc_strerror(status);
          throw std::runtime_error(ostr.str());
        }
      }
      else // Z
      {
        if ((status = nc_put_att_text(ncid, coordid[i], "positive", strlen("up"), "up")))
        {
          std::ostringstream ostr;
          ostr << "Error nc_put_att_text z:positive: " << nc_strerror(status);
          throw std::runtime_error(ostr.str());
        }
      }
      if ((status = nc_put_att_text(ncid, coordid[i], "axis", 1, coordNameUpper[i].c_str())))
      {
        std::ostringstream ostr;
        ostr << "Error nc_put_att_text " << COORD_NAME[attributeType][i]
             << ":axis: " << nc_strerror(status);
        throw std::runtime_error(ostr.str());
      }
      if ((status = nc_put_att_text(ncid, coordid[i], "units", strlen("m"), "m")))
      {
        std::ostringstream ostr;
        ostr << "Error nc_put_att_text " << COORD_NAME[attributeType][i]
             << ":units: " << nc_strerror(status);
        throw std::runtime_error(ostr.str());
      }
      if (attributeType == vtkDataObject::CELL)
      {
        if ((status = nc_put_att_text(
               ncid, coordid[i], "bounds", BOUNDS_NAME[i].length(), BOUNDS_NAME[i].c_str())))
        {
          std::ostringstream ostr;
          ostr << "Error nc_put_att_text " << COORD_NAME[attributeType][i]
               << ":bounds: " << nc_strerror(status);
          throw std::runtime_error(ostr.str());
        }
      }
    }

    if (attributeType == vtkDataObject::CELL)
    {
      for (int i = 0; i < 3; ++i)
      {
        int boundsdim[2] = { dimid[i], nvid };
        if ((status =
                nc_def_var(ncid, BOUNDS_NAME[i].c_str(), NC_DOUBLE, 2, boundsdim, &boundsid[i])))
        {
          std::ostringstream ostr;
          ostr << "Error nc_def_var " << BOUNDS_NAME[attributeType][i] << ": "
               << nc_strerror(status);
          throw std::runtime_error(ostr.str());
        }
      }
    }
  }

  int DefineAttribute(int ncid, const std::array<int, 3>& dimid, int vtkType, const char* arrayName)
  {
    int attributeid;
    int status, dimidOrder[3];
    int fillValue = this->Obj->GetFillValue();

    // How the array is laid out in memory
    dimidOrder[0] = dimid[2];
    dimidOrder[1] = dimid[1];
    dimidOrder[2] = dimid[0];
    nc_type ncType = VTKTypeToNetCDFType(vtkType);
    if ((status = nc_def_var(ncid, arrayName, ncType, 3, dimidOrder, &attributeid)))
    {
      std::ostringstream ostr;
      ostr << "Error nc_def_var " << arrayName << ": " << nc_strerror(status);
      throw std::runtime_error(ostr.str());
    }
    status = NC_NOERR;
    if (std::string(arrayName).rfind("vtk", 0) > 0)
    {
      // for an array that starts with vtk we don't specify _FillValue
      switch (vtkType)
      {
        case VTK_CHAR:
        case VTK_SIGNED_CHAR:
        case VTK_UNSIGNED_CHAR:
          if (fillValue != NC_FILL_INT)
          {
            unsigned char fillByte = fillValue;
            status = nc_put_att(ncid, attributeid, "_FillValue", NC_BYTE, 1, &fillByte);
          }
          break;
        case VTK_SHORT:
          if (fillValue != NC_FILL_SHORT)
          {
            short fillShort = fillValue;
            status = nc_put_att_short(ncid, attributeid, "_FillValue", NC_SHORT, 1, &fillShort);
          }
          break;
        case VTK_INT:
          if (fillValue != NC_FILL_INT)
          {
            status = nc_put_att_int(ncid, attributeid, "_FillValue", NC_INT, 1, &fillValue);
          }
          break;
        case VTK_FLOAT:
          if (fillValue != NC_FILL_INT)
          {
            float fillFloat = fillValue;
            status = nc_put_att_float(ncid, attributeid, "_FillValue", NC_FLOAT, 1, &fillFloat);
          }
          break;
        case VTK_DOUBLE:
          if (fillValue != NC_FILL_INT)
          {
            double fillDouble = fillValue;
            status = nc_put_att_double(ncid, attributeid, "_FillValue", NC_DOUBLE, 1, &fillDouble);
          }
          break;
        default:
          std::ostringstream ostr;
          ostr << "CF conventions does not support VTK type " << vtkType;
          throw std::runtime_error(ostr.str());
      }
      if (status)
      {
        std::ostringstream ostr;
        ostr << "Error nc_put_att_xxx " << arrayName << ":_FillValue: " << nc_strerror(status);
        throw std::runtime_error(ostr.str());
      }
    }

    if ((status = nc_put_att_text(ncid, attributeid, "grid_mapping", strlen("crs"), "crs")))
    {
      std::ostringstream ostr;
      ostr << "Error nc_put_att_text " << arrayName << ":grid_mapping: " << nc_strerror(status);
      throw std::runtime_error(ostr.str());
    }
    return attributeid;
  }

  void SaveAttribute(int ncid, int attributeType, int varid, vtkDataArray* a)
  {
    int status;
    if (varid < 0)
    {
      return;
    }
    int type = a->GetDataType();
    switch (type)
    {
      case VTK_CHAR:
        status =
          nc_put_var(ncid, varid, (unsigned char*)vtkCharArray::SafeDownCast(a)->GetPointer(0));
        break;
      case VTK_SIGNED_CHAR:
        status = nc_put_var(
          ncid, varid, (unsigned char*)vtkSignedCharArray::SafeDownCast(a)->GetPointer(0));
        break;
      case VTK_UNSIGNED_CHAR: // write to a byte array in netcdf
      {
        unsigned char* p = vtkUnsignedCharArray::SafeDownCast(a)->GetPointer(0);
        if (attributeType == vtkDataObject::CELL && std::string(a->GetName()) == "vtkGhostType")
        {
          vtkNew<vtkUnsignedCharArray> newA;
          newA->DeepCopy(a);
          p = newA->GetPointer(0);
          // save a vtkGhostType cell array as a point array
          for (int i = 0; i < a->GetNumberOfTuples(); ++i)
          {
            if (p[i] & vtkDataSetAttributes::HIDDENCELL)
            {
              p[i] ^= vtkDataSetAttributes::HIDDENCELL;
              p[i] |= vtkDataSetAttributes::HIDDENPOINT;
            }
          }
          status = nc_put_var(ncid, varid, p);
        }
        else
        {
          status = nc_put_var(ncid, varid, p);
        }
      }
      break;
      case VTK_SHORT:
        status = nc_put_var_short(ncid, varid, vtkShortArray::SafeDownCast(a)->GetPointer(0));
        break;
      case VTK_INT:
        status = nc_put_var_int(ncid, varid, vtkIntArray::SafeDownCast(a)->GetPointer(0));
        break;
      case VTK_FLOAT:
        status = nc_put_var_float(ncid, varid, vtkFloatArray::SafeDownCast(a)->GetPointer(0));
        break;
      case VTK_DOUBLE:
        status = nc_put_var_double(ncid, varid, vtkDoubleArray::SafeDownCast(a)->GetPointer(0));
        break;
      default:
        std::ostringstream ostr;
        ostr << "CF conventions does not support VTK type " << type;
        throw std::runtime_error(ostr.str());
    }
    if (status)
    {
      std::ostringstream ostr;
      ostr << "Error nc_put_var type(" << type << ") " << a->GetName() << ": "
           << nc_strerror(status);
      vtkWarningWithObjectMacro(this->Obj, << ostr.str());
    }
  }

  std::map<std::string, std::string> StringAttributes;
  std::map<std::string, double> DoubleAttributes;
  vtkNetCDFCFWriter* Obj;
};

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkNetCDFCFWriter);

//------------------------------------------------------------------------------
vtkNetCDFCFWriter::vtkNetCDFCFWriter()
{
  this->FileName = nullptr;
  this->CellArrayNamePostfix = nullptr;
  this->SetCellArrayNamePostfix("_c");
  this->FillBlankedAttributes = false;
  this->FillValue = NC_FILL_INT;
  this->AttributeType = vtkDataObject::POINT;
  this->Impl = new Implementation(this);
}

//------------------------------------------------------------------------------
vtkNetCDFCFWriter::~vtkNetCDFCFWriter()
{
  delete this->Impl;
  this->SetCellArrayNamePostfix(nullptr);
  this->SetFileName(nullptr);
}

//------------------------------------------------------------------------------
void vtkNetCDFCFWriter::WriteData()
{
  try
  {
    int status;
    vtkDataSet* dataset = vtkDataSet::SafeDownCast(this->GetInput(0));
    vtkImageData* id = vtkImageData::SafeDownCast(dataset);
    if (!id)
    {
      throw std::runtime_error("Writer expects an input of type vtkImageData");
    }
    vtkDataSetAttributes* attributes = dataset->GetAttributes(this->AttributeType);
    if (!attributes || !attributes->GetNumberOfArrays())
    {
      std::ostringstream ostr;
      ostr << "There are no array for attribute " << this->AttributeType
           << "POINT (0) and CELL (1). Try the other attribute type.";
      throw std::runtime_error(ostr.str());
    }

    if (this->FillBlankedAttributes)
    {
      for (int i = 0; i < attributes->GetNumberOfArrays(); ++i)
      {
        vtkDataArray* array = attributes->GetArray(i);
        std::string name(array->GetName());
        if (name.rfind("vtk", 0) == 0)
        {
          // if its an array that starts with vtk, skip it.
          continue;
        }
        auto newArray = vtk::TakeSmartPointer(array->NewInstance());
        newArray->SetNumberOfTuples(array->GetNumberOfTuples());
        newArray->SetNumberOfComponents(array->GetNumberOfComponents());
        newArray->SetName(array->GetName());
        BlankToFillValue(dataset->GetGhostArray(this->AttributeType), array, newArray,
          this->AttributeType, this->FillValue);
        attributes->AddArray(newArray);
      }
    }

    // needed for POINTs and CELLs
    std::array<std::vector<double>, 3> coords;
    std::array<int, 3> dimid, coordid;
    std::vector<int> attributeid;

    // needed only for CELLs
    std::array<std::vector<std::array<double, 2>>, 3> bounds;
    std::array<int, 3> boundsid;

    // create the nc file
    int ncid = this->Impl->CreateFile();

    GetCoords(id, this->AttributeType, &coords, &bounds);
    this->Impl->DefineCoords(ncid, this->AttributeType, coords, dimid, coordid, boundsid);
    attributeid.resize(attributes->GetNumberOfArrays());
    for (int i = 0; i < attributes->GetNumberOfArrays(); ++i)
    {
      vtkDataArray* a = attributes->GetArray(i);
      if (VTKTypeToNetCDFType(a->GetDataType()) < 0)
      {
        vtkWarningMacro(<< a->GetName()
                        << " has a type not supported by CF conventions: " << a->GetDataType());
        attributeid[i] = -1;
      }
      else
      {
        attributeid[i] = this->Impl->DefineAttribute(ncid, dimid, a->GetDataType(), a->GetName());
      }
    }

    /* End the definition of the files */
    if ((status = nc_enddef(ncid)))
    {
      std::ostringstream ostr;
      ostr << "Error nc_enddef "
           << ": " << nc_strerror(status);
      throw std::runtime_error(ostr.str());
    }

    SaveCoords(ncid, this->AttributeType, coordid, coords, boundsid, bounds);
    for (int i = 0; i < attributes->GetNumberOfArrays(); ++i)
    {
      vtkDataArray* a = attributes->GetArray(i);
      this->Impl->SaveAttribute(ncid, this->AttributeType, attributeid[i], a);
    }

    if ((status = nc_close(ncid)))
    {
      std::ostringstream ostr;
      ostr << "Error nc_close "
           << ": " << nc_strerror(status);
      throw std::runtime_error(ostr.str());
    }
  }
  catch (const std::runtime_error& e)
  {
    vtkErrorMacro(<< e.what());
  }
}

//------------------------------------------------------------------------------
void vtkNetCDFCFWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "FileName: " << (this->GetFileName() ? this->GetFileName() : "(none)") << endl;
  os << indent << "Input: " << this->GetInput(0) << endl;
}

//------------------------------------------------------------------------------
int vtkNetCDFCFWriter::FillInputPortInformation(int port, vtkInformation* info)
{
  if (port == 0)
  {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkImageData");
    return 1;
  }
  return 0;
}

//------------------------------------------------------------------------------
void vtkNetCDFCFWriter::ClearGridMappingAttributes()
{
  this->Impl->StringAttributes.clear();
  this->Impl->DoubleAttributes.clear();
}

//------------------------------------------------------------------------------
void vtkNetCDFCFWriter::AddGridMappingAttribute(const char* name, const char* value)
{
  auto it = GridMappingAttribute.find(name);
  if (it == GridMappingAttribute.end())
  {
    vtkWarningMacro(<< name << " is not a known attribute.");
  }
  else
  {
    if (it->first == "grid_mapping_name")
    {
      auto itMapping = GridMappingName.find(value);
      if (itMapping == GridMappingName.end())
      {
        vtkWarningMacro(<< value << " is not a known grid_mapping_name.");
      }
    }

    if (it->second == Double)
    {
      vtkWarningMacro(<< name << " should have a double value.");
    }
  }
  this->Impl->StringAttributes[name] = value;
}

//------------------------------------------------------------------------------
void vtkNetCDFCFWriter::AddGridMappingAttribute(const char* name, double value)
{
  auto it = GridMappingAttribute.find(name);
  if (it == GridMappingAttribute.end())
  {
    vtkWarningMacro(<< name << " is not a known attribute.");
  }
  else
  {
    if (it->second == String)
    {
      vtkWarningMacro(<< name << " should have a string value.");
    }
  }
  this->Impl->DoubleAttributes[name] = value;
}
VTK_ABI_NAMESPACE_END
