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
#include "vtkType.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

static size_t skip_space(const char *text)
{
  size_t i = 0;
  while (isspace(text[i]) && text[i] != '\n') { i++; }
  return i;
}

static size_t skip_name(const char *text)
{
  unsigned int depth = 0;
  size_t i = 0;

  if (isalpha(text[i]) || text[i] == '_' ||
      (text[i] == ':' && text[i+1] == ':'))
    {
    if (text[i] == ':') { i++; }
    i++;
    while (isalnum(text[i]) || text[i] == '_' ||
           (text[i] == ':' && text[i+1] == ':') ||
           text[i] == '<' || text[i] == '>')
      {
      if (text[i] == '<')
        {
        while (text[i] != '\0' && text[i] != '\n')
          {
          if (text[i] == '<') { depth++; }
          if (text[i] == '>') { if (--depth == 0) { break; } }
          i++;
          }
        }
      if (text[i] == ':') { i++; }
      i++;
      }
    }
  return i;
}

/* helper: comparison of entries */
static int compare_hierarchy_entries(const void *vp1, const void *vp2)
{
  const HierarchyEntry *entry1 = (const HierarchyEntry *)vp1;
  const HierarchyEntry *entry2 = (const HierarchyEntry *)vp2;

  return strcmp(entry1->Name, entry2->Name);
}

/* helper: sort the entries to facilitate searching */
static void sort_hierarchy_entries(HierarchyInfo *info)
{
  qsort(info->Entries, info->NumberOfEntries, sizeof(HierarchyEntry),
        &compare_hierarchy_entries);
}

/* Find an entry with a binary search */
HierarchyEntry *vtkParseHierarchy_FindEntry(
  const HierarchyInfo *info, const char *classname)
{
  HierarchyEntry key;
  key.Name = (char *)classname;

  return (HierarchyEntry *)bsearch(&key, info->Entries,
    info->NumberOfEntries, sizeof(HierarchyEntry),
    &compare_hierarchy_entries);
}


/* read a hierarchy file into a HeirarchyInfo struct, or return NULL */
HierarchyInfo *vtkParseHierarchy_ReadFile(const char *filename)
{
  HierarchyInfo *info;
  HierarchyEntry *entry;
  int maxClasses = 500;
  FILE *fp;
  char *line;
  char *cp;
  const char *ccp;
  size_t maxlen = 15;
  size_t i, j, k, n;
  unsigned int bits, pointers;

  line = (char *)malloc(maxlen);

  fp = fopen(filename, "r");

  if (fp == NULL)
    {
    return NULL;
    }

  info = (HierarchyInfo *)malloc(sizeof(HierarchyInfo));
  info->NumberOfEntries = 0;
  info->Entries = (HierarchyEntry *)malloc(maxClasses*sizeof(HierarchyEntry));

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

    while (n > 0 && isspace(line[n-1]))
      {
      n--;
      }
    line[n] = '\0';

    if (line[0] == '\0')
      {
      continue;
      }

    if (info->NumberOfEntries == maxClasses)
      {
      maxClasses *= 2;
      info->Entries = (HierarchyEntry *)realloc(
        info->Entries, sizeof(HierarchyEntry)*maxClasses*2);
      }

    entry = &info->Entries[info->NumberOfEntries++];
    entry->Name = NULL;
    entry->HeaderFile = NULL;
    entry->NumberOfProperties = 0;
    entry->Properties = NULL;
    entry->NumberOfSuperClasses = 0;
    entry->SuperClasses = NULL;
    entry->SuperClassIndex = NULL;
    entry->Typedef = NULL;
    entry->IsTypedef = 0;
    entry->IsEnum = 0;

    i = skip_space(line);
    n = skip_name(&line[i]);

    cp = (char *)malloc(n+1);
    strncpy(cp, &line[i], n);
    cp[n] = '\0';
    entry->Name = cp;
    i += n;

    i += skip_space(&line[i]);

    /* classes (and possibly enums) */
    if (line[i] == ':')
      {
      i++;
      i += skip_space(&line[i]);
      n = skip_name(&line[i]);
      /* check for enum indicators */
      if ((n == 3 && strncmp(&line[i], "int", n)) ||
          (n == 4 && strncmp(&line[i], "enum", n)))
        {
        entry->IsEnum = 1;
        i += n;
        i += skip_space(&line[i]);
        }
      /* else check for superclasses */
      else for (j = 0; ; j++)
        {
        if (j == 0)
          {
          entry->SuperClasses = (const char **)malloc(sizeof(char *));
          entry->SuperClassIndex = (int *)malloc(sizeof(int));
          }
        else
          {
          entry->SuperClasses = (const char **)realloc(
            (char **)entry->SuperClasses, (j+1)*sizeof(char *));
          entry->SuperClassIndex = (int *)realloc(
            entry->SuperClassIndex, (j+1)*sizeof(int));
          }
        entry->NumberOfSuperClasses++;

        i += skip_space(&line[i]);
        n = skip_name(&line[i]);
        cp = (char *)malloc(n+1);
        strncpy(cp, &line[i], n);
        cp[n] = '\0';
        entry->SuperClasses[j] = cp;
        entry->SuperClassIndex[j] = -1;
        i += n;

        i += skip_space(&line[i]);
        if (line[i] != ',')
          {
          break;
          }
        i++;
        }
      }

    /* read typedefs */
    else if (line[i] == '=')
      {
      i++;
      i += skip_space(&line[i]);
      entry->IsTypedef = 1;
      entry->Typedef = (ValueInfo *)malloc(sizeof(ValueInfo));
      vtkParse_InitValue(entry->Typedef);

      /* type is a reference (does this ever occur?) */
      if (line[i] == '&')
        {
        i++;
        i += skip_space(&line[i]);
        entry->Typedef->Type |= VTK_PARSE_REF;
        }

      /* type has array dimensions */
      if (line[i] == '[')
        {
        entry->Typedef->Count = 1;
        }

      while (line[i] == '[')
        {
        i++;
        n = 0;
        while (line[i+n] != ']' && line[i+n] != '\n' && line[i+n] != '\0')
          {
          n++;
          }
        ccp = vtkParse_DuplicateString(&line[i], n);
        vtkParse_AddStringToArray(&entry->Typedef->Dimensions,
                                  &entry->Typedef->NumberOfDimensions, ccp);
        if (ccp[0] >= '0' && ccp[0] <= '9')
          {
          entry->Typedef->Count *= (int)strtol(ccp, NULL, 0);
          }
        else
          {
          entry->Typedef->Count = 0;
          }
        i += n;
        if (line[i] == ']')
          {
          i++;
          }
        }
      i += skip_space(&line[i]);

      /* look for pointers (and const pointers) */
      bits = 0;
      while (line[i] == '*' || strncmp(&line[i], "const*", 6) == 0)
        {
        bits = (bits << 2);
        if (line[i] == '*')
          {
          bits = (bits | VTK_PARSE_POINTER);
          }
        else
          {
          bits = (bits | VTK_PARSE_CONST_POINTER);
          i += 5;
          }
        bits = (bits & VTK_PARSE_POINTER_MASK);
        i++;
        i += skip_space(&line[i]);
        }

      /* need to reverse to get correct pointer order */
      pointers = 0;
      while (bits)
        {
        pointers = (pointers << 2);
        pointers = (pointers | (bits & VTK_PARSE_POINTER_LOWMASK));
        bits = ((bits >> 2) & VTK_PARSE_POINTER_MASK);
        }

      /* add pointer indirection to correspond to first array dimension */
      if (entry->Typedef->NumberOfDimensions > 1)
        {
        pointers = ((pointers << 2) | VTK_PARSE_ARRAY);
        }
      else if (entry->Typedef->NumberOfDimensions == 1)
        {
        pointers = ((pointers << 2) | VTK_PARSE_POINTER);
        }

      /* include the pointers in the type */
      entry->Typedef->Type |= (pointers & VTK_PARSE_POINTER_MASK);

      /* read the base type (and const) */
      bits = 0;
      while (line[i] != ';' && line[i] != '\n' && line[i] != '\0')
        {
        n = skip_name(&line[i]);
        if (n == 5 && strncmp(&line[i], "const", n) == 0)
          {
          entry->Typedef->Type |= VTK_PARSE_CONST;
          }
        else if (n == 8 && strncmp(&line[i], "unsigned", n) == 0)
          {
          entry->Typedef->Type |= VTK_PARSE_UNSIGNED;
          if (bits == 0)
            {
            bits = VTK_PARSE_INT;
            }
          }
        else if (n == 6 && strncmp(&line[i], "signed", n) == 0)
          {
          if (bits == VTK_PARSE_CHAR)
            {
            bits = VTK_PARSE_SIGNED_CHAR;
            }
          else
            {
            bits = VTK_PARSE_INT;
            }
          }
        else if (n == 3 && strncmp(&line[i], "int", n) == 0)
          {
          if (bits == 0)
            {
            bits = VTK_PARSE_INT;
            }
          }
        else if (n == 4 && strncmp(&line[i], "long", n) == 0)
          {
          if (bits == VTK_PARSE_LONG)
            {
            bits = VTK_PARSE_LONG_LONG;
            }
          else
            {
            bits = VTK_PARSE_LONG;
            }
          }
        else if (n == 5 && strncmp(&line[i], "short", n) == 0)
          {
          bits = VTK_PARSE_SHORT;
          }
        else if (n == 4 && strncmp(&line[i], "char", n) == 0)
          {
          if (bits == VTK_PARSE_INT)
            {
            bits = VTK_PARSE_SIGNED_CHAR;
            }
          else
            {
            bits = VTK_PARSE_CHAR;
            }
          }
        else if (n == 5 && strncmp(&line[i], "float", n) == 0)
          {
          bits = VTK_PARSE_FLOAT;
          }
        else if (n == 6 && strncmp(&line[i], "double", n) == 0)
          {
          bits = VTK_PARSE_DOUBLE;
          }
        else if (n == 4 && strncmp(&line[i], "bool", n) == 0)
          {
          bits = VTK_PARSE_BOOL;
          }
        else if (n == 4 && strncmp(&line[i], "void", n) == 0)
          {
          bits = VTK_PARSE_VOID;
          }
        else if (n == 7 && strncmp(&line[i], "__int64", n) == 0)
          {
          bits = VTK_PARSE___INT64;
          }
        else if (n == 6 && strncmp(&line[i], "size_t", n) == 0)
          {
          entry->Typedef->Class = "size_t";
          bits = VTK_PARSE_SIZE_T;
          }
        else if (n == 7 && strncmp(&line[i], "ssize_t", n) == 0)
          {
          entry->Typedef->Class = "ssize_t";
          bits = VTK_PARSE_SSIZE_T;
          }
        else if (n == 9 && strncmp(&line[i], "vtkIdType", n) == 0)
          {
          entry->Typedef->Class = "vtkIdType";
          bits = vtkParse_MapType(VTK_ID_TYPE);
          }
        else if (n == 11 && strncmp(&line[i], "vtkTypeInt8", n) == 0)
          {
          entry->Typedef->Class = vtkParse_DuplicateString(&line[i], n);
          bits = vtkParse_MapType(VTK_TYPE_INT8);
          }
        else if (n == 12 && strncmp(&line[i], "vtkTypeUInt8", n) == 0)
          {
          entry->Typedef->Class = vtkParse_DuplicateString(&line[i], n);
          bits = vtkParse_MapType(VTK_TYPE_UINT8);
          }
        else if (n == 12 && strncmp(&line[i], "vtkTypeInt16", n) == 0)
          {
          entry->Typedef->Class = vtkParse_DuplicateString(&line[i], n);
          bits = vtkParse_MapType(VTK_TYPE_UINT16);
          }
        else if (n == 13 && strncmp(&line[i], "vtkTypeUInt16", n) == 0)
          {
          entry->Typedef->Class = vtkParse_DuplicateString(&line[i], n);
          bits = vtkParse_MapType(VTK_TYPE_UINT16);
          }
        else if (n == 12 && strncmp(&line[i], "vtkTypeInt32", n) == 0)
          {
          entry->Typedef->Class = vtkParse_DuplicateString(&line[i], n);
          bits = vtkParse_MapType(VTK_TYPE_UINT32);
          }
        else if (n == 13 && strncmp(&line[i], "vtkTypeUInt32", n) == 0)
          {
          entry->Typedef->Class = vtkParse_DuplicateString(&line[i], n);
          bits = vtkParse_MapType(VTK_TYPE_UINT32);
          }
        else if (n == 12 && strncmp(&line[i], "vtkTypeInt64", n) == 0)
          {
          entry->Typedef->Class = vtkParse_DuplicateString(&line[i], n);
          bits = vtkParse_MapType(VTK_TYPE_UINT64);
          }
        else if (n == 13 && strncmp(&line[i], "vtkTypeUInt64", n) == 0)
          {
          entry->Typedef->Class = vtkParse_DuplicateString(&line[i], n);
          bits = vtkParse_MapType(VTK_TYPE_UINT64);
          }
        else if (n == 14 && strncmp(&line[i], "vtkTypeFloat32", n) == 0)
          {
          entry->Typedef->Class = vtkParse_DuplicateString(&line[i], n);
          bits = vtkParse_MapType(VTK_TYPE_FLOAT32);
          }
        else if (n == 14 && strncmp(&line[i], "vtkTypeFloat64", n) == 0)
          {
          entry->Typedef->Class = vtkParse_DuplicateString(&line[i], n);
          bits = vtkParse_MapType(VTK_TYPE_FLOAT64);
          }
        else if (n == 12 && strncmp(&line[i], "vtkStdString", n) == 0)
          {
          entry->Typedef->Class = vtkParse_DuplicateString(&line[i], n);
          bits = VTK_PARSE_STRING;
          }
        else if (n == 16 && strncmp(&line[i], "vtkUnicodeString", n) == 0)
          {
          entry->Typedef->Class = vtkParse_DuplicateString(&line[i], n);
          bits = VTK_PARSE_UNICODE_STRING;
          }
        else if (strncmp(&line[i], "vtk", 3) == 0)
          {
          entry->Typedef->Class = vtkParse_DuplicateString(&line[i], n);
          bits = VTK_PARSE_OBJECT;
          for (k = 0; k < n; k++)
            {
            if (line[i+k] == ':')
              {
              bits = VTK_PARSE_UNKNOWN;
              break;
              }
            }
          }
        else
          {
          entry->Typedef->Class = vtkParse_DuplicateString(&line[i], n);
          bits = VTK_PARSE_UNKNOWN;
          }

        i += n;
        i += skip_space(&line[i]);
        }

      /* add the base type to the type */
      entry->Typedef->Type |= bits;
      if (!entry->Typedef->Class)
        {
        ccp = 0;
        switch (entry->Typedef->Type & VTK_PARSE_BASE_TYPE)
          {
          case VTK_PARSE_CHAR: ccp = "char"; break;
          case VTK_PARSE_SHORT: ccp = "short"; break;
          case VTK_PARSE_INT: ccp = "int"; break;
          case VTK_PARSE_LONG: ccp = "long"; break;
          case VTK_PARSE_LONG_LONG: ccp = "long long"; break;
          case VTK_PARSE___INT64: ccp = "__int64"; break;
          case VTK_PARSE_UNSIGNED_CHAR: ccp = "unsigned char"; break;
          case VTK_PARSE_UNSIGNED_SHORT: ccp="unsigned short"; break;
          case VTK_PARSE_UNSIGNED_INT: ccp = "unsigned int"; break;
          case VTK_PARSE_UNSIGNED_LONG: ccp="unsigned long"; break;
          case VTK_PARSE_UNSIGNED_LONG_LONG: ccp = "unsigned long long"; break;
          case VTK_PARSE_UNSIGNED___INT64: ccp = "unsigned __int64"; break;
          case VTK_PARSE_SIGNED_CHAR: ccp = "signed char"; break;
          case VTK_PARSE_FLOAT: ccp = "float"; break;
          case VTK_PARSE_DOUBLE: ccp = "double"; break;
          case VTK_PARSE_BOOL: ccp = "bool"; break;
          case VTK_PARSE_VOID: ccp = "void"; break;
          }
        entry->Typedef->Class = ccp;
        }
      }

    /* get the header file */
    if (line[i] == ';')
      {
      i++;
      i += skip_space(&line[i]);
      n = 0;
      while(line[i+n] != '\0' && line[i+n] != ';' &&
            !isspace(line[i+n])) { n++; };
      cp = (char *)malloc(n+1);
      strncpy(cp, &line[i], n);
      cp[n] = '\0';
      entry->HeaderFile = cp;

      i += n;
      i += skip_space(&line[i]);

      /* get all flags */
      while (line[i] == ';')
        {
        i++;
        i += skip_space(&line[i]);
        if (entry->NumberOfProperties == 0)
          {
          entry->Properties = (const char **)malloc(sizeof(char **));
          }
        else
          {
          entry->Properties = (const char **)realloc(
            (char **)entry->Properties,
            (entry->NumberOfProperties+1)*sizeof(char **));
          }
        n = 0;
        while (line[i+n] != '\0' && line[i+n] != '\n' && line[i+n] != ';')
          { n++; }
        if (n && skip_space(&line[i]) != n)
          {
          cp = (char *)malloc((n+1)*sizeof(char *));
          strncpy(cp, &line[i], n);
          cp[n] = '\0';
          entry->Properties[entry->NumberOfProperties++] = cp;
          }
        i += n;
        }
      }
    }

  if (!feof(fp))
    {
    vtkParseHierarchy_Free(info);
    info = NULL;
    }

  free(line);

  sort_hierarchy_entries(info);

  return info;
}

/* free a HierarchyInfo struct */
void vtkParseHierarchy_Free(HierarchyInfo *info)
{
  HierarchyEntry *entry;
  int i, j;

  for (i = 0; i < info->NumberOfEntries; i++)
    {
    entry = &info->Entries[i];
    free((char *)entry->Name);
    free((char *)entry->HeaderFile);
    for (j = 0; j < entry->NumberOfSuperClasses; j++)
      {
      free((char *)entry->SuperClasses[j]);
      }
    if (entry->NumberOfSuperClasses)
      {
      free((char **)entry->SuperClasses);
      free(entry->SuperClassIndex);
      }
    for (j = 0; j < entry->NumberOfProperties; j++)
      {
      free((char *)entry->Properties[j]);
      }
    if (entry->NumberOfProperties)
      {
      free((char **)entry->Properties);
      }
    }

  free(info->Entries);
  free(info);
}

int vtkParseHierarchy_IsTypeOf(
  const HierarchyInfo *info, const HierarchyEntry *entry,
  const char *superclass)
{
  int iterating = 1;
  int i, j;

  while (iterating)
    {
    iterating = 0;

    if (strcmp(entry->Name, superclass) == 0)
      {
      return 1;
      }

    if (entry->NumberOfSuperClasses == 0)
      {
      return 0;
      }

    for (j = 0; j < entry->NumberOfSuperClasses; j++)
      {
      if (strcmp(entry->SuperClasses[j], superclass) == 0)
        {
        return 1;
        }

      i = entry->SuperClassIndex[j];
      if (i == -1)
        {
        HierarchyEntry *tmp =
          vtkParseHierarchy_FindEntry(info, entry->SuperClasses[j]);
        if (tmp)
          {
          i = (int)(tmp - info->Entries);
          /* cache the position of the superclass */
          ((HierarchyEntry *)entry)->SuperClassIndex[j] = i;
          }
        else
          {
          /* outside of hierarchy, can't search */
          i = -2;
          ((HierarchyEntry *)entry)->SuperClassIndex[j] = i;
          }
        }
      if (i >= 0)
        {
        if (j+1 >= entry->NumberOfSuperClasses)
          {
          iterating = 1;
          entry = &info->Entries[i];
          break;
          }

        /* recurse for multiple inheritance */
        if (vtkParseHierarchy_IsTypeOf(info, &info->Entries[i], superclass))
          {
          return 1;
          }
        }
      }
    }

  return 0;
}


/* get the specified property, or return NULL */
const char *vtkParseHierarchy_GetProperty(
  const HierarchyEntry *entry, const char *property)
{
  int i;
  size_t k;

  if (entry)
    {
    for (i = 0; i < entry->NumberOfProperties; i++)
      {
      /* skip the property name, everything after is the property */
      k = skip_name(entry->Properties[i]);
      if (k == strlen(property) &&
          strncmp(entry->Properties[i], property, k) == 0)
        {
        if (entry->Properties[i][k] == ' ' ||
            entry->Properties[i][k] == '=') { k++; }
        return &entry->Properties[i][k];
        }
      }
    }

  return NULL;
}

/* Expand an unrecognized type in a ValueInfo struct by
 * using the typedefs in the HierarchyInfo struct. */
int vtkParseHierarchy_ExpandTypedefs(
  const HierarchyInfo *info, ValueInfo *val, const char *scope)
{
  char text[128];
  char *cp;
  size_t n, m;
  int i;
  HierarchyEntry *entry;

  while (((val->Type & VTK_PARSE_BASE_TYPE) == VTK_PARSE_OBJECT ||
          (val->Type & VTK_PARSE_BASE_TYPE) == VTK_PARSE_UNKNOWN) &&
         val->Class != 0)
   {
   entry = 0;

   /* search for the type in the provided scope */
   while (entry == 0 && scope != 0)
     {
     cp = text;
     n = strlen(scope);
     m = strlen(val->Class);
     if (n + m + 2 >= 128)
       {
       cp = (char *)malloc(n+m+3);
       }

     /* scope the name */
     strncpy(cp, scope, n);
     cp[n++] = ':';
     cp[n++] = ':';
     strncpy(&cp[n], val->Class, m);
     cp[n+m] = '\0';

     entry = vtkParseHierarchy_FindEntry(info, cp);

     if (cp != text)
       {
       free(cp);
       }

     /* if not found, try inherited scopes */
     if (entry == 0)
       {
       entry = vtkParseHierarchy_FindEntry(info, scope);
       scope = 0;
       if (entry && entry->NumberOfSuperClasses)
         {
         for (i = 0; i+1 < entry->NumberOfSuperClasses; i++)
           {
           scope = entry->SuperClasses[i];
           /* recurse if more than one superclass */
           if (vtkParseHierarchy_ExpandTypedefs(info, val, scope))
             {
             return 1;
             }
           }
         scope = entry->SuperClasses[i];
         }
       entry = 0;
       }
     }

   /* if not found, try again with no scope */
   if (entry == 0)
     {
     entry = vtkParseHierarchy_FindEntry(info, val->Class);
     }

   if (entry && entry->IsTypedef)
     {
     vtkParse_ExpandTypedef(val, entry->Typedef);
     }
   else if (entry)
     {
     return 1;
     }
   else
     {
     return 0;
     }
   }

  return 1;
}
