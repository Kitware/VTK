#ifndef __vtkTestUtilities_h
#define __vtkTestUtilities_h

struct vtkTestUtilities
{
  // Description:
  // Function necessary for accessing the root directory for VTK data.
  // The returned string has to be deleted (with delete[]) by the user.
  static char* GetDataRoot(int argc, char* argv[]);

  // Description:
  // Given a file name, this function returns a new string which
  // is (in theory) the full path. This path is constructed by
  // prepending the file name with a command line argument 
  // (-D path) or VTK_DATA_ROOT env. variable.
  static char* ExpandDataFileName(int argc, char* argv[], 
				  const char* fname);
};

char* vtkTestUtilities::GetDataRoot(int argc, char* argv[])
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
    char *root = getenv("VTK_DATA_ROOT");
    if (!root)
      {
      dataRoot = new char[strlen("../../../../VTKData")+1];
      strcpy(dataRoot, "../../../../VTKData");
      }
    else
      {
      dataRoot = new char[strlen(root)+1];
      strcpy(dataRoot, root);
      }
    }

  return dataRoot;

} 

char* vtkTestUtilities::ExpandDataFileName(int argc, char* argv[], 
					   const char* fname)
{
  char* fullName=0;
  char* dataRoot=GetDataRoot(argc, argv);
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

#endif // __vtkTestUtilities_h
