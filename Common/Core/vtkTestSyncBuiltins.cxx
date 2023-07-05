// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include <cstdio>
#include <cstdlib>

int main()
{
  /* Test whether gcc/clang __atomic_ builtins are defined */

  static volatile unsigned long v = 0;
  unsigned long u = __atomic_add_fetch(&v, 1, __ATOMIC_SEQ_CST);

  return 0;
}
