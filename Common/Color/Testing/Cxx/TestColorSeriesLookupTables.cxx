/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestColorSeriesLookupTables.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkLookupTable.h"
#include "vtkColorSeries.h"
#include "vtkSmartPointer.h"
#include "vtkDoubleArray.h"
#include "vtkUnsignedCharArray.h"
#include "vtkVariantArray.h"

#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>

#include <cstdio> // For EXIT_SUCCESS

//-----------------------------------------------------------------------------
//! Get a string of [R, G, B, A] as double.
std::string RGBAToDoubleString(double *rgba)
{
  std::ostringstream os;
  os << "[";
  for (int i = 0; i < 4; ++i)
    {
    if (i == 0)
      {
      os << std::fixed << std::setw(8) << std::setprecision(6) << rgba[i];
      }
    else
      {
      os << std::fixed << std::setw(9) << std::setprecision(6) << rgba[i];
      }
    if (i < 3)
      {
      os << ",";
      }
    }
  os << "]";
  return os.str();
}

//-----------------------------------------------------------------------------
//! Get a string of [R, G, B, A] as unsigned char.
std::string RGBAToCharString(double *rgba)
{
  std::ostringstream os;
  os << "[";
  for (int i = 0; i < 4; ++i)
    {
    if (i == 0)
      {
      os << std::setw(3) << static_cast<int>(rgba[i] * 255);
      }
    else
      {
      os << std::setw(4) << static_cast<int>(rgba[i] * 255);
      }
    if (i < 3)
      {
      os << ",";
      }
    }
  os << "]";
  return os.str();
}

//-----------------------------------------------------------------------------
//! Get a hexadecimal string of the RGB colors.
std::string RGBToHexString(const double *rgba)
{
  std::ostringstream os;
  for (int i = 0; i < 3; ++i)
  {
    os << std::setw(2) << std::setfill('0')
    << std::hex << static_cast<int>(rgba[i] * 255);
  }
  return os.str();
}

//-----------------------------------------------------------------------------
//! Get a hexadecimal string of the RGBA colors.
std::string RGBAToHexString(const double *rgba)
{
  std::ostringstream os;
  for (int i = 0; i < 4; ++i)
  {
    os << std::setw(2) << std::setfill('0')
    << std::hex << static_cast<int>(rgba[i] * 255);
  }
  return os.str();
}

//-----------------------------------------------------------------------------
//! Display the contents of the lookup table.
std::string DisplayOrdinalLUTAsString(vtkLookupTable *lut)
{
  vtkIdType tv = lut->GetNumberOfTableValues();
  double dR[2];
  lut->GetTableRange(dR);
  std::ostringstream os;
  os << "Lookup Table\nNmber of values : " << std::setw(2) << tv
    << " Table Range: " << std::fixed
    << std::setw(8) << std::setprecision(6)
    << dR[0] << " to " << dR[1] << std::endl;
  std::vector<double> indices;
  for (int i = 0; i < tv; ++i)
    {
    indices.push_back((dR[1] - dR[0]) * i / tv + dR[0]);
    }
  for (std::vector<double>::const_iterator p = indices.begin();
       p != indices.end(); ++p)
    {
    double rgba[4];
    lut->GetColor(*p, rgba);
    rgba[3] = lut->GetOpacity(*p);
    os << RGBAToDoubleString(rgba);
    os << " ";
    os << RGBAToCharString(rgba);
    os << " 0x";
    os << RGBToHexString(rgba);
    os << " ";
    os << std::endl;
    }
  return os.str();
}

//-----------------------------------------------------------------------------
//! Display the contents of the lookup table.
std::string DisplayCategoricalLUTAsString(vtkLookupTable *lut)
{
  vtkIdType tv = lut->GetNumberOfTableValues();
  double dR[2];
  lut->GetTableRange(dR);
  std::ostringstream os;
  os << "Lookup Table\nNmber of values : " << std::setw(2) << tv
    << " Table Range: " << std::fixed << std::setw(8) << std::setprecision(6)
    << dR[0] << " to " << dR[1] << std::endl;
  for (vtkIdType i = 0; i < tv; ++i)
    {
    const unsigned char* cval = lut->MapValue(i);
    double rgba[4];
    for (int j = 0; j < 4; ++j)
    {
      rgba[j] = cval[j] / 255.0;
    }
    os << RGBAToDoubleString(rgba);
    os << " ";
    os << RGBAToCharString(rgba);
    os << " 0x";
    os << RGBToHexString(rgba);
    os << " ";
    os << std::endl;
    }
  return os.str();
}

//-----------------------------------------------------------------------------
//! Compare two ordinal lookup tables.
std::pair<bool, std::string> CompareOrdinalLUTs(vtkLookupTable *lut1,
                                                vtkLookupTable *lut2)
{
  std::pair<bool, std::string> res(true, "");
  if (lut1->GetNumberOfTableValues() != lut2->GetNumberOfTableValues())
    {
    res.first = false;
    res.second = "Table values do not match.";
    }
  else
    {
    double dR1[2];
    lut1->GetTableRange(dR1);
    double dR2[2];
    lut2->GetTableRange(dR2);
    if (dR1[0] != dR2[0] && dR2[1] != dR1[1])
      {
      res.first = false;
      res.second = "Table ranges do not match.";
      }
    else
      {
      vtkIdType tv = lut1->GetNumberOfTableValues();
      double dR[2];
      lut1->GetTableRange(dR);
      std::vector<double> indices;
      for (int i = 0; i < tv; ++i)
        {
        indices.push_back((dR[1] - dR[0]) * i / tv + dR[0]);
        }
      for (std::vector<double>::const_iterator p = indices.begin();
           p != indices.end(); ++p)
        {
        double rgba1[4];
        lut1->GetColor(*p, rgba1);
        rgba1[3] = lut1->GetOpacity(*p);
        double rgba2[4];
        lut2->GetColor(*p, rgba2);
        rgba2[3] = lut2->GetOpacity(*p);
        bool areEquivalent = true;
        for (int i = 0; i < 4; ++i)
          {
          areEquivalent &= rgba1[i] == rgba2[i];
          if (!areEquivalent)
            {
            break;
            }
          }
        if (!areEquivalent)
          {
          res.first = false;
          res.second = "Colors do not match.";
          break;
          }
        }
      }
    }
  return res;
}

//-----------------------------------------------------------------------------
//! Compare two categorical lookup tables.
std::pair<bool, std::string> CompareCategoricalLUTs(vtkLookupTable *lut1,
                                                    vtkLookupTable *lut2)
{
  std::pair<bool, std::string> res(true, "");
  if (lut1->GetNumberOfTableValues() != lut2->GetNumberOfTableValues())
    {
    res.first = false;
    res.second = "Table values do not match.";
    }
  else
    {
    double dR1[2];
    lut1->GetTableRange(dR1);
    double dR2[2];
    lut2->GetTableRange(dR2);
    if (dR1[0] != dR2[0] && dR2[1] != dR1[1])
      {
      res.first = false;
      res.second = "Table ranges do not match.";
      }
    else
      {
      vtkIdType tv = lut1->GetNumberOfTableValues();
      double dR[2];
      lut1->GetTableRange(dR);
      std::vector<double> indices;
      for (int i = 0; i < tv; ++i)
        {
        indices.push_back((dR[1] - dR[0]) * i / tv + dR[0]);
        }
      vtkSmartPointer<vtkDoubleArray> data =
       vtkSmartPointer<vtkDoubleArray>::New();
      for (std::vector<double>::const_iterator p = indices.begin();
           p != indices.end(); ++p)
        {
        data->InsertNextValue(*p);
        }
      vtkUnsignedCharArray *color1 =
        lut1->MapScalars(data, VTK_RGBA, 0);
      vtkUnsignedCharArray *color2 =
        lut2->MapScalars(data, VTK_RGBA, 0);
      unsigned char* cval1;
      unsigned char* cval2;
      for (vtkIdType i = 0; i < color1->GetNumberOfTuples(); ++i)
        {
        cval1 = color1->GetPointer(i * 4);
        cval2 = color2->GetPointer(i * 4);
        bool areEquivalent = true;
        for (int j = 0; j < 4; ++j)
          {
          areEquivalent &= cval1[j] == cval2[j];
          if (!areEquivalent)
            {
            break;
            }
          }
        if (!areEquivalent)
          {
          res.first = false;
          res.second = "Colors do not match.";
          break;
          }
        }
      color1->Delete();
      color2->Delete();
    }
    }
  return res;
}

int TestColorSeriesLookupTables(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  std::string line("-----------------------------------------------------------------------------\n");
  bool res = true;
  vtkSmartPointer<vtkColorSeries> colorSeries =
   vtkSmartPointer<vtkColorSeries>::New();
  int colorSeriesEnum = colorSeries->BREWER_DIVERGING_BROWN_BLUE_GREEN_10;
  colorSeries->SetColorScheme(colorSeriesEnum);
  vtkSmartPointer<vtkLookupTable> lut1 = vtkSmartPointer<vtkLookupTable>::New();
  vtkSmartPointer<vtkLookupTable> lut2 = vtkSmartPointer<vtkLookupTable>::New();

  // These next two tables will be categorical tables.
  vtkSmartPointer<vtkLookupTable> lut3 = vtkSmartPointer<vtkLookupTable>::New();
  vtkSmartPointer<vtkLookupTable> lut4 = vtkSmartPointer<vtkLookupTable>::New();
  // For the annotation just use a letter of the alphabet.
  vtkSmartPointer <vtkVariantArray> values =
   vtkSmartPointer <vtkVariantArray>::New();
  std::string str = "abcdefghijklmnopqrstuvwxyz";
  for (size_t i = 0; i < 10; ++i)
    {
    values->InsertNextValue(vtkVariant(str.substr(i, 1)));
    }
  for (int i = 0; i < values->GetNumberOfTuples(); ++i)
    {
    lut3->SetAnnotation(i, values->GetValue(i).ToString());
    lut4->SetAnnotation(i, values->GetValue(i).ToString());
    }

  colorSeries->BuildLookupTable(lut1);
  // Convert from categorical to ordinal.
  lut1->IndexedLookupOff();
  colorSeries->BuildLookupTable(lut2,colorSeries->ORDINAL);
  // lut1 & lut2 should be ordinal lookup tables.
  std::pair<bool, std::string> comparison = CompareOrdinalLUTs(lut1, lut2);
  if (!comparison.first)
    {
    std::cout << line;
    std::cout << std::boolalpha << comparison.first << " "
      << std::noboolalpha << comparison.second << std::endl;
    std::cout << "lut1 (ordinal)" << std::endl;
    std::cout << DisplayOrdinalLUTAsString(lut1) << std::endl;
    std::cout << "lut2 (ordinal)" << std::endl;
    std::cout << DisplayOrdinalLUTAsString(lut2) << std::endl;
    std::cout << line;
    res &= comparison.first;
    }
  // lut3 will be categorical.
  colorSeries->BuildLookupTable(lut3, 99);
  // Expecting a fail here as the tables are different.
  // lut2 is ordinal and lut3 is categorical.
  comparison = CompareOrdinalLUTs(lut2, lut3);
  if (comparison.first)
    {
    std::cout << line;
    std::cout << std::boolalpha << comparison.first << " "
      << std::noboolalpha << comparison.second << std::endl;
    std::cout << "lut2 (ordinal)" << std::endl;
    std::cout << DisplayOrdinalLUTAsString(lut2) << std::endl;
    std::cout << "lut3 (categorical)" << std::endl;
    std::cout << DisplayCategoricalLUTAsString(lut3) << std::endl;
    std::cout << line;
    res &= !comparison.first;
    }
  else
    {
    res &= !comparison.first;
    }
  // lut3 and lut4 will be categorical.
  colorSeries->BuildLookupTable(lut4);
  comparison = CompareCategoricalLUTs(lut3, lut4);
  if (!comparison.first)
    {
    std::cout << line;
    std::cout << std::boolalpha << comparison.first << " "
      << std::noboolalpha << comparison.second << std::endl;
    std::cout << "lut3 (categorical)" << std::endl;
    std::cout << DisplayCategoricalLUTAsString(lut3) << std::endl;
    std::cout << "lut4 (categorical)" << std::endl;
    std::cout << DisplayCategoricalLUTAsString(lut4) << std::endl;
    std::cout << line;
    res &= comparison.first;
    }

  return (res) ? EXIT_SUCCESS : EXIT_FAILURE;
}
