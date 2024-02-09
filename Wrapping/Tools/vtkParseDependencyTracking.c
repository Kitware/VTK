// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Kitware, Inc.
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkParseDependencyTracking.h"
#include "vtkParseSystem.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct DependencyTracking_
{
  char* Target;
  char** Dependencies;
  size_t NumberOfDependencies;
  size_t DependenciesCapacity;
} DependencyTracking;
DependencyTracking DepTracker;

void vtkParse_InitDependencyTracking(const char* target)
{
  if (!target)
  {
    return;
  }

  DepTracker.Target = strdup(target);
  DepTracker.Dependencies = NULL;
  DepTracker.NumberOfDependencies = 0;
  DepTracker.DependenciesCapacity = 0;
}

void vtkParse_AddFileDependency(const char* dep)
{
  size_t newSize = DepTracker.NumberOfDependencies + 1;

  if (!DepTracker.Target)
  {
    return;
  }

  if (newSize + 1 >= DepTracker.DependenciesCapacity)
  {
    char** newDeps = NULL;
    /* Allocate 2 for the first call. */
    size_t newCapacity =
      DepTracker.DependenciesCapacity ? (DepTracker.DependenciesCapacity << 1) : 2;
    size_t i;

    newDeps = (char**)realloc(DepTracker.Dependencies, newCapacity * sizeof(char*));
    if (!newDeps)
    {
      fprintf(stderr, "error: out of memory (DepTracker.Dependencies @ %zu)", newCapacity);
      return;
    }

    /* Initialize new memory. */
    for (i = DepTracker.NumberOfDependencies; i < newCapacity; ++i)
    {
      newDeps[i] = NULL;
    }
    DepTracker.Dependencies = newDeps;
    DepTracker.DependenciesCapacity = newCapacity;
  }

  DepTracker.Dependencies[DepTracker.NumberOfDependencies] = strdup(dep);
  if (!DepTracker.Dependencies[DepTracker.NumberOfDependencies])
  {
    fprintf(stderr, "error: out of memory (DepTracker.Dependencies[%zu] = %s)",
      DepTracker.NumberOfDependencies, dep);
    return;
  }

  ++DepTracker.NumberOfDependencies;
}

static void write_path(FILE* fout, const char* path)
{
  const char* c;

  c = path;
  while (*c)
  {
    if (*c == '\\')
    {
      fprintf(fout, "\\\\");
    }
    else if (*c == '$')
    {
      fprintf(fout, "\\$");
    }
    else if (*c == '#')
    {
      fprintf(fout, "\\#");
    }
    else if (*c == ' ')
    {
      fprintf(fout, "\\ ");
    }
    else
    {
      fputc(*c, fout);
    }
    ++c;
  }
}

static void write_line(FILE* fout, const char* target, const char* input)
{
  write_path(fout, target);
  fprintf(fout, ": ");
  write_path(fout, input);
  fputc('\n', fout);
}

int vtkParse_DependencyTrackingWrite(const char* fname)
{
  FILE* fout = NULL;
  char** input;

  if (!DepTracker.Target)
  {
    return 0;
  }

  fout = vtkParse_FileOpen(fname, "w+");
  if (!fout)
  {
    return 1;
  }

  input = DepTracker.Dependencies;
  if (input)
  {
    while (*input)
    {
      write_line(fout, DepTracker.Target, *input);
      ++input;
    }
  }
  else
  {
    write_line(fout, DepTracker.Target, "");
  }

  fclose(fout);

  return 0;
}

void vtkParse_FinalizeDependencyTracking(void)
{
  char** input;

  if (!DepTracker.Target)
  {
    return;
  }

  input = DepTracker.Dependencies;
  if (input)
  {
    while (*input)
    {
      free(*input);
      ++input;
    }
  }

  free(DepTracker.Dependencies);
  DepTracker.Dependencies = NULL;
  DepTracker.NumberOfDependencies = 0;
  DepTracker.DependenciesCapacity = 0;
  free(DepTracker.Target);
  DepTracker.Target = NULL;
}
