// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkWrapPythonProperty.h"
#include "vtkParseData.h"
#include "vtkParseHierarchy.h"
#include "vtkParseProperties.h"
#include "vtkWrapPythonMethodDef.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

/* Regex-less algorithm to convert from PascalCase to snake_case
 * Caller must make sure to free the memory of the returned pointer after use. */
static char* vtkWrapPython_ConvertPascalToSnake(const char* pascalCase)
{
  size_t i;
  size_t pascalLen = strlen(pascalCase);
  /* Max length is 2 times the input length (underscores added between words) */
  char* snakeCase = (char*)malloc((2 * pascalLen) + 1);
  size_t snakeIndex = 0;

  /* Convert the first character to lowercase */
  snakeCase[snakeIndex++] = tolower(pascalCase[0]);

  for (i = 1; i < pascalLen; i++)
  {
    char currentChar = pascalCase[i];

    /* Begin a new word only if
     *     1. the current character is uppercase
     * and 2. the current character follows a lowercase character
     * or  3. the current character is followed by a lowercase character */
    if (isupper(currentChar))
    {
      if (islower(pascalCase[i - 1]) || (i + 1 < pascalLen && islower(pascalCase[i + 1])))
      {
        snakeCase[snakeIndex++] = '_';
      }
      snakeCase[snakeIndex++] = tolower(currentChar);
    }
    else
    {
      snakeCase[snakeIndex++] = currentChar;
    }
  }

  /* Null-terminate the snake_case string */
  snakeCase[snakeIndex] = '\0';

  return snakeCase;
}

/* Helper used to construct a single PyGetSetDef item corresponding to a VTK property */
typedef struct
{
  const char* Name;
  char* SnakeName;
  int HasGetter;
  int HasSetter;
  int HasMultiSetter;
} GetSetDefInfo;

/* Returns a new zero-filled GetSetDefInfo, increments the count.
 * - arraymem: pointer to the array used for storage (pass-by-reference)
 * - count: the number of used values in the array (pass-by-reference) */
static GetSetDefInfo* vtkWrapPython_NewGetSet(GetSetDefInfo*** arraymem, int* count)
{
  int n = *count;
  if (n == 0)
  {
    /* if empty, allocate for the first time */
    *arraymem = (GetSetDefInfo**)malloc(sizeof(GetSetDefInfo*));
  }
  else if ((n & (n - 1)) == 0) /* power-of-two check */
  {
    /* if count is power of two, reallocate with double size */
    *arraymem = (GetSetDefInfo**)realloc(*arraymem, (n << 1) * sizeof(GetSetDefInfo*));
  }

  /* allocate and initialize one element, then return it */
  (*arraymem)[*count] = (GetSetDefInfo*)calloc(1, sizeof(GetSetDefInfo));
  return (*arraymem)[(*count)++];
}

/* Search for existing GetSetDefInfo, or return a new one if not found */
static GetSetDefInfo* vtkWrapPython_FindGetSet(
  const char* name, GetSetDefInfo*** arraymem, int* count)
{
  GetSetDefInfo* item;
  int i;

  for (i = 0; i < *count; ++i)
  {
    if (strcmp(name, (*arraymem)[i]->Name) == 0)
    {
      return (*arraymem)[i];
    }
  }

  item = vtkWrapPython_NewGetSet(arraymem, count);
  item->Name = name;
  item->SnakeName = vtkWrapPython_ConvertPascalToSnake(name);
  return item;
}

/* Returns true if the method can be used inside the get member of PyGetSetDef */
static int vtkWrapPython_IsGetter(const unsigned int methodType)
{
  return methodType == VTK_METHOD_GET;
}

/* Returns true if the method can be used inside the set member of PyGetSetDef */
static int vtkWrapPython_IsSetter(const unsigned int methodType)
{
  return methodType == VTK_METHOD_SET || methodType == VTK_METHOD_SET_MULTI;
}

/* Returns true if the setter method takes multiple arguments e.g. SetPoint(x,y,z) */
static int vtkWrapPython_IsMultiSetter(const unsigned int methodType)
{
  return methodType == VTK_METHOD_SET_MULTI;
}

/* Calls vtkWrapPython_MethodCheck to figure out the wrappability of the method. */
static int vtkWrapPython_IsWrappable(
  const ClassInfo* classInfo, FunctionInfo* functionInfo, const HierarchyInfo* hinfo)
{
  const int isWrappable = vtkWrapPython_MethodCheck(classInfo, functionInfo, hinfo);
  return isWrappable;
}

/* print out all properties in the getset table. */
void vtkWrapPython_GenerateProperties(FILE* fp, const char* classname, ClassInfo* classInfo,
  const HierarchyInfo* hinfo, ClassProperties* properties, int is_vtkobject)
{
  int i, j;
  int propCount = 0;
  GetSetDefInfo* getSetInfo = NULL;
  GetSetDefInfo** getSetsInfo = NULL;
  FunctionInfo* theFunc = NULL;

  /* Populate the table of property methods */
  for (i = 0; i < classInfo->NumberOfFunctions; ++i)
  {
    theFunc = classInfo->Functions[i];
    /* Ignore unwrappable methods */
    if (!vtkWrapPython_IsWrappable(classInfo, theFunc, hinfo))
    {
      continue;
    }
    /* Is this method associated with a property? */
    if (properties->MethodHasProperty[i])
    {
      int isGetter = vtkWrapPython_IsGetter(properties->MethodTypes[i]);
      int isSetter = vtkWrapPython_IsSetter(properties->MethodTypes[i]);
      if ((isGetter || isSetter) && strlen(theFunc->Name) > 3)
      {
        getSetInfo = vtkWrapPython_FindGetSet(&theFunc->Name[3], &getSetsInfo, &propCount);
        getSetInfo->HasGetter |= isGetter;
        getSetInfo->HasSetter |= isSetter;
        getSetInfo->HasMultiSetter |= vtkWrapPython_IsMultiSetter(properties->MethodTypes[i]);
      }
    }
  }

  if (propCount > 0)
  {
    /* generate a table of the class getter/setter methods */
    fprintf(fp, "static PyVTKGetSet Py%s_GetSetMethods[] = {\n", classname);

    for (j = 0; j < propCount; ++j)
    {
      getSetInfo = getSetsInfo[j];
      if (getSetInfo->HasGetter && !getSetInfo->HasSetter)
      {
        fprintf(fp, "  { Py%s_Get%s, nullptr },\n", classname, getSetInfo->Name);
      }
      else if (!getSetInfo->HasGetter && getSetInfo->HasSetter)
      {
        fprintf(fp, "  { nullptr, Py%s_Set%s },\n", classname, getSetInfo->Name);
      }
      else
      {
        fprintf(fp, "  { Py%s_Get%s, Py%s_Set%s },\n", classname, getSetInfo->Name, classname,
          getSetInfo->Name);
      }
    }

    fprintf(fp, "};\n\n");
  }

  /* useful macro for Python 3.6 and earlier */
  fprintf(fp,
    "#if PY_VERSION_HEX >= 0x03070000\n"
    "#define pystr(x) x\n"
    "#else\n"
    "#define pystr(x) const_cast<char*>(x)\n"
    "#endif\n"
    "\n");

  /* start the PyGetSetDef for this class */
  fprintf(fp, "static PyGetSetDef Py%s_GetSets[] = {\n", classname);

  /* these properties are available to all vtk object types. */
  if (is_vtkobject)
  {
    for (i = 0; i < 2; ++i)
    {
      fprintf(fp, "  PyVTKObject_GetSet[%d],\n", i);
    }
  }

  for (j = 0; j < propCount; ++j)
  {
    getSetInfo = getSetsInfo[j];

    /* Start a new PyGetSetDef item */
    fprintf(fp, "  {\n");
    fprintf(fp, "    pystr(\"%s\"), // name\n", getSetInfo->SnakeName);

    /* The getter and setter */
    if (getSetInfo->HasGetter)
    {
      fprintf(fp, "    PyVTKObject_GetProperty, // get\n");
    }
    else
    {
      fprintf(fp, "    nullptr, // get\n");
    }
    if (getSetInfo->HasMultiSetter)
    {
      fprintf(fp, "    PyVTKObject_SetPropertyMulti, // set\n");
    }
    else if (getSetInfo->HasSetter)
    {
      fprintf(fp, "    PyVTKObject_SetProperty, // set\n");
    }
    else
    {
      fprintf(fp, "    nullptr, // set\n");
    }

    /* Define the doc string */
    if (getSetInfo->HasGetter && !getSetInfo->HasSetter)
    {
      fprintf(fp, "    pystr(\"read-only, calls Get%s\\n\"), // doc\n", getSetInfo->Name);
    }
    else if (!getSetInfo->HasGetter && getSetInfo->HasSetter)
    {
      fprintf(fp, "    pystr(\"write-only, calls Set%s\\n\"), // doc\n", getSetInfo->Name);
    }
    else
    {
      fprintf(fp, "    pystr(\"read-write, calls Get%s/Set%s\\n\"), // doc\n", getSetInfo->Name,
        getSetInfo->Name);
    }
    /* closure provides the methods that we call */
    fprintf(fp, "    &Py%s_GetSetMethods[%d], // closure\n", classname, j);
    /* Finish the definition of a PyGetSetDef entry */
    fprintf(fp, "  },\n");
  }

  /* add sentinel entry */
  fprintf(fp, "  { nullptr, nullptr, nullptr, nullptr, nullptr }\n");
  fprintf(fp, "};\n\n");

  /* clean up memory of data structures */
  for (j = 0; j < propCount; ++j)
  {
    if (getSetsInfo[j])
    {
      free(getSetsInfo[j]->SnakeName);
      free(getSetsInfo[j]);
    }
  }
  free(getSetsInfo);
}
