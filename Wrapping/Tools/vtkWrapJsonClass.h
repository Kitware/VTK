// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * vtkWrapJsonClass emits a JSON type manifest describing one serializable
 * class: its title, parent class, marshalable properties and invokable methods.
 * The manifest is consumed by @kitware/vtk-wasm's generate-types.mjs to produce
 * TypeScript definitions.
 *
 * It deliberately reuses the SerDes allowability predicates
 * (vtkWrapSerDes_IsAllowable, vtkWrapSerDes_IsFunctionAllowed,
 * vtkWrapSerDes_GetSuperClass) so the manifest describes exactly the members
 * the serializer/invoker actually marshal.
 *
 * Numeric types are emitted with concrete C width as the "type"
 * (int8/uint8/.../int64/uint64/float32/float64). Word-width types (size_t,
 * ssize_t, bare long) are baked to a concrete width using the target pointer
 * size (wordSize): 32-bit on wasm32, 64-bit on wasm64. The manifest is thus
 * architecture-specific, each wasm build ships its own set, and the
 * generator translates it directly wihout resolving types per architecture.
 */
#ifndef vtkWrapJsonClass_h
#define vtkWrapJsonClass_h

#include "vtkParseHierarchy.h"

#include <stdio.h>

/* Print the JSON type manifest for classInfo to fp. wordSize is the target
   pointer size in bytes (4 for wasm32, 8 for wasm64) and fixes the width of
   word-width C types (long, size_t, ssize_t). */
void vtkWrapJson_Class(FILE* fp, const HierarchyInfo* hinfo, ClassInfo* classInfo, int wordSize);

#endif
/* VTK-HeaderTest-Exclude: vtkWrapJsonClass.h */
