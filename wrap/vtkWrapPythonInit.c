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
    fprintf(stdout,"extern \"C\" { void init%s(); }\n",names[i]);
    }

  fprintf(stdout,"\nstatic PyMethodDef Py%s_ClassMethods[] = {\n",
	  kitName);
  fprintf(stdout,"{NULL, NULL}};\n\n");
  
  fprintf(stdout,"extern \"C\" { void init%s();}\n\n",kitName);

  /* module init function */
  fprintf(stdout,"void init%s()\n{\n",kitName);
  fprintf(stdout,"  PyObject *m1, *d1, *d2, *n, *m2, *meth;\n\n");
  fprintf(stdout,"  m1 = Py_InitModule(\"%s\", Py%s_ClassMethods);\n",
	  kitName, kitName);
  
  fprintf(stdout,"  d1 = PyModule_GetDict(m1);\n");
  fprintf(stdout,"  if (!d1) Py_FatalError(\"can't get dictionary for module %s!\");\n\n",
	  kitName);
  fprintf(stdout,"  n = PyString_FromString(\"New\");\n");

  for (i = 0; i < anindex; i++)
    {
      fprintf(stdout,"  init%s();\n",names[i]);
      fprintf(stdout,"  m2 = PyImport_ImportModule(\"%s\");\n", names[i]);
      fprintf(stdout,"  if (!m2) Py_FatalError(\"can't initialize module %s!\");\n",
	      names[i]);
      fprintf(stdout,"  d2 = PyModule_GetDict(m2);\n");
      fprintf(stdout,"  meth = PyDict_GetItem(d2, n);\n");
      fprintf(stdout,"  if (-1 == PyDict_SetItemString(d1, \"%s\", (meth?meth:m2)))\n",names[i]);
      fprintf(stdout,"    Py_FatalError(\"can't add module %s to dictionary!\");\n\n",
	      names[i]);
    }

  fprintf(stdout,"  Py_DECREF(n);\n");
  fprintf(stdout,"}\n\n");
};


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
    argv[i][strlen(argv[i])-2] = '\0';
    names[i-2] = strdup(argv[i]);
    }
  anindex = argc - 2;
  
  fprintf(stdout,"#include <string.h>\n");
  fprintf(stdout,"#include \"Python.h\"\n\n");

  stuffit();
  
  return 0;
}

