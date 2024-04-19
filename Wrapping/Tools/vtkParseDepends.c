// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Kitware, Inc.
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkParseDepends.h"
#include "vtkParseData.h"
#include "vtkParseString.h"
#include "vtkParseSystem.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct ParseDepends_
{
  StringCache Strings;
  const char* Target;
  const char** Dependencies;
  int NumberOfDependencies;
} ParseDepends;

// dependency tracking is done globally
static ParseDepends DepTracker;

void vtkParse_InitDependencyTracking(const char* target)
{
  if (!target)
  {
    return;
  }

  vtkParse_InitStringCache(&DepTracker.Strings);
  DepTracker.Target = vtkParse_CacheString(&DepTracker.Strings, target, strlen(target));
  DepTracker.Dependencies = NULL;
  DepTracker.NumberOfDependencies = 0;
}

void vtkParse_AddDependency(const char* dep)
{
  if (!DepTracker.Target)
  {
    return;
  }

  // in C++, an unordered set could be used
  vtkParse_AddStringToArray(&DepTracker.Dependencies, &DepTracker.NumberOfDependencies,
    vtkParse_CacheString(&DepTracker.Strings, dep, strlen(dep)));
}

static void write_path(FILE* fout, const char* path)
{
  const char* c;

  for (c = path; *c != '\0'; ++c)
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
  }
}

static void write_line(FILE* fout, const char* target, const char* dep)
{
  // format: "target: dependency\n" (escape ':' and ' ' in filenames)
  write_path(fout, target);
  fprintf(fout, ": ");
  write_path(fout, dep);
  fputc('\n', fout);
}

// helper for qsort
static int string_compare(const void* a, const void* b)
{
  return strcmp(*((const char**)a), *((const char**)b));
}

int vtkParse_WriteDependencyFile(const char* fname)
{
  FILE* fout = NULL;
  const char* prev_dep = NULL;
  const char* dep;
  int i;

  if (!DepTracker.Target)
  {
    return 0;
  }

  fout = vtkParse_FileOpen(fname, "w+");
  if (!fout)
  {
    fprintf(stderr, "Error opening dependency file %s\n", fname);
    return 1;
  }

  // sort the data so that we can easily identify duplicates
  if (DepTracker.NumberOfDependencies > 1)
  {
    qsort(DepTracker.Dependencies, DepTracker.NumberOfDependencies, sizeof(char*), string_compare);
  }

  for (i = 0; i < DepTracker.NumberOfDependencies; ++i)
  {
    // only write if not a duplicate of the previous value
    dep = DepTracker.Dependencies[i];
    if (prev_dep == NULL || strcmp(dep, prev_dep) != 0)
    {
      write_line(fout, DepTracker.Target, dep);
      prev_dep = dep;
    }
  }

  if (DepTracker.NumberOfDependencies == 0)
  {
    write_line(fout, DepTracker.Target, "");
  }

  fclose(fout);

  return 0;
}

void vtkParse_FinalizeDependencyTracking(void)
{
  if (!DepTracker.Target)
  {
    return;
  }

  free(DepTracker.Dependencies);
  DepTracker.Dependencies = NULL;
  DepTracker.NumberOfDependencies = 0;
  DepTracker.Target = NULL;
  vtkParse_FreeStringCache(&DepTracker.Strings);
}
