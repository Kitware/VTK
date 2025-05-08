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

#include <viskores/cont/ErrorExecution.h>

#include <viskores/io/FileUtils.h>
#include <viskores/io/ImageReaderBase.h>
#include <viskores/io/ImageReaderPNG.h>
#include <viskores/io/ImageReaderPNM.h>
#include <viskores/io/ImageUtils.h>
#include <viskores/io/ImageWriterBase.h>
#include <viskores/io/ImageWriterPNG.h>
#include <viskores/io/ImageWriterPNM.h>

#include <viskores/cont/ErrorBadValue.h>

#include <memory>

namespace viskores
{
namespace io
{

void WriteImageFile(const viskores::cont::DataSet& dataSet,
                    const std::string& fullPath,
                    const std::string& fieldName)
{
  std::unique_ptr<viskores::io::ImageWriterBase> writer;
  if (EndsWith(fullPath, ".ppm"))
  {
    writer = std::unique_ptr<viskores::io::ImageWriterPNM>(new ImageWriterPNM(fullPath));
  }
  else
  {
    writer = std::unique_ptr<viskores::io::ImageWriterPNG>(new ImageWriterPNG(fullPath));
  }
  writer->WriteDataSet(dataSet, fieldName);
  VISKORES_LOG_S(viskores::cont::LogLevel::Info, "Wrote image data at: " << fullPath);
}

viskores::cont::DataSet ReadImageFile(const std::string& fullPath, const std::string& fieldName)
{
  std::ifstream check(fullPath.c_str());
  if (!check.good())
  {
    throw viskores::cont::ErrorBadValue("File does not exist: " + fullPath);
  }

  std::unique_ptr<viskores::io::ImageReaderBase> reader;
  if (EndsWith(fullPath, ".png"))
  {
    reader = std::unique_ptr<viskores::io::ImageReaderPNG>(new ImageReaderPNG(fullPath));
  }
  else if (EndsWith(fullPath, ".ppm") || EndsWith(fullPath, ".pnm"))
  {
    reader = std::unique_ptr<viskores::io::ImageReaderPNM>(new ImageReaderPNM(fullPath));
  }
  else
  {
    throw viskores::cont::ErrorBadValue("Unsupported file type: " + fullPath);
  }
  reader->SetPointFieldName(fieldName);
  return reader->ReadDataSet();
}

} // namespace viskores::io
} // namespace viskores:
