/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkParse.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef __vtkParse_h
#define __vtkParse_h

#if defined(__cplusplus)
#include <string>
#include <vector>
#endif

#define MAX_ARGS 20

typedef struct _FunctionInfo
  {
#if defined(__cplusplus)
    vtkstd::string Name;
#else
    char *Name;
#endif
    int   NumberOfArguments;
    int   ArrayFailure;
    int   IsPureVirtual;
    int   IsPublic;
    int   IsProtected;
    int   IsOperator;
    int   HaveHint;
    int   HintSize;
    int   ArgTypes[MAX_ARGS];
    int   ArgCounts[MAX_ARGS];

#if defined(__cplusplus)
    vtkstd::string ArgClasses[MAX_ARGS];
#else
    char *ArgClasses[MAX_ARGS];
#endif

    int   ReturnType;

#if defined(__cplusplus)
    vtkstd::string ReturnClass;
    vtkstd::string Comment;
    vtkstd::string Signature;
#else
    char *ReturnClass;
    char *Comment;
    char *Signature;
#endif

    int   IsLegacy;
  } FunctionInfo;

typedef struct _FileInfo
  {
    int   HasDelete;
    int   IsAbstract;
    int   IsConcrete;
#if defined(__cplusplus)
    vtkstd::string ClassName;
    vtkstd::string FileName;
    vtkstd::string OutputFileName;
    vtkstd::string SuperClasses[10];
#else
    char *ClassName;
    char *FileName;
    char *OutputFileName;
    char *SuperClasses[10];
#endif

    int   NumberOfSuperClasses;
    int   NumberOfFunctions;
    FunctionInfo Functions[1000];
#if defined(__cplusplus)
    vtkstd::string NameComment;
    vtkstd::string Description;
    vtkstd::string Caveats;
    vtkstd::string SeeAlso;
#else
    char *NameComment;
    char *Description;
    char *Caveats;
    char *SeeAlso;
#endif

  } FileInfo;

//--------------------------------------------------------------------------nix
/*
 * This structure is used internally to sort+collect individual functions.
 * Polymorphed functions will be combined and can be handeled together.
 *
 */
typedef struct _UniqueFunctionInfo
{
#if defined(__cplusplus)
  vtkstd::string Name;
#else
  char *Name;
#endif
  int TotalPolymorphTypes;
#if defined(__cplusplus)
  vtkstd::vector<FunctionInfo> Function;
#else
  FunctionInfo Function[20];
#endif
}UniqueFunctionInfo;


//--------------------------------------------------------------------------nix
/*
 * This structure is used to collect and hold class information. It is a
 * modified version of FileInfo
 *
 */
typedef struct _ClassInfo
{
  int   HasDelete;
  int   IsAbstract;
  int   IsConcrete;
#if defined(__cplusplus)
  vtkstd::string ClassName;
  vtkstd::string FileName;
  vtkstd::string OutputFileName;
  vtkstd::string SuperClasses[10];
#else
  char *ClassName;
  char *FileName;
  char *OutputFileName;
  char *SuperClasses[10];
#endif

  int   NumberOfSuperClasses;
  int   NumberOfFunctions;
#if defined(__cplusplus)
  vtkstd::vector<UniqueFunctionInfo> Functions;
#else
  UniqueFunctionInfo Functions[1000];
#endif
#if defined(__cplusplus)
  vtkstd::string NameComment;
  vtkstd::string Description;
  vtkstd::string Caveats;
  vtkstd::string SeeAlso;
#else
  char *NameComment;
  char *Description;
  char *Caveats;
  char *SeeAlso;
#endif
}ClassInfo;


#endif //__vtkParse_h
