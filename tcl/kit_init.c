#include <stdio.h>
#include <stdlib.h>
#include <string.h>
char *names[1000];
char *kitName;
int anindex = 0;

void stuffit()
{
  int i;
  
  for (i = 0; i < anindex; i++)
    {
    fprintf(stdout,"int %sCommand(ClientData cd, Tcl_Interp *interp,\n             int argc, char *argv[]);\n",names[i]);
    fprintf(stdout,"ClientData %sNewCommand();\n",names[i]);
    }

  if (!strcmp(kitName,"Vtkcoretcl"))
    {
    fprintf(stdout,"\nTcl_HashTable vtkInstanceLookup;\n");
    fprintf(stdout,"Tcl_HashTable vtkPointerLookup;\n");
    fprintf(stdout,"Tcl_HashTable vtkCommandLookup;\n");
    }
  else
    {
    fprintf(stdout,"\nextern Tcl_HashTable vtkInstanceLookup;\n");
    fprintf(stdout,"extern Tcl_HashTable vtkPointerLookup;\n");
    fprintf(stdout,"extern Tcl_HashTable vtkCommandLookup;\n");
    }
  
  fprintf(stdout,"\n\nextern \"C\" {int %s_SafeInit(Tcl_Interp *interp);}\n\n",
	  kitName);
  fprintf(stdout,"\n\nextern \"C\" {int %s_Init(Tcl_Interp *interp);}\n\n",
	  kitName);

  /* create an extern ref to the generic delete function */
  fprintf(stdout,"\n\nextern void vtkTclGenericDeleteObject(ClientData cd);\n\n");

  /* define the vtkNewInstanceCommand */
  fprintf(stdout,"int vtk%sNewInstanceCommand(ClientData cd, Tcl_Interp *interp,\n                         int argc, char *argv[])\n{\n",kitName);
  fprintf(stdout,"  Tcl_HashEntry *entry;\n  int is_new;\n  char temps[80];\n");
  fprintf(stdout,"  cd = 0; /* just prevents compiler warnings */\n");
  fprintf(stdout,"\n  if (argc != 2)\n    {\n    interp->result = \"vtk object creation requires one argument, a name.\";\n    return TCL_ERROR;\n    }\n\n");

  fprintf(stdout,"\n  if (argc != 2)\n    {\n    interp->result = \"vtk object creation requires one argument, a name.\";\n    return TCL_ERROR;\n    }\n\n");
  fprintf(stdout,"  if ((argv[1][0] >= '0')&&(argv[1][0] <= '9'))\n    {\n    interp->result = \"vtk object names must start with a letter.\";\n    return TCL_ERROR;\n    }\n\n");
  fprintf(stdout,"  if (Tcl_FindHashEntry(&vtkInstanceLookup,argv[1]))\n    {\n    interp->result = \"a vtk object with that name already exists.\";\n    return TCL_ERROR;\n    }\n\n");

  for (i = 0; i < anindex; i++)
    {
    fprintf(stdout,"  if (!strcmp(\"%s\",argv[0]))\n    {\n",names[i]);
    fprintf(stdout,"    ClientData temp = %sNewCommand();\n",names[i]);
    fprintf(stdout,"\n    entry = Tcl_CreateHashEntry(&vtkInstanceLookup,argv[1],&is_new);\n    Tcl_SetHashValue(entry,temp);\n");
    fprintf(stdout,"    sprintf(temps,\"%%p\",(void *)temp);\n");
    fprintf(stdout,"    entry = Tcl_CreateHashEntry(&vtkPointerLookup,temps,&is_new);\n    Tcl_SetHashValue(entry,(ClientData)(strdup(argv[1])));\n");
    fprintf(stdout,"    Tcl_CreateCommand(interp,argv[1],%sCommand,\n",
	    names[i]);
    fprintf(stdout,"                      temp,(Tcl_CmdDeleteProc *)vtkTclGenericDeleteObject);\n");
    fprintf(stdout,"    entry = Tcl_CreateHashEntry(&vtkCommandLookup,argv[1],&is_new);\n    Tcl_SetHashValue(entry,(ClientData)(%sCommand));\n",names[i]);
    fprintf(stdout,"    }\n\n");
    }

  fprintf(stdout,"  sprintf(interp->result,\"%%s\",argv[1]);\n  return TCL_OK;\n}");

  /* the main declaration */
  fprintf(stdout,"\n\nint %s_SafeInit(Tcl_Interp *interp)\n{\n",kitName);
  fprintf(stdout,"  return %s_Init(interp);\n}\n",kitName);
  
  fprintf(stdout,"\n\nint %s_Init(Tcl_Interp *interp)\n{\n",kitName);
  if (!strcmp(kitName,"Vtkcoretcl"))
    {
    fprintf(stdout,
	    "  Tcl_InitHashTable(&vtkInstanceLookup, TCL_STRING_KEYS);\n");
    fprintf(stdout,
	    "  Tcl_InitHashTable(&vtkPointerLookup, TCL_STRING_KEYS);\n");
    fprintf(stdout,
	    "  Tcl_InitHashTable(&vtkCommandLookup, TCL_STRING_KEYS);\n");
    }
  
  for (i = 0; i < anindex; i++)
    {
    fprintf(stdout,"  Tcl_CreateCommand(interp,\"%s\",vtk%sNewInstanceCommand,\n		    (ClientData *)NULL,\n		    (Tcl_CmdDeleteProc *)NULL);\n\n",
	    names[i],kitName);
    }

  fprintf(stdout,"  return TCL_OK;\n}\n");
};


int main(int argc,char *argv[])
{
  int i;

  if (argc < 3)
    {
    fprintf(stderr,"Usage: %s kit_name file1 file2 file3 ...\n",argv[0]);
    exit(1);
    }
  
  /* we have to make sure that the name is the correct case */
  kitName = strdup(argv[1]);
  if (kitName[0] > 90) kitName[0] -= 32;
  for (i = 1; i < strlen(kitName); i++)
    {
    if ((kitName[i] > 64)&&(kitName[i] < 91))
      {
      kitName[i] += 32;
      }
    }
  
  /* fill in the correct arrays */
  for (i = 2; i < argc; i++)
    {
    /* remove the .h and store */
    argv[i][strlen(argv[i])-2] = '\0';
    names[i-2] = strdup(argv[i]);
    }
  anindex = argc - 2;
  
  fprintf(stdout,"#include <string.h>\n");
  fprintf(stdout,"#include <tcl.h>\n\n");

  stuffit();
  
  return 0;
}

