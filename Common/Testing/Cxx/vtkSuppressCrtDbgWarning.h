#ifndef _vtkSuppressCrtDbgWarning_h
#define _vtkSuppressCrtDbgWarning_h

/* The crtdbg.h header on MSVC6 produces a warning with /W4.  This
   suppresses the warning.  */
void vtkSuppressCrtDbgWarning(int*, char**[])
{
# if _MSC_VER == 1200
  (void)(void (*)(void*,int,const char*,int))::operator delete;
# endif
}

#endif
