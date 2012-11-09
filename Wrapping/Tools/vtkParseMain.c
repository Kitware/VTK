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

*/

#include "vtkParse.h"
#include "vtkParseData.h"
#include "vtkParseMain.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* This is the struct that contains the options */
OptionInfo options;

/* Get the base filename */
static const char *parse_exename(const char *cmd)
{
  const char *exename;

  /* remove directory part of exe name */
  for (exename = cmd + strlen(cmd); exename > cmd; --exename)
    {
    char pc = exename[-1];
    if (pc == ':' || pc == '/' || pc == '\\')
      {
      break;
      }
    }

  return exename;
}

/* Print the help */
static void parse_print_help(FILE *fp, const char *cmd, int multi)
{
  fprintf(fp,
    "Usage: %s [options] infile... \n"
    "  --help            print this help message\n"
    "  --version         print the VTK version\n"
    "  -o <file>         the output file\n"
    "  -I <dir>          add an include directory\n"
    "  -D <macro[=def]>  define a preprocessor macro\n"
    "  -U <macro>        undefine a preprocessor macro\n"
    "  @<file>           read arguments from a file\n",
    parse_exename(cmd));

  /* args for describing a singe header file input */
  if (!multi)
    {
    fprintf(fp,
    "  --hints <file>    the hints file to use\n"
    "  --types <file>    the type hierarchy file to use\n"
    "  --concrete        force concrete class (ignored, deprecated)\n"
    "  --abstract        force abstract class (ignored, deprecated)\n"
    "  --vtkobject       vtkObjectBase-derived class (ignored, deprecated)\n"
    "  --special         non-vtkObjectBase class (ignored, deprecated)\n");
    }
}

/* append an arg to the arglist */
static void parse_append_arg(int *argn, char ***args, char *arg)
{
  /* if argn is a power of two, allocate more space */
  if (*argn > 0 && (*argn & (*argn - 1)) == 0)
    {
    *args = (char **)realloc(*args, 2*(*argn)*sizeof(char *));
    }
  /* append argument to list */
  (*args)[*argn] = arg;
  (*argn)++;
}

/* read options from a file, return zero on error */
static int read_option_file(
  StringCache *strings, const char *filename, int *argn, char ***args)
{
  static int option_file_stack_max = 10;
  static int option_file_stack_size = 0;
  static const char *option_file_stack[10];
  FILE *fp;
  char *line;
  const char *ccp;
  char *argstring;
  char *arg;
  size_t maxlen = 15;
  size_t i, n;
  int j;
  int in_string;

  line = (char *)malloc(maxlen);

  fp = fopen(filename, "r");

  if (fp == NULL)
    {
    return 0;
    }

  /* read the file line by line */
  while (fgets(line, (int)maxlen, fp))
    {
    n = strlen(line);

    /* if buffer not long enough, increase it */
    while (n == maxlen-1 && line[n-1] != '\n' && !feof(fp))
      {
      maxlen *= 2;
      line = (char *)realloc(line, maxlen);
      if (!fgets(&line[n], (int)(maxlen-n), fp)) { break; }
      n += strlen(&line[n]);
      }

    /* allocate a string to hold the parsed arguments */
    argstring = vtkParse_NewString(strings, n);
    arg = argstring;
    i = 0;

    /* break the line into individual options */
    ccp = line;
    in_string = 0;
    while (*ccp != '\0')
      {
      for (;;)
        {
        if (*ccp == '\\')
          {
          ccp++;
          }
        else if (*ccp == '\"' || *ccp == '\'')
          {
          if (!in_string)
            {
            in_string = *ccp++;
            continue;
            }
          else if (*ccp == in_string)
            {
            in_string = 0;
            ccp++;
            continue;
            }
          }
        else if (!in_string && isspace(*ccp))
          {
          do { ccp++; } while (isspace(*ccp));
          break;
          }
        if (*ccp == '\0')
          {
          break;
          }
        /* append character to argument */
        arg[i++] = *ccp++;
        }
      arg[i++] = '\0';

      if (arg[0] == '@')
        {
        /* recursively expand '@file' option */
        if (option_file_stack_size == option_file_stack_max)
          {
          fprintf(stderr, "%s: @file recursion is too deep.\n",
                  (*args)[0]);
          exit(1);
          }
        /* avoid reading the same file recursively */
        option_file_stack[option_file_stack_size++] = filename;
        for (j = 0; j < option_file_stack_size; j++)
          {
          if (strcmp(&arg[1], option_file_stack[j]) == 0)
            {
            break;
            }
          }
        if (j < option_file_stack_size)
          {
          parse_append_arg(argn, args, arg);
          }
        else if (read_option_file(strings, &arg[1], argn, args) == 0)
          {
          parse_append_arg(argn, args, arg);
          }
        option_file_stack_size--;
        }
      else if (arg[0] != '\0')
        {
        parse_append_arg(argn, args, arg);
        }
      /* prepare for next arg */
      arg += i;
      i = 0;
      }
    }

  return 1;
}

/* expand any "@file" args that occur in the command-line args */
static void parse_expand_args(
  StringCache *strings, int argc, char *argv[], int *argn, char ***args)
{
  int i;

  *argn = 0;
  *args = (char **)malloc(sizeof(char *));

  for (i = 0; i < argc; i++)
    {
    /* check for "@file" unless this is the command name */
    if (i > 0 || argv[i][0] == '@')
      {
      /* if read_option_file returns null, add "@file" to the args */
      /* (this mimics the way that gcc expands @file arguments) */
      if (read_option_file(strings, &argv[i][1], argn, args) == 0)
        {
        parse_append_arg(argn, args, argv[i]);
        }
      }
    else
      {
      /* append any other arg */
      parse_append_arg(argn, args, argv[i]);
      }
    }
}

/* Check the options: "multi" should be zero for wrapper tools that
 * only take one input file, or one for wrapper tools that take multiple
 * input files.  Returns zero for "--version" or "--help", or returns -1
 * if an error occurred.  Otherwise, it returns the number of args
 * that were successfully parsed. */
static int parse_check_options(int argc, char *argv[], int multi)
{
  int i;
  size_t j;
  char *cp;
  char c;

  options.NumberOfFiles = 0;
  options.Files = NULL;
  options.InputFileName = NULL;
  options.OutputFileName = NULL;
  options.HierarchyFileName = 0;
  options.HintFileName = 0;

  for (i = 1; i < argc; i++)
    {
    if (strcmp(argv[i], "--help") == 0)
      {
      parse_print_help(stdout, argv[0], multi);
      return 0;
      }
    else if (strcmp(argv[i], "--version") == 0)
      {
      const char *ver = VTK_PARSE_VERSION;
      fprintf(stdout, "%s %s\n", parse_exename(argv[0]), ver);
      return 0;
      }
    else if (argv[i][0] != '-')
      {
      if (options.NumberOfFiles == 0)
        {
        options.Files = (char **)malloc(sizeof(char *));
        }
      else if ((options.NumberOfFiles & (options.NumberOfFiles - 1)) == 0)
        {
        options.Files = (char **)realloc(
          options.Files, 2*options.NumberOfFiles*sizeof(char *));
        }
      options.Files[options.NumberOfFiles++] = argv[i];
      }
    else if (argv[i][0] == '-' && isalpha(argv[i][1]))
      {
      c = argv[i][1];
      cp = &argv[i][2];
      if (*cp == '\0')
        {
        i++;
        if (i >= argc || argv[i][0] == '-')
          {
          return -1;
          }
        cp = argv[i];
        }

      if (c == 'o')
        {
        options.OutputFileName = cp;
        }
      else if (c == 'I')
        {
        vtkParse_IncludeDirectory(cp);
        }
      else if (c == 'D')
        {
        j = 0;
        while (cp[j] != '\0' && cp[j] != '=') { j++; }
        if (cp[j] == '=') { j++; }
        vtkParse_DefineMacro(cp, &cp[j]);
        }
      else if (c == 'U')
        {
        vtkParse_UndefineMacro(cp);
        }
      }
    else if (!multi && strcmp(argv[i], "--hints") == 0)
      {
      i++;
      if (i >= argc || argv[i][0] == '-')
        {
        return -1;
        }
      options.HintFileName = argv[i];
      }
    else if (!multi && strcmp(argv[i], "--types") == 0)
      {
      i++;
      if (i >= argc || argv[i][0] == '-')
        {
        return -1;
        }
      options.HierarchyFileName = argv[i];
      }
    else if (strcmp(argv[i], "--vtkobject") == 0 ||
             strcmp(argv[i], "--special") == 0 ||
             strcmp(argv[i], "--abstract") == 0 ||
             strcmp(argv[i], "--concrete") == 0)
      {
      fprintf(stderr, "Warning: the %s option is deprecated "
              "and will be ignored.\n", argv[i]);
      }
    }

  return i;
}

/* Return a pointer to the static OptionInfo struct */
OptionInfo *vtkParse_GetCommandLineOptions()
{
  return &options;
}

/* Command-line argument handler for wrapper tools */
FileInfo *vtkParse_Main(int argc, char *argv[])
{
  int argi;
  int expected_files;
  FILE *ifile;
  FILE *hfile = 0;
  FileInfo *data;
  StringCache strings;
  int argn;
  char **args;

  /* expand any "@file" args */
  vtkParse_InitStringCache(&strings);
  parse_expand_args(&strings, argc, argv, &argn, &args);

  /* read the args into the static OptionInfo struct */
  argi = parse_check_options(argn, args, 0);

  /* was output file already specified by the "-o" option? */
  expected_files = (options.OutputFileName == NULL ? 2 : 1);

  /* verify number of args, print usage if not valid */
  if (argi == 0)
    {
    free(args);
    exit(0);
    }
  else if (argi < 0 || options.NumberOfFiles != expected_files)
    {
    parse_print_help(stderr, args[0], 0);
    exit(1);
    }

  /* open the input file */
  options.InputFileName = options.Files[0];

  if (!(ifile = fopen(options.InputFileName, "r")))
    {
    fprintf(stderr, "Error opening input file %s\n", options.InputFileName);
    exit(1);
    }

  if (options.OutputFileName == NULL &&
      options.NumberOfFiles > 1)
    {
    /* allow outfile to be given after infile, if "-o" option not used */
    options.OutputFileName = options.Files[1];
    fprintf(stderr, "Deprecated: specify output file with \"-o\".\n");
    }

  /* free the expanded args */
  free(args);

  /* open the hint file, if given on the command line */
  if (options.HintFileName && options.HintFileName[0] != '\0')
    {
    if (!(hfile = fopen(options.HintFileName, "r")))
      {
      fprintf(stderr, "Error opening hint file %s\n", options.HintFileName);
      fclose(ifile);
      exit(1);
      }
    }

  /* make sure than an output file was given on the command line */
  if (options.OutputFileName == NULL)
    {
    fprintf(stderr, "No output file was specified\n");
    fclose(ifile);
    if (hfile)
      {
      fclose(hfile);
      }
    exit(1);
    }

  /* if a hierarchy is was given, then BTX/ETX can be ignored */
  vtkParse_SetIgnoreBTX(0);
  if (options.HierarchyFileName)
    {
    vtkParse_SetIgnoreBTX(1);
    }

  /* parse the input file */
  data = vtkParse_ParseFile(options.InputFileName, ifile, stderr);

  if (!data)
    {
    exit(1);
    }

  /* fill in some blanks by using the hints file */
  if (hfile)
    {
    vtkParse_ReadHints(data, hfile, stderr);
    }

  if (data->MainClass)
    {
    /* mark class as abstract unless it has New() method */
    int nfunc = data->MainClass->NumberOfFunctions;
    int ifunc;
    for (ifunc = 0; ifunc < nfunc; ifunc++)
      {
      FunctionInfo *func = data->MainClass->Functions[ifunc];
      if (func && func->Access == VTK_ACCESS_PUBLIC &&
          func->Name && strcmp(func->Name, "New") == 0 &&
          func->NumberOfParameters == 0)
        {
        break;
        }
      }
    data->MainClass->IsAbstract = ((ifunc == nfunc) ? 1 : 0);
    }

  return data;
}

/* Command-line argument handler for wrapper tools */
void vtkParse_MainMulti(int argc, char *argv[])
{
  int argi;
  int argn;
  char **args;
  StringCache strings;

  /* expand any "@file" args */
  vtkParse_InitStringCache(&strings);
  parse_expand_args(&strings, argc, argv, &argn, &args);

  /* read the args into the static OptionInfo struct */
  argi = parse_check_options(argn, args, 1);
  free(args);

  if (argi == 0)
    {
    exit(0);
    }
  else if (argi < 0 || options.NumberOfFiles == 0)
    {
    parse_print_help(stderr, argv[0], 1);
    exit(1);
    }

  /* the input file */
  options.InputFileName = options.Files[0];
}
