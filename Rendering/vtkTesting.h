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
#include <vtkstd/vector> // used for argv
#include <vtkstd/string> // used for argv

class vtkRenderWindow;
class vtkImageData;

//BTX
struct VTK_RENDERING_EXPORT vtkTestUtilities
{
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
  // Perform the test and return result. At the same time the output will be
  // written cout and also placed into LastResultText
  virtual int RegressionTest(double thresh);

  // Description:
  // Compare the image with the valid image.
  virtual int RegressionTest(vtkImageData* image, double thresh);

  // Description:
  // Set and get the render window that will be used for regression testing.
  virtual void SetRenderWindow(vtkRenderWindow* rw);
  vtkGetObjectMacro(RenderWindow, vtkRenderWindow);

  // Description:
  // Set/Get the name of the valid image file
  vtkSetStringMacro(ValidImageFileName);
  const char *GetValidImageFileName();

  // Description:
  // Get the image difference.
  vtkGetMacro(ImageDifference, double);

  // Description:
  // Get the text output for the last RegressionTest invocation. This is
  // useful for scripting languages
  vtkGetStringMacro(LastResultText);
  vtkSetStringMacro(LastResultText);

  // Description:
  // Pass the command line arguments into this class to be processed. Many of
  // the Get methods such as GetValidImage and GetBaselineRoot rely on the
  // arguments to be passed in prior to retrieving these values. Just call
  // AddArgument for each argument that was passed into the command line
  void AddArgument(const char *argv);
  
  // Description:
  // Get some paramters from the command line arguments, env, or defaults
  const char *GetDataRoot();
  vtkSetStringMacro(DataRoot);

  // Description:
  // Get some paramters from the command line arguments, env, or defaults
  const char *GetTempDirectory();
  vtkSetStringMacro(TempDirectory);

  // Description:
  // Is a valid image specified on the command line areguments?
  int IsValidImageSpecified();

  // Description:
  // Is the interactive mode specified?
  int IsInteractiveModeSpecified();
  
protected:
  vtkTesting();
  ~vtkTesting();

  static char* IncrementFileName(const char* fname, int count);
  static int LookForFile(const char* newFileName);
  virtual int RegressionTest(double thresh,ostream &os);
  virtual int RegressionTest(vtkImageData* image, double thresh, ostream& os);

  int FrontBuffer;
  vtkRenderWindow* RenderWindow;
  char* ValidImageFileName;
  double ImageDifference;
  char *LastResultText;
  char *TempDirectory;
  
//BTX
  vtkstd::vector<vtkstd::string> Args;
//ETX
  char *DataRoot;
  double StartWallTime;
  double StartCPUTime;
  
private:
  vtkTesting(const vtkTesting&);  // Not implemented.
  void operator=(const vtkTesting&);  // Not implemented.
};

#endif


