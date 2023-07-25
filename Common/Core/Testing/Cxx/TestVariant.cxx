// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov

#include "vtkFloatArray.h"
#include "vtkStringArray.h"
#include "vtkVariant.h"
#include "vtkVariantArray.h"

int TestVariant(int, char*[])
{
  double value = 123456;
  const char* strValue = "123456";
  int errors = 0;
  int type[] = { VTK_INT, VTK_UNSIGNED_INT, VTK_TYPE_INT64, VTK_TYPE_UINT64, VTK_FLOAT, VTK_DOUBLE,
    VTK_STRING };
  int numTypes = 7;

  for (int i = 0; i < numTypes; i++)
  {
    vtkVariant v;
    switch (type[i])
    {
      case VTK_INT:
        v = static_cast<int>(value);
        break;
      case VTK_UNSIGNED_INT:
        v = static_cast<unsigned int>(value);
        break;
      case VTK_TYPE_INT64:
        v = static_cast<vtkTypeInt64>(value);
        break;
      case VTK_TYPE_UINT64:
        v = static_cast<vtkTypeUInt64>(value);
        break;
      case VTK_FLOAT:
        v = static_cast<float>(value);
        break;
      case VTK_DOUBLE:
        v = static_cast<double>(value);
        break;
      case VTK_STRING:
        v = strValue;
        break;
      default:
        continue;
    }
    cerr << "v = " << v << " (" << vtkImageScalarTypeNameMacro(type[i]) << ")\n";
    for (int j = 0; j < numTypes; j++)
    {
      std::string str;
      switch (type[j])
      {
        case VTK_INT:
        {
          int conv = v.ToInt();
          if (conv != static_cast<int>(value))
          {
            cerr << "conversion invalid (" << vtkImageScalarTypeNameMacro(type[i]) << " " << conv
                 << " != " << vtkImageScalarTypeNameMacro(type[j]) << " " << static_cast<int>(value)
                 << ")" << endl;
            errors++;
          }
          break;
        }
        case VTK_UNSIGNED_INT:
        {
          unsigned int conv = v.ToUnsignedInt();
          if (conv != static_cast<unsigned int>(value))
          {
            cerr << "conversion invalid (" << vtkImageScalarTypeNameMacro(type[i]) << " " << conv
                 << " != " << vtkImageScalarTypeNameMacro(type[j]) << " "
                 << static_cast<unsigned int>(value) << ")" << endl;
            errors++;
          }
          break;
        }
        case VTK_TYPE_INT64:
        {
          vtkTypeInt64 conv = v.ToTypeInt64();
          if (conv != static_cast<vtkTypeInt64>(value))
          {
            cerr << "conversion invalid (" << vtkImageScalarTypeNameMacro(type[i]) << " " << conv
                 << " != " << vtkImageScalarTypeNameMacro(type[j]) << " "
                 << static_cast<vtkTypeInt64>(value) << ")" << endl;
            errors++;
          }
          break;
        }
        case VTK_TYPE_UINT64:
        {
          vtkTypeUInt64 conv = v.ToTypeUInt64();
          if (conv != static_cast<vtkTypeUInt64>(value))
          {
            cerr << "conversion invalid (" << vtkImageScalarTypeNameMacro(type[i]) << " " << conv
                 << " != " << vtkImageScalarTypeNameMacro(type[j]) << " "
                 << static_cast<vtkTypeUInt64>(value) << ")" << endl;
            errors++;
          }
          break;
        }
        case VTK_FLOAT:
        {
          float conv = v.ToFloat();
          if (conv != static_cast<float>(value))
          {
            cerr << "conversion invalid (" << vtkImageScalarTypeNameMacro(type[i]) << " " << conv
                 << " != " << vtkImageScalarTypeNameMacro(type[j]) << " "
                 << static_cast<float>(value) << ")" << endl;
            errors++;
          }
          break;
        }
        case VTK_DOUBLE:
        {
          double conv = v.ToDouble();
          if (conv != static_cast<double>(value))
          {
            cerr << "conversion invalid (" << vtkImageScalarTypeNameMacro(type[i]) << " " << conv
                 << " != " << vtkImageScalarTypeNameMacro(type[j]) << " "
                 << static_cast<double>(value) << ")" << endl;
            errors++;
          }
          break;
        }
        case VTK_STRING:
        {
          std::string conv = v.ToString();
          if (conv != strValue)
          {
            cerr << "conversion invalid (" << vtkImageScalarTypeNameMacro(type[i]) << " " << conv
                 << " != " << vtkImageScalarTypeNameMacro(type[j]) << " " << strValue << ")"
                 << endl;
            errors++;
          }
          break;
        }
        default:
          continue;
      }
    }
  }

  vtkVariant flt(0.583f);
  vtkVariant dbl(0.583);
  vtkVariant str("0.583");
  if (!(flt == dbl) || flt < dbl || flt > dbl || !(str == dbl) || str < dbl || str > dbl ||
    !(flt == str) || flt < str || flt > str)
  {
    cerr << "Comparison of dissimilar-precision floats failed.\n";
    errors++;
  }

  vtkVariant doubleToString(103.317);
  if (doubleToString.ToString() != "103.317")
  {
    cerr << "double to string complex conversion failed with default parameters.\n";
    errors++;
  }
  if (doubleToString.ToString(vtkVariant::FIXED_FORMATTING, 8) != "103.31700000")
  {
    cerr << "double to string complex conversion failed with fixed formatting.\n";
    errors++;
  }
  if (doubleToString.ToString(vtkVariant::SCIENTIFIC_FORMATTING, 2) != "1.03e+02")
  {
    cerr << "double to string complex conversion failed with scientific formatting.\n";
    errors++;
  }

  // Regression test: ensure that empty arrays (of the 3 types) survive conversion to numeric.
  // There used to be an incorrect assumption that arrays always had a 0th element.
  {
    vtkNew<vtkFloatArray> emptyArray;
    vtkVariant arrayVariant(emptyArray);
    bool isValid = true;
    short numericValue = arrayVariant.ToShort(&isValid);
    if (isValid || (numericValue != 0))
    {
      cerr << "empty vtkFloatArray should have failed to convert to numeric.\n";
      errors++;
    }
  }
  {
    vtkNew<vtkStringArray> emptyArray;
    vtkVariant arrayVariant(emptyArray);
    bool isValid = true;
    int numericValue = arrayVariant.ToInt(&isValid);
    if (isValid || (numericValue != 0))
    {
      cerr << "empty vtkStringArray should have failed to convert to numeric.\n";
      errors++;
    }
  }
  {
    vtkNew<vtkVariantArray> emptyArray;
    vtkVariant arrayVariant(emptyArray);
    bool isValid = true;
    char numericValue = arrayVariant.ToChar(&isValid);
    if (isValid || (numericValue != 0))
    {
      cerr << "empty vtkVariantArray should have failed to convert to numeric.\n";
      errors++;
    }
  }

  return errors;
}
