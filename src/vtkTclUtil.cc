/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTclUtil.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

=========================================================================*/

#include <stdlib.h>
#include "vtkTclUtil.hh"

extern Tcl_HashTable vtkInstanceLookup;
extern Tcl_HashTable vtkPointerLookup;
extern Tcl_HashTable vtkCommandLookup;

static int num = 0;

// we do no error checking in this.  We assume that if we were called
// then tcl must have been able to find the command function and object
void vtkTclGenericDeleteObject(ClientData cd)
{
  char temps[80];
  Tcl_HashEntry *entry;
  int (*command)(ClientData, Tcl_Interp *,int, char *[]);
  char *args[1];
  char *temp;

  /* set up the args */
  args[0] = "Delete";

  // lookup the objects name
  sprintf(temps,"%x",(void *)cd);
  entry = Tcl_FindHashEntry(&vtkPointerLookup,temps); 
  temp = (char *)(Tcl_GetHashValue(entry));

  // get the command function and invoke the delete operation
  entry = Tcl_FindHashEntry(&vtkCommandLookup,temp);
  command = (int (*)(ClientData,Tcl_Interp *,int,char *[]))Tcl_GetHashValue(entry);
  command(cd,(Tcl_Interp *)NULL,1,args);

  // now delete the three hash entries
  Tcl_DeleteHashEntry(entry);
  entry = Tcl_FindHashEntry(&vtkPointerLookup,temps); 
  Tcl_DeleteHashEntry(entry);
  entry = Tcl_FindHashEntry(&vtkInstanceLookup,temp);
  Tcl_DeleteHashEntry(entry);
  // finally free the name we got from the hash table
  // it was created using strdup
  free (temp);
}
  
vtkTclGetObjectFromPointer(Tcl_Interp *interp,void *temp,
			  int command(ClientData, Tcl_Interp *,int, char *[]))
{
  int is_new;
  char temps[80];
  char name[80];
  Tcl_HashEntry *entry;

  /* return a pointer to a vtk Object */
  /* first we must look up the pointer to see if it already exists */
  sprintf(temps,"%x",temp);
  if ((entry = Tcl_FindHashEntry(&vtkPointerLookup,temps))) 
    {
    sprintf(interp->result,"%s",(char *)(Tcl_GetHashValue(entry)));
    }
  /* we must create a new name */
  else
    {
    sprintf(name,"vtkTemp%i",num);
    num++;

    entry = Tcl_CreateHashEntry(&vtkInstanceLookup,name,&is_new);
    Tcl_SetHashValue(entry,(ClientData)(temp));
    entry = Tcl_CreateHashEntry(&vtkPointerLookup,temps,&is_new);
    Tcl_SetHashValue(entry,(ClientData)(strdup(name)));
    Tcl_CreateCommand(interp,name,command,
                      (ClientData)(temp),
                      (Tcl_CmdDeleteProc *)NULL);
    entry = Tcl_CreateHashEntry(&vtkCommandLookup,name,&is_new);
    Tcl_SetHashValue(entry,(ClientData)command);
    sprintf(interp->result,"%s",name); 
    }
}
      
void *vtkTclGetPointerFromObject(char *name,char *result_type)
{
  Tcl_HashEntry *entry;
  ClientData temp;
  int (*command)(ClientData, Tcl_Interp *,int, char *[]);
  char *args[3];


  /* set up the args */
  args[0] = "DoTypecasting";
  args[1] = result_type;
  args[2] = NULL;

  if (entry = Tcl_FindHashEntry(&vtkInstanceLookup,name))
    {
    temp = (ClientData)Tcl_GetHashValue(entry);
    }
  else
    {
    fprintf(stderr,"vtk bad argument, could not find object named %s\n", name);
    return NULL;
    }
  
  /* now handle the typecasting, get the command proc */
  if (entry = Tcl_FindHashEntry(&vtkCommandLookup,name))
    {
    command = (int (*)(ClientData,Tcl_Interp *,int,char *[]))Tcl_GetHashValue(entry);
    }
  else
    {
    fprintf(stderr,"vtk bad argument, could not find command process.\n", name);
    return NULL;
    }

  if (command(temp,(Tcl_Interp *)NULL,3,args) == TCL_OK)
    {
    return (void *)(args[2]);
    }
  else
    {
    fprintf(stderr,"vtk bad argument, type conversion failed.\n", name);
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
