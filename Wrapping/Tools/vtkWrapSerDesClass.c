// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkWrapSerDesClass.h"
#include "vtkWrap.h"
#include "vtkWrapSerDesProperty.h"

#include <stdlib.h>
#include <string.h>

/* -------------------------------------------------------------------- */
/* get the true superclass */
const char* vtkWrapSerDes_GetSuperClass(
  const ClassInfo* data, const HierarchyInfo* hinfo, const char** supermodule)
{
  const char* supername = NULL;
  const char* module = NULL;
  HierarchyEntry* entry;
  int i;

  /* if there are multiple superclasses, we just need the relevant one */
  for (i = 0; i < data->NumberOfSuperClasses; i++)
  {
    supername = data->SuperClasses[i];
    if (vtkWrap_IsClassWrapped(hinfo, supername))
    {
      if (vtkWrap_IsVTKObjectBaseType(hinfo, data->Name))
      {
        /* if class derived from vtkObjectBase, then only accept a
           superclass that is also a vtkObjectBase */
        if (vtkWrap_IsVTKObjectBaseType(hinfo, supername))
        {
          break;
        }
      }
      else
      {
        break;
      }
    }

    supername = NULL;
  }

  if (supermodule)
  {
    *supermodule = NULL;

    if (hinfo && supername)
    {
      /* get superclass module and check against our own */
      entry = vtkParseHierarchy_FindEntry(hinfo, data->Name);
      if (entry)
      {
        module = entry->Module;
      }
      entry = vtkParseHierarchy_FindEntry(hinfo, supername);
      if (entry && (!module || strcmp(entry->Module, module) != 0))
      {
        *supermodule = entry->Module;
      }
    }
  }

  return supername;
}

/* Define registrar functions for this class */
static void vtkWrapSerDes_DefineClassRegistrars(FILE* fp, const ClassInfo* classInfo)
{
  fprintf(fp,
    "int RegisterHandlers_%sSerDes(void* ser, void* deser)\n"
    "{\n"
    "  int success = 0;\n"
    "  if (auto* asObjectBase = static_cast<vtkObjectBase*>(ser))\n"
    "  {\n"
    "    if (auto* serializer = vtkSerializer::SafeDownCast(asObjectBase))\n"
    "    {\n"
    "      serializer->RegisterHandler(typeid(%s), Serialize_%s);\n"
    "      success = 1;\n"
    "    }\n"
    "  }\n"
    "  if (auto* asObjectBase = static_cast<vtkObjectBase*>(deser))\n"
    "  {\n"
    "    if (auto* deserializer = vtkDeserializer::SafeDownCast(asObjectBase))\n"
    "    {\n"
    "      deserializer->RegisterHandler(typeid(%s), Deserialize_%s);\n"
    "      deserializer->RegisterConstructor(\"%s\", []() { return %s::New(); });\n"
    "      success = 1;\n"
    "    }\n"
    "  }\n"
    "  return success;\n"
    "}\n",
    classInfo->Name, classInfo->Name, classInfo->Name, classInfo->Name, classInfo->Name,
    classInfo->Name, classInfo->Name);
}

/* start serializer */
static void vtkWrapSerDes_BeginSerializer(
  FILE* fp, const HierarchyInfo* hinfo, const ClassInfo* classInfo)
{
  fprintf(fp,
    "static nlohmann::json Serialize_%s(vtkObjectBase* objectBase, vtkSerializer* "
    "serializer)\n"
    "{\n"
    "  using json = nlohmann::json;\n"
    "  json state;\n",
    classInfo->Name);
  if (!strcmp(classInfo->Name, "vtkObjectBase"))
  {
    fprintf(fp, "  auto object = objectBase;\n");
    fprintf(fp, "  state[\"SuperClassNames\"] = json::array({});\n");
  }
  else
  {
    fprintf(fp, "  auto object = %s::SafeDownCast(objectBase);\n", classInfo->Name);
    fprintf(fp,
      "  if (auto f = serializer->GetHandler(typeid(%s::Superclass))) { state = f(object, "
      "serializer); }\n",
      classInfo->Name);
    /* Write the superclass */
    const char* superModuleName = NULL;
    const char* superClassName = vtkWrapSerDes_GetSuperClass(classInfo, hinfo, &superModuleName);
    fprintf(fp, "  state[\"SuperClassNames\"].push_back(\"%s\");\n", superClassName);
  }
}

/* end serializer */
static void vtkWrapSerDes_EndSerializer(FILE* fp)
{
  // might not be used, so silence unused-variable warnings
  fprintf(fp, "  (void)serializer;\n");
  fprintf(fp, "  return state;\n");
  fprintf(fp, "}\n\n");
}

/* start deserializer */
static void vtkWrapSerDes_BeginDeserializer(FILE* fp, const ClassInfo* classInfo)
{
  fprintf(fp,
    "static void Deserialize_%s(const nlohmann::json& state, vtkObjectBase* objectBase,"
    "vtkDeserializer* deserializer)\n",
    classInfo->Name);
  fprintf(fp, "{\n");
  if (!strcmp(classInfo->Name, "vtkObjectBase"))
  {
    fprintf(fp, "  auto object = objectBase;\n");
  }
  else
  {
    fprintf(fp,
      "  auto object = %s::SafeDownCast(objectBase);\n"
      "  if (auto f = deserializer->GetHandler(typeid(%s::Superclass)))\n"
      "  {\n"
      "    try\n"
      "    {\n"
      "      f(state, object, deserializer);\n"
      "    }\n"
      "    catch(std::exception& e)"
      "    {\n"
      "       vtkErrorWithObjectMacro(deserializer, << \"In \" << __func__ << \", failed to "
      "deserialize state=\" << "
      "state.dump()\n"
      "                << \". message=\" << e.what());\n"
      "    }\n"
      "  }\n",
      classInfo->Name, classInfo->Name);
  }
}

/* end deserializer */
static void vtkWrapSerDes_EndDeserializer(FILE* fp)
{
  // might not be used, so silence unused-variable warnings
  fprintf(fp, "  (void)deserializer;\n");
  fprintf(fp, "  (void)objectBase;\n");
  fprintf(fp, "  (void)object;\n");
  fprintf(fp, "  (void)state;\n");
  fprintf(fp, "}\n\n");
}

/* -------------------------------------------------------------------- */
void vtkWrapSerDes_ExportClassRegistrars(FILE* fp, const char* name)
{
  fprintf(fp,
    "extern \"C\"\n"
    "{\n"
    "  /**\n"
    "   * Register the (de)serialization handlers of classes from all serialized libraries.\n"
    "   * @param ser   a vtkSerializer instance\n"
    "   * @param deser a vtkDeserializer instance\n"
    "   * @param error when registration fails, the error message is pointed to by `error`. Use it "
    "for logging purpose.\n"
    "   * @warning The memory pointed to by `error` is not dynamically allocated. Do not free it.\n"
    "   */\n"
    "  int RegisterHandlers_%sSerDes(void* ser, void* deser);\n"
    "}\n",
    name);
}

/* -------------------------------------------------------------------- */
void vtkWrapSerDes_Class(FILE* fp, const HierarchyInfo* hinfo, ClassInfo* classInfo)
{
  vtkWrapSerDes_ExportClassRegistrars(fp, classInfo->Name);
  vtkWrapSerDes_BeginSerializer(fp, hinfo, classInfo);
  vtkWrapSerDes_Properties(fp, classInfo, hinfo, &vtkWrapSerDes_WritePropertySerializer);
  vtkWrapSerDes_EndSerializer(fp);
  vtkWrapSerDes_BeginDeserializer(fp, classInfo);
  vtkWrapSerDes_Properties(fp, classInfo, hinfo, &vtkWrapSerDes_WritePropertyDeserializer);
  vtkWrapSerDes_EndDeserializer(fp);
  vtkWrapSerDes_DefineClassRegistrars(fp, classInfo);
}
