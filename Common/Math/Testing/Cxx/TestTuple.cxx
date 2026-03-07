// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkLogger.h"
#include "vtkTuple.h"

#include <array>
#include <cmath>
#include <sstream>
#include <string>
#include <type_traits>

// Returns a tolerance value appropriate for the type being tested. For floating point
// types, this is a small value to allow for fuzzy comparisons. For integral types, this
// is 1, meaning that values must be exactly equal to be considered a match.
//
// \return A tolerance value of the appropriate type for comparisons in tests.
template <typename T>
T GetTupleTolerance()
{
  if (std::is_floating_point<T>::value)
  {
    return static_cast<T>(1e-5);
  }
  return static_cast<T>(1);
}

// Fill the tuple with some data values based on the index
//
// \param index The index of the element for which to generate a value.
// \return A value of the appropriate type for the given index.
template <typename T>
T GetTupleValue(int index)
{
  return static_cast<T>(index * 3 + 1);
}

// Exercises and tests the core functionality of vtkTuple for a given data type and size.
// Tests include: default construction, data access via operator[], const accessors,
// scalar construction, construction from std::array, construction from C array,
// comparison operations (Compare, operator==, operator!=), type casting to double,
// and stream output formatting.
//
// \param label A descriptive string for error messages identifying the test case.
// \return 0 if all tests pass, otherwise the number of test failures.
template <typename T, int Size>
int ExerciseTupleMembers(const std::string& label)
{
  int retVal = 0;

  vtkTuple<T, Size> defaultTuple;
  T* defaultData = defaultTuple.GetData();
  for (int i = 0; i < Size; ++i)
  {
    defaultData[i] = GetTupleValue<T>(i);
  }

  if (defaultTuple.GetSize() != Size)
  {
    vtkLog(ERROR, << label << ": GetSize() returned " << defaultTuple.GetSize() << ", expected "
                  << Size);
    ++retVal;
  }

  for (int i = 0; i < Size; ++i)
  {
    if (defaultTuple[i] != GetTupleValue<T>(i))
    {
      vtkLog(ERROR, << label << ": operator[] write/read failed at index " << i);
      ++retVal;
      break;
    }
  }

  const vtkTuple<T, Size>& constRef = defaultTuple;
  const T* constData = constRef.GetData();
  for (int i = 0; i < Size; ++i)
  {
    if (constData[i] != defaultTuple[i] || constRef(i) != defaultTuple[i])
    {
      vtkLog(ERROR, << label << ": const accessors failed at index " << i);
      ++retVal;
      break;
    }
  }

  vtkTuple<T, Size> scalarTuple(static_cast<T>(7));
  for (int i = 0; i < Size; ++i)
  {
    if (scalarTuple[i] != static_cast<T>(7))
    {
      vtkLog(ERROR, << label << ": scalar constructor failed at index " << i);
      ++retVal;
      break;
    }
  }

  std::array<T, Size> stdArray;
  for (int i = 0; i < Size; ++i)
  {
    stdArray[i] = GetTupleValue<T>(i + 10);
  }
  vtkTuple<T, Size> stdArrayTuple(stdArray);
  for (int i = 0; i < Size; ++i)
  {
    if (stdArrayTuple[i] != stdArray[i])
    {
      vtkLog(ERROR, << label << ": std::array constructor failed at index " << i);
      ++retVal;
      break;
    }
  }

  T cArray[Size];
  for (int i = 0; i < Size; ++i)
  {
    cArray[i] = GetTupleValue<T>(i + 20);
  }
  vtkTuple<T, Size> cArrayTuple(cArray);
  for (int i = 0; i < Size; ++i)
  {
    if (cArrayTuple[i] != cArray[i])
    {
      vtkLog(ERROR, << label << ": pointer constructor failed at index " << i);
      ++retVal;
      break;
    }
  }

  vtkTuple<T, Size> equalTuple(defaultTuple.GetData());
  if (!defaultTuple.Compare(equalTuple, GetTupleTolerance<T>()))
  {
    vtkLog(ERROR, << label << ": Compare() failed for equal tuples");
    ++retVal;
  }

  vtkTuple<T, Size> differentTuple(defaultTuple.GetData());
  differentTuple[0] = static_cast<T>(differentTuple[0] + static_cast<T>(2));
  if (defaultTuple.Compare(differentTuple, GetTupleTolerance<T>()))
  {
    vtkLog(ERROR, << label << ": Compare() should have failed for different tuples");
    ++retVal;
  }

  if (!(defaultTuple == equalTuple))
  {
    vtkLog(ERROR, << label << ": operator== failed for equal tuples");
    ++retVal;
  }

  if (defaultTuple != equalTuple)
  {
    vtkLog(ERROR, << label << ": operator!= failed for equal tuples");
    ++retVal;
  }

  if (!(defaultTuple != differentTuple))
  {
    vtkLog(ERROR, << label << ": operator!= failed for different tuples");
    ++retVal;
  }

  const vtkTuple<double, Size> castTuple = defaultTuple.template Cast<double>();
  for (int i = 0; i < Size; ++i)
  {
    if (std::abs(castTuple[i] - static_cast<double>(defaultTuple[i])) > 1e-12)
    {
      vtkLog(ERROR, << label << ": Cast<double>() failed at index " << i);
      ++retVal;
      break;
    }
  }

  std::ostringstream stream;
  stream << defaultTuple;
  const std::string streamOutput = stream.str();

  if (streamOutput.size() < 2 || streamOutput.front() != '(' || streamOutput.back() != ')')
  {
    vtkLog(ERROR, << label << ": stream operator produced unexpected output: " << streamOutput);
    ++retVal;
  }

  return retVal;
}

// Tests the stream output specialization for vtkTuple<unsigned char>. Ensures that
// unsigned char values are streamed as numeric values rather than as ASCII characters.
//
// \return 0 if all tests pass, otherwise the number of test failures.
template <int Size>
int ExerciseUnsignedCharStreamSpecialization()
{
  int retVal = 0;

  vtkTuple<unsigned char, Size> tuple;
  for (int i = 0; i < Size; ++i)
  {
    tuple[i] = static_cast<unsigned char>(65 + i);
  }

  std::ostringstream stream;
  stream << tuple;
  const std::string output = stream.str();

  if (output.find("65") == std::string::npos)
  {
    vtkLog(ERROR,
      "unsigned char stream specialization output did not contain numeric values: " << output);
    ++retVal;
  }

  return retVal;
}

int TestTuple(int, char*[])
{
  int retVal = 0;

  retVal += ExerciseTupleMembers<unsigned char, 1>("vtkTuple<unsigned char, 1>");
  retVal += ExerciseTupleMembers<unsigned char, 3>("vtkTuple<unsigned char, 3>");
  retVal += ExerciseTupleMembers<int, 1>("vtkTuple<int, 1>");
  retVal += ExerciseTupleMembers<int, 3>("vtkTuple<int, 3>");
  retVal += ExerciseTupleMembers<float, 1>("vtkTuple<float, 1>");
  retVal += ExerciseTupleMembers<float, 3>("vtkTuple<float, 3>");
  retVal += ExerciseTupleMembers<double, 1>("vtkTuple<double, 1>");
  retVal += ExerciseTupleMembers<double, 3>("vtkTuple<double, 3>");

  retVal += ExerciseUnsignedCharStreamSpecialization<1>();
  retVal += ExerciseUnsignedCharStreamSpecialization<3>();

  return retVal;
}
