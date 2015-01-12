/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTestUtilities.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkTestUtilities - Utility functions used for regression testing.
// .SECTION Description
// vtkTestUtilities provides methods to perform common testing operations.
// These include getting a command line argument or an environment variable,
// or a default value. Particularly, there are specialized methods to get the
// root directory for VTK Data, expanding a filename with this root directory.

#ifndef vtkTestUtilities_h
#define vtkTestUtilities_h

#include "vtkSystemIncludes.h"

#if defined( _MSC_VER )      /* Visual C++ (and Intel C++) */
#pragma warning( disable : 4996 ) // 'function': was declared deprecated
#endif

struct vtkTestUtilities
{
  // Description:
  // Function necessary for accessing the root directory for VTK data. Try the
  // -D command line argument or VTK_DATA_ROOT or a default value. The returned
  // string has to be deleted (with delete[]) by the user.
  static inline char* GetDataRoot(int argc, char* argv[]);

  // Description:
  // Given a file name, this function returns a new string which is (in theory)
  // the full path. This path is constructed by prepending the file name with a
  // command line argument (-D path) or VTK_DATA_ROOT env. variable. If slash
  // is true, appends a slash to the resulting string. The returned string has
  // to be deleted (with delete[]) by the user.
  static inline char* ExpandDataFileName(int argc, char* argv[],
                                         const char* fname,
                                         int slash = 0);
  // Description:
  // Function returning either a command line argument, an environment variable
  // or a default value. The returned string has to be deleted (with delete[])
  // by the user.
  static inline char* GetArgOrEnvOrDefault(const char* arg,
                                           int argc, char* argv[],
                                           const char* env,
                                           const char* def);

  // Description:
  // Given a file name, this function returns a new string which is (in theory)
  // the full path. This path is constructed by prepending the file name with a
  // command line argument, an environment variable or a default value. If
  // slash is true, appends a slash to the resulting string. The returned
  // string has to be deleted (with delete[]) by the user.
  static inline char* ExpandFileNameWithArgOrEnvOrDefault(const char* arg,
                                                          int argc, char* argv[],
                                                          const char* env,
                                                          const char* def,
                                                          const char* fname,
                                                          int slash = 0);
};

inline
char* vtkTestUtilities::GetDataRoot(int argc, char* argv[])
{
  return vtkTestUtilities::GetArgOrEnvOrDefault(
    "-D", argc, argv,
    "VTK_DATA_ROOT",
    "../../../../VTKData");
}

inline
char* vtkTestUtilities::ExpandDataFileName(int argc, char* argv[],
                                           const char* fname,
                                           int slash)
{
  return vtkTestUtilities::ExpandFileNameWithArgOrEnvOrDefault(
    "-D", argc, argv,
    "VTK_DATA_ROOT",
    "../../../../VTKData",
    fname,
    slash);
}

inline
char* vtkTestUtilities::GetArgOrEnvOrDefault(const char* arg,
                                             int argc, char* argv[],
                                             const char* env,
                                             const char *def)
{
  int index = -1;

  for (int i = 0; i < argc; i++)
    {
    if (strcmp(arg, argv[i]) == 0 && i < argc - 1)
      {
      index = i + 1;
      }
    }

  char* value;

  if (index != -1)
    {
    value = new char[strlen(argv[index]) + 1];
    strcpy(value, argv[index]);
    }
  else
    {
    char *foundenv = getenv(env);
    if (foundenv)
      {
      value = new char[strlen(foundenv) + 1];
      strcpy(value, foundenv);
      }
    else if (def)
      {
      value = new char[strlen(def) + 1];
      strcpy(value, def);
      }
    else
      {
      value = NULL;
      }
    }

  return value;
}

inline
char* vtkTestUtilities::ExpandFileNameWithArgOrEnvOrDefault(const char* arg,
                                                            int argc,
                                                            char* argv[],
                                                            const char* env,
                                                            const char *def,
                                                            const char* fname,
                                                            int slash)
{
  char* fullName;

  char* value = vtkTestUtilities::GetArgOrEnvOrDefault(arg, argc, argv,
                                                       env,
                                                       def);
  if (value)
    {
    fullName = new char[strlen(value) + strlen(fname) + 2 +
                        static_cast<size_t>(slash ? 1 : 0)];
    fullName[0] = 0;
    strcat(fullName, value);
    size_t len = strlen(fullName);
    fullName[len] = '/';
    fullName[len+1] = 0;
    strcat(fullName, fname);
    }
  else
    {
    fullName = new char[strlen(fname) + 1 + static_cast<size_t>(slash ? 1 : 0)];
    strcpy(fullName, fname);
    }

  if (slash)
    {
    strcat(fullName, "/");
    }

  delete[] value;

  return fullName;
}

#endif // vtkTestUtilities_h
