/*=========================================================================

  Program:   Visualization Library
  Module:    vlTclUtil.hh
  Language:  C++
  Date:      01/18/95
  Version:   1.2

This file is part of the Visualization Library. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

=========================================================================*/


#include <tcl.h>
#include <string.h>

extern vlTclGetObjectFromPointer(Tcl_Interp *interp,void *temp,
			  int command(ClientData, Tcl_Interp *,int, char *[]));
extern void *vlTclGetPointerFromObject(char *name,char *result_type);
extern void vlTclVoidFunc(void *);
extern void vlTclVoidFuncArgDelete(void *);
typedef  struct _vlTclVoidFuncArg 
{
  Tcl_Interp *interp;
  char *command;
} vlTclVoidFuncArg;
