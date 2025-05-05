// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include <vtkDataArraySelection.h>
#include <vtkOpenFOAMReader.h>
#include <vtkSmartPointer.h>
#include <vtkSystemIncludes.h>
#include <vtkTestUtilities.h>

// Simple RAII wrapper to ensure deletion of the filename and optionnaly the file itself at the end
// of the scope.
class RAIIFilename
{
public:
  enum DeletionStatus : std::uint8_t
  {
    DELETE,
    DO_NOT_DELETE
  };
  RAIIFilename(const char* filename, DeletionStatus deletionStatus)
    : Filename(filename)
    , Status(deletionStatus)
  {
  }

  ~RAIIFilename()
  {
    if (Status == DeletionStatus::DELETE)
    {
      vtksys::SystemTools::RemoveFile(this->Filename);
    }
    delete[] this->Filename;
  }

  const char* data() const { return this->Filename; }

private:
  const char* Filename;
  DeletionStatus Status;
};

int TestOpenFOAMReaderRestartFiles(int argc, char* argv[])
{
  // Read file name.
  RAIIFilename filename(
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/OpenFOAM/cavity/cavity.foam"),
    RAIIFilename::DO_NOT_DELETE);

  // Copy an existing result file to "test_0" file, which looks like a restart file.
  RAIIFilename initialFile(
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/OpenFOAM/cavity/0/U"),
    RAIIFilename::DO_NOT_DELETE);
  RAIIFilename testFile(
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/OpenFOAM/cavity/0/test_0"),
    RAIIFilename::DELETE);
  vtksys::SystemTools::CopyFileAlways(initialFile.data(), testFile.data());

  // Read the OpenFOAM data.
  vtkSmartPointer<vtkOpenFOAMReader> reader = vtkSmartPointer<vtkOpenFOAMReader>::New();
  reader->SetFileName(filename.data());

  // Check that by default the restart file is ignored.
  reader->UpdateInformation();
  if (reader->GetCellDataArraySelection()->ArrayExists("test_0") != 0)
  {
    std::cerr << "Error: SetIgnoreRestartFiles should be true by default." << std::endl;
    return EXIT_FAILURE;
  }

  // Check the IgnoreRestartFiles option.
  reader->SetIgnoreRestartFiles(false);
  reader->SetRefresh();
  reader->UpdateInformation();
  if (reader->GetCellDataArraySelection()->ArrayExists("test_0") != 1)
  {
    std::cerr << "Error: can't find the test_0 cell data." << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
