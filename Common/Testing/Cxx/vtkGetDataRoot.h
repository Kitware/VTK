#ifndef __vtkGetDataRoot_h
#define __vtkGetDataRoot_h

// Function necessary for accessing the root directory for VTK data.
// The returned string has to be deleted (with delete[]) by the user.
char* vtkGetDataRoot(int argc, char* argv[])
{
  int dataIndex=-1;

  for (int i=0; i<argc; i++)
    {
    if ( strcmp("-D", argv[i]) == 0 )
      {
      if ( i < argc-1 )
	{
	dataIndex = i+1;
	}
      }
    }

  char* dataRoot = 0;
  if ( dataIndex != -1 ) 
    {
    dataRoot = new char[strlen(argv[dataIndex])+1];
    strcpy(dataRoot, argv[dataIndex]);
    }
  else 
    {
    dataRoot = getenv("VTK_DATA_ROOT");
    }

  return dataRoot;

} 

// Given a file name, this function returns a new string which
// is (in theory) the full path. This path is constructed by
// prepending the file name with a command line argument 
// (-D path) or VTK_DATA_ROOT env. variable.
char* vtkExpandDataFileName(int argc, char* argv[], const char* fname)
{
  char* fullName=0;
  char* dataRoot=vtkGetDataRoot(argc, argv);
  if (dataRoot)
    {
    fullName = new char[strlen(dataRoot)+strlen(fname)+2];
    fullName[0] = 0;
    strcat(fullName, dataRoot);
    int len = strlen(fullName);
    fullName[len] = '/';
    fullName[len+1] = 0;
    strcat(fullName, fname);
    }
  else
    {
    fullName = new char[strlen(fname)+1];
    strcpy(fullName, fname);
    }
  delete[] dataRoot;
  return fullName;
}

#endif // __vtkGetDataRoot_h
