#include "vtkObjectFactory.h"
#include "vtkObjectFactoryCollection.h"
#include "vtkDynamicLoader.h"
#include "vtkDirectory.h"
#include "vtkVersion.h"
#include <stdlib.h>
#include <ctype.h>
#include "vtkDebugLeaks.h"

vtkObjectFactoryCollection* vtkObjectFactory::RegisteredFactories = 0;


// Create an instance of a named vtk object using the loaded
// factories

vtkObject* vtkObjectFactory::CreateInstance(const char* vtkclassname)
{
#ifdef VTK_DEBUG_LEAKS
  vtkDebugLeaks::ConstructClass(vtkclassname);
#endif
  if(!vtkObjectFactory::RegisteredFactories)
    {
    vtkObjectFactory::Init();
    }
  
  vtkObjectFactory* factory = 0;
  
  for(vtkObjectFactory::RegisteredFactories->InitTraversal();
      (factory = vtkObjectFactory::RegisteredFactories->GetNextItem());)
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
  if(vtkObjectFactory::RegisteredFactories)
    {
    return;
    }
  
  vtkObjectFactory::RegisteredFactories = vtkObjectFactoryCollection::New();
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
  copy[len] = 0;
  char* ret = strstr(copy, vtkDynamicLoader::LibExtension());
  delete [] copy;
  return (ret != NULL);
}

void vtkObjectFactory::LoadLibrariesInPath(const char* path)
{
  vtkDirectory* dir = vtkDirectory::New();
  if(!dir->Open(path))
    {
    dir->Delete();
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
	  newfactory->Delete();
	  }
	}
      delete [] fullpath;
      }
    }
  dir->Delete();
}


// Recheck the VTK_AUTOLOAD_PATH for new libraries

void vtkObjectFactory::ReHash()
{
  vtkObjectFactory::UnRegisterAllFactories();
  vtkObjectFactory::Init();
}

// initialize class members
vtkObjectFactory::vtkObjectFactory()
{
  this->LibraryHandle = 0;
  this->LibraryDate = 0;
  this->LibraryPath = 0;
  this->OverrideArray = 0;
  this->OverrideClassNames = 0;
  this->SizeOverrideArray = 0;
  this->OverrideArrayLength = 0;
}


// Unload the library and free the path string
vtkObjectFactory::~vtkObjectFactory()
{
  delete [] this->LibraryPath;
  this->LibraryPath = 0;
  
  for(int i =0; i < this->OverrideArrayLength; i++)
    {
    delete [] this->OverrideClassNames[i];
    delete [] this->OverrideArray[i].Description;
    delete [] this->OverrideArray[i].OverrideWithName;
    }
  delete [] this->OverrideArray;
  delete [] this->OverrideClassNames;
  this->OverrideArray = NULL;
  this->OverrideClassNames = NULL;
}

// Add a factory to the registered list
void vtkObjectFactory::RegisterFactory(vtkObjectFactory* factory)
{
  if(factory->LibraryHandle == 0)
    {
    const char* nonDynamicName = "Non-Dynamicly loaded factory";
    factory->LibraryPath = strcpy(new char[strlen(nonDynamicName)+1], 
				  nonDynamicName);
    }
  if(strcmp(factory->GetVTKSourceVersion(), 
	    vtkVersion::GetVTKSourceVersion()) != 0)
    {
    vtkGenericWarningMacro(<< "Possible incompatible factory load:" 
    << "\nRunning vtk version :\n" << vtkVersion::GetVTKSourceVersion() 
    << "\nLoaded Factory version:\n" << factory->GetVTKSourceVersion()
    << "\nLoading factory:\n" << factory->LibraryPath << "\n");
    }
  vtkObjectFactory::Init();
  vtkObjectFactory::RegisteredFactories->AddItem(factory);
}


void vtkObjectFactory::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkObject::PrintSelf(os, indent);
  os << indent 
     << "Factory DLL path: " << this->LibraryPath << "\n";
  os << indent << "Factory description: " << this->GetDescription() << endl;
  int num = this->GetNumberOfOverrides();
  os << indent << "Factory overides " << num << " classes:" << endl;
  indent = indent.GetNextIndent();
  for(int i =0; i < num; i++)
  {
    os << indent << "Class : " <<  this->GetClassOverrideName(i) << endl;
    os << indent << "Overriden with: " <<  this->GetClassOverrideWithName(i) 
       << endl;
    os << indent << "Enable flag: " <<  this->GetEnableFlag(i) << endl;
    os << endl;
  }
}

void vtkObjectFactory::UnRegisterFactory(vtkObjectFactory* factory)
{ 
  void* lib = factory->LibraryHandle;
  vtkObjectFactory::RegisteredFactories->RemoveItem(factory);
  if(lib)
    {
    vtkDynamicLoader::CloseLibrary((vtkLibHandle)lib);
    }
}

// unregister all factories and delete the RegisteredFactories list
void vtkObjectFactory::UnRegisterAllFactories()
{
  vtkObjectFactory* factory = 0;
  
  vtkObjectFactory::RegisteredFactories->InitTraversal();
  while((factory =
	 vtkObjectFactory::RegisteredFactories->GetNextItem()))
    {
    vtkObjectFactory::UnRegisterFactory(factory);
    vtkObjectFactory::RegisteredFactories->InitTraversal();
    }
  vtkObjectFactory::RegisteredFactories->Delete();
  vtkObjectFactory::RegisteredFactories = 0;
}



void vtkObjectFactory::RegisterOverride(const char* classOverride,
					const char* subclass,
					const char* description,
					int enableFlag,
					CreateFunction createFunction)
{
  this->GrowOverrideArray();
  int nextIndex = this->OverrideArrayLength;
  this->OverrideArrayLength++;
  char* override = strcpy(new char[strlen(classOverride)+1], classOverride);
  char* desc = strcpy(new char[strlen(description)+1], description);
  char* ocn =  strcpy(new char[strlen(subclass)+1], 
		      subclass);
  this->OverrideClassNames[nextIndex] = override;
  this->OverrideArray[nextIndex].Description = desc;
  this->OverrideArray[nextIndex].OverrideWithName = ocn;
  this->OverrideArray[nextIndex].EnabledFlag = enableFlag;
  this->OverrideArray[nextIndex].CreateCallback = createFunction;
}


vtkObject* vtkObjectFactory::CreateObject(const char* vtkclassname)
{
  for(int i=0; i < this->OverrideArrayLength; i++)
  {
    if(this->OverrideArray[i].EnabledFlag &&
       strcmp(this->OverrideClassNames[i], vtkclassname) == 0)
    {
      return (*this->OverrideArray[i].CreateCallback)();
    }
  }
  return 0;
}
  
// grow the array if the length is greater than the size.
void vtkObjectFactory::GrowOverrideArray()
{
  if(this->OverrideArrayLength+1 > this->SizeOverrideArray)
  {
    int newLength = this->OverrideArrayLength + 50;
    OverrideInformation* newArray = new OverrideInformation[newLength];
    char** newNameArray = new char*[newLength];
    for(int i =0; i < this->OverrideArrayLength; i++)
    {
      newNameArray[i] = this->OverrideClassNames[i];
      newArray[i] = this->OverrideArray[i];
    }
    delete [] this->OverrideClassNames;
    this->OverrideClassNames = newNameArray;
    delete [] this->OverrideArray;
    this->OverrideArray = newArray;
  }
}

int vtkObjectFactory::GetNumberOfOverrides()
{
  return this->OverrideArrayLength;
}

const char* vtkObjectFactory::GetClassOverrideName(int index)
{
  return this->OverrideClassNames[index];
}

const char* vtkObjectFactory::GetClassOverrideWithName(int index)
{
  return this->OverrideArray[index].OverrideWithName;
}
  
int vtkObjectFactory::GetEnableFlag(int index)
{
  return this->OverrideArray[index].EnabledFlag;
}


const char* vtkObjectFactory::GetDescription(int index)
{
  return this->OverrideArray[index].Description;
}



void vtkObjectFactory::SetEnableFlag(int flag,
				     const char* className,
				     const char* subclassName)
{
  for(int i =0; i < this->OverrideArrayLength; i++)
  {
    if(strcmp(this->OverrideClassNames[i], className) == 0)
    {
      if(strcmp(this->OverrideArray[i].OverrideWithName, subclassName) == 0)
      {
	this->OverrideArray[i].EnabledFlag = flag;
	return;
      }
    }
  }
}

int vtkObjectFactory::GetEnableFlag(const char* className,
				    const char* subclassName)
{
  for(int i =0; i < this->OverrideArrayLength; i++)
  {
    if(strcmp(this->OverrideClassNames[i], className) == 0)
    {
      if(strcmp(this->OverrideArray[i].OverrideWithName, subclassName) == 0)
      {
	return this->OverrideArray[i].EnabledFlag;
      }
    }
  }
  return 0;
}



void vtkObjectFactory::Disable(const char* className)
{
  for(int i =0; i < this->OverrideArrayLength; i++)
  {
    if(strcmp(this->OverrideClassNames[i], className) == 0)
    {
      this->OverrideArray[i].EnabledFlag = 0;
    }
  }
}


vtkObjectFactoryCollection* vtkObjectFactory::GetRegisteredFactories()
{
  return vtkObjectFactory::RegisteredFactories;
}
