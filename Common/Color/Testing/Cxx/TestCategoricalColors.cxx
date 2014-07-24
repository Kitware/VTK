/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestCategoricalColors.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkLookupTable.h"
#include "vtkColorSeries.h"
#include "vtkDoubleArray.h"
#include "vtkUnsignedCharArray.h"

#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include <map>

#include <cstdio> // For EXIT_SUCCESS

//-----------------------------------------------------------------------------
//! Get a hexadecimal string of the RGBA colors.
std::string RGBAToHexString(const unsigned char *rgba)
{
  std::ostringstream os;
  for (int i = 0; i < 4; ++i)
  {
    os << std::setw(2) << std::setfill('0')
    << std::hex << static_cast<int>(rgba[i]);
  }
  return os.str();
}

int TestCategoricalColors(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  bool res = true;
  // Create the LUT and add some annotations.
  vtkLookupTable* lut = vtkLookupTable::New();
  lut->SetAnnotation(0., "Zero");
  lut->SetAnnotation(.5, "Half");
  lut->SetAnnotation(1., "Ichi");
  lut->SetAnnotation(1., "One");
  lut->SetAnnotation(2., "Ni");
  lut->SetAnnotation(2., "Two");
  lut->SetAnnotation(3, "San");
  lut->SetAnnotation(4, "Floor");
  lut->SetAnnotation(5, "Hive");
  lut->SetAnnotation(6, "Licks");
  lut->SetAnnotation(7, "Leaven");
  lut->SetAnnotation(9, "Kyuu");
  lut->RemoveAnnotation(2.);

  vtkColorSeries* palettes = vtkColorSeries::New();
#if 0
  vtkIdType numSchemes = palettes->GetNumberOfColorSchemes();
  for (vtkIdType i = 0; i < numSchemes; ++ i)
    {
    palettes->SetColorScheme(i);
    std::cout << i << ": " << palettes->GetColorSchemeName() << std::endl;
    }
#endif
  palettes->SetColorSchemeByName("Brewer Qualitative Accent");
  palettes->BuildLookupTable(lut);

  std::map<double, std::string> expectedColors;
  expectedColors[0] = "0x7fc97fff";
  expectedColors[9] = "0x7fc97fff";
  expectedColors[1] = "0xfdc086ff";
  expectedColors[2] = "0x800000ff";
  expectedColors[3] = "0xffff99ff";
  expectedColors[0.5] = "0xbeaed4ff";
  expectedColors[-999] = "0x800000ff"; // NaN

  const unsigned char* rgba = lut->MapValue(0.);
  std::string v = "0x" + RGBAToHexString(rgba);
  if (expectedColors[0] != v)
    {
    std::cout
      << "Fail for "
      << std::setw(3) << std::left
      << 0 << ": got: "
      << v
      << " expected: "
      << expectedColors[0]
      << std::endl;
    res &= false;
    }
  rgba = lut->MapValue(3.);
  v = "0x" + RGBAToHexString(rgba);
  if (expectedColors[3] != v)
    {
    std::cout
      << "Fail for "
      << std::setw(3) << std::left
      << 3 << ": got: "
      << v
      << " expected: "
      << expectedColors[3]
      << std::endl;
    res &= false;
    }

  vtkDoubleArray* data = vtkDoubleArray::New();
  data->InsertNextValue(0.);
  data->InsertNextValue(9.);
  data->InsertNextValue(1.);
  data->InsertNextValue(2.);
  data->InsertNextValue(3.);
  data->InsertNextValue(.5);


  vtkUnsignedCharArray* color = lut->MapScalars(data, VTK_RGBA, 0);
  unsigned char* cval;
  for (vtkIdType i = 0; i < color->GetNumberOfTuples(); ++ i)
    {
    cval = color->GetPointer(i * 4);
    v = "0x" + RGBAToHexString(cval);
    if (expectedColors[data->GetTuple1(i)] != v)
      {
      std::cout
        << "Fail for "
        << std::setw(3) << std::left
        << data->GetTuple1(i) << ": got: "
        << v
        << " expected: "
        << expectedColors[data->GetTuple1(i)]
        << std::endl;
      res &= false;
      }
    }
  cval = lut->GetNanColorAsUnsignedChars();
  v = "0x" + RGBAToHexString(cval);
  if (expectedColors[-999] != v)
    {
    std::cout
      << "Fail for "
      << "NaN: got: "
      << v
      << " expected: "
      << expectedColors[-999]
      << std::endl;
    res &= false;
    }

  color->Delete();
  data->Delete();
  lut->Delete();
  palettes->Delete();

  return (res) ? EXIT_SUCCESS : EXIT_FAILURE;
}
