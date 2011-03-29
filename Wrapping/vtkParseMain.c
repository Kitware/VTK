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
#include "vtkConfigure.h" // VTK_VERSION
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

/* This is the struct that contains the options */
OptionInfo options;

/* Flags for --help and --version */
int vtk_parse_help = 0;
int vtk_parse_version = 0;

/* This method provides the back-end for the generators */
extern void vtkParseOutput(FILE *, FileInfo *);

/* Check the options */
static int check_options(int argc, char *argv[])
{
  int i;
  size_t j;

  options.InputFileName = NULL;
  options.OutputFileName = NULL;
  options.IsAbstract = 0;
  options.IsConcrete = 0;
  options.IsVTKObject = 0;
  options.IsSpecialObject = 0;
  options.HierarchyFileName = 0;
  options.HintFileName = 0;

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
    else if (strcmp(argv[i], "--types") == 0)
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
      vtkParse_IncludeDirectory(argv[i]);
      }
    else if (strcmp(argv[i], "-D") == 0)
      {
      i++;
      j = 0;
      if (i >= argc || argv[i][0] == '-')
        {
        return -1;
        }
      while (argv[i][j] != '\0' && argv[i][j] != '=') { j++; }
      if (argv[i][j] == '=') { j++; }
      vtkParse_DefineMacro(argv[i], &argv[i][j]);
      }
    else if (strcmp(argv[i], "-U") == 0)
      {
      i++;
      if (i >= argc || argv[i][0] == '-')
        {
        return -1;
        }
      vtkParse_UndefineMacro(argv[i]);
      }
    else if (strcmp(argv[i], "--help") == 0)
      {
      vtk_parse_help = 1;
      }
    else if (strcmp(argv[i], "--version") == 0)
      {
      vtk_parse_version = 1;
      }
    }

  return i;
}

/* Return a pointer to the static OptionInfo struct */
OptionInfo *vtkParse_GetCommandLineOptions()
{
  return &options;
}

static void vtk_parse_print_help(FILE *stream, const char *cmd)
{
  fprintf(stream,
    "Usage: %s [options] input_file output_file\n"
    "  --help          print this help message\n"
    "  --version       print the VTK version\n"
    "  --concrete      force concrete class\n"
    "  --abstract      force abstract class\n"
    "  --vtkobject     vtkObjectBase-derived class\n"
    "  --special       non-vtkObjectBase class\n"
    "  --hints <file>  the hints file to use\n"
    "  --types <file>  the type hierarchy file to use\n"
    "  -I <dir>        add an include directory\n"
    "  -D <macro>      define a preprocessor macro\n"
    "  -U <macro>      undefine a preprocessor macro\n",
    cmd);
}

int main(int argc, char *argv[])
{
  int argi;
  int has_options = 0;
  FILE *ifile;
  FILE *ofile;
  FILE *hfile = 0;
  const char *cp;
  char *classname;
  size_t i;
  FileInfo *data;

  argi = check_options(argc, argv);
  if (argi > 1 && argc - argi == 2)
    {
    has_options = 1;
    }
  else if (argi < 0 || argc > 5 ||
           (argc < 3 && !vtk_parse_help && !vtk_parse_version))
    {
    vtk_parse_print_help(stderr, argv[0]);
    exit(1);
    }

  if (vtk_parse_version)
    {
    const char *ver = VTK_VERSION;
    const char *exename = argv[0];
    /* remove directory part of exe name */
    for (exename += strlen(exename); exename > argv[0]; --exename)
      {
      char pc = *(exename - 1);
      if (pc == ':' || pc == '/' || pc == '\\')
        {
        break;
        }
      }
    fprintf(stdout, "%s %s\n", exename, ver);
    exit(0);
    }
  if (vtk_parse_help)
    {
    vtk_parse_print_help(stdout, argv[0]);
    exit(0);
    }

  options.InputFileName = argv[argi++];

  ifile = fopen(options.InputFileName, "r");
  if (!ifile)
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
    hfile = fopen(options.HintFileName, "r");
    if (!hfile)
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

  if (options.IsConcrete)
    {
    cp = options.InputFileName;
    i = strlen(cp);
    classname = (char *)malloc(i+1);
    while (i > 0 &&
           cp[i-1] != '/' && cp[i-1] != '\\' && cp[i-1] != ':') { i--; }
    strcpy(classname, &cp[i]);
    i = 0;
    while (classname[i] != '\0' && classname[i] != '.') { i++; }
    classname[i] = '\0';

    vtkParse_SetClassProperty(classname, "concrete");
    }

  vtkParse_SetIgnoreBTX(0);
  if (options.HierarchyFileName)
    {
    vtkParse_SetIgnoreBTX(1);
    }

  data = vtkParse_ParseFile(options.InputFileName, ifile, stderr);

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
