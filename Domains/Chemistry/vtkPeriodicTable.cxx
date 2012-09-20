/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPeriodicTable.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

  =========================================================================*/

#include "vtkPeriodicTable.h"

#include "vtkAbstractArray.h"
#include "vtkBlueObeliskData.h"
#include "vtkColor.h"
#include "vtkFloatArray.h"
#include "vtkLookupTable.h"
#include "vtkMutexLock.h"
#include "vtkObjectFactory.h"
#include "vtkStdString.h"
#include "vtkStringArray.h"
#include "vtkUnsignedShortArray.h"

#include <assert.h>
#include <cctype>
#include <cstring>

// Setup static variables
vtkNew<vtkBlueObeliskData> vtkPeriodicTable::BlueObeliskData;

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPeriodicTable);

//----------------------------------------------------------------------------
vtkPeriodicTable::vtkPeriodicTable()
{
  this->BlueObeliskData->GetWriteMutex()->Lock();

  if (!this->BlueObeliskData->IsInitialized())
    {
    this->BlueObeliskData->Initialize();
    }

  this->BlueObeliskData->GetWriteMutex()->Unlock();
}

//----------------------------------------------------------------------------
vtkPeriodicTable::~vtkPeriodicTable()
{
}

//----------------------------------------------------------------------------
void vtkPeriodicTable::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "BlueObeliskData:\n";
  this->BlueObeliskData->PrintSelf(os, indent.GetNextIndent());
}


//----------------------------------------------------------------------------
unsigned short vtkPeriodicTable::GetNumberOfElements()
{
  return this->BlueObeliskData->GetNumberOfElements();
}

//----------------------------------------------------------------------------
const char * vtkPeriodicTable::GetSymbol(const unsigned short atomicNum)
{
  assert(atomicNum <= this->GetNumberOfElements());
  return this->BlueObeliskData->GetSymbols()->GetValue(atomicNum).c_str();
}

//----------------------------------------------------------------------------
const char * vtkPeriodicTable::GetElementName(const unsigned short atomicNum)
{
  assert(atomicNum <= this->GetNumberOfElements());
  return this->BlueObeliskData->GetNames()->GetValue(atomicNum).c_str();
}

//----------------------------------------------------------------------------
unsigned short vtkPeriodicTable::GetAtomicNumber(const vtkStdString &str)
{
  return this->GetAtomicNumber(str.c_str());
}

//----------------------------------------------------------------------------
unsigned short vtkPeriodicTable::GetAtomicNumber(const char *str)
{
  // If the string is null or the BODR object is not initialized, just
  // return 0.
  if (!str)
    {
    return 0;
    }

  // First attempt to just convert the string to an integer. If this
  // works, return the integer
  int atoi_num = atoi(str);
  if (atoi_num > 0 &&
      atoi_num <= static_cast<int>(this->GetNumberOfElements()))
    {
    return static_cast<unsigned short>(atoi_num);
    }

  // Convert str to lowercase
  int i = 0;
  char *lowerStr = new char[strlen(str) + 1];
  strcpy(lowerStr, str);
  while (char &c = lowerStr[i++])
    {
    c = tolower(c);
    }

  // Cache pointers:
  vtkStringArray *lnames = this->BlueObeliskData->GetLowerNames();
  vtkStringArray *lsymbols = this->BlueObeliskData->GetLowerSymbols();
  const unsigned short numElements = this->GetNumberOfElements();

  // Compare with other lowercase strings
  for (unsigned short ind = 0; ind <= numElements; ++ind)
    {
    if (lnames->GetValue(ind).compare(lowerStr) == 0 ||
        lsymbols->GetValue(ind).compare(lowerStr) == 0)
      {
      delete [] lowerStr;
      return ind;
      }
    }

  // Manually test some non-standard names:
  // - Deuterium
  if (strcmp(lowerStr, "d") == 0 ||
      strcmp(lowerStr, "deuterium") == 0 )
    {
    delete [] lowerStr;
    return 1;
    }
  // - Tritium
  else if (strcmp(lowerStr, "t") == 0 ||
           strcmp(lowerStr, "tritium") == 0 )
    {
    delete [] lowerStr;
    return 1;
    }
  // - Aluminum (vs. Aluminium)
  else if (strcmp(lowerStr, "aluminum") == 0)
    {
    delete [] lowerStr;
    return 13;
    }

  delete [] lowerStr;
  return 0;
}

//----------------------------------------------------------------------------
float vtkPeriodicTable::GetCovalentRadius(const unsigned short atomicNum)
{
  assert(atomicNum <= this->GetNumberOfElements());
  return this->BlueObeliskData->GetCovalentRadii()->GetValue(atomicNum);
}

//----------------------------------------------------------------------------
float vtkPeriodicTable::GetVDWRadius(const unsigned short atomicNum)
{
  assert(atomicNum <= this->GetNumberOfElements());
  return this->BlueObeliskData->GetVDWRadii()->GetValue(atomicNum);
}

//----------------------------------------------------------------------------
void vtkPeriodicTable::GetDefaultLUT(vtkLookupTable * lut)
{
  const unsigned short numColors = this->GetNumberOfElements() + 1;
  vtkFloatArray *colors = this->BlueObeliskData->GetDefaultColors();
  lut->SetNumberOfColors(numColors);
  float rgb[3];
  for (vtkIdType i = 0; static_cast<unsigned int>(i) < numColors; ++i)
    {
    colors->GetTupleValue(i, rgb);
    lut->SetTableValue(i, rgb[0], rgb[1], rgb[2]);
    }
}

//----------------------------------------------------------------------------
void vtkPeriodicTable::GetDefaultRGBTuple(unsigned short atomicNum,
 float rgb[3])
{
  this->BlueObeliskData->GetDefaultColors()->GetTupleValue(atomicNum, rgb);
}

//----------------------------------------------------------------------------
vtkColor3f vtkPeriodicTable::GetDefaultRGBTuple(unsigned short atomicNum)
{
  vtkColor3f result;
  this->BlueObeliskData->GetDefaultColors()->GetTupleValue(atomicNum,
                                                           result.GetData());
  return result;
}
