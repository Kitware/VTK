/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTclUtil.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/

#ifndef __vtkTclInclude_h
#define __vtkTclInclude_h

#include "vtkObject.h"
#include "vtkCommand.h"
#include <tcl.h>

#ifdef WIN32
#define VTKTCL_EXPORT __declspec( dllexport )
#else
#define VTKTCL_EXPORT
#endif

extern VTKTCL_EXPORT void vtkTclDeleteObjectFromHash(vtkObject *,
						     unsigned long eventId,
                                                     void *, void *);
extern VTKTCL_EXPORT void vtkTclGenericDeleteObject(ClientData cd);

extern VTKTCL_EXPORT void 
vtkTclGetObjectFromPointer(Tcl_Interp *interp, void *temp,
			   int (*command)(ClientData, 
					  Tcl_Interp *,int, char *[]));

extern VTKTCL_EXPORT void *
vtkTclGetPointerFromObject(const char *name, const char *result_type,
			   Tcl_Interp *interp, int &error);

extern VTKTCL_EXPORT void vtkTclVoidFunc(void *);
extern VTKTCL_EXPORT void vtkTclVoidFuncArgDelete(void *);
extern VTKTCL_EXPORT void vtkTclListInstances(Tcl_Interp *interp, 
					      ClientData arg);
extern VTKTCL_EXPORT int  vtkTclInDelete(Tcl_Interp *interp);

extern VTKTCL_EXPORT int vtkTclNewInstanceCommand(ClientData cd, 
						  Tcl_Interp *interp,
						  int argc, char *argv[]);
extern VTKTCL_EXPORT void vtkTclDeleteCommandStruct(ClientData cd);
extern VTKTCL_EXPORT 
void vtkTclCreateNew(Tcl_Interp *interp, const char *cname,
		     ClientData (*NewCommand)(),
		     int (*CommandFunction)(ClientData cd,
					    Tcl_Interp *interp,
					    int argc, char *argv[]));

class vtkTclCommand : public vtkCommand
{
public:
  vtkTclCommand();
  ~vtkTclCommand(); 
  static vtkTclCommand *New() { return new vtkTclCommand; };

  void SetStringCommand(char *arg) { this->StringCommand = arg; };
  void SetInterp(Tcl_Interp *interp) { this->Interp = interp; };
  
  void Execute(vtkObject *, unsigned long, void *);

  char *StringCommand;
  Tcl_Interp *Interp;
};

typedef struct _vtkTclVoidFuncArg 
{
  Tcl_Interp *interp;
  char *command;
} vtkTclVoidFuncArg;

struct vtkTclCommandArgStruct
{
  void *Pointer;
  Tcl_Interp *Interp;
  unsigned long Tag;
};

struct vtkTclCommandStruct
{
  ClientData (*NewCommand)();
  int (*CommandFunction)(ClientData cd, Tcl_Interp *interp,
                         int argc, char *argv[]);
};

struct vtkTclInterpStruct
{
  Tcl_HashTable InstanceLookup;
  Tcl_HashTable PointerLookup;
  Tcl_HashTable CommandLookup;

  int Number;
  int DebugOn;
  int InDelete;
};

#endif

