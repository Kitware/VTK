#include "vtkObjectFactory.h"
#include "vtkObjectFactoryCollection.h"
#include "vtkDynamicLoader.h"
#include "vtkDirectory.h"
#include "vtkVersion.h"
#include <stdlib.h>
#include <ctype.h>


vtkObjectFactoryCollection* vtkObjectFactory::RegisterdFactories = 0;


// Create an instance of a named vtk object using the loaded
// factories

vtkObject* vtkObjectFactory::CreateInstance(const char* vtkclassname)
{
  if(!vtkObjectFactory::RegisterdFactories)
    {
    vtkObjectFactory::Init();
    }
  
  vtkObjectFactory* factory = 0;
  
  for(vtkObjectFactory::RegisterdFactories->InitTraversal();
      (factory = vtkObjectFactory::RegisterdFactories->GetNextItem());)
    {
    vtkObject* newobject = factory->CreateObject(vtkclassname);
    if(newobject)
      {
      return newobject;
      }
    }
  return 0;
}




// A one time initialization method.   
void vtkObjectFactory::Init()
{
  // Don't do anything if we are already initialized
  if(vtkObjectFactory::RegisterdFactories)
    {
    return;
    }
  
  vtkObjectFactory::RegisterdFactories = vtkObjectFactoryCollection::New();
  vtkObjectFactory::RegisterDefaults();
  vtkObjectFactory::LoadDynamicFactories();
}


// Register any factories that are always present in VTK like
// the OpenGL factory, currently this is not done.

void vtkObjectFactory::RegisterDefaults()
{
}


// Load all libraries in VTK_AUTOLOAD_PATH

void vtkObjectFactory::LoadDynamicFactories()
{
  // follow PATH convensions
#ifdef _WIN32
  char PathSeparator = ';';
#else
  char PathSeparator = ':';
#endif
  char* LoadPath = getenv("VTK_AUTOLOAD_PATH");
  if(LoadPath == 0)
    {
    return;
    }
  
  char* CurrentPath = new char[strlen(LoadPath)+1];
  char* SeparatorPosition = LoadPath; // initialize to env variable
  while(SeparatorPosition)
  {
    int PathLength =0;
    // find PathSeparator in LoadPath
    SeparatorPosition = strchr(LoadPath, PathSeparator);
    // if not found then use the whole string
    if(SeparatorPosition == 0)
    {
      PathLength = strlen(LoadPath);
    }
    else
    {
      PathLength = SeparatorPosition - LoadPath;
    }
    // copy the path out of LoadPath into CurrentPath
    strncpy(CurrentPath, LoadPath, PathLength);
    // add a null terminator
    CurrentPath[PathLength] = 0;
    // Get ready for the next path 
    LoadPath = SeparatorPosition+1;
    // Load the libraries in the current path
    vtkObjectFactory::LoadLibrariesInPath(CurrentPath);
  }
  // clean up memory
  delete [] CurrentPath;
}

// A file scope helper function to concat path and file into
// a full path
static char* CreateFullPath(const char* path, const char* file)
{
  int lenpath = strlen(path);
  char* ret = new char[lenpath + strlen(file)+2];
#ifdef _WIN32
  const char sep = '\\';
#else
  const char sep = '/';
#endif
  // make sure the end of path is a separator
  strcpy(ret, path);
  if(ret[lenpath-1] != sep)
    {
    ret[lenpath] = sep;
    ret[lenpath+1] = 0;
    }
  strcat(ret, file);
  return ret;
}

// A file scope typedef to make the cast code to the load
// function cleaner to read.
typedef vtkObjectFactory* (* VTK_LOAD_FUNCTION)();


// A file scoped function to determine if a file has
// the shared library extension in its name, this converts name to lower
// case before the compare, vtkDynamicLoader always uses
// lower case for LibExtension values. 

inline int vtkNameIsSharedLibrary(const char* name)
{
  int len = strlen(name);
  char* copy = new char[len+1];
  
  for(int i = 0; i < len; i++)
    {
    copy[i] = tolower(name[i]);
    }
  char* ret = strstr(copy, vtkDynamicLoader::LibExtension());
  delete [] copy;
  return (ret != NULL);
}

void vtkObjectFactory::LoadLibrariesInPath(const char* path)
{
  vtkDirectory* dir = vtkDirectory::New();
  if(!dir->Open(path))
    {
    return;
    }
  
  // Attempt to load each file in the directory as a shared library
  for(int i = 0; i < dir->GetNumberOfFiles(); i++)
    {
    const char* file = dir->GetFile(i);
    // try to make sure the file has at least the extension
    // for a shared library in it.
    if(vtkNameIsSharedLibrary(file))
      {
      char* fullpath = CreateFullPath(path, file);
      vtkLibHandle lib = vtkDynamicLoader::OpenLibrary(fullpath);
      if(lib)
	{
	// Look for the symbol vtkLoad in the library
	VTK_LOAD_FUNCTION loadfunction
	  = (VTK_LOAD_FUNCTION)vtkDynamicLoader::GetSymbolAddress(lib, "vtkLoad");
	// if the symbol is found call it to create the factory
	// from the library
	if(loadfunction)
	  {
	  vtkObjectFactory* newfactory = (*loadfunction)();
	  // initialize class members if load worked
	  newfactory->LibraryHandle = (void*)lib;
	  newfactory->LibraryPath = strcpy(new char[strlen(fullpath)+1], fullpath);
	  newfactory->LibraryDate = 0; // unused for now...
	  vtkObjectFactory::RegisterFactory(newfactory);
	  }
	}
      delete [] fullpath;
      }
    }
}


// Recheck the VTK_AUTOLOAD_PATH for new libraries

void vtkObjectFactory::ReHash()
{
  vtkObjectFactory::UnRegisterAllFactories();
  vtkObjectFactory::Init();
}

// initialize class members
vtkObjectFactory::vtkObjectFactory()
:LibraryHandle(0), LibraryPath(0), LibraryDate(0)
{
}


// Unload the library and free the path string
vtkObjectFactory::~vtkObjectFactory()
{
  delete [] this->LibraryPath;
  vtkDynamicLoader::CloseLibrary((vtkLibHandle)this->LibraryHandle);
}

// Add a factory to the registerd list
void vtkObjectFactory::RegisterFactory(vtkObjectFactory* factory)
{
  if(factory->LibraryHandle == 0)
    {
    const char* nonDynamicName = "Non-Dynamic loaded vtkObjectFactory";
    factory->LibraryPath = strcpy(new char[strlen(nonDynamicName)+1], 
				  nonDynamicName);
    }
  if(strcmp(factory->GetVTKSourceVersion(), 
	    vtkVersion::GetVTKSourceVersion()) != 0)
    {
    vtkGenericWarningMacro(<< "Possible incompatible factory load:" 
    << "\nRunning vtk version :\n" << vtkVersion::GetVTKSourceVersion() 
    << "\nLoaded Factory version:\n" << factory->GetVTKSourceVersion()
    << "\nLoading factory: " << factory->LibraryPath << "\n");
    }
  vtkObjectFactory::Init();
  vtkObjectFactory::RegisterdFactories->AddItem(factory);
}


void vtkObjectFactory::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkObject::PrintSelf(os, indent);
  os << indent 
     << "vtk Object Factory loaded from " << this->LibraryPath << "\n";
}

void vtkObjectFactory::UnRegisterFactory(vtkObjectFactory* factory)
{ 
  vtkObjectFactory::RegisterdFactories->RemoveItem(factory);
  factory->Delete();
}

// unregister all factories and delete the RegisterdFactories list
void vtkObjectFactory::UnRegisterAllFactories()
{
  vtkObjectFactory* factory = 0;
  
  for(vtkObjectFactory::RegisterdFactories->InitTraversal();
      (factory = vtkObjectFactory::RegisterdFactories->GetNextItem());)
    {
    factory->Delete();
    }
  vtkObjectFactory::RegisterdFactories->RemoveAllItems();
  vtkObjectFactory::RegisterdFactories->Delete();
  vtkObjectFactory::RegisterdFactories = 0;
}

  
