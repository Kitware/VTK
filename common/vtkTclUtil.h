/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTclUtil.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

=========================================================================*/

#include <tcl.h>
#include <tk.h>
#include <string.h>

extern Tcl_Interp *vtkGlobalTclInterp;
extern int vtkTclEval(char *str);
extern char *vtkTclGetResult();
extern int vtkTclDeleteObjectFromHash(ClientData cd);
extern void vtkTclGenericDeleteObject(ClientData cd);
extern void vtkTclGetObjectFromPointer(Tcl_Interp *interp,void *temp,
			  int command(ClientData, Tcl_Interp *,int, char *[]));
extern void *vtkTclGetPointerFromObject(char *name,char *result_type,
					Tcl_Interp *interp);
extern void vtkTclVoidFunc(void *);
extern void vtkTclVoidFuncArgDelete(void *);
extern void vtkTclListInstances(Tcl_Interp *interp, ClientData arg);
extern int  vtkTclInDelete();

typedef  struct _vtkTclVoidFuncArg 
{
  Tcl_Interp *interp;
  char *command;
} vtkTclVoidFuncArg;



