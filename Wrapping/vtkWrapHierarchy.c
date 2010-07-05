/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWrapHierarchy.c

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright (c) 2010 David Gobbi.

  Contributed to the VisualizationToolkit by the author in June 2010
  under the terms of the Visualization Toolkit 2008 copyright.
-------------------------------------------------------------------------*/

/**
 The vtkWrapHierarchy program builds a text file that describes the
 class hierarchy.
 For each class, the output file will have a line in the following
 format:

 classname [ : superclass ] ; header.h
*/

#include "vtkParse.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#ifdef _WIN32
# include <windows.h>
#else
# include <unistd.h>
#endif

#define MAX_NUM_CLASSES 10000

/**
 * Read a header file with vtkParse.tab.c
 */
static int vtkWrapHierarchy_ParseHeaderFile(
  FILE *fp, const char *filename, char *lines[])
{
  char line[2048];
  char *cp;
  FileInfo *data;
  ClassInfo *class_info;
  size_t i, j, k;

  /* the "concrete" flag doesn't matter, just set to zero */
  data = vtkParse_ParseFile(filename, 0, fp, stderr);

  if (!data)
    {
    return 0;
    }

  /* find the last line in "lines" */
  k = 0;
  while (lines[k] != NULL)
    {
    k++;
    }

  cp = line;

  /* add a line for each class that is found */
  for (i = 0; i < data->Contents->NumberOfClasses; i++)
    {
    class_info = data->Contents->Classes[i];

    sprintf(cp, "%s ", class_info->Name);
    cp += strlen(cp);
    if (class_info->NumberOfSuperClasses)
      {
      sprintf(cp, ": ");
      cp += strlen(cp);
      }

    for (j = 0; j < class_info->NumberOfSuperClasses; j++)
      {
      sprintf(cp, "%s ", class_info->SuperClasses[j]);
      cp += strlen(cp);
      if (j+1 < class_info->NumberOfSuperClasses)
        {
        sprintf(cp, ", ");
        cp += strlen(cp);
        }
      }

    j = strlen(data->FileName) - 1;
    while (j > 0 && data->FileName[j-1] != '/' && data->FileName[j-1] != '\\')
      {
      j--;
      }

    sprintf(cp, "; %s", &data->FileName[j]);
    cp = line;
    lines[k] = (char *)malloc(strlen(cp)+1);
    strcpy(lines[k++], cp);
    lines[k] = NULL;
    }

  vtkParse_Free(data);

  return 1;
}

/**
 * Read a hierarchy file into "lines" without duplicating lines
 */
static int vtkWrapHierarchy_ReadHierarchyFile(FILE *fp, char *lines[])
{
  char line[2048];
  size_t i, n;

  while (fgets(line, 2048, fp))
    {
    n = strlen(line);
    while (n > 0 && isspace(line[n-1]))
      {
      n--;
      }
    line[n] = '\0';

    if (line[0] == '\0')
      {
      continue;
      }

    for (i = 0; lines[i] != NULL; i++)
      {
      if (strcmp(line, lines[i]) == 0)
        {
        break;
        }
      }

    if (lines[i] == NULL)
      {
      if (i+1 >= MAX_NUM_CLASSES)
        {
        fclose(fp);
        fprintf(stderr, "In vtkWrapHierarchy: "
                "Number of classes exceeds MAX_NUM_CLASSES\n");
        exit(1);
        }
      lines[i] = (char *)malloc(n+1);
      strcpy(lines[i], line);
      lines[i+1] = NULL;
      }
    }

  if (!feof(fp))
    {
    return 0;
    }

  return 1;
}

/**
 * Compare a file to "lines", return 0 if they are different
 */
static int vtkWrapHierarchy_CompareHierachyFile(FILE *fp, char *lines[])
{
  char line[2048];
  unsigned char *matched;
  size_t i, n;

  for (i = 0; lines[i] != NULL; i++) { ; };
  matched = (unsigned char *)malloc(i);
  memset(matched, 0, i);

  while (fgets(line, 2048, fp))
    {
    n = strlen(line);
    while (n > 0 && isspace(line[n-1]))
      {
      n--;
      }
    line[n] = '\0';

    if (line[0] == '\0')
      {
      continue;
      }

    for (i = 0; lines[i] != NULL; i++)
      {
      if (strcmp(line, lines[i]) == 0)
        {
        break;
        }
      }

    if (lines[i] == NULL)
      {
      free(matched);
      return 0;
      }

    matched[i] = 1;
    }

  for (i = 0; lines[i] != NULL; i++)
    {
    if (matched[i] == 0)
      {
      free(matched);
      return 0;
      }
    }

  free(matched);

  if (!feof(fp))
    {
    return 0;
    }

  return 1;
}

/**
 * Write "lines" to a hierarchy file
 */
static int vtkWrapHierarchy_WriteHierarchyFile(FILE *fp, char *lines[])
{
  size_t i;

  for (i = 0; lines[i] != NULL; i++)
    {
    if (fprintf(fp, "%s\n", lines[i]) < 0)
      {
      return 0;
      }
    }

  return 1;
}

/**
 * Try to parse a header file, print error and exit if fail
 */
static int vtkWrapHierarchy_TryParseHeaderFile(
  const char *file_name, char *lines[])
{
  FILE *input_file;

  input_file = fopen(file_name, "r");

  if (!input_file)
    {
    fprintf(stderr, "vtkWrapHierarchy: couldn't open file %s\n",
            file_name);
    exit(1);
    }

  if (!vtkWrapHierarchy_ParseHeaderFile(input_file, file_name, lines))
    {
    fclose(input_file);
    exit(1);
    }
  fclose(input_file);

  return 0;
}

/**
 * Try to read a file, print error and exit if fail
 */
static int vtkWrapHierarchy_TryReadHierarchyFile(
  const char *file_name, char *lines[])
{
  FILE *input_file;

  input_file = fopen(file_name, "r");
  if (!input_file)
    {
    fprintf(stderr, "vtkWrapHierarchy: couldn't open file %s\n",
            file_name);
    exit(1);
    }

  if (!vtkWrapHierarchy_ReadHierarchyFile(input_file, lines))
    {
    fclose(input_file);
    fprintf(stderr, "vtkWrapHierarchy: error reading file %s\n",
            file_name);
    exit(1);
    }
  fclose(input_file);

  return 0;
}

/**
 * Try to write a file, print error and exit if fail
 */
static int vtkWrapHierarchy_TryWriteHierarchyFile(
  const char *file_name, char *lines[])
{
  FILE *output_file;
  int matched = 0;

  output_file = fopen(file_name, "r");
  if (output_file && vtkWrapHierarchy_CompareHierachyFile(output_file, lines))
    {
    matched = 1;
    }
  if (output_file)
    {
    fclose(output_file);
    }

  if (!matched)
    {
    int tries = 1;
    output_file = fopen(file_name, "w");
    while (!output_file && tries < 5)
      {
      /* There are two CMAKE_CUSTOM_COMMANDS for vtkWrapHierarchy,
       * make sure they do not collide. */
      tries++;
#ifdef _WIN32
      Sleep(1000);
#else
      sleep(1);
#endif
      output_file = fopen(file_name, "w");
      if (output_file)
        {
        /* pretend that we wrote it, even though it was the other
         * process that did */
        fclose(output_file);
        return 1;
        }
      }
    if (!output_file)
      {
      fprintf(stderr, "vtkWrapHierarchy: tried %i times to write %s\n",
              tries, file_name);
      exit(1);
      }
    if (!vtkWrapHierarchy_WriteHierarchyFile(output_file, lines))
      {
      fclose(output_file);
      fprintf(stderr, "vtkWrapHierarchy: error writing file %s\n",
              file_name);
      exit(1);
      }
    fclose(output_file);
    }

  return 0;
}

#define MAX_INCLUDE_DIRS 255

int main(int argc, char *argv[])
{
  int usage_error = 0;
  char *output_filename = 0;
  int i, argi;
  char **lines;
  char **files;

  /* parse command-line options */
  for (argi = 1; argi < argc && argv[argi][0] == '-'; argi++)
    {
    if (strcmp(argv[argi], "-o") == 0)
      {
      argi++;
      if (argi >= argc || argv[argi][0] == '-')
        {
        usage_error = 1;
        break;
        }
      output_filename = argv[argi];
      }
    }

  if (usage_error || !output_filename || argc - argi < 1)
    {
    fprintf(stderr,
            "Usage: %s -o output_file data_file [files_to_merge]\n",
            argv[0]);
    exit(1);
    }

  lines = (char **)malloc(sizeof(char **)*MAX_NUM_CLASSES);
  lines[0] = NULL;
  files = (char **)malloc(sizeof(char **)*MAX_NUM_CLASSES);
  files[0] = NULL;

  /* read the data file */
  vtkWrapHierarchy_TryReadHierarchyFile(argv[argi++], files);

  /* read in all the prior files */
  while (argi < argc)
    {
    vtkWrapHierarchy_TryReadHierarchyFile(argv[argi++], lines);
    }

  /* merge the files listed in the data file */
  for (i = 0; files[i] != NULL; i++)
    {
    vtkWrapHierarchy_TryParseHeaderFile(files[i], lines);
    }

  /* write the file, if it has changed */
  vtkWrapHierarchy_TryWriteHierarchyFile(output_filename, lines);

  free(files);
  free(lines);
  return 0;
}
