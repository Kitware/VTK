#include <stdio.h>

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
  
  /* start by counting */
  for (i = 1; i < argc; i++)
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

    fprintf(fp,"TCL_OBJ = ");
    for (i = concrete_start; i <= concrete_end; i++)
      {
      fprintf(fp,"\\\ntcl/%sTcl.o ",argv[i]);
      }
    for (i = abstract_start; i <= abstract_end; i++)
      {
      fprintf(fp,"\\\ntcl/%sTcl.o ",argv[i]);
      }
    fprintf(fp,"\n\n");
    }
  
  /* create TCL_NEWS */
  if ((concrete_num + concrete_h_num) > 0)
    {
    fprintf(fp,"TCL_NEWS = ");
    for (i = concrete_start; i <= concrete_end; i++)
      {
      fprintf(fp,"\\\n%s.h ",argv[i]);
      }
    for (i = abstract_start; i <= abstract_end; i++)
      {
      fprintf(fp,"\\\n%s.h ",argv[i]);
      }
    fprintf(fp,"\n\n");
    }
  
  /* some more tcl rules */
  for (i = concrete_start; i <= concrete_end; i++)
    {
    fprintf(fp,"tcl/%sTcl.cxx: %s.h ../tcl/cpp_parse\n\trm -f tcl/%sTcl.cxx; ../tcl/cpp_parse ${srcdir}/%s.h ${srcdir}/../tcl/hints %i > tcl/%sTcl.cxx\n",
	    argv[i],argv[i],argv[i], argv[i], 1, argv[i]);
    }
  for (i = abstract_start; i <= abstract_end; i++)
    {
    fprintf(fp,"tcl/%sTcl.cxx: %s.h ../tcl/cpp_parse\n\trm -f tcl/%sTcl.cxx; ../tcl/cpp_parse ${srcdir}/%s.h ${srcdir}/../tcl/hints %i > tcl/%sTcl.cxx\n",
	    argv[i],argv[i],argv[i], argv[i], 0, argv[i]);
    }
  for (i = concrete_h_start; i <= concrete_h_end; i++)
    {
    fprintf(fp,"tcl/%sTcl.cxx: %s.h ../tcl/cpp_parse\n\trm -f tcl/%sTcl.cxx; ../tcl/cpp_parse ${srcdir}/%s.h ${srcdir}/../tcl/hints %i > tcl/%sTcl.cxx\n",
	    argv[i],argv[i],argv[i], argv[i], 1, argv[i]);
    }
  for (i = abstract_h_start; i <= abstract_h_end; i++)
    {
    fprintf(fp,"tcl/%sTcl.cxx: %s.h ../tcl/cpp_parse\n\trm -f tcl/%sTcl.cxx; ../tcl/cpp_parse ${srcdir}/%s.h ${srcdir}/../tcl/hints %i > tcl/%sTcl.cxx\n",
	    argv[i],argv[i],argv[i], argv[i], 0, argv[i]);
    }

  /* create JAVA_CLASSES */
  fprintf(fp,"JAVA_CLASSES = ");
  for (i = 1; i < argc; i++)
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
  for (i = 1; i < argc; i++)
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
  for (i = 1; i < argc; i++)
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
  for (i = 1; i < argc; i++)
    {
    if (strcmp(argv[i],"concrete")&&strcmp(argv[i],"abstract")&&
	strcmp(argv[i],"concrete_h")&&strcmp(argv[i],"abstract_h"))
      {
      fprintf(fp,"\\\njava/%sJava.o ",argv[i]);
      }
    }
  fprintf(fp,"\n\n");

  for (i = 1; i < argc; i++)
    {
    if (strcmp(argv[i],"concrete")&&strcmp(argv[i],"abstract")&&
	strcmp(argv[i],"concrete_h")&&strcmp(argv[i],"abstract_h"))
      {
      fprintf(fp,"../java/vtk/%s.java: %s.h ../java/java_parse\n\trm -f ../java/vtk/%s.java; ../java/java_parse ${srcdir}/%s.h ${srcdir}/../tcl/hints > ../java/vtk/%s.java\n",
	      argv[i],argv[i],argv[i], argv[i], argv[i]);
      fprintf(fp,"java/%sJava.cxx: %s.h ../java/java_wrap java/vtk_%s.h\n\trm -f java/%sJava.cxx; ../java/java_wrap ${srcdir}/%s.h ${srcdir}/../tcl/hints > java/%sJava.cxx\n",
	      argv[i],argv[i],argv[i],argv[i], argv[i], argv[i]);
      fprintf(fp,"java/vtk_%s.h: ../java/vtk/%s.java\n\trm -f java/vtk_%s.h; ${JAVAH} -d java vtk.%s\n",
	      argv[i],argv[i],argv[i], argv[i]);
      fprintf(fp,"java/vtk_%s.c: ../java/vtk/%s.java\n\trm -f java/vtk_%s.c; ${JAVAH} -stubs -d java vtk.%s\n",
	      argv[i],argv[i],argv[i], argv[i]);
      }
    }

  return 0;
}
