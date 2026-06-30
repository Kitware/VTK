// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkWrapJsonClass.h"
#include "vtkParseExtras.h"
#include "vtkParseProperties.h"
#include "vtkParseType.h"
#include "vtkWrap.h"
#include "vtkWrapSerDesClass.h"
#include "vtkWrapSerDesFunction.h"
#include "vtkWrapSerDesProperty.h"

#include <stdlib.h>
#include <string.h>

// NOLINTBEGIN(bugprone-unsafe-functions)

/* Method bits that make a property writable (deserializable). If none are set,
   the property is GET-only and is emitted with "readOnly": true. */
#define VTK_JSON_WRITE_METHODS                                                                     \
  (VTK_METHOD_SET | VTK_METHOD_SET_MULTI | VTK_METHOD_SET_IDX | VTK_METHOD_SET_NTH |               \
    VTK_METHOD_SET_NUMBER_OF | VTK_METHOD_SET_VALUE_TO | VTK_METHOD_SET_BOOL | VTK_METHOD_ADD |    \
    VTK_METHOD_ADD_MULTI | VTK_METHOD_ADD_IDX | VTK_METHOD_ADD_NODISCARD | VTK_METHOD_REMOVE |     \
    VTK_METHOD_REMOVE_IDX | VTK_METHOD_REMOVE_ALL | VTK_METHOD_REMOVE_NODISCARD)

/* Target pointer size (bytes) used to bake word-width C types. Set once per
   class from the wordSize argument to vtkWrapJson_Class(). 8 (64-bit) by
   default so a native host build is correct without the flag. */
static int vtkWrapJson_TargetWordSize = 8;

/* -------------------------------------------------------------------- */
/* Concrete name for a numeric VTK_PARSE base type.
   Fixed-width types map to an exact width; word-width types (long, size_t,
   ssize_t) are baked to a concrete width from vtkWrapJson_TargetWordSize, so
   the manifest is architecture-specific (int32 on wasm32, int64 on wasm64). */
static const char* vtkWrapJson_NumericType(unsigned int type)
{
  const int wide = (vtkWrapJson_TargetWordSize == 8);
  switch (type & VTK_PARSE_BASE_TYPE)
  {
    case VTK_PARSE_FLOAT:
      return "float32";
    case VTK_PARSE_DOUBLE:
    case VTK_PARSE_LONG_DOUBLE:
      return "float64";
    case VTK_PARSE_CHAR:
    case VTK_PARSE_SIGNED_CHAR:
      return "int8";
    case VTK_PARSE_UNSIGNED_CHAR:
      return "uint8";
    case VTK_PARSE_SHORT:
      return "int16";
    case VTK_PARSE_UNSIGNED_SHORT:
      return "uint16";
    case VTK_PARSE_INT:
      return "int32";
    case VTK_PARSE_UNSIGNED_INT:
      return "uint32";
    case VTK_PARSE_LONG_LONG:
      return "int64";
    case VTK_PARSE_UNSIGNED_LONG_LONG:
      return "uint64";
    /* word-width: 32-bit on wasm32, 64-bit on wasm64 -- baked per target */
    case VTK_PARSE_LONG:
    case VTK_PARSE_SSIZE_T:
      return wide ? "int64" : "int32";
    case VTK_PARSE_UNSIGNED_LONG:
    case VTK_PARSE_SIZE_T:
      return wide ? "uint64" : "uint32";
#ifdef VTK_PARSE_ID_TYPE
    /* should not survive typedef expansion; fall back to the target word width */
    case VTK_PARSE_ID_TYPE:
      return wide ? "int64" : "int32";
    case VTK_PARSE_UNSIGNED_ID_TYPE:
      return wide ? "uint64" : "uint32";
#endif
    default:
      return "number";
  }
}

/* -------------------------------------------------------------------- */
/* Strip vtkSmartPointer<X> down to X; otherwise return cls unchanged. */
static const char* vtkWrapJson_StripClass(const char* cls, char* buf, size_t bufsz)
{
  if (cls && !strncmp(cls, "vtkSmartPointer<", 16))
  {
    size_t len = strlen(cls);
    /* drop the leading "vtkSmartPointer<" and the trailing '>' */
    size_t inner = (len > 17) ? (len - 17) : 0;
    if (inner >= bufsz)
    {
      inner = bufsz - 1;
    }
    memcpy(buf, cls + 16, inner);
    buf[inner] = '\0';
    return buf;
  }
  return cls;
}

/* -------------------------------------------------------------------- */
/* Emit the inline JSON schema object for a single method parameter or return
   value, e.g. { "type": "float64" } or { "$ref": "vtkMapper" }. */
static void vtkWrapJson_WriteValueSchema(
  FILE* fp, ValueInfo* val, const HierarchyInfo* hinfo, ClassInfo* classInfo)
{
  char buf[256];

  if (vtkWrap_IsVoid(val))
  {
    fprintf(fp, "{ \"type\": \"null\" }");
    return;
  }
  /* vtk object pointer or smart pointer -> typed proxy reference */
  if (vtkWrap_IsVTKObjectBaseType(hinfo, val->Class) || vtkWrap_IsVTKSmartPointer(val))
  {
    fprintf(fp, "{ \"$ref\": \"%s\" }", vtkWrapJson_StripClass(val->Class, buf, sizeof(buf)));
    return;
  }
  /* strings: std::string/vtkStdString (VTK_PARSE_STRING) or a char* C string */
  if (vtkWrap_IsString(val) || vtkWrap_IsCharPointer(val))
  {
    fprintf(fp, "{ \"type\": \"string\" }");
    return;
  }
  /* enums marshal as integers */
  if (val->IsEnum || vtkWrap_IsEnumMember(classInfo, val))
  {
    fprintf(fp, "{ \"type\": \"int32\" }");
    return;
  }
  /* fixed-size numeric tuples (vtkVector/vtkTuple/vtkColor/vtkRect) and
     vtkBoundingBox marshal as numeric arrays */
  if (!strncmp(val->Class, "vtkVector", 9) || !strncmp(val->Class, "vtkTuple", 8) ||
    !strncmp(val->Class, "vtkColor", 8) || !strncmp(val->Class, "vtkRect", 7) ||
    !strcmp(val->Class, "vtkBoundingBox"))
  {
    fprintf(fp, "{ \"type\": \"array\", \"items\": { \"type\": \"float64\" } }");
    return;
  }
  /* std::vector<T> -> array of element type */
  if (vtkWrap_IsStdVector(val))
  {
    const char* arg = vtkWrap_TemplateArg(val->Class);
    ValueInfo element;
    size_t n = 0;
    const char* elemType = "number";
    memset(&element, 0, sizeof(element));
    vtkParse_BasicTypeFromString(arg, &element.Type, &element.Class, &n);
    if (vtkWrap_IsString(&element))
    {
      elemType = "string";
    }
    else
    {
      elemType = vtkWrapJson_NumericType(element.Type);
    }
    fprintf(fp, "{ \"type\": \"array\", \"items\": { \"type\": \"%s\" } }", elemType);
    free((char*)arg);
    return;
  }
  /* numeric scalars and numeric C arrays */
  if (vtkWrap_IsNumeric(val))
  {
    if (vtkWrap_IsBool(val))
    {
      fprintf(fp, "{ \"type\": \"boolean\" }");
    }
    else if (vtkWrap_IsScalar(val))
    {
      fprintf(fp, "{ \"type\": \"%s\" }", vtkWrapJson_NumericType(val->Type));
    }
    else if (vtkWrap_IsArray(val) || vtkWrap_IsCharPointer(val))
    {
      fprintf(fp, "{ \"type\": \"array\", \"items\": { \"type\": \"%s\" } }",
        vtkWrapJson_NumericType(val->Type));
    }
    else
    {
      fprintf(fp, "{ \"type\": \"object\" }");
    }
    return;
  }
  /* unknown / opaque */
  fprintf(fp, "{ \"type\": \"object\" }");
}

/* -------------------------------------------------------------------- */
/* Emit the inline JSON schema for a property, derived from its discovered
   value type, array count, and class name. Adds "readOnly": true when the
   property has no setter. */
static void vtkWrapJson_WritePropertySchema(
  FILE* fp, const PropertyInfo* prop, const HierarchyInfo* hinfo, int readOnly)
{
  const char* ro = readOnly ? ", \"readOnly\": true" : "";
  char buf[256];

  /* vtk object property -> typed proxy reference */
  if (prop->ClassName && vtkWrap_IsVTKObjectBaseType(hinfo, prop->ClassName))
  {
    fprintf(
      fp, "{ \"$ref\": \"%s\"%s }", vtkWrapJson_StripClass(prop->ClassName, buf, sizeof(buf)), ro);
    return;
  }

  unsigned int base = prop->Type & VTK_PARSE_BASE_TYPE;
  /* std::string/vtkStdString, or a char* C string (names, filenames) */
  if (base == VTK_PARSE_STRING ||
    (base == VTK_PARSE_CHAR && (prop->Type & VTK_PARSE_POINTER_MASK)) ||
    (prop->ClassName &&
      (!strcmp(prop->ClassName, "vtkStdString") || !strcmp(prop->ClassName, "string"))))
  {
    fprintf(fp, "{ \"type\": \"string\"%s }", ro);
    return;
  }
  if (base == VTK_PARSE_BOOL)
  {
    fprintf(fp, "{ \"type\": \"boolean\"%s }", ro);
    return;
  }
  /* fixed-size numeric arrays (scalars report Count 0 or 1) */
  if (prop->Count > 1)
  {
    fprintf(fp, "{ \"type\": \"array\", \"items\": { \"type\": \"%s\" }%s }",
      vtkWrapJson_NumericType(prop->Type), ro);
    return;
  }
  fprintf(fp, "{ \"type\": \"%s\"%s }", vtkWrapJson_NumericType(prop->Type), ro);
}

/* -------------------------------------------------------------------- */
/* Emit the "properties" object. Mirrors vtkWrapSerDes_Properties' iteration so
   the same properties are selected. */
static void vtkWrapJson_WriteProperties(FILE* fp, ClassInfo* classInfo, const HierarchyInfo* hinfo)
{
  ClassProperties* properties = vtkParseProperties_Create(classInfo, hinfo);
  int* isWritten = (int*)calloc(properties->NumberOfProperties, sizeof(int));
  int first = 1;
  int i = 0;

  fprintf(fp, "  \"properties\": {");
  for (i = 0; i < classInfo->NumberOfFunctions; ++i)
  {
    FunctionInfo* theFunc = classInfo->Functions[i];
    if (!theFunc->IsPublic || vtkWrap_IsInheritedMethod(classInfo, theFunc))
    {
      continue;
    }
    if (!properties->MethodHasProperty[i])
    {
      continue;
    }
    int j = properties->MethodProperties[i];
    if (isWritten[j])
    {
      continue;
    }
    PropertyInfo* theProp = properties->Properties[j];
    const char* skipReason = NULL;
    if (!vtkWrapSerDes_IsAllowable(hinfo, classInfo, theFunc, theProp, &skipReason) &&
      !theFunc->MarshalPropertyName)
    {
      continue;
    }
    isWritten[j] = 1;
    const int readOnly = (theProp->PublicMethods & VTK_JSON_WRITE_METHODS) == 0;
    fprintf(fp, "%s\n    \"%s\": ", first ? "" : ",", theProp->Name);
    vtkWrapJson_WritePropertySchema(fp, theProp, hinfo, readOnly);
    first = 0;
  }
  fprintf(fp, "%s  }", first ? "" : "\n");

  free(isWritten);
  vtkParseProperties_Free(properties);
}

/* -------------------------------------------------------------------- */
/* Emit the "methods" object. Mirrors vtkWrapSerDes_DefineFunctions' allowability
   and overload de-duplication so the same methods are selected; one entry per
   unique method name. */
static void vtkWrapJson_WriteMethods(FILE* fp, ClassInfo* classInfo, const HierarchyInfo* hinfo)
{
  int* handled = (int*)calloc(classInfo->NumberOfFunctions, sizeof(int));
  int first = 1;
  int i = 0, k = 0, p = 0;

  fprintf(fp, "  \"methods\": {");
  for (i = 0; i < classInfo->NumberOfFunctions; ++i)
  {
    FunctionInfo* theFunc = classInfo->Functions[i];
    const char* rejectReason = NULL;
    int rejectedParameterId = -1;
    if (handled[i])
    {
      continue;
    }
    if (!vtkWrapSerDes_IsFunctionAllowed(
          theFunc, classInfo, hinfo, &rejectReason, &rejectedParameterId))
    {
      continue;
    }
    /* mark every same-named allowed overload as handled (one manifest entry) */
    for (k = i; k < classInfo->NumberOfFunctions; ++k)
    {
      if (!strcmp(theFunc->Name, classInfo->Functions[k]->Name))
      {
        handled[k] = 1;
      }
    }

    fprintf(fp, "%s\n    \"%s\": {\n      \"parameters\": {", first ? "" : ",", theFunc->Name);
    for (p = 0; p < theFunc->NumberOfParameters; ++p)
    {
      ValueInfo* paramInfo = theFunc->Parameters[p];
      if (paramInfo->Name)
      {
        fprintf(fp, "%s \"%s\": ", p ? "," : "", paramInfo->Name);
      }
      else
      {
        fprintf(fp, "%s \"arg%d\": ", p ? "," : "", p);
      }
      vtkWrapJson_WriteValueSchema(fp, paramInfo, hinfo, classInfo);
    }
    fprintf(fp, "%s},\n      \"returns\": ", theFunc->NumberOfParameters ? " " : "");
    vtkWrapJson_WriteValueSchema(fp, theFunc->ReturnValue, hinfo, classInfo);
    fprintf(fp, "\n    }");
    first = 0;
  }
  fprintf(fp, "%s  }", first ? "" : "\n");

  free(handled);
}

/* -------------------------------------------------------------------- */
void vtkWrapJson_Class(FILE* fp, const HierarchyInfo* hinfo, ClassInfo* classInfo, int wordSize)
{
  const char* supermodule = NULL;
  const char* super = vtkWrapSerDes_GetSuperClass(classInfo, hinfo, &supermodule);

  vtkWrapJson_TargetWordSize = (wordSize == 4) ? 4 : 8;

  fprintf(fp, "{\n");
  fprintf(fp, "  \"title\": \"%s\",\n", classInfo->Name);
  fprintf(fp, "  \"type\": \"object\",\n");
  if (super)
  {
    fprintf(fp, "  \"inherits\": \"%s\",\n", super);
  }
  vtkWrapJson_WriteProperties(fp, classInfo, hinfo);
  fprintf(fp, ",\n");
  vtkWrapJson_WriteMethods(fp, classInfo, hinfo);
  fprintf(fp, "\n}\n");
}

// NOLINTEND(bugprone-unsafe-functions)
