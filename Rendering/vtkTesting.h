/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTesting.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkTesting - a unified VTK regression testing framework
// .SECTION Description
//

#ifndef __vtkTesting_h
#define __vtkTesting_h

#include "vtkObject.h"

class vtkLine;
class vtkTriangle;
class vtkRenderWindow;

//BTX
struct VTK_RENDERING_EXPORT vtkTestUtilities
{
  // Description:
  // Function necessary for accessing the root directory for VTK data.
  // Try the -D command line argument or VTK_DATA_ROOT or a default value.
  // The returned string has to be deleted (with delete[]) by the user.
  static char* GetDataRoot(int argc, char* argv[]);

  // Description:
  // Given a file name, this function returns a new string which
  // is (in theory) the full path. This path is constructed by
  // prepending the file name with a command line argument 
  // (-D path) or VTK_DATA_ROOT env. variable.
  // If slash is true, appends a slash to the resulting string.
  // The returned string has to be deleted (with delete[]) by the user.
  static char* ExpandDataFileName(int argc, char* argv[], 
                                         const char* fname,
                                         int slash = 0);
  // Description:
  // Function returning either a command line argument, an environment 
  // variable or a default value.
  // The returned string has to be deleted (with delete[]) by the user.
  static char* GetArgOrEnvOrDefault(const char* arg, 
                                           int argc, char* argv[], 
                                           const char* env, 
                                           const char* def);

  // Description:
  // Given a file name, this function returns a new string which
  // is (in theory) the full path. This path is constructed by
  // prepending the file name with a command line argument, an environment 
  // variable or a default value.
  // If slash is true, appends a slash to the resulting string.
  // The returned string has to be deleted (with delete[]) by the user.
  static char* ExpandFileNameWithArgOrEnvOrDefault(const char* arg, 
                                                          int argc, char* argv[], 
                                                          const char* env, 
                                                          const char* def, 
                                                          const char* fname,
                                                          int slash = 0);
};
//ETX

class VTK_RENDERING_EXPORT vtkTesting : public vtkObject
{
public:
  static vtkTesting *New();
  vtkTypeRevisionMacro(vtkTesting,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

//BTX
  enum ReturnValue {
    FAILED = 0,
    PASSED = 1,
    NOT_RUN = 2,
    DO_INTERACTOR = 3
  };

  static int Test(int argc, char *argv[], vtkRenderWindow *rw, double thresh);
//ETX
  
  // Description:
  // Use front buffer for tests. By default use back buffer.
  vtkSetClampMacro(FrontBuffer, int, 0, 1);
  vtkBooleanMacro(FrontBuffer, int);
  vtkGetMacro(FrontBuffer, int);

  // Description:
  // Perform the test and return result.
  virtual int RegressionTest(double thresh);

  // Description:
  // Set and get the render window that will be used for regression testing.
  virtual void SetRenderWindow(vtkRenderWindow* rw);
  vtkGetObjectMacro(RenderWindow, vtkRenderWindow);

  // Description:
  //
  vtkSetStringMacro(DataFileName);
  vtkGetStringMacro(DataFileName);

protected:
  vtkTesting();
  ~vtkTesting();

  static char* IncrementFileName(const char* fname, int count);
  static int LookForFile(const char* newFileName);

  int FrontBuffer;
  vtkRenderWindow* RenderWindow;
  char* DataFileName;

private:
  vtkTesting(const vtkTesting&);  // Not implemented.
  void operator=(const vtkTesting&);  // Not implemented.
};

#endif


