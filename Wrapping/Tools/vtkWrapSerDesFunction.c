// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkWrapSerDesFunction.h"
#include "vtkParseExtras.h"
#include "vtkParseHierarchy.h"
#include "vtkWrap.h"

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

// NOLINTBEGIN(bugprone-unsafe-functions)

static int vtkWrapSerDes_CanMarshalValue(
  ValueInfo* valInfo, ClassInfo* classInfo, const HierarchyInfo* hinfo, int isReturnValue)
{
  if (isReturnValue == 1 && vtkWrap_IsVoid(valInfo))
  {
    return 1;
  }

  const int isVTKObject = vtkWrap_IsVTKObjectBaseType(hinfo, valInfo->Class);
  const int isVTKSmartPointer = vtkWrap_IsVTKSmartPointer(valInfo);
  const int isPointer = vtkWrap_IsPointer(valInfo);
  const int isScalar = vtkWrap_IsScalar(valInfo);
  const int isNumeric = vtkWrap_IsNumeric(valInfo);
  const int isString = vtkWrap_IsString(valInfo);
  const int isCharPointer = vtkWrap_IsCharPointer(valInfo);
  const int isEnumMember = vtkWrap_IsEnumMember(classInfo, valInfo);
  const int isEnum = valInfo->IsEnum;
  const int isArray = vtkWrap_IsArray(valInfo);
  const int isStdVector = vtkWrap_IsStdVector(valInfo);

  int isAllowed = -1;
  // vtkAOSDataArrayTemplate does not get recognized as a template class through valInfo->Template.
  if (strstr(valInfo->Class, "vtkAOSDataArrayTemplate") != NULL ||
    strstr(valInfo->Class, "vtkSOADataArrayTemplate") != NULL)
  {
    isAllowed = 0;
  }
  else if (valInfo->Template != NULL)
  {
    isAllowed = 0;
  }
  else if (vtkWrap_IsNonConstRef(valInfo))
  {
    isAllowed = 0;
  }
  else if (!strcmp(valInfo->Class, "vtkStdString") && isPointer)
  {
    isAllowed = 0;
  }
  else if ((isVTKObject && (isPointer || isVTKSmartPointer)) || isVTKSmartPointer)
  {
    isAllowed = 1;
  }
  else if (isNumeric)
  {
    if (isScalar)
    {
      isAllowed = 1;
    }
    else if (isArray)
    {
      isAllowed = 1;
    }
    else if (isCharPointer)
    {
      isAllowed = 1;
    }
    else
    {
      isAllowed = 0;
    }
  }
  else if (isString)
  {
    isAllowed = 1;
  }
  else if (isEnumMember)
  {
    isAllowed = 1;
  }
  else if (isEnum)
  {
    isAllowed = 1;
  }
  else if (!strncmp(valInfo->Class, "vtkVector", 9) || !strncmp(valInfo->Class, "vtkTuple", 8) ||
    !strncmp(valInfo->Class, "vtkColor", 8) || !strncmp(valInfo->Class, "vtkRect", 7))
  {
    isAllowed = 1;
  }
  else if (!strcmp(valInfo->Class, "vtkBoundingBox"))
  {
    isAllowed = 1;
  }
  else if (isStdVector)
  {
    const char* arg = vtkWrap_TemplateArg(valInfo->Class);
    size_t n;
    ValueInfo* element = (ValueInfo*)calloc(1, sizeof(ValueInfo));
    size_t l = vtkParse_BasicTypeFromString(arg, &(element->Type), &(element->Class), &n);
    (void)l;
    /* check that type is a string or real or integer */
    if (vtkWrap_IsString(element) || vtkWrap_IsRealNumber(element) || vtkWrap_IsInteger(element))
    {
      isAllowed = 1;
    }
    else if (vtkWrap_IsVTKObjectBaseType(hinfo, element->Class))
    {
      isAllowed = 0;
    }
    else
    {
      isAllowed = 0;
    }
    free(element);
  }
  else
  {
    isAllowed = 0;
  }
  return isAllowed;
}

static int vtkWrapSerDes_IsFunctionAllowed(FunctionInfo* functionInfo, ClassInfo* classInfo,
  const HierarchyInfo* hinfo, const char** rejectReason, int* rejectedParameterId)
{
  *rejectedParameterId = -1;
  /* Ignore static methods */
  if (functionInfo->IsStatic)
  {
    *rejectReason = "static";
    return 0;
  }
  /* Ignore inaccessible methods */
  if (!functionInfo->IsPublic)
  {
    *rejectReason = "not-public";
    return 0;
  }
  /* Ignore template methods */
  if (functionInfo->Template)
  {
    *rejectReason = "templated";
    return 0;
  }
  /* Ignore NewInstance */
  if (!strcmp(functionInfo->Name, "NewInstance"))
  {
    *rejectReason = "NewInstance";
    return 0;
  }
  /* Return value must be allowable */
  if (!vtkWrapSerDes_CanMarshalValue(
        functionInfo->ReturnValue, classInfo, hinfo, /*isReturnValue=*/1))
  {
    *rejectReason = "rejected-return-type";
    return 0;
  }
  /* Inherited methods and overriden methods are handled by superclasses */
  if (vtkWrap_IsInheritedMethod(classInfo, functionInfo) || functionInfo->IsOverride)
  {
    *rejectReason = "inherited";
    return 0;
  }
  /* Ignore constructors and destructors. */
  if (vtkWrap_IsConstructor(classInfo, functionInfo) ||
    vtkWrap_IsDestructor(classInfo, functionInfo))
  {
    *rejectReason = "constructor-or-destructor";
    return 0;
  }
  /* All parameters must be allowable */
  int parameterId = 0;
  for (parameterId = 0; parameterId < functionInfo->NumberOfParameters; ++parameterId)
  {
    ValueInfo* parameterInfo = functionInfo->Parameters[parameterId];
    if (!vtkWrapSerDes_CanMarshalValue(parameterInfo, classInfo, hinfo, /*isReturnValue=*/0))
    {
      *rejectReason = "rejected-parameter-type";
      *rejectedParameterId = parameterId;
      return 0;
    }
  }
  return 1;
}

static char* vtkWrapSerDes_SmartPointerTypeTemplateArg(const char* name)
{
  const char* defaults[1] = { NULL };
  const char** args;
  char* arg;

  vtkParse_DecomposeTemplatedType(name, NULL, 1, &args, defaults);
  arg = strdup(args[0]);
  vtkParse_FreeTemplateDecomposition(NULL, 1, args);

  return arg;
}

static int vtkWrapSerDes_DecomposeTemplatedTuple(
  ValueInfo* valInfo, char** elementType, const HierarchyInfo* hinfo)
{
  const HierarchyEntry* entry;
  const char* classname = NULL;
  int tupleSize = 0;

  entry = vtkParseHierarchy_FindEntry(hinfo, valInfo->Class);
  assert(entry != NULL);
  if (entry &&
    vtkParseHierarchy_IsTypeOfTemplated(hinfo, entry, valInfo->Class, "vtkTuple", &classname))
  {
    const char* defaults[2] = { NULL, NULL };
    const char** args;

    vtkParse_DecomposeTemplatedType(classname, NULL, 2, &args, defaults);
    *elementType = strdup(args[0]);
    tupleSize = atoi(args[1]);
    vtkParse_FreeTemplateDecomposition(NULL, 2, args);
  }
  assert(tupleSize > 0);
  assert(*elementType != NULL);
  return tupleSize;
}

static void vtkWrapSerDes_FreeTemplatedTupleDecomposition(char** elementType)
{
  free(*elementType);
}

static void vtkWrapSerDes_WriteArgumentDeserializer(
  FILE* fp, int paramId, ValueInfo* valInfo, ClassInfo* classInfo, const HierarchyInfo* hinfo)
{
  const int isVTKObject = vtkWrap_IsVTKObjectBaseType(hinfo, valInfo->Class);
  const int isVTKSmartPointer = vtkWrap_IsVTKSmartPointer(valInfo);
  const int isPointer = vtkWrap_IsPointer(valInfo);
  const int isScalar = vtkWrap_IsScalar(valInfo);
  const int isNumeric = vtkWrap_IsNumeric(valInfo);
  const int isString = vtkWrap_IsString(valInfo);
  const int isCharPointer = vtkWrap_IsCharPointer(valInfo);
  const int isEnumMember = vtkWrap_IsEnumMember(classInfo, valInfo);
  const int isEnum = valInfo->IsEnum;
  const int isArray = vtkWrap_IsArray(valInfo);
  const int isStdVector = vtkWrap_IsStdVector(valInfo);

  // vtkObject and vtkSmartPointer<T> are handled in the same way
  // vtkSmartPointer<T> is a template class, so we need to get the type of the
  // template argument.
  // vtkObject is a class, so we can use the class name directly.
  char* className = NULL;
  if (isVTKSmartPointer)
  {
    className = vtkWrapSerDes_SmartPointerTypeTemplateArg(valInfo->Class);
  }
  else if (isVTKObject && isPointer)
  {
    className = strdup(valInfo->Class);
  }
  if (isVTKSmartPointer || (isVTKObject && isPointer))
  {
    fprintf(fp,
      "    auto arg_%d = "
      "reinterpret_cast<%s*>(objectFromContext%d.GetPointer());\n",
      paramId, className, paramId);
    free(className);
    return;
  }
  if (isNumeric)
  {
    if (isScalar)
    {
      fprintf(fp, "    auto arg_%d = args[%d].get<%s>();\n", paramId, paramId, valInfo->Class);
      return;
    }
    if (isArray)
    {
      fprintf(fp, "    auto elements_%d = args[%d].get<std::vector<%s>>();\n", paramId, paramId,
        valInfo->Class);
      fprintf(fp, "    auto* arg_%d = elements_%d.data();\n", paramId, paramId);
      return;
    }
    if (isCharPointer)
    {
      fprintf(fp, "    auto elements_%d = args[%d].get<std::string>();\n", paramId, paramId);
      fprintf(fp, "    auto* arg_%d = elements_%d.data();\n", paramId, paramId);
      return;
    }
    const int declarationLength = vtkParse_ValueInfoToString(valInfo, NULL, VTK_PARSE_EVERYTHING);
    char* declaration = (char*)malloc((declarationLength + 1) * sizeof(char));
    vtkParse_ValueInfoToString(valInfo, declaration, VTK_PARSE_EVERYTHING);
    fprintf(stderr, "%s:%d Unexpected parameter \'%s\' in %s!\n", __FILE__, __LINE__, declaration,
      __func__);
    free(declaration);
    abort();
  }
  if (isString)
  {
    fprintf(fp, "    auto arg_%d = args[%d].get<std::string>();\n", paramId, paramId);
    return;
  }
  if (isEnumMember)
  {
    fprintf(fp,
      "    auto arg_%d = "
      "static_cast<%s::%s>(args[%d].get<std::underlying_type<%s::%s>::type>());\n",
      paramId, classInfo->Name, valInfo->Class, paramId, classInfo->Name, valInfo->Class);
    return;
  }
  if (isEnum)
  {
    const char* cp = valInfo->Class;
    size_t l;
    /* search for scope operator */
    for (l = 0; cp[l] != '\0'; l++)
    {
      if (cp[l] == ':')
      {
        break;
      }
    }
    if (cp[l] == ':' && cp[l + 1] == ':')
    {
      fprintf(fp,
        "    auto arg_%d = "
        "static_cast<%*.*s::%s>(args[%d].get<std::underlying_type<%*.*s::%s>::type>());\n",
        paramId, (int)l, (int)l, cp, &cp[l + 2], paramId, (int)l, (int)l, cp, &cp[l + 2]);
    }
    else
    {
      fprintf(fp,
        "    auto arg_%d = static_cast<%s>(args[%d].get<std::underlying_type<%s>::type>());\n",
        paramId, cp, paramId, cp);
    }
    return;
  }
  if (!strncmp(valInfo->Class, "vtkVector", 9) || !strncmp(valInfo->Class, "vtkTuple", 8) ||
    !strncmp(valInfo->Class, "vtkColor", 8) || !strncmp(valInfo->Class, "vtkRect", 7))
  {
    char* elementType = NULL;
    const int elementCount = vtkWrapSerDes_DecomposeTemplatedTuple(valInfo, &elementType, hinfo);
    fprintf(fp, "    auto elements_%d = args[%d].get<std::array<%s, %d>>();\n", paramId, paramId,
      elementType, elementCount);
    fprintf(fp, "    %s arg_%d{elements_%d.data()};\n", valInfo->Class, paramId, paramId);
    vtkWrapSerDes_FreeTemplatedTupleDecomposition(&elementType);
    return;
  }
  if (!strcmp(valInfo->Class, "vtkBoundingBox"))
  {
    fprintf(
      fp, "    auto elements_%d = args[%d].get<std::array<double, 6>>();\n", paramId, paramId);
    fprintf(fp, "    vtkBoundingBox arg_%d{elements_%d.data()};\n", paramId, paramId);
    return;
  }
  if (isStdVector)
  {
    const char* arg = vtkWrap_TemplateArg(valInfo->Class);
    size_t n;
    ValueInfo* element = (ValueInfo*)calloc(1, sizeof(ValueInfo));
    vtkParse_BasicTypeFromString(arg, &(element->Type), &(element->Class), &n);
    /* check that type is a string or real or integer */
    if (vtkWrap_IsString(element) || vtkWrap_IsRealNumber(element) || vtkWrap_IsInteger(element))
    {
      fprintf(fp, "    auto arg_%d = args[%d].get<std::vector<%s>>();\n", paramId, paramId,
        element->Class);
      free(element);
      return;
    }
    free(element);
  }
  const int declarationLength = vtkParse_ValueInfoToString(valInfo, NULL, VTK_PARSE_EVERYTHING);
  char* declaration = (char*)malloc((declarationLength + 1) * sizeof(char));
  vtkParse_ValueInfoToString(valInfo, declaration, VTK_PARSE_EVERYTHING);
  fprintf(
    stderr, "%s:%d Unexpected parameter %s in %s!\n", __FILE__, __LINE__, declaration, __func__);
  free(declaration);
  abort();
}

/* -------------------------------------------------------------------- */
static void vtkWrapSerDes_WriteReturnValueSerializer(
  FILE* fp, const ClassInfo* classInfo, const HierarchyInfo* hinfo, ValueInfo* valInfo)
{
  const int isVTKObject = vtkWrap_IsVTKObjectBaseType(hinfo, valInfo->Class);
  const int isVTKSmartPointer = vtkWrap_IsVTKSmartPointer(valInfo);
  const int isPointer = vtkWrap_IsPointer(valInfo);
  const int isScalar = vtkWrap_IsScalar(valInfo);
  const int isNumeric = vtkWrap_IsNumeric(valInfo);
  const int isString = vtkWrap_IsString(valInfo);
  const int isCharPointer = vtkWrap_IsCharPointer(valInfo);
  const int isEnumMember = vtkWrap_IsEnumMember(classInfo, valInfo);
  const int isEnum = valInfo->IsEnum;
  const int isArray = vtkWrap_IsArray(valInfo);
  const int isStdVector = vtkWrap_IsStdVector(valInfo);

  if (isVTKObject && isPointer)
  {
    fprintf(fp,
      "    // NOLINTNEXTLINE(readability-redundant-casting)\n"
      "    vtkTypeUInt32 identifier = "
      "context->GetId(reinterpret_cast<vtkObjectBase*>(methodReturnValue));\n");
    fprintf(fp,
      "    if (identifier == 0)\n"
      "    {\n"
      "      // NOLINTNEXTLINE(readability-redundant-casting)\n"
      "      context->RegisterObject(reinterpret_cast<vtkObjectBase*>(methodReturnValue), "
      "identifier);\n"
      "    }\n");
    if (vtkWrap_IsNewInstance(valInfo))
    {
      // Manage the new instance in invoker context.
      fprintf(fp,
        "    context->KeepAlive(invoker->GetObjectDescription(), "
        "reinterpret_cast<vtkObjectBase*>(methodReturnValue));\n");
    }
    fprintf(fp, "    result[\"Id\"] = identifier;\n");
    return;
  }
  if (isVTKSmartPointer)
  {
    fprintf(fp,
      "    // NOLINTNEXTLINE(readability-redundant-casting)\n"
      "    vtkTypeUInt32 identifier = "
      "context->GetId(reinterpret_cast<vtkObjectBase*>(methodReturnValue.GetPointer()"
      "));\n");
    fprintf(fp,
      "    if (identifier == 0) { "
      "// NOLINTNEXTLINE(readability-redundant-casting)\n"
      "context->RegisterObject(reinterpret_cast<vtkObjectBase*>(methodReturnValue."
      "GetPointer()), identifier); }");
    if (vtkWrap_IsNewInstance(valInfo))
    {
      // Manage the new instance in invoker context.
      fprintf(fp,
        "    context->KeepAlive(invoker->GetObjectDescription(), "
        "reinterpret_cast<vtkObjectBase*>(methodReturnValue.GetPointer()));\n");
    }
    fprintf(fp, "    result[\"Id\"] = identifier;\n");
    return;
  }
  if (isNumeric)
  {
    if (isScalar)
    {
      fprintf(fp, "    result[\"Value\"] = methodReturnValue;\n");
      return;
    }
    if (isArray)
    {
      fprintf(fp, "    if(methodReturnValue != nullptr)\n");
      fprintf(fp, "    {\n");
      fprintf(fp, "      auto& dst = result[\"Value\"] = nlohmann::json::array();\n");
      fprintf(fp, "      for (int i = 0; i < %d; ++i) { dst.push_back(methodReturnValue[i]); }\n",
        valInfo->Count);
      fprintf(fp, "    }\n");
      return;
    }
    if (isCharPointer)
    {
      fprintf(
        fp, "    if (methodReturnValue != nullptr) { result[\"Value\"] = methodReturnValue; }\n");
      return;
    }
    const int declarationLength = vtkParse_ValueInfoToString(valInfo, NULL, VTK_PARSE_EVERYTHING);
    char* declaration = (char*)malloc((declarationLength + 1) * sizeof(char));
    vtkParse_ValueInfoToString(valInfo, declaration, VTK_PARSE_EVERYTHING);
    fprintf(stderr, "%s:%d Unexpected parameter \'%s\' in %s!\n", __FILE__, __LINE__, declaration,
      __func__);
    free(declaration);
    abort();
  }
  if (isString)
  {
    if (!strcmp(valInfo->Class, "vtkStdString"))
    {
      // workaround error: call to '__is_path_src' is ambiguous on el8.
      fprintf(fp, "    result[\"Value\"] = std::string(methodReturnValue);\n");
    }
    else
    {
      fprintf(fp, "    result[\"Value\"] = methodReturnValue;\n");
    }
    return;
  }
  if (isEnumMember)
  {
    fprintf(fp,
      "    result[\"Value\"] = "
      "static_cast<std::underlying_type<%s::%s>::type>(methodReturnValue);\n",
      classInfo->Name, valInfo->Class);
    return;
  }
  if (isEnum)
  {
    const char* cp = valInfo->Class;
    size_t l;
    /* search for scope operator */
    for (l = 0; cp[l] != '\0'; l++)
    {
      if (cp[l] == ':')
      {
        break;
      }
    }
    if (cp[l] == ':' && cp[l + 1] == ':')
    {
      fprintf(fp,
        "    // NOLINTNEXTLINE(readability-redundant-casting)\n"
        "    result[\"Value\"] = static_cast<%*.*s::%s>(methodReturnValue);\n",
        (int)l, (int)l, cp, &cp[l + 2]);
    }
    else
    {
      fprintf(fp,
        "    // NOLINTNEXTLINE(readability-redundant-casting)\n"
        "    result[\"Value\"] = static_cast<%s>(methodReturnValue);\n",
        cp);
    }
    return;
  }
  if (!strncmp(valInfo->Class, "vtkVector", 9) || !strncmp(valInfo->Class, "vtkTuple", 8) ||
    !strncmp(valInfo->Class, "vtkColor", 8) || !strncmp(valInfo->Class, "vtkRect", 7))
  {
    fprintf(fp, "    {\n");
    fprintf(fp, "      auto& dst = result[\"Value\"] = nlohmann::json::array();\n");
    fprintf(fp,
      "      for (int i = 0; i < methodReturnValue.GetSize(); ++i) { "
      "dst.push_back(methodReturnValue[i]); }\n");
    fprintf(fp, "    }\n");
    return;
  }
  if (!strcmp(valInfo->Class, "vtkBoundingBox"))
  {
    fprintf(fp, "    {\n");
    fprintf(fp, "      auto& dst = result[\"Value\"] = nlohmann::json::array();\n");
    fprintf(fp, "      double bounds[6] = {};\n");
    fprintf(fp, "      methodReturnValue.GetBounds(bounds);\n");
    fprintf(fp, "      for (int i = 0; i < 6; ++i) { dst.push_back(bounds[i]); }\n");
    fprintf(fp, "    }\n");
    return;
  }
  if (isStdVector)
  {
    const char* arg = vtkWrap_TemplateArg(valInfo->Class);
    size_t n;
    ValueInfo* element = (ValueInfo*)calloc(1, sizeof(ValueInfo));
    size_t l = vtkParse_BasicTypeFromString(arg, &(element->Type), &(element->Class), &n);
    (void)l;
    /* check that type is a string or real or integer */
    if (vtkWrap_IsString(element) || vtkWrap_IsRealNumber(element) || vtkWrap_IsInteger(element))
    {
      if (isPointer)
      {
        fprintf(fp, "    result[\"Value\"] = *methodReturnValue;\n");
      }
      else
      {
        fprintf(fp, "    result[\"Value\"] = methodReturnValue;\n");
      }
      free(element);
      return;
    }
    free(element);
  }
  const int declarationLength = vtkParse_ValueInfoToString(valInfo, NULL, VTK_PARSE_EVERYTHING);
  char* declaration = (char*)malloc((declarationLength + 1) * sizeof(char));
  vtkParse_ValueInfoToString(valInfo, declaration, VTK_PARSE_EVERYTHING);
  fprintf(
    stderr, "%s:%d Unexpected parameter %s in %s!\n", __FILE__, __LINE__, declaration, __func__);
  free(declaration);
  abort();
}

static void vtkWrapSerDes_WriteArgumentCheck(
  FILE* fp, FunctionInfo* functionInfo, ClassInfo* classInfo, const HierarchyInfo* hinfo)
{
  int i = 0;
  for (i = 0; i < functionInfo->NumberOfParameters; ++i)
  {
    ValueInfo* valInfo = functionInfo->Parameters[i];
    const int isVTKObject = vtkWrap_IsVTKObjectBaseType(hinfo, valInfo->Class);
    const int isVTKSmartPointer = vtkWrap_IsVTKSmartPointer(valInfo);
    const int isPointer = vtkWrap_IsPointer(valInfo);
    if (isVTKSmartPointer || (isVTKObject && isPointer))
    {
      fprintf(fp, "  nlohmann::json::const_iterator idIter%d;\n", i);
      fprintf(fp, "  vtkSmartPointer<vtkObjectBase> objectFromContext%d;\n", i);
    }
  }
  if (functionInfo->NumberOfParameters == 0)
  {
    fprintf(fp, "  if (args.empty())\n");
  }
  else
  {
    fprintf(fp, "  if (args.size() == %d", functionInfo->NumberOfParameters);
  }
  for (i = 0; i < functionInfo->NumberOfParameters; ++i)
  {
    ValueInfo* valInfo = functionInfo->Parameters[i];
    const int isVTKObject = vtkWrap_IsVTKObjectBaseType(hinfo, valInfo->Class);
    const int isVTKSmartPointer = vtkWrap_IsVTKSmartPointer(valInfo);
    const int isPointer = vtkWrap_IsPointer(valInfo);
    const int isScalar = vtkWrap_IsScalar(valInfo);
    const int isNumeric = vtkWrap_IsNumeric(valInfo);
    const int isString = vtkWrap_IsString(valInfo);
    const int isCharPointer = vtkWrap_IsCharPointer(valInfo);
    const int isEnumMember = vtkWrap_IsEnumMember(classInfo, valInfo);
    const int isEnum = valInfo->IsEnum;
    const int isArray = vtkWrap_IsArray(valInfo);
    const int isStdVector = vtkWrap_IsStdVector(valInfo);

    // vtkObject and vtkSmartPointer<T> are handled in the same way
    // vtkSmartPointer<T> is a template class, so we need to get the type of the
    // template argument.
    // vtkObject is a class, so we can use the class name directly.
    char* className = NULL;
    if (isVTKSmartPointer)
    {
      className = vtkWrapSerDes_SmartPointerTypeTemplateArg(valInfo->Class);
    }
    else if (isVTKObject && isPointer)
    {
      className = strdup(valInfo->Class);
    }
    if (isVTKSmartPointer || (isVTKObject && isPointer))
    {
      // Verify that the parameter is a JSON object and contains the "Id" key
      // This is needed for vtkSmartPointer and vtkObjectBase types.
      fprintf(fp,
        "\n   && ((args[%d].is_object()"
        "\n   && (static_cast<void>(idIter%d = args[%d].find(\"Id\")), idIter%d != "
        "args[%d].end())" // uses the comma to initialize idIter inline. This cannot be done outside
                          // because args[i] might throw out of range error. The first expression is
                          // casted to void in order to silence the `-Wcomma` warning
        "\n   && idIter%d->is_number_unsigned()"
        "\n   && (objectFromContext%d = context->GetObjectAtId(*idIter%d))"
        "\n   && context->GetObjectAtId(*idIter%d)->IsA(\"%s\"))"
        "\n   || args[%d].is_null())",
        i, i, i, i, i, i, i, i, i, className, i);
      free(className);
    }
    else if (isNumeric)
    {
      if (isScalar)
      {
        if (vtkWrap_IsBool(valInfo))
        {
          fprintf(fp, "\n   && args[%d].is_boolean()", i);
        }
        else if (vtkWrap_IsInteger(valInfo))
        {
          fprintf(fp, "\n   && args[%d].is_number_integer()", i);
        }
        else
        {
          fprintf(fp, "\n   && args[%d].is_number()", i);
        }
      }
      else if (isArray)
      {
        fprintf(fp, "\n   && args[%d].is_array()", i);
      }
      else if (isCharPointer)
      {
        fprintf(fp, "\n   && args[%d].is_string()", i);
      }
    }
    else if (isString)
    {
      fprintf(fp, "\n   && args[%d].is_string()", i);
    }
    else if (isEnumMember)
    {
      fprintf(fp, "\n   && args[%d].is_number_integer()", i);
    }
    else if (isEnum)
    {
      fprintf(fp, "\n   && args[%d].is_number_integer()", i);
    }
    else if (!strncmp(valInfo->Class, "vtkVector", 9) || !strncmp(valInfo->Class, "vtkTuple", 8) ||
      !strncmp(valInfo->Class, "vtkColor", 8) || !strncmp(valInfo->Class, "vtkRect", 7))
    {
      char* elementType = NULL;
      const int elementCount = vtkWrapSerDes_DecomposeTemplatedTuple(valInfo, &elementType, hinfo);
      fprintf(fp, "\n   && args[%d].is_array() && (args[%d].size() == %d)", i, i, elementCount);
      vtkWrapSerDes_FreeTemplatedTupleDecomposition(&elementType);
    }
    else if (!strcmp(valInfo->Class, "vtkBoundingBox"))
    {
      fprintf(fp, "\n   && args[%d].is_array() && (args[%d].size() == 6)", i, i);
    }
    else if (isStdVector)
    {
      fprintf(fp, "\n   && args[%d].is_array()", i);
    }
  }
  if (functionInfo->NumberOfParameters > 0)
  {
    fprintf(fp, "\n     )\n");
  }
}

static int vtkWrapSerDes_WriteMemberFunctionCall(
  FILE* fp, ClassInfo* classInfo, FunctionInfo* functionInfo, const HierarchyInfo* hinfo)
{
  int i = 0;
  fprintf(fp, "  {\n"); // some arguments need locals, so scope them.
  vtkWrapSerDes_WriteArgumentCheck(fp, functionInfo, classInfo, hinfo);
  fprintf(fp, "  {\n");
  for (i = 0; i < functionInfo->NumberOfParameters; ++i)
  {
    // Retrieve individual arguments to stack and call method on object with arguments.
    // Then pack the result in json and return the json object. Handle errors by storing message
    // in the result json and set Success = false
    // Return json: {"Value" (or) "Id": value, "Success": false/true, "Message": "Failed to parse
    // args ... etc."}

    ValueInfo* paramInfo = functionInfo->Parameters[i];
    vtkWrapSerDes_WriteArgumentDeserializer(fp, i, paramInfo, classInfo, hinfo);
  }
  const char* argStart = "";
  const char* argEnd = "";
  if (functionInfo->NumberOfParameters > 0)
  {
    argStart = "\n";
    argEnd = "    ";
  }
  fprintf(fp,
    "    vtkVLog(invoker->GetInvokerLogVerbosity(), \"Calling %s::%s with args\" << "
    "args.dump());\n",
    classInfo->Name, functionInfo->Name);
  if (vtkWrap_IsVoid(functionInfo->ReturnValue))
  {
    fprintf(fp, "    object->%s(%s", functionInfo->Name, argStart);
    for (i = 0; i < functionInfo->NumberOfParameters; ++i)
    {
      ValueInfo* paramInfo = functionInfo->Parameters[i];
      const char* parameterName = paramInfo->Name ? paramInfo->Name : "noname";
      fprintf(fp, "      /*%s=*/arg_%d", parameterName, i);
      if (functionInfo->NumberOfParameters > 1 && i != functionInfo->NumberOfParameters - 1)
      {
        fprintf(fp, ",");
      }
      fprintf(fp, "\n");
    }
    fprintf(fp, "%s);\n", argEnd);
  }
  else
  {
    fprintf(fp,
      "    // NOLINTNEXTLINE(performance-unnecessary-copy-initialization)\n"
      "    auto%s methodReturnValue = object->%s(%s",
      (vtkWrap_IsPointer(functionInfo->ReturnValue) ? "*" : ""), functionInfo->Name, argStart);
    for (i = 0; i < functionInfo->NumberOfParameters; ++i)
    {
      ValueInfo* paramInfo = functionInfo->Parameters[i];
      const char* parameterName = paramInfo->Name ? paramInfo->Name : "noname";
      fprintf(fp, "      /*%s=*/arg_%d", parameterName, i);
      if (functionInfo->NumberOfParameters > 1 && i != functionInfo->NumberOfParameters - 1)
      {
        fprintf(fp, ",\n");
      }
    }
    fprintf(fp, "%s);\n", argEnd);
    vtkWrapSerDes_WriteReturnValueSerializer(fp, classInfo, hinfo, functionInfo->ReturnValue);
  }
  fprintf(fp,
    "    result[\"Message\"] = std::string(\"Call to \") + object->GetClassName() + "
    "std::string(\"::\") + \"%s\" + std::string(\" is successful.\");\n",
    functionInfo->Name);
  fprintf(fp, "    result[\"Success\"] = true;\n");
  fprintf(fp, "  }\n"); // end if arguments check
  fprintf(fp, "  }\n");
  return 1;
}

void vtkWrapSerDes_DefineFunctions(FILE* fp, ClassInfo* classInfo, const HierarchyInfo* hinfo)
{
  // Ignore the invoker for vtkObjectBase, as its methods are sensitive to object lifetime.
  if (!strcmp(classInfo->Name, "vtkObjectBase"))
  {
    return;
  }
  int functionId = 0, functionId2 = 0;
  FunctionInfo *theFunc = NULL, *overloadedFunc = NULL;
  int* generatedFunctionCalls = (int*)calloc(classInfo->NumberOfFunctions, sizeof(int));
  /* Loop through all functions in the class */
  for (functionId = 0; functionId < classInfo->NumberOfFunctions; ++functionId)
  {
    theFunc = classInfo->Functions[functionId];
    fprintf(fp, "//Method:\'%s\'\n", theFunc->Signature);
    const char* rejectReason = NULL;
    int rejectedParameterId = -1;
    /* Skip unallowable function */
    if (!vtkWrapSerDes_IsFunctionAllowed(
          theFunc, classInfo, hinfo, &rejectReason, &rejectedParameterId))
    {
      fprintf(fp, "//- not allowed: %s", rejectReason);
      if (rejectedParameterId >= 0)
      {
        fprintf(fp, ", parameter at index:%d cannot be marshalled.", rejectedParameterId);
      }
      fprintf(fp, "\n");
      continue;
    }
    /* Skip function whose calling code was already generated */
    if (generatedFunctionCalls[functionId])
    {
      fprintf(fp, "//Overload already handled\n");
      continue;
    }
    fprintf(fp,
      "static void Invoke_%s_%s(vtkInvoker* invoker, "
      "%s* object, const nlohmann::json& args, nlohmann::json& result)\n"
      "{\n",
      classInfo->Name, theFunc->Name, classInfo->Name);
    fprintf(fp, "  vtkVLogScopeFunction(invoker->GetInvokerLogVerbosity());\n");
    fprintf(fp,
      "  result[\"Message\"] = std::string(\"No suitable overload of "
      "\'%s::%s\' takes the specified arguments.\") + args.dump();\n",
      classInfo->Name, theFunc->Name);
    fprintf(fp, "  result[\"Success\"] = false;\n");
    fprintf(fp, "  auto context = invoker->GetContext();\n");
    fprintf(fp, "  (void)context;\n");
    fprintf(fp, "  (void)object;\n");
    fprintf(fp, "  (void)args;\n");
    generatedFunctionCalls[functionId] =
      vtkWrapSerDes_WriteMemberFunctionCall(fp, classInfo, theFunc, hinfo);
    for (functionId2 = 0; functionId2 < classInfo->NumberOfFunctions; ++functionId2)
    {
      /* Skip function same as outer function */
      if (functionId == functionId2)
      {
        continue;
      }
      /* Skip different function */
      if (strcmp(theFunc->Name, classInfo->Functions[functionId2]->Name) != 0)
      {
        continue;
      }
      // found an overload.
      overloadedFunc = classInfo->Functions[functionId2];
      fprintf(fp, "//Overload:\'%s\'\n", overloadedFunc->Signature);
      /* Skip unallowable function */
      if (!vtkWrapSerDes_IsFunctionAllowed(
            overloadedFunc, classInfo, hinfo, &rejectReason, &rejectedParameterId))
      {
        continue;
      }
      generatedFunctionCalls[functionId2] =
        vtkWrapSerDes_WriteMemberFunctionCall(fp, classInfo, overloadedFunc, hinfo);
    }
    fprintf(fp, "}\n");
  }
  free(generatedFunctionCalls);
}

void vtkWrapSerDes_CallFunctions(FILE* fp, ClassInfo* classInfo, const HierarchyInfo* hinfo)
{
  // Ignore the invoker for vtkObjectBase, as its methods are sensitive to object lifetime.
  if (!strcmp(classInfo->Name, "vtkObjectBase"))
  {
    fprintf(fp,
      "  result[\"Message\"] = std::string(\"Call to %s\") + "
      "std::string(\"::\") + methodName + "
      "std::string(\" is "
      "not permitted.\");\n",
      classInfo->Name);
    return;
  }
  // try the superclass
  fprintf(fp,
    "  if (auto f = invoker->GetHandler(typeid(%s::Superclass)))\n"
    "  {\n"
    "    result = f(invoker, objectBase, methodName, args);\n"
    "  }\n",
    classInfo->Name);
  fprintf(fp, "  if (result[\"Success\"]) { return result; }\n");
  // if the superclass handler did not return a successful result, we need to
  // handle the method ourselves.
  int functionId = 0, functionId2 = 0;
  FunctionInfo* theFunc = NULL;
  int* generatedFunctionCalls = (int*)calloc(classInfo->NumberOfFunctions, sizeof(int));
  int generateDefaultBlock = 0;
  int switchCaseStarted = 0;
  /* Loop through all functions in the class */
  for (functionId = 0; functionId < classInfo->NumberOfFunctions; ++functionId)
  {
    theFunc = classInfo->Functions[functionId];
    const char* rejectReason = NULL;
    int rejectedParameterId = -1;
    /* Skip unallowable function */
    if (!vtkWrapSerDes_IsFunctionAllowed(
          theFunc, classInfo, hinfo, &rejectReason, &rejectedParameterId))
    {
      continue;
    }
    /* Skip function whose calling code was already generated */
    if (generatedFunctionCalls[functionId])
    {
      continue;
    }

    if (!switchCaseStarted)
    {
      fprintf(fp, "  using namespace vtk::literals;\n");
      fprintf(fp, "  const vtkStringToken methodToken(methodName);\n");
      fprintf(fp, "  switch(methodToken.GetId())\n");
      fprintf(fp, "  {\n");
      switchCaseStarted = 1;
      generateDefaultBlock = 1;
    }
    fprintf(fp, "    case \"%s\"_hash:\n", theFunc->Name);
    fprintf(
      fp, "      Invoke_%s_%s(invoker, object, args, result);\n", classInfo->Name, theFunc->Name);
    fprintf(fp, "      break;\n");
    generatedFunctionCalls[functionId] = 1;
    // mark all overloads as generated
    for (functionId2 = 0; functionId2 < classInfo->NumberOfFunctions; ++functionId2)
    {
      /* Skip function same as outer function */
      if (functionId == functionId2)
      {
        continue;
      }
      if (!strcmp(theFunc->Name, classInfo->Functions[functionId2]->Name))
      {
        generatedFunctionCalls[functionId2] = 1;
      }
    }
  }
  free(generatedFunctionCalls);
  if (generateDefaultBlock)
  {
    fprintf(fp,
      "    default:\n"
      "    {\n"
      "      if (result[\"Message\"].get<std::string>().empty())\n"
      "      {\n"
      "        result[\"Message\"] = std::string(\"No such method exists %s::\") + "
      "(methodName ? std::string(methodName) : \"null\");\n"
      "      }\n"
      "      result[\"Success\"] = false;\n"
      "      break;\n"
      "    }\n"
      "  }\n",
      classInfo->Name);
  }
  else
  {
    fprintf(fp, "  (void)object;\n");
  }
}

// NOLINTEND(bugprone-unsafe-functions)
