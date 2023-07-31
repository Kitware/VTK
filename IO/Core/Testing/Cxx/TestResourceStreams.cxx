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

#define Check(expr, message)                                                                       \
  do                                                                                               \
  {                                                                                                \
    if (!(expr))                                                                                   \
    {                                                                                              \
      vtkErrorWithObjectMacro(nullptr, "Test failed: \n" << message);                              \
      return false;                                                                                \
    }                                                                                              \
  } while (false)

namespace
{

bool TestStream(vtkResourceStream* stream)
{
  Check(!stream->EndOfStream(), "Invalid stream");

  std::array<char, 5> buffer;

  Check(stream->Read(buffer.data(), buffer.size()) == 5, "Read wrong size");
  Check(!stream->EndOfStream(), "Reach end of file too early");
  Check(std::strncmp(buffer.data(), "Hello", 5) == 0, "Read wrong data");

  if (stream->SupportSeek())
  {
    Check(stream->Tell() == 5, "Tell wrong position");

    Check(stream->Seek(2, vtkResourceStream::SeekDirection::Current) == 7, "Seek wrong position");
    Check(!stream->EndOfStream(), "Seek must not modify EndOfStream value");
    Check(stream->Tell() == 7, "Tell wrong position");

    Check(stream->Seek(10, vtkResourceStream::SeekDirection::Current) == 17, "Seek wrong position");
    Check(!stream->EndOfStream(), "Seek must not modify EndOfStream value");
    Check(stream->Read(nullptr, 0) == 0, "Read wrong size");
    Check(!stream->EndOfStream(), "Last zero byte read must not set EOS");

    Check(stream->Seek(10, vtkResourceStream::SeekDirection::Current) == 27, "Seek wrong position");
    Check(!stream->EndOfStream(), "EndOfStream must be false after Seek");
    Check(stream->Read(buffer.data(), 0) == 0, "Read wrong size");
    Check(!stream->EndOfStream(), "Last zero byte read must not set EOS");

    Check(stream->Seek(10, vtkResourceStream::SeekDirection::Current) == 37, "Seek wrong position");
    Check(!stream->EndOfStream(), "EndOfStream must be false after Seek");
    Check(stream->Read(buffer.data(), buffer.size()) == 0, "Read wrong size");
    Check(stream->EndOfStream(), "Last read must lead the stream to EOS");

    Check(stream->Seek(2, vtkResourceStream::SeekDirection::Begin) == 2, "Seek wrong position");
    Check(!stream->EndOfStream(), "EndOfStream must be false after Seek");
    Check(stream->Seek(-6, vtkResourceStream::SeekDirection::End) == 6, "Seek wrong position");
    Check(!stream->EndOfStream(), "EndOfStream must be false after Seek");
  }
  else
  {
    Check(!stream->EndOfStream(), "Reach end of file too early");
  }

  Check(stream->Read(buffer.data(), buffer.size()) == 5, "Read wrong size");
  Check(!stream->EndOfStream(), "Reach end of file too early");
  Check(std::strncmp(buffer.data(), "world", 5) == 0, "Read wrong data");

  Check(stream->Read(buffer.data(), buffer.size()) == 1, "Read wrong size");
  Check(std::strncmp(buffer.data(), "!", 1) == 0, "Read wrong data");
  Check(stream->EndOfStream(), "Last read must lead the stream to EOS");

  return true;
}

bool TestFileResource(const std::string& temp_dir)
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

  return TestStream(file);
}

bool TestMemoryResource()
{
  const std::string str{ "Hello world!" };
  vtkNew<vtkMemoryResourceStream> memory;
  memory->SetBuffer(str.data(), str.size());

  return TestStream(memory);
}

bool TestOwnedMemoryResource()
{
  const std::string str{ "Hello world!" };
  vtkNew<vtkMemoryResourceStream> memory;
  memory->SetBuffer(str.data(), str.size(), true); // copied

  Check(TestStream(memory), "Basic checks failed");
  Check(memory->OwnsBuffer(), "OwnsBuffer must return true");
  memory->SetBuffer(nullptr, 0);
  Check(!memory->OwnsBuffer(), "OwnsBuffer must return false");
  Check(memory->EndOfStream(), "EndOfStream must return true");

  memory->SetBuffer(str.data(), str.size(), true); // copied
  Check(memory->OwnsBuffer(), "OwnsBuffer must return true");
  memory->SetBuffer(nullptr, 0, true); // must have same effect
  Check(!memory->OwnsBuffer(), "OwnsBuffer must return false");
  Check(memory->EndOfStream(), "EndOfStream must return true");

  std::vector<char> vec{ 'H', 'e', 'l', 'l', 'o', ' ', 'w', 'o', 'r', 'l', 'd', '!' };
  memory->SetBuffer(vec);
  Check(TestStream(memory), "Basic checks failed");
  Check(memory->OwnsBuffer(), "OwnsBuffer must return true");
  memory->SetBuffer(std::move(vec));
  Check(TestStream(memory), "Basic checks failed");
  Check(memory->OwnsBuffer(), "OwnsBuffer must return true");

  std::string tmpstr{ "Hello world!" };
  memory->SetBuffer(tmpstr);
  Check(TestStream(memory), "Basic checks failed");
  Check(memory->OwnsBuffer(), "OwnsBuffer must return true");
  memory->SetBuffer(std::move(tmpstr));
  Check(TestStream(memory), "Basic checks failed");
  Check(memory->OwnsBuffer(), "OwnsBuffer must return true");

  return true;
}

}

int TestResourceStreams(int argc, char* argv[])
{
  char* tempDir =
    vtkTestUtilities::GetArgOrEnvOrDefault("-T", argc, argv, "VTK_TEMP_DIR", "Testing/Temporary");
  if (!TestFileResource(tempDir))
  {
    return 1;
  }
  delete[] tempDir;

  if (!TestMemoryResource())
  {
    return 1;
  }

  if (!TestOwnedMemoryResource())
  {
    return 1;
  }

  return 0;
}
