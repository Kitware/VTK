/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkParseMain.c

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/*

This file provides a unified front-end for the wrapper generators.
It contains the main() function and argument parsing, and calls
the code that parses the header file.

*/

#include "vtkParse.h"
#include "vtkParseMain.h"
#include "vtkParseInternal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

/* This is the struct that contains the options */
OptionInfo options;

/* This method provides the back-end for the generators */
extern void vtkParseOutput(FILE *, FileInfo *);

/* Check the options */
static int check_options(int argc, char *argv[])
{
  int i;

  options.InputFileName = NULL;
  options.OutputFileName = NULL;
  options.IsAbstract = 0;
  options.IsConcrete = 0;
  options.IsVTKObject = 0;
  options.IsSpecialObject = 0;
  options.HierarchyFileName = 0;
  options.HintFileName = 0;
  options.NumberOfIncludeDirectories = 0;

  for (i = 1; i < argc && argv[i][0] == '-'; i++)
    {
    if (strcmp(argv[i], "--concrete") == 0)
      {
      options.IsConcrete = 1;
      }
    else if (strcmp(argv[i], "--abstract") == 0)
      {
      options.IsAbstract = 1;
      }
    else if (strcmp(argv[i], "--vtkobject") == 0)
      {
      options.IsVTKObject = 1;
      }
    else if (strcmp(argv[i], "--special") == 0)
      {
      options.IsSpecialObject = 1;
      }
    else if (strcmp(argv[i], "--hints") == 0)
      {
      i++;
      if (i >= argc || argv[i][0] == '-')
        {
        return -1;
        }
      options.HintFileName = argv[i];
      }
    else if (strcmp(argv[i], "--hierarchy") == 0)
      {
      i++;
      if (i >= argc || argv[i][0] == '-')
        {
        return -1;
        }
      options.HierarchyFileName = argv[i];
      }
    else if (strcmp(argv[i], "-I") == 0)
      {
      i++;
      if (i >= argc || argv[i][0] == '-')
        {
        return -1;
        }
      vtkParse_AddPointerToArray(&options.IncludeDirectories,
                                 &options.NumberOfIncludeDirectories,
                                 argv[i]);
      }
    }

  return i;
}

/* Return a pointer to the static OptionInfo struct */
OptionInfo *vtkParse_GetCommandLineOptions()
{
  return &options;
}

/* Find a filename, given the "-I" options from the command line */
char *vtkParse_FindPath(const char *filename)
{
  int i;
  struct stat fs;
  char filepath[512];
  const char *directory;
  char *output;
  const char *sep;

  for (i = -1; i < options.NumberOfIncludeDirectories; i++)
    {
    if (i < 0)
      {
      /* try first with no path */
      strcpy(filepath, filename);
      }
    else
      {
      directory = options.IncludeDirectories[i];
      sep = "/";
      if (directory[strlen(directory)-1] == sep[0])
        {
        sep = "";
        }
      sprintf(filepath, "%s%s%s", directory, sep, filename);
      }

    if (stat(filepath, &fs) == 0)
      {
      output = (char *)malloc(strlen(filepath)+1);
      strcpy(output, filepath);
      return output;
      }
    }

  return NULL;
}

/* Free a path returned by FindPath */
void vtkParse_FreePath(char *filepath)
{
  if (filepath)
    {
    free(filepath);
    }
}

int main(int argc, char *argv[])
{
  int argi;
  int has_options = 0;
  FILE *ifile;
  FILE *ofile;
  FILE *hfile = 0;
  FileInfo *data;

  argi = check_options(argc, argv);
  if (argi > 1 && argc - argi == 2)
    {
    has_options = 1;
    }
  else if (argi < 0 || argc < 3 || argc > 5)
    {
    fprintf(stderr,
            "Usage: %s [options] input_file output_file\n"
            "  --concrete      force concrete class\n"
            "  --abstract      force abstract class\n"
            "  --vtkobject     vtkObjectBase-derived class\n"
            "  --special       non-vtkObjectBase class\n"
            "  --hints <file>  hints file\n"
            "  --hierarchy <file>  hierarchy file\n"
            "  -I <dir>        add an include directory\n",
            argv[0]);
    exit(1);
    }

  options.InputFileName = argv[argi++];

  if (!(ifile = fopen(options.InputFileName, "r")))
    {
    fprintf(stderr,"Error opening input file %s\n", options.InputFileName);
    exit(1);
    }

  if (!has_options)
    {
    if (argc == 5)
      {
      options.HintFileName = argv[argi++];
      }
    if (argc >= 4)
      {
      options.IsConcrete = atoi(argv[argi++]);
      options.IsAbstract = !options.IsConcrete;
      }
    }

  if (options.HintFileName && options.HintFileName[0] != '\0')
    {
    if (!(hfile = fopen(options.HintFileName, "r")))
      {
      fprintf(stderr, "Error opening hint file %s\n", options.HintFileName);
      fclose(ifile);
      exit(1);
      }
    }

  options.OutputFileName = argv[argi++];
  ofile = fopen(options.OutputFileName, "w");

  if (!ofile)
    {
    fprintf(stderr, "Error opening output file %s\n", options.OutputFileName);
    fclose(ifile);
    if (hfile)
      {
      fclose(hfile);
      }
    exit(1);
    }

  data = vtkParse_ParseFile(
    options.InputFileName, options.IsConcrete, ifile, stderr);

  if (!data)
    {
    fclose(ifile);
    fclose(ofile);
    if (hfile)
      {
      fclose(hfile);
      }
    exit(1);
    }

  if (hfile)
    {
    vtkParse_ReadHints(data, hfile, stderr);
    }

  if (options.IsConcrete && data->MainClass)
    {
    data->MainClass->IsAbstract = 0;
    }
  else if (options.IsAbstract && data->MainClass)
    {
    data->MainClass->IsAbstract = 1;
    }

  vtkParseOutput(ofile, data);

  fclose(ofile);

  vtkParse_Free(data);

  return 0;
}
