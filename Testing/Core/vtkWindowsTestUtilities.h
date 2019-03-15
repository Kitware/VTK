/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWindowsTestUtilities.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// on msvc add in stack trace info as systeminformation
// does not seem to include it.
//
#ifndef VTK_WINDOWS_TEST_UTILITIES
#define VTK_WINDOWS_TEST_UTILITIES

#if defined(VTK_COMPILER_MSVC) && defined(WIN32)
#define TRACE_MAX_STACK_FRAMES 1024
#define TRACE_MAX_FUNCTION_NAME_LENGTH 1024
#include <windows.h>
#pragma warning( push )
#pragma warning( disable : 4091 )
#include "dbghelp.h"
#pragma warning( pop )
#include <sstream>

inline
std::string vtkWindowsTestUltitiesGetProgramStack()
{
  void *stack[TRACE_MAX_STACK_FRAMES];
  HANDLE process = GetCurrentProcess();
  SymInitialize(process, NULL, TRUE);
  WORD numberOfFrames = CaptureStackBackTrace(0, TRACE_MAX_STACK_FRAMES, stack, NULL);
  SYMBOL_INFO *symbol = (SYMBOL_INFO *)malloc(sizeof(SYMBOL_INFO)+(TRACE_MAX_FUNCTION_NAME_LENGTH - 1) * sizeof(TCHAR));
  symbol->MaxNameLen = TRACE_MAX_FUNCTION_NAME_LENGTH;
  symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
  DWORD displacement;
  IMAGEHLP_LINE64 *line = (IMAGEHLP_LINE64 *)malloc(sizeof(IMAGEHLP_LINE64));
  line->SizeOfStruct = sizeof(IMAGEHLP_LINE64);
  std::ostringstream oss;
  for (int i = 4; i < numberOfFrames - 3; i++)
  {
    DWORD64 address = (DWORD64)(stack[i]);
    SymFromAddr(process, address, NULL, symbol);
    if (SymGetLineFromAddr64(process, address, &displacement, line))
    {
      oss << " at " << symbol->Name << " in " <<  line->FileName << " line " << line->LineNumber << "\n";
    }
    else
    {
      oss << " at " << symbol->Name << "\n";
    }
  }
  return oss.str();
}

inline
LONG WINAPI vtkWindowsTestUlititiesExceptionHandler(EXCEPTION_POINTERS * ExceptionInfo)
{
  switch(ExceptionInfo->ExceptionRecord->ExceptionCode)
  {
    case EXCEPTION_ACCESS_VIOLATION:
      vtkLog(ERROR, << "Error: EXCEPTION_ACCESS_VIOLATION\n");
      break;
    case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
      vtkLog(ERROR, << "Error: EXCEPTION_ARRAY_BOUNDS_EXCEEDED\n");
      break;
    case EXCEPTION_BREAKPOINT:
      vtkLog(ERROR, << "Error: EXCEPTION_BREAKPOINT\n");
      break;
    case EXCEPTION_DATATYPE_MISALIGNMENT:
      vtkLog(ERROR, << "Error: EXCEPTION_DATATYPE_MISALIGNMENT\n");
      break;
    case EXCEPTION_FLT_DENORMAL_OPERAND:
      vtkLog(ERROR, << "Error: EXCEPTION_FLT_DENORMAL_OPERAND\n");
      break;
    case EXCEPTION_FLT_DIVIDE_BY_ZERO:
      vtkLog(ERROR, << "Error: EXCEPTION_FLT_DIVIDE_BY_ZERO\n");
      break;
    case EXCEPTION_FLT_INEXACT_RESULT:
      vtkLog(ERROR, << "Error: EXCEPTION_FLT_INEXACT_RESULT\n");
      break;
    case EXCEPTION_FLT_INVALID_OPERATION:
      vtkLog(ERROR, << "Error: EXCEPTION_FLT_INVALID_OPERATION\n");
      break;
    case EXCEPTION_FLT_OVERFLOW:
      vtkLog(ERROR, << "Error: EXCEPTION_FLT_OVERFLOW\n");
      break;
    case EXCEPTION_FLT_STACK_CHECK:
      vtkLog(ERROR, << "Error: EXCEPTION_FLT_STACK_CHECK\n");
      break;
    case EXCEPTION_FLT_UNDERFLOW:
      vtkLog(ERROR, << "Error: EXCEPTION_FLT_UNDERFLOW\n");
      break;
    case EXCEPTION_ILLEGAL_INSTRUCTION:
      vtkLog(ERROR, << "Error: EXCEPTION_ILLEGAL_INSTRUCTION\n");
      break;
    case EXCEPTION_IN_PAGE_ERROR:
      vtkLog(ERROR, << "Error: EXCEPTION_IN_PAGE_ERROR\n");
      break;
    case EXCEPTION_INT_DIVIDE_BY_ZERO:
      vtkLog(ERROR, << "Error: EXCEPTION_INT_DIVIDE_BY_ZERO\n");
      break;
    case EXCEPTION_INT_OVERFLOW:
      vtkLog(ERROR, << "Error: EXCEPTION_INT_OVERFLOW\n");
      break;
    case EXCEPTION_INVALID_DISPOSITION:
      vtkLog(ERROR, << "Error: EXCEPTION_INVALID_DISPOSITION\n");
      break;
    case EXCEPTION_NONCONTINUABLE_EXCEPTION:
      vtkLog(ERROR, << "Error: EXCEPTION_NONCONTINUABLE_EXCEPTION\n");
      break;
    case EXCEPTION_PRIV_INSTRUCTION:
      vtkLog(ERROR, << "Error: EXCEPTION_PRIV_INSTRUCTION\n");
      break;
    case EXCEPTION_SINGLE_STEP:
      vtkLog(ERROR, << "Error: EXCEPTION_SINGLE_STEP\n");
      break;
    case EXCEPTION_STACK_OVERFLOW:
      vtkLog(ERROR, << "Error: EXCEPTION_STACK_OVERFLOW\n");
      break;
    default:
      vtkLog(ERROR, << "Error: Unrecognized Exception\n");
      break;
  }

  std::string stack =
    vtkWindowsTestUltitiesGetProgramStack();

  vtkLog(ERROR, << stack);

  return EXCEPTION_CONTINUE_SEARCH;
}

inline void vtkWindowsTestUtilitiesSetupForTesting(void)
{
  SetUnhandledExceptionFilter(vtkWindowsTestUlititiesExceptionHandler);
}
#else
inline
void vtkWindowsTestUtilitiesSetupForTesting(void)
{
  return;
}
#endif
#endif
// VTK-HeaderTest-Exclude: vtkWindowsTestUtilities.h
