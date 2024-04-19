// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// .NAME Test of vtkLZMADataCompressor
// .SECTION Description
//

#include "vtkLZMADataCompressor.h"
#include "vtkObjectFactory.h"

int TestCompressLZMA(int argc, char* argv[])
{
  int res = 1;
  const unsigned int start_size = 100024;
  unsigned int cc;
  unsigned char buffer[start_size];
  unsigned char* cbuffer;
  unsigned char* ucbuffer;
  size_t nlen;
  size_t rlen;

  vtkLZMADataCompressor* compressor = vtkLZMADataCompressor::New();
  for (cc = 0; cc < start_size; cc++)
  {
    buffer[cc] = static_cast<unsigned char>(cc % sizeof(unsigned char));
  }
  buffer[0] = 'v';
  buffer[1] = 't';
  buffer[2] = 'k';

  nlen = compressor->GetMaximumCompressionSpace(start_size);
  cbuffer = new unsigned char[nlen];
  rlen = compressor->Compress(buffer, start_size, cbuffer, nlen);
  if (rlen > 0)
  {
    ucbuffer = new unsigned char[start_size];
    rlen = compressor->Uncompress(cbuffer, rlen, ucbuffer, start_size);
    if (rlen == start_size)
    {
      cout << argv[0] << " Works " << argc << endl;
      cout << ucbuffer[0] << ucbuffer[1] << ucbuffer[2] << endl;
      res = 0;
    }
    delete[] ucbuffer;
  }
  delete[] cbuffer;

  compressor->Delete();
  return res;
}
