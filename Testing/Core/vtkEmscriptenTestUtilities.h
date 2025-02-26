// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// Emscripten makes use of a sandboxed filesystem inside a web browser
// These are utility functions to preload files into the sandbox
#ifndef vtkEmscriptenTestUtilities_h
#define vtkEmscriptenTestUtilities_h
#ifdef __EMSCRIPTEN__

#include "vtkTestingCoreModule.h" // for export macro
#include <cstdint>                // for uint8_t
#include <string>                 // for string

#include <emscripten/emscripten.h> // for emscripten_ API

VTK_ABI_NAMESPACE_BEGIN

class VTKTESTINGCORE_EXPORT vtkEmscriptenTestUtilities
{
public:
  struct PreloadDescriptor
  {
    uint8_t* buffer;
    size_t size;
  };
  static void PreloadDataFile(
    const std::string& hostFilePath, const std::string& sandboxedFilePath);
  static std::string PreloadDataFile(const std::string& hostFilePath);
  static void DumpFile(const std::string& hostFilePath, const void* data, std::size_t n);
  static void PostExitCode(int code);
};

// These functions are implemented externally in javascript.
// The static methods of vtkEmscriptenTestUtilities call into these methods.
extern "C"
{
  void* vtkPreloadDataFileIntoMemory(const char* hostFilePath);
  void vtkDumpFile(const char* hostFilePath, const uint8_t* data, size_t nbytes);
  void vtkPostExitCode(int code);
}

VTK_ABI_NAMESPACE_END
#endif // __EMSCRIPTEN__
#endif // vtkEmscriptenTestUtilities_h
