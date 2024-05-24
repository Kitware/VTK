// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#ifdef __EMSCRIPTEN__

#include "vtkEmscriptenTestUtilities.h"

#include <cstdlib>                // for free
#include <emscripten.h>           // for EM_ASM macro
#include <iostream>               // for cerr, cout
#include <vtksys/SystemTools.hxx> // for filesystem utilities

VTK_ABI_NAMESPACE_BEGIN

/**
 * Given a path to a file "a/b/c/d.ext" on the server hosting webassembly runtime/page,
 * this method preloads it into the sandbox at the given `sandboxedFilePath`
 */
void vtkEmscriptenTestUtilities::PreloadDataFile(
  const std::string& hostFilePath, const std::string& sandboxedFilePath)
{
  FILE* f = nullptr;
  // This function preloads the file and it's size in bytes into wasm memory from javascript.
  // It returns a pointer to a PreloadData.
  auto* payload =
    reinterpret_cast<PreloadDescriptor*>(vtkPreloadDataFileIntoMemory(hostFilePath.c_str()));
  if (payload != nullptr)
  {
    const auto dirName = vtksys::SystemTools::GetFilenamePath(sandboxedFilePath);
    if (!dirName.empty() && dirName != "/")
    {
      const auto status = vtksys::SystemTools::MakeDirectory(dirName);
      if (!status.IsSuccess())
      {
        std::cerr << "Failed to create parent directory for " << sandboxedFilePath << '\n';
        free(payload->buffer);
        free(payload);
        return;
      }
      else
      {
        std::cout << "Created directory for " << sandboxedFilePath << '\n';
      }
    }
    f = fopen(sandboxedFilePath.c_str(), "wb+");
    if (f != nullptr)
    {
      auto written = fwrite(payload->buffer, sizeof(payload->buffer[0]), payload->size, f);
      fclose(f);
      if (written != payload->size)
      {
        std::cerr << "Failed to preload " << payload->size << " bytes from " << hostFilePath
                  << " into " << sandboxedFilePath << "\n";
      }
      else
      {
        std::cout << "Preloaded " << payload->size << " bytes from " << hostFilePath << " into "
                  << sandboxedFilePath << "\n";
      }
    }
    else
    {
      std::cerr << "Failed to create sandboxed " << sandboxedFilePath << " for " << hostFilePath
                << '\n';
    }
    free(payload->buffer);
    free(payload);
  }
}

/**
 * Given a path to a file "a/b/c/d.ext" on the server hosting webassembly runtime/page,
 * this method preloads it into the sandbox at "/d.ext" and returns "/d.ext"
 */
std::string vtkEmscriptenTestUtilities::PreloadDataFile(const std::string& hostFilePath)
{
  std::string sandboxedFilePath = vtksys::SystemTools::GetFilenameName(hostFilePath);
  vtkEmscriptenTestUtilities::PreloadDataFile(hostFilePath, sandboxedFilePath);
  return sandboxedFilePath;
}

/**
 * This method helps to write files outside of the webassembly sandbox.
 * Writes `n` number of bytes from `data` into a file called `hostFilePath` on the server's file
 * system. The server is a system which hosts the webassembly runtime/page.
 */
void vtkEmscriptenTestUtilities::DumpFile(
  const std::string& hostFilePath, const void* data, size_t n)
{
  const auto* bytes = reinterpret_cast<const uint8_t*>(data);
  vtkDumpFile(hostFilePath.c_str(), bytes, n);
}
VTK_ABI_NAMESPACE_END
#endif // __EMSCRIPTEN__
