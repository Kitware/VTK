/*=========================================================================

  Program:   Visualization Toolkit
  Module:    VTKRenderTimings.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkRenderTimings_h
#define vtkRenderTimings_h

/**
 * Define the classes we use for running timing benchmarks
 */

#include "vtkTimerLog.h"
#include <vtksys/SystemInformation.hxx>
#include <vtksys/RegularExpression.hxx>
#include <vtksys/CommandLineArguments.hxx>
#include <map>

class vtkRTTestResult;
class vtkRTTestSequence;
class vtkRenderTimings;

class vtkRTTest
{
public:
  // what is the name of this test
  std::string GetName() { return this->Name; }

  // when reporting a summary result use this key to
  // determine the amount of triangles rendered
  virtual const char *GetSecondSummaryResultName() = 0;

  // when reporting a summary result this is the
  // field that should be reported.
  virtual const char *GetSummaryResultName() = 0;

  // when reporting a summary result should we use the
  // largest value or smallest?
  virtual bool UseLargestSummaryResult() { return true; }

  // Set/Get the time allowed for this test
  // Tests should check if they are going more than 50%
  // beyond this number they should short circuit if
  // they can gracefully.
  virtual void SetTargetTime(float tt) { this->TargetTime = tt; }
  virtual float GetTargetTime() { return this->TargetTime; }

  void SetRenderSize(int width, int height) { this->RenderWidth = width; this->RenderHeight = height; }
  int GetRenderWidth() { return this->RenderWidth; }
  int GetRenderHeight() { return this->RenderHeight; }

  // run the test, argc and argv are extra arguments that the test might
  // use.
  virtual vtkRTTestResult Run(vtkRTTestSequence *ats, int argc, char *argv[]) = 0;

  vtkRTTest(const char *name)
  {
    this->TargetTime = 1.0;
    this->Name = name;
    RenderWidth = RenderHeight = 600;
  }

  virtual ~vtkRTTest() {}

protected:
  float TargetTime;
  std::string Name;
  int RenderWidth, RenderHeight;
};

class vtkRTTestResult
{
public:
  std::map<std::string,double> Results;
  int SequenceNumber;
  void ReportResults(vtkRTTest *test, ostream &ost)
  {
    ost << test->GetName();
    std::map<std::string, double>::iterator rItr;
    for (rItr = this->Results.begin(); rItr != this->Results.end(); ++rItr)
      {
      ost << ", " << rItr->first << ", " << rItr->second;
      }
    ost << "\n";
  }
};

class vtkRTTestSequence
{
public:
  virtual void Run();
  virtual void ReportSummaryResults(ostream &ost);
  virtual void ReportDetailedResults(ostream &ost);

  // tests should use these functions to determine what resolution
  // to use in scaling their test. The functions will always return
  // numbers then when multiplied will result in 1, 2, 3, or 5
  // times 10 to some power. These functions use the SequenceCount
  // to determine what number to return. When the dimensions
  // are not equal, we guarantee that the larger dimensions
  // come first
  void GetSequenceNumbers(int &xdim);
  void GetSequenceNumbers(int &xdim, int &ydim);
  void GetSequenceNumbers(int &xdim, int &ydim, int &zdim);
  void GetSequenceNumbers(int &xdim, int &ydim, int &zdim, int &wdim);

  // display the results in realtime using VTK charting
  void SetChartResults(bool v) { this->ChartResults = v; }

  vtkRTTest *Test;
  float TargetTime;

  vtkRTTestSequence(vtkRenderTimings *rt)
    {
    this->Test = NULL;
    this->TargetTime = 10.0;
    this->RenderTimings = rt;
    this->ChartResults = true;
    }

virtual ~vtkRTTestSequence() {}

protected:
  std::vector<vtkRTTestResult> TestResults;
  int SequenceCount;
  vtkRenderTimings *RenderTimings;
  bool ChartResults;
};

// a class to run a bunch of timing tests and
// report the results
class vtkRenderTimings
{
public:
  vtkRenderTimings();

  // get the sequence start and end values
  int GetSequenceStart() { return this->SequenceStart; }
  int GetSequenceEnd() { return this->SequenceEnd; }

  // get the maxmimum time allowed per step
  double GetSequenceStepTimeLimit() { return this->SequenceStepTimeLimit; }

  // get the render size
  int GetRenderWidth() { return this->RenderWidth; }
  int GetRenderHeight() { return this->RenderHeight; }

  // parse and act on the command line arguments
  int ParseCommandLineArguments(int argc, char *argv[]);

  // get the arguments
  vtksys::CommandLineArguments &GetArguments() { return this->Arguments; }

  std::string GetSystemName() { return this->SystemName; }

  std::vector<vtkRTTest *> TestsToRun;
  std::vector<vtkRTTestSequence *> TestSequences;

protected:
  int RunTests();
  void ReportResults();

private:
  std::string Regex; // regualr expression for tests
  double TargetTime;
  std::string SystemName;
  vtksys::CommandLineArguments Arguments;
  bool DisplayHelp;
  bool ListTests;
  bool NoChartResults;
  int SequenceStart;
  int SequenceEnd;
  double SequenceStepTimeLimit;
  std::string DetailedResultsFileName;
  int RenderWidth;
  int RenderHeight;
};

#endif
