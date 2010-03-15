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

#define MAX_ARGS 20  

  typedef struct _FunctionInfo
  {
    char *Name;
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
    int   ArgExternals[MAX_ARGS];
    char *ArgClasses[MAX_ARGS];
    int   ReturnType;
    char *ReturnClass;
    char *Comment;
    char *Signature;  
    int   IsLegacy;
    int   ReturnExternal;
  } FunctionInfo;
  
  typedef struct _FileInfo
  {
    int   HasDelete;
    int   IsAbstract;
    int   IsConcrete;
    char *ClassName;
    char *FileName;
    char *OutputFileName;
    
    char *SuperClasses[10];
    int   NumberOfSuperClasses;
    int   NumberOfFunctions;
    FunctionInfo Functions[1000];
    char *NameComment;
    char *Description;
    char *Caveats;
    char *SeeAlso;
  } FileInfo;
