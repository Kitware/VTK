// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Kitware, Inc.
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkParseDependencyTracking.h"
#include "vtkParseData.h"
#include "vtkParseString.h"
#include "vtkParseSystem.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct DependencyTracking_
{
  StringCache Strings;
  const char* Target;
  const char** Dependencies;
  int NumberOfDependencies;
} DependencyTracking;

// dependency tracking is done globally
DependencyTracking DepTracker;

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

void vtkParse_AddFileDependency(const char* dep)
{
  if (!DepTracker.Target)
  {
    return;
  }

  vtkParse_AddStringToArray(&DepTracker.Dependencies, &DepTracker.NumberOfDependencies,
    vtkParse_CacheString(&DepTracker.Strings, dep, strlen(dep)));
}

static void write_path(FILE* fout, const char* path)
{
  const char* c;

  for (c = path; *c != '\0'; ++c)
  {
    if (*c == ':')
    {
      fprintf(fout, "\\:");
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

int vtkParse_DependencyTrackingWrite(const char* fname)
{
  FILE* fout = NULL;
  int i;

  if (!DepTracker.Target)
  {
    return 0;
  }

  fout = vtkParse_FileOpen(fname, "w+");
  if (!fout)
  {
    return 1;
  }

  for (i = 0; i < DepTracker.NumberOfDependencies; ++i)
  {
    write_line(fout, DepTracker.Target, DepTracker.Dependencies[i]);
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
