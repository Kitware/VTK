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

#define VTK_AUTOINIT(M) VTK_AUTOINIT0(M##_AUTOINIT)
#define VTK_AUTOINIT0(T) VTK_AUTOINIT1(T)
#define VTK_AUTOINIT1(T) VTK_AUTOINIT_##T
#define VTK_AUTOINIT_0()
#define VTK_AUTOINIT_1(t1) VTK_AUTOINIT_0() VTK_AUTOINIT_IMPL(t1)
#define VTK_AUTOINIT_2(t1,t2) VTK_AUTOINIT_1(t1) VTK_AUTOINIT_IMPL(t2)
#define VTK_AUTOINIT_3(t1,t2,t3) VTK_AUTOINIT_2(t1,t2) VTK_AUTOINIT_IMPL(t3)
#define VTK_AUTOINIT_4(t1,t2,t3,t4) VTK_AUTOINIT_3(t1,t2,t3) VTK_AUTOINIT_IMPL(t4)
#define VTK_AUTOINIT_5(t1,t2,t3,t4,t5) VTK_AUTOINIT_4(t1,t2,t3,t4) VTK_AUTOINIT_IMPL(t5)
#define VTK_AUTOINIT_6(t1,t2,t3,t4,t5,t6) VTK_AUTOINIT_5(t1,t2,t3,t4,t5) VTK_AUTOINIT_IMPL(t6)
#define VTK_AUTOINIT_7(t1,t2,t3,t4,t5,t6,t7) VTK_AUTOINIT_6(t1,t2,t3,t4,t5,t6) VTK_AUTOINIT_IMPL(t7)
#define VTK_AUTOINIT_8(t1,t2,t3,t4,t5,t6,t7,t8) VTK_AUTOINIT_7(t1,t2,t3,t4,t5,t6,t7) VTK_AUTOINIT_IMPL(t8)
#define VTK_AUTOINIT_9(t1,t2,t3,t4,t5,t6,t7,t8,t9) VTK_AUTOINIT_8(t1,t2,t3,t4,t5,t6,t7,t8) VTK_AUTOINIT_IMPL(t9)
#define VTK_AUTOINIT_IMPL(M) \
  struct M##_AutoInit { M##_AutoInit(); ~M##_AutoInit(); }; static M##_AutoInit M##_AutoInit_Instance;

#endif
