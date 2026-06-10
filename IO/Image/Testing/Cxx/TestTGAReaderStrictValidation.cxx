// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkMemoryResourceStream.h"
#include "vtkNew.h"
#include "vtkTGAReader.h"

#include <iostream>
#include <vector>

namespace
{
int CanRead(const std::vector<unsigned char>& bytes)
{
  vtkNew<vtkMemoryResourceStream> stream;
  stream->SetBuffer(bytes.data(), bytes.size());
  vtkNew<vtkTGAReader> reader;
  return reader->CanReadFile(stream);
}
} // namespace

int TestTGAReaderStrictValidation(int argc, char* argv[])
{
  // 1x1 24-bpp uncompressed TGA 1.0 with minimal pixel payload
  std::vector<unsigned char> valid = { 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x18, 0x00, 0x00, 0x00, 0x00 };
  if (!::CanRead(valid))
  {
    std::cerr << "FAIL: valid TGA 1.0 header rejected\n";
    return EXIT_FAILURE;
  }

  std::vector<unsigned char> badColorMap = { 0x00, 0x01, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x18, 0x00, 0x00, 0x00, 0x00 };
  if (::CanRead(badColorMap))
  {
    std::cerr << "FAIL: non-zero colorMapType accepted\n";
    return EXIT_FAILURE;
  }

  std::vector<unsigned char> zeroWidth = { 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x18, 0x00, 0x00, 0x00, 0x00 };
  if (::CanRead(zeroWidth))
  {
    std::cerr << "FAIL: zero width accepted\n";
    return EXIT_FAILURE;
  }

  std::vector<unsigned char> zeroHeight = { 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00, 0x00 };
  if (::CanRead(zeroHeight))
  {
    std::cerr << "FAIL: zero height accepted\n";
    return EXIT_FAILURE;
  }

  std::vector<unsigned char> badDescriptor = { 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x18, 0x80, 0x00, 0x00, 0x00 };
  if (::CanRead(badDescriptor))
  {
    std::cerr << "FAIL: reserved descriptor bits accepted\n";
    return EXIT_FAILURE;
  }

  // 2x2 image needs 12 payload bytes; only 3 supplied
  std::vector<unsigned char> tooSmall = { 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x02, 0x00, 0x02, 0x00, 0x18, 0x00, 0x00, 0x00, 0x00 };
  if (::CanRead(tooSmall))
  {
    std::cerr << "FAIL: undersized payload accepted\n";
    return EXIT_FAILURE;
  }

  // byte[2]=LF(10) and byte[16]=space(32) satisfy the old two-byte check
  // byte[1]='0' (non-zero) triggers the colorMapType rejection
  std::vector<unsigned char> ptsLike(100, 0x20);
  ptsLike[0] = '1';
  ptsLike[1] = '0';
  ptsLike[2] = 0x0A;
  if (::CanRead(ptsLike))
  {
    std::cerr << "FAIL: ASCII PTS-like content accepted as TGA\n";
    return EXIT_FAILURE;
  }

  if (argc > 1)
  {
    vtkNew<vtkTGAReader> reader;
    if (!reader->CanReadFile(argv[1]))
    {
      std::cerr << "FAIL: valid TGA file rejected: " << argv[1] << "\n";
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}
