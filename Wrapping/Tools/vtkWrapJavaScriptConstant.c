/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWrapJavaScriptConstant.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkWrapJavaScriptConstant.h"
#include "vtkParseData.h"
#include "vtkWrap.h"
#include "vtkWrapText.h"

#include <string.h>

/* -------------------------------------------------------------------- */
/* The "attrib" is the attribute to set in the module, if null then
   val->Name is used as the attribute name.

   The "attribval" is the value to set the attribute to, if null then
   val->Value is used.
*/
void vtkWrapJavaScript_AddConstantHelper(
  FILE* fp, const char* indent, const char* attrib, const char* attribval, ValueInfo* val)
{
  unsigned int valtype;
  const char* valstring;

  valtype = (val->Type & VTK_PARSE_UNQUALIFIED_TYPE);
  valstring = attribval;
  if (valstring == 0)
  {
    valstring = val->Value;
  }

  if (valtype == 0 && (valstring == NULL || valstring[0] == '\0'))
  {
    valtype = VTK_PARSE_VOID;
  }
  else if (strcmp(valstring, "nullptr") == 0)
  {
    valtype = VTK_PARSE_VOID;
  }

  if (valtype == 0 || val->Name == NULL)
  {
    return;
  }
  char key[512];
  if (!attrib)
  {
    snprintf(key, sizeof(key), "\"%s\"", val->Name);
  }
  else
  {
    snprintf(key, sizeof(key), "%s", attrib);
  }
  if (val->IsEnum)
  {
    if (val->Class && val->Class[0] != '\0' && strcmp(val->Class, "int") != 0)
    {
      fprintf(fp, "%semscripten::constant(%s, static_cast<int>(%s));\n", indent, key, valstring);
    }
    else
    {
      fprintf(fp, "%semscripten::constant(%s, %s);\n", indent, key, valstring);
    }
  }
  else
  {
    switch (valtype)
    {
      case VTK_PARSE_VOID:
        fprintf(fp, "%semscripten::constant(%s, std::string(\"null\"));\n", indent, key);
        break;
      case VTK_PARSE_CHAR_PTR:
        fprintf(fp, "%semscripten::constant(%s, std::string(%s));\n", indent, key, valstring);
        break;
      case VTK_PARSE_FLOAT:
      case VTK_PARSE_DOUBLE:
      case VTK_PARSE_LONG:
      case VTK_PARSE_INT:
      case VTK_PARSE_SHORT:
      case VTK_PARSE_UNSIGNED_SHORT:
      case VTK_PARSE_CHAR:
      case VTK_PARSE_SIGNED_CHAR:
      case VTK_PARSE_UNSIGNED_CHAR:
      case VTK_PARSE_UNSIGNED_INT:
      case VTK_PARSE_UNSIGNED_LONG:
      case VTK_PARSE_LONG_LONG:
      case VTK_PARSE_UNSIGNED_LONG_LONG:
      case VTK_PARSE_BOOL:
        fprintf(fp, "%semscripten::constant(%s, %s);\n", indent, key, valstring);
        break;
    }
  }
}

/* -------------------------------------------------------------------- */
/* Add all constants defined in the namespace to the module */

void vtkWrapJavaScript_GenerateConstants(
  FILE* fp, const char* module, const char* basename, const char* indent, NamespaceInfo* data)
{
  const char* nextindent = "        ";
  ValueInfo* val;
  ValueInfo* firstval;
  const char* scope;
  int scopeType, scopeValue;
  unsigned int valtype;
  const char* typeName;
  const char* tname;
  int j = 0;
  int count, k;
  size_t l, m;

  /* get the next indent to use */
  l = strlen(indent);
  m = strlen(nextindent);
  if (m > l + 2)
  {
    nextindent += m - l - 2;
  }

  /* get the name of the namespace, or NULL if global */
  scope = data->Name;
  if (scope && scope[0] == '\0')
  {
    scope = 0;
  }

  int constBindingBlockId = 0;
  /* go through the constants, collecting them by type */
  while (j < data->NumberOfConstants)
  {
    val = data->Constants[j];
    if (val->Access != VTK_ACCESS_PUBLIC && (val->Attributes & VTK_PARSE_WRAPEXCLUDE) == 0)
    {
      j++;
      continue;
    }

    /* write a single constant if not numerical */
    if (j + 1 == data->NumberOfConstants || val->Type != data->Constants[j + 1]->Type ||
      !vtkWrap_IsScalar(val) || (!val->IsEnum && !vtkWrap_IsNumeric(val)))
    {
      fprintf(fp, "EMSCRIPTEN_BINDINGS(%s_%s_%d_%u_constants) {\n", module,
        (scope ? scope : basename), constBindingBlockId++, data->ItemType);
      vtkWrapJavaScript_AddConstant(fp, indent, val);
      fprintf(fp, "}\n");
      j++;
      continue;
    }

    /* get important information about the value */
    valtype = val->Type;
    typeName = (val->IsEnum ? val->Class : vtkWrap_GetTypeName(val));
    scopeType = (scope && val->IsEnum && strncmp(typeName, "int", 3) != 0);
    scopeValue = (scope && val->IsEnum);

    /* count a series of constants of the same type */
    firstval = val;
    count = 0;
    for (k = j; k < data->NumberOfConstants; k++)
    {
      val = data->Constants[k];
      if (val->Access == VTK_ACCESS_PUBLIC && (val->Attributes & VTK_PARSE_WRAPEXCLUDE) == 0)
      {
        tname = (val->IsEnum ? val->Class : vtkWrap_GetTypeName(val));
        if (val->Type != valtype || strcmp(tname, typeName) != 0)
        {
          break;
        }
        count++;
      }
    }

    /* if no constants to generate, then continue */
    if (count == 0)
    {
      j = k;
      continue;
    }

    if (scopeType)
    {
      int found = 0;

      /* check to make sure that the enum type is wrapped */
      for (int i = 0; i < data->NumberOfEnums && !found; i++)
      {
        EnumInfo* info = data->Enums[i];
        found = (info->IsExcluded && info->Name && strcmp(typeName, info->Name) == 0);
      }
      if (found)
      {
        j = k;
        continue;
      }

      /* check to make sure there won't be a name conflict between an
         enum type and some other class member, it happens specifically
         for vtkImplicitBoolean which has a variable and enum type both
         with the name OperationType */
      for (int i = 0; i < data->NumberOfVariables && !found; i++)
      {
        found = (strcmp(data->Variables[i]->Name, typeName) == 0);
      }
      if (found)
      {
        valtype = VTK_PARSE_INT;
        typeName = "int";
        scopeType = 0;
      }
    }

    /* generate the code */
    fprintf(fp, "EMSCRIPTEN_BINDINGS(%s_%s_%d_%u_constants) {\n", module,
      (scope ? scope : basename), constBindingBlockId++, data->ItemType);
    if (scopeType)
    {
      fprintf(fp, "%s  typedef %s::%s cxx_enum_type;\n\n", indent, scope, typeName);
    }
    fprintf(fp,
      "%sconst struct { const char *name; %s value; }\n"
      "%s  constants[%d] = {\n",
      indent, (scopeType ? "cxx_enum_type" : typeName), indent, count);

    char registeredName[512];
    while (j < k)
    {
      val = data->Constants[j++];
      if (val->Access == VTK_ACCESS_PUBLIC && (val->Attributes & VTK_PARSE_WRAPEXCLUDE) == 0)
      {
        if (scopeType)
        {
          snprintf(
            registeredName, sizeof(registeredName), "%s_%s_%s", scope, val->Class, val->Name);
        }
        else if (scopeValue)
        {
          snprintf(registeredName, sizeof(registeredName), "%s_%s", scope, val->Name);
        }
        else
        {
          snprintf(registeredName, sizeof(registeredName), "%s", val->Name);
        }

        fprintf(fp, "%s    { \"%s%s\", %s%s%s },%s\n", indent, registeredName,
          (vtkWrapText_IsJavaScriptKeyword(registeredName) ? "_" : ""), (scopeValue ? scope : ""),
          (scopeValue ? "::" : ""), (val->IsEnum ? val->Name : val->Value),
          ((val->Attributes & VTK_PARSE_DEPRECATED) ? " /* deprecated */" : ""));
      }
    }
    fprintf(fp, "%s};\n", indent);
    fprintf(fp,
      "%sfor (int c = 0; c < %d; c++)\n"
      "%s{\n",
      indent, count, indent);
    vtkWrapJavaScript_AddConstantHelper(
      fp, nextindent, "constants[c].name", "constants[c].value", firstval);
    fprintf(fp, "%s}\n", indent);
    fprintf(fp, "}\n");
  }
}

/* -------------------------------------------------------------------- */
/* This method adds one constant defined in the file to the module */

void vtkWrapJavaScript_AddConstant(FILE* fp, const char* indent, ValueInfo* val)
{
  vtkWrapJavaScript_AddConstantHelper(fp, indent, NULL, NULL, val);
}
