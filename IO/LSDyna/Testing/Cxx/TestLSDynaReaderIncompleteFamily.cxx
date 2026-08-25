// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// Tests that vtkLSDynaReader survives incomplete d3plot families and
// databases with multi-solver extra data. Synthetic minimal databases
// are generated on the fly, no input data is required.

#include "vtkLSDynaReader.h"

#include "vtkCommand.h"
#include "vtkExecutive.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkPointSet.h"
#include "vtkSmartPointer.h"
#include "vtkTestErrorObserver.h"
#include "vtkTestUtilities.h"

#include "vtksys/SystemTools.hxx"

#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace
{

void AppendWord(std::vector<char>& words, std::int32_t value)
{
  const char* raw = reinterpret_cast<const char*>(&value);
  words.insert(words.end(), raw, raw + sizeof(value));
}

void AppendWord(std::vector<char>& words, float value)
{
  const char* raw = reinterpret_cast<const char*>(&value);
  words.insert(words.end(), raw, raw + sizeof(value));
}

void PadWithZeroWords(std::vector<char>& words, std::size_t totalWords)
{
  while (words.size() < totalWords * sizeof(std::int32_t))
  {
    AppendWord(words, std::int32_t(0));
  }
}

// Control section (64 words) and geometry (33 words) of the smallest
// database the reader accepts, using 4 byte words: one hexahedron with
// unpacked (NDIM = 4) connectivity, NGLBV = 6 global values and only
// the deflection (IU) point state, so one state is
// 1 + 6 + 8 * 3 + 7 = 38 words.
std::vector<char> MakeControlAndGeometry(std::int32_t ncfdv1, std::int32_t ncfdv2)
{
  std::vector<char> words;

  const char title[41] = "vtkLSDynaReader incomplete family test  ";
  words.insert(words.end(), title, title + 40); // words 0-9
  AppendWord(words, std::int32_t(0));           // word 10: runtime
  AppendWord(words, std::int32_t(1));           // word 11: file type
  AppendWord(words, std::int32_t(0));           // word 12: source version
  const char release[5] = " R11";
  words.insert(words.end(), release, release + 4); // word 13: release
  AppendWord(words, 960.0f);                       // word 14: version

  const std::int32_t controlWords[] = {
    4,          // NDIM: 3D with unpacked connectivity
    8,          // NUMNP
    6,          // ICODE
    6,          // NGLBV
    0,          // IT
    1,          // IU
    0,          // IV
    0,          // IA
    1,          // NEL8
    1,          // NUMMAT8
    0, 0,       // BLANK
    7,          // NV3D
    0,          // NEL2
    0,          // NUMMAT2
    0,          // NV1D
    0,          // NEL4
    0,          // NUMMAT4
    0,          // NV2D
    0,          // NEIPH
    0,          // NEIPS
    0,          // MAXINT
    0,          // NMSPH
    0,          // NGPSPH
    0,          // NARBS
    0,          // NELT
    0,          // NUMMATT
    0,          // NV3DT
    0, 0, 0, 0, // IOSHL(1-4)
    0,          // IALEMAT
    ncfdv1,     // NCFDV1
    ncfdv2,     // NCFDV2
    0,          // NADAPT
    0,          // NMMAT
    0,          // NUMFLUID
  };
  for (std::int32_t word : controlWords)
  {
    AppendWord(words, word);
  }
  PadWithZeroWords(words, 64);

  // geometry: unit cube corner coordinates followed by one hexahedron
  // with 8 node ids and a material id
  const float coords[8][3] = { { 0.f, 0.f, 0.f }, { 1.f, 0.f, 0.f }, { 1.f, 1.f, 0.f },
    { 0.f, 1.f, 0.f }, { 0.f, 0.f, 1.f }, { 1.f, 0.f, 1.f }, { 1.f, 1.f, 1.f }, { 0.f, 1.f, 1.f } };
  for (const auto& coord : coords)
  {
    AppendWord(words, coord[0]);
    AppendWord(words, coord[1]);
    AppendWord(words, coord[2]);
  }
  for (std::int32_t node = 1; node <= 8; ++node)
  {
    AppendWord(words, node);
  }
  AppendWord(words, std::int32_t(1)); // material id

  return words;
}

// One complete state: time word, global values, nodal deflection and
// the solid element variables.
void AppendState(std::vector<char>& words, float time)
{
  AppendWord(words, time);
  for (int i = 0; i < 6; ++i)
  {
    AppendWord(words, 0.5f); // global values
  }
  for (int i = 0; i < 8 * 3; ++i)
  {
    AppendWord(words, 1.5f); // deflected coordinates
  }
  for (int i = 0; i < 7; ++i)
  {
    AppendWord(words, 2.5f); // solid element variables
  }
}

bool WriteFile(const std::string& path, const std::vector<char>& words)
{
  std::ofstream file(path.c_str(), std::ios::binary);
  file.write(words.data(), static_cast<std::streamsize>(words.size()));
  return file.good();
}

vtkSmartPointer<vtkLSDynaReader> MakeObservedReader(
  const std::string& rootFile, vtkTest::ErrorObserver* observer)
{
  auto reader = vtkSmartPointer<vtkLSDynaReader>::New();
  reader->SetFileName(rootFile.c_str());
  reader->AddObserver(vtkCommand::ErrorEvent, observer);
  reader->AddObserver(vtkCommand::WarningEvent, observer);
  reader->GetExecutive()->AddObserver(vtkCommand::ErrorEvent, observer);
  return reader;
}

// An incomplete database must yield zero time steps and a readable
// error, never a crash.
bool TestIncompleteDatabase(
  const std::string& dir, const std::vector<char>& rootTrailer, const std::vector<char>* stateFile)
{
  vtksys::SystemTools::MakeDirectory(dir);

  std::vector<char> root = MakeControlAndGeometry(0, 0);
  root.insert(root.end(), rootTrailer.begin(), rootTrailer.end());
  PadWithZeroWords(root, 128); // storage detection reads 128 words
  if (!WriteFile(dir + "/d3plot", root))
  {
    std::cerr << "could not write " << dir << "/d3plot" << std::endl;
    return false;
  }
  if (stateFile && !WriteFile(dir + "/d3plot01", *stateFile))
  {
    std::cerr << "could not write " << dir << "/d3plot01" << std::endl;
    return false;
  }

  vtkNew<vtkTest::ErrorObserver> observer;
  auto reader = MakeObservedReader(dir + "/d3plot", observer);
  reader->UpdateInformation();
  if (reader->GetExecutive()->Update())
  {
    std::cerr << dir << ": expected the update of an incomplete database to fail" << std::endl;
    return false;
  }

  if (reader->GetNumberOfTimeSteps() != 0)
  {
    std::cerr << dir << ": expected 0 time steps, got " << reader->GetNumberOfTimeSteps()
              << std::endl;
    return false;
  }
  if (!observer->HasErrorMessage("No valid time steps"))
  {
    std::cerr << dir << ": expected the 'No valid time steps' error" << std::endl;
    return false;
  }
  return true;
}

}

int TestLSDynaReaderIncompleteFamily(int argc, char* argv[])
{
  char* rawTempDir =
    vtkTestUtilities::GetArgOrEnvOrDefault("-T", argc, argv, "VTK_TEMP_DIR", "Testing/Temporary");
  const std::string tempDir = std::string(rawTempDir) + "/TestLSDynaReaderIncompleteFamily";
  delete[] rawTempDir;

  // A root file whose trailing words are neither a complete state nor
  // an end-of-file marker: a truncated or unparsed section.
  std::vector<char> truncatedTrailer;
  ::AppendWord(truncatedTrailer, std::int32_t(6));
  if (!::TestIncompleteDatabase(tempDir + "/truncatedRoot", truncatedTrailer, nullptr))
  {
    return EXIT_FAILURE;
  }

  // A family whose state file holds only a fraction of one state.
  std::vector<char> eofTrailer;
  ::AppendWord(eofTrailer, -999999.0f);
  std::vector<char> halfState;
  ::AppendState(halfState, 0.001f);
  halfState.resize(19 * sizeof(std::int32_t));
  if (!::TestIncompleteDatabase(tempDir + "/truncatedState", eofTrailer, &halfState))
  {
    return EXIT_FAILURE;
  }

  // A complete database with multi-solver extra data: the root file
  // ends with unparsed extra data and the state lives in the next
  // family file. The reader must skip the extra data, find the state
  // and warn that the multi-solver datasets themselves are not read.
  {
    const std::string dir = tempDir + "/multiSolver";
    vtksys::SystemTools::MakeDirectory(dir);

    // NCFDV1 = 67108864 is the multi-solver sentinel, NCFDV2 = 3 datasets
    std::vector<char> root = ::MakeControlAndGeometry(67108864, 3);
    for (std::int32_t junk = 0; junk < 4; ++junk)
    {
      ::AppendWord(root, junk); // stand-in for the multi-solver control data
    }
    ::AppendWord(root, -999999.0f);
    ::PadWithZeroWords(root, 128);

    std::vector<char> state;
    ::AppendState(state, 0.001f);
    ::AppendWord(state, -999999.0f);

    if (!::WriteFile(dir + "/d3plot", root) || !::WriteFile(dir + "/d3plot01", state))
    {
      std::cerr << "could not write the multi-solver database" << std::endl;
      return EXIT_FAILURE;
    }

    vtkNew<vtkTest::ErrorObserver> observer;
    auto reader = ::MakeObservedReader(dir + "/d3plot", observer);
    reader->UpdateInformation();

    if (!observer->HasWarningMessage("multi-solver extra data"))
    {
      std::cerr << "expected the multi-solver warning" << std::endl;
      return EXIT_FAILURE;
    }
    if (reader->GetNumberOfTimeSteps() != 1 || std::abs(reader->GetTimeValue(0) - 0.001) > 1e-6)
    {
      std::cerr << "expected one time step at t=0.001, got " << reader->GetNumberOfTimeSteps()
                << " steps" << std::endl;
      return EXIT_FAILURE;
    }

    if (!reader->GetExecutive()->Update())
    {
      std::cerr << "the multi-solver database failed to update" << std::endl;
      return EXIT_FAILURE;
    }
    auto solids = vtkPointSet::SafeDownCast(reader->GetOutput()->GetBlock(0));
    if (!solids || solids->GetNumberOfCells() != 1 || solids->GetNumberOfPoints() != 8)
    {
      std::cerr << "expected one hexahedron with 8 points in the first block" << std::endl;
      return EXIT_FAILURE;
    }
    if (solids->GetPointData()->HasArray("Species01"))
    {
      std::cerr << "NCFDV2 must not be interpreted as species bit flags" << std::endl;
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}
