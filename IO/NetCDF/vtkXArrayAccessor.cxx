// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkXArrayAccessor.h"

#include "vtkDoubleArray.h"
#include "vtkLogger.h"
#include "vtkObjectFactory.h"
#include "vtkType.h"
#include "vtk_netcdf.h"
#include <algorithm>
#include <iterator>

VTK_ABI_NAMESPACE_BEGIN

namespace
{
void copyToDouble(int vtktype, void* src, double* dst)
{
  switch (vtktype)
  {
    case VTK_UNSIGNED_CHAR:
      *dst = *static_cast<unsigned char*>(src);
      break;
    case VTK_CHAR:
      *dst = *static_cast<char*>(src);
      break;
    case VTK_SHORT:
      *dst = *static_cast<short*>(src);
      break;
    case VTK_UNSIGNED_SHORT:
      *dst = *static_cast<unsigned short*>(src);
      break;
    case VTK_INT:
      *dst = *static_cast<int*>(src);
      break;
    case VTK_UNSIGNED_INT:
      *dst = *static_cast<unsigned int*>(src);
      break;
    case VTK_LONG_LONG:
      *dst = *static_cast<long long*>(src);
      break;
    case VTK_UNSIGNED_LONG_LONG:
      *dst = *static_cast<unsigned long long*>(src);
      break;
    case VTK_FLOAT:
      *dst = *static_cast<float*>(src);
      break;
    case VTK_DOUBLE:
      *dst = *static_cast<double*>(src);
      break;
    default:
      vtkGenericWarningMacro(<< "Unknown VTK type " << vtktype);
  }
}

int NetCDFTypeToVTKType(nc_type type)
{
  switch (type)
  {
    case NC_BYTE:
      return VTK_SIGNED_CHAR;
    case NC_UBYTE:
      return VTK_UNSIGNED_CHAR;
    case NC_CHAR:
      return VTK_CHAR;
    case NC_SHORT:
      return VTK_SHORT;
    case NC_USHORT:
      return VTK_UNSIGNED_SHORT;
    case NC_INT:
      return VTK_INT;
    case NC_UINT:
      return VTK_UNSIGNED_INT;
    case NC_INT64:
      return VTK_LONG_LONG;
    case NC_UINT64:
      return VTK_UNSIGNED_LONG_LONG;
    case NC_FLOAT:
      return VTK_FLOAT;
    case NC_DOUBLE:
      return VTK_DOUBLE;
    case NC_STRING:
      return VTK_STRING;
    default:
      vtkGenericWarningMacro(<< "Unknown netCDF variable type " << type);
      return -1;
  }
}

int VTKSizeof(int vtktype)
{
  switch (vtktype)
  {
    case VTK_UNSIGNED_CHAR:
    case VTK_CHAR:
      return VTK_SIZEOF_CHAR;
    case VTK_SHORT:
    case VTK_UNSIGNED_SHORT:
      return VTK_SIZEOF_SHORT;
    case VTK_INT:
    case VTK_UNSIGNED_INT:
      return VTK_SIZEOF_INT;
    case VTK_LONG:
    case VTK_UNSIGNED_LONG:
      return VTK_SIZEOF_LONG;
    case VTK_LONG_LONG:
    case VTK_UNSIGNED_LONG_LONG:
      return VTK_SIZEOF_LONG_LONG;
    case VTK_FLOAT:
      return VTK_SIZEOF_FLOAT;
    case VTK_DOUBLE:
      return VTK_SIZEOF_DOUBLE;
    default:
      vtkGenericWarningMacro(<< "Unknown VTK type " << vtktype);
      return -1;
  }
}

void nc_strcpy(char* dst, std::string src)
{
  size_t length = std::min(static_cast<size_t>(NC_MAX_NAME), src.length());
  strncpy(dst, src.c_str(), length);
  dst[length] = '\0';
}

};

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkXArrayAccessor);

int vtkXArrayAccessor::close(int vtkNotUsed(ncid))
{
  return NC_NOERR;
}

int vtkXArrayAccessor::open(const char* vtkNotUsed(path), int vtkNotUsed(omode), int* ncidp)
{
  *ncidp = 0;
  return NC_NOERR;
}

const char* vtkXArrayAccessor::strerror(int ncerr1)
{
  return nc_strerror(ncerr1);
}

int vtkXArrayAccessor::inq_dimlen(int vtkNotUsed(ncid), int dimid, size_t* lenp)
{
  if (static_cast<size_t>(dimid) >= Dim.size())
    return NC_EBADDIM;
  *lenp = this->DimLen[dimid];
  return NC_NOERR;
}

int vtkXArrayAccessor::inq_dimname(int vtkNotUsed(ncid), int dimid, char* name)
{
  if (static_cast<size_t>(dimid) >= Dim.size())
    return NC_EBADDIM;
  nc_strcpy(name, this->Dim[dimid]);
  return NC_NOERR;
}

int vtkXArrayAccessor::inq_nvars(int vtkNotUsed(ncid), int* nvarsp)
{
  *nvarsp = static_cast<int>(this->Var.size());
  return NC_NOERR;
}

int vtkXArrayAccessor::inq_ndims(int vtkNotUsed(ncid), int* ndimsp)
{
  *ndimsp = static_cast<int>(this->Dim.size());
  return NC_NOERR;
}

int vtkXArrayAccessor::inq_vardimid(int vtkNotUsed(ncid), int varid, int* dimidsp)
{
  if (static_cast<size_t>(varid) >= this->Var.size())
    return NC_ENOTVAR;

  std::transform(this->VarDims[varid].begin(), this->VarDims[varid].end(), dimidsp,
    [](size_t v) { return static_cast<int>(v); });
  return NC_NOERR;
}

int vtkXArrayAccessor::inq_varid(int vtkNotUsed(ncid), const char* name, int* varidp)
{
  auto it = this->VarIndex.find(name);
  if (it == this->VarIndex.end())
  {
    return NC_ENOTVAR;
  }
  *varidp = static_cast<int>(it->second);
  return NC_NOERR;
}

int vtkXArrayAccessor::inq_varname(int vtkNotUsed(ncid), int varid, char* name)
{
  if (static_cast<size_t>(varid) >= this->Var.size())
    return NC_ENOTVAR;
  nc_strcpy(name, this->Var[varid]);
  return NC_NOERR;
}

int vtkXArrayAccessor::inq_varndims(int vtkNotUsed(ncid), int varid, int* ndimsp)
{
  if (static_cast<size_t>(varid) >= this->Var.size())
    return NC_ENOTVAR;
  *ndimsp = static_cast<int>(VarDims[varid].size());
  return NC_NOERR;
}

int vtkXArrayAccessor::inq_vartype(int vtkNotUsed(ncid), int varid, int* typep)
{
  if (static_cast<size_t>(varid) >= this->Var.size())
    return NC_ENOTVAR;
  *typep = this->VarType[varid];
  return NC_NOERR;
}

int vtkXArrayAccessor::inq_attlen(int vtkNotUsed(ncid), int varid, const char* name, size_t* lenp)
{
  if (static_cast<size_t>(varid) >= this->Var.size())
    return NC_ENOTVAR;
  auto it = Att[varid].find(name);
  if (it == Att[varid].end())
    return NC_ENOTATT;
  auto value = it->second;
  if (value.IsString())
    *lenp = value.ToString().length();
  else if (value.IsVTKObject())
  {
    vtkErrorMacro("Invalid attribute: VTK object");
    *lenp = 1;
  }
  else
    *lenp = 1;
  return NC_NOERR;
}

int vtkXArrayAccessor::get_att_text(int vtkNotUsed(ncid), int varid, const char* name, char* value)
{
  if (static_cast<size_t>(varid) >= this->Var.size())
    return NC_ENOTVAR;
  auto it = Att[varid].find(name);
  if (it == Att[varid].end())
    return NC_ENOTATT;
  auto v = it->second;
  if (v.IsString())
    nc_strcpy(value, v.ToString());
  else
  {
    return NC_ECHAR;
  }
  return NC_NOERR;
}

int vtkXArrayAccessor::get_att_double(
  int vtkNotUsed(ncid), int varid, const char* name, double* value)
{
  if (static_cast<size_t>(varid) >= this->Var.size())
    return NC_ENOTVAR;
  auto it = Att[varid].find(name);
  if (it == Att[varid].end())
    return NC_ENOTATT;
  auto v = it->second;
  if (v.IsDouble())
  {
    *value = v.ToDouble();
  }
  else
  {
    return NC_ERANGE;
  }
  return NC_NOERR;
}

int vtkXArrayAccessor::get_att_float(
  int vtkNotUsed(ncid), int varid, const char* name, float* value)
{
  if (static_cast<size_t>(varid) >= this->Var.size())
    return NC_ENOTVAR;
  auto it = Att[varid].find(name);
  if (it == Att[varid].end())
    return NC_ENOTATT;
  auto v = it->second;
  if (v.IsFloat())
  {
    *value = v.ToFloat();
  }
  else
  {
    return NC_ERANGE;
  }

  return NC_NOERR;
}

void vtkXArrayAccessor::PrintVarValue(const char* name, int varid)
{
  vtkLog(INFO, << name << ": " << this->Var[varid]
               << " Value: " << static_cast<void*>(this->VarValue[varid]));
}

void vtkXArrayAccessor::PrintVarValue(
  const char* name, int varid, const size_t* startp, const size_t* countp)
{
  vtkLog(INFO, << name << ": " << this->Var[varid]
               << " Value: " << static_cast<void*>(this->VarValue[varid]));
  size_t ndims = this->VarDims[varid].size();
  vtkLog(INFO, "startp: ");
  for (size_t i = 0; i < ndims; ++i)
  {
    vtkLog(INFO, << startp[i]);
  }
  vtkLog(INFO, "countp: ");
  for (size_t i = 0; i < ndims; ++i)
  {
    vtkLog(INFO, << countp[i]);
  }
}

bool vtkXArrayAccessor::IsContiguous(int varid, const size_t* startp, const size_t* countp)
{
  // the last dim is the most rapidly varying
  int ndims = static_cast<int>(this->VarDims[varid].size());
  int contiguousDims = 0;
  for (int i = ndims - 1; i >= 0; --i)
  {
    if (startp[i] == 0 && countp[i] == this->DimLen[this->VarDims[varid][i]])
    {
      ++contiguousDims;
    }
    else
    {
      break;
    }
  }
  return (contiguousDims >= ndims - 1);
}

std::vector<size_t> vtkXArrayAccessor::GetDimIncrement(int varid)
{
  size_t ndims = this->VarDims[varid].size();
  std::vector<size_t> dimIncrement;
  if (ndims > 0)
  {
    dimIncrement.resize(ndims);
    dimIncrement[ndims - 1] = 1;
    for (size_t i = ndims - 1; i >= 1; --i)
    {
      dimIncrement[i - 1] = dimIncrement[i] * this->DimLen[this->VarDims[varid][i]];
    }
  }
  return dimIncrement;
}

bool vtkXArrayAccessor::DecrementAndUpdate(size_t varid, std::vector<size_t>& count,
  const size_t* vtkNotUsed(startp), const size_t* countp, const std::vector<size_t>& dimIncrement,
  char*& src)
{
  size_t ndims = count.size();
  // we need i to signed as we test if it becomes less than 0
  int i = static_cast<int>(ndims - 1);
  // first i for which count[i] > 0
  while (i >= 0 && count[i] == 0)
    --i;
  if (i < 0)
  {
    // all count is 0, we are done
    return false;
  }
  --count[i];
  if (i == 0 && count[i] == 0)
  {
    // all count is 0, we are done
    return false;
  }

  size_t j = i + 1;
  size_t length = this->DimLen[this->VarDims[varid][j]];
  length -= countp[j] * dimIncrement[j];
  int vtkType = NetCDFTypeToVTKType(this->VarType[varid]);
  src += length * VTKSizeof(vtkType);

  // count[ndims-1] is always 0
  for (size_t k = j; k < count.size() - 1; ++k)
  {
    count[k] = countp[k];
  }
  return true;
}

void vtkXArrayAccessor::Copy(int varid, const size_t* startp, const size_t* countp, char* dst)
{
  // the last dim is the most rapidly varying
  auto dimIncrement = this->GetDimIncrement(varid);
  size_t ndims = dimIncrement.size();
  std::vector<size_t> count;
  count.resize(ndims);
  std::copy(countp, countp + count.size(), count.data());
  char* src = this->VarValue[varid];
  int vtkType = NetCDFTypeToVTKType(this->VarType[varid]);
  for (size_t i = 0; i < ndims; ++i)
  {
    src += dimIncrement[i] * startp[i] * VTKSizeof(vtkType);
  }
  size_t copyLength = countp[ndims - 1] * VTKSizeof(vtkType);
  count[ndims - 1] = 0;
  do
  {
    std::copy(src, src + copyLength, dst);
    src += copyLength;
    dst += copyLength;
  } while (this->DecrementAndUpdate(varid, count, startp, countp, dimIncrement, src));
}

void vtkXArrayAccessor::GetContiguousStartSize(
  int varid, const size_t* startp, const size_t* countp, char*& arrayStart, vtkIdType& arraySize)
{
  // the last dim is the most rapidly varying
  arraySize = 1;
  auto dimIncrement = this->GetDimIncrement(varid);
  size_t increment = 0;
  for (size_t i = 0; i < dimIncrement.size(); ++i)
  {
    arraySize *= countp[i];
    increment += startp[i] * dimIncrement[i];
  }
  int vtkType = NetCDFTypeToVTKType(this->VarType[varid]);
  arrayStart = this->VarValue[varid] + increment * VTKSizeof(vtkType);
}

int vtkXArrayAccessor::get_vars(int vtkNotUsed(ncid), int varid, const size_t* startp,
  const size_t* countp, const ptrdiff_t* vtkNotUsed(stridep), int vt, vtkIdType numberOfComponents,
  vtkIdType numberOfTuples, vtkDataArray* dataArray)
{
  if (static_cast<size_t>(varid) >= this->Var.size())
    return NC_ENOTVAR;
  size_t ndims = this->VarDims[varid].size();
  if (std::any_of(countp, countp + ndims, [](size_t val) { return val == 0; }))
  {
    vtkErrorMacro("Invalid countp: one of the elements is 0");
    return NC_ERANGE;
  }
  char* arrayStart;
  vtkIdType arraySize;
  int vtkType = NetCDFTypeToVTKType(this->VarType[varid]);
  if (vtkType != vt)
  {
    vtkErrorMacro("Mismatched VTKType: " << vtkType << ", " << vt);
    return NC_ERANGE;
  }

  dataArray->SetNumberOfComponents(numberOfComponents);
  if (this->IsContiguous(varid, startp, countp))
  {
    this->GetContiguousStartSize(varid, startp, countp, arrayStart, arraySize);
    if (arraySize != numberOfComponents * numberOfTuples)
    {
      vtkErrorMacro(
        "Mismatch array size: " << arraySize << ", " << (numberOfComponents * numberOfTuples));
      return NC_ERANGE;
    }
    dataArray->SetVoidArray(arrayStart, arraySize, 1);
  }
  else
  {
    dataArray->SetNumberOfTuples(numberOfTuples);
    char* dst = static_cast<char*>(dataArray->GetVoidPointer(0));
    this->Copy(varid, startp, countp, dst);
  }
  return NC_NOERR;
}

int vtkXArrayAccessor::get_vars(int vtkNotUsed(ncid), int varid, const size_t* startp,
  const size_t* countp, const ptrdiff_t* vtkNotUsed(stridep), void* ip)
{
  if (static_cast<size_t>(varid) >= this->Var.size())
    return NC_ENOTVAR;
  size_t ndims = this->VarDims[varid].size();
  if (std::any_of(countp, countp + ndims, [](size_t val) { return val == 0; }))
  {
    vtkErrorMacro("Invalid countp: one of the elements is 0");
    return NC_ERANGE;
  }

  char* arrayStart;
  vtkIdType arraySize;
  int vtkType = NetCDFTypeToVTKType(this->VarType[varid]);
  if (this->IsContiguous(varid, startp, countp))
  {
    this->GetContiguousStartSize(varid, startp, countp, arrayStart, arraySize);
    std::copy(arrayStart, arrayStart + arraySize * VTKSizeof(vtkType), (char*)ip);
  }
  else
  {
    this->Copy(varid, startp, countp, static_cast<char*>(ip));
  }
  return NC_NOERR;
}

int vtkXArrayAccessor::get_vars_double(int vtkNotUsed(ncid), int varid, const size_t* startp,
  const size_t* countp, const ptrdiff_t* vtkNotUsed(stridep), double* ip)
{
  if (static_cast<size_t>(varid) >= this->Var.size())
    return NC_ENOTVAR;
  size_t ndims = this->VarDims[varid].size();
  if (std::any_of(countp, countp + ndims, [](size_t val) { return val == 0; }))
  {
    vtkErrorMacro("Invalid countp: one of the elements is 0");
    return NC_ERANGE;
  }

  char* arrayStart;
  vtkIdType arraySize;
  int vtkType = NetCDFTypeToVTKType(this->VarType[varid]);
  if (this->IsContiguous(varid, startp, countp))
  {
    this->GetContiguousStartSize(varid, startp, countp, arrayStart, arraySize);
    for (vtkIdType i = 0; i < arraySize; ++i)
      copyToDouble(vtkType, arrayStart + i * VTKSizeof(vtkType), ip + i);
  }
  else
  {
    this->Copy(varid, startp, countp, reinterpret_cast<char*>(ip));
  }
  return NC_NOERR;
}

int vtkXArrayAccessor::get_var_double(int ncid, int varid, double* ip)
{
  if (static_cast<size_t>(varid) >= this->Var.size())
    return NC_ENOTVAR;

  size_t ndims = this->VarDims[varid].size();
  std::vector<size_t> start, count;
  start.resize(ndims, 0);
  count.resize(ndims, 0);
  for (size_t i = 0; i < ndims; ++i)
  {
    count[i] = this->DimLen[this->VarDims[varid][i]];
  }

  return get_vars_double(ncid, varid, start.data(), count.data(), nullptr, ip);
}

void vtkXArrayAccessor::SetDim(const std::vector<std::string>& v)
{
  this->Dim.resize(v.size());
  this->DimLen.resize(v.size());
  std::copy(v.begin(), v.end(), this->Dim.begin());
  for (size_t i = 0; i < this->Dim.size(); ++i)
  {
    this->DimIndex[this->Dim[i]] = i;
  }
}

void vtkXArrayAccessor::SetDimLen(const std::vector<size_t>& v)
{
  std::copy(v.begin(), v.end(), this->DimLen.begin());
}

void vtkXArrayAccessor::SetVar(const std::vector<std::string>& v, const std::vector<int>& isCoord)
{
  if (v.size() != isCoord.size())
  {
    vtkErrorMacro(
      "Var and IsCoord vectors have different size: " << v.size() << " " << isCoord.size());
    return;
  }
  this->Var.resize(v.size());
  this->IsCoord.resize(v.size());
  this->VarValue.resize(v.size());
  std::fill(this->VarValue.begin(), this->VarValue.end(), nullptr);
  this->Att.resize(Var.size());
  this->VarDims.resize(Var.size());
  this->VarCoords.resize(Var.size());
  this->VarType.resize(Var.size());
  std::copy(v.begin(), v.end(), this->Var.begin());
  std::copy(isCoord.begin(), isCoord.end(), this->IsCoord.begin());
  for (size_t i = 0; i < this->Var.size(); ++i)
  {
    this->VarIndex[this->Var[i]] = i;
  }
}

void vtkXArrayAccessor::SetVarValue(size_t varIndex, void* value)
{
  if (varIndex >= this->VarValue.size())
  {
    vtkErrorMacro("Index " << varIndex << " greater than the number of values "
                           << this->VarValue.size() << ". Did you call SetVar first?");
    return;
  }
  this->VarValue[varIndex] = static_cast<char*>(value);
}

void vtkXArrayAccessor::SetAtt(size_t varIndex, std::string attributeName, const vtkVariant& var)
{
  if (varIndex >= this->Att.size())
  {
    vtkErrorMacro("Index " << varIndex << " greater than the number of attributes "
                           << this->Att.size() << ". Did you call SetVar first?");
    return;
  }
  this->Att[varIndex][attributeName] = var;
}

void vtkXArrayAccessor::SetVarType(size_t varIndex, int nctype)
{
  if (varIndex >= this->VarType.size())
  {
    vtkErrorMacro("Index " << varIndex << " greater than the number of types "
                           << this->VarType.size() << ". Did you call SetVar first?");
    return;
  }
  this->VarType[varIndex] = nctype;
}

void vtkXArrayAccessor::SetVarDims(size_t varIndex, const std::vector<size_t>& dims)
{
  if (varIndex >= this->VarDims.size())
  {
    vtkErrorMacro("Index " << varIndex << " greater than the number of VarDimId "
                           << this->VarDims.size() << ". Did you call SetVar first?");
    return;
  }
  this->VarDims[varIndex].resize(dims.size());
  std::copy(dims.begin(), dims.end(), this->VarDims[varIndex].begin());
}

void vtkXArrayAccessor::SetVarCoords(size_t varIndex, const std::vector<size_t>& coords)
{
  if (varIndex >= this->VarDims.size())
  {
    vtkErrorMacro("Index " << varIndex << " greater than the number of VarDimId "
                           << this->VarDims.size() << ". Did you call SetVar first?");
    return;
  }
  this->VarCoords[varIndex].resize(coords.size());
  std::copy(coords.begin(), coords.end(), this->VarCoords[varIndex].begin());
}

void vtkXArrayAccessor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  for (size_t i = 0; i < this->Dim.size(); ++i)
  {
    os << "Dim: " << this->Dim[i] << " Len: " << this->DimLen[i] << endl;
  }
  for (size_t i = 0; i < this->Var.size(); ++i)
  {
    std::string v = this->Var[i];
    os << "Var: " << v << endl;
    os << indent << "IsCoord: " << this->IsCoord[i] << endl;
    os << indent << "VarType: " << this->VarType[i] << endl;
    os << indent << "VarValue: " << static_cast<void*>(this->VarValue[i]) << endl;
    for (auto it = this->Att[i].begin(); it != this->Att[i].end(); ++it)
    {
      os << indent << "Att: " << it->first << " value: " << it->second.ToString() << endl;
    }
    for (size_t j = 0; j < this->VarDims.size(); ++j)
    {
      os << indent << "VarDimId: " << this->VarDims[i][j] << endl;
    }
  }
}

bool vtkXArrayAccessor::GetCoordinates(
  int vtkNotUsed(ncid), int varId, std::vector<std::string>& coordName)
{
  coordName.resize(this->VarCoords[varId].size());
  std::transform(this->VarCoords[varId].begin(), this->VarCoords[varId].end(), coordName.begin(),
    [this](size_t index) { return this->Var[index]; });
  return true;
}

bool vtkXArrayAccessor::NeedsFileName()
{
  return false;
}

bool vtkXArrayAccessor::IsCOARDSCoordinate(std::string name)
{
  auto it = this->VarIndex.find(name);
  if (it == this->VarIndex.end())
  {
    return false;
  }
  size_t nameId = it->second;
  auto nameDims = this->VarDims[nameId];
  if (nameDims.size() != 1)
  {
    return false;
  }
  if (this->Dim[nameDims[0]] != name)
  {
    return false;
  }
  return true;
}

VTK_ABI_NAMESPACE_END
