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
#include "vtkParseExtras.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <assert.h>

static size_t skip_space(const char *text)
{
  size_t i = 0;
  while (isspace(text[i]) && text[i] != '\n') { i++; }
  return i;
}

static size_t skip_expression(const char *text, const char *delims)
{
  char newdelims[2];
  size_t i = 0;
  size_t j;
  int use_angle = 0;
  char c;

  for (j = 0; delims[j] != '\0'; j++)
  {
    if (delims[j] == '>')
    {
      use_angle = 1;
    }
  }

  while (text[i] != '\0')
  {
    c = text[i];
    j = 0;
    while (c != delims[j] && delims[j] != '\0') { j++; }
    if (delims[j] != '\0' || c == '\0') { break; }
    if (c == '\"' || c == '\'')
    {
      char d = c;
      i++;
      while (text[i] != d && text[i] != '\0')
      {
        if (text[i] == '\\' && text[i+1] != '\0') { i++; }
        i++;
      }
      c = text[i];
      if (c == '\0') { break; }
    }
    i++;
    if (c == '(' || c == '[' || c == '{' || (use_angle && c == '<'))
    {
      if (c == '(') { newdelims[0] = ')'; }
      if (c == '[') { newdelims[0] = ']'; }
      if (c == '{') { newdelims[0] = '}'; }
      if (c == '<') { newdelims[0] = '>'; }
      newdelims[1] = '\0';

      i += skip_expression(&text[i], newdelims);

      if (text[i] == newdelims[0]) { i++; } else { break; }
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

/* forward declaration */
static int vtkParseHierarchy_ReadFileIntoInfo(
  HierarchyInfo* info, const char *filename);

/* Find an entry with a binary search */
HierarchyEntry *vtkParseHierarchy_FindEntry(
  const HierarchyInfo *info, const char *classname)
{
  HierarchyEntry key;
  HierarchyEntry *entry;
  size_t i, n;
  char name[32];
  char *cp;

  /* use classname as-is for the search if possible */
  cp = (char *)classname;

  /* get portion of name before final template parameters */
  n = vtkParse_UnscopedNameLength(classname);
  i = 0;
  while (classname[i+n] == ':' && classname[i+n+1] == ':')
  {
    i += n + 2;
    n = vtkParse_UnscopedNameLength(&classname[i]);
  }
  i += vtkParse_IdentifierLength(&classname[i]);

  /* create a new (shorter) search string if necessary */
  if (classname[i] != '\0')
  {
    /* use stack space if possible */
    cp = name;
    /* otherwise, use malloc */
    if (i > 31)
    {
      cp = (char *)malloc(i+1);
    }
    strncpy(cp, classname, i);
    cp[i] = '\0';
  }

  key.Name = cp;

  entry = (HierarchyEntry *)bsearch(&key, info->Entries,
    info->NumberOfEntries, sizeof(HierarchyEntry),
    &compare_hierarchy_entries);

  if (cp != classname && cp != name)
  {
    free(cp);
  }

  return entry;
}

/* read a hierarchy file into a HeirarchyInfo struct, or return NULL
 * XXX DEPRECATED; use vtkParseHierarchy_ReadFiles
 */
HierarchyInfo *vtkParseHierarchy_ReadFile(const char *filename)
{
  char *fn = (char *)filename;
  return vtkParseHierarchy_ReadFiles(1, &fn);
}

/* read hierarchy files into a HierarchyInfo struct, or return NULL */
HierarchyInfo *vtkParseHierarchy_ReadFiles(int n, char **filenames)
{
  HierarchyInfo *info;
  int currentFile = 0;

  info = (HierarchyInfo *)malloc(sizeof(HierarchyInfo));
  info->MaxNumberOfEntries = 500;
  info->NumberOfEntries = 0;
  info->Entries =
    (HierarchyEntry *)malloc(info->MaxNumberOfEntries*sizeof(HierarchyEntry));
  info->Strings = (StringCache *)malloc(sizeof(StringCache));
  vtkParse_InitStringCache(info->Strings);

  for (currentFile = 0; currentFile < n; currentFile++)
  {
    if (!vtkParseHierarchy_ReadFileIntoInfo(info, filenames[currentFile]))
    {
      vtkParseHierarchy_Free(info);
      info = NULL;
      break;
    }
  }

  if (info)
  {
    sort_hierarchy_entries(info);
  }

  return info;
}

/* read hierarchy file into a HierarchyInfo struct, return 1 if success */
static int vtkParseHierarchy_ReadFileIntoInfo(
  HierarchyInfo* info, const char *filename)
{
  HierarchyEntry *entry;

  FILE *fp;
  char *line;
  char *cp;
  const char *ccp;
  size_t maxlen = 15;
  size_t i, j, n, m;
  unsigned int bits, pointers;
  static const char *delims = ">,=";
  int success = 1;

  fp = fopen(filename, "r");

  if (fp == NULL)
  {
    return 0;
  }

  line = (char *)malloc(maxlen);

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

    if (info->NumberOfEntries == info->MaxNumberOfEntries)
    {
      info->MaxNumberOfEntries *= 2;
      info->Entries = (HierarchyEntry *)realloc(
        info->Entries, sizeof(HierarchyEntry)*info->MaxNumberOfEntries);
    }

    entry = &info->Entries[info->NumberOfEntries++];
    entry->Name = NULL;
    entry->HeaderFile = NULL;
    entry->Module = NULL;
    entry->NumberOfTemplateParameters = 0;
    entry->TemplateParameters = NULL;
    entry->TemplateDefaults = NULL;
    entry->NumberOfProperties = 0;
    entry->Properties = NULL;
    entry->NumberOfSuperClasses = 0;
    entry->SuperClasses = NULL;
    entry->SuperClassIndex = NULL;
    entry->Typedef = NULL;
    entry->IsTypedef = 0;
    entry->IsEnum = 0;

    i = skip_space(line);
    n = vtkParse_NameLength(&line[i]);
    for (m = 0; m < n; m++)
    {
      if (line[i+m] == '<') { break; }
    }

    entry->Name = vtkParse_CacheString(info->Strings, &line[i], m);
    i += m;

    if (line[i] == '<')
    {
      i++;
      i += skip_space(&line[i]);

      for (j = 0; line[i] != '>' && line[i] != '\0'; j++)
      {
        if (j == 0)
        {
          entry->TemplateParameters = (const char **)malloc(sizeof(char *));
          entry->TemplateDefaults = (const char **)malloc(sizeof(char *));
        }
        else
        {
          entry->TemplateParameters = (const char **)realloc(
            (char **)entry->TemplateParameters, (j+1)*sizeof(char *));
          entry->TemplateDefaults = (const char **)realloc(
            (char **)entry->TemplateDefaults, (j+1)*sizeof(char *));
        }
        entry->NumberOfTemplateParameters++;
        entry->TemplateDefaults[j] = NULL;

        m = skip_expression(&line[i], delims);
        while (m > 0 && (line[i+m-1] == ' ' || line[i+m-1] == '\t'))
        {
          --m;
        }

        entry->TemplateParameters[j] =
          vtkParse_CacheString(info->Strings, &line[i], m);
        i += m;
        i += skip_space(&line[i]);

        if (line[i] == '=')
        {
          i++;
          i += skip_space(&line[i]);
          m = skip_expression(&line[i], delims);
          while (m > 0 && (line[i+m-1] == ' ' || line[i+m-1] == '\t'))
          {
            --m;
          }
          entry->TemplateDefaults[j] =
            vtkParse_CacheString(info->Strings, &line[i], m);
          i += m;
          i += skip_space(&line[i]);
        }

        if (line[i] == ',')
        {
          i++;
          i += skip_space(&line[i]);
        }
      }

      if (line[i] == '>')
      {
        i++;
        i += skip_space(&line[i]);
      }

      if (line[i] == ':' && line[i+1] == ':')
      {
        i += 2;
        m = vtkParse_NameLength(&line[i]);
        n = strlen(entry->Name);
        cp = vtkParse_NewString(info->Strings, n+m+2);
        strcpy(cp, entry->Name);
        strcpy(&cp[n], "::");
        strncpy(&cp[n+2], &line[i], m);
        i += m;
        cp[n+m+2] = '\0';
        entry->Name = cp;
      }
    }

    i += skip_space(&line[i]);

    /* classes (and possibly enums) */
    if (line[i] == ':')
    {
      i++;
      i += skip_space(&line[i]);
      n = vtkParse_NameLength(&line[i]);
      /* check for enum indicators */
      if ((n == 3 && strncmp(&line[i], "int", n) == 0) ||
          (n == 4 && strncmp(&line[i], "enum", n) == 0))
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
        n = vtkParse_NameLength(&line[i]);
        entry->SuperClasses[j] =
          vtkParse_CacheString(info->Strings, &line[i], n);
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
        ccp = vtkParse_CacheString(info->Strings, &line[i], n);
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
      i += vtkParse_BasicTypeFromString(&line[i], &bits, &ccp, &n);
      entry->Typedef->Class = vtkParse_CacheString(info->Strings, ccp, n);
      entry->Typedef->Type |= bits;
    }

    /* get the header file */
    if (line[i] == ';')
    {
      i++;
      i += skip_space(&line[i]);
      n = 0;
      while(line[i+n] != '\0' && line[i+n] != ';' &&
            !isspace(line[i+n])) { n++; };
      entry->HeaderFile = vtkParse_CacheString(info->Strings, &line[i], n);

      i += n;
      i += skip_space(&line[i]);

      /* get the module */
      if (line[i] == ';')
      {
        i++;
        i += skip_space(&line[i]);
        n = 0;
        while(line[i+n] != '\0' && line[i+n] != ';' &&
              !isspace(line[i+n])) { n++; };
        entry->Module = vtkParse_CacheString(info->Strings, &line[i], n);

        i += n;
        i += skip_space(&line[i]);
      }

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
          entry->Properties[entry->NumberOfProperties++] =
            vtkParse_CacheString(info->Strings, &line[i], n);
        }
        i += n;
      }
    }
  }

  free(line);

  if (!feof(fp))
  {
    success = 0;
  }

  fclose(fp);

  return success;
}

/* free a HierarchyInfo struct */
void vtkParseHierarchy_Free(HierarchyInfo *info)
{
  HierarchyEntry *entry;
  int i;

  for (i = 0; i < info->NumberOfEntries; i++)
  {
    entry = &info->Entries[i];
    if (entry->NumberOfTemplateParameters)
    {
      free((char **)entry->TemplateParameters);
      free((char **)entry->TemplateDefaults);
    }
    if (entry->NumberOfSuperClasses)
    {
      free((char **)entry->SuperClasses);
      free(entry->SuperClassIndex);
    }
    if (entry->NumberOfProperties)
    {
      free((char **)entry->Properties);
    }
  }

  free(info->Entries);
  free(info);
}


/* Check whether class is derived from baseclass.  You must supply
 * the entry for the class (returned by FindEntry) as well as the
 * classname.  If the class is templated, the classname can include
 * template args in angle brackets.  If you provide a pointer for
 * baseclass_with_args, then it will be used to return the name of
 * name of the baseclass with template args in angle brackets. */

int vtkParseHierarchy_IsTypeOfTemplated(
  const HierarchyInfo *info,
  const HierarchyEntry *entry, const char *classname,
  const char *baseclass, const char **baseclass_with_args)
{
  HierarchyEntry *tmph;
  const char *name = NULL;
  const char *supername;
  char *tmp;
  int templated;
  int baseclass_is_template_parameter;
  int supername_needs_free = 0;
  int classname_needs_free = 0;
  int i, j, k;
  int nargs;
  const char **args;
  size_t m;
  int iterating = 1;
  int rval = 0;

  while (iterating)
  {
    iterating = 0;
    templated = 0;
    baseclass_is_template_parameter = 0;
    nargs = 0;
    args = NULL;

    /* if classname is the same as baseclass, done! */
    if (strcmp(entry->Name, baseclass) == 0)
    {
      if (baseclass_with_args)
      {
        if (!classname_needs_free)
        {
          tmp = (char *)malloc(strlen(classname) + 1);
          strcpy(tmp, classname);
          classname = tmp;
        }
        *baseclass_with_args = classname;
        classname_needs_free = 0;
      }
      rval = 1;
      break;
    }
    else if (entry->NumberOfSuperClasses == 0)
    {
      rval = 0;
      break;
    }

    /* if class is templated */
    if (entry->NumberOfTemplateParameters)
    {
      /* check for template args for classname */
      m = strlen(entry->Name);
      if (classname[m] == '<')
      {
        templated = 1;

        nargs = entry->NumberOfTemplateParameters;
        vtkParse_DecomposeTemplatedType(classname, &name, nargs, &args,
          entry->TemplateDefaults);
      }
    }

    /* check all baseclasses */
    for (j = 0; j < entry->NumberOfSuperClasses && rval == 0; j++)
    {
      supername = entry->SuperClasses[j];

      if (templated)
      {
        for (k = 0; k < entry->NumberOfTemplateParameters; k++)
        {
          /* check if the baseclass itself is a template parameter */
          m = strlen(entry->TemplateParameters[k]);
          if (strncmp(entry->TemplateParameters[k], supername, m) == 0 &&
              !isalnum(supername[m]) && supername[m] != '_')
          {
            baseclass_is_template_parameter = 1;
            break;
          }
        }

        /* use the class template args to find baseclass template args */
        supername = vtkParse_StringReplace(
          supername, entry->NumberOfTemplateParameters, entry->TemplateParameters, args);
        if (supername != entry->SuperClasses[j])
        {
          supername_needs_free = 1;
        }
      }

      /* check the cached index for the baseclass entry */
      i = entry->SuperClassIndex[j];
      if (i == -1)
      {
        /* index was not set yet, so search for the entry */
        tmph = vtkParseHierarchy_FindEntry(info, supername);
        while (tmph && tmph->IsTypedef)
        {
          if (tmph->Typedef->Class)
          {
            tmph = vtkParseHierarchy_FindEntry(info, tmph->Typedef->Class);
            continue;
          }
          break;
        }

        if (tmph)
        {
          i = (int)(tmph - info->Entries);
        }
        else
        {
          /* entry not found, don't try again */
          /* i = -2; messes things up for templates */
          /* fprintf(stderr, "not found \"%s\"\n", entry->SuperClasses[j]); */
        }

        /* if baseclass is a template parameter, its entry cannot be cached */
        if (!baseclass_is_template_parameter)
        {
          /* cache the position of the baseclass */
          ((HierarchyEntry *)entry)->SuperClassIndex[j] = i;
        }
      }

      /* if entry was found, continue down the chain */
      if (i >= 0)
      {
        if (classname_needs_free)
        {
          free((char *)classname);
        }
        classname = supername;
        classname_needs_free = supername_needs_free;
        supername_needs_free = 0;

        /* use the iteration loop instead of recursion */
        if (j+1 >= entry->NumberOfSuperClasses)
        {
          entry = &info->Entries[i];
          iterating = 1;
        }

        /* recurse for multiple inheritance */
        else
        {
          rval = vtkParseHierarchy_IsTypeOfTemplated(
                   info, &info->Entries[i], classname, baseclass,
                   baseclass_with_args);
        }
      }

      if (supername_needs_free)
      {
        free((char *)supername);
        supername_needs_free = 0;
      }

    } /* end of loop over superclasses */

    if (templated)
    {
      vtkParse_FreeTemplateDecomposition(name, nargs, args);
    }

  } /* end of "while (iterating)" */

  if (classname_needs_free)
  {
    free((char *)classname);
  }

  if (baseclass_with_args && !rval)
  {
    *baseclass_with_args = NULL;
  }

  return rval;
}

int vtkParseHierarchy_IsTypeOf(
  const HierarchyInfo *info, const HierarchyEntry *entry,
  const char *baseclass)
{
  return vtkParseHierarchy_IsTypeOfTemplated(
    info, entry, entry->Name, baseclass, NULL);
}

/* Free args returned by IsTypeOfTemplated */
void vtkParseHierarchy_FreeTemplateArgs(int n, const char *args[])
{
  int i;

  for (i = 0; i < n; i++)
  {
    free((char *)args[i]);
  }

  free((char **)args);
}

/* Given a classname with template parameters, get the superclass name
 * with corresponding template parameters.  Returns null if 'i' is out
 * of range, i.e. greater than or equal to the number of superclasses.
 * The returned classname must be freed with "free()". */
const char *vtkParseHierarchy_TemplatedSuperClass(
  const HierarchyEntry *entry, const char *classname, int i)
{
  const char *supername = NULL;
  const char *name;
  const char **args;
  char *cp;
  size_t j;

  if (i < entry->NumberOfSuperClasses)
  {
    supername = entry->SuperClasses[i];
    j = vtkParse_IdentifierLength(classname);

    if (classname[j] == '<')
    {
      vtkParse_DecomposeTemplatedType(classname, &name,
        entry->NumberOfTemplateParameters, &args, entry->TemplateDefaults);
      supername = vtkParse_StringReplace(entry->SuperClasses[i],
        entry->NumberOfTemplateParameters, entry->TemplateParameters, args);
      vtkParse_FreeTemplateDecomposition(
        name, entry->NumberOfTemplateParameters, args);
    }

    if (supername == entry->SuperClasses[i])
    {
      cp = (char *)malloc(strlen(supername) + 1);
      strcpy(cp, supername);
      supername = cp;
    }
  }

  return supername;
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
      k = vtkParse_NameLength(entry->Properties[i]);
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

/* Expand all unrecognized types in a ValueInfo struct by
 * using the typedefs in the HierarchyInfo struct. */
int vtkParseHierarchy_ExpandTypedefsInValue(
  const HierarchyInfo *info, ValueInfo *val, StringCache *cache,
  const char *scope)
{
  char text[128];
  char *cp;
  const char *newclass;
  size_t n, m;
  int i;
  HierarchyEntry *entry;
  int scope_needs_free = 0;
  int result = 1;

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
      /* only malloc if more than 128 chars needed */
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

      if (cp != text) { free(cp); }

      /* if not found, try inherited scopes */
      if (entry == 0)
      {
        entry = vtkParseHierarchy_FindEntry(info, scope);
        scope = 0;
        scope_needs_free = 0;
        if (entry && entry->NumberOfSuperClasses)
        {
          for (i = 0; i+1 < entry->NumberOfSuperClasses; i++)
          {
            if (scope_needs_free) { free((char *)scope); }
            scope = vtkParseHierarchy_ExpandTypedefsInName(
              info, entry->SuperClasses[i], NULL);
            scope_needs_free = (scope != entry->SuperClasses[i]);
            /* recurse if more than one superclass */
            if (vtkParseHierarchy_ExpandTypedefsInValue(
                  info, val, cache, scope))
            {
              if (scope_needs_free) { free((char *)scope); }
              return 1;
            }
          }
          if (scope_needs_free) { free((char *)scope); }
          scope = vtkParseHierarchy_ExpandTypedefsInName(
            info, entry->SuperClasses[i], NULL);
          scope_needs_free = (scope != entry->SuperClasses[i]);
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
      newclass = vtkParseHierarchy_ExpandTypedefsInName(
         info, val->Class, scope);
      if (newclass != val->Class)
      {
        val->Class = vtkParse_CacheString(cache, newclass, strlen(newclass));
        free((char *)newclass);
      }
      result = 1;
      break;
    }
    else
    {
      result = 0;
      break;
    }
  }

  if (scope_needs_free) { free((char *)scope); }

  return result;
}

/* Expand typedefs found in an expression stored as a string.
 * The value of "text" will be returned if no expansion occurred,
 * else a new string is returned that must be freed with "free()". */
const char *vtkParseHierarchy_ExpandTypedefsInName(
  const HierarchyInfo *info, const char *name, const char *scope)
{
  char text[128];
  char *cp;
  size_t n, m;
  const char *newname = name;
  HierarchyEntry *entry = NULL;

  /* note: unlike ExpandTypedefsInValue, this does not yet recurse
   * or look in superclass scopes */

  /* doesn't yet handle names that are scoped or templated */
  m = vtkParse_IdentifierLength(name);
  if (name[m] != '\0')
  {
    return name;
  }

  if (scope)
  {
    cp = text;
    n = strlen(scope);
    m = strlen(name);
    /* only malloc if more than 128 chars needed */
    if (n + m + 2 >= 128)
    {
      cp = (char *)malloc(n+m+3);
    }

    /* scope the name */
    strncpy(cp, scope, n);
    cp[n++] = ':';
    cp[n++] = ':';
    strncpy(&cp[n], name, m);
    cp[n+m] = '\0';

    entry = vtkParseHierarchy_FindEntry(info, cp);

    if (cp != text) { free(cp); }
  }

  if (!entry)
  {
    entry = vtkParseHierarchy_FindEntry(info, name);
  }

  newname = NULL;
  if (entry && entry->IsTypedef && entry->Typedef->Class)
  {
    newname = entry->Typedef->Class;
  }
  if (newname)
  {
    cp = (char *)malloc(strlen(newname) + 1);
    strcpy(cp, newname);
    name = cp;
  }

  return name;
}

/* -------------------------------------------------------------------- */
const char *vtkParseHierarchy_QualifiedEnumName(
  HierarchyInfo *hinfo, ClassInfo *data, StringCache *cache,
  const char *name)
{
  // check to see if this is an enum defined in the class
  if (data)
  {
    int j;
    for (j = 0; j < data->NumberOfEnums; j++)
    {
      EnumInfo *info = data->Enums[j];
      if (name && info->Name && strcmp(name, info->Name) == 0)
      {
        char *scoped_name;
        size_t scoped_len = strlen(data->Name) + strlen(info->Name) + 2;
        scoped_name = vtkParse_NewString(cache, scoped_len);
        sprintf(scoped_name, "%s::%s", data->Name, info->Name);
        return scoped_name;
      }
    }
  }

  // check the hierarchy information for the enum type
  if (hinfo)
  {
    HierarchyEntry *entry;
    entry = vtkParseHierarchy_FindEntry(hinfo, name);
    if (entry && entry->IsEnum)
    {
      return name;
    }
  }

  return NULL;
}
