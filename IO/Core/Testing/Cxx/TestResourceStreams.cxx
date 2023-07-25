// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkFileResourceStream.h"
#include "vtkMemoryResourceStream.h"
#include "vtkNew.h"
#include "vtkTestUtilities.h"

#include <vtksys/FStream.hxx>

#include <array>
#include <cstring>
#include <iostream>

#define check(expr, message)                                                                       \
  if (!(expr))                                                                                     \
  {                                                                                                \
    vtkErrorWithObjectMacro(nullptr, "Test failed: \n" << message);                                \
    return false;                                                                                  \
  }

bool testStream(vtkResourceStream* stream)
{
  check(!stream->EndOfStream(), "Invalid stream");

  std::array<char, 5> buffer;

  check(stream->Read(buffer.data(), buffer.size()) == 5, "Read wrong size");
  check(!stream->EndOfStream(), "Reach end of file too early");
  check(std::strncmp(buffer.data(), "Hello", 5) == 0, "Read wrong data");

  if (stream->SupportSeek())
  {
    check(stream->Tell() == 5, "Tell wrong position");

    check(stream->Seek(2, vtkResourceStream::SeekDirection::Current) == 7, "Seek wrong position");
    check(!stream->EndOfStream(), "Seek must not modify EndOfStream value");
    check(stream->Tell() == 7, "Tell wrong position");

    check(stream->Seek(10, vtkResourceStream::SeekDirection::Current) == 17, "Seek wrong position");
    check(!stream->EndOfStream(), "Seek must not modify EndOfStream value");
    check(stream->Read(nullptr, 0) == 0, "Read wrong size");
    check(!stream->EndOfStream(), "Last zero byte read must not set EOS");

    check(stream->Seek(10, vtkResourceStream::SeekDirection::Current) == 27, "Seek wrong position");
    check(!stream->EndOfStream(), "EndOfStream must be false after Seek");
    check(stream->Read(buffer.data(), 0) == 0, "Read wrong size");
    check(!stream->EndOfStream(), "Last zero byte read must not set EOS");

    check(stream->Seek(10, vtkResourceStream::SeekDirection::Current) == 37, "Seek wrong position");
    check(!stream->EndOfStream(), "EndOfStream must be false after Seek");
    check(stream->Read(buffer.data(), buffer.size()) == 0, "Read wrong size");
    check(stream->EndOfStream(), "Last read must lead the stream to EOS");

    check(stream->Seek(2, vtkResourceStream::SeekDirection::Begin) == 2, "Seek wrong position");
    check(!stream->EndOfStream(), "EndOfStream must be false after Seek");
    check(stream->Seek(-6, vtkResourceStream::SeekDirection::End) == 6, "Seek wrong position");
    check(!stream->EndOfStream(), "EndOfStream must be false after Seek");
  }
  else
  {
    check(!stream->EndOfStream(), "Reach end of file too early");
  }

  check(stream->Read(buffer.data(), buffer.size()) == 5, "Read wrong size");
  check(!stream->EndOfStream(), "Reach end of file too early");
  check(std::strncmp(buffer.data(), "world", 5) == 0, "Read wrong data");

  check(stream->Read(buffer.data(), buffer.size()) == 1, "Read wrong size");
  check(std::strncmp(buffer.data(), "!", 1) == 0, "Read wrong data");
  check(stream->EndOfStream(), "Last read must lead the stream to EOS");

  return true;
}

bool testFileResource(const std::string& temp_dir)
{
  const auto file_path = temp_dir + "/restmp.txt";

  vtksys::ofstream{ file_path.c_str(), std::ios_base::binary } << "Hello world!";

  vtkNew<vtkFileResourceStream> file;
  if (!file->Open(file_path.c_str()))
  {
    return false;
  }

  if (file->Open(nullptr))
  {
    return false;
  }

  if (!file->Open(file_path.c_str()))
  {
    return false;
  }

  return testStream(file);
}

bool testMemoryResource()
{
  const std::string str{ "Hello world!" };
  vtkNew<vtkMemoryResourceStream> memory;
  memory->SetBuffer(str.data(), str.size());

  return testStream(memory);
}

int TestResourceStreams(int argc, char* argv[])
{
  char* tempDir =
    vtkTestUtilities::GetArgOrEnvOrDefault("-T", argc, argv, "VTK_TEMP_DIR", "Testing/Temporary");
  if (!testFileResource(tempDir))
  {
    return 1;
  }
  delete[] tempDir;

  if (!testMemoryResource())
  {
    return 1;
  }

  return 0;
}
