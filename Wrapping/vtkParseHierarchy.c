/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkParseHierarchy.c

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

#include "vtkParseHierarchy.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

static int skip_space(const char *text)
{
  int i = 0;
  while (isspace(text[i]) && text[i] != '\n') { i++; }
  return i;
}

static int skip_name(const char *text)
{
  int i = 0;
  if (isalpha(text[i]) || text[i] == '_' ||
      (text[i] == ':' && text[i+1] == ':'))
    {
    i++;
    while (isalnum(text[i]) || text[i] == '_' ||
           (text[i] == ':' && text[i+1] == ':'))
      {
      if (text[i] == ':') { i++; }
      i++;
      }
    }
  return i;
}

/* read a hierarchy file into a HeirarchyInfo struct, or return NULL */
HierarchyInfo *vtkParseHierarchy_ReadFile(const char *filename)
{
  HierarchyInfo *info;
  HierarchyEntry *entry;
  int maxClasses = 512;
  FILE *fp;
  char line[2048];
  int i, j, n;

  fp = fopen(filename, "r");

  if (fp == NULL)
    {
    return NULL;
    }

  info = (HierarchyInfo *)malloc(sizeof(HierarchyInfo));
  info->NumberOfClasses = 0;
  info->Classes = (HierarchyEntry *)malloc(maxClasses*sizeof(HierarchyEntry));

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

    if (info->NumberOfClasses == maxClasses)
      {
      entry = info->Classes;
      info->Classes =
        (HierarchyEntry *)malloc(sizeof(HierarchyEntry)*maxClasses*2);
      memcpy(info->Classes, entry, sizeof(HierarchyEntry)*maxClasses);
      free(entry);
      maxClasses *= 2;
      }

    entry = &info->Classes[info->NumberOfClasses++];
    entry->ClassName = NULL;
    entry->SuperClasses[0] = NULL;
    entry->HeaderFile = NULL;

    i = skip_space(line);
    n = skip_name(&line[i]);

    entry->ClassName = (char *)malloc(n+1);
    strncpy(entry->ClassName, &line[i], n);
    entry->ClassName[n] = '\0';
    i += n;

    i += skip_space(&line[i]);

    if (line[i] == ':')
      {
      i++;
      for (j = 0; j < 9; j++)
        {
        i += skip_space(&line[i]);
        n = skip_name(&line[i]);
        entry->SuperClasses[j] = (char *)malloc(n+1);
        strncpy(entry->SuperClasses[j], &line[i], n);
        entry->SuperClasses[j][n] = '\0';
        entry->SuperClassIndex[j] = -1;
        entry->SuperClasses[j+1] = NULL;
        i += n;

        i += skip_space(&line[i]);
        if (line[i] != ',')
          {
          break;
          }
        }
      }

    if (line[i] == ';')
      {
      i++;
      i += skip_space(&line[i]);
      n = 0;
      while(line[i+n] != '\0' && !isspace(line[i+n])) { n++; };
      entry->HeaderFile = (char *)malloc(n+1);
      strncpy(entry->HeaderFile, &line[i], n);
      entry->HeaderFile[n] = '\0';
      }
    }

  if (!feof(fp))
    {
    vtkParseHierarchy_Free(info);
    info = NULL;
    }

  return info;
}

/* free a HierarchyInfo struct */
void vtkParseHierarchy_Free(HierarchyInfo *info)
{
  HierarchyEntry *entry;
  int i, j;

  for (i = 0; i < info->NumberOfClasses; i++)
    {
    entry = &info->Classes[i];
    free(entry->ClassName);
    free(entry->HeaderFile);
    for (j = 0; entry->SuperClasses[j] != NULL; j++)
      {
      free(entry->SuperClasses[j]);
      }
    }

  free(info->Classes);
  free(info);
}

static int superclass_helper(
  const HierarchyInfo *info, const HierarchyEntry *entry,
  const char *superclass)
{
  int iterating = 1;
  int i, j;

  while (iterating)
    {
    iterating = 0;

    if (strcmp(entry->ClassName, superclass) == 0)
      {
      return 1;
      }

    if (entry->SuperClasses[0] == NULL)
      {
      return 0;
      }

    for (j = 0; entry->SuperClasses[j] != NULL; j++)
      {
      if (strcmp(entry->SuperClasses[j], superclass) == 0)
        {
        return 1;
        }

      i = entry->SuperClassIndex[j];
      if (i == -1)
        {
        for (i = 0; i < info->NumberOfClasses; i++)
          {
          if (strcmp(entry->SuperClasses[j], info->Classes[i].ClassName) == 0)
            {
            /* cache the position of the superclass */
            ((HierarchyEntry *)entry)->SuperClassIndex[j] = i;
            break;
            }
          }
        if (i >= info->NumberOfClasses)
          {
          /* outside of hierarchy, can't search */
          i = -2;
          ((HierarchyEntry *)entry)->SuperClassIndex[j] = i;
          }
        }
      if (i >= 0)
        {
        if (entry->SuperClasses[j+1] == NULL)
          {
          iterating = 1;
          entry = &info->Classes[i];
          break;
          }

        /* recurse for multiple inheritance */
        if (superclass_helper(info, &info->Classes[i], superclass))
          {
          return 1;
          }
        }
      }
    }

  return 0;
}


/* check whether class 2 is a subclass of class 1 */
int vtkParseHierarchy_IsTypeOf(
  const HierarchyInfo *info, const char *subclass, const char *superclass)
{
  HierarchyEntry *entry;
  int i;

  for (i = 0; i < info->NumberOfClasses; i++)
    {
    entry = &info->Classes[i];
    if (strcmp(subclass, entry->ClassName) == 0)
      {
      return superclass_helper(info, entry, superclass);
      }
    }

  return 0;
}

/* check if the specified class is external to the hierarchy */
int vtkParseHierarchy_IsExtern(
  const HierarchyInfo *info, const char *classname)
{
  HierarchyEntry *entry;
  int i;

  for (i = 0; i < info->NumberOfClasses; i++)
    {
    entry = &info->Classes[i];
    if (strcmp(classname, entry->ClassName) == 0)
      {
      return 0;
      }
    }

  return 1;
}

/* get the header file for the specified class */
const char *vtkParseHierarchy_ClassHeader(
  const HierarchyInfo *info, const char *classname)
{
  HierarchyEntry *entry;
  int i;

  for (i = 0; i < info->NumberOfClasses; i++)
    {
    entry = &info->Classes[i];
    if (strcmp(classname, entry->ClassName) == 0)
      {
      return entry->HeaderFile;
      }
    }

  return NULL;
}

/* get the nth superclass for specified class, or return null */
const char *vtkParseHierarchy_ClassSuperClass(
  const HierarchyInfo *info, const char *classname, int i)
{
  HierarchyEntry *entry;
  int j, k;

  for (j = 0; j < info->NumberOfClasses; j++)
    {
    entry = &info->Classes[j];
    if (strcmp(classname, entry->ClassName) == 0)
      {
      for (k = 0; k <= i; k++)
        {
        if (entry->SuperClasses[k] == NULL)
          {
          return NULL;
          }
        return entry->SuperClasses[i];
        }
      }
    }

  return NULL;
}
