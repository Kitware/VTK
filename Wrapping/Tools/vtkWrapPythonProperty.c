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

/* Helper used to construct a single PyGetSetDef item corresponding to a VTK property */
typedef struct
{
  char* PropertyName;
  int HasGetter;
  int HasSetter;
  int HasMultiSetter;
} GetSetDefInfo;

/* print out all properties in the getset table. */
void vtkWrapPython_GenerateProperties(FILE* fp, const char* classname, ClassInfo* classInfo,
  const HierarchyInfo* hinfo, ClassProperties* properties, int is_vtkobject)
{
  int i, j;
  int propCount;
  GetSetDefInfo* getSetInfo = NULL;
  GetSetDefInfo** getSetsInfo = NULL;
  FunctionInfo* theFunc = NULL;
  const PropertyInfo* theProp = NULL;
  const char* propName = NULL;
  char* snakeCasePropName;

  getSetsInfo = (GetSetDefInfo**)calloc(properties->NumberOfProperties, sizeof(GetSetDefInfo*));

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
      /* Get the property associated with this method */
      j = properties->MethodProperties[i];
      propName = properties->Properties[j]->Name;
      if (propName != NULL)
      {
        /* Encountering this property for the first time. */
        if (getSetsInfo[j] == NULL)
        {
          getSetsInfo[j] = (GetSetDefInfo*)calloc(1, sizeof(GetSetDefInfo));
        }
        /* Update the methods, method types for this property. */
        getSetInfo = getSetsInfo[j];
        getSetInfo->HasGetter |= vtkWrapPython_IsGetter(properties->MethodTypes[i]);
        getSetInfo->HasSetter |= vtkWrapPython_IsSetter(properties->MethodTypes[i]);
        getSetInfo->HasMultiSetter |= vtkWrapPython_IsMultiSetter(properties->MethodTypes[i]);
      }
    }
  }

  /* Count the usable properties */
  propCount = 0;
  for (j = 0; j < properties->NumberOfProperties; ++j)
  {
    getSetInfo = getSetsInfo[j];
    if (!getSetInfo || (!getSetInfo->HasGetter && !getSetInfo->HasSetter))
    {
      continue;
    }

    theProp = properties->Properties[j];
    snakeCasePropName = vtkWrapPython_ConvertPascalToSnake(theProp->Name);
    getSetInfo->PropertyName = snakeCasePropName;
    ++propCount;
  }

  if (propCount > 0)
  {
    /* generate a table of the class getter/setter methods */
    fprintf(fp, "static PyVTKGetSet Py%s_GetSetMethods[] = {\n", classname);

    for (j = 0; j < properties->NumberOfProperties; ++j)
    {
      getSetInfo = getSetsInfo[j];
      if (!getSetInfo)
      {
        continue;
      }

      snakeCasePropName = getSetInfo->PropertyName;
      if (!snakeCasePropName)
      {
        continue;
      }

      theProp = properties->Properties[j];

      if (getSetInfo->HasGetter && !getSetInfo->HasSetter)
      {
        fprintf(fp, "  { Py%s_Get%s, nullptr },\n", classname, theProp->Name);
      }
      else if (!getSetInfo->HasGetter && getSetInfo->HasSetter)
      {
        fprintf(fp, "  { nullptr, Py%s_Set%s },\n", classname, theProp->Name);
      }
      else
      {
        fprintf(fp, "  { Py%s_Get%s, Py%s_Set%s },\n", classname, theProp->Name, classname,
          theProp->Name);
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

  propCount = 0;
  for (j = 0; j < properties->NumberOfProperties; ++j)
  {
    getSetInfo = getSetsInfo[j];
    if (!getSetInfo)
    {
      continue;
    }

    snakeCasePropName = getSetInfo->PropertyName;
    if (!snakeCasePropName)
    {
      continue;
    }

    theProp = properties->Properties[j];

    /* Start a new PyGetSetDef item */
    fprintf(fp, "  {\n");
    fprintf(fp, "    pystr(\"%s\"), // name\n", snakeCasePropName);

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
      fprintf(fp, "    pystr(\"read-only, calls Get%s\\n\"), // doc\n", theProp->Name);
    }
    else if (!getSetInfo->HasGetter && getSetInfo->HasSetter)
    {
      fprintf(fp, "    pystr(\"write-only, calls Set%s\\n\"), // doc\n", theProp->Name);
    }
    else
    {
      fprintf(fp, "    pystr(\"read-write, calls Get%s/Set%s\\n\"), // doc\n", theProp->Name,
        theProp->Name);
    }
    /* closure provides the methods that we call */
    fprintf(fp, "    &Py%s_GetSetMethods[%d], // closure\n", classname, propCount++);
    /* Finish the definition of a PyGetSetDef entry */
    fprintf(fp, "  },\n");
  }

  /* add sentinel entry */
  fprintf(fp, "  { nullptr, nullptr, nullptr, nullptr, nullptr }\n");
  fprintf(fp, "};\n\n");

  /* clean up memory of data structures */
  for (j = 0; j < properties->NumberOfProperties; ++j)
  {
    if (getSetsInfo[j])
    {
      free(getSetsInfo[j]->PropertyName);
      free(getSetsInfo[j]);
    }
  }
  free(getSetsInfo);
}
