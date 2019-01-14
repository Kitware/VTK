/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkObjectFactory.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkObjectFactory
 * @brief   abstract base class for vtkObjectFactories
 *
 * vtkObjectFactory is used to create vtk objects.   The base class
 * vtkObjectFactory contains a static method CreateInstance which is used
 * to create vtk objects from the list of registered vtkObjectFactory
 * sub-classes.   The first time CreateInstance is called, all dll's or shared
 * libraries in the environment variable VTK_AUTOLOAD_PATH are loaded into
 * the current process.   The C functions vtkLoad, vtkGetFactoryCompilerUsed,
 * and vtkGetFactoryVersion are called on each dll.  To implement these
 * functions in a shared library or dll, use the macro:
 * VTK_FACTORY_INTERFACE_IMPLEMENT.
 * VTK_AUTOLOAD_PATH is an environment variable
 * containing a colon separated (semi-colon on win32) list of paths.
 *
 * The vtkObjectFactory can be use to override the creation of any object
 * in VTK with a sub-class of that object.  The factories can be registered
 * either at run time with the VTK_AUTOLOAD_PATH, or at compile time
 * with the vtkObjectFactory::RegisterFactory method.
 *
*/

#ifndef vtkObjectFactory_h
#define vtkObjectFactory_h

#include "vtkDebugLeaksManager.h" // Must be included before singletons
#include "vtkCommonCoreModule.h" // For export macro
#include "vtkObject.h"

#include <string> // for std::string

class vtkObjectFactoryCollection;
class vtkOverrideInformationCollection;
class vtkCollection;

class VTKCOMMONCORE_EXPORT vtkObjectFactory : public vtkObject
{
public:
  // Class Methods used to interface with the registered factories

  /**
   * Create and return an instance of the named vtk object.
   * Each loaded vtkObjectFactory will be asked in the order
   * the factory was in the VTK_AUTOLOAD_PATH.  After the
   * first factory returns the object no other factories are asked.
   * isAbstract is no longer used. This method calls
   * vtkObjectBase::InitializeObjectBase() on the instance when the
   * return value is non-nullptr.
   */
  VTK_NEWINSTANCE
  static vtkObject* CreateInstance(const char* vtkclassname,
                                   bool isAbstract = false);

  /**
   * Create all possible instances of the named vtk object.
   * Each registered vtkObjectFactory will be asked, and the
   * result will be stored in the user allocated vtkCollection
   * passed in to the function.
   */
  static void CreateAllInstance(const char* vtkclassname,
                                vtkCollection* retList);
  /**
   * Re-check the VTK_AUTOLOAD_PATH for new factory libraries.
   * This calls UnRegisterAll before re-loading
   */
  static void ReHash();
  /**
   * Register a factory so it can be used to create vtk objects
   */
  static void RegisterFactory(vtkObjectFactory* );
  /**
   * Remove a factory from the list of registered factories
   */
  static void UnRegisterFactory(vtkObjectFactory*);
  /**
   * Unregister all factories
   */
  static void UnRegisterAllFactories();

  /**
   * Return the list of all registered factories.  This is NOT a copy,
   * do not remove items from this list!
   */
  static vtkObjectFactoryCollection* GetRegisteredFactories();

  /**
   * return 1 if one of the registered factories
   * overrides the given class name
   */
  static int HasOverrideAny(const char* className);

  /**
   * Fill the given collection with all the overrides for
   * the class with the given name.
   */
  static void GetOverrideInformation(const char* name,
                                     vtkOverrideInformationCollection*);

  /**
   * Set the enable flag for a given named class for all registered
   * factories.
   */
  static void SetAllEnableFlags(vtkTypeBool flag,
                                const char* className);
  /**
   * Set the enable flag for a given named class subclass pair
   * for all registered factories.
   */
  static void SetAllEnableFlags(vtkTypeBool flag,
                                const char* className,
                                const char* subclassName);

  // Instance methods to be used on individual instances of vtkObjectFactory

  // Methods from vtkObject
  vtkTypeMacro(vtkObjectFactory,vtkObject);
  /**
   * Print ObjectFactory to stream.
   */
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * All sub-classes of vtkObjectFactory should must return the version of
   * VTK they were built with.  This should be implemented with the macro
   * VTK_SOURCE_VERSION and NOT a call to vtkVersion::GetVTKSourceVersion.
   * As the version needs to be compiled into the file as a string constant.
   * This is critical to determine possible incompatible dynamic factory loads.
   */
  virtual const char* GetVTKSourceVersion() = 0;

  /**
   * Return a descriptive string describing the factory.
   */
  virtual const char* GetDescription() = 0;

  /**
   * Return number of overrides this factory can create.
   */
  virtual int GetNumberOfOverrides();

  /**
   * Return the name of a class override at the given index.
   */
  virtual const char* GetClassOverrideName(int index);

  /**
   * Return the name of the class that will override the class
   * at the given index
   */
  virtual const char* GetClassOverrideWithName(int index);

  /**
   * Return the enable flag for the class at the given index.
   */
  virtual vtkTypeBool GetEnableFlag(int index);

  /**
   * Return the description for a the class override at the given
   * index.
   */
  virtual const char* GetOverrideDescription(int index);

  //@{
  /**
   * Set and Get the Enable flag for the specific override of className.
   * if subclassName is null, then it is ignored.
   */
  virtual void SetEnableFlag(vtkTypeBool flag,
                             const char* className,
                             const char* subclassName);
  virtual vtkTypeBool GetEnableFlag(const char* className,
                                const char* subclassName);
  //@}

  /**
   * Return 1 if this factory overrides the given class name, 0 otherwise.
   */
  virtual int HasOverride(const char* className);
  /**
   * Return 1 if this factory overrides the given class name, 0 otherwise.
   */
  virtual int HasOverride(const char* className, const char* subclassName);

  /**
   * Set all enable flags for the given class to 0.  This will
   * mean that the factory will stop producing class with the given
   * name.
   */
  virtual void Disable(const char* className);

  //@{
  /**
   * This returns the path to a dynamically loaded factory.
   */
  vtkGetStringMacro(LibraryPath);
  //@}

  typedef vtkObject* (*CreateFunction)();

protected:

  /**
   * Register object creation information with the factory.
   */
  void RegisterOverride(const char* classOverride,
                        const char* overrideClassName,
                        const char* description,
                        int enableFlag,
                        CreateFunction createFunction);

  /**
   * This method is provided by sub-classes of vtkObjectFactory.
   * It should create the named vtk object or return 0 if that object
   * is not supported by the factory implementation.
   */
  virtual vtkObject* CreateObject(const char* vtkclassname );

  vtkObjectFactory();
  ~vtkObjectFactory() override;

  struct OverrideInformation
  {
    char* Description;
    char* OverrideWithName;
    vtkTypeBool EnabledFlag;
    CreateFunction CreateCallback;
  };

  OverrideInformation* OverrideArray;
  char** OverrideClassNames;
  int SizeOverrideArray;
  int OverrideArrayLength;

private:
  void GrowOverrideArray();

  /**
   * Initialize the static members of vtkObjectFactory.   RegisterDefaults
   * is called here.
   */
  static void Init();
  /**
   * Register default factories which are not loaded at run time.
   */
  static void RegisterDefaults();
  /**
   * Load dynamic factories from the VTK_AUTOLOAD_PATH
   */
  static void LoadDynamicFactories();
  /**
   * Load all dynamic libraries in the given path
   */
  static void LoadLibrariesInPath(const std::string&);

  // list of registered factories
  static vtkObjectFactoryCollection* RegisteredFactories;

  // member variables for a factory set by the base class
  // at load or register time
  void* LibraryHandle;
  char* LibraryVTKVersion;
  char* LibraryCompilerUsed;
  char* LibraryPath;
private:
  vtkObjectFactory(const vtkObjectFactory&) = delete;
  void operator=(const vtkObjectFactory&) = delete;
};

// Implementation detail for Schwartz counter idiom.
class VTKCOMMONCORE_EXPORT vtkObjectFactoryRegistryCleanup
{
public:
  vtkObjectFactoryRegistryCleanup();
  ~vtkObjectFactoryRegistryCleanup();

private:
  vtkObjectFactoryRegistryCleanup(const vtkObjectFactoryRegistryCleanup& other) = delete;
  vtkObjectFactoryRegistryCleanup& operator=(const vtkObjectFactoryRegistryCleanup& rhs) = delete;
};
static vtkObjectFactoryRegistryCleanup vtkObjectFactoryRegistryCleanupInstance;


// Macro to create an object creation function.
// The name of the function will by vtkObjectFactoryCreateclassname
// where classname is the name of the class being created
#define VTK_CREATE_CREATE_FUNCTION(classname) \
static vtkObject* vtkObjectFactoryCreate##classname() \
{ return classname::New(); }

#endif

#define VTK_FACTORY_INTERFACE_EXPORT VTKCOMMONCORE_EXPORT

// Macro to create the interface "C" functions used in
// a dll or shared library that contains a VTK object factory.
// Put this function in the .cxx file of your object factory,
// and pass in the name of the factory sub-class that you want
// the dll to create.
#define VTK_FACTORY_INTERFACE_IMPLEMENT(factoryName)  \
extern "C"                                      \
VTK_FACTORY_INTERFACE_EXPORT                    \
const char* vtkGetFactoryCompilerUsed()         \
{                                               \
  return VTK_CXX_COMPILER;                      \
}                                               \
extern "C"                                      \
VTK_FACTORY_INTERFACE_EXPORT                    \
const char* vtkGetFactoryVersion()              \
{                                               \
  return VTK_SOURCE_VERSION;                    \
}                                               \
extern "C"                                      \
VTK_FACTORY_INTERFACE_EXPORT                    \
vtkObjectFactory* vtkLoad()                     \
{                                               \
  return factoryName ::New();                   \
}

// Macro to implement the body of the object factory form of the New() method.
#define VTK_OBJECT_FACTORY_NEW_BODY(thisClass) \
  vtkObject* ret = vtkObjectFactory::CreateInstance(#thisClass, false); \
  if(ret) \
  { \
    return static_cast<thisClass*>(ret); \
  } \
  thisClass *result = new thisClass; \
  result->InitializeObjectBase(); \
  return result;

// Macro to implement the body of the abstract object factory form of the New()
// method, i.e. an abstract base class that can only be instantiated if the
// object factory overrides it.
#define VTK_ABSTRACT_OBJECT_FACTORY_NEW_BODY(thisClass) \
  vtkObject* ret = vtkObjectFactory::CreateInstance(#thisClass, true); \
  if(ret) \
  { \
    return static_cast<thisClass*>(ret); \
  } \
  vtkGenericWarningMacro("Error: no override found for '" #thisClass "'."); \
  return nullptr;

// Macro to implement the body of the standard form of the New() method.
#if defined(VTK_ALL_NEW_OBJECT_FACTORY)
# define VTK_STANDARD_NEW_BODY(thisClass) \
  VTK_OBJECT_FACTORY_NEW_BODY(thisClass)
#else
# define VTK_STANDARD_NEW_BODY(thisClass) \
  thisClass *result = new thisClass; \
  result->InitializeObjectBase(); \
  return result;
#endif

// Macro to implement the standard form of the New() method.
#define vtkStandardNewMacro(thisClass) \
  thisClass* thisClass::New() \
  { \
  VTK_STANDARD_NEW_BODY(thisClass) \
  }

// Macro to implement the object factory form of the New() method.
#define vtkObjectFactoryNewMacro(thisClass) \
  thisClass* thisClass::New() \
  { \
  VTK_OBJECT_FACTORY_NEW_BODY(thisClass) \
  }

// Macro to implement the abstract object factory form of the New() method.
// That is an abstract base class that can only be instantiated if the
// object factory overrides it.
#define vtkAbstractObjectFactoryNewMacro(thisClass) \
  thisClass* thisClass::New() \
  { \
  VTK_ABSTRACT_OBJECT_FACTORY_NEW_BODY(thisClass) \
  }
