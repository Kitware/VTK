/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTclUtil.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

=========================================================================*/

#include <iostream.h>
#include <stdlib.h>
#include "vtkTclUtil.h"

extern Tcl_Interp *vtkGlobalTclInterp;
int vtkTclEval(char *str)
{
  return Tcl_GlobalEval(vtkGlobalTclInterp, str);
}

// The string result returned is volatile so you should copy it.
char *vtkTclGetResult()
{
  return vtkGlobalTclInterp->result;
}


int vtkRendererCommand(ClientData cd, Tcl_Interp *interp,
		       int argc, char *argv[]);
int vtkRenderWindowCommand(ClientData cd, Tcl_Interp *interp,
			      int argc, char *argv[]);

extern Tcl_HashTable vtkInstanceLookup;
extern Tcl_HashTable vtkPointerLookup;
extern Tcl_HashTable vtkCommandLookup;

static int num = 0;
static int vtkTclDebugOn = 0;
static int vtkInDelete = 0;

int vtkTclInDelete()
{  
  return vtkInDelete;
}

// we do no error checking in this.  We assume that if we were called
// then tcl must have been able to find the command function and object
int vtkTclDeleteObjectFromHash(ClientData cd)
{
  char temps[80];
  Tcl_HashEntry *entry;
  char *temp;
  int (*command)(ClientData, Tcl_Interp *,int, char *[]);

  // lookup the objects name
  sprintf(temps,"%p",(void *)cd);
  entry = Tcl_FindHashEntry(&vtkPointerLookup,temps); 
  temp = (char *)(Tcl_GetHashValue(entry));

  // now delete the three hash entries
  entry = Tcl_FindHashEntry(&vtkCommandLookup,temp);
  command = 
    (int (*)(ClientData,Tcl_Interp *,int,char *[]))Tcl_GetHashValue(entry);
  Tcl_DeleteHashEntry(entry);
  entry = Tcl_FindHashEntry(&vtkPointerLookup,temps); 
  Tcl_DeleteHashEntry(entry);
  entry = Tcl_FindHashEntry(&vtkInstanceLookup,temp);
  Tcl_DeleteHashEntry(entry);

  if (vtkTclDebugOn)
    {
    cerr << "vtkTcl Attempting to free object named " << temp << "\n";
    }
  // if it isn't a temp object (i.e. we created it) then delete it 
  if (strncmp(temp,"vtkTemp",7))
    {
    // finally free the name we got from the hash table
    // it was created using strdup
    free (temp);
    return 1;
    }
  // finally free the name we got from the hash table
  // it was created using strdup
  free (temp);
  return 0;
}

// we do no error checking in this.  We assume that if we were called
// then tcl must have been able to find the command function and object
void vtkTclGenericDeleteObject(ClientData cd)
{
  char temps[80];
  Tcl_HashEntry *entry;
  int (*command)(ClientData, Tcl_Interp *,int, char *[]);
  char *args[2];
  char *temp;
  Tcl_Interp *i;
  
  /* set up the args */
  args[1] = "Delete";

  // lookup the objects name
  sprintf(temps,"%p",(void *)cd);
  entry = Tcl_FindHashEntry(&vtkPointerLookup,temps); 
  temp = (char *)(Tcl_GetHashValue(entry));
  args[0] = temp;
  
  // get the command function and invoke the delete operation
  entry = Tcl_FindHashEntry(&vtkCommandLookup,temp);
  command = (int (*)(ClientData,Tcl_Interp *,int,char *[]))Tcl_GetHashValue(entry);

  // do we need to delete the c++ obj
  if (vtkTclDeleteObjectFromHash(cd))
    {
    i = Tcl_CreateInterp();
    vtkInDelete = 1;
    command(cd,i,2,args);
    vtkInDelete = 0;
    Tcl_DeleteInterp(i);
    }
}

int vtkCommand(ClientData cd, Tcl_Interp *interp, int argc, char *argv[])
{
  Tcl_HashEntry *entry;
  Tcl_HashSearch search;
  
  cd = 0; // shut up the compiler

  if (argc < 2) return TCL_OK;
  
  if (!strcmp(argv[1],"DeleteAllObjects"))
    {
    for (entry = Tcl_FirstHashEntry(&vtkPointerLookup,&search); 
	 entry != NULL;
	 entry = Tcl_NextHashEntry(&search))
      {
      Tcl_DeleteCommand(interp,(char *)Tcl_GetHashValue(entry));
      }
    return TCL_OK;
    }
  if (!strcmp(argv[1],"DebugOn"))
    {
    vtkTclDebugOn = 1;
    return TCL_OK;
    }
  if (!strcmp(argv[1],"DebugOff"))
    {
    vtkTclDebugOn = 0;
    return TCL_OK;
    }
  if (!strcmp("ListMethods",argv[1]))
    {
    Tcl_AppendResult(interp,"Methods for vtkCommand:\n",NULL);
    Tcl_AppendResult(interp,"  DebugOn\n",NULL);
    Tcl_AppendResult(interp,"  DebugOff\n",NULL);
    Tcl_AppendResult(interp,"  DeleteAllObjects\n",NULL);
    return TCL_OK;
    }

  Tcl_AppendResult(interp,"invalid method for vtkCommand\n",NULL);
  return TCL_ERROR;
}

void vtkTclGetObjectFromPointer(Tcl_Interp *interp,void *temp,
			  int command(ClientData, Tcl_Interp *,int, char *[]))
{
  int is_new;
  char temps[80];
  char name[80];
  Tcl_HashEntry *entry;

  /* if it is NULL then return empty string */
  if (!temp)
    {
    interp->result[0] = '\0';
    return;
    }
  
  /* return a pointer to a vtk Object */
  /* first we must look up the pointer to see if it already exists */
  sprintf(temps,"%p",temp);
  if ((entry = Tcl_FindHashEntry(&vtkPointerLookup,temps))) 
    {
    sprintf(interp->result,"%s",(char *)(Tcl_GetHashValue(entry)));
    return;
    }

  /* we must create a new name if it isn't NULL */
  sprintf(name,"vtkTemp%i",num);
  num++;
  
  entry = Tcl_CreateHashEntry(&vtkInstanceLookup,name,&is_new);
  Tcl_SetHashValue(entry,(ClientData)(temp));
  entry = Tcl_CreateHashEntry(&vtkPointerLookup,temps,&is_new);
  Tcl_SetHashValue(entry,(ClientData)(strdup(name)));
  Tcl_CreateCommand(interp,name,command,
		    (ClientData)(temp),
		    (Tcl_CmdDeleteProc *)vtkTclGenericDeleteObject);
  entry = Tcl_CreateHashEntry(&vtkCommandLookup,name,&is_new);
  Tcl_SetHashValue(entry,(ClientData)command);
  sprintf(interp->result,"%s",name); 
}
      
void *vtkTclGetPointerFromObject(char *name,char *result_type,
				 Tcl_Interp *interp)
{
  Tcl_HashEntry *entry;
  ClientData temp;
  int (*command)(ClientData, Tcl_Interp *,int, char *[]);
  char *args[3];
  char temps[256];

  /* set up the args */
  args[0] = "DoTypecasting";
  args[1] = result_type;
  args[2] = NULL;
  
  // object names cannot start with a number
  if ((name[0] >= '0')&&(name[0] <= '9'))
    {
    return NULL;
    }
  if ((entry = Tcl_FindHashEntry(&vtkInstanceLookup,name)))
    {
    temp = (ClientData)Tcl_GetHashValue(entry);
    }
  else
    {
    sprintf(temps,"vtk bad argument, could not find object named %s\n", name);
    Tcl_AppendResult(interp,temps,NULL);
    return NULL;
    }
  
  /* now handle the typecasting, get the command proc */
  if ((entry = Tcl_FindHashEntry(&vtkCommandLookup,name)))
    {
    command = (int (*)(ClientData,Tcl_Interp *,int,char *[]))Tcl_GetHashValue(entry);
    }
  else
    {
    sprintf(temps,"vtk bad argument, could not find command process for %s.\n", name);
    Tcl_AppendResult(interp,temps,NULL);
    return NULL;
    }

  if (command(temp,(Tcl_Interp *)NULL,3,args) == TCL_OK)
    {
    return (void *)(args[2]);
    }
  else
    {
    sprintf(temps,"vtk bad argument, type conversion failed for object %s.\nCould not type convert %s to type %s.\n", name, name, result_type);
    Tcl_AppendResult(interp,temps,NULL);
    return NULL;
    }

}

void vtkTclVoidFunc(void *arg)
{
  vtkTclVoidFuncArg *arg2;

  arg2 = (vtkTclVoidFuncArg *)arg;

  Tcl_GlobalEval(arg2->interp, arg2->command);
}

void vtkTclVoidFuncArgDelete(void *arg)
{
  vtkTclVoidFuncArg *arg2;

  arg2 = (vtkTclVoidFuncArg *)arg;
  
  // free the string and then structure
  delete [] arg2->command;
  delete arg2;
}

void vtkTclListInstances(Tcl_Interp *interp, ClientData arg)
{
  Tcl_HashSearch srch;
  Tcl_HashEntry *entry;
  int first = 1;
  
  // iteratively search hash table for command function
  entry = Tcl_FirstHashEntry(&vtkCommandLookup, &srch);
  if (!entry) 
    {
    interp->result[0] = '\0';
    return;
    }
  while (entry)
    {
    if (Tcl_GetHashValue(entry) == arg)
      {
      if (first)
	{
	first = 0;
	Tcl_AppendResult(interp,Tcl_GetHashKey(&vtkCommandLookup,entry),NULL);
	}
      else
	{
	Tcl_AppendResult(interp, " ", Tcl_GetHashKey(&vtkCommandLookup,entry), 
			 NULL);
	}
      }
    entry = Tcl_NextHashEntry(&srch);
    }
}

  
