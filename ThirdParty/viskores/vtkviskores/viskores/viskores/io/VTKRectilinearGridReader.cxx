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

#include <viskores/io/VTKRectilinearGridReader.h>

#include <viskores/cont/ArrayCopy.h>

namespace viskores
{
namespace io
{

VTKRectilinearGridReader::VTKRectilinearGridReader(const char* fileName)
  : VTKDataSetReaderBase(fileName)
{
}

VTKRectilinearGridReader::VTKRectilinearGridReader(const std::string& fileName)
  : VTKDataSetReaderBase(fileName)
{
}

void VTKRectilinearGridReader::Read()
{
  if (this->DataFile->Structure != viskores::io::internal::DATASET_RECTILINEAR_GRID)
    throw viskores::io::ErrorIO("Incorrect DataSet type");

  //We need to be able to handle VisIt files which dump Field data
  //at the top of a VTK file
  std::string tag;
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

  //Read the points.
  std::string fileStorageDataType;
  std::size_t numPoints[3];
  viskores::cont::UnknownArrayHandle X, Y, Z;

  this->DataFile->Stream >> tag >> numPoints[0] >> fileStorageDataType >> std::ws;
  if (tag != "X_COORDINATES")
    throw viskores::io::ErrorIO("X_COORDINATES tag not found");

  // In binary mode, we must read the data as they are stored in the file.
  // In text mode we can parse as FloatDefault no matter the precision of the storage.
  X = this->DoReadArrayVariant(
    viskores::cont::Field::Association::Any, fileStorageDataType, numPoints[0], 1);


  this->DataFile->Stream >> tag >> numPoints[1] >> fileStorageDataType >> std::ws;
  if (tag != "Y_COORDINATES")
    throw viskores::io::ErrorIO("Y_COORDINATES tag not found");

  Y = this->DoReadArrayVariant(
    viskores::cont::Field::Association::Any, fileStorageDataType, numPoints[1], 1);

  this->DataFile->Stream >> tag >> numPoints[2] >> fileStorageDataType >> std::ws;
  if (tag != "Z_COORDINATES")
    throw viskores::io::ErrorIO("Z_COORDINATES tag not found");

  Z = this->DoReadArrayVariant(
    viskores::cont::Field::Association::Any, fileStorageDataType, numPoints[2], 1);


  if (dim !=
      viskores::Id3(static_cast<viskores::Id>(numPoints[0]),
                    static_cast<viskores::Id>(numPoints[1]),
                    static_cast<viskores::Id>(numPoints[2])))
    throw viskores::io::ErrorIO("DIMENSIONS not equal to number of points");

  viskores::cont::ArrayHandleCartesianProduct<viskores::cont::ArrayHandle<viskores::FloatDefault>,
                                              viskores::cont::ArrayHandle<viskores::FloatDefault>,
                                              viskores::cont::ArrayHandle<viskores::FloatDefault>>
    coords;

  // We need to store all coordinate arrays as FloatDefault.
  viskores::cont::ArrayHandle<viskores::FloatDefault> Xc, Yc, Zc;
  // But the UnknownArrayHandle has type fileStorageDataType.
  viskores::cont::ArrayCopyShallowIfPossible(X, Xc);
  viskores::cont::ArrayCopyShallowIfPossible(Y, Yc);
  viskores::cont::ArrayCopyShallowIfPossible(Z, Zc);

  coords = viskores::cont::make_ArrayHandleCartesianProduct(Xc, Yc, Zc);
  viskores::cont::CoordinateSystem coordSys("coordinates", coords);
  this->DataSet.AddCoordinateSystem(coordSys);
  this->DataSet.SetCellSet(internal::CreateCellSetStructured(dim));

  // Read points and cell attributes
  this->ReadAttributes();
}
}
} // namespace viskores::io
