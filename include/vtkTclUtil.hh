/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTclUtil.hh
  Language:  C++
  Date:      01/18/95
  Version:   1.2

This file is part of the Visualization Toolkit. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

=========================================================================*/


#include <tcl.h>
#include <string.h>

extern vtkTclGetObjectFromPointer(Tcl_Interp *interp,void *temp,
			  int command(ClientData, Tcl_Interp *,int, char *[]));
extern void *vtkTclGetPointerFromObject(char *name,char *result_type);
extern void vtkTclVoidFunc(void *);
extern void vtkTclVoidFuncArgDelete(void *);
typedef  struct _vtkTclVoidFuncArg 
{
  Tcl_Interp *interp;
  char *command;
} vtkTclVoidFuncArg;
