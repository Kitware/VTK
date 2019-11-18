/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestLogger.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// Test some generic features of vtkLogger.

#include "vtkLogger.h"
#include "vtkObject.h"

#include <string>
#include <vector>

namespace
{
void log_handler(void* user_data, const vtkLogger::Message& message)
{
  auto lines = reinterpret_cast<std::string*>(user_data);
  (*lines) += "\n";
  (*lines) += message.message;
}
}

int TestLogger(int, char*[])
{
  std::string lines;
  vtkLogF(INFO, "changing verbosity to %d", vtkLogger::VERBOSITY_TRACE);
  vtkLogger::AddCallback("sonnet-grabber", log_handler, &lines, vtkLogger::VERBOSITY_2);
  vtkLogger::SetStderrVerbosity(vtkLogger::VERBOSITY_TRACE);
  vtkLogScopeFunction(TRACE);
  {
    vtkLogScopeF(TRACE, "Sonnet 18");
    auto whom = "thee";
    vtkLog(2, "Shall I compare " << whom << " to a summer's day?");

    auto what0 = "lovely";
    auto what1 = "temperate";
    vtkLogF(2, "Thou art more %s and more %s:", what0, what1);

    auto month = "May";
    vtkLogIf(2, true, << "Rough winds do shake the darling buds of " << month << ",");
    vtkLogIfF(2, true, "And %s’s lease hath all too short a date;", "summers");
  }

  cerr << "--------------------------------------------" << endl
       << lines << endl
       << endl
       << "--------------------------------------------" << endl;

  vtkGenericWarningMacro("testing generic warning -- should only show up in the log");

  // remove callback since the user-data becomes invalid out of this function.
  vtkLogger::RemoveCallback("sonnet-grabber");

  // test out explicit scope start and end markers.
  {
    vtkLogStartScope(INFO, "scope-0");
  }
  vtkLogStartScopeF(INFO, "scope-1", "scope %d", 1);
  vtkLog(INFO, "some text");
  vtkLogEndScope("scope-1");
  {
    vtkLogEndScope("scope-0");
  }
  return EXIT_SUCCESS;
}
