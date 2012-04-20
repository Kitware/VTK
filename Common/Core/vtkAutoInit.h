/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAutoInit.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef __vtkAutoInit_h
#define __vtkAutoInit_h

#include "vtkDebugLeaksManager.h" // DebugLeaks exists longer.

#define VTK_AUTOINIT(M) VTK_AUTOINIT0(M,M##_AUTOINIT)
#define VTK_AUTOINIT0(M,T) VTK_AUTOINIT1(M,T)
#define VTK_AUTOINIT1(M,T)                                              \
  /* Declare every <mod>_AutoInit_(Construct|Destruct) function.  */    \
  VTK_AUTOINIT_DECLARE_##T                                              \
  static struct M##_AutoInit {                                          \
    /* Call every <mod>_AutoInit_Construct during initialization.  */   \
    M##_AutoInit()  { VTK_AUTOINIT_CONSTRUCT_##T }                      \
    /* Call every <mod>_AutoInit_Destruct during finalization.  */      \
    ~M##_AutoInit() { VTK_AUTOINIT_DESTRUCT_##T  }                      \
  } M##_AutoInit_Instance;

#define VTK_AUTOINIT_DECLARE_0()
#define VTK_AUTOINIT_DECLARE_1(t1) VTK_AUTOINIT_DECLARE_0() VTK_AUTOINIT_DECLARE(t1)
#define VTK_AUTOINIT_DECLARE_2(t1,t2) VTK_AUTOINIT_DECLARE_1(t1) VTK_AUTOINIT_DECLARE(t2)
#define VTK_AUTOINIT_DECLARE_3(t1,t2,t3) VTK_AUTOINIT_DECLARE_2(t1,t2) VTK_AUTOINIT_DECLARE(t3)
#define VTK_AUTOINIT_DECLARE_4(t1,t2,t3,t4) VTK_AUTOINIT_DECLARE_3(t1,t2,t3) VTK_AUTOINIT_DECLARE(t4)
#define VTK_AUTOINIT_DECLARE_5(t1,t2,t3,t4,t5) VTK_AUTOINIT_DECLARE_4(t1,t2,t3,t4) VTK_AUTOINIT_DECLARE(t5)
#define VTK_AUTOINIT_DECLARE_6(t1,t2,t3,t4,t5,t6) VTK_AUTOINIT_DECLARE_5(t1,t2,t3,t4,t5) VTK_AUTOINIT_DECLARE(t6)
#define VTK_AUTOINIT_DECLARE_7(t1,t2,t3,t4,t5,t6,t7) VTK_AUTOINIT_DECLARE_6(t1,t2,t3,t4,t5,t6) VTK_AUTOINIT_DECLARE(t7)
#define VTK_AUTOINIT_DECLARE_8(t1,t2,t3,t4,t5,t6,t7,t8) VTK_AUTOINIT_DECLARE_7(t1,t2,t3,t4,t5,t6,t7) VTK_AUTOINIT_DECLARE(t8)
#define VTK_AUTOINIT_DECLARE_9(t1,t2,t3,t4,t5,t6,t7,t8,t9) VTK_AUTOINIT_DECLARE_8(t1,t2,t3,t4,t5,t6,t7,t8) VTK_AUTOINIT_DECLARE(t9)
#define VTK_AUTOINIT_DECLARE(M) \
  void M##_AutoInit_Construct(); \
  void M##_AutoInit_Destruct();

#define VTK_AUTOINIT_CONSTRUCT_0()
#define VTK_AUTOINIT_CONSTRUCT_1(t1) VTK_AUTOINIT_CONSTRUCT_0() VTK_AUTOINIT_CONSTRUCT(t1)
#define VTK_AUTOINIT_CONSTRUCT_2(t1,t2) VTK_AUTOINIT_CONSTRUCT_1(t1) VTK_AUTOINIT_CONSTRUCT(t2)
#define VTK_AUTOINIT_CONSTRUCT_3(t1,t2,t3) VTK_AUTOINIT_CONSTRUCT_2(t1,t2) VTK_AUTOINIT_CONSTRUCT(t3)
#define VTK_AUTOINIT_CONSTRUCT_4(t1,t2,t3,t4) VTK_AUTOINIT_CONSTRUCT_3(t1,t2,t3) VTK_AUTOINIT_CONSTRUCT(t4)
#define VTK_AUTOINIT_CONSTRUCT_5(t1,t2,t3,t4,t5) VTK_AUTOINIT_CONSTRUCT_4(t1,t2,t3,t4) VTK_AUTOINIT_CONSTRUCT(t5)
#define VTK_AUTOINIT_CONSTRUCT_6(t1,t2,t3,t4,t5,t6) VTK_AUTOINIT_CONSTRUCT_5(t1,t2,t3,t4,t5) VTK_AUTOINIT_CONSTRUCT(t6)
#define VTK_AUTOINIT_CONSTRUCT_7(t1,t2,t3,t4,t5,t6,t7) VTK_AUTOINIT_CONSTRUCT_6(t1,t2,t3,t4,t5,t6) VTK_AUTOINIT_CONSTRUCT(t7)
#define VTK_AUTOINIT_CONSTRUCT_8(t1,t2,t3,t4,t5,t6,t7,t8) VTK_AUTOINIT_CONSTRUCT_7(t1,t2,t3,t4,t5,t6,t7) VTK_AUTOINIT_CONSTRUCT(t8)
#define VTK_AUTOINIT_CONSTRUCT_9(t1,t2,t3,t4,t5,t6,t7,t8,t9) VTK_AUTOINIT_CONSTRUCT_8(t1,t2,t3,t4,t5,t6,t7,t8) VTK_AUTOINIT_CONSTRUCT(t9)
#define VTK_AUTOINIT_CONSTRUCT(M) \
  M##_AutoInit_Construct();

#define VTK_AUTOINIT_DESTRUCT_0()
#define VTK_AUTOINIT_DESTRUCT_1(t1) VTK_AUTOINIT_DESTRUCT_0() VTK_AUTOINIT_DESTRUCT(t1)
#define VTK_AUTOINIT_DESTRUCT_2(t1,t2) VTK_AUTOINIT_DESTRUCT_1(t1) VTK_AUTOINIT_DESTRUCT(t2)
#define VTK_AUTOINIT_DESTRUCT_3(t1,t2,t3) VTK_AUTOINIT_DESTRUCT_2(t1,t2) VTK_AUTOINIT_DESTRUCT(t3)
#define VTK_AUTOINIT_DESTRUCT_4(t1,t2,t3,t4) VTK_AUTOINIT_DESTRUCT_3(t1,t2,t3) VTK_AUTOINIT_DESTRUCT(t4)
#define VTK_AUTOINIT_DESTRUCT_5(t1,t2,t3,t4,t5) VTK_AUTOINIT_DESTRUCT_4(t1,t2,t3,t4) VTK_AUTOINIT_DESTRUCT(t5)
#define VTK_AUTOINIT_DESTRUCT_6(t1,t2,t3,t4,t5,t6) VTK_AUTOINIT_DESTRUCT_5(t1,t2,t3,t4,t5) VTK_AUTOINIT_DESTRUCT(t6)
#define VTK_AUTOINIT_DESTRUCT_7(t1,t2,t3,t4,t5,t6,t7) VTK_AUTOINIT_DESTRUCT_6(t1,t2,t3,t4,t5,t6) VTK_AUTOINIT_DESTRUCT(t7)
#define VTK_AUTOINIT_DESTRUCT_8(t1,t2,t3,t4,t5,t6,t7,t8) VTK_AUTOINIT_DESTRUCT_7(t1,t2,t3,t4,t5,t6,t7) VTK_AUTOINIT_DESTRUCT(t8)
#define VTK_AUTOINIT_DESTRUCT_9(t1,t2,t3,t4,t5,t6,t7,t8,t9) VTK_AUTOINIT_DESTRUCT_8(t1,t2,t3,t4,t5,t6,t7,t8) VTK_AUTOINIT_DESTRUCT(t9)
#define VTK_AUTOINIT_DESTRUCT(M) \
  M##_AutoInit_Destruct();

#endif
// VTK-HeaderTest-Exclude: vtkAutoInit.h
