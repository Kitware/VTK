#include <stdio.h>
#include <stdlib.h>
#include <string.h>
char *names[1000];
char *kitName;
int anindex = 0;

/* warning this code is also in getclasses.cxx under pcmaker */
void stuffit()
{
  int i;
  
  for (i = 0; i < anindex; i++)
    {
    fprintf(stdout,"int %sCommand(ClientData cd, Tcl_Interp *interp,\n             int argc, char *argv[]);\n",names[i]);
    fprintf(stdout,"ClientData %sNewCommand();\n",names[i]);
    }

  if (!strcmp(kitName,"Vtkcommontcl"))
    {
    fprintf(stdout,"int vtkCommand(ClientData cd, Tcl_Interp *interp,\n             int argc, char *argv[]);\n");
    /* claw: I am adding this so c++ can evaluate strings. */
    fprintf(stdout,"\nTcl_Interp *vtkGlobalTclInterp;\n");
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
  fprintf(stdout,"extern void vtkTclDeleteObjectFromHash(void *);\n");  
  fprintf(stdout,"extern void vtkTclListInstances(Tcl_Interp *interp, ClientData arg);\n");
  
  fprintf(stdout,"\n\nextern \"C\" {int VTK_EXPORT %s_SafeInit(Tcl_Interp *interp);}\n\n",
	  kitName);
  fprintf(stdout,"\n\nextern \"C\" {int VTK_EXPORT %s_Init(Tcl_Interp *interp);}\n\n",
	  kitName);

  /* create an extern ref to the generic delete function */
  fprintf(stdout,"\n\nextern void vtkTclGenericDeleteObject(ClientData cd);\n\n");

  /* the main declaration */
  fprintf(stdout,"\n\nint VTK_EXPORT %s_SafeInit(Tcl_Interp *interp)\n{\n",kitName);
  fprintf(stdout,"  return %s_Init(interp);\n}\n",kitName);
  
  fprintf(stdout,"\n\nint VTK_EXPORT %s_Init(Tcl_Interp *interp)\n{\n",
          kitName);
  if (!strcmp(kitName,"Vtkcommontcl"))
    {
    fprintf(stdout,
	    "  vtkTclInterpStruct *info = new vtkTclInterpStruct;\n");
    fprintf(stdout,
            "  info->Number = 0; info->InDelete = 0; info->DebugOn = 0;\n");
    fprintf(stdout,
            "\n");
    fprintf(stdout,
            "\n");
    fprintf(stdout,
	    "  Tcl_InitHashTable(&info->InstanceLookup, TCL_STRING_KEYS);\n");
    fprintf(stdout,
	    "  Tcl_InitHashTable(&info->PointerLookup, TCL_STRING_KEYS);\n");
    fprintf(stdout,
	    "  Tcl_InitHashTable(&info->CommandLookup, TCL_STRING_KEYS);\n");
    fprintf(stdout,
            "  Tcl_SetAssocData(interp,(char *) \"vtk\",NULL,(ClientData *)info);\n");

    /* create special vtkCommand command */
    fprintf(stdout,"  Tcl_CreateCommand(interp,(char *) \"vtkCommand\",vtkCommand,\n		    (ClientData *)NULL, NULL);\n\n");
    }
  
  for (i = 0; i < anindex; i++)
    {
    fprintf(stdout,"  vtkTclCreateNew(interp,(char *) \"%s\", %sNewCommand,\n",
	    names[i], names[i]);
    fprintf(stdout,"                  %sCommand);\n",names[i]);
    }

  fprintf(stdout,"  return TCL_OK;\n}\n");
}


int main(int argc,char *argv[])
{
  int i,j;
  char tmp[128];
  FILE *file;

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
  for (i = 2, j = 0; i < argc; i++)
    {
    /* first check validity of the wrapper for the class */ 
    strcpy(tmp,"./tcl/");
    strcpy(tmp+strlen(tmp),argv[i]);
    strcpy(tmp+strlen(tmp)-2,"Tcl.cxx");
    file = fopen(tmp,"r");
    if (file) 
      {
      fgets(tmp,19,file);
      tmp[19] = '\0';
      fclose(file);
      }
    if (file && strcmp(tmp,"// tcl wrapper for") != 0)
      {
      /* the wrapper file wasn't built or isn't valid */ 
      continue;
      }
    /* remove the .h and store */
    argv[i][strlen(argv[i])-2] = '\0';
    names[j++] = strdup(argv[i]);    
    }
  anindex = j;
  
  fprintf(stdout,"#include \"vtkTclUtil.h\"\n");

  stuffit();
  
  return 0;
}

