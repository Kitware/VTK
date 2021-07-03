/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTGAReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkTGAReader.h"

#include "vtkObjectFactory.h"

#include <fstream>

vtkStandardNewMacro(vtkTGAReader);

//----------------------------------------------------------------------------
void vtkTGAReader::ExecuteInformation()
{
  this->ComputeInternalFileName(0);
  std::ifstream file(this->InternalFileName, std::ios::binary);

  char header[18];
  file.read(header, 18 * sizeof(char));

  file.close();

  // tmp char needed to avoid strict anti aliasing warning
  char* tmp = &header[8];
  this->DataOrigin[0] = static_cast<double>(*reinterpret_cast<short*>(tmp));
  tmp = &header[10];
  this->DataOrigin[1] = static_cast<double>(*reinterpret_cast<short*>(tmp));
  this->DataOrigin[2] = 0.0;

  this->DataExtent[0] = 0;
  tmp = &header[12];
  this->DataExtent[1] = static_cast<int>(*reinterpret_cast<short*>(tmp) - 1);
  this->DataExtent[2] = 0;
  tmp = &header[14];
  this->DataExtent[3] = static_cast<int>(*reinterpret_cast<short*>(tmp) - 1);

  bool upperLeft = static_cast<bool>((header[17] >> 5) & 1);
  this->SetFileLowerLeft(!upperLeft);

  this->SetHeaderSize(18);
  this->SetDataScalarTypeToUnsignedChar();

  this->SetNumberOfScalarComponents(header[16] / 8);
  this->SwapRBCompsOn();

  this->vtkImageReader2::ExecuteInformation();
}

//----------------------------------------------------------------------------
int vtkTGAReader::CanReadFile(const char* fname)
{
  std::ifstream file(fname, std::ios::binary);

  if (!file.is_open())
  {
    return 0;
  }

  char header[18];
  file.read(header, 18 * sizeof(char));

  // only uncompressed data is supported
  if (header[2] != 2)
  {
    vtkWarningMacro("This TGA file is not supported");
    return 0;
  }

  return 1;
}
