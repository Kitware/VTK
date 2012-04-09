#include  "FTLibrary.h"
#ifdef FTGL_DEBUG
  #include "mmgr.h"
#endif

#include <NoSTL/FTCallbackVector.h>

#ifdef FTGL_USE_NAMESPACE
namespace ftgl
{
#endif

FTLibrary *FTLibrary::Instance = 0;
FTLibraryCleanup FTLibrary::Cleanup;
FTCallbackVector *FTLibraryCleanup::Dependencies = 0;

// (static) Call and remove dependencies

void FTLibraryCleanup::CallAndRemoveDependencies()
{
  if (!FTLibraryCleanup::Dependencies)
    {
    return;
    }

  FTCallbackVector::iterator iter;
  for(iter = FTLibraryCleanup::Dependencies->begin(); 
      iter != FTLibraryCleanup::Dependencies->end(); 
      ++iter)
    {
    if(*iter)
      {
      (*iter)();
      }
    }

  delete FTLibraryCleanup::Dependencies;
  FTLibraryCleanup::Dependencies = 0;
}

// (static) Add dependency
// This gives a chance to dependent singleton (i.e. a font cache for ex.)
// to get cleaned up before the FTLibrary singleton is destroyed
// WARNING: this method auto-allocate the FTCallbackVector, which is
// freed by a call to CallAndRemoveDependencies in the destructor, so
// there better be at least one instance of that object, which is the
// case anyway otherwise this would make no sense.
// Why is Dependencies a static member ? Because a regular member would
// have to be initialized by the FTLibraryCleanup singleton, which can
// occurs after any other dependent singleton :(
// Why dynamic (allocated) ? Because (again) a static one would indeed
// be a singleton itself, that can be initialized *after* the other
// singletons, etc.

void FTLibraryCleanup::AddDependency(FTCallback callback)
{
  if (!FTLibraryCleanup::Dependencies)
    {
    FTLibraryCleanup::Dependencies = new FTCallbackVector;
    }
  FTLibraryCleanup::Dependencies->push_back(callback);
}

// Create the singleton cleanup

FTLibraryCleanup::FTLibraryCleanup()
{
}

// Delete the singleton cleanup
// Thiswill delete the FTLibrary singleton itself, and call all dependencies
// before, so that external dependent singletons can be deleted first.

FTLibraryCleanup::~FTLibraryCleanup()
{
  FTLibraryCleanup::CallAndRemoveDependencies();
  FTLibrary::SetInstance(0);
}

// (static) Return the single instance

FTLibrary* FTLibrary::GetInstance()
{
  if (!FTLibrary::Instance)
    {
    FTLibrary::Instance = new FTLibrary;
    }
  return FTLibrary::Instance;
}

// (static) Set the singleton instance

void FTLibrary::SetInstance(FTLibrary* instance)
{
  if (FTLibrary::Instance == instance)
    {
    return;
    }

  if (FTLibrary::Instance)
    {
    delete FTLibrary::Instance;
    }

  FTLibrary::Instance = instance;
}

FTLibrary::FTLibrary()
:  lib(0),
  err(0)
{
  Init();
}

FTLibrary::~FTLibrary()
{
  if( lib != 0)
  {
    FT_Done_FreeType( *lib);

    delete lib;
    lib= 0;
  }

//  if( manager != 0)
//  {
//    FTC_Manager_Done( manager );
//
//    delete manager;
//    manager= 0;
//  }
}

bool FTLibrary::Init()
{
  if( lib != 0 )
    return true;

  lib = new FT_Library;
  
  err = FT_Init_FreeType( lib);
  if( err)
  {
    delete lib;
    lib = 0;
    return false;
  }
  
//   FTC_Manager* manager;
//   
//   if( FTC_Manager_New( lib, 0, 0, 0, my_face_requester, 0, manager )
//   {
//     delete manager;
//     manager= 0;
//     return false;
//   }

  return true;
}

#ifdef FTGL_USE_NAMESPACE
} // namespace ftgl
#endif
