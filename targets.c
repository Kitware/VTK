#include <stdio.h>

int main (int argc, char *argv[])
{
  int concrete;
  int cxx;
  int num, i;
  FILE *fp;
  
  concrete = atoi(argv[1]);
  cxx = atoi(argv[2]);
  num = argc - 3;
  
  /* concrete should be called first */
  if (concrete&&cxx)
    {
    fp = fopen("targets.make","w");
    }
  else
    {
    fp = fopen("targets.make","a");
    }
  if (!fp)
    {
    fprintf(stderr,"Unable to open target.make for writing!");
    exit(1);
    }
  
  /* create SRC_OBJ */
  if (cxx)
    {
    fprintf(fp,"SRC_OBJ := $(SRC_OBJ) \\\n");
    for (i = 0; i < num; i++)
      {
      if (i) fprintf(fp,"\\\n");
      fprintf(fp,"%s.o ",argv[i+3]);
      }
    fprintf(fp,"\n\n");
    }
  
  /* create TCL_OBJ */
  fprintf(fp,"TCL_OBJ := $(TCL_OBJ) \\\n");
  for (i = 0; i < num; i++)
    {
    if (i) fprintf(fp,"\\\n");
    fprintf(fp,"tcl/%sTcl.o ",argv[i+3]);
    }
  fprintf(fp,"\n\n");
  
  /* create TCL_NEWS */
  if (concrete)
    {
    fprintf(fp,"TCL_NEWS := $(TCL_NEWS) \\\n");
    for (i = 0; i < num; i++)
      {
      if (i) fprintf(fp,"\\\n");
      fprintf(fp,"%s.h ",argv[i+3]);
      }
    fprintf(fp,"\n\n");
    }
  
  for (i = 0; i < num; i++)
    {
    fprintf(fp,"tcl/%sTcl.cxx: %s.h ../tcl/cpp_parse\n\trm -f tcl/%sTcl.cxx; ../tcl/cpp_parse ${srcdir}/%s.h ${srcdir}/../tcl/hints %i > tcl/%sTcl.cxx\n",
	    argv[i+3],argv[i+3],argv[i+3], argv[i+3], concrete, argv[i+3]);
    }

  /* create JAVA_CLASSES */
  fprintf(fp,"JAVA_CLASSES := $(JAVA_CLASSES) \\\n");
  for (i = 0; i < num; i++)
    {
    if (i) fprintf(fp,"\\\n");
    fprintf(fp,"../java/vtk/%s.java ",argv[i+3]);
    }
  fprintf(fp,"\n\n");

  /* create JAVA_CODE */
  fprintf(fp,"JAVA_CODE := $(JAVA_CODE) \\\n");
  for (i = 0; i < num; i++)
    {
    if (i) fprintf(fp,"\\\n");
    fprintf(fp,"../java/vtk/%s.class ",argv[i+3]);
    }
  fprintf(fp,"\n\n");

  /* create JAVA_O */
  fprintf(fp,"JAVA_O := $(JAVA_O) \\\n");
  for (i = 0; i < num; i++)
    {
    if (i) fprintf(fp,"\\\n");
    fprintf(fp,"java/vtk_%s.o ",argv[i+3]);
    }
  fprintf(fp,"\n\n");

  /* create JAVA_WRAP */
  fprintf(fp,"JAVA_WRAP := $(JAVA_WRAP) \\\n");
  for (i = 0; i < num; i++)
    {
    if (i) fprintf(fp,"\\\n");
    fprintf(fp,"java/%sJava.o ",argv[i+3]);
    }
  fprintf(fp,"\n\n");

  for (i = 0; i < num; i++)
    {
    fprintf(fp,"../java/vtk/%s.java: %s.h ../java/java_parse\n\trm -f ../java/vtk/%s.java; ../java/java_parse ${srcdir}/%s.h ${srcdir}/../tcl/hints > ../java/vtk/%s.java\n",
	    argv[i+3],argv[i+3],argv[i+3], argv[i+3], argv[i+3]);
    }

  for (i = 0; i < num; i++)
    {
    fprintf(fp,"java/%sJava.cxx: %s.h ../java/java_wrap java/vtk_%s.h\n\trm -f java/%sJava.cxx; ../java/java_wrap ${srcdir}/%s.h ${srcdir}/../tcl/hints > java/%sJava.cxx\n",
	    argv[i+3],argv[i+3],argv[i+3],argv[i+3], argv[i+3], argv[i+3]);
    }

  for (i = 0; i < num; i++)
    {
    fprintf(fp,"java/vtk_%s.h: ../java/vtk/%s.java\n\trm -f java/vtk_%s.h; ${JAVAH} -d java vtk.%s\n",
	    argv[i+3],argv[i+3],argv[i+3], argv[i+3]);
    }

  for (i = 0; i < num; i++)
    {
    fprintf(fp,"java/vtk_%s.c: ../java/vtk/%s.java\n\trm -f java/vtk_%s.c; ${JAVAH} -stubs -d java vtk.%s\n",
	    argv[i+3],argv[i+3],argv[i+3], argv[i+3]);
    }

  return 0;
}
