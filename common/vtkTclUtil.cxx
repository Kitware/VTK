/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTclUtil.cxx
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

#include <iostream.h>
#include <stdlib.h>
#include "vtkObject.h"
#include "vtkTclUtil.h"
#include "vtkSetGet.h"

VTKTCL_EXPORT int vtkTclEval(char *str)
{
  int res;
  
  res = Tcl_GlobalEval(vtkGlobalTclInterp, str);
  
  if (res == TCL_ERROR)
    {
    vtkGenericWarningMacro("Error returned from vtk/tcl callback.\n" <<
			   vtkGlobalTclInterp->result << endl);
    }
  return res;
}

// The string result returned is volatile so you should copy it.
VTKTCL_EXPORT char *vtkTclGetResult()
{
  return vtkGlobalTclInterp->result;
}


extern Tcl_HashTable vtkInstanceLookup;
extern Tcl_HashTable vtkPointerLookup;
extern Tcl_HashTable vtkCommandLookup;

static int num = 0;
static int vtkTclDebugOn = 0;
static int vtkInDelete = 0;

VTKTCL_EXPORT int vtkTclInDelete()
{  
  return vtkInDelete;
}

// we do no error checking in this.  We assume that if we were called
// then tcl must have been able to find the command function and object
VTKTCL_EXPORT int vtkTclDeleteObjectFromHash(ClientData cd)
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
    vtkGenericWarningMacro("vtkTcl Attempting to free object named " << temp);
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
VTKTCL_EXPORT void vtkTclGenericDeleteObject(ClientData cd)
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
  temp = strdup((char *)(Tcl_GetHashValue(entry)));
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
  if (temp) free(temp);
}

int vtkCommand(ClientData cd, Tcl_Interp *interp, int argc, char *argv[])
{
  Tcl_HashEntry *entry;
  Tcl_HashSearch search;
  char * tmp;

  cd = 0; // shut up the compiler

  if (argc < 2) return TCL_OK;
  
  if (!strcmp(argv[1],"DeleteAllObjects"))
    {
    for (entry = Tcl_FirstHashEntry(&vtkPointerLookup,&search); 
	 entry != NULL;
	 entry = Tcl_FirstHashEntry(&vtkPointerLookup,&search))
      {
      tmp = strdup((char *)Tcl_GetHashValue(entry));
      if (tmp) Tcl_DeleteCommand(interp,tmp);
	  if (tmp) free(tmp);
      }
    return TCL_OK;
    }
  if (!strcmp(argv[1],"ListAllInstances"))
    {
    for (entry = Tcl_FirstHashEntry(&vtkInstanceLookup,&search); 
	 entry != NULL; entry = Tcl_NextHashEntry(&search))
      {
      Tcl_AppendResult(interp,
		       (char *)Tcl_GetHashKey(&vtkInstanceLookup,entry),NULL);
      Tcl_AppendResult(interp,"\n",NULL);
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
    Tcl_AppendResult(interp,"  ListAllInstances\n",NULL);
    return TCL_OK;
    }

  Tcl_AppendResult(interp,"invalid method for vtkCommand\n",NULL);
  return TCL_ERROR;
}

VTKTCL_EXPORT void vtkTclGetObjectFromPointer(Tcl_Interp *interp,void *temp,
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
  if (vtkTclDebugOn)
    {
      vtkGenericWarningMacro("Looking up name for vtk pointer: " << temp);
    }

  /* first we must look up the pointer to see if it already exists */
  sprintf(temps,"%p",temp);
  if ((entry = Tcl_FindHashEntry(&vtkPointerLookup,temps))) 
    {
    /* if it already exists then are the command functions the same ? */
    int (*command2)(ClientData, Tcl_Interp *,int, char *[]);
    char *args[3];
    Tcl_HashEntry *entry2;
  
    entry2 = Tcl_FindHashEntry(&vtkCommandLookup,
                               (char *)(Tcl_GetHashValue(entry)));
    command2 = 
      (int (*)(ClientData,Tcl_Interp *,int,char *[]))Tcl_GetHashValue(entry2);
    
    if (vtkTclDebugOn)
      {
      vtkGenericWarningMacro("Found name: " 
                             << (char *)(Tcl_GetHashValue(entry)) 
                             << " for vtk pointer: " << temp);
      }

    /* if the commands are not the same try to pick the best one */
    /* the best one is the one that is lowest in the tree */
    if (command2 != command)
      {
      /* to find the best one we call GetClassName and see if one of the */
      /* commands can typeconvert to that class while the other can't    */
      /* if that is the case then we use the one that can. Otherwise we  */
      /* just let the original command function stay */
      /* set up the args for the GetClassName call */
      args[0] = "Dummy";
      args[1] = "GetClassName";
      args[2] = NULL;
      command2((ClientData)temp,interp,2,args);
      args[0] = "DoTypecasting";
      args[1] = interp->result;
      args[2] = NULL;
      if (command2((ClientData)temp,(Tcl_Interp *)NULL,3,args) != TCL_OK)
	{
	/* the original function couldn't handle the type conversion so try the new one */
	if (command((ClientData)temp,(Tcl_Interp *)NULL,3,args) == TCL_OK)
	  {
	  /* the new one can handle the conversion so lets use it instead */
	  Tcl_SetHashValue(entry2,(ClientData)command);
	  }
	}
      }
    
    /* while we are at it store the name since it is required anyhow */
    sprintf(interp->result,"%s",(char *)(Tcl_GetHashValue(entry)));
    return;
    }

  /* we must create a new name if it isn't NULL */
  sprintf(name,"vtkTemp%i",num);
  num++;
  
  if (vtkTclDebugOn)
    {
      vtkGenericWarningMacro("Created name: " << name
			   << " for vtk pointer: " << temp);
    }

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
      
VTKTCL_EXPORT void *vtkTclGetPointerFromObject(char *name,char *result_type,
					       Tcl_Interp *interp, int &error)
{
  Tcl_HashEntry *entry;
  ClientData temp;
  int (*command)(ClientData, Tcl_Interp *,int, char *[]);
  char *args[3];
  char temps[256];

  /* check for empty string, empty string is the same as passing NULL */
  if (name[0] == '\0') return NULL;
  
  /* set up the args */
  args[0] = "DoTypecasting";
  args[1] = result_type;
  args[2] = NULL;
  
  // object names cannot start with a number
  if ((name[0] >= '0')&&(name[0] <= '9'))
    {
    error = 1;
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
    error = 1;
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
    error = 1;
    return NULL;
    }

  if (command(temp,(Tcl_Interp *)NULL,3,args) == TCL_OK)
    {
    return (void *)(args[2]);
    }
  else
    {
    // provide more diagnostic info
    args[0] = "Dummy";
    args[1] = "GetClassName";
    args[2] = NULL;
    command(temp,interp,2,args);

    sprintf(temps,"vtk bad argument, type conversion failed for object %s.\nCould not type convert %s which is of type %s, to type %s.\n", name, name, interp->result, result_type);
    interp->result[0] = '\0';
    Tcl_AppendResult(interp,temps,NULL);
    error = 1;
    return NULL;
    }

}

VTKTCL_EXPORT void vtkTclVoidFunc(void *arg)
{
  int res;

  vtkTclVoidFuncArg *arg2;

  arg2 = (vtkTclVoidFuncArg *)arg;

  res = Tcl_GlobalEval(arg2->interp, arg2->command);

  if (res == TCL_ERROR)
    {
    if (Tcl_GetVar(arg2->interp,"errorInfo",0))
      {
      vtkGenericWarningMacro("Error returned from vtk/tcl callback:\n" <<
			     arg2->command << endl <<
			     Tcl_GetVar(arg2->interp,"errorInfo",0) <<
			     " at line number " << arg2->interp->errorLine);
      }
    else
      {
      vtkGenericWarningMacro("Error returned from vtk/tcl callback:\n" <<
			     arg2->command << endl <<
			     " at line number " << arg2->interp->errorLine);
      }
    }
}

VTKTCL_EXPORT void vtkTclVoidFuncArgDelete(void *arg)
{
  vtkTclVoidFuncArg *arg2;

  arg2 = (vtkTclVoidFuncArg *)arg;
  
  // free the string and then structure
  delete [] arg2->command;
  delete arg2;
}

VTKTCL_EXPORT void vtkTclListInstances(Tcl_Interp *interp, ClientData arg)
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

  
