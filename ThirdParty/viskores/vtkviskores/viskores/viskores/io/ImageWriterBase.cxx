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

#include <viskores/io/FileUtils.h>
#include <viskores/io/ImageWriterBase.h>

#include <viskores/cont/Logging.h>

namespace viskores
{
namespace io
{

ImageWriterBase::ImageWriterBase(const char* filename)
  : FileName(filename)
{
}

ImageWriterBase::ImageWriterBase(const std::string& filename)
  : FileName(filename)
{
}

ImageWriterBase::~ImageWriterBase() noexcept {}

void ImageWriterBase::WriteDataSet(const viskores::cont::DataSet& dataSet,
                                   const std::string& colorFieldName)
{
  using CellSetType = viskores::cont::CellSetStructured<2>;
  if (!dataSet.GetCellSet().IsType<CellSetType>())
  {
    throw viskores::cont::ErrorBadType(
      "Image writers can only write data sets with 2D structured data.");
  }
  CellSetType cellSet = dataSet.GetCellSet().AsCellSet<CellSetType>();
  viskores::Id2 cellDimensions = cellSet.GetCellDimensions();
  // Number of points is one more in each dimension than number of cells
  viskores::Id width = cellDimensions[0] + 1;
  viskores::Id height = cellDimensions[1] + 1;

  viskores::cont::Field colorField;
  if (!colorFieldName.empty())
  {
    if (!dataSet.HasPointField(colorFieldName))
    {
      throw viskores::cont::ErrorBadValue("Data set does not have requested field " +
                                          colorFieldName);
    }
    colorField = dataSet.GetPointField(colorFieldName);
  }
  else
  {
    // Find a field of the correct type.
    viskores::Id numFields = dataSet.GetNumberOfFields();
    bool foundField = false;
    for (viskores::Id fieldId = 0; fieldId < numFields; ++fieldId)
    {
      colorField = dataSet.GetField(fieldId);
      if ((colorField.GetAssociation() == viskores::cont::Field::Association::Points) &&
          (colorField.GetData().IsType<ColorArrayType>()))
      {
        foundField = true;
        break;
      }
    }
    if (!foundField)
    {
      throw viskores::cont::ErrorBadValue(
        "Data set does not have any fields that look like color data.");
    }
  }

  if (CreateDirectoriesFromFilePath(this->FileName))
  {
    VISKORES_LOG_S(viskores::cont::LogLevel::Info,
                   "Created output directory: " << ParentPath(this->FileName));
  }
  this->Write(width, height, colorField.GetData().AsArrayHandle<ColorArrayType>());
}

} // namespace viskores::io
} // namespace viskores
