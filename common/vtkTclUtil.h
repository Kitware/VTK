/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTclUtil.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/

#ifndef __vtkTclInclude_h
#define __vtkTclInclude_h

#include <tcl.h>
#include <tk.h>
#include <string.h>

#ifdef WIN32
#define VTKTCL_EXPORT __declspec( dllexport )
#else
#define VTKTCL_EXPORT
#endif

extern VTKTCL_EXPORT Tcl_Interp *vtkGlobalTclInterp;
extern VTKTCL_EXPORT int vtkTclEval(char *str);
extern VTKTCL_EXPORT char *vtkTclGetResult();
extern VTKTCL_EXPORT int vtkTclDeleteObjectFromHash(ClientData cd);
extern VTKTCL_EXPORT void vtkTclGenericDeleteObject(ClientData cd);
extern VTKTCL_EXPORT void vtkTclGetObjectFromPointer(Tcl_Interp *interp,void *temp,
			  int command(ClientData, Tcl_Interp *,int, char *[]));
extern VTKTCL_EXPORT void *vtkTclGetPointerFromObject(char *name,char *result_type,
					Tcl_Interp *interp);
extern VTKTCL_EXPORT void vtkTclVoidFunc(void *);
extern VTKTCL_EXPORT void vtkTclVoidFuncArgDelete(void *);
extern VTKTCL_EXPORT void vtkTclListInstances(Tcl_Interp *interp, ClientData arg);
extern VTKTCL_EXPORT int  vtkTclInDelete();

typedef  struct _vtkTclVoidFuncArg 
{
  Tcl_Interp *interp;
  char *command;
} vtkTclVoidFuncArg;


#endif

