#include <cstring>

#include "ADIOSScalar.h"
#include "ADIOSUtilities.h"

#include <adios_read.h>

template<typename T>
void DecodeValues(ADIOS_FILE *f, const ADIOS_VARINFO *v,
  std::vector<void*> &values)
{
  for(int t = 0; t < v->nsteps; ++t)
    {
    std::vector<T> *data = new std::vector<T>(v->nblocks[t]);
    values[t] = data;

    for(int b = 0; b < v->nblocks[t]; ++b)
      {
      ADIOS_SELECTION *s = adios_selection_writeblock(b);
      adios_schedule_read_byid(f, s, v->varid, t, 1, &(*data)[b]);
      adios_perform_reads(f, 1);
      adios_selection_delete(s);
      }
    }
}

//----------------------------------------------------------------------------
ADIOSScalar::ADIOSScalar(ADIOS_FILE *f, ADIOS_VARINFO *v)
: Id(v->varid), Type(ADIOSUtilities::TypeADIOSToVTK(v->type)),
  NumSteps(v->nsteps), Name(f->var_namelist[v->varid]), Values(v->nsteps)
{
  int err;

  err = adios_inq_var_stat(f, v, 1, 1);
  ADIOSUtilities::TestReadErrorEq<int>(0, err);

  err = adios_inq_var_blockinfo(f, v);
  ADIOSUtilities::TestReadErrorEq<int>(0, err);

  switch(v->type)
    {
    case adios_byte:             DecodeValues<int8_t>(f, v, this->Values);
      break;
    case adios_short:            DecodeValues<int16_t>(f, v, this->Values);
      break;
    case adios_integer:          DecodeValues<int32_t>(f, v, this->Values);
      break;
    case adios_long:             DecodeValues<vtkIdType>(f, v, this->Values);
      break;
    case adios_unsigned_byte:    DecodeValues<uint8_t>(f, v, this->Values);
      break;
    case adios_unsigned_short:   DecodeValues<uint16_t>(f, v, this->Values);
      break;
    case adios_unsigned_integer: DecodeValues<uint32_t>(f, v, this->Values);
      break;
    case adios_unsigned_long:    DecodeValues<uint64_t>(f, v, this->Values);
      break;
    case adios_real:             DecodeValues<float>(f, v, this->Values);
      break;
    case adios_double:           DecodeValues<double>(f, v, this->Values);
      break;
    default: break;
    }
}

//----------------------------------------------------------------------------
template<typename T>
void Cleanup(std::vector<void*>& values)
{
  for(std::vector<void*>::iterator i = values.begin(); i != values.end(); ++i)
    {
    delete reinterpret_cast<std::vector<T>*>(*i);
    *i = NULL;
    }
  values.clear();
}

ADIOSScalar::~ADIOSScalar(void)
{
  switch(this->Type)
    {
    case VTK_TYPE_INT8:    Cleanup<int8_t>(this->Values); break;
    case VTK_TYPE_INT16:   Cleanup<int16_t>(this->Values); break;
    case VTK_TYPE_INT32:   Cleanup<int32_t>(this->Values); break;
    case VTK_ID_TYPE:      Cleanup<vtkIdType>(this->Values); break;
    case VTK_TYPE_UINT8:   Cleanup<uint8_t>(this->Values); break;
    case VTK_TYPE_UINT16:  Cleanup<uint16_t>(this->Values); break;
    case VTK_TYPE_UINT32:  Cleanup<uint32_t>(this->Values); break;
    case VTK_TYPE_UINT64:  Cleanup<uint64_t>(this->Values); break;
    case VTK_TYPE_FLOAT32: Cleanup<float>(this->Values); break;
    case VTK_TYPE_FLOAT64: Cleanup<double>(this->Values); break;
    default: break;
    }
}

//----------------------------------------------------------------------------
template<typename TN>
const std::vector<TN>& ADIOSScalar::GetValues(int step) const
{
  if(ADIOSUtilities::TypeNativeToVTK<TN>::T != this->Type)
    {
    throw std::invalid_argument("Incompatible type");
    }
  return *reinterpret_cast<std::vector<TN>*>(this->Values[step]);
}
#define INSTANTIATE(T) \
template const std::vector<T>& ADIOSScalar::GetValues(int) const;
INSTANTIATE(int8_t)
INSTANTIATE(int16_t)
INSTANTIATE(int32_t)
INSTANTIATE(vtkIdType)
INSTANTIATE(uint8_t)
INSTANTIATE(uint16_t)
INSTANTIATE(uint32_t)
INSTANTIATE(uint64_t)
INSTANTIATE(float)
INSTANTIATE(double)
#undef INSTANTIATE
