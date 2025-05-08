//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <viskores/io/VTKStructuredGridReader.h>

namespace viskores
{
namespace io
{

VTKStructuredGridReader::VTKStructuredGridReader(const char* fileName)
  : VTKDataSetReaderBase(fileName)
{
}

VTKStructuredGridReader::VTKStructuredGridReader(const std::string& fileName)
  : VTKDataSetReaderBase(fileName)
{
}

void VTKStructuredGridReader::Read()
{
  if (this->DataFile->Structure != viskores::io::internal::DATASET_STRUCTURED_GRID)
  {
    throw viskores::io::ErrorIO("Incorrect DataSet type");
  }

  std::string tag;

  //We need to be able to handle VisIt files which dump Field data
  //at the top of a VTK file
  this->DataFile->Stream >> tag;
  if (tag == "FIELD")
  {
    this->ReadGlobalFields();
    this->DataFile->Stream >> tag;
  }

  // Read structured grid specific meta-data
  internal::parseAssert(tag == "DIMENSIONS");
  viskores::Id3 dim;
  this->DataFile->Stream >> dim[0] >> dim[1] >> dim[2] >> std::ws;

  this->DataSet.SetCellSet(internal::CreateCellSetStructured(dim));

  // Read the points
  this->DataFile->Stream >> tag;
  internal::parseAssert(tag == "POINTS");
  this->ReadPoints();

  // Read points and cell attributes
  this->ReadAttributes();
}
}
} // namespace viskores::io
