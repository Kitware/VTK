As of December 15, 2004 VTK no longer includes windows.h by default in
most of its header files.  This change results in a 70% compile time
reduction on Windows compilers because the compiler does not have to
parse the 280K lines of source the preprocessor produces from
windows.h.  All VTK code that still includes windows.h does so through
vtkWindows.h which includes a minimal part of the real windows.h
header.  This also avoids contaminating user code with the windows.h
API just because it included a VTK header.

The main problem with windows.h is what we call the "windows mangling"
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
order to avoid the problems listed in case 2, we rename all affected
methods to have names that do not get mangled.  For example,
GetClassName becomes GetNameOfClass.  In order to maintain
compatibility with application code, the old names of the methods must
still be provided.  We now export all three possible names of the
original method from the library.  In the case of GetClassName, the
names "GetClassName", "GetClassNameA", and "GetClassNameW" are all
provided as methods in the class and exported from the library.  This
way when user code calls the method it does not matter which name it
gets because all three names are available.

Here is how all three names can be provided in a VTK header regardless
of whether windows.h has been included before it:

// See vtkWin32Header.h for definition of VTK_WORKAROUND_WINDOWS_MANGLE.
#ifdef VTK_WORKAROUND_WINDOWS_MANGLE
# define GetClassNameA GetClassName
# define GetClassNameW GetClassName
#endif
  const char* GetClassName() const;
#ifdef VTK_WORKAROUND_WINDOWS_MANGLE
# undef GetClassNameW
# undef GetClassNameA
  //BTX
  const char* GetClassNameA() const;
  const char* GetClassNameW() const;
  //ETX
#endif

The method GetClassName has three cases.  If windows.h is not included
then it is not mangled and the name is provided.  If windows.h is
included and UNICODE is not defined then GetClassName gets mangled to
GetClassNameA, but then gets replaced by GetClassName again.  The
preprocessor will not recursively expand a macro, so replacement stops
there and the GetClassName method is declared.  When UNICODE is
defined the same process occurs but through GetClassNameW instead.
The methods GetClassNameA and GetClassNameW are not mangled so they
can be provided directly.  They are surrounded by a BTX/ETX pair
because they should not be wrapped since scripting language code will
not be mangled by windows.h.

Now that all three names are provided we can address the fact that
GetClassName is supposed to be a virtual function.  When a subclass
wants to override a method with a mangled name it has to use this same
trick to override all three names.  Existing user code that tries to
override the function will only replace one of the three names, and it
will not implement the new unmangled name at all.  It is not possible
to get this code to work out-of-the-box, but we can at least produce a
compiler error to get the user's attention.  We change the signature
of the three possible mangled names to:

  virtual const char* const GetClassName() const;
  virtual const char* const GetClassNameA() const;
  virtual const char* const GetClassNameW() const;

and provide the new name with the original signature:

  virtual const char* GetNameOfClass() const;

When existing code tries to override the original method it will get a
compiler error that the return type does not match due to the extra
"const" qualifier.  This qualifier is otherwise meaningless and does
not affect calls to the method.  When the user encounters the error
and reads the source with the new signature, it will be clear that the
method is deprecated and that the replacement is called
GetNameOfClass.  The user code can then be modified to override the
new method name.

Users can detect places in their own code that may need modification
by using this cmake script:

  VTK/Utilities/Upgrading/FindWindowsMangledMethods.cmake

There are three backward-compatibility issues:

1.) User code that used the windows API without including windows.h
    that worked before because VTK included it will now break until
    the explicit inclusion is added.  This is considered acceptable
    because the code was technically wrong in the first place.  As a
    quick-fix, users can define VTK_INCLUDE_WINDOWS_H in their
    application and VTK will include windows.h as it did before.

2.) The virtual methods that have been renamed and replaced as
    described above must be renamed in user code.

3.) All functions that fall victim to mangling have been deprecated.
    User code will work (with warnings) but should be modified to call
    the new non-mangled versions of the methods.  VTK 5.0 includes
    support for the original names but it will be removed in a future
    version.  Use the VTK_LEGACY_REMOVE setting when building VTK to
    help make sure your application can build without using deprecated
    code.

Frequently Proposed Alternatives:

Several people have proposed alternatives that they think solve the
problem with less of a backward compatibility problem.  Here are some
of the common proposals and the reasons they were rejected:

1.) Use #undef to avoid the name mangling altogether.

    If VTK includes windows.h and then does the #undef then user code
    will not be able to access the windows API through the standard
    means.  If VTK does not include windows.h and uses #undef just in
    case the user included windows.h first then user code can still
    include windows.h after the VTK header and then their calls to VTK
    methods will be mangled and will not compile.

2.) Do not include windows.h but instead provide the same mangled
    names by defining the macros in VTK the same way windows.h does.

    The idea behind this solution is that the compile time improvement
    is achieved without breaking the previous mangling behavior.  This
    solution does not address the problems when users build VTK
    without UNICODE and then build their application with UNICODE.  It
    defines macros in VTK that are supposed to be defined in a system
    header.  This is always dangerous.  The solution used above does
    not every actually change or redefine any macros defined by
    windows.h.  It just temporarily defines extra macros.

3.) Use the above solution but change VTK_INCLUDE_WINDOWS_H to a
    VTK_DO_NOT_INCLUDE_WINDOWS_H so that the previous default behavior
    of including windows.h is preserved for user applications.  VTK
    can define VTK_DO_NOT_INCLUDE_WINDOWS_H when it is building
    itself.

    This helps existing applications but will also allow new
    applications to be written that do not include windows.h properly.
    It will also prevent the compile-time improvements from
    propagating to application code by default.  The policy we are
    trying to achieve is that including a VTK header should not do
    anything but define VTK... and vtk... symbols to avoid namespacing
    violations.  We are willing to let users break this policy by
    defining macros but we do not want to require users to define
    macros to get this policy.

NOTE: Since GetClassName is so widely used it was decided that it
would not be renamed or deprecated at this time.  The documentation
above uses GetClassName as an example but not all of these changes
were actually applied to it in VTK.  All changes were applied to other
offending methods, but only enough changes were applied to
GetClassName to get it to work whether or not windows.h is included or
UNICODE is defined.  The method is no longer virtual so user code must
define a GetClassNameInternal protected method instead of GetClassName
in order to override it.  Since most user code defines the method with
vtkTypeRevisionMacro anyway this should not require many changes.
