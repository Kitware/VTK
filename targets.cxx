#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern void OutputUNIXDepends(char *file, FILE *fp);
extern void AddToDepends(const char *filename);
extern void BuildDepends(const char *vtkHome, char *argv[],
			 int extra_start, int extra_num);
void SetupDepends(const char *vtkLocal, const char *vtkHome, int argc, char *argv[]);

int concrete_num, concrete_start, concrete_end;
int abstract_num, abstract_start, abstract_end;
int concrete_h_num, concrete_h_start, concrete_h_end;
int abstract_h_num, abstract_h_start, abstract_h_end;
int extra_num, extra_start, extra_end;

int main (int argc, char *argv[])
{
  int i;
  FILE *fp;
  char *vtkLocal = argv[2];
  char *vtkHome = argv[1];
  char filename[1024];
  
  /* start by counting */
  for (i = 3; i < argc; i++)
    {
    if (!strcmp(argv[i],"extra"))
      {
      extra_start = i + 1;
      }
    if (!strcmp(argv[i],"concrete"))
      {
      extra_end = i - 1;
      concrete_start = i+1;
      }
    if (!strcmp(argv[i],"abstract"))   
      {
      concrete_end = i -1;
      abstract_start = i+1;
      }
    if (!strcmp(argv[i],"concrete_h")) 
      {
      abstract_end = i -1;
      concrete_h_start = i+1;
      }
    if (!strcmp(argv[i],"abstract_h")) 
      {
      concrete_h_end = i -1;
      abstract_h_start = i+1;
      abstract_h_end = argc - 1;
      }
    }
  extra_num = extra_end - extra_start + 1;
  concrete_num = concrete_end - concrete_start + 1;
  abstract_num = abstract_end - abstract_start + 1;
  concrete_h_num = concrete_h_end - concrete_h_start + 1;
  abstract_h_num = abstract_h_end - abstract_h_start + 1;

  SetupDepends(vtkLocal,vtkHome,argc,argv);

  /* concrete should be called first */
  fp = fopen("targets.make","w");
  if (!fp)
    {
    fprintf(stderr,"Unable to open target.make for writing!");
    exit(1);
    }
  
  // for all the .cxx files generate depends
  if ((concrete_num + abstract_num) > 0)
    {
    for (i = concrete_start; i <= concrete_end; i++)
      {
      fprintf(fp,"%s.o : %s/%s.cxx ",argv[i],vtkLocal,argv[i]);
      sprintf(filename,"%s/%s.cxx",vtkLocal,argv[i]);
      OutputUNIXDepends(filename,fp);
      fprintf(fp,"\n");
      }
    for (i = abstract_start; i <= abstract_end; i++)
      {
      fprintf(fp,"%s.o : %s/%s.cxx ",argv[i],vtkLocal, argv[i]);
      sprintf(filename,"%s/%s.cxx",vtkLocal,argv[i]);
      OutputUNIXDepends(filename,fp);
      fprintf(fp,"\n");
      }
    fprintf(fp,"\n\n");
    }

  // if this is the graphics library we need to add dependencies
  // for two odd classes vtkXRenderWindowInteractor and
  // vtkXRenderTclWindowInteractor
  if (!strcmp(vtkLocal + strlen(vtkLocal) - 8,"graphics"))
    {
    fprintf(fp,"vtkXRenderWindowInteractor.o : %s/vtkXRenderWindowInteractor.cxx",
	    vtkLocal,vtkLocal,argv[i]);
    sprintf(filename,"%s/vtkXRenderWindowInteractor.cxx",vtkLocal);
    OutputUNIXDepends(filename,fp);
    fprintf(fp,"\n");
    fprintf(fp,"vtkXRenderTclWindowInteractor.o : %s/vtkXRenderTclWindowInteractor.cxx",
	    vtkLocal,vtkLocal,argv[i]);
    sprintf(filename,"%s/vtkXRenderTclWindowInteractor.cxx",vtkLocal);
    OutputUNIXDepends(filename,fp);
    fprintf(fp,"\n");
    fprintf(fp,"vtkTkRenderWidget.o : %s/vtkTkRenderWidget.cxx",
	    vtkLocal,vtkLocal,argv[i]);
    sprintf(filename,"%s/vtkTkRenderWidget.cxx",vtkLocal);
    OutputUNIXDepends(filename,fp);
    fprintf(fp,"\n");
    }
      
  // if this is the imaging library we need to add dependencies
  if (!strcmp(vtkLocal + strlen(vtkLocal) - 7,"imaging"))
    {
    fprintf(fp,"vtkTkImageViewerWidget.o : %s/vtkTkImageViewerWidget.cxx",
	    vtkLocal,vtkLocal,argv[i]);
    sprintf(filename,"%s/vtkTkImageViewerWidget.cxx",vtkLocal);
    OutputUNIXDepends(filename,fp);
    fprintf(fp,"\n");
    }

  // generate depends for all the tcl wrappers
  for (i = concrete_start; i < argc; i++)
    {
    if (strcmp(argv[i],"abstract")&&
	strcmp(argv[i],"concrete_h")&&strcmp(argv[i],"abstract_h"))
      {
      fprintf(fp,"tcl/%sTcl.cxx : %s/%s.h %s/common/vtkTclUtil.h %s/tcl/cpp_parse.y ",
	      argv[i],vtkLocal,argv[i],vtkHome,vtkHome);
      sprintf(filename,"%s/%s.h",vtkLocal,argv[i]);
      OutputUNIXDepends(filename,fp);
      fprintf(fp,"\n");
      }
    }
  fprintf(fp,"\n\n");

  /* create SRC_OBJ */
  /* create TCL_OBJ */
  if ((concrete_num + abstract_num) > 0)
    {
    fprintf(fp,"SRC_OBJ = ");
    for (i = concrete_start; i <= concrete_end; i++)
      {
      fprintf(fp,"\\\n%s.o ",argv[i]);
      }
    for (i = abstract_start; i <= abstract_end; i++)
      {
      fprintf(fp,"\\\n%s.o ",argv[i]);
      }
    fprintf(fp,"\n\n");
    }
  
  fprintf(fp,"TCL_OBJ = ");
  for (i = concrete_start; i < argc; i++)
    {
    if (strcmp(argv[i],"abstract")&&
	strcmp(argv[i],"concrete_h")&&strcmp(argv[i],"abstract_h"))
      {
      fprintf(fp,"\\\ntcl/%sTcl.o ",argv[i]);
      }
    }
  fprintf(fp,"\n\n");

  /* create TCL_NEWS */
  if ((concrete_num + concrete_h_num) > 0)
    {
    fprintf(fp,"TCL_NEWS = ");
    for (i = concrete_start; i <= concrete_end; i++)
      {
      fprintf(fp,"\\\n%s.h ",argv[i]);
      }
    for (i = concrete_h_start; i <= concrete_h_end; i++)
      {
      fprintf(fp,"\\\n%s.h ",argv[i]);
      }
    fprintf(fp,"\n\n");
    }
  
  /* some more tcl rules */
  for (i = concrete_start; i <= concrete_end; i++)
    {
    fprintf(fp,"tcl/%sTcl.cxx: %s.h ${VTK_OBJ}/tcl/cpp_parse ../tcl/hints\n\trm -f tcl/%sTcl.cxx; ${VTK_OBJ}/tcl/cpp_parse ${srcdir}/%s.h ${srcdir}/../tcl/hints %i > tcl/%sTcl.cxx\n",
	    argv[i],argv[i],argv[i], argv[i], 1, argv[i]);
    }
  for (i = abstract_start; i <= abstract_end; i++)
    {
    fprintf(fp,"tcl/%sTcl.cxx: %s.h ${VTK_OBJ}/tcl/cpp_parse ../tcl/hints\n\trm -f tcl/%sTcl.cxx; ${VTK_OBJ}/tcl/cpp_parse ${srcdir}/%s.h ${srcdir}/../tcl/hints %i > tcl/%sTcl.cxx\n",
	    argv[i],argv[i],argv[i], argv[i], 0, argv[i]);
    }
  for (i = concrete_h_start; i <= concrete_h_end; i++)
    {
    fprintf(fp,"tcl/%sTcl.cxx: %s.h ${VTK_OBJ}/tcl/cpp_parse ../tcl/hints\n\trm -f tcl/%sTcl.cxx; ${VTK_OBJ}/tcl/cpp_parse ${srcdir}/%s.h ${srcdir}/../tcl/hints %i > tcl/%sTcl.cxx\n",
	    argv[i],argv[i],argv[i], argv[i], 1, argv[i]);
    }
  for (i = abstract_h_start; i <= abstract_h_end; i++)
    {
    fprintf(fp,"tcl/%sTcl.cxx: %s.h ${VTK_OBJ}/tcl/cpp_parse ../tcl/hints\n\trm -f tcl/%sTcl.cxx; ${VTK_OBJ}/tcl/cpp_parse ${srcdir}/%s.h ${srcdir}/../tcl/hints %i > tcl/%sTcl.cxx\n",
	    argv[i],argv[i],argv[i], argv[i], 0, argv[i]);
    }

  /* create JAVA_CLASSES */
  fprintf(fp,"JAVA_CLASSES = ");
  for (i = concrete_start; i < argc; i++)
    {
    if (strcmp(argv[i],"abstract")&&
	strcmp(argv[i],"concrete_h")&&strcmp(argv[i],"abstract_h"))
      {
      fprintf(fp,"\\\n../java/vtk/%s.java ",argv[i]);
      }
    }
  fprintf(fp,"\n\n");
  
  /* create JAVA_CODE */
  fprintf(fp,"JAVA_CODE = ");
  for (i = concrete_start; i < argc; i++)
    {
    if (strcmp(argv[i],"abstract")&&
	strcmp(argv[i],"concrete_h")&&strcmp(argv[i],"abstract_h"))
      {
      fprintf(fp,"\\\n../java/vtk/%s.class ",argv[i]);
      }
    }
  fprintf(fp,"\n\n");

  /* create JAVA_WRAP */
  fprintf(fp,"JAVA_WRAP = ");
  for (i = concrete_start; i < argc; i++)
    {
    if (strcmp(argv[i],"abstract")&&
	strcmp(argv[i],"concrete_h")&&strcmp(argv[i],"abstract_h"))
      {
      fprintf(fp,"\\\njava/%sJava.o ",argv[i]);
      }
    }
  fprintf(fp,"\n\n");

  for (i = concrete_start; i < argc; i++)
    {
    if (strcmp(argv[i],"abstract")&&
	strcmp(argv[i],"concrete_h")&&strcmp(argv[i],"abstract_h"))
      {
      fprintf(fp,"../java/vtk/%s.java: %s.h ${VTK_OBJ}/java/java_parse ../tcl/hints\n\trm -f ../java/vtk/%s.java; ${VTK_OBJ}/java/java_parse ${srcdir}/%s.h ${srcdir}/../tcl/hints > ../java/vtk/%s.java\n",
	      argv[i],argv[i],argv[i], argv[i], argv[i]);
      fprintf(fp,"java/%sJava.cxx: %s.h ${VTK_OBJ}/java/java_wrap ../tcl/hints\n\trm -f java/%sJava.cxx; ${VTK_OBJ}/java/java_wrap ${srcdir}/%s.h ${srcdir}/../tcl/hints > java/%sJava.cxx\n",
	      argv[i],argv[i],argv[i], argv[i], argv[i]);
      }
    }

  /* create PYTHON_WRAP */
  fprintf(fp,"PYTHON_WRAP = ");
  for (i = concrete_start; i < argc; i++)
    {
    if (strcmp(argv[i],"abstract")&&
	strcmp(argv[i],"concrete_h")&&strcmp(argv[i],"abstract_h"))
      {
      fprintf(fp,"\\\npython/%sPython.o ",argv[i]);
      }
    }
  fprintf(fp,"\n\n");

  for (i = concrete_start; i < argc; i++)
    {
    if (strcmp(argv[i],"abstract")&&
	strcmp(argv[i],"concrete_h")&&strcmp(argv[i],"abstract_h"))
      {
      fprintf(fp,"python/%sPython.cxx: %s.h ${VTK_OBJ}/python/python_wrap ../tcl/hints\n\trm -f python/%sPython.cxx; ${VTK_OBJ}/python/python_wrap ${srcdir}/%s.h ${srcdir}/../tcl/hints > python/%sPython.cxx\n",
	      argv[i],argv[i],argv[i], argv[i], argv[i]);
      }
    }

  return 0;
}




// all files which will have OutputUNIXDepends called for should be "setup" first in thsi function
void SetupDepends(const char *vtkLocal, const char *vtkHome, int argc, char *argv[])
{
  char filename[256];
  int i;

  // for all the .cxx files generate depends
  if ((concrete_num + abstract_num) > 0)
    {
    for (i = concrete_start; i <= concrete_end; i++)
      {
      sprintf(filename,"%s/%s.cxx",vtkLocal,argv[i]);
      AddToDepends(filename);
      }
    for (i = abstract_start; i <= abstract_end; i++)
      {
      sprintf(filename,"%s/%s.cxx",vtkLocal,argv[i]);
      AddToDepends(filename);
      }
    }


  // if this is the graphics library we need to add dependencies
  // for two odd classes vtkXRenderWindowInteractor and
  // vtkXRenderTclWindowInteractor
  if (!strcmp(vtkLocal + strlen(vtkLocal) - 8,"graphics"))
    {
    sprintf(filename,"%s/vtkXRenderWindowInteractor.cxx",vtkLocal);
    AddToDepends(filename);
    sprintf(filename,"%s/vtkXRenderTclWindowInteractor.cxx",vtkLocal);
    AddToDepends(filename);
    sprintf(filename,"%s/vtkTkRenderWidget.cxx",vtkLocal);
    AddToDepends(filename);
    }
  if (!strcmp(vtkLocal + strlen(vtkLocal) - 7,"imaging"))
    {
    sprintf(filename,"%s/vtkTkImageViewerWidget.cxx",vtkLocal);
    AddToDepends(filename);
    }
      
  // generate depends for all the tcl wrappers
  for (i = concrete_start; i < argc; i++)
    {
    if (strcmp(argv[i],"abstract")&&
	strcmp(argv[i],"concrete_h")&&strcmp(argv[i],"abstract_h"))
      {
      sprintf(filename,"%s/%s.h",vtkLocal,argv[i]);
      AddToDepends(filename);
      }
    }

  BuildDepends(vtkHome, argv, extra_start, extra_num);
}
