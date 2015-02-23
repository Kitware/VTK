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
//#include "vtkNew.h"

#include "vtkAxis.h"
#include "vtkChartLegend.h"
#include "vtkChartXY.h"
#include "vtkContextScene.h"
#include "vtkContextView.h"
#include "vtkDelimitedTextWriter.h"
#include "vtkDoubleArray.h"
#include "vtkPlot.h"
#include "vtkTable.h"
#include "vtkRenderWindow.h"

void vtkRTTestSequence::GetSequenceNumbers(int &xdim)
{
  static int linearSequence[] = {1, 2, 3, 5};

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

  vtkNew<vtkContextView> chartView;
  vtkNew<vtkChartXY> chart;
  vtkNew<vtkTable> results;
  vtkNew<vtkDoubleArray> summary;
  vtkNew<vtkDoubleArray> secondSummary;
  if (this->ChartResults)
    {
    // Set up our results table, this will be used for our timings etc.
    summary->SetName(this->Test->GetSummaryResultName());
    secondSummary->SetName(this->Test->GetSecondSummaryResultName());
    results->AddColumn(secondSummary.Get());
    results->AddColumn(summary.Get());

    // Set up a chart to show the data being generated in real time.
    chartView->GetRenderWindow()->SetSize(700, 500);
    chartView->GetRenderWindow()->SetPosition(700, 0);
    chartView->GetScene()->AddItem(chart.Get());
    vtkPlot *plot = chart->AddPlot(vtkChart::LINE);
    plot->SetInputData(results.Get(), 0, 1);
    chart->GetAxis(vtkAxis::LEFT)->SetTitle(this->Test->GetSummaryResultName());
    chart->GetAxis(vtkAxis::LEFT)->LogScaleOn();
    chart->GetAxis(vtkAxis::BOTTOM)->SetTitle(this->Test->GetSecondSummaryResultName());
    chart->GetAxis(vtkAxis::BOTTOM)->LogScaleOn();
    results->SetNumberOfRows(100);  // have to initialze this or it fails
    }

  double remainingTime = this->TargetTime;
  double lastRunTime = 0.0;
  int argc;
  char **argv;
  this->RenderTimings->GetArguments().GetUnusedArguments(&argc, &argv);
  // while we have enough time remaning to do a 50% longer run
  double stepLimit = this->RenderTimings->GetSequenceStepTimeLimit();
  while (remainingTime > 1.5 * lastRunTime &&
      (sequenceEnd == 0 || this->SequenceCount <= sequenceEnd) &&
      lastRunTime < stepLimit)
    {
    double targetTime = remainingTime > stepLimit ? stepLimit : remainingTime;
    double startTime = vtkTimerLog::GetUniversalTime();
    this->Test->SetTargetTime(targetTime);
    vtkRTTestResult tr = this->Test->Run(this, argc, argv);
    tr.SequenceNumber = this->SequenceCount;
    this->TestResults.push_back(tr);

    if (this->ChartResults)
      {
      int numRows = this->SequenceCount -
        this->RenderTimings->GetSequenceStart() + 1;
      results->SetNumberOfRows(numRows);
      results->SetValue(numRows-1, 0,
        tr.Results[this->Test->GetSecondSummaryResultName()]);
      results->SetValue(numRows-1, 1,
        tr.Results[this->Test->GetSummaryResultName()]);
      results->Modified();
      if (numRows > 1)
        {
        chart->RecalculateBounds();
        chartView->Render();
        }
      }

    lastRunTime = vtkTimerLog::GetUniversalTime() - startTime;
    remainingTime -= lastRunTime;
    this->SequenceCount++;
    }
}

void vtkRTTestSequence::ReportSummaryResults(ostream &ost)
{
  double result = 0.0;
  vtkRTTestResult *bestTestResult = NULL;
  bool initialized = false;
  std::vector<vtkRTTestResult>::iterator trItr;
  for (trItr = this->TestResults.begin(); trItr != this->TestResults.end(); trItr++)
    {
    if (!initialized)
      {
      result = trItr->Results[this->Test->GetSummaryResultName()];
      bestTestResult = &(*trItr);
      initialized = true;
      }
    else
      {
      if (this->Test->UseLargestSummaryResult())
        {
        if (trItr->Results[this->Test->GetSummaryResultName()] > result)
          {
          result = trItr->Results[this->Test->GetSummaryResultName()];
          bestTestResult = &(*trItr);
          }
        }
      else
        {
        if (trItr->Results[this->Test->GetSummaryResultName()] < result)
          {
          result = trItr->Results[this->Test->GetSummaryResultName()];
          bestTestResult = &(*trItr);
          }
        }
      }
    }
  ost << this->Test->GetName() << ":" << bestTestResult->SequenceNumber
    << ": " << result << " " << this->Test->GetSummaryResultName()
    << " and " << static_cast<vtkIdType>(bestTestResult->Results[this->Test->GetSecondSummaryResultName()])
    << " " << this->Test->GetSecondSummaryResultName() << endl;
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
  this->TargetTime = 600.0;  // 10 minutes
  vtksys::SystemInformation si;
  si.RunOSCheck();
  this->SystemName = si.GetOSDescription();
  this->DisplayHelp = false;
  this->ListTests = false;
  this->NoChartResults = false;
  this->SequenceStart = 0;
  this->SequenceEnd = 0;
  this->SequenceStepTimeLimit = 15.0; // seconds
  this->DetailedResultsFileName = "results.csv";
}

int vtkRenderTimings::RunTests()
{
  // what tests to run?
  bool useRegex = false;
  vtksys::RegularExpression re;
  if (this->Regex.size())
    {
    useRegex = true;
    re.compile(this->Regex);
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
      ats->SetChartResults(!this->NoChartResults);
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

  typedef vtksys::CommandLineArguments argT;
  this->Arguments.AddArgument("-rn", argT::SPACE_ARGUMENT, &this->DetailedResultsFileName,
    "Specify where to write the detailed results to. Defaults to results.csv.");
  this->Arguments.AddArgument("-regex", argT::SPACE_ARGUMENT, &this->Regex,
    "Specify a regular expression for what tests should be run.");
  this->Arguments.AddArgument("-tls", argT::SPACE_ARGUMENT,
    &this->SequenceStepTimeLimit,
    "Specify a maximum time in seconds allow for a sequence step. Once exceeded "
    "the test sequence will terminate.");
  this->Arguments.AddArgument("-tl", argT::SPACE_ARGUMENT, &this->TargetTime,
    "Specify a target total amount of time for the tests to run. ");
  this->Arguments.AddArgument("-platform", argT::SPACE_ARGUMENT, &this->SystemName,
    "Specify a name for this platform. This is included in the output.");
  this->Arguments.AddBooleanArgument("--help", &this->DisplayHelp,
    "Provide a listing of command line options.");
  this->Arguments.AddBooleanArgument("-help", &this->DisplayHelp,
    "Provide a listing of command line options.");
  this->Arguments.AddArgument("-ss", argT::SPACE_ARGUMENT, &this->SequenceStart,
    "Specify a starting index for test sequences. Tests are designed to start at "
    "a scale that can run on even very small systems. If you have a more powerful "
    "system, you can use this option to skip the first few steps in the test "
    "sequence. The sequence starts at zero and increases an order of magnitude "
    "for every four steps");
  this->Arguments.AddArgument("-se", argT::SPACE_ARGUMENT, &this->SequenceEnd,
    "Specify an ending index for test sequences. Even if there is time remaining "
    "a test sequence will not go beyond this value. You can combine this option "
    "with -ss to run just one iteration of a sequece. For example you can "
    "use -ss 6 -se 6 to only run the 6th sequence. A value of 0 means that "
    "there is no limit (the time limit will still stop the tests).");
  this->Arguments.AddBooleanArgument("-list", &this->ListTests,
    "Provide a listing of available tests.");
  this->Arguments.AddBooleanArgument("-nochart", &this->NoChartResults,
    "Suppress realtime charting of test performance.");

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

  if (this->ListTests)
    {
    bool useRegex = false;
    vtksys::RegularExpression re;
    if (this->Regex.size())
      {
      useRegex = true;
      re.compile(this->Regex);
      }
    std::vector<vtkRTTest *>::iterator testItr;
    for (testItr = this->TestsToRun.begin(); testItr != this->TestsToRun.end(); testItr++)
      {
      if (!useRegex || re.find((*testItr)->GetName()))
        {
        cerr << (*testItr)->GetName() << endl;
        }
      }
    return 0;
    }

  // run the tests
  cout << "Starting tests, maximum time allowed is " << this->TargetTime << " seconds." << endl;
  this->RunTests();
  this->ReportResults();

  return 0;
 }
