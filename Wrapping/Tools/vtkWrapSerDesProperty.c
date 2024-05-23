// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/* Returns true if the property can be serialized and later deserialized*/
#include "vtkWrapSerDesProperty.h"
#include "vtkParseData.h"
#include "vtkParseExtras.h"
#include "vtkParseProperties.h"
#include "vtkWrap.h"

#include <stdlib.h>
#include <string.h>

#define callSetterBeginMacro(fp, indent) fprintf(fp, "%sobject->%s(", indent, setterName)

#define callSetterParameterMacro(fp, ...) fprintf(fp, __VA_ARGS__);

#define callSetterNextParameterMacro(fp, ...) fprintf(fp, ", " __VA_ARGS__);

#define callSetterEndMacro(fp) fprintf(fp, ");\n");

/* test whether all types in testTypes exist in methodTypes */
static int vtkWrapSerDes_MethodTypeMatches(
  const unsigned int methodTypes, const unsigned int testTypes)
{
  int i = 0;
  int allTestTypesMatch = 1;
  for (i = 0; i <= VTK_METHOD_MAX_MSB_POSITION; i++)
  {
    unsigned int methodType = (1u << i);
    if ((methodType & testTypes) == methodType)
    {
      if ((methodType & methodTypes) == methodType)
      {
        allTestTypesMatch &= 1;
      }
      else
      {
        allTestTypesMatch &= 0;
      }
    }
  }
  return allTestTypesMatch;
}

/* -------------------------------------------------------------------- */
static int vtkWrapSerDes_IsCollectionLike(const unsigned int methodType)
{
  return vtkWrapSerDes_MethodTypeMatches(methodType,
           VTK_METHOD_GET_IDX | VTK_METHOD_REMOVE_ALL | VTK_METHOD_ADD | VTK_METHOD_REMOVE) ||
    // vtkCollection has an iterator api that can be used instead of GetItem(idx)
    vtkWrapSerDes_MethodTypeMatches(
      methodType, VTK_METHOD_ADD | VTK_METHOD_REMOVE | VTK_METHOD_REMOVE_ALL);
}

/* -------------------------------------------------------------------- */
static int vtkWrapSerDes_IsCollectionLikeNoDiscard(const unsigned int methodType)
{
  return vtkWrapSerDes_MethodTypeMatches(methodType,
           VTK_METHOD_GET_IDX | VTK_METHOD_REMOVE_ALL | VTK_METHOD_ADD_NODISCARD |
             VTK_METHOD_REMOVE_NODISCARD) ||
    // vtkCollection has an iterator api that can be used instead of GetItem(idx)
    vtkWrapSerDes_MethodTypeMatches(
      methodType, VTK_METHOD_ADD_NODISCARD | VTK_METHOD_REMOVE_NODISCARD | VTK_METHOD_REMOVE_ALL);
}

/* -------------------------------------------------------------------- */
static int vtkWrapSerDes_IsSerializable(const unsigned int methodType)
{
  return vtkWrapSerDes_MethodTypeMatches(methodType, VTK_METHOD_GET) ||
    vtkWrapSerDes_MethodTypeMatches(methodType, VTK_METHOD_GET_RHS) ||
    vtkWrapSerDes_MethodTypeMatches(methodType, VTK_METHOD_GET_MULTI) ||
    vtkWrapSerDes_MethodTypeMatches(methodType, VTK_METHOD_ADD) ||
    vtkWrapSerDes_MethodTypeMatches(methodType, VTK_METHOD_REMOVE) ||
    vtkWrapSerDes_MethodTypeMatches(methodType, VTK_METHOD_ADD_NODISCARD) ||
    vtkWrapSerDes_MethodTypeMatches(methodType, VTK_METHOD_REMOVE_NODISCARD);
}

/* -------------------------------------------------------------------- */
static int vtkWrapSerDes_IsDeserializable(const unsigned int methodType)
{
  return vtkWrapSerDes_MethodTypeMatches(methodType, VTK_METHOD_SET) ||
    vtkWrapSerDes_MethodTypeMatches(methodType, VTK_METHOD_SET_MULTI) ||
    vtkWrapSerDes_MethodTypeMatches(methodType, VTK_METHOD_ADD) ||
    vtkWrapSerDes_MethodTypeMatches(methodType, VTK_METHOD_REMOVE) ||
    vtkWrapSerDes_MethodTypeMatches(methodType, VTK_METHOD_ADD_NODISCARD) ||
    vtkWrapSerDes_MethodTypeMatches(methodType, VTK_METHOD_REMOVE_NODISCARD);
}

/* -------------------------------------------------------------------- */
/* If property type or name is to be excluded from marshalling, return 0 */
static int vtkWrapSerDes_IsAllowable(const HierarchyInfo* hinfo, const FunctionInfo* functionInfo,
  const PropertyInfo* propertyInfo, const char** reason)
{
  const int ALLOWABLE = 1;
  const int UNALLOWABLE = 0;
  /* Check if this property is to be skipped */
  if (functionInfo->IsMarshalExcluded)
  {
    *reason = functionInfo->MarshalExcludeReason;
    return UNALLOWABLE;
  }

  /* Check whether a custom property-function mapping is declared */
  if (functionInfo->MarshalPropertyName != NULL)
  {
    return ALLOWABLE;
  }

  unsigned int methBitFlags = propertyInfo->PublicMethods;
  /* property must be accessible through a public method */
  if (methBitFlags == 0)
  {
    *reason = "Property does not have public getter and setter functions.";
    return UNALLOWABLE;
  }
  /* these are allowable method types */
  if (vtkWrapSerDes_MethodTypeMatches(methBitFlags, VTK_METHOD_GET | VTK_METHOD_SET) ||
    vtkWrapSerDes_MethodTypeMatches(methBitFlags, VTK_METHOD_GET_MULTI | VTK_METHOD_SET_MULTI) ||
    vtkWrapSerDes_MethodTypeMatches(methBitFlags, VTK_METHOD_GET_RHS | VTK_METHOD_SET_MULTI) ||
    vtkWrapSerDes_MethodTypeMatches(methBitFlags, VTK_METHOD_GET_RHS | VTK_METHOD_SET) ||
    vtkWrapSerDes_IsCollectionLike(methBitFlags) ||
    vtkWrapSerDes_IsCollectionLikeNoDiscard(methBitFlags))
  {
    return ALLOWABLE;
  }
  else
  {
    /* or a GET and derived from vtkCollection or vtkDataSetAttributes */
    if (vtkWrapSerDes_MethodTypeMatches(methBitFlags, VTK_METHOD_GET))
    {
      if (vtkWrap_IsTypeOf(hinfo, propertyInfo->ClassName, "vtkCollection") ||
        vtkWrap_IsTypeOf(hinfo, propertyInfo->ClassName, "vtkDataSetAttributes"))
      {
        return ALLOWABLE;
      }
      else
      {
        *reason = "Unsupported methBitFlags or the property type is not a vtkCollection or "
                  "vtkDataSetAttributes";
      }
    }
    else
    {
      *reason = "Unsupported methBitflags";
    }
  }
  return UNALLOWABLE;
}

/* -------------------------------------------------------------------- */
/* Useful to understand why (de)serializers were not generated for a property. */
void vtkWrapSerDes_WriteBitField(FILE* fp, unsigned int methodBitfield)
{
  unsigned int i;
  unsigned int methodType;
  int first = 1;
  fprintf(fp, "methBitflags=");
  for (i = 0; i < 32; i++)
  {
    methodType = methodBitfield & (1U << i);
    if (methodType)
    {
      if ((methodType & VTK_METHOD_SET_CLAMP) != 0 &&
        vtkWrapSerDes_MethodTypeMatches(methodBitfield, VTK_METHOD_SET_CLAMP))
      {
        methodType = VTK_METHOD_SET_CLAMP;
        methodBitfield &= ~VTK_METHOD_SET_CLAMP;
      }
      else if ((methodType & VTK_METHOD_SET_BOOL) != 0 &&
        vtkWrapSerDes_MethodTypeMatches(methodBitfield, VTK_METHOD_SET_BOOL))
      {
        methodType = VTK_METHOD_SET_BOOL;
        methodBitfield &= ~VTK_METHOD_SET_BOOL;
      }
      fprintf(
        fp, "%s%s", ((first == 0) ? "|" : ""), vtkParseProperties_MethodTypeAsString(methodType));
      first = 0;
    }
  }
}

/* -------------------------------------------------------------------- */
void vtkWrapSerDes_WriteSerializerVTKObject(
  FILE* fp, const int isConst, const int isVTKSmartPointer)
{
  if (isConst)
  {
    fprintf(fp,
      "serializer->SerializeJSON("
      "const_cast<vtkObjectBase*>(reinterpret_cast<const vtkObjectBase*>("
      "value%s"
      ")))",
      isVTKSmartPointer ? ".Get()" : "");
  }
  else
  {
    fprintf(fp,
      "serializer->SerializeJSON("
      "reinterpret_cast<vtkObjectBase*>("
      "value%s"
      "))",
      isVTKSmartPointer ? ".Get()" : "");
  }
}

/* -------------------------------------------------------------------- */
void vtkWrapSerDes_WriteSerializerVectorOfVTKObjects(FILE* fp, const char* getterFunctionName,
  const char* getterArgs, const char* keyName, const int propCount, const int isConst,
  const int isVTKSmartPointer, const char* indent)
{
  fprintf(
    fp, "%sconst auto& values = object->Get%s(%s);\n", indent, getterFunctionName, getterArgs);
  fprintf(fp, "%sauto& dst = state[\"%s\"] = json::array();\n", indent, keyName);
  if (isConst)
  {
    fprintf(fp,
      "%sfor (size_t i = 0; i < %d; ++i)\n"
      "%s{\n"
      "%s  "
      "dst.emplace_back(serializer->SerializeJSON(const_cast<vtkObjectBase*>(reinterpret_cast<"
      "const vtkObjectBase*>(values[i]%s))));\n"
      "%s}\n",
      indent, propCount, indent, indent, isVTKSmartPointer ? ".Get()" : "", indent);
  }
  else
  {
    fprintf(fp,
      "%sfor (size_t i = 0; i < %d; ++i)\n"
      "%s{\n"
      "%s  "
      "dst.emplace_back(serializer->SerializeJSON(reinterpret_cast<vtkObjectBase*>(values[i]%s)));"
      "\n"
      "%s}\n",
      indent, propCount, indent, indent, isVTKSmartPointer ? ".Get()" : "", indent);
  }
}

/* -------------------------------------------------------------------- */
// Allocates memory for 1 ValueInfo object. Caller is responsible to free it after use.
ValueInfo* vtkWrapSerDes_ValueInfoFromPropertyInfo(const PropertyInfo* propertyInfo)
{
  ValueInfo* propertyValueInfo = (ValueInfo*)calloc(1, sizeof(ValueInfo));
  propertyValueInfo->Class = propertyInfo->ClassName;
  propertyValueInfo->Name = propertyInfo->Name;
  propertyValueInfo->Count = propertyInfo->Count;
  propertyValueInfo->Type = propertyInfo->Type;
  propertyValueInfo->IsStatic = propertyInfo->IsStatic;
  propertyValueInfo->Comment = propertyInfo->Comment;
  return propertyValueInfo;
}

/* -------------------------------------------------------------------- */
int vtkWrapSerDes_WritePropertySerializer(FILE* fp, const ClassInfo* classInfo,
  const HierarchyInfo* hinfo, const FunctionInfo* functionInfo, const unsigned int methodType,
  const PropertyInfo* propertyInfo)
{
  if (!vtkWrapSerDes_IsSerializable(methodType))
  {
    return 0;
  }

  int i = 0;
  const int isMappedProperty = functionInfo->MarshalPropertyName != NULL;
  const int isRHSGetter = vtkWrapSerDes_MethodTypeMatches(methodType, VTK_METHOD_GET_RHS);
  const int isMultiGetter = vtkWrapSerDes_MethodTypeMatches(methodType, VTK_METHOD_GET_MULTI);

  ValueInfo* propertyValueInfo = vtkWrapSerDes_ValueInfoFromPropertyInfo(propertyInfo);
  const int isVTKObject = vtkWrap_IsVTKObjectBaseType(hinfo, propertyInfo->ClassName);
  const int isVTKSmartPointer = vtkWrap_IsVTKSmartPointer(propertyValueInfo);
  const int isScalar = vtkWrap_IsScalar(propertyValueInfo);
  const int isNumeric = vtkWrap_IsNumeric(propertyValueInfo);
  const int isString = vtkWrap_IsString(propertyValueInfo);
  const int isCharPointer = vtkWrap_IsCharPointer(propertyValueInfo);
  const int isArray = vtkWrap_IsArray(propertyValueInfo);
  const int isStdVector = vtkWrap_IsStdVector(propertyValueInfo);
  const int isEnumMember = vtkWrap_IsEnumMember(classInfo, propertyValueInfo);
  const int isEnum = functionInfo->ReturnValue->IsEnum;
  const int isConst = vtkWrap_IsConst(propertyValueInfo);
  free(propertyValueInfo);
  propertyValueInfo = NULL;

  const char* getterName = functionInfo->Name;
  const char* keyName = isMappedProperty ? functionInfo->MarshalPropertyName : propertyInfo->Name;

  if (isRHSGetter && propertyInfo->Count > 0)
  {
    // is void GetValues(type*) or void GetValues(type[])
    fprintf(fp, "  {\n");
    fprintf(fp, "    std::vector<%s> values(%d);\n", propertyInfo->ClassName, propertyInfo->Count);
    fprintf(fp, "    object->%s(values.data());\n", getterName);
    fprintf(fp, "    state[\"%s\"] = values;\n", keyName);
    fprintf(fp, "  }\n");
    return 1;
  }
  else if (isMultiGetter)
  {
    fprintf(fp, "  {\n");
    fprintf(fp, "    std::vector<%s> values(%d);\n", propertyInfo->ClassName, propertyInfo->Count);
    fprintf(fp, "    object->%s(values[0]", getterName);
    for (i = 1; i < propertyInfo->Count; ++i)
    {
      fprintf(fp, ", values[%d]", i);
    }
    fprintf(fp, ");\n");
    fprintf(fp, "    state[\"%s\"] = values;\n", keyName);
    fprintf(fp, "  }\n");
    return 1;
  }

  if (!isRHSGetter &&
    (vtkWrapSerDes_IsCollectionLike(propertyInfo->PublicMethods) ||
      vtkWrapSerDes_IsCollectionLikeNoDiscard(propertyInfo->PublicMethods)))
  {
    if (vtkWrap_IsTypeOf(hinfo, classInfo->Name, "vtkCollection"))
    {
      fprintf(fp, "  {\n");
      fprintf(fp, "    auto& dst = state[\"%ss\"] = json::array();\n", keyName);
      fprintf(fp, "    vtkCollectionSimpleIterator cookie;\n");
      fprintf(fp, "    object->InitTraversal(cookie);\n");
      fprintf(fp, "    while (auto* itemAsObject = object->GetNextItemAsObject(cookie))\n");
      fprintf(fp, "    {\n");
      fprintf(fp, "      dst.emplace_back(serializer->SerializeJSON(itemAsObject));\n");
      fprintf(fp, "    }\n");
      fprintf(fp, "  }\n");
      return 1;
    }
    else if (isVTKObject || isVTKSmartPointer)
    {
      fprintf(fp, "  {\n");
      fprintf(fp, "    auto& dst = state[\"%ss\"] = json::array();\n", keyName);
      fprintf(fp, "    auto numItems = object->GetNumberOf%ss();\n", keyName);
      fprintf(fp, "    using IdxType = decltype(numItems);\n");
      fprintf(fp, "    for (IdxType i = 0; i < numItems; ++i)\n");
      fprintf(fp, "    {\n");
      fprintf(fp, "      dst.emplace_back(serializer->SerializeJSON(");
      fprintf(fp, "reinterpret_cast<vtkObjectBase*>(object->Get%s(i))", keyName);
      fprintf(fp, "));\n");
      fprintf(fp, "    }\n");
      fprintf(fp, "  }\n");
      return 1;
    }
    else
    {
      return 0;
    }
  }

  if (isVTKObject)
  {
    if (isRHSGetter)
    {
      fprintf(fp, "  {\n");
      fprintf(fp, "    auto value = %s::New();\n", propertyInfo->ClassName);
      // get id of the object currently being serialized.
      fprintf(fp, "    const auto identifier = serializer->GetContext()->GetId(object);\n");
      fprintf(fp,
        "    const auto objectState = "
        "serializer->GetContext()->States().find(std::to_string(identifier));\n");
      // edit the current object's state
      fprintf(fp, "    if (objectState != serializer->GetContext()->States().end())\n");
      fprintf(fp, "    {\n");
      fprintf(fp,
        "      const auto subId = objectState->at(\"%s\").at(\"Id\").get<vtkTypeUInt32>();\n",
        keyName);
      fprintf(fp, "      serializer->GetContext()->UnRegisterState(subId);\n");
      fprintf(fp, "    }\n");
      // keep new object alive
      fprintf(fp, "    serializer->GetContext()->KeepAlive(\"%s\", value);\n", classInfo->Name);
      fprintf(fp, "    object->%s(value);\n", getterName);
      fprintf(fp, "    state[\"%s\"] = ", keyName);
      vtkWrapSerDes_WriteSerializerVTKObject(fp, isConst, isVTKSmartPointer);
      fprintf(fp, ";\n");
      fprintf(fp, "    value->Delete();\n");
      fprintf(fp, "  }\n");
    }
    else
    {
      fprintf(fp, "  {\n");
      fprintf(fp, "    auto value = object->%s();\n", getterName);
      fprintf(fp, "    if (value)\n");
      fprintf(fp, "    {\n");
      fprintf(fp, "      state[\"%s\"] = ", keyName);
      vtkWrapSerDes_WriteSerializerVTKObject(fp, isConst, isVTKSmartPointer);
      fprintf(fp, ";\n");
      fprintf(fp, "    }\n");
      fprintf(fp, "  }\n");
    }
    return 1;
  }
  else if (isNumeric)
  {
    if (isScalar || isStdVector)
    {
      fprintf(fp, "  state[\"%s\"] = ", keyName);
      fprintf(fp, "object->%s();\n", getterName);
      return 1;
    }
    else if (isArray)
    {
      fprintf(fp, "  if(auto ptr = object->%s())\n", getterName);
      fprintf(fp, "  {\n");
      fprintf(fp, "    auto& dst = state[\"%s\"] = json::array();\n", keyName);
      fprintf(
        fp, "    for (int i = 0; i < %d; ++i) { dst.push_back(ptr[i]); }\n", propertyInfo->Count);
      fprintf(fp, "  }\n");
      return 1;
    }
    else if (isCharPointer)
    {
      fprintf(fp, "  if (auto ptr = object->%s()) { state[\"%s\"] = ptr; }\n", getterName, keyName);
      return 1;
    }
  }
  else if (isString || (isStdVector && isString))
  {
    fprintf(fp, "  state[\"%s\"] = ", keyName);
    fprintf(fp, "object->%s();\n", getterName);
    return 1;
  }
  else if (isEnumMember)
  {
    fprintf(fp, "  state[\"%s\"] = ", keyName);
    fprintf(fp, "static_cast<std::underlying_type<%s::%s>::type>(object->%s());\n", classInfo->Name,
      propertyInfo->ClassName, getterName);
    return 1;
  }
  else if (isEnum)
  {
    fprintf(fp, "  state[\"%s\"] = ", keyName);
    const char* cp = functionInfo->ReturnValue->Class;
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
      fprintf(fp, "static_cast<std::underlying_type<%*.*s::%s>::type>(object->%s());\n", (int)l,
        (int)l, cp, &cp[l + 2], getterName);
    }
    else
    {
      fprintf(fp, "static_cast<std::underlying_type<%s>::type>(object->%s());\n", cp, getterName);
    }
    return 1;
  }
  else if (strncmp(propertyInfo->ClassName, "vtkVector", 9) == 0 ||
    strncmp(propertyInfo->ClassName, "vtkTuple", 8) == 0 ||
    strncmp(propertyInfo->ClassName, "vtkColor", 8) == 0 ||
    strncmp(propertyInfo->ClassName, "vtkRect", 7) == 0)
  {
    fprintf(fp, "  {\n");
    fprintf(fp, "    const auto& values = object->%s();\n", getterName);
    fprintf(fp, "    auto& dst = state[\"%s\"] = json::array();\n", keyName);
    fprintf(fp, "    for (int i = 0; i < values.GetSize(); ++i) { dst.push_back(values[i]); }\n");
    fprintf(fp, "  }\n");
    return 1;
  }
  else if (!strcmp(propertyInfo->ClassName, "vtkBoundingBox"))
  {
    fprintf(fp, "  {\n");
    fprintf(fp, "    const auto& bbox = object->%s();\n", getterName);
    fprintf(fp, "    auto& dstObject = state[\"%s\"] = json::object();\n", keyName);
    fprintf(fp, "    dstObject[\"ClassName\"] = \"%s\";\n", propertyInfo->ClassName);
    fprintf(fp, "    auto& dst = dstObject[\"Bounds\"] = json::array();\n");
    fprintf(fp, "    for (size_t i = 0; i < 6; ++i) { dst.push_back(bbox.GetBounds()[i]); }\n");
    fprintf(fp, "  }\n");
    return 1;
  }
  else if (isStdVector)
  {
    char* arg = vtkWrap_TemplateArg(propertyInfo->ClassName);
    size_t n;
    ValueInfo* element = (ValueInfo*)calloc(1, sizeof(ValueInfo));
    size_t l = vtkParse_BasicTypeFromString(arg, &(element->Type), &(element->Class), &n);
    (void)l;
    /* check that type is a string or real or integer */
    if (vtkWrap_IsString(element) || vtkWrap_IsRealNumber(element) || vtkWrap_IsInteger(element))
    {
      fprintf(fp, "  state[\"%s\"] = ", keyName);
      fprintf(fp, "object->%s();\n", getterName);
      free(element);
      return 1;
    }
    free(element);
  }
  // __builtin_debugtrap();
  // __builtin_trap();
  fprintf(stderr,
    "Uh oh, the property %s::%s cannot be serialized. Please create an issue at "
    "https://gitlab.kitware.com/vtk/vtk/-/issues/new\n",
    classInfo->Name, propertyInfo->Name);
  exit(1);
}

/* -------------------------------------------------------------------- */
int vtkWrapSerDes_WritePropertyDeserializer(FILE* fp, const ClassInfo* classInfo,
  const HierarchyInfo* hinfo, const FunctionInfo* functionInfo, const unsigned int methodType,
  const PropertyInfo* propertyInfo)
{
  const int isMappedProperty = functionInfo->MarshalPropertyName != NULL;
  const char* keyName = isMappedProperty ? functionInfo->MarshalPropertyName : propertyInfo->Name;
  if (!vtkWrapSerDes_IsDeserializable(methodType))
  {
    if (vtkWrap_IsTypeOf(hinfo, propertyInfo->ClassName, "vtkCullerCollection") ||
      vtkWrap_IsTypeOf(hinfo, propertyInfo->ClassName, "vtkLightCollection") ||
      vtkWrap_IsTypeOf(hinfo, propertyInfo->ClassName, "vtkPropCollection") ||
      vtkWrap_IsTypeOf(hinfo, propertyInfo->ClassName, "vtkRendererCollection") ||
      vtkWrap_IsTypeOf(hinfo, propertyInfo->ClassName, "vtkDataSetAttributes"))
    {
      // These types are not settable on any instance.
      // For example:
      //  `vtkPolyData::GetPointData()` exists, but there is no `SetPointData`.
      //  The owning type, vtkPolyData, in this exmaple constructs and returns an instance of point
      //  data. Similar story for vtkRenderer::Cullers, vtkViewPort::ViewProps, etc.
      // To overcome the absence of a setter, this code retrieves the instance and registers it
      // as a weak reference before deserializing it.
      fprintf(fp, "  {\n");
      fprintf(fp, "    auto iter = state.find(\"%s\");\n", keyName);
      fprintf(fp, "    if ((iter != state.end()) && !iter->is_null())\n");
      fprintf(fp, "    {\n");
      fprintf(fp,
        "      auto* context = deserializer->GetContext();\n"
        "      const auto identifier = iter->at(\"Id\").get<vtkTypeUInt32>();\n"
        "      vtkSmartPointer<vtkObjectBase> subObject = "
        "reinterpret_cast<vtkObjectBase*>(object->Get%s());\n",
        propertyInfo->Name);
      fprintf(fp,
        "      if (subObject == nullptr)\n"
        "      {\n"
        "        vtkErrorWithObjectMacro(context, << \"An internal collection object "
        "is "
        "null!\");\n"
        "      }\n"
        "      else"
        "      {\n"
        "        if (context->GetObjectAtId(identifier) != subObject)\n"
        "        {\n"
        "          auto registrationId = identifier;\n"
        "          context->RegisterObject(subObject, registrationId);\n"
        "        }\n"
        "        deserializer->DeserializeJSON(identifier, subObject);\n"
        "      }\n");
      fprintf(fp, "    }\n");
      fprintf(fp, "  }\n");
      return 1;
    }
    else
    {
      return 0;
    }
  }

  const char* setterName = functionInfo->Name;

  ValueInfo* val = calloc(1, sizeof(ValueInfo));
  val->Type = propertyInfo->Type;
  val->Class = propertyInfo->ClassName;
  val->Count = propertyInfo->Count;
  val->IsStatic = propertyInfo->IsStatic;
  const int isVTKObject = vtkWrap_IsVTKObjectBaseType(hinfo, val->Class);
  const int isVTKSmartPointer = vtkWrap_IsVTKSmartPointer(val);
  const int isPointer = vtkWrap_IsPointer(val);
  const int isScalar = vtkWrap_IsScalar(val);
  const int isNumeric = vtkWrap_IsNumeric(val);
  const int isString = vtkWrap_IsString(val);
  const int isCharPointer = vtkWrap_IsCharPointer(val);
  const int isEnumMember = vtkWrap_IsEnumMember(classInfo, val);
  const int isArray = vtkWrap_IsArray(val);
  const int isStdVector = vtkWrap_IsStdVector(val);

  int isEnum = 0;
  if (functionInfo->NumberOfParameters > 0)
  {
    isEnum = functionInfo->Parameters[0]->IsEnum;
  }

  if (vtkWrapSerDes_IsCollectionLike(propertyInfo->PublicMethods) ||
    vtkWrapSerDes_IsCollectionLikeNoDiscard(propertyInfo->PublicMethods))
  {
    if (isVTKObject && (isPointer || isVTKSmartPointer))
    {
      fprintf(fp, "  {\n");
      fprintf(fp, "    auto iter = state.find(\"%ss\");\n", keyName);
      fprintf(fp, "    if ((iter != state.end()) && !iter->is_null())\n");
      fprintf(fp, "    {\n");
      fprintf(fp, "      const auto items = iter->get<nlohmann::json::array_t>();\n");
      fprintf(fp,
        "      const size_t numExistingItems = static_cast<size_t>(object->GetNumberOf%ss());\n",
        keyName);
      fprintf(fp, "      bool populateCollection = numExistingItems == 0;\n");
      fprintf(fp, "      if (items.size() != numExistingItems)\n");
      fprintf(fp, "      {\n");
      fprintf(fp, "        object->RemoveAll%ss();\n", keyName);
      fprintf(fp, "        populateCollection = true;\n");
      fprintf(fp, "      }\n");
      fprintf(fp, "      const auto* context = deserializer->GetContext();\n");
      fprintf(fp, "      for (const auto& item: items)\n");
      fprintf(fp, "      {\n");
      fprintf(fp, "        const auto identifier = item.at(\"Id\").get<vtkTypeUInt32>();\n");
      fprintf(fp, "        auto subObject = context->GetObjectAtId(identifier);\n");
      fprintf(fp, "        deserializer->DeserializeJSON(identifier, subObject);\n");
      fprintf(fp, "        if (populateCollection && subObject != nullptr)\n");
      fprintf(fp, "        {\n");
      fprintf(fp, "          auto* itemAsObject = vtkObject::SafeDownCast(subObject);\n");
      fprintf(fp, "          object->Add%s(reinterpret_cast<%s*>(itemAsObject));\n", keyName,
        propertyInfo->ClassName);
      fprintf(fp, "        }\n");
      fprintf(fp, "      }\n");
      fprintf(fp, "    }\n");
      fprintf(fp, "  }\n");
      return 1;
    }
  }
  else if (isVTKObject && (isPointer || isVTKSmartPointer))
  {
    fprintf(fp, "  {\n");
    fprintf(fp, "    auto iter = state.find(\"%s\");\n", keyName);
    fprintf(fp, "    if ((iter != state.end()) && !iter->is_null())\n");
    fprintf(fp, "    {\n");
    fprintf(fp,

      "      const auto* context = deserializer->GetContext();\n"
      "      const auto identifier = iter->at(\"Id\").get<vtkTypeUInt32>();\n"
      "      auto subObject = context->GetObjectAtId(identifier);\n"
      "      deserializer->DeserializeJSON(identifier, subObject);\n"
      "      if (subObject != nullptr)\n");
    fprintf(fp, "      {\n");
    callSetterBeginMacro(fp, "        ");
    callSetterParameterMacro(fp, "static_cast<%s*>(static_cast<void*>(subObject))", val->Class);
    callSetterEndMacro(fp);
    fprintf(fp, "      }\n");
    fprintf(fp, "    }\n");
    fprintf(fp, "  }\n");
    return 1;
  }
  else if (isNumeric)
  {
    fprintf(fp, "  {\n");
    if (isScalar)
    {
      fprintf(fp, "    const auto iter = state.find(\"%s\");\n", keyName);
      fprintf(fp, "    if ((iter != state.end()) && !iter->is_null())\n");
      fprintf(fp, "    {\n");
      callSetterBeginMacro(fp, "      ");
      callSetterParameterMacro(fp, "iter->get<%s>()", propertyInfo->ClassName);
      callSetterEndMacro(fp);
      fprintf(fp, "    }\n");
    }
    else if (isArray)
    {
      fprintf(fp, "    const auto iter = state.find(\"%s\");\n", keyName);
      fprintf(fp, "    if ((iter != state.end()) && !iter->is_null())\n");
      fprintf(fp, "    {\n");
      fprintf(fp, "      auto values = iter->get<std::vector<%s>>();\n", propertyInfo->ClassName);
      if ((propertyInfo->PublicMethods & VTK_METHOD_SET_MULTI) == VTK_METHOD_SET_MULTI)
      {
        callSetterBeginMacro(fp, "      ");
        int i = 0;
        for (i = 0; i < propertyInfo->Count; ++i)
        {
          if (i != 0)
          {
            callSetterNextParameterMacro(fp, "values[%d]", i);
          }
          else
          {
            callSetterParameterMacro(fp, "values[%d]", i);
          }
        }
        callSetterEndMacro(fp);
      }
      else
      {
        callSetterBeginMacro(fp, "      ");
        callSetterParameterMacro(fp, "values.data()");
        callSetterEndMacro(fp);
      }
      fprintf(fp, "    }\n");
    }
    else if (isCharPointer)
    {
      fprintf(fp, "    const auto iter = state.find(\"%s\");\n", keyName);
      fprintf(fp, "    if ((iter != state.end()) && !iter->is_null())\n");
      fprintf(fp, "    {\n");
      fprintf(fp, "      auto values = iter->get<std::string>();\n");
      callSetterBeginMacro(fp, "      ");
      callSetterParameterMacro(fp, "&(values.front())");
      callSetterEndMacro(fp);
      fprintf(fp, "    }\n");
    }
    fprintf(fp, "  }\n");
    return 1;
  }
  else if (isString)
  {
    fprintf(fp, "  {\n");
    fprintf(fp, "    const auto iter = state.find(\"%s\");\n", keyName);
    fprintf(fp, "    if ((iter != state.end()) && !iter->is_null())\n");
    fprintf(fp, "    {\n");
    fprintf(fp, "      auto values = iter->get<std::string>();\n");
    callSetterBeginMacro(fp, "      ");
    callSetterParameterMacro(fp, "&(values.front())");
    callSetterEndMacro(fp);
    fprintf(fp, "    }\n");
    fprintf(fp, "  }\n");
    return 1;
  }
  else if (isEnumMember)
  {
    fprintf(fp, "  {\n");
    fprintf(fp, "    const auto iter = state.find(\"%s\");\n", keyName);
    fprintf(fp, "    if ((iter != state.end()) && !iter->is_null())\n");
    fprintf(fp, "    {\n");
    fprintf(fp,
      "      auto value = "
      "static_cast<%s::%s>(iter->get<std::underlying_type<%s::%s>::type>());\n",
      classInfo->Name, val->Class, classInfo->Name, val->Class);
    callSetterBeginMacro(fp, "      ");
    callSetterParameterMacro(fp, "value");
    callSetterEndMacro(fp);
    fprintf(fp, "    }\n");
    fprintf(fp, "  }\n");
    return 1;
  }
  else if (isEnum)
  {
    fprintf(fp, "  {\n");
    fprintf(fp, "    const auto iter = state.find(\"%s\");\n", keyName);
    fprintf(fp, "    if ((iter != state.end()) && !iter->is_null())\n");
    fprintf(fp, "    {\n");
    const char* cp = functionInfo->Parameters[0]->Class;
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
        "      auto value = "
        "static_cast<%*.*s::%s>(iter->get<std::underlying_type<%*.*s::%s>::type>());\n",
        (int)l, (int)l, cp, &cp[l + 2], (int)l, (int)l, cp, &cp[l + 2]);
    }
    else
    {
      fprintf(fp,
        "      auto value = "
        "static_cast<%s>(iter->get<std::underlying_type<%s>::type>());\n",
        cp, cp);
    }
    callSetterBeginMacro(fp, "      ");
    callSetterParameterMacro(fp, "value");
    callSetterEndMacro(fp);
    fprintf(fp, "    }\n");
    fprintf(fp, "  }\n");
    return 1;
  }
  else if (isStdVector)
  {
    char* arg = vtkWrap_TemplateArg(val->Class);
    size_t n;
    ValueInfo* element = (ValueInfo*)calloc(1, sizeof(ValueInfo));
    size_t l = vtkParse_BasicTypeFromString(arg, &(element->Type), &(element->Class), &n);
    (void)l;
    /* check that type is a string or real or integer */
    if (vtkWrap_IsString(element) || vtkWrap_IsRealNumber(element) || vtkWrap_IsInteger(element))
    {
      fprintf(fp, "  {\n");
      fprintf(fp, "    const auto iter = state.find(\"%s\");\n", keyName);
      fprintf(fp, "    if ((iter != state.end()) && !iter->is_null())\n");
      fprintf(fp, "    {\n");
      fprintf(fp, "      auto values = iter->get<std::vector<%s>>();\n", element->Class);
      callSetterBeginMacro(fp, "      ");
      callSetterParameterMacro(fp, "values");
      callSetterEndMacro(fp);
      fprintf(fp, "    }\n");
      fprintf(fp, "  }\n");
      free(element);
      return 1;
    }
    free(element);
  }
  free(val);
  // __builtin_debugtrap();
  // __builtin_trap();
  fprintf(stderr,
    "Uh oh, the property %s::%s cannot be deserialized. Please create an issue at "
    "https://gitlab.kitware.com/vtk/vtk/-/issues/new\n",
    classInfo->Name, propertyInfo->Name);
  exit(1);
}

/* -------------------------------------------------------------------- */
void vtkWrapSerDes_Properties(
  FILE* fp, ClassInfo* classInfo, const HierarchyInfo* hinfo, WriteProperty* writeFn)
{
  ClassProperties* properties = vtkParseProperties_Create(classInfo, hinfo);
  int i = 0, j = 0;
  unsigned int methodType = 0;
  FunctionInfo* theFunc = NULL;
  PropertyInfo* theProp = NULL;
  int* isWritten = calloc(properties->NumberOfProperties, sizeof(int));
  for (i = 0; i < classInfo->NumberOfFunctions; ++i)
  {
    theFunc = classInfo->Functions[i];
    methodType = properties->MethodTypes[i];
    /* Ignore inaccessible methods*/
    if (!theFunc->IsPublic)
    {
      continue;
    }
    /* Ignore inherited methods */
    if (vtkWrap_IsInheritedMethod(classInfo, theFunc))
    {
      continue;
    }
    /* Is this method associated with a property? */
    if (properties && properties->MethodHasProperty[i])
    {
      /* Get the property associated with this method */
      j = properties->MethodProperties[i];
      if (isWritten[j])
      {
        continue;
      }
      theProp = properties->Properties[j];
      /* Describe the property like signature="...", name="...", type="...", bitfield=GET|SET|... */
      fprintf(fp,
        "  /**      name=\"%s\"\n"
        "   *  signature=\"%s\"\n"
        "   * methodtype=\"%s\"\n"
        "   *  valuetype=\"%s\"\n"
        "   *   ",
        theProp->Name, theFunc->Signature, vtkParseProperties_MethodTypeAsString(methodType),
        theProp->ClassName);
      vtkWrapSerDes_WriteBitField(fp, theProp->PublicMethods);
      const char* skipReason = NULL;
      if (!vtkWrapSerDes_IsAllowable(hinfo, theFunc, theProp, &skipReason) &&
        !theFunc->MarshalPropertyName)
      {
        fprintf(fp, "\n   * - skipped due to reason=%s*/\n", skipReason);
        continue;
      }
      fprintf(fp, "\n   */\n");
      /* Write the property serializer if not skipped */
      isWritten[j] = writeFn(fp, classInfo, hinfo, theFunc, methodType, theProp);
    }
  }
  free(isWritten);
  vtkParseProperties_Free(properties);
}
