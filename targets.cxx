#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern void OutputUNIXDepends(char *file, FILE *fp, const char *vtkHome);

int main (int argc, char *argv[])
{
  int concrete;
  int cxx;
  int concrete_num, concrete_start, concrete_end;
  int abstract_num, abstract_start, abstract_end;
  int concrete_h_num, concrete_h_start, concrete_h_end;
  int abstract_h_num, abstract_h_start, abstract_h_end;
  int i, typenum;
  FILE *fp;
  char *vtkLocal = argv[1];
  char vtkHome[256];
  char filename[1024];
  
  sprintf(vtkHome,"%s/..",vtkLocal);
  
  /* start by counting */
  for (i = 2; i < argc; i++)
    {
    if (!strcmp(argv[i],"concrete"))
      {
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
  concrete_num = concrete_end - concrete_start + 1;
  abstract_num = abstract_end - abstract_start + 1;
  concrete_h_num = concrete_h_end - concrete_h_start + 1;
  abstract_h_num = abstract_h_end - abstract_h_start + 1;
  
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
      OutputUNIXDepends(filename,fp,vtkHome);
      fprintf(fp,"\n");
      }
    for (i = abstract_start; i <= abstract_end; i++)
      {
      fprintf(fp,"%s.o : %s/%s.cxx ",argv[i],vtkLocal, argv[i]);
      sprintf(filename,"%s/%s.cxx",vtkLocal,argv[i]);
      OutputUNIXDepends(filename,fp,vtkHome);
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
    OutputUNIXDepends(filename,fp,vtkHome);
    fprintf(fp,"\n");
    fprintf(fp,"vtkXRenderTclWindowInteractor.o : %s/vtkXRenderTclWindowInteractor.cxx",
	    vtkLocal,vtkLocal,argv[i]);
    sprintf(filename,"%s/vtkXRenderTclWindowInteractor.cxx",vtkLocal);
    OutputUNIXDepends(filename,fp,vtkHome);
    fprintf(fp,"\n");
    }
      
  // generate depends for all the tcl wrappers
  for (i = 2; i < argc; i++)
    {
    if (strcmp(argv[i],"concrete")&&strcmp(argv[i],"abstract")&&
	strcmp(argv[i],"concrete_h")&&strcmp(argv[i],"abstract_h"))
      {
      fprintf(fp,"tcl/%sTcl.cxx : %s/%s.h %s/common/vtkTclUtil.h %s/tcl/cpp_parse.y ",
	      argv[i],vtkLocal,argv[i],vtkHome,vtkHome);
      sprintf(filename,"%s/%s.h",vtkLocal,argv[i]);
      OutputUNIXDepends(filename,fp,vtkHome);
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
  for (i = 2; i < argc; i++)
    {
    if (strcmp(argv[i],"concrete")&&strcmp(argv[i],"abstract")&&
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
    fprintf(fp,"tcl/%sTcl.cxx: %s.h ../tcl/cpp_parse ../tcl/hints\n\trm -f tcl/%sTcl.cxx; ../tcl/cpp_parse ${srcdir}/%s.h ${srcdir}/../tcl/hints %i > tcl/%sTcl.cxx\n",
	    argv[i],argv[i],argv[i], argv[i], 1, argv[i]);
    }
  for (i = abstract_start; i <= abstract_end; i++)
    {
    fprintf(fp,"tcl/%sTcl.cxx: %s.h ../tcl/cpp_parse ../tcl/hints\n\trm -f tcl/%sTcl.cxx; ../tcl/cpp_parse ${srcdir}/%s.h ${srcdir}/../tcl/hints %i > tcl/%sTcl.cxx\n",
	    argv[i],argv[i],argv[i], argv[i], 0, argv[i]);
    }
  for (i = concrete_h_start; i <= concrete_h_end; i++)
    {
    fprintf(fp,"tcl/%sTcl.cxx: %s.h ../tcl/cpp_parse ../tcl/hints\n\trm -f tcl/%sTcl.cxx; ../tcl/cpp_parse ${srcdir}/%s.h ${srcdir}/../tcl/hints %i > tcl/%sTcl.cxx\n",
	    argv[i],argv[i],argv[i], argv[i], 1, argv[i]);
    }
  for (i = abstract_h_start; i <= abstract_h_end; i++)
    {
    fprintf(fp,"tcl/%sTcl.cxx: %s.h ../tcl/cpp_parse ../tcl/hints\n\trm -f tcl/%sTcl.cxx; ../tcl/cpp_parse ${srcdir}/%s.h ${srcdir}/../tcl/hints %i > tcl/%sTcl.cxx\n",
	    argv[i],argv[i],argv[i], argv[i], 0, argv[i]);
    }

  /* create JAVA_CLASSES */
  fprintf(fp,"JAVA_CLASSES = ");
  for (i = 2; i < argc; i++)
    {
    if (strcmp(argv[i],"concrete")&&strcmp(argv[i],"abstract")&&
	strcmp(argv[i],"concrete_h")&&strcmp(argv[i],"abstract_h"))
      {
      fprintf(fp,"\\\n../java/vtk/%s.java ",argv[i]);
      }
    }
  fprintf(fp,"\n\n");
  
  /* create JAVA_CODE */
  fprintf(fp,"JAVA_CODE = ");
  for (i = 2; i < argc; i++)
    {
    if (strcmp(argv[i],"concrete")&&strcmp(argv[i],"abstract")&&
	strcmp(argv[i],"concrete_h")&&strcmp(argv[i],"abstract_h"))
      {
      fprintf(fp,"\\\n../java/vtk/%s.class ",argv[i]);
      }
    }
  fprintf(fp,"\n\n");

  /* create JAVA_O */
  fprintf(fp,"JAVA_O = ");
  for (i = 2; i < argc; i++)
    {
    if (strcmp(argv[i],"concrete")&&strcmp(argv[i],"abstract")&&
	strcmp(argv[i],"concrete_h")&&strcmp(argv[i],"abstract_h"))
      {
      fprintf(fp,"\\\njava/vtk_%s.o ",argv[i]);
      }
    }
  fprintf(fp,"\n\n");

  /* create JAVA_WRAP */
  fprintf(fp,"JAVA_WRAP = ");
  for (i = 2; i < argc; i++)
    {
    if (strcmp(argv[i],"concrete")&&strcmp(argv[i],"abstract")&&
	strcmp(argv[i],"concrete_h")&&strcmp(argv[i],"abstract_h"))
      {
      fprintf(fp,"\\\njava/%sJava.o ",argv[i]);
      }
    }
  fprintf(fp,"\n\n");

  for (i = 2; i < argc; i++)
    {
    if (strcmp(argv[i],"concrete")&&strcmp(argv[i],"abstract")&&
	strcmp(argv[i],"concrete_h")&&strcmp(argv[i],"abstract_h"))
      {
      fprintf(fp,"../java/vtk/%s.java: %s.h ../java/java_parse ../tcl/hints\n\trm -f ../java/vtk/%s.java; ../java/java_parse ${srcdir}/%s.h ${srcdir}/../tcl/hints > ../java/vtk/%s.java\n",
	      argv[i],argv[i],argv[i], argv[i], argv[i]);
      fprintf(fp,"java/%sJava.cxx: %s.h ../java/java_wrap ../tcl/hints\n\trm -f java/%sJava.cxx; ../java/java_wrap ${srcdir}/%s.h ${srcdir}/../tcl/hints > java/%sJava.cxx\n",
	      argv[i],argv[i],argv[i], argv[i], argv[i]);
      }
    }

  return 0;
}
