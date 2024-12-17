// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkNetCDFAccessor.h"
#include "vtkDataArray.h"
#include "vtkDoubleArray.h"
#include "vtkObjectFactory.h"
#include "vtk_netcdf.h"

VTK_ABI_NAMESPACE_BEGIN

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkNetCDFAccessor);

int vtkNetCDFAccessor::close(int ncid)
{
  return nc_close(ncid);
}

int vtkNetCDFAccessor::open(const char* path, int omode, int* ncidp)
{
  return nc_open(path, omode, ncidp);
}

const char* vtkNetCDFAccessor::strerror(int ncerr1)
{
  return nc_strerror(ncerr1);
}

int vtkNetCDFAccessor::inq_attlen(int ncid, int varid, const char* name, size_t* lenp)
{
  return nc_inq_attlen(ncid, varid, name, lenp);
}

int vtkNetCDFAccessor::inq_dimlen(int ncid, int dimid, size_t* lenp)
{
  return nc_inq_dimlen(ncid, dimid, lenp);
}

int vtkNetCDFAccessor::inq_dimname(int ncid, int dimid, char* name)
{
  return nc_inq_dimname(ncid, dimid, name);
}

int vtkNetCDFAccessor::inq_nvars(int ncid, int* nvarsp)
{
  return nc_inq_nvars(ncid, nvarsp);
}

int vtkNetCDFAccessor::inq_ndims(int ncid, int* ndimsp)
{
  return nc_inq_ndims(ncid, ndimsp);
}

int vtkNetCDFAccessor::inq_vardimid(int ncid, int varid, int* dimidsp)
{
  return nc_inq_vardimid(ncid, varid, dimidsp);
}

int vtkNetCDFAccessor::inq_varid(int ncid, const char* name, int* varidp)
{
  return nc_inq_varid(ncid, name, varidp);
}

int vtkNetCDFAccessor::inq_varname(int ncid, int varid, char* name)
{
  return nc_inq_varname(ncid, varid, name);
}

int vtkNetCDFAccessor::inq_varndims(int ncid, int varid, int* ndimsp)
{
  return nc_inq_varndims(ncid, varid, ndimsp);
}

int vtkNetCDFAccessor::inq_vartype(int ncid, int varid, int* typep)
{
  return nc_inq_vartype(ncid, varid, typep);
}

int vtkNetCDFAccessor::get_att_text(int ncid, int varid, const char* name, char* value)
{
  return nc_get_att_text(ncid, varid, name, value);
}

int vtkNetCDFAccessor::get_att_double(int ncid, int varid, const char* name, double* value)
{
  return nc_get_att_double(ncid, varid, name, value);
}

int vtkNetCDFAccessor::get_att_float(int ncid, int varid, const char* name, float* value)
{
  return nc_get_att_float(ncid, varid, name, value);
}

int vtkNetCDFAccessor::get_vars(int ncid, int varid, const size_t* startp, const size_t* countp,
  const ptrdiff_t* stridep, int vtkNotUsed(vtkType), vtkIdType numberOfComponents,
  vtkIdType numberOfTuples, vtkDataArray* dataArray)
{
  dataArray->SetNumberOfComponents(numberOfComponents);
  dataArray->SetNumberOfTuples(numberOfTuples);
  return nc_get_vars(ncid, varid, startp, countp, stridep, dataArray->GetVoidPointer(0));
}

int vtkNetCDFAccessor::get_vars(int ncid, int varid, const size_t* startp, const size_t* countp,
  const ptrdiff_t* stridep, void* ip)
{
  return nc_get_vars(ncid, varid, startp, countp, stridep, ip);
}

int vtkNetCDFAccessor::get_vars_double(int ncid, int varid, const size_t* startp,
  const size_t* countp, const ptrdiff_t* stridep, double* ip)
{
  return nc_get_vars_double(ncid, varid, startp, countp, stridep, ip);
}

int vtkNetCDFAccessor::get_var_double(int ncid, int varid, double* ip)
{
  return nc_get_var_double(ncid, varid, ip);
}

void vtkNetCDFAccessor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

bool vtkNetCDFAccessor::GetCoordinates(int ncid, int varId, std::vector<std::string>& coordName)
{
  std::string coordinates;
  if (!this->ReadTextAttribute(ncid, varId, "coordinates", coordinates))
    return false;

  vtksys::SystemTools::Split(coordinates, coordName, ' ');
  return true;
}

bool vtkNetCDFAccessor::ReadTextAttribute(
  int ncid, int varId, const char* name, std::string& result)
{
  size_t length;
  if (this->inq_attlen(ncid, varId, name, &length) != NC_NOERR)
  {
    return false;
  }

  result.resize(length);
  if (length > 0)
  {
    if (this->get_att_text(ncid, varId, name, &result.at(0)) != NC_NOERR)
    {
      return false;
    }
  }
  else
  {
    // If length == 0, then there really is nothing to read. Do nothing
  }

  // The line below seems weird, but it is here for a good reason.  In general,
  // text attributes are not null terminated, so you have to add your own (which
  // the std::string will do for us).  However, sometimes a null terminating
  // character is written in the attribute anyway.  In a C string this is no big
  // deal.  But it means that the std::string has a null character in it and it
  // is technically different than its own C string.  This line corrects that
  // regardless of whether the null string was written we will get the right
  // string.
  // NOLINTNEXTLINE(readability-redundant-string-cstr)
  result = result.c_str();

  return true;
}

bool vtkNetCDFAccessor::NeedsFileName()
{
  return true;
}

VTK_ABI_NAMESPACE_END
