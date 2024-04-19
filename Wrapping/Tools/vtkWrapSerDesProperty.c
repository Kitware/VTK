// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/* Returns true if the property can be serialized and later deserialized*/
#include "vtkWrapSerDesProperty.h"
#include "vtkWrap.h"

#include <stdlib.h>
#include <string.h>

#define callSetterBeginMacro(fp, indent) fprintf(fp, "%sobject->%s(", indent, setterName)

#define callSetterParameterMacro(fp, ...) fprintf(fp, __VA_ARGS__);

#define callSetterNextParameterMacro(fp, ...) fprintf(fp, ", " __VA_ARGS__);

#define callSetterEndMacro(fp) fprintf(fp, ");\n");

/* test whether all types in testTypes exist in methodTypes */
static int vtkSerdesCoder_MethodTypeMatches(
  const unsigned int methodTypes, const unsigned int testTypes)
{
  int i = 0;
  int allTestTypesMatch = 1;
  for (i = 0; i < 28; i++)
  {
    unsigned int methodType = (1U << i);
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
static int vtkWrapSerDes_IsSerializable(const unsigned int methodType)
{
  return vtkSerdesCoder_MethodTypeMatches(methodType, VTK_METHOD_GET) ||
    vtkSerdesCoder_MethodTypeMatches(methodType, VTK_METHOD_GET_MULTI);
}

/* -------------------------------------------------------------------- */
static int vtkWrapSerDes_IsDeserializable(const unsigned int methodType)
{
  return vtkSerdesCoder_MethodTypeMatches(methodType, VTK_METHOD_SET) ||
    vtkSerdesCoder_MethodTypeMatches(methodType, VTK_METHOD_SET_MULTI);
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

  unsigned int methodTypes = propertyInfo->PublicMethods;
  /* property must be accessible through a public method */
  if (methodTypes == 0)
  {
    *reason = "Property does not have public getter and setter functions.";
    return UNALLOWABLE;
  }
  /* property must have a GET AND SET or GET_MULTI AND SET_MULTI*/
  if (vtkSerdesCoder_MethodTypeMatches(methodTypes, VTK_METHOD_GET | VTK_METHOD_SET) ||
    vtkSerdesCoder_MethodTypeMatches(methodTypes, VTK_METHOD_GET_MULTI | VTK_METHOD_SET_MULTI))
  {
    return ALLOWABLE;
  }
  else
  {
    /* or a GET and derived from vtkCollection or vtkDataSetAttributes */
    if (vtkSerdesCoder_MethodTypeMatches(methodTypes, VTK_METHOD_GET))
    {
      if (vtkWrap_IsTypeOf(hinfo, propertyInfo->ClassName, "vtkCollection") ||
        vtkWrap_IsTypeOf(hinfo, propertyInfo->ClassName, "vtkDataSetAttributes"))
      {
        return ALLOWABLE;
      }
      else
      {
        *reason = "Getter return type is not a vtkCollection or vtkDataSetAttributes";
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
        vtkSerdesCoder_MethodTypeMatches(methodBitfield, VTK_METHOD_SET_CLAMP))
      {
        methodType = VTK_METHOD_SET_CLAMP;
        methodBitfield &= ~VTK_METHOD_SET_CLAMP;
      }
      else if ((methodType & VTK_METHOD_SET_BOOL) != 0 &&
        vtkSerdesCoder_MethodTypeMatches(methodBitfield, VTK_METHOD_SET_BOOL))
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
  FILE* fp, const char* getterFunctionName, const int isConst, const int isVTKSmartPointer)
{
  if (isConst)
  {
    fprintf(fp,
      "serializer->SerializeJSON("
      "const_cast<vtkObjectBase*>(reinterpret_cast<const vtkObjectBase*>("
      "object->%s()%s"
      ")))",
      getterFunctionName, isVTKSmartPointer ? ".Get()" : "");
  }
  else
  {
    fprintf(fp,
      "serializer->SerializeJSON("
      "reinterpret_cast<vtkObjectBase*>("
      "object->%s()%s"
      "))",
      getterFunctionName, isVTKSmartPointer ? ".Get()" : "");
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
int vtkWrapSerDes_WritePropertySerializer(FILE* fp, const ClassInfo* classInfo,
  const HierarchyInfo* hinfo, const FunctionInfo* functionInfo, const unsigned int methodType,
  const PropertyInfo* propertyInfo)
{
  if (!vtkWrapSerDes_IsSerializable(methodType))
  {
    return 0;
  }

  const int isMappedProperty = functionInfo->MarshalPropertyName != NULL;

  const char* keyName = isMappedProperty ? functionInfo->MarshalPropertyName : propertyInfo->Name;
  const char* getterName = functionInfo->Name;

  const ValueInfo* returnValue = functionInfo->ReturnValue;
  const int isVTKObject = vtkWrap_IsVTKObjectBaseType(hinfo, propertyInfo->ClassName);
  const int isVTKSmartPointer = vtkWrap_IsVTKSmartPointer(returnValue);
  const int isScalar = vtkWrap_IsScalar(returnValue);
  const int isNumeric = vtkWrap_IsNumeric(returnValue);
  const int isString = vtkWrap_IsString(returnValue);
  const int isCharPointer = vtkWrap_IsCharPointer(returnValue);
  const int isArray = vtkWrap_IsArray(returnValue);
  const int isStdVector = vtkWrap_IsStdVector(returnValue);
  const int isEnumMember = vtkWrap_IsEnumMember(classInfo, returnValue);
  const int isConst = vtkWrap_IsConst(returnValue);

  if (isVTKObject)
  {
    fprintf(fp, "  if (object->%s())\n", getterName);
    fprintf(fp, "  {\n");
    fprintf(fp, "    state[\"%s\"] = ", keyName);
    vtkWrapSerDes_WriteSerializerVTKObject(fp, getterName, isConst, isVTKSmartPointer);
    fprintf(fp, ";\n");
    fprintf(fp, "  }\n");
  }
  else if (isNumeric)
  {
    if (isScalar || isStdVector)
    {
      fprintf(fp, "  state[\"%s\"] = ", keyName);
      fprintf(fp, "object->%s();\n", getterName);
    }
    else if (isArray)
    {
      fprintf(fp, "  if(auto ptr = object->%s())\n", getterName);
      fprintf(fp, "  {\n");
      fprintf(fp, "    auto& dst = state[\"%s\"] = json::array();\n", keyName);
      fprintf(
        fp, "    for (int i = 0; i < %d; ++i) { dst.push_back(ptr[i]); }\n", propertyInfo->Count);
      fprintf(fp, "  }\n");
    }
    else if (isCharPointer)
    {
      fprintf(fp, "  if (auto ptr = object->%s()) { state[\"%s\"] = ptr; }\n", getterName, keyName);
    }
  }
  else if (isString || (isStdVector && isString))
  {
    fprintf(fp, "  state[\"%s\"] = ", keyName);
    fprintf(fp, "object->%s();\n", getterName);
  }
  else if (isEnumMember)
  {
    fprintf(fp, "  state[\"%s\"] = ", keyName);
    fprintf(fp, "static_cast<std::underlying_type<%s::%s>::type>(object->%s());\n", classInfo->Name,
      returnValue->Class, getterName);
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
  }
  else
  {
#ifndef NDEBUG
    // __builtin_debugtrap();
    fprintf(fp, "  // %s::%s (type=%s) could not be serialized\n", classInfo->Name,
      propertyInfo->Name, propertyInfo->ClassName);
#endif
    return 0;
  }
  return 1;
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
  if (isVTKObject && (isPointer || isVTKSmartPointer))
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
  }
  else
  {
#ifndef NDEBUG
    // __builtin_debugtrap();
    fprintf(fp, "  // %s::%s (type=%d) could not be deserialized\n", classInfo->Name,
      propertyInfo->Name, propertyInfo->Type);
#endif
    return 0;
  }
  free(val);
  return 1;
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
        "  /** signature=\"%s\"\n"
        "   * methodtype=\"%s\"\n"
        "   *       name=\"%s\"\n"
        "   *  valuetype=\"%s\"\n"
        "   *   ",
        theFunc->Signature, vtkParseProperties_MethodTypeAsString(methodType), theProp->Name,
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
