// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkDataArray.h"
#include "vtkImageData.h"
#include "vtkMetaImageReader.h"
#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkStringFormatter.h"
#include "vtkTestUtilities.h"

#include "vtksys/FStream.hxx"
#include "vtksys/SystemTools.hxx"

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>

namespace
{
constexpr unsigned char Pixels[] = { 0, 1, 2, 3, 127, 128, 254, 255 };

bool WriteImage(const std::string& fileName, bool local, const char* nameField)
{
  vtksys::ofstream header(fileName.c_str(), std::ios::binary);
  header << "ObjectType = Image\n"
         << "NDims = 3\n"
         << "BinaryData = True\n"
         << "BinaryDataByteOrderMSB = False\n"
         << nameField << "DimSize = 2 2 2\n"
         << "ElementType = MET_UCHAR\n"
         << "ElementDataFile = " << (local ? "LOCAL" : "TestMetaImageReaderName.raw") << '\n';
  if (local)
  {
    header.write(reinterpret_cast<const char*>(Pixels), sizeof(Pixels));
  }
  header.close();
  return !header.fail();
}

bool CheckImage(vtkMetaImageReader* reader, const char* expectedName)
{
  vtkImageData* image = reader->GetOutput();
  vtkDataArray* scalars = image->GetPointData()->GetScalars();
  if (!scalars || !scalars->GetName() || std::strcmp(scalars->GetName(), expectedName) != 0)
  {
    std::cerr << reader->GetFileName() << ": expected scalar name " << expectedName << ", got "
              << (scalars && scalars->GetName() ? scalars->GetName() : "(null)") << '\n';
    return false;
  }

  const int* dimensions = image->GetDimensions();
  if (dimensions[0] != 2 || dimensions[1] != 2 || dimensions[2] != 2 ||
    scalars->GetDataType() != VTK_UNSIGNED_CHAR || scalars->GetNumberOfComponents() != 1 ||
    scalars->GetNumberOfTuples() != 8)
  {
    std::cerr << reader->GetFileName() << ": unexpected image dimensions or scalar layout\n";
    return false;
  }
  for (vtkIdType i = 0; i < 8; ++i)
  {
    if (scalars->GetComponent(i, 0) != Pixels[i])
    {
      std::cerr << reader->GetFileName() << ": unexpected value at voxel " << i << '\n';
      return false;
    }
  }
  return true;
}
}

int TestMetaImageReaderName(int argc, char* argv[])
{
  char* temp =
    vtkTestUtilities::GetArgOrEnvOrDefault("-T", argc, argv, "VTK_TEMP_DIR", "Testing/Temporary");
  const std::string tempDir = temp;
  delete[] temp;
  if (!vtksys::SystemTools::MakeDirectory(tempDir))
  {
    std::cerr << "Cannot create temporary directory " << tempDir << '\n';
    return EXIT_FAILURE;
  }

  const std::string rawName = tempDir + "/TestMetaImageReaderName.raw";
  vtksys::ofstream raw(rawName.c_str(), std::ios::binary);
  raw.write(reinterpret_cast<const char*>(Pixels), sizeof(Pixels));
  raw.close();
  if (raw.fail())
  {
    std::cerr << "Cannot write " << rawName << '\n';
    vtksys::SystemTools::RemoveFile(rawName);
    return EXIT_FAILURE;
  }

  const struct
  {
    const char* Field;
    const char* Expected;
  } cases[] = {
    { "Name = First volume\n", "First volume" },
    { "", "MetaImage" },
    { "Name = Second volume\n", "Second volume" },
    { "Name =\n", "MetaImage" },
    { "Name = Third volume\n", "Third volume" },
    { "Name = \t  \n", "MetaImage" },
  };

  // Reuse one reader to catch stale names when switching files.
  vtkNew<vtkMetaImageReader> reader;
  bool success = true;
  for (bool local : { true, false })
  {
    int index = 0;
    for (const auto& test : cases)
    {
      const std::string fileName =
        tempDir + "/TestMetaImageReaderName" + vtk::to_string(index++) + (local ? ".mha" : ".mhd");
      if (!WriteImage(fileName, local, test.Field))
      {
        std::cerr << "Cannot write " << fileName << '\n';
        success = false;
      }
      else
      {
        reader->SetFileName(fileName.c_str());
        reader->UpdateInformation();
        reader->Update();
        success = CheckImage(reader, test.Expected) && success;

        // Check both a cached update and a forced reread.
        reader->Update();
        success = CheckImage(reader, test.Expected) && success;
        reader->Modified();
        reader->Update();
        success = CheckImage(reader, test.Expected) && success;
      }
      vtksys::SystemTools::RemoveFile(fileName);
    }
  }
  vtksys::SystemTools::RemoveFile(rawName);
  return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
