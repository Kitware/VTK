// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// on msvc add in stack trace info as systeminformation
// does not seem to include it.
//
#ifndef VTK_WINDOWS_TEST_UTILITIES
#define VTK_WINDOWS_TEST_UTILITIES

#include "vtkCompiler.h"

#if defined(VTK_COMPILER_MSVC) && defined(_WIN32)
#include <sstream>
#include <windows.h>

VTK_ABI_NAMESPACE_BEGIN
inline LONG WINAPI vtkWindowsTestUlititiesExceptionHandler(EXCEPTION_POINTERS* ExceptionInfo)
{
  switch (ExceptionInfo->ExceptionRecord->ExceptionCode)
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

  std::string stack = vtksys::SystemInformation::GetProgramStack(0, 0);

  vtkLog(ERROR, << stack);

  return EXCEPTION_CONTINUE_SEARCH;
}

inline void vtkWindowsTestUtilitiesSetupForTesting()
{
  SetUnhandledExceptionFilter(vtkWindowsTestUlititiesExceptionHandler);
}
VTK_ABI_NAMESPACE_END
#else
VTK_ABI_NAMESPACE_BEGIN
inline void vtkWindowsTestUtilitiesSetupForTesting() {}
VTK_ABI_NAMESPACE_END
#endif
#endif
// VTK-HeaderTest-Exclude: vtkWindowsTestUtilities.h
