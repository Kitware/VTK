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
    fprintf(stdout,"extern \"C\" { PyObject *PyVTKClass_%sNew(char *); }\n",
	    names[i]);
    }

  fprintf(stdout,"\nstatic PyMethodDef Py%s_ClassMethods[] = {\n",
	  kitName);
  fprintf(stdout,"{NULL, NULL}};\n\n");
  
  fprintf(stdout,"extern \"C\" { void init%s();}\n\n",kitName);

  /* module init function */
  fprintf(stdout,"void init%s()\n{\n",kitName);
  fprintf(stdout,"  PyObject *m, *d, *c;\n\n");
  fprintf(stdout,"  static char modulename[] = \"%s\";\n",kitName);
  fprintf(stdout,"  m = Py_InitModule(modulename, Py%s_ClassMethods);\n",
	  kitName);
  
  fprintf(stdout,"  d = PyModule_GetDict(m);\n");
  fprintf(stdout,"  if (!d) Py_FatalError(\"can't get dictionary for module %s!\");\n\n",
	  kitName);

  for (i = 0; i < anindex; i++)
    {
    fprintf(stdout,"  if ((c = PyVTKClass_%sNew(modulename)))\n",names[i]);
    fprintf(stdout,"    if (-1 == PyDict_SetItemString(d, \"%s\", c))\n",
	    names[i]);
    fprintf(stdout,"      Py_FatalError(\"can't add class %s to dictionary!\");\n\n",
	    names[i]);
    }
  fprintf(stdout,"}\n\n");
}

int main(int argc,char *argv[])
{
  int i;

  if (argc < 3)
    {
    fprintf(stderr,"Usage: %s kit_name file1 file2 file3 ...\n",argv[0]);
    exit(1);
    }
  
  kitName = strdup(argv[1]);
  
  /* fill in the correct arrays */
  for (i = 2; i < argc; i++)
    {
    /* remove the .h and store */
    names[i-2] = strdup(argv[i]);
    names[i-2][strlen(argv[i])-2] = '\0';
    }
  anindex = argc - 2;

  fprintf(stdout,"#include <string.h>\n");
  fprintf(stdout,"#include \"Python.h\"\n\n");

  stuffit();
  
  return 0;
}

