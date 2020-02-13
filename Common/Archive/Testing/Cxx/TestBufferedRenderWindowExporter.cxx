/*=========================================================================

Program:   Visualization Toolkit

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkActor.h"
#include "vtkLight.h"
#include "vtkNew.h"
#include "vtkPolyDataMapper.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkSphereSource.h"
#include "vtkTestUtilities.h"
#include "vtkWindowNode.h"
#include "vtksys/SystemTools.hxx"

#include "vtkArchiver.h"
#include "vtkJSONRenderWindowExporter.h"

#include "vtkBufferedArchiver.h"

#include <archive.h>
#include <archive_entry.h>

#include <cstdio>
#include <cstring>
#include <fstream>

// Construct a render window and write it to disk and to buffer. Decompress the
// buffer and compare its contents to the files on disk.
int TestBufferedRenderWindowExporter(int argc, char* argv[])
{
  char* tempDir =
    vtkTestUtilities::GetArgOrEnvOrDefault("-T", argc, argv, "VTK_TEMP_DIR", "Testing/Temporary");
  if (!tempDir)
  {
    std::cout << "Could not determine temporary directory.\n";
    return EXIT_FAILURE;
  }
  std::string testDirectory = tempDir;
  delete[] tempDir;

  std::string directoryName = testDirectory + std::string("/") + std::string("ExportVtkJS");

  vtkNew<vtkSphereSource> sphere;
  vtkNew<vtkPolyDataMapper> pmap;
  pmap->SetInputConnection(sphere->GetOutputPort());

  vtkNew<vtkRenderWindow> rwin;

  vtkNew<vtkRenderer> ren;
  rwin->AddRenderer(ren);

  vtkNew<vtkLight> light;
  ren->AddLight(light);

  vtkNew<vtkActor> actor;
  ren->AddActor(actor);

  actor->SetMapper(pmap);

  {
    vtkNew<vtkJSONRenderWindowExporter> exporter;
    exporter->GetArchiver()->SetArchiveName(directoryName.c_str());
    exporter->SetRenderWindow(rwin);
    exporter->Write();
  }

  vtkNew<vtkJSONRenderWindowExporter> exporter;
  vtkNew<vtkBufferedArchiver> bufferedArchiver;
  exporter->SetArchiver(bufferedArchiver);
  exporter->SetRenderWindow(rwin);
  exporter->Write();

  struct archive* a = archive_read_new();
  archive_read_support_filter_gzip(a);
  archive_read_support_format_zip(a);
#if ARCHIVE_VERSION_NUMBER < 3002000
  int r = archive_read_open_memory(
    a, const_cast<char*>(bufferedArchiver->GetBuffer()), bufferedArchiver->GetBufferSize());
#else
  int r =
    archive_read_open_memory(a, bufferedArchiver->GetBuffer(), bufferedArchiver->GetBufferSize());
#endif

  if (r != ARCHIVE_OK)
  {
    vtkErrorWithObjectMacro(nullptr, "Cannot open archive from memory");
    return EXIT_FAILURE;
  }

  struct archive_entry* entry;
  char* buffer;
  std::size_t size;
  while (archive_read_next_header(a, &entry) == ARCHIVE_OK && r == ARCHIVE_OK)
  {
    std::string fileName = directoryName + "/" + archive_entry_pathname(entry);
    size = archive_entry_size(entry);
    buffer = (char*)malloc(size);
    if (buffer == nullptr)
    {
      vtkErrorWithObjectMacro(nullptr, "Could not allocate buffer");
      r = ARCHIVE_FATAL;
      break;
    }

    archive_read_data(a, buffer, size);

    {
      std::FILE* fp;
      char* fbuffer;

      fp = std::fopen(fileName.c_str(), "rb");
      if (fp == nullptr)
      {
        vtkErrorWithObjectMacro(nullptr, "Could not open file on disk");
        r = ARCHIVE_FATAL;
        std::free(buffer);
        break;
      }

      std::fseek(fp, 0L, SEEK_END);
      long lSize = std::ftell(fp);
      if (size != static_cast<std::size_t>(lSize))
      {
        vtkErrorWithObjectMacro(nullptr, "Buffered file size does not match file size on disk");
        r = ARCHIVE_FATAL;
        std::fclose(fp);
        std::free(buffer);
        break;
      }

      std::rewind(fp);

      // allocate memory for entire content
      fbuffer = (char*)malloc(lSize);
      if (fbuffer == nullptr)
      {
        r = ARCHIVE_FATAL;
        std::fclose(fp);
        std::free(buffer);
        break;
      }

      // copy the file into the buffer
      std::size_t r_ = std::fread(fbuffer, lSize, 1, fp);
      (void)r_;

      if (std::memcmp(fbuffer, buffer, size) != 0)
      {
        vtkErrorWithObjectMacro(nullptr, "Buffered file does not match file on disk");
        r = ARCHIVE_FATAL;
      }

      std::fclose(fp);
      std::free(buffer);
      std::free(fbuffer);
    }
  }

  if (r != ARCHIVE_OK)
  {
    vtkErrorWithObjectMacro(nullptr, "Comparison to on-disk archive failed");
    archive_read_free(a);
    return EXIT_FAILURE;
  }

  r = archive_read_free(a);
  if (r != ARCHIVE_OK)
  {
    vtkErrorWithObjectMacro(nullptr, "Cannot close archive");
    return EXIT_FAILURE;
  }

  vtksys::SystemTools::RemoveADirectory(directoryName.c_str());

  return EXIT_SUCCESS;
}
