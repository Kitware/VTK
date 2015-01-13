/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRenderTimings.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkRenderTimings.h"

void vtkRTTestSequence::GetSequenceNumbers(int &xdim)
{
  static int linearSequence[] = {1,2,3,5};

  xdim = 1;
  int sc = this->SequenceCount;
  while (sc >= 4)
    {
    xdim *= 10;
    sc -= 4;
    }
  xdim *= linearSequence[sc];
}

void vtkRTTestSequence::GetSequenceNumbers(int &xdim, int &ydim)
{
  static int squareSequenceX[] = {1, 2, 3, 5, 5, 5, 6, 10};
  static int squareSequenceY[] = {1, 1, 1, 1, 2, 4, 5,  5};

  xdim = 1;
  ydim = 1;
  int sc = this->SequenceCount;
  while (sc >= 8)
    {
    xdim *= 10;
    ydim *= 10;
    sc -= 8;
    }
  xdim *= squareSequenceX[sc];
  ydim *= squareSequenceY[sc];
}

void vtkRTTestSequence::GetSequenceNumbers(int &xdim, int &ydim, int &zdim)
{
  static int cubeSequenceX[] = {1, 2, 3, 5,  5, 5, 5, 5,  5, 8, 10, 10};
  static int cubeSequenceY[] = {1, 1, 1, 1,  2, 2, 3, 5,  5, 5,  6, 10};
  static int cubeSequenceZ[] = {1, 1, 1, 1,  1, 2, 2, 2,  4, 5,  5,  5};

  xdim = 1;
  ydim = 1;
  zdim = 1;
  int sc = this->SequenceCount;
  while (sc >= 12)
    {
    xdim *= 10;
    ydim *= 10;
    zdim *= 10;
    sc -= 12;
    }
  xdim *= cubeSequenceX[sc];
  ydim *= cubeSequenceY[sc];
  zdim *= cubeSequenceZ[sc];
}

void vtkRTTestSequence::GetSequenceNumbers(int &xdim, int &ydim, int &zdim, int &wdim)
{
  static int sequenceX[] = {1, 2, 3, 5,  5, 5, 5, 5,  5, 5, 5, 5,  8, 10, 10, 10};
  static int sequenceY[] = {1, 1, 1, 1,  2, 2, 3, 5,  5, 5, 5, 5,  5,  8, 10, 10};
  static int sequenceZ[] = {1, 1, 1, 1,  1, 2, 2, 2,  2, 4, 4, 5,  5,  5,  6, 10};
  static int sequenceW[] = {1, 1, 1, 1,  1, 1, 1, 1,  2, 2, 3, 4,  5,  5,  5,  5};

  xdim = 1;
  ydim = 1;
  zdim = 1;
  wdim = 1;
  int sc = this->SequenceCount;
  while (sc >= 16)
    {
    xdim *= 10;
    ydim *= 10;
    zdim *= 10;
    wdim *= 10;
    sc -= 16;
    }
  xdim *= sequenceX[sc];
  ydim *= sequenceY[sc];
  zdim *= sequenceZ[sc];
  wdim *= sequenceW[sc];
}

void vtkRTTestSequence::Run()
{
  this->SequenceCount = this->RenderTimings->GetSequenceStart();
  int sequenceEnd = this->RenderTimings->GetSequenceEnd();
  double remainingTime = this->TargetTime;
  double lastRunTime = 0.0;
  int argc; char **argv;
  this->RenderTimings->GetArguments().GetUnusedArguments(&argc, &argv);
  // while we have enough time remaning to do a 50%
  // longer run
  while (remainingTime > 1.5*lastRunTime &&
      (sequenceEnd == 0 || this->SequenceCount <= sequenceEnd))
    {
    double startTime = vtkTimerLog::GetUniversalTime();
    this->Test->SetTargetTime(remainingTime);
    this->TestResults.push_back(this->Test->Run(this, argc, argv));
    lastRunTime = vtkTimerLog::GetUniversalTime() - startTime;
    remainingTime -= lastRunTime;
    this->SequenceCount++;
    }
}

void vtkRTTestSequence::ReportSummaryResults(ostream &ost)
{
  double result = 0.0;
  bool initialized = false;
  std::vector<vtkRTTestResult>::iterator trItr;
  for (trItr = this->TestResults.begin(); trItr != this->TestResults.end(); trItr++)
    {
    if (!initialized)
      {
      result = trItr->Results[this->Test->GetSummaryResultName()];
      initialized = true;
      }
    else
      {
      if (this->Test->UseLargestSummaryResult())
        {
        if (trItr->Results[this->Test->GetSummaryResultName()] > result)
          {
          result = trItr->Results[this->Test->GetSummaryResultName()];
          }
        }
      else
        {
        if (trItr->Results[this->Test->GetSummaryResultName()] < result)
          {
          result = trItr->Results[this->Test->GetSummaryResultName()];
          }
        }
      }
    }
  ost << this->Test->GetName() << ": " << result << " " << this->Test->GetSummaryResultName() << endl;
}

void vtkRTTestSequence::ReportDetailedResults(ostream &ost)
{
  std::vector<vtkRTTestResult>::iterator trItr;
  for (trItr = this->TestResults.begin(); trItr != this->TestResults.end(); trItr++)
    {
    ost << this->RenderTimings->GetSystemName() << ", ";
    trItr->ReportResults(this->Test, ost);
    }
}

vtkRenderTimings::vtkRenderTimings()
{
  this->TargetTime = 20.0;  // seconds
  vtksys::SystemInformation si;
  si.RunOSCheck();
  this->SystemName = si.GetOSDescription();
  this->DisplayHelp = false;
  this->SequenceStart = 0;
  this->SequenceEnd = 0;
  this->DetailedResultsFileName = "results.csv";
}

int vtkRenderTimings::RunTests()
{
  // what tests to run?
  bool useRegex = false;
  vtksys::RegularExpression re;
  if (this->Trex.size())
    {
    useRegex = true;
    re.compile(this->Trex);
    }

  size_t testCount = this->TestsToRun.size();
  std::vector<vtkRTTest *>::iterator testItr;
  if (useRegex)
    {
    testCount = 0;
    for (testItr = this->TestsToRun.begin(); testItr != this->TestsToRun.end(); testItr++)
      {
      if (re.find((*testItr)->GetName()))
        {
        testCount++;
        }
      }
    }
  for (testItr = this->TestsToRun.begin(); testItr != this->TestsToRun.end(); testItr++)
    {
    if (!useRegex || re.find((*testItr)->GetName()))
      {
      vtkRTTestSequence *ats = new vtkRTTestSequence(this);
      ats->TargetTime = this->TargetTime/testCount;
      ats->Test = *testItr;
      ats->Run();
      this->TestSequences.push_back(ats);
      }
    }

  return 0;
}

void vtkRenderTimings::ReportResults()
{
  // report the summary results to cout
  cout << "Summary results: (detailed results written to " << this->DetailedResultsFileName << ")" << endl;
  std::vector<vtkRTTestSequence *>::iterator tsItr;
  for (tsItr = this->TestSequences.begin(); tsItr != this->TestSequences.end(); tsItr++)
    {
    (*tsItr)->ReportSummaryResults(cout);
    }

  // then the detailed to a csv file
  ofstream rfile;
  rfile.open (this->DetailedResultsFileName.c_str());
  for (tsItr = this->TestSequences.begin(); tsItr != this->TestSequences.end(); tsItr++)
    {
    (*tsItr)->ReportDetailedResults(rfile);
    }
  rfile.close();
}

int vtkRenderTimings::ParseCommandLineArguments( int argc, char *argv[] )
{
  this->Arguments.Initialize(argc, argv);
  this->Arguments.StoreUnusedArguments(true);

  // default run time to be 10 seconds per test sequence
  this->TargetTime = this->TestsToRun.size()*10.0;  // seconds

  typedef vtksys::CommandLineArguments argT;
  this->Arguments.AddArgument("-Tresults", argT::SPACE_ARGUMENT, &this->DetailedResultsFileName,
    "Specify where to write the detailed results to. Defaults to results.csv.");
  this->Arguments.AddArgument("-Trex", argT::SPACE_ARGUMENT, &this->Trex,
    "Specify a regular expression for what tests should be run.");
  this->Arguments.AddArgument("-Ttime", argT::SPACE_ARGUMENT, &this->TargetTime,
    "Specify a target total amount of time for the tests to run.");
  this->Arguments.AddArgument("-Tname", argT::SPACE_ARGUMENT, &this->SystemName,
    "Specify a name for this platform. This is included in the output.");
  this->Arguments.AddBooleanArgument("--help", &this->DisplayHelp,
    "Provide a listing of command line options.");
  this->Arguments.AddArgument("-Tss", argT::SPACE_ARGUMENT, &this->SequenceStart,
    "Specify a starting index for test sequences. Tests are designed to start at "
    "a scale that can run on even very small systems. If you have a more powerful "
    "system, you can use this option to skip the first few steps in the test "
    "sequence. The sequence starts at zero and increases an order of magnitude "
    "for every four steps");
  this->Arguments.AddArgument("-Tse", argT::SPACE_ARGUMENT, &this->SequenceEnd,
    "Specify an ending index for test sequences. Even if there is time remaining "
    "a test sequence will not go beyond this value. You can combine this option "
    "with -Tss to run just one iteration of a sequece. For example you can "
    "use -Tss 6 -Tse 6 to only run the 6th sequence. A value of 0 means that "
    "there is no limit (the time limit will still stop the tests).");

  if ( !this->Arguments.Parse() )
    {
    cerr << "Problem parsing arguments" << endl;
    return 1;
    }

  if (this->DisplayHelp)
    {
    cerr << "Usage" << endl << endl << "  VTKRenderTimings [options]" << endl << endl << "Options" << endl;
    cerr << this->Arguments.GetHelp();
    return 0;
    }

  // run the tests
  cout << "Starting tests, expected to run for " << this->TargetTime << " seconds." << endl;
  this->RunTests();
  this->ReportResults();

  return 0;
 }
