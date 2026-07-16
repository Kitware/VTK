// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkWrapSerDesProperty_h
#define vtkWrapSerDesProperty_h

#include "vtkParseProperties.h"

#include <stdio.h>

/* Returns nonzero if the property can be serialized and later deserialized.
   Exposed so the JSON type-manifest emitter (vtkWrapJsonClass) selects exactly
   the same properties that the serializer marshals. */
int vtkWrapSerDes_IsAllowable(const HierarchyInfo* hinfo, const ClassInfo* classInfo,
  const FunctionInfo* functionInfo, const PropertyInfo* propertyInfo, const char** reason);

int vtkWrapSerDes_WritePropertySerializer(FILE* fp, const ClassInfo* classInfo,
  const HierarchyInfo* hinfo, const FunctionInfo* functionInfo, unsigned int methodType,
  const PropertyInfo* propertyInfo);

int vtkWrapSerDes_WritePropertyDeserializer(FILE* fp, const ClassInfo* classInfo,
  const HierarchyInfo* hinfo, const FunctionInfo* functionInfo, unsigned int methodType,
  const PropertyInfo* propertyInfo);

typedef int(WriteProperty)(FILE*, const ClassInfo*, const HierarchyInfo*, const FunctionInfo*,
  unsigned int, const PropertyInfo*);

/* write code that (de)serializes every (de)serializable property accessible through GetXXX() */
void vtkWrapSerDes_Properties(
  FILE* fp, ClassInfo* classInfo, const HierarchyInfo* hinfo, WriteProperty* writeFn);

#endif
