/*=========================================================================

  Program:   Visualization Library
  Module:    vlTclUtil.cc
  Language:  C++
  Date:      01/18/95
  Version:   1.2

This file is part of the Visualization Library. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

=========================================================================*/

#include "vlTclUtil.hh"

extern Tcl_HashTable vlInstanceLookup;
extern Tcl_HashTable vlPointerLookup;
extern Tcl_HashTable vlCommandLookup;

static int num = 0;

vlTclGetObjectFromPointer(Tcl_Interp *interp,void *temp,
			  int command(ClientData, Tcl_Interp *,int, char *[]))
{
  int is_new;
  char temps[80];
  char name[80];
  Tcl_HashEntry *entry;

  /* return a pointer to a vl Object */
  /* first we must look up the pointer to see if it already exists */
  sprintf(temps,"%x",temp);
  if ((entry = Tcl_FindHashEntry(&vlPointerLookup,temps))) 
    {
    sprintf(interp->result,"%s",(char *)(Tcl_GetHashValue(entry)));
    }
  /* we must create a new name */
  else
    {
    sprintf(name,"vlTemp%i",num);
    num++;

    entry = Tcl_CreateHashEntry(&vlInstanceLookup,name,&is_new);
    Tcl_SetHashValue(entry,(ClientData)(temp));
    entry = Tcl_CreateHashEntry(&vlPointerLookup,temps,&is_new);
    Tcl_SetHashValue(entry,(ClientData)(strdup(name)));
    Tcl_CreateCommand(interp,name,command,
                      (ClientData)(temp),
                      (Tcl_CmdDeleteProc *)NULL);
    entry = Tcl_CreateHashEntry(&vlCommandLookup,name,&is_new);
    Tcl_SetHashValue(entry,(ClientData)command);
    sprintf(interp->result,"%s",name); 
    }
}
      
void *vlTclGetPointerFromObject(char *name,char *result_type)
{
  Tcl_HashEntry *entry;
  ClientData temp;
  int (*command)(ClientData, Tcl_Interp *,int, char *[]);
  char *args[3];


  /* set up the args */
  args[0] = "DoTypecasting";
  args[1] = result_type;
  args[2] = NULL;

  if (entry = Tcl_FindHashEntry(&vlInstanceLookup,name))
    {
    temp = (ClientData)Tcl_GetHashValue(entry);
    }
  else
    {
    fprintf(stderr,"vl bad argument, could not find object named %s\n", name);
    return NULL;
    }
  
  /* now handle the typecasting, get the command proc */
  if (entry = Tcl_FindHashEntry(&vlCommandLookup,name))
    {
    command = (int (*)(ClientData,Tcl_Interp *,int,char *[]))Tcl_GetHashValue(entry);
    }
  else
    {
    fprintf(stderr,"vl bad argument, could not find command process.\n", name);
    return NULL;
    }

  if (command(temp,(Tcl_Interp *)NULL,3,args) == TCL_OK)
    {
    return (void *)(args[2]);
    }
  else
    {
    fprintf(stderr,"vl bad argument, type conversion failed.\n", name);
    return NULL;
    }

}

void vlTclVoidFunc(void *arg)
{
  vlTclVoidFuncArg *arg2;

  arg2 = (vlTclVoidFuncArg *)arg;

  Tcl_GlobalEval(arg2->interp, arg2->command);
}

void vlTclVoidFuncArgDelete(void *arg)
{
  vlTclVoidFuncArg *arg2;

  arg2 = (vlTclVoidFuncArg *)arg;
  
  // free the string and then structure
  delete [] arg2->command;
  delete arg2;
}
