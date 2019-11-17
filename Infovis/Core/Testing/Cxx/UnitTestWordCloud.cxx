/*=========================================================================

  Program:   Visualization Toolkit
  Module:    UnitTestWordCloud.cxx

-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSmartPointer.h"
#include "vtkWordCloud.h"

#include "vtkExecutive.h"
#include "vtkTestErrorObserver.h"

#include <algorithm>
#include <iostream>
#include <random>
#include <sstream>

namespace
{
int TestOneByOne(vtkSmartPointer<vtkWordCloud>& wc, std::string name, size_t keptExpected,
  size_t skippedExpected, size_t stoppedExpected, int alternateOffset = 0)
{
  int status = 0;
  wc->Update();
  if (keptExpected != wc->GetKeptWords().size() &&
    keptExpected + alternateOffset != wc->GetKeptWords().size())
  {
    std::cout << "\n  Regression failed for " << name << ". Expected # of kept words "
              << keptExpected << " but got " << wc->GetKeptWords().size();
    status++;
  }
  if (skippedExpected != wc->GetSkippedWords().size() &&
    skippedExpected - alternateOffset != wc->GetSkippedWords().size())
  {
    std::cout << "\n  Regression failed for " << name << ". Expected # of skipped words "
              << skippedExpected << " but got " << wc->GetSkippedWords().size();
    status++;
  }
  if (stoppedExpected != wc->GetStoppedWords().size())
  {
    std::cout << "\n  Regression failed for " << name << ". Expected # of stopped words "
              << stoppedExpected << " but got " << wc->GetStoppedWords().size() << std::endl;
    status++;
  }
  return status;
}
}

int UnitTestWordCloud(int argc, char* argv[])
{
  // This test uses random variables, so differences may exist from
  // compiler to compiler
#if defined(__GNUC__)
  if (std::system("/usr/bin/gcc --version >gccversion.txt") == 0)
  {
    std::cout << std::ifstream("gccversion.txt").rdbuf();
  }
#endif

  if (argc < 2)
  {
    std::cout << "Usage: " << argv[0] << "filename" << std::endl;
    return EXIT_FAILURE;
  }

  auto status = 0;

  // Create a word cloud source
  auto wordCloud = vtkSmartPointer<vtkWordCloud>::New();

  // Test empty print
  std::cout << "Testing empty Print...";
  std::ostringstream emptyPrint;
  wordCloud->Print(emptyPrint);
  std::cout << "Passed" << std::endl;

  // Test defaults
  wordCloud->SetFileName(argv[1]);
  wordCloud->Update();

  // Test Regressions for default settings
  // There are some numerical issues with some compilers that can
  // cause alternate results to be produced
  auto status1 = 0;
  std::cout << "Testing regressions of default word cloud...";
  size_t keptExpected = 31;
  size_t alternateKeptExpected = 23;
  if (keptExpected != wordCloud->GetKeptWords().size() &&
    alternateKeptExpected != wordCloud->GetKeptWords().size())
  {
    std::cout << "\n  Default regression failed. Received unexpected # of kept words "
              << wordCloud->GetKeptWords().size();
    status1++;
  }

  size_t skippedExpected = 42;
  size_t alternateSkippedExpected = 50;
  if (skippedExpected != wordCloud->GetSkippedWords().size() &&
    alternateSkippedExpected != wordCloud->GetSkippedWords().size())
  {
    std::cout << "\n  Default regression failed. Expected # of skipped words " << skippedExpected
              << " but got " << wordCloud->GetSkippedWords().size();
    status1++;
  }
  size_t stoppedExpected = 65;
  if (stoppedExpected != wordCloud->GetStoppedWords().size())
  {
    std::cout << "\n  Default regression failed. Expected # of stopped words " << stoppedExpected
              << " but got " << wordCloud->GetStoppedWords().size() << std::endl;
    status1++;
  }
  if (status1)
  {
    std::cout << "\n..Failed" << std::endl;
    ++status;
  }
  else
  {
    std::cout << "..Passed" << std::endl;
  }

  // Check modified times for containers
#define CHECK_CONTAINER_MTIMES(name)                                                               \
  {                                                                                                \
    auto name = wordCloud->Get##name();                                                            \
    auto mtime = wordCloud->GetMTime();                                                            \
    wordCloud->Set##name(name);                                                                    \
    auto mtimeModified = mtime;                                                                    \
    if (mtime != mtimeModified)                                                                    \
    {                                                                                              \
      std::cout << "\n  Modify time is bad for " #name;                                            \
      status2++;                                                                                   \
    }                                                                                              \
    name[0] = name[name.size() - 1];                                                               \
    wordCloud->Set##name(name);                                                                    \
    mtimeModified = wordCloud->GetMTime();                                                         \
    if (mtime == mtimeModified)                                                                    \
    {                                                                                              \
      std::cout << "\n Modify time is bad for " #name;                                             \
      status2++;                                                                                   \
    }                                                                                              \
  }

  std::cout << "Testing Container MTimes...";
  auto status2 = 0;

  vtkWordCloud::ColorDistributionContainer colorDistribution = { { .6, 1.0 } };
  wordCloud->SetColorDistribution(colorDistribution);

  vtkWordCloud::OffsetDistributionContainer offsetDistribution = { { -10, 20 } };
  wordCloud->SetOffsetDistribution(offsetDistribution);

  vtkWordCloud::OrientationsContainer orientations = { -90 };
  wordCloud->SetOrientations(orientations);
  wordCloud->AddOrientation(90.0);

  wordCloud->SetOrientations(orientations);
  wordCloud->AddOrientation(0.0);

  vtkWordCloud::ReplacementPairsContainer replacementPairs;
  vtkWordCloud::PairType pt1("old", "new");
  vtkWordCloud::PairType pt2("bill", "will");
  replacementPairs.push_back(pt1);
  wordCloud->SetReplacementPairs(replacementPairs);
  wordCloud->AddReplacementPair(pt2);

  vtkWordCloud::SizesContainer sizes;
  sizes[0] = 100;
  sizes[1] = 10;
  wordCloud->SetSizes(sizes);

  vtkWordCloud::StopWordsContainer words;
  words.insert("albany");
  wordCloud->SetStopWords(words);
  wordCloud->AddStopWord("troy");
  wordCloud->AddStopWord("clifton");

  CHECK_CONTAINER_MTIMES(ColorDistribution);
  CHECK_CONTAINER_MTIMES(OffsetDistribution);
  CHECK_CONTAINER_MTIMES(OrientationDistribution);
  CHECK_CONTAINER_MTIMES(Orientations);
  CHECK_CONTAINER_MTIMES(ReplacementPairs);
  CHECK_CONTAINER_MTIMES(Sizes);

  if (status2)
  {
    std::cout << " ...Failed" << std::endl;
    ++status;
  }
  else
  {
    std::cout << " ...Passed" << std::endl;
  }
  std::cout << "Testing Set..";
  wordCloud->SetBackgroundColorName("banana");
  wordCloud->SetBWMask(1);
  wordCloud->SetColorSchemeName("foo");
  wordCloud->SetDPI(100);
  wordCloud->SetFontMultiplier(3);
  wordCloud->SetGap(5);
  wordCloud->SetFontFileName(argv[2]);
  wordCloud->SetMaskColorName("white");
  wordCloud->SetMaskFileName("maskfile");
  wordCloud->SetMaxFontSize(100);
  wordCloud->SetMinFontSize(100);
  wordCloud->SetMinFrequency(3);
  wordCloud->SetWordColorName("Brown");
  wordCloud->SetTitle("Unit Test");
  auto wordCloudNew = wordCloud->NewInstance();
  std::cout << "..Passed" << std::endl;

  std::cout << "Testing Set one-by-one..";
  int status4 = 0;
  {
    auto wc = vtkSmartPointer<vtkWordCloud>::New();

    wc->SetFileName(argv[1]);
    status4 += TestOneByOne(wc, "Defaults", 31, 42, 65, -8);

    wc->SetFontFileName(argv[2]);
    status4 += TestOneByOne(wc, "FontFileName", 40, 33, 65);

    wc->SetGap(4);
    status4 += TestOneByOne(wc, "Gap", 28, 45, 65);

    wc->SetFontMultiplier(8);
    status4 += TestOneByOne(wc, "FontMultiplier", 20, 53, 65);

    wc->SetMinFrequency(2);
    status4 += TestOneByOne(wc, "MinFrequency", 10, 63, 65);

    wc->SetMaxFontSize(100);
    status4 += TestOneByOne(wc, "MaxFontSize", 10, 63, 65);

    wc->AddStopWord("nation");
    wc->AddStopWord("dedicated");
    status4 += TestOneByOne(wc, "StopWords", 11, 60, 67);

    vtkWordCloud::OrientationDistributionContainer oDist;
    oDist[0] = -90.0;
    oDist[1] = 90.0;
    status4 += TestOneByOne(wc, "OrientationDistribution", 11, 60, 67);

    wc->AddOrientation(90.0);
    wc->AddOrientation(0.0);
    status4 += TestOneByOne(wc, "Orientations", 11, 60, 67);

    wc->SetTitle("Gettysburg");
    status4 += TestOneByOne(wc, "Title", 11, 61, 67);

    wc->SetDPI(100);
    status4 += TestOneByOne(wc, "DPI", 11, 61, 67);

    wc->SetMaskColorName("white");
    wc->SetFontMultiplier(2);
    wc->SetMaxFontSize(10);
    wc->SetMaskFileName(argv[3]);
    status4 += TestOneByOne(wc, "MaskFileName", 12, 60, 67);

    wc->SetMaskFileName(argv[4]);
    wc->SetBWMask(1);
    status4 += TestOneByOne(wc, "MaskFileName(8bit)", 12, 60, 67);

    wc->SetColorSchemeName("Brewer Qualitative Pastel2");
    status4 += TestOneByOne(wc, "ColorSchemeName", 12, 60, 67);

    auto repPair = std::make_tuple("consecrate", "consecrated");
    wc->AddReplacementPair(repPair);
    status4 += TestOneByOne(wc, " ReplacementPairs", 12, 58, 68);

    wc->SetWordColorName("Peacock");
    status4 += TestOneByOne(wc, "WordColorName", 12, 58, 68);

    vtkWordCloud::ColorDistributionContainer colorDist = { { 0.0, 1.0 } };
    wc->SetColorDistribution(colorDist);
    status4 += TestOneByOne(wc, "ColorDistribution", 12, 58, 68);

    wc->SetStopListFileName(argv[5]);
    status4 += TestOneByOne(wc, "StopListFileName", 18, 73, 47);
  }
  if (status4)
  {
    std::cout << "\n..Failed" << std::endl;
  }
  else
  {
    std::cout << "..Passed" << std::endl;
  }
  status += status4;

  // Test Errors
  std::cout << "Testing Errors..";
  auto errorObserver = vtkSmartPointer<vtkTest::ErrorObserver>::New();
  auto errorObserver1 = vtkSmartPointer<vtkTest::ErrorObserver>::New();
  int status5 = 0;
  {
    auto wc = vtkSmartPointer<vtkWordCloud>::New();
    wc->AddObserver(vtkCommand::ErrorEvent, errorObserver);
    wc->GetExecutive()->AddObserver(vtkCommand::ErrorEvent, errorObserver1);
    wc->SetFileName(argv[1]);
    wc->SetWordColorName("");
    wc->SetColorSchemeName("foo");
    wc->Update();
    status5 += errorObserver->CheckErrorMessage("The color scheme foo does not exist");
    errorObserver->Clear();
  }
  {
    auto wc = vtkSmartPointer<vtkWordCloud>::New();
    wc->AddObserver(vtkCommand::ErrorEvent, errorObserver);
    wc->GetExecutive()->AddObserver(vtkCommand::ErrorEvent, errorObserver1);
    wc->SetFileName("Foo.txt");
    wc->Update();
    status5 += errorObserver->CheckErrorMessage("FileName Foo.txt does not exist");
    errorObserver->Clear();
  }
  {
    auto wc = vtkSmartPointer<vtkWordCloud>::New();
    wc->AddObserver(vtkCommand::ErrorEvent, errorObserver);
    wc->GetExecutive()->AddObserver(vtkCommand::ErrorEvent, errorObserver1);
    wc->SetFileName(argv[1]);
    wc->SetFontFileName("BadFontFile.txt");
    wc->Update();
    status5 += errorObserver->CheckErrorMessage("FontFileName BadFontFile.txt does not exist");
    errorObserver->Clear();
  }
  {
    auto wc = vtkSmartPointer<vtkWordCloud>::New();
    wc->AddObserver(vtkCommand::ErrorEvent, errorObserver);
    wc->GetExecutive()->AddObserver(vtkCommand::ErrorEvent, errorObserver1);
    wc->SetFileName(argv[1]);
    wc->SetMaskFileName("BadMaskFile.txt");
    wc->Update();
    status5 += errorObserver->CheckErrorMessage("MaskFileName BadMaskFile.txt does not exist");
    errorObserver->Clear();
  }
  {
    auto wc = vtkSmartPointer<vtkWordCloud>::New();
    wc->AddObserver(vtkCommand::ErrorEvent, errorObserver);
    wc->GetExecutive()->AddObserver(vtkCommand::ErrorEvent, errorObserver1);
    wc->SetFileName(argv[1]);
    wc->SetMaskFileName("BadStopListFile.txt");
    wc->Update();
    status5 += errorObserver->CheckErrorMessage("BadStopListFile.txt does not exist");
    errorObserver->Clear();
  }
  {
    auto wc = vtkSmartPointer<vtkWordCloud>::New();
    wc->AddObserver(vtkCommand::ErrorEvent, errorObserver);
    wc->GetExecutive()->AddObserver(vtkCommand::ErrorEvent, errorObserver1);
    wc->Update();
    status5 +=
      errorObserver->CheckErrorMessage("No FileName is set. Use SetFileName to set a file");
    errorObserver->Clear();
  }
  status += status5;
  if (status5)
  {
    std::cout << "..Failed" << std::endl;
  }
  else
  {
    std::cout << "..Passed" << std::endl;
  }

  std::cout << "Testing populated Print...";
  std::ostringstream populatedPrint;
  wordCloud->Print(populatedPrint);
  std::cout << "..Passed" << std::endl;

  auto className = wordCloud->GetClassName();
  std::cout << "className: " << className << std::endl;

  wordCloudNew->Delete();

  if (status)
  {
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
