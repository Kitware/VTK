#include "vtkObjectFactory.h"
#include "vtkObjectFactoryCollection.h"
#include "vtkDynamicLoader.h"
#include "vtkDirectory.h"

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
      return newobject;
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
  const char* seps = ";";
#else
  const char* seps = ":";
#endif
  
  char* loadpath = getenv("VTK_AUTOLOAD_PATH");
  if(!loadpath)
    {
    return;
    }
  
  const char* path = strtok( loadpath, seps );
  while( path != NULL )   
    {
    vtkObjectFactory::LoadLibrariesInPath(path);
    path = strtok( NULL, seps );
    }
}

// A full scope helper function to concat path and file into
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
	// initialize class members 
	newfactory->LibraryHandle = (void*)lib;
	newfactory->LibraryPath = strcpy(new char[strlen(fullpath)+1], fullpath);
	newfactory->LibraryDate = 0; // unused for now...
	vtkObjectFactory::RegisterFactory(newfactory);
	}
      }
    delete [] fullpath;
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
  vtkObjectFactory::Init();
  vtkObjectFactory::RegisterdFactories->AddItem(factory);
  if(factory->LibraryHandle == 0)
    {
    const char* nonDynamicName = "Non-Dynamic loaded vtkObjectFactory";
    factory->LibraryPath = strcpy(new char[strlen(nonDynamicName)+1], 
				  nonDynamicName);
    }
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

  
