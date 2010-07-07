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

 For each typedef, the output file will have a line like this:

 name = [2][3]* const type ; header.h

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

/**
 * Helper to append to a line, given the end marker
 */
static char *append_to_line(
  char *line, const char *text, size_t *pos, size_t *maxlen)
{
  size_t n;

  n = strlen(text);

  if ((*pos) + n + 1 > (*maxlen))
    {
    *maxlen = ((*pos) + n + 1 + 2*(*maxlen));
    line = (char *)realloc(line, (*maxlen));
    }

  strcpy(&line[*pos], text);
  *pos = (*pos) + n;

  return line;
}


/**
 * Read a header file with vtkParse.tab.c
 *
 * If "lines" is provided, the file contents
 * will be appended to them.
 */
static char **vtkWrapHierarchy_ParseHeaderFile(
  FILE *fp, const char *filename, const char *flags, char **lines)
{
  FileInfo *data;
  int i, j;
  size_t k, l, n;
  const char *tmpflags;
  char *line;
  size_t m, maxlen;

  /* start with a buffer of 15 chars and grow from there */
  maxlen = 1;
  m = 0;
  line = (char *)malloc(maxlen);

  /* start with just a single output line and grow from there */
  if (lines == NULL)
    {
    lines = (char **)malloc(sizeof(char *));
    lines[0] = NULL;
    }

  /* the "concrete" flag doesn't matter, just set to zero */
  data = vtkParse_ParseFile(filename, fp, stderr);

  if (!data)
    {
    free(lines);
    return 0;
    }

  /* find the last line in "lines" */
  n = 0;
  while (lines[n] != NULL)
    {
    n++;
    }

  /* add a line for each class that is found */
  for (i = 0; i < data->Contents->NumberOfItems; i++)
    {
    /* all but the main class in each file is excluded from wrapping */
    tmpflags = "WRAP_EXCLUDE";

    m = 0;
    line[m] = '\0';

    if (data->Contents->Items[i]->ItemType == VTK_CLASS_INFO ||
        data->Contents->Items[i]->ItemType == VTK_STRUCT_INFO)
      {
      ClassInfo *class_info = (ClassInfo *)data->Contents->Items[i];

      if (class_info == data->MainClass)
        {
        tmpflags = flags;
        }

      line = append_to_line(line, class_info->Name, &m, &maxlen);
      line = append_to_line(line, " ", &m, &maxlen);
      if (class_info->NumberOfSuperClasses)
        {
        line = append_to_line(line, ": ", &m, &maxlen);
        }

      for (j = 0; j < class_info->NumberOfSuperClasses; j++)
        {
        line = append_to_line(line, class_info->SuperClasses[j], &m, &maxlen);
        line = append_to_line(line, " ", &m, &maxlen);
        if (j+1 < class_info->NumberOfSuperClasses)
          {
          line = append_to_line(line, ", ", &m, &maxlen);
          }
        }

      }
    else if (data->Contents->Items[i]->ItemType == VTK_ENUM_INFO)
      {
      EnumInfo *enum_info = (EnumInfo *)data->Contents->Items[i];

      line = append_to_line(line, enum_info->Name, &m, &maxlen);
      line = append_to_line(line, " : int ", &m, &maxlen);
      }
    else if (data->Contents->Items[i]->ItemType == VTK_TYPEDEF_INFO)
      {
      ValueInfo *typedef_info = (ValueInfo *)data->Contents->Items[i];
      unsigned int type;
      int ndims;
      int dim;

      line = append_to_line(line, typedef_info->Name, &m, &maxlen);
      line = append_to_line(line, " = ", &m, &maxlen);

      type = typedef_info->Type;

      if ((type & VTK_PARSE_REF) != 0)
        {
        line = append_to_line(line, "&", &m, &maxlen);
        }

      ndims = typedef_info->NumberOfDimensions;

      for (dim = 0; dim < ndims; dim++)
        {
        line = append_to_line(line, "[", &m, &maxlen);
        line = append_to_line(line, typedef_info->Dimensions[dim],
                              &m, &maxlen);
        line = append_to_line(line, "]", &m, &maxlen);
        }

      type = (type & VTK_PARSE_POINTER_MASK);
      if (ndims > 0 && (type & VTK_PARSE_POINTER_LOWMASK) == VTK_PARSE_ARRAY)
        {
        type = ((type >> 2) & VTK_PARSE_POINTER_MASK);
        }
      else if (ndims == 1)
        {
        type = ((type >> 2) & VTK_PARSE_POINTER_MASK);
        }

      /* pointers are printed after brackets, and are intentionally
       * printed in reverse order as compared to C++ declarations */
      while (type)
        {
        unsigned int bits = (type & VTK_PARSE_POINTER_LOWMASK);
        type = ((type >> 2) & VTK_PARSE_POINTER_MASK);

        if (bits == VTK_PARSE_POINTER)
          {
          line = append_to_line(line, "*", &m, &maxlen);
          }
        else if (bits == VTK_PARSE_CONST_POINTER)
          {
          line = append_to_line(line, "const*", &m, &maxlen);
          }
        else
          {
          line = append_to_line(line, "[]", &m, &maxlen);
          }
        }

      if (line[m-1] != ' ')
        {
        line = append_to_line(line, " ", &m, &maxlen);
        }

      if ((type & VTK_PARSE_CONST) != 0)
        {
        line = append_to_line(line, "const ", &m, &maxlen);
        }

      line = append_to_line(line, typedef_info->Class, &m, &maxlen);
      line = append_to_line(line, " ", &m, &maxlen);
      }
    else
      {
      /* unhandled file element */
      continue;
      }

    k = strlen(data->FileName) - 1;
    while (k > 0 && data->FileName[k-1] != '/' && data->FileName[k-1] != '\\')
      {
      k--;
      }

    line = append_to_line(line, "; ", &m, &maxlen);
    line = append_to_line(line, &data->FileName[k], &m, &maxlen);

    if (tmpflags && tmpflags[0] != '\0')
      {
      line = append_to_line(line, " ; ", &m, &maxlen);
      line = append_to_line(line, tmpflags, &m, &maxlen);
      }

    /* check to make sure this line isn't a duplicate */
    for (l = 0; l < n; l++)
      {
      if (strcmp(line, lines[l]) == 0)
        {
        break;
        }
      }
    if (l == n)
      {
      /* allocate more memory if n+1 is a power of two */
      if (((n+1) & n) == 0)
        {
        lines = (char **)realloc(lines, (n+1)*2*sizeof(char *));
        }

      lines[n] = (char *)malloc(strlen(line)+1);
      strcpy(lines[n++], line);
      lines[n] = NULL;
      }
    }

  free(line);

  vtkParse_Free(data);

  return lines;
}

/**
 * Read a hierarchy file into "lines" without duplicating lines
 */
static char **vtkWrapHierarchy_ReadHierarchyFile(FILE *fp, char **lines)
{
  char *line;
  size_t maxlen = 15;
  size_t i, n;

  line = (char *)malloc(maxlen);

  if (lines == NULL)
    {
    lines = (char **)malloc(sizeof(char *));
    lines[0] = NULL;
    }

  while (fgets(line, maxlen, fp))
    {
    n = strlen(line);

    /* if buffer not long enough, increase it */
    while (n == maxlen-1 && line[n-1] != '\n' && !feof(fp))
      {
      maxlen *= 2;
      line = (char *)realloc(line, maxlen);
      if (!fgets(&line[n], maxlen-n, fp)) { break; }
      n += strlen(&line[n]);
      }

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
      /* allocate more memory if n+1 is a power of two */
      if (((i+1) & i) == 0)
        {
        lines = (char **)realloc(lines, (i+1)*2*sizeof(char *));
        }

      lines[i] = (char *)malloc(n+1);
      strcpy(lines[i], line);
      lines[i+1] = NULL;
      }
    }

  free(line);

  if (!feof(fp))
    {
    free(lines);
    return 0;
    }

  return lines;
}

/**
 * Compare a file to "lines", return 0 if they are different
 */
static int vtkWrapHierarchy_CompareHierarchyFile(FILE *fp, char *lines[])
{
  unsigned char *matched;
  char *line;
  size_t maxlen = 15;
  size_t i, n;

  line = (char *)malloc(maxlen);

  for (i = 0; lines[i] != NULL; i++) { ; };
  matched = (unsigned char *)malloc(i);
  memset(matched, 0, i);

  while (fgets(line, maxlen, fp))
    {
    n = strlen(line);

    /* if buffer not long enough, increase it */
    while (n == maxlen-1 && line[n-1] != '\n' && !feof(fp))
      {
      maxlen *= 2;
      line = (char *)realloc(line, maxlen);
      if (!fgets(&line[n], maxlen-n, fp)) { break; }
      n += strlen(&line[n]);
      }

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

  free(line);
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
static char **vtkWrapHierarchy_TryParseHeaderFile(
  const char *file_name, const char *flags, char **lines)
{
  FILE *input_file;

  input_file = fopen(file_name, "r");

  if (!input_file)
    {
    fprintf(stderr, "vtkWrapHierarchy: couldn't open file %s\n",
            file_name);
    exit(1);
    }

  lines = vtkWrapHierarchy_ParseHeaderFile(
                 input_file, file_name, flags, lines);

  if (!lines)
    {
    fclose(input_file);
    exit(1);
    }
  fclose(input_file);

  return lines;
}

/**
 * Try to read a file, print error and exit if fail
 */
static char **vtkWrapHierarchy_TryReadHierarchyFile(
  const char *file_name, char **lines)
{
  FILE *input_file;

  input_file = fopen(file_name, "r");
  if (!input_file)
    {
    fprintf(stderr, "vtkWrapHierarchy: couldn't open file %s\n",
            file_name);
    exit(1);
    }

  lines = vtkWrapHierarchy_ReadHierarchyFile(input_file, lines);
  if (!lines)
    {
    fclose(input_file);
    fprintf(stderr, "vtkWrapHierarchy: error reading file %s\n",
            file_name);
    exit(1);
    }
  fclose(input_file);

  return lines;
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
  if (output_file && vtkWrapHierarchy_CompareHierarchyFile(output_file, lines))
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
      output_file = fopen(file_name, "r+");
      if (output_file &&
          vtkWrapHierarchy_CompareHierarchyFile(output_file, lines))
        {
        /* if the contents match, no need to write it */
        fclose(output_file);
        return 0;
        }
      if (output_file)
        {
        /* close and open in order to truncate the file */
        fclose(output_file);
        output_file = fopen(file_name, "w");
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

static int string_compare(const void *vp1, const void *vp2)
{
  return strcmp(*(const char **)vp1, *(const char **)vp2);
}

int main(int argc, char *argv[])
{
  int usage_error = 0;
  char *output_filename = 0;
  int i, argi;
  size_t j, n;
  char **lines = 0;
  char **files = 0;
  char *flags;

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

  /* read the data file */
  files = vtkWrapHierarchy_TryReadHierarchyFile(argv[argi++], files);

  /* read in all the prior files */
  while (argi < argc)
    {
    lines = vtkWrapHierarchy_TryReadHierarchyFile(argv[argi++], lines);
    }

  /* merge the files listed in the data file */
  for (i = 0; files[i] != NULL; i++)
    {
    flags = files[i];
    /* look for semicolon that marks start of flags */
    while(*flags != ';' && *flags != '\0') { flags++; };
    if (*flags == ';') { *flags++ = '\0'; }

    lines = vtkWrapHierarchy_TryParseHeaderFile(files[i], flags, lines);
    }

  /* sort the lines to ease lookups in the file */
  for (n = 0; lines[n]; n++) { ; };
  qsort(lines, n, sizeof(char *), &string_compare);

  /* write the file, if it has changed */
  vtkWrapHierarchy_TryWriteHierarchyFile(output_filename, lines);

  for (j = 0; j < n; j++)
    {
    free(lines[j]);
    }

  free(files);
  free(lines);
  return 0;
}
