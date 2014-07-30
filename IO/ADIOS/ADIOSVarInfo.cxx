#include "ADIOSVarInfo.h"
#include "ADIOSUtilities.h"
#include <cstring>
#include <stdexcept>
#include <adios_read.h>

//----------------------------------------------------------------------------
struct ADIOSVarInfo::ADIOSVarInfoImpl
{
  ADIOSVarInfoImpl(const std::string& name = "", ADIOS_VARINFO *var = NULL)
  : Name(name), Var(var)
  { }

  ~ADIOSVarInfoImpl(void)
  {
    if(this->Var)
      {
      adios_free_varinfo(this->Var);
      }
  }

  std::string Name;
  ADIOS_VARINFO *Var;
};

//----------------------------------------------------------------------------
ADIOSVarInfo::ADIOSVarInfo(const std::string& name, void* v)
: Impl(new ADIOSVarInfo::ADIOSVarInfoImpl(name,
    reinterpret_cast<ADIOS_VARINFO*>(v)))
{
}

//----------------------------------------------------------------------------
ADIOSVarInfo::~ADIOSVarInfo(void)
{
  delete this->Impl;
}

//----------------------------------------------------------------------------
std::string ADIOSVarInfo::GetName(void) const
{
  return this->Impl->Name;
}

//----------------------------------------------------------------------------
int ADIOSVarInfo::GetId(void) const
{
  return this->Impl->Var->varid;
}

//----------------------------------------------------------------------------
int ADIOSVarInfo::GetType(void) const
{
  return ADIOSUtilities::TypeADIOSToVTK(this->Impl->Var->type);
}

//----------------------------------------------------------------------------
size_t ADIOSVarInfo::GetNumSteps(void) const
{
  return this->Impl->Var->nsteps;
}

//----------------------------------------------------------------------------
bool ADIOSVarInfo::IsGlobal(void) const
{
  return this->Impl->Var->global == 1;
}

//----------------------------------------------------------------------------
bool ADIOSVarInfo::IsScalar(void) const
{
  return this->Impl->Var->ndim == 0;
}

//----------------------------------------------------------------------------
void ADIOSVarInfo::GetDims(std::vector<size_t>& dims, int block) const
{
  dims.resize(this->Impl->Var->ndim);
  std::copy(
    this->Impl->Var->blockinfo[block].count,
    this->Impl->Var->blockinfo[block].count+this->Impl->Var->ndim,
    dims.begin());
}

//----------------------------------------------------------------------------
template<typename T>
T ADIOSVarInfo::GetValue(int step) const
{
  if(ADIOSUtilities::TypeNativeToADIOS<T>::T != this->Impl->Var->type)
    {
    throw std::runtime_error("Incompatible type");
    }
  return reinterpret_cast<const T*>(this->Impl->Var->value)[step];
}

template<>
std::string ADIOSVarInfo::GetValue<std::string>(int step) const
{
  if(this->Impl->Var->type != ADIOSUtilities::TypeNativeToADIOS<std::string>::T)
    {
    throw std::runtime_error("Incompatible type");
    }
  return reinterpret_cast<const char**>(this->Impl->Var->value)[step];
}

#define INSTANTIATE(T) template T ADIOSVarInfo::GetValue<T>(int) const;
INSTANTIATE(int8_t)
INSTANTIATE(int16_t)
INSTANTIATE(int32_t)
//INSTANTIATE(int64_t)
INSTANTIATE(uint8_t)
INSTANTIATE(uint16_t)
INSTANTIATE(uint32_t)
INSTANTIATE(uint64_t)
INSTANTIATE(vtkIdType)
INSTANTIATE(float)
INSTANTIATE(double)
#undef INSTANTIATE

//----------------------------------------------------------------------------
template<typename T>
const T* ADIOSVarInfo::GetAllValues(void) const
{
  if(ADIOSUtilities::TypeNativeToADIOS<T>::T != this->Impl->Var->type)
    {
    throw std::runtime_error("Incompatible type");
    }
  return reinterpret_cast<const T*>(this->Impl->Var->value);
}

#define INSTANTIATE(T) \
template const T* ADIOSVarInfo::GetAllValues<T>(void) const;
INSTANTIATE(int8_t)
INSTANTIATE(int16_t)
INSTANTIATE(int32_t)
//INSTANTIATE(int64_t)
INSTANTIATE(uint8_t)
INSTANTIATE(uint16_t)
INSTANTIATE(uint32_t)
INSTANTIATE(uint64_t)
INSTANTIATE(vtkIdType)
INSTANTIATE(float)
INSTANTIATE(double)
#undef INSTANTIATE
