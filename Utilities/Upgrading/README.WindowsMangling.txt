As of December 15, 2004 VTK no longer includes windows.h by default in
most of its header files.  This change results in a 70% compile time
reduction on Windows compilers because the compiler does not have to
parse the 280K lines of source the preprocessor produces from
windows.h.  All VTK code that still includes windows.h does so through
vtkWindows.h which includes a minimal part of the real windows.h
header.  This also avoids contaminating user code with the windows.h
API just because it included a VTK header.

The main problem with windows.h what we call the "windows mangling"
problem.  In order to support UNICODE and ASCII characters APIs,
windows.h defines most of its API as macros.  This code appears in the
windows header:

WINUSERAPI int WINAPI GetClassNameA(HWND hWnd, LPSTR lpClassName,
                                    int nMaxCount);
WINUSERAPI int WINAPI GetClassNameW(HWND hWnd, LPWSTR lpClassName,
                                    int nMaxCount);
#ifdef UNICODE
#define GetClassName  GetClassNameW
#else
#define GetClassName  GetClassNameA
#endif // !UNICODE

The idea for windows.h is that user code can simply call GetClassName
and automatically support both UNICODE and ASCII character sets based
on a preprocessor switch.  The problem for everyone else is that the
windows API does not have any prefix on its functions, and the
preprocessor has no notion of namespaces or classes.  Therefore when
windows.h is included it invisibly renames methods in VTK like
GetClassName to another name.  Examples of other mangled names include
GetObject, SetProp, GetProp, RemoveProp, and CreateDirectory.  In fact
there are over 500 such names in the windows API.

Now consider what happens in each of these cases:

1.) VTK includes windows.h everywhere (as it did previously).  In this
    case the methods are always renamed for all VTK code and all user
    code that includes a VTK header.  The method GetClassNameA is
    actually visible as an export from the DLL.  Since the renaming is
    consistent then user code links correctly to VTK libraries.

    However, applications and other libraries do not necessarily
    include VTK or windows.h in all of their sources.  When this
    outside code uses methods that are mangled by windows.h then they
    get mangled in some translation units but not others.  This
    results in linking errors within the outside application for no
    other reason than that it included a VTK header!

    Also, when VTK is built without UNICODE defined then all the
    methods are renamed to have "A" at the end.  If the user
    application defines UNICODE and includes a VTK header then the
    methods get renamed to have "W" at the end, which results in
    unresolved symbols like "GetClassNameW".  Then the user complains
    and is told he/she has to rebuild VTK from scratch with UNICODE
    defined.  Basically VTK must be built separately for ASCII and
    UNICODE applications.

2.) VTK does not include windows.h everywhere.  In this case the
    methods are not renamed when VTK is built.  The method
    GetClassName is visible as an export from the DLL.  User code that
    includes windows.h before a VTK header will cause the method to be
    renamed in the user's translation units.  This will cause
    unresolved symbols with names like "GetClassNameA" or
    "GetClassNameW".

The solution to this problem is to start by NOT including windows.h
everywhere.  This avoids all the problems listed in case 1 above.  In
order to avoid the problems listed in case 2, we now export all three
possible names from the library.  In the case of GetClassName, the
names "GetClassName", "GetClassNameA", and "GetClassNameW" are all
provided as methods in the class and exported from the library.  This
way when user code calls the method it does not matter which name it
gets because all three names are available.

Here is how all three names can be provided in a VTK header regardless
of whether windows.h has been included before it:

#ifdef VTK_WORKAROUND_WINDOWS_MANGLE
  // Avoid windows name mangling.
# define GetClassNameA GetClassName
# define GetClassNameW GetClassName
#endif
  const char* GetClassName() const;
#ifdef VTK_WORKAROUND_WINDOWS_MANGLE
# undef GetClassNameW
# undef GetClassNameA
  //BTX
  // Define possible mangled names.
  const char* GetClassNameA() const;
  const char* GetClassNameW() const;
  //ETX
#endif

The methods GetClassNameA and GetClassNameW are not mangled so they
can be provided directly.  The method GetClassName has three cases.
If windows.h is not included then it is not mangled and the name is
provided.  If windows.h is included and UNICODE is not defined then
GetClassName gets mangled to GetClassNameA, but then gets replaced by
GetClassName again.  The preprocessor will not recursively expand a
macro, so replacement stops there and the GetClassName method is
declared.  When UNICODE is defined the same process occurs but through
GetClassNameW instead.

Now that all three names are provided we can address the fact that
GetClassName is supposed to be a virtual function.  When a subclass
wants to override a method with a mangled name it has to use this same
trick to override all three names.  The alternative is to rename the
original method to a name such as GetClassNameInternal that is NOT
mangled and make it protected.  The original name is preserved for
user code by providing the three non-virtual methods that just call
the virtual one internally.  Now subclasses can override just the
internal method.  This approach is used by vtkTypeMacro to make
GetClassName work.

Users can detect places in their own code that may need modification
by using this cmake script:

  VTK/Utilities/Upgrading/FindWindowsMangledMethods.cmake

There are two backward-compatibility issues:

1.) User code that used the windows API without including windows.h
    that worked before because VTK included it will now break until
    the explicit inclusion is added.  This is considered acceptable
    because the code was technically wrong in the first place.  As a
    quick-fix, users can define VTK_INCLUDE_WINDOWS_H in their
    application and VTK will include windows.h as it did before.

2.) Virtual functions that fall victim to mangling were renamed to
    names that are not mangled and made protected.  The original names
    of the functions were preserved for application code by creating
    non-virtual replacements with the original name and the mangled
    versions of the name.  For example RemoveProp is renamed to
    RemovePropInternal and replaced by RemoveProp, RemovePropA, and
    RemovePropW.  User code calling these methods will not be
    affected.  User code wishing to override such virtual methods will
    have to rename to the internal non-mangled version of the name.
    Such user methods will be exposed by the above mentioned CMake
    script.  The most common such method is GetClassName, which has
    been renamed to GetClassNameInternal.  Since user code is supposed
    to define this method by using vtkTypeMacro or
    vtkTypeRevisionMacro, this should not be a problem.
