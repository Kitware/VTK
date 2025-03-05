// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkWrapSerDesFunction.h"
#include "vtkParseExtras.h"
#include "vtkParseHierarchy.h"
#include "vtkWrap.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

static int vtkWrapSerDes_CanMarshalValue(
  FILE* fp, ValueInfo* valInfo, ClassInfo* classInfo, const HierarchyInfo* hinfo, int isReturnValue)
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
  if (valInfo->Template != NULL)
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
      isAllowed = 1;
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
  if (!isAllowed)
  {
    const int declarationLength = vtkParse_ValueInfoToString(valInfo, NULL, VTK_PARSE_EVERYTHING);
    char* declaration = (char*)malloc((declarationLength + 1) * sizeof(char));
    vtkParse_ValueInfoToString(valInfo, declaration, VTK_PARSE_EVERYTHING);
    fprintf(fp, "  //  Unallowable method:has-rejected-type=%s\n", declaration);
    free(declaration);
  }
  return isAllowed;
}

static int vtkWrapSerDes_IsFunctionAllowed(
  FILE* fp, FunctionInfo* functionInfo, ClassInfo* classInfo, const HierarchyInfo* hinfo)
{
  /* Ignore static methods */
  if (functionInfo->IsStatic)
  {
    fprintf(fp, "  //  Unallowable method:static\n");
    return 0;
  }
  /* Ignore inaccessible methods */
  if (!functionInfo->IsPublic)
  {
    fprintf(fp, "  //  Unallowable method:not public\n");
    return 0;
  }
  /* Ignore template methods */
  if (functionInfo->Template)
  {
    fprintf(fp, "  //  Unallowable method:templated\n");
    return 0;
  }
  /* Return value must be allowable */
  if (!vtkWrapSerDes_CanMarshalValue(
        fp, functionInfo->ReturnValue, classInfo, hinfo, /*isReturnValue=*/1))
  {
    return 0;
  }
  /* Inherited methods and overriden methods are handled by superclasses */
  if (vtkWrap_IsInheritedMethod(classInfo, functionInfo) || functionInfo->IsOverride)
  {
    fprintf(fp, "  //  Unallowable method:inherited\n");
    return 0;
  }
  /* All parameters must be allowable */
  int parameterId = 0;
  for (parameterId = 0; parameterId < functionInfo->NumberOfParameters; ++parameterId)
  {
    ValueInfo* parameterInfo = functionInfo->Parameters[parameterId];
    if (!vtkWrapSerDes_CanMarshalValue(fp, parameterInfo, classInfo, hinfo, /*isReturnValue=*/0))
    {
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

  const char* parameterName = valInfo->Name ? valInfo->Name : "noname";

  fprintf(fp, "      if (args[%d].is_null())\n", paramId);
  fprintf(fp, "      {\n");
  fprintf(fp,
    "        vtkErrorWithObjectMacro(invoker, << \"Expected JSON equivalent of C++ type \'%s\' at "
    "position %d for parameter "
    "\'%s\'\");\n",
    valInfo->Class, paramId, parameterName);
  fprintf(fp, "      }\n");

  if (isVTKObject && isPointer)
  {
    fprintf(fp,
      "      auto objectFromContext_%d = "
      "context->GetObjectAtId(args[%d].get<vtkTypeUInt32>());\n",
      paramId, paramId);
    fprintf(fp,
      "      auto* arg_%d = "
      "reinterpret_cast<%s*>(objectFromContext_%d.GetPointer());\n",
      paramId, valInfo->Class, paramId);
    return;
  }
  if (isVTKSmartPointer)
  {
    char* className = vtkWrapSerDes_SmartPointerTypeTemplateArg(valInfo->Class);
    fprintf(fp,
      "      auto objectFromContext_%d = "
      "context->GetObjectAtId(args[%d].get<vtkTypeUInt32>());\n",
      paramId, paramId);
    fprintf(fp,
      "      auto arg_%d = "
      "reinterpret_cast<%s*>(objectFromContext_%d.GetPointer());\n",
      paramId, className, paramId);
    free(className);
    return;
  }
  if (isNumeric)
  {
    if (isScalar)
    {
      fprintf(fp, "      auto arg_%d = args[%d].get<%s>();\n", paramId, paramId, valInfo->Class);
      return;
    }
    if (isArray)
    {
      fprintf(fp, "      auto elements_%d = args[%d].get<std::vector<%s>>();\n", paramId, paramId,
        valInfo->Class);
      fprintf(fp, "      auto* arg_%d = elements_%d.data();\n", paramId, paramId);
      return;
    }
    if (isCharPointer)
    {
      fprintf(fp, "      auto elements_%d = args[%d].get<std::string>();\n", paramId, paramId);
      fprintf(fp, "      auto* arg_%d = elements_%d.data();\n", paramId, paramId);
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
    fprintf(fp, "      auto arg_%d = args[%d].get<std::string>();\n", paramId, paramId);
    return;
  }
  if (isEnumMember)
  {
    fprintf(fp,
      "      auto arg_%d = "
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
        "      auto arg_%d = "
        "static_cast<%*.*s::%s>(args[%d].get<std::underlying_type<%*.*s::%s>::type>());\n",
        paramId, (int)l, (int)l, cp, &cp[l + 2], paramId, (int)l, (int)l, cp, &cp[l + 2]);
    }
    else
    {
      fprintf(fp,
        "      auto arg_%d = static_cast<%s>(args[%d].get<std::underlying_type<%s>::type>());\n",
        paramId, cp, paramId, cp);
    }
    return;
  }
  if (!strncmp(valInfo->Class, "vtkVector", 9) || !strncmp(valInfo->Class, "vtkTuple", 8) ||
    !strncmp(valInfo->Class, "vtkColor", 8) || !strncmp(valInfo->Class, "vtkRect", 7))
  {
    char* elementType = NULL;
    const int elementCount = vtkWrapSerDes_DecomposeTemplatedTuple(valInfo, &elementType, hinfo);
    fprintf(fp, "      if (args[%d].size() != %d)\n", paramId, elementCount);
    fprintf(fp, "      {\n");
    fprintf(fp,
      "         vtkErrorWithObjectMacro(invoker, << \"Expected \" << %d << \"-element "
      "vector at "
      "position %d for parameter "
      "\'%s\'\");\n",
      elementCount, paramId, parameterName);
    fprintf(fp, "        return {{\"Success\", false}};\n");
    fprintf(fp, "      }\n");
    fprintf(fp, "      auto elements_%d = args[%d].get<std::array<%s, %d>>();\n", paramId, paramId,
      elementType, elementCount);
    fprintf(fp, "      %s arg_%d{elements_%d.data()};\n", valInfo->Class, paramId, paramId);
    vtkWrapSerDes_FreeTemplatedTupleDecomposition(&elementType);
    return;
  }
  if (!strcmp(valInfo->Class, "vtkBoundingBox"))
  {
    fprintf(fp, "      if (args[%d].size() != 6)\n", paramId);
    fprintf(fp, "      {\n");
    fprintf(fp,
      "         vtkErrorWithObjectMacro(invoker, << \"Expected 6-element vector<%s> at "
      "position %d for parameter "
      "\'%s\'\");\n",
      valInfo->Class, paramId, parameterName);
    fprintf(fp, "        return {{\"Success\", false}};\n");
    fprintf(fp, "      }\n");
    fprintf(
      fp, "      auto elements_%d = args[%d].get<std::array<double, 6>>();\n", paramId, paramId);
    fprintf(fp, "      vtkBoundingBox arg_%d{elements_%d.data()};\n", paramId, paramId);
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
      fprintf(fp, "      auto arg_%d = args[%d].get<std::vector<%s>>();\n", paramId, paramId,
        element->Class);
      free(element);
      return;
    }
    else if (vtkWrap_IsVTKObjectBaseType(hinfo, element->Class))
    {
      fprintf(fp, "      std::vector<%s> arg_%d;\n", element->Class, paramId);
      fprintf(fp, "      auto arg_%d_ids = args[%d].get<std::vector<vtkTypeUInt32>>();\n", paramId,
        paramId);
      fprintf(fp, "      for(const auto& id: arg_%d_ids)\n", paramId);
      fprintf(fp, "      {\n");
      fprintf(fp, "        auto objectFromContext = context->GetObjectAtId(id);\n");
      fprintf(fp,
        "        arg_%d.emplace_back(reinterpret_cast<%s>(objectFromContext.GetPointer()));\n",
        paramId, element->Class);
      fprintf(fp, "      }\n");
      free(element);
      return;
    }
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

  fprintf(fp, "      json result;\n");
  if (isVTKObject && isPointer)
  {
    fprintf(fp,
      "      vtkTypeUInt32 identifier = "
      "context->GetId(reinterpret_cast<vtkObjectBase*>(methodReturnValue));\n");
    fprintf(fp,
      "      if (identifier == 0)\n"
      "      {\n"
      "        context->RegisterObject(reinterpret_cast<vtkObjectBase*>(methodReturnValue), "
      "identifier);\n"
      "      }\n");
    if (vtkWrap_IsNewInstance(valInfo))
    {
      // Manage the new instance with object manager.
      fprintf(fp,
        "      context->KeepAlive(invoker->GetObjectDescription(), "
        "reinterpret_cast<vtkObjectBase*>(methodReturnValue));\n");
    }
    fprintf(fp, "      result[\"Id\"] = identifier;\n");
    return;
  }
  if (isVTKSmartPointer)
  {
    fprintf(fp,
      "      vtkTypeUInt32 identifier = "
      "context->GetId(reinterpret_cast<vtkObjectBase*>(methodReturnValue.GetPointer()"
      "));\n");
    fprintf(fp,
      "      if (identifier == 0) { "
      "context->RegisterObject(reinterpret_cast<vtkObjectBase*>(methodReturnValue."
      "GetPointer()), identifier); }");
    if (vtkWrap_IsNewInstance(valInfo))
    {
      // Manage the new instance with object manager.
      fprintf(fp,
        "      context->KeepAlive(invoker->GetObjectDescription(), "
        "reinterpret_cast<vtkObjectBase*>(methodReturnValue.GetPointer()));\n");
    }
    fprintf(fp, "      result[\"Id\"] = identifier;\n");
    return;
  }
  if (isNumeric)
  {
    if (isScalar)
    {
      fprintf(fp, "      result[\"Value\"] = methodReturnValue;\n");
      return;
    }
    if (isArray)
    {
      fprintf(fp, "      if(methodReturnValue != nullptr)\n");
      fprintf(fp, "      {\n");
      fprintf(fp, "        auto& dst = result[\"Value\"] = json::array();\n");
      fprintf(fp, "        for (int i = 0; i < %d; ++i) { dst.push_back(methodReturnValue[i]); }\n",
        valInfo->Count);
      fprintf(fp, "      }\n");
      return;
    }
    if (isCharPointer)
    {
      fprintf(
        fp, "      if (methodReturnValue != nullptr) { result[\"Value\"] = methodReturnValue; }\n");
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
      fprintf(fp, "      result[\"Value\"] = std::string(methodReturnValue);\n");
    }
    else
    {
      fprintf(fp, "      result[\"Value\"] = methodReturnValue;\n");
    }
    return;
  }
  if (isEnumMember)
  {
    fprintf(fp,
      "      result[\"Value\"] = "
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
      fprintf(fp, "      result[\"Value\"] = static_cast<%*.*s::%s>(methodReturnValue);\n", (int)l,
        (int)l, cp, &cp[l + 2]);
    }
    else
    {
      fprintf(fp, "      result[\"Value\"] = static_cast<%s>(methodReturnValue);\n", cp);
    }
    return;
  }
  if (!strncmp(valInfo->Class, "vtkVector", 9) || !strncmp(valInfo->Class, "vtkTuple", 8) ||
    !strncmp(valInfo->Class, "vtkColor", 8) || !strncmp(valInfo->Class, "vtkRect", 7))
  {
    fprintf(fp, "      {\n");
    fprintf(fp, "        auto& dst = result[\"Value\"] = json::array();\n");
    fprintf(fp,
      "        for (int i = 0; i < methodReturnValue.GetSize(); ++i) { "
      "dst.push_back(methodReturnValue[i]); }\n");
    fprintf(fp, "      }\n");
    return;
  }
  if (!strcmp(valInfo->Class, "vtkBoundingBox"))
  {
    fprintf(fp, "      {\n");
    fprintf(fp, "        auto& dst = result[\"Value\"] = json::array();\n");
    fprintf(fp, "        double bounds[6] = {};\n");
    fprintf(fp, "        methodReturnValue.GetBounds(bounds);\n");
    fprintf(fp, "        for (int i = 0; i < 6; ++i) { dst.push_back(bounds[i]); }\n");
    fprintf(fp, "      }\n");
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
        fprintf(fp, "      result[\"Value\"] = *methodReturnValue;\n");
      }
      else
      {
        fprintf(fp, "      result[\"Value\"] = methodReturnValue;\n");
      }
      free(element);
      return;
    }
    else if (vtkWrap_IsVTKObjectBaseType(hinfo, element->Class))
    {
      fprintf(fp, "      auto& dst = result[\"Value\"] = json::array();\n");
      if (isPointer)
      {
        fprintf(fp, "      for (auto* element: *methodReturnValue)\n");
      }
      else
      {
        fprintf(fp, "      for (auto* element: methodReturnValue)\n");
      }
      fprintf(fp,
        "      {\n"
        "        vtkTypeUInt32 identifier = "
        "context->GetId(reinterpret_cast<vtkObjectBase*>(element));\n"
        "        if (identifier == 0)\n"
        "        {\n"
        "          context->RegisterObject(reinterpret_cast<vtkObjectBase*>(element), "
        "identifier);\n"
        "        }\n"
        "        dst.emplace_back(identifier);\n"
        "      }\n");
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

static int vtkWrapSerDes_WriteMemberFunctionCall(
  FILE* fp, ClassInfo* classInfo, FunctionInfo* functionInfo, const HierarchyInfo* hinfo)
{
  int i = 0;
  fprintf(fp, "    // Call %s\n", functionInfo->Signature);
  if (functionInfo->NumberOfParameters > 0)
  {
    fprintf(fp, "    if (args.size() == %d)\n", functionInfo->NumberOfParameters);
  }
  else
  {
    // avoids -Wreadability-container-size-empty
    fprintf(fp, "    if (args.empty())\n");
  }
  fprintf(fp, "    {\n");
  for (i = 0; i < functionInfo->NumberOfParameters; ++i)
  {
    // Retrieve individual arguments to stack and call method on object with arguments.
    // Then pack the result in json and return the json object. Handle errors by storing message
    // in the result json and set Success = false
    // Return json: {"Value" (or) "Id": value, "Success": false/true, "message": "Failed to parse
    // args ... etc."}

    ValueInfo* paramInfo = functionInfo->Parameters[i];
    vtkWrapSerDes_WriteArgumentDeserializer(fp, i, paramInfo, classInfo, hinfo);
  }
  fprintf(fp, "      vtkVLogF(invoker->GetInvokerLogVerbosity(), \"Calling %s\");\n",
    functionInfo->Signature);
  const char* argStart = "";
  const char* argEnd = "";
  if (functionInfo->NumberOfParameters > 0)
  {
    argStart = "\n";
    argEnd = "      ";
  }
  if (vtkWrap_IsVoid(functionInfo->ReturnValue))
  {
    fprintf(fp, "      object->%s(%s", functionInfo->Name, argStart);
    for (i = 0; i < functionInfo->NumberOfParameters; ++i)
    {
      ValueInfo* paramInfo = functionInfo->Parameters[i];
      const char* parameterName = paramInfo->Name ? paramInfo->Name : "noname";
      fprintf(fp, "        /*%s=*/arg_%d", parameterName, i);
      if (functionInfo->NumberOfParameters > 1 && i != functionInfo->NumberOfParameters - 1)
      {
        fprintf(fp, ",");
      }
      fprintf(fp, "\n");
    }
    fprintf(fp, "%s);\n", argEnd);
    fprintf(fp, "      return {{\"Success\", true}};\n");
  }
  else
  {
    fprintf(fp, "      auto%s methodReturnValue = object->%s(%s",
      (vtkWrap_IsPointer(functionInfo->ReturnValue) ? "*" : ""), functionInfo->Name, argStart);
    for (i = 0; i < functionInfo->NumberOfParameters; ++i)
    {
      ValueInfo* paramInfo = functionInfo->Parameters[i];
      const char* parameterName = paramInfo->Name ? paramInfo->Name : "noname";
      fprintf(fp, "        /*%s=*/arg_%d", parameterName, i);
      if (functionInfo->NumberOfParameters > 1 && i != functionInfo->NumberOfParameters - 1)
      {
        fprintf(fp, ",\n");
      }
    }
    fprintf(fp, "%s);\n", argEnd);
    vtkWrapSerDes_WriteReturnValueSerializer(fp, classInfo, hinfo, functionInfo->ReturnValue);
    fprintf(fp, "      result[\"Success\"] = true;\n");
    fprintf(fp, "      return result;\n");
  }
  fprintf(fp, "    }\n");
  return 1;
}

void vtkWrapSerDes_Functions(FILE* fp, ClassInfo* classInfo, const HierarchyInfo* hinfo)
{
  int functionId = 0, functionId2 = 0;
  FunctionInfo *theFunc = NULL, *overloadedFunc = NULL;
  int* generatedFunctionCalls = (int*)calloc(classInfo->NumberOfFunctions, sizeof(int));
  for (functionId = 0; functionId < classInfo->NumberOfFunctions; ++functionId)
  {
    theFunc = classInfo->Functions[functionId];
    fprintf(fp, "  // Method:\'%s\'\n", theFunc->Signature);
    /* Skip unallowable function */
    if (!vtkWrapSerDes_IsFunctionAllowed(fp, theFunc, classInfo, hinfo))
    {
      continue;
    }
    /* Skip function whose calling code was already generated */
    if (generatedFunctionCalls[functionId])
    {
      fprintf(fp, "  //  Overload already handled\n");
      continue;
    }

    fprintf(fp, "  if (!strcmp(methodName, \"%s\"))\n", theFunc->Name);
    fprintf(fp, "  {\n");
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
      fprintf(fp, "  // Overload:\'%s\'\n", overloadedFunc->Signature);
      /* Skip unallowable function */
      if (!vtkWrapSerDes_IsFunctionAllowed(fp, overloadedFunc, classInfo, hinfo))
      {
        continue;
      }
      generatedFunctionCalls[functionId2] =
        vtkWrapSerDes_WriteMemberFunctionCall(fp, classInfo, overloadedFunc, hinfo);
    }
    fprintf(fp, "  }\n");
  }
  free(generatedFunctionCalls);
  fprintf(fp, "  return {{\"Success\", false}};\n");
}
