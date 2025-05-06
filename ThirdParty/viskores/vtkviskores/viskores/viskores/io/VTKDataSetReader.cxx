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

#include <viskores/io/VTKDataSetReader.h>

#include <viskores/io/VTKPolyDataReader.h>
#include <viskores/io/VTKRectilinearGridReader.h>
#include <viskores/io/VTKStructuredGridReader.h>
#include <viskores/io/VTKStructuredPointsReader.h>
#include <viskores/io/VTKUnstructuredGridReader.h>

#include <memory>

namespace viskores
{
namespace io
{

VTKDataSetReader::VTKDataSetReader(const char* fileName)
  : VTKDataSetReaderBase(fileName)
{
}

VTKDataSetReader::VTKDataSetReader(const std::string& fileName)
  : VTKDataSetReaderBase(fileName)
{
}

VTKDataSetReader::~VTKDataSetReader() {}

void VTKDataSetReader::PrintSummary(std::ostream& out) const
{
  if (this->Reader)
  {
    this->Reader->PrintSummary(out);
  }
  else
  {
    VTKDataSetReaderBase::PrintSummary(out);
  }
}

void VTKDataSetReader::CloseFile()
{
  if (this->Reader)
  {
    this->Reader->CloseFile();
  }
  else
  {
    VTKDataSetReaderBase::CloseFile();
  }
}

void VTKDataSetReader::Read()
{
  switch (this->DataFile->Structure)
  {
    case viskores::io::internal::DATASET_STRUCTURED_POINTS:
      this->Reader.reset(new VTKStructuredPointsReader(""));
      break;
    case viskores::io::internal::DATASET_STRUCTURED_GRID:
      this->Reader.reset(new VTKStructuredGridReader(""));
      break;
    case viskores::io::internal::DATASET_RECTILINEAR_GRID:
      this->Reader.reset(new VTKRectilinearGridReader(""));
      break;
    case viskores::io::internal::DATASET_POLYDATA:
      this->Reader.reset(new VTKPolyDataReader(""));
      break;
    case viskores::io::internal::DATASET_UNSTRUCTURED_GRID:
      this->Reader.reset(new VTKUnstructuredGridReader(""));
      break;
    default:
      throw viskores::io::ErrorIO("Unsupported DataSet type.");
  }

  this->TransferDataFile(*this->Reader.get());
  this->Reader->Read();
  this->DataSet = this->Reader->GetDataSet();
}
}
} // namespace viskores::io
