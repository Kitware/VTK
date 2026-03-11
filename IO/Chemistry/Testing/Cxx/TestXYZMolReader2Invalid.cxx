// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// Regression test for paraview/paraview#19558: vtkXYZMolReader2 used to crash
// on malformed XYZ files because RequestInformation indexed an empty TimeSteps
// vector. Verify that CanReadFile() rejects such files and that reading one
// fails gracefully instead of crashing.

#include "vtkFileResourceStream.h"
#include "vtkNew.h"
#include "vtkObject.h"
#include "vtkTestUtilities.h"
#include "vtkTesting.h"
#include "vtkXYZMolReader2.h"

#include <iostream>
#include <string>
#include <vector>

#include <vtksys/FStream.hxx>
#include <vtksys/SystemTools.hxx>

namespace
{
bool WriteFile(const std::string& path, const std::string& contents)
{
  vtksys::SystemTools::MakeDirectory(vtksys::SystemTools::GetFilenamePath(path));
  vtksys::ofstream out(path.c_str(), std::ios::binary);
  if (!out.is_open())
  {
    std::cerr << "Could not open '" << path << "' for writing.\n";
    return false;
  }
  out << contents;
  out.close();
  return true;
}

// Run the file through both CanReadFile() overloads and require them to agree.
// Returns the (shared) verdict; on disagreement returns false and flags status.
bool CanReadBothWays(const std::string& path, int& status)
{
  const bool byName = vtkXYZMolReader2::CanReadFile(path.c_str());

  vtkNew<vtkFileResourceStream> stream;
  bool byStream = false;
  if (stream->Open(path.c_str()))
  {
    byStream = vtkXYZMolReader2::CanReadFile(stream);
  }

  if (byName != byStream)
  {
    std::cerr << "CanReadFile(const char*) and CanReadFile(vtkResourceStream*) disagree for '"
              << path << "': " << byName << " vs " << byStream << ".\n";
    status = EXIT_FAILURE;
  }
  return byName;
}
}

int TestXYZMolReader2Invalid(int argc, char* argv[])
{
  vtkNew<vtkTesting> testing;
  testing->AddArguments(argc, argv);
  const char* tempDir = testing->GetTempDirectory();
  if (!tempDir)
  {
    std::cerr << "Could not determine temp directory.\n";
    return EXIT_FAILURE;
  }
  const std::string dir(tempDir);

  int status = EXIT_SUCCESS;

  // Track everything we write so the temp directory is left clean.
  std::vector<std::string> writtenFiles;

  // CanReadFile() must accept a well-formed XYZ file...
  const std::string validName = dir + "/TestXYZMolReader2_valid.xyz";
  if (!::WriteFile(validName, "2\nwater\nO 0 0 0\nH 1 0 0\n"))
  {
    return EXIT_FAILURE;
  }
  writtenFiles.emplace_back(validName);
  if (!::CanReadBothWays(validName, status))
  {
    std::cerr << "CanReadFile() rejected a valid XYZ file.\n";
    status = EXIT_FAILURE;
  }

  // ...and reject files that do not begin with a positive atom count.
  // For files that yield no timesteps at all (garbage/empty), RequestInformation
  // must additionally fail rather than index an empty TimeSteps vector -- this is
  // the crash being regression-tested. A zero/negative count still parses into a
  // (degenerate) timestep, so there the contract is only "CanReadFile rejects it
  // and reading does not crash".
  struct InvalidCase
  {
    const char* name;
    const char* contents;
    bool expectInfoFailure;
  };
  const InvalidCase invalidFiles[] = {
    { "TestXYZMolReader2_garbage.xyz", "this is not an xyz file\n", true },
    { "TestXYZMolReader2_empty.xyz", "", true },
    { "TestXYZMolReader2_zero.xyz", "0\ntitle\n", false },
    { "TestXYZMolReader2_negative.xyz", "-3\ntitle\n", false },
  };

  for (const auto& entry : invalidFiles)
  {
    const std::string name = dir + "/" + entry.name;
    if (!::WriteFile(name, entry.contents))
    {
      return EXIT_FAILURE;
    }
    writtenFiles.emplace_back(name);

    if (::CanReadBothWays(name, status))
    {
      std::cerr << "CanReadFile() accepted invalid file '" << entry.name << "'.\n";
      status = EXIT_FAILURE;
    }

    // The crux of the regression: reading a malformed file must not crash. The
    // reader is expected to emit an error for the empty-timesteps cases, so the
    // error output below is benign.
    vtkNew<vtkXYZMolReader2> reader;
    reader->SetFileName(name.c_str());
    // The empty-timesteps cases intentionally emit a vtkErrorMacro; silence it
    // so the test harness does not flag the expected error output as a failure.
    vtkObject::GlobalWarningDisplayOff();
    const int infoOk = reader->UpdateInformation();
    vtkObject::GlobalWarningDisplayOn();
    if (entry.expectInfoFailure && infoOk != 0)
    {
      std::cerr << "UpdateInformation() unexpectedly succeeded for invalid file '" << entry.name
                << "'.\n";
      status = EXIT_FAILURE;
    }
  }

  // A non-existent file must also be rejected rather than crash.
  if (vtkXYZMolReader2::CanReadFile((dir + "/does_not_exist.xyz").c_str()))
  {
    std::cerr << "CanReadFile() accepted a non-existent file.\n";
    status = EXIT_FAILURE;
  }
  if (vtkXYZMolReader2::CanReadFile(static_cast<const char*>(nullptr)))
  {
    std::cerr << "CanReadFile() accepted a null file name.\n";
    status = EXIT_FAILURE;
  }
  if (vtkXYZMolReader2::CanReadFile(static_cast<vtkResourceStream*>(nullptr)))
  {
    std::cerr << "CanReadFile() accepted a null stream.\n";
    status = EXIT_FAILURE;
  }

  // Clean up the fixtures we wrote.
  for (const std::string& file : writtenFiles)
  {
    vtksys::SystemTools::RemoveFile(file);
  }

  return status;
}
