/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkArrayWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkArrayWriter.h"
#include "vtkArrayData.h"
#include "vtkArrayPrint.h"
#include "vtkDenseArray.h"
#include "vtkExecutive.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"
#include "vtkSparseArray.h"
#include "vtkUnicodeString.h"

#include <cmath>
#include <limits>
#include <stdexcept>
#include <sstream>

namespace {

template<typename T>
inline void WriteValue(std::ostream& stream, const T& value)
{
  stream << value;
}

inline void WriteValue(std::ostream& stream, const double& value)
{
  if(std::abs(value) < std::numeric_limits<double>::min())
    stream << 0;
  else
    stream << value;
}

inline void WriteValue(std::ostream& stream, const vtkStdString& value)
{
  stream << value;
}

inline void WriteValue(std::ostream& stream, const vtkUnicodeString& value)
{
  stream << value.utf8_str();
}

void WriteHeader(const vtkStdString& array_type,
                 const vtkStdString& type_name,
                 vtkArray* array,
                 ostream& stream,
                 bool WriteBinary)
{
  // Serialize the array type ...
  stream << array_type << " " << type_name << "\n";

  // Serialize output format, binary or ascii
  WriteBinary ? stream << "binary" <<"\n" : stream << "ascii" << "\n";

  const vtkArrayExtents extents = array->GetExtents();
  const vtkIdType dimensions = array->GetDimensions();

  // Serialize the array name ...
  stream << array->GetName() << "\n";

  // Serialize the array extents and number of non-NULL values ...
  for(vtkIdType i = 0; i != dimensions; ++i)
    stream << extents[i].GetBegin() << " " << extents[i].GetEnd() << " ";
  stream << array->GetNonNullSize() << "\n";

  // Serialize the dimension-label for each dimension ...
  for(vtkIdType i = 0; i != dimensions; ++i)
    stream << array->GetDimensionLabel(i) << "\n";
}

void WriteEndianOrderMark(ostream& stream)
{
  // Serialize an endian-order mark ...
  const vtkTypeUInt32 endian_order = 0x12345678;
  stream.write(
    reinterpret_cast<const char*>(&endian_order),
    sizeof(endian_order));
}

template<typename ValueT>
bool WriteSparseArrayBinary(const vtkStdString& type_name, vtkArray* array, ostream& stream)
{
  vtkSparseArray<ValueT>* const concrete_array = vtkSparseArray<ValueT>::SafeDownCast(array);
  if(!concrete_array)
    return false;

  // Write the array header ...
  WriteHeader("vtk-sparse-array", type_name, array, stream, true);
  WriteEndianOrderMark(stream);

  // Serialize the array NULL value ...
  stream.write(
    reinterpret_cast<const char*>(&concrete_array->GetNullValue()),
    sizeof(ValueT));

  // Serialize the array coordinates ...
  for(vtkIdType i = 0; i != array->GetDimensions(); ++i)
  {
    stream.write(
      reinterpret_cast<char*>(concrete_array->GetCoordinateStorage(i)),
      concrete_array->GetNonNullSize() * sizeof(vtkIdType));
  }

  // Serialize the array values ...
  stream.write(
    reinterpret_cast<char*>(concrete_array->GetValueStorage()),
    concrete_array->GetNonNullSize() * sizeof(ValueT));

  return true;
}

template<>
bool WriteSparseArrayBinary<vtkStdString>(const vtkStdString& type_name, vtkArray* array, ostream& stream)
{
  vtkSparseArray<vtkStdString>* const concrete_array = vtkSparseArray<vtkStdString>::SafeDownCast(array);
  if(!concrete_array)
    return false;

  // Write the array header ...
  WriteHeader("vtk-sparse-array", type_name, array, stream, true);
  WriteEndianOrderMark(stream);

  // Serialize the array NULL value ...
  stream.write(
    concrete_array->GetNullValue().c_str(),
    concrete_array->GetNullValue().size() + 1);

  // Serialize the array coordinates ...
  for(vtkIdType i = 0; i != array->GetDimensions(); ++i)
  {
    stream.write(
      reinterpret_cast<char*>(concrete_array->GetCoordinateStorage(i)),
      concrete_array->GetNonNullSize() * sizeof(vtkIdType));
  }

  // Serialize the array values as a set of packed, NULL-terminated strings ...
  const vtkIdType value_count = array->GetNonNullSize();
  for(vtkIdType n = 0; n != value_count; ++n)
  {
    const vtkStdString& value = concrete_array->GetValueN(n);

    stream.write(
      value.c_str(),
      value.size() + 1);
  }

  return true;
}

template<>
bool WriteSparseArrayBinary<vtkUnicodeString>(const vtkStdString& type_name, vtkArray* array, ostream& stream)
{
  vtkSparseArray<vtkUnicodeString>* const concrete_array = vtkSparseArray<vtkUnicodeString>::SafeDownCast(array);
  if(!concrete_array)
    return false;

  // Write the array header ...
  WriteHeader("vtk-sparse-array", type_name, array, stream, true);
  WriteEndianOrderMark(stream);

  // Serialize the array NULL value ...
  stream.write(
    concrete_array->GetNullValue().utf8_str(),
    strlen(concrete_array->GetNullValue().utf8_str()) + 1);

  // Serialize the array coordinates ...
  for(vtkIdType i = 0; i != array->GetDimensions(); ++i)
  {
    stream.write(
      reinterpret_cast<char*>(concrete_array->GetCoordinateStorage(i)),
      concrete_array->GetNonNullSize() * sizeof(vtkIdType));
  }

  // Serialize the array values as a set of packed, NULL-terminated strings ...
  const vtkIdType value_count = array->GetNonNullSize();
  for(vtkIdType n = 0; n != value_count; ++n)
  {
    const vtkUnicodeString& value = concrete_array->GetValueN(n);

    stream.write(
      value.utf8_str(),
      strlen(value.utf8_str()) + 1);
  }

  return true;
}

template<typename ValueT>
bool WriteDenseArrayBinary(const vtkStdString& type_name, vtkArray* array, ostream& stream)
{
  vtkDenseArray<ValueT>* const concrete_array = vtkDenseArray<ValueT>::SafeDownCast(array);
  if(!concrete_array)
    return false;

  // Write the array header ...
  WriteHeader("vtk-dense-array", type_name, array, stream, true);
  WriteEndianOrderMark(stream);

  // Serialize the array values ...
  stream.write(
    reinterpret_cast<char*>(concrete_array->GetStorage()),
    concrete_array->GetNonNullSize() * sizeof(ValueT));

  return true;
}

template<>
bool WriteDenseArrayBinary<vtkStdString>(const vtkStdString& type_name, vtkArray* array, ostream& stream)
{
  vtkDenseArray<vtkStdString>* const concrete_array = vtkDenseArray<vtkStdString>::SafeDownCast(array);
  if(!concrete_array)
    return false;

  // Write the array header ...
  WriteHeader("vtk-dense-array", type_name, array, stream, true);
  WriteEndianOrderMark(stream);

  // Serialize the array values as a set of packed, NULL-terminated strings ...
  const vtkIdType value_count = array->GetNonNullSize();
  for(vtkIdType n = 0; n != value_count; ++n)
  {
    const vtkStdString& value = concrete_array->GetValueN(n);

    stream.write(
      value.c_str(),
      value.size() + 1);
  }

  return true;
}

template<>
bool WriteDenseArrayBinary<vtkUnicodeString>(const vtkStdString& type_name, vtkArray* array, ostream& stream)
{
  vtkDenseArray<vtkUnicodeString>* const concrete_array = vtkDenseArray<vtkUnicodeString>::SafeDownCast(array);
  if(!concrete_array)
    return false;

  // Write the array header ...
  WriteHeader("vtk-dense-array", type_name, array, stream, true);
  WriteEndianOrderMark(stream);

  // Serialize the array values as a set of packed, NULL-terminated strings ...
  const vtkIdType value_count = array->GetNonNullSize();
  for(vtkIdType n = 0; n != value_count; ++n)
  {
    const vtkUnicodeString& value = concrete_array->GetValueN(n);

    stream.write(
      value.utf8_str(),
      strlen(value.utf8_str()) + 1);
  }

  return true;
}

template<typename ValueT>
bool WriteSparseArrayAscii(const vtkStdString& type_name, vtkArray* array, ostream& stream)
{
  vtkSparseArray<ValueT>* const concrete_array = vtkSparseArray<ValueT>::SafeDownCast(array);
  if(!concrete_array)
    return false;

  // Write the header ...
  WriteHeader("vtk-sparse-array", type_name, array, stream, false);

  // Ensure that floating-point types are serialized with full precision
  if(std::numeric_limits<ValueT>::is_specialized)
    stream.precision(std::numeric_limits<ValueT>::digits10 + 1);

  // Write the array NULL value ...
  WriteValue(stream, concrete_array->GetNullValue());
  stream << "\n";

  // Write the array contents ...
  const vtkIdType dimensions = array->GetDimensions();
  const vtkIdType non_null_size = array->GetNonNullSize();

  vtkArrayCoordinates coordinates;
  for(vtkIdType n = 0; n != non_null_size; ++n)
  {
    array->GetCoordinatesN(n, coordinates);
    for(vtkIdType i = 0; i != dimensions; ++i)
      stream << coordinates[i] << " ";
    WriteValue(stream, concrete_array->GetValueN(n));
    stream << "\n";
  }

  return true;
}

template<typename ValueT>
bool WriteDenseArrayAscii(const vtkStdString& type_name, vtkArray* array, ostream& stream)
{
  vtkDenseArray<ValueT>* const concrete_array = vtkDenseArray<ValueT>::SafeDownCast(array);
  if(!concrete_array)
    return false;

  // Write the header ...
  WriteHeader("vtk-dense-array", type_name, array, stream, false);

  // Write the array contents ...
  const vtkArrayExtents extents = array->GetExtents();

  // Ensure that floating-point types are serialized with full precision
  if(std::numeric_limits<ValueT>::is_specialized)
    stream.precision(std::numeric_limits<ValueT>::digits10 + 1);

  vtkArrayCoordinates coordinates;
  for(vtkArrayExtents::SizeT n = 0; n != extents.GetSize(); ++n)
  {
    extents.GetRightToLeftCoordinatesN(n, coordinates);
    WriteValue(stream, concrete_array->GetValue(coordinates));
    stream << "\n";
  }

  return true;
}

} // End anonymous namespace

vtkStandardNewMacro(vtkArrayWriter);

vtkArrayWriter::vtkArrayWriter() :
  FileName(0),
  Binary(false),
  WriteToOutputString(false)
{
}

vtkArrayWriter::~vtkArrayWriter()
{
  this->SetFileName(0);
}

void vtkArrayWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "FileName: " << (this->FileName ? this->FileName : "(none)") << endl;
  os << indent << "Binary: " << this->Binary << endl;
  os << indent << "WriteToOutputString: " << (this->WriteToOutputString ? "on" : "off") << endl;
  os << indent << "OutputString: " << this->OutputString << endl;
}

int vtkArrayWriter::FillInputPortInformation( int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkArrayData");
  return 1;
}

void vtkArrayWriter::WriteData()
{
  if(this->WriteToOutputString)
  {
    this->OutputString = this->Write(this->Binary > 0 ? true : false);
  }
  else
  {
    this->Write(this->FileName ? this->FileName : "", this->Binary > 0 ? true : false);
  }
}

int vtkArrayWriter::Write()
{
  return Superclass::Write();
}

bool vtkArrayWriter::Write(const vtkStdString& file_name, bool WriteBinary)
{
  ofstream file(file_name.c_str(), std::ios::binary);
  return this->Write(file, WriteBinary);
}

bool vtkArrayWriter::Write(vtkArray* array, const vtkStdString& file_name, bool WriteBinary)
{
  ofstream file(file_name.c_str(), std::ios::binary);
  return vtkArrayWriter::Write(array, file, WriteBinary);
}

bool vtkArrayWriter::Write(ostream& stream, bool WriteBinary)
{
  try
  {
    if(this->GetNumberOfInputConnections(0) != 1)
      throw std::runtime_error("Exactly one input required.");

    vtkArrayData* const array_data = vtkArrayData::SafeDownCast(this->GetExecutive()->GetInputData(0, 0));
    if(!array_data)
      throw std::runtime_error("vtkArrayData input required.");

    if(array_data->GetNumberOfArrays() != 1)
      throw std::runtime_error("vtkArrayData with exactly one array required.");

    vtkArray* const array = array_data->GetArray(static_cast<vtkIdType>(0));
    if(!array)
      throw std::runtime_error("Cannot serialize NULL vtkArray.");

    return this->Write(array, stream, WriteBinary);
  }
  catch(std::exception& e)
  {
    vtkErrorMacro("caught exception: " << e.what());
  }
  return false;
}

bool vtkArrayWriter::Write(vtkArray* array, ostream& stream, bool WriteBinary)
{
  try
  {
    if(!array)
      throw std::runtime_error("Cannot serialize NULL vtkArray.");

    if(WriteBinary)
    {
      if(WriteSparseArrayBinary<vtkIdType>("integer", array, stream)) return true;
      if(WriteSparseArrayBinary<double>("double", array, stream)) return true;
      if(WriteSparseArrayBinary<vtkStdString>("string", array, stream)) return true;
      if(WriteSparseArrayBinary<vtkUnicodeString>("unicode-string", array, stream)) return true;

      if(WriteDenseArrayBinary<vtkIdType>("integer", array, stream)) return true;
      if(WriteDenseArrayBinary<double>("double", array, stream)) return true;
      if(WriteDenseArrayBinary<vtkStdString>("string", array, stream)) return true;
      if(WriteDenseArrayBinary<vtkUnicodeString>("unicode-string", array, stream)) return true;
    }
    else
    {
      if(WriteSparseArrayAscii<vtkIdType>("integer", array, stream)) return true;
      if(WriteSparseArrayAscii<double>("double", array, stream)) return true;
      if(WriteSparseArrayAscii<vtkStdString>("string", array, stream)) return true;
      if(WriteSparseArrayAscii<vtkUnicodeString>("unicode-string", array, stream)) return true;

      if(WriteDenseArrayAscii<vtkIdType>("integer", array, stream)) return true;
      if(WriteDenseArrayAscii<double>("double", array, stream)) return true;
      if(WriteDenseArrayAscii<vtkStdString>("string", array, stream)) return true;
      if(WriteDenseArrayAscii<vtkUnicodeString>("unicode-string", array, stream)) return true;
    }

    throw std::runtime_error(std::string("Unhandled array type: ") + array->GetClassName());
  }
  catch(std::exception& e)
  {
    vtkGenericWarningMacro("caught exception: " << e.what());
  }
  return false;
}

vtkStdString vtkArrayWriter::Write(bool WriteBinary)
{
  std::ostringstream oss;
  this->Write(oss, WriteBinary);
  return oss.str();
}

vtkStdString vtkArrayWriter::Write(vtkArray* array, bool WriteBinary)
{
  std::ostringstream oss;
  vtkArrayWriter::Write(array, oss, WriteBinary);
  return oss.str();
}
