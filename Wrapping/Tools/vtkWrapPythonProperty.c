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

/* Regex-less algorithm to convert from PascalCase to snake_case */
// Don't have regex because vtksys is not linked to the wrapping module
// and regex.h is limited to POSIX systems.
// Caller must make sure to free the memory of the returned pointer after use.
static char* vtkWrapPython_ConvertPascalToSnake(const char* pascalCase)
{
  size_t i;
  if (pascalCase == NULL || *pascalCase == '\0')
  {
    return NULL; // Handle empty input
  }

  size_t pascalLen = strlen(pascalCase);
  // Max length is 2 times the input length (underscores added between words)
  char* snakeCase = (char*)malloc((2 * pascalLen) + 1);
  if (snakeCase == NULL)
  {
    return NULL; // Memory allocation failed
  }

  size_t snakeIndex = 0;

  // Convert the first character to lowercase
  snakeCase[snakeIndex++] = tolower(pascalCase[0]);

  for (i = 1; i < pascalLen; i++)
  {
    char currentChar = pascalCase[i];

    // Begin a new world only if
    //     1. the current character is uppercase
    // and 2. the current character follows a lowercase character
    // or  3. the current character is followed by a lowercase character
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

  // Null-terminate the snake_case string
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

/* Calls vtkWrapPython_MethodCheck to figure out the wrappability of the method. */
static int vtkWrapPython_IsWrappable(
  const ClassInfo* classInfo, FunctionInfo* functionInfo, const HierarchyInfo* hinfo)
{
  // TODO: Figure out a cleaner way for vtkWrapPython_GenerateOneMethod to ignore an occurrence
  // instead of it erasing the function name.
  int restoreFuncName = 0;
  if (functionInfo->Name == NULL)
  {
    functionInfo->Name = "Placeholder";
    restoreFuncName = 1;
  }
  const int isWrappable = vtkWrapPython_MethodCheck(classInfo, functionInfo, hinfo);
  if (restoreFuncName)
  {
    functionInfo->Name = NULL;
  }
  return isWrappable;
}

/* Helper used to construct a single PyGetSetDef item corresponding to a VTK property */
typedef struct
{
  int HasGetter;
  int HasSetter;
} GetSetDefInfo;

/* print out all properties in the getset table. */
void vtkWrapPython_GenerateProperties(FILE* fp, const char* classname, ClassInfo* classInfo,
  const HierarchyInfo* hinfo, ClassProperties* properties, int is_vtkobject)
{
  int i, j;
  GetSetDefInfo* getSetInfo = NULL;
  GetSetDefInfo** getSetsInfo = NULL;
  FunctionInfo* theFunc = NULL;
  const PropertyInfo* theProp = NULL;
  const char* propName = NULL;

  fprintf(fp,
    "#if PY_VERSION_HEX >= 0x03070000\n"
    "#define pystr(x) x\n"
    "#else\n"
    "#define pystr(x) const_cast<char*>(x)\n"
    "#endif\n");
  fprintf(fp, "static PyGetSetDef Py%s_GetSets[] = {\n", classname);
  /* these properties are available to all vtk object types. */
  if (is_vtkobject)
  {
    for (i = 0; i < 2; ++i)
    {
      fprintf(fp, "  PyVTKObject_GetSet[%d],\n", i);
    }
  }
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
      }
    }
  }

  for (j = 0; j < properties->NumberOfProperties; ++j)
  {
    getSetInfo = getSetsInfo[j];
    theProp = properties->Properties[j];
    if ((getSetInfo == NULL) || (!getSetInfo->HasGetter && !getSetInfo->HasSetter))
    {
      continue;
    }

    /* Start a new PyGetSetDef item */
    char* snakeCasePropName = vtkWrapPython_ConvertPascalToSnake(theProp->Name);
    if (!snakeCasePropName)
    {
      continue;
    }

    fprintf(fp, "  {\n");
    fprintf(fp, "    /*name=*/pystr(\"%s\"),\n", snakeCasePropName);
    free(snakeCasePropName);

    /* Write getter code block */
    if (getSetInfo->HasGetter)
    {
      fprintf(fp, "    /*get=*/[](PyObject* self, void*) -> PyObject*\n");
      fprintf(fp, "    {\n");
      fprintf(fp, "      auto args = PyTuple_New(0); // placeholder\n");
      fprintf(fp, "      auto result = Py%s_Get%s(self, args);\n", classname, theProp->Name);
      fprintf(fp, "      Py_DECREF(args);\n");
      fprintf(fp, "      return result;\n");
      fprintf(fp, "    },\n");
    }
    else
    {
      fprintf(fp, "    /*get=*/nullptr,\n");
    }
    /* Write setter code block */
    if (getSetInfo->HasSetter)
    {
      fprintf(fp, "    /*set=*/[](PyObject* self, PyObject* value, void*) -> int\n");
      fprintf(fp, "    {\n");
      // either value is a tuple
      fprintf(fp, "      if (PyTuple_Check(value))\n");
      fprintf(fp, "      {\n");
      fprintf(fp, "        auto result = Py%s_Set%s(self, value);\n", classname, theProp->Name);
      fprintf(fp, "        return (result == nullptr) ? -1 : 0;\n");
      fprintf(fp, "      }\n");
      // or it's just a single argument.
      fprintf(fp, "      else\n");
      fprintf(fp, "      {\n");
      // wrapped setter method still expects a tuple
      fprintf(fp, "        auto args = PyTuple_Pack(1, value);\n");
      fprintf(fp, "        auto result = Py%s_Set%s(self, args);\n", classname, theProp->Name);
      fprintf(fp, "        Py_DECREF(args);\n");
      fprintf(fp, "        return (result == nullptr) ? -1 : 0;\n");
      fprintf(fp, "      }\n");
      fprintf(fp, "    },\n");
    }
    else
    {
      fprintf(fp, "    /*set=*/nullptr,\n");
    }

    /* Define the doc string */
    fprintf(fp, "    /*doc=*/");
    if (getSetInfo->HasGetter && !getSetInfo->HasSetter)
    {
      fprintf(fp, "pystr(\"read-only, Calls Get%s\\n\"),\n", theProp->Name);
    }
    else if (!getSetInfo->HasGetter && getSetInfo->HasSetter)
    {
      fprintf(fp, "pystr(\"write-only, Calls Set%s\\n\"),\n", theProp->Name);
    }
    else
    {
      fprintf(fp, "pystr(\"read-write, Calls Get%s/Set%s\\n\"),\n", theProp->Name, theProp->Name);
    }
    /* closure is nullptr, we do not rely upon it */
    fprintf(fp, "    /*closure=*/nullptr,\n");
    /* Finish the definition of a PyGetSetDef entry */
    fprintf(fp, "  },\n");
  }

  /* add sentinel entry */
  fprintf(fp, "  { nullptr, nullptr, nullptr, nullptr, nullptr }\n");
  fprintf(fp, "};\n");
  /* clean up memory of data structures */
  for (j = 0; j < properties->NumberOfProperties; ++j)
  {
    free(getSetsInfo[j]);
  }
  free(getSetsInfo);
}
