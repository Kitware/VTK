// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkWrapSerDesFunction_h
#define vtkWrapSerDesFunction_h

#include "vtkParseData.h"
#include "vtkParseHierarchy.h"

#include <stdio.h>

/* Returns nonzero if the method can be marshalled (invoked) — static, public,
   non-template, non-inherited, with marshalable return + parameter types.
   Exposed so the JSON type-manifest emitter (vtkWrapJsonClass) selects exactly
   the same methods that the invoker generates. */
int vtkWrapSerDes_IsFunctionAllowed(FunctionInfo* functionInfo, const ClassInfo* classInfo,
  const HierarchyInfo* hinfo, const char** rejectReason, int* rejectedParameterId);

/* Returns nonzero if the value is a zero copy pointer, i.e. memory that outlives the call and
   that neither side copies. Such a value travels as the numeric address of its first element
   rather than as its contents. A parameter points at memory of the client's that the object
   borrows; a return value points into memory of the object's that the client borrows. Exposed so
   the JSON type-manifest emitter spells it the same way the invoker reads and writes it. */
int vtkWrapSerDes_CanMarshalZeroCopyPointer(const ValueInfo* valInfo);

/* Returns the index of the parameter that the method fills in and that the invoker returns as
   the value of the call, or -1 when the method has none. Exposed so the JSON type-manifest
   emitter leaves that parameter out of "parameters" and reports it under "returns", which is
   where a caller of the invoker finds it. */
int vtkWrapSerDes_FindOutParameterPosition(const FunctionInfo* functionInfo);

/* Define function void Invoke_ClassName_FuncName(..) for all methods in class*/
void vtkWrapSerDes_DefineFunctions(FILE* fp, ClassInfo* classInfo, const HierarchyInfo* hinfo);

/* Generate code that calls Invoke_ClassName_FuncName() */
void vtkWrapSerDes_CallFunctions(FILE* fp, ClassInfo* classInfo, const HierarchyInfo* hinfo);

#endif
