// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#define _LARGEFILE_SOURCE
#define _LARGE_FILES
#define _FILE_OFFSET_BITS 64
#include <cassert>
#include <cstdio>
#include <sys/types.h>

// include sys/stat.h after sys/types.h to ensure correct off_t definition
#include <sys/stat.h>

int main(int, char** argv)
{
  // check that off_t can hold 2^63 - 1 and perform basic operations...
#define OFF_T_64 (((off_t)1 << 62) - 1 + ((off_t)1 << 62))
  if (OFF_T_64 % 2147483647 != 1)
    return 1;

  // stat breaks on SCO OpenServer
  struct stat buf;
  stat(argv[0], &buf);
  if (!S_ISREG(buf.st_mode))
    return 2;

  FILE* file = fopen(argv[0], "r");
  off_t offset = ftello(file);
  fseek(file, offset, SEEK_CUR);
  fclose(file);
  return 0;
}
