/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTclUtil.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef __vtkTclInclude_h
#define __vtkTclInclude_h

#include "vtkObject.h"
#include "vtkCommand.h"
#include "vtkTcl.h"

#ifdef WIN32
#define VTKTCL_EXPORT __declspec( dllexport )
#else
#define VTKTCL_EXPORT
#endif

extern VTKTCL_EXPORT void vtkTclUpdateCommand(Tcl_Interp *interp,
                                              char *name,
                                              vtkObject *obj);

extern VTKTCL_EXPORT void vtkTclDeleteObjectFromHash(vtkObject *,
                                                     unsigned long eventId,
                                                     void *, void *);
extern VTKTCL_EXPORT void vtkTclGenericDeleteObject(ClientData cd);

extern VTKTCL_EXPORT void
vtkTclGetObjectFromPointer(Tcl_Interp *interp, void *temp,
                           const char *targetType);

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
  static vtkTclCommand *New() { return new vtkTclCommand; };

  void SetStringCommand(const char *arg);
  void SetInterp(Tcl_Interp *interp) { this->Interp = interp; };

  void Execute(vtkObject *, unsigned long, void *);

  char *StringCommand;
  Tcl_Interp *Interp;
protected:
  vtkTclCommand();
  ~vtkTclCommand();
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
  int DeleteExistingObjectOnNew;
};

extern VTKTCL_EXPORT
void vtkTclApplicationInitExecutable(int argc, const char* const argv[]);
extern VTKTCL_EXPORT
void vtkTclApplicationInitTclTk(Tcl_Interp* interp,
                                const char* const relative_dirs[]);

#endif
