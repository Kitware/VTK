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
/**
 * @class   vtkTesting
 * @brief   a unified VTK regression testing framework
 *
 *
 *  This is a VTK regression testing framework. Looks like this:
 *
 *  vtkTesting* t = vtkTesting::New();
 *
 *  Two options for setting arguments
 *
 *  Option 1:
 *  for ( cc = 1; cc < argc; cc ++ )
 *    {
 *    t->AddArgument(argv[cc]);
 *    }
 *
 *  Option 2:
 *  t->AddArgument("-D");
 *  t->AddArgument(my_data_dir);
 *  t->AddArgument("-V");
 *  t->AddArgument(my_valid_image);
 *
 *  ...
 *
 *  Two options of doing testing:
 *
 *  Option 1:
 *  t->SetRenderWindow(renWin);
 *  int res = t->RegressionTest(threshold);
 *
 *  Option 2:
 *  int res = t->RegressionTest(test_image, threshold);
 *
 *  ...
 *
 *  if (res == vtkTesting::PASSED)
 *    {
 *    Test passed
 *    }
 *  else
 *    {
 *    Test failed
 *    }
 *
*/

#ifndef vtkTesting_h
#define vtkTesting_h

#include "vtkTestingRenderingModule.h" // For export macro
#include "vtkObject.h"
#include <vector> // STL Header used for argv
#include <string> // STL Header used for argv

class vtkAlgorithm;
class vtkRenderWindow;
class vtkImageData;
class vtkDataArray;
class vtkDataSet;
class vtkRenderWindowInteractor;

class VTKTESTINGRENDERING_EXPORT vtkTesting : public vtkObject
{
public:
  static vtkTesting *New();
  vtkTypeMacro(vtkTesting,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  enum ReturnValue {
    FAILED = 0,
    PASSED = 1,
    NOT_RUN = 2,
    DO_INTERACTOR = 3
  };

  static int Test(int argc, char *argv[], vtkRenderWindow *rw, double thresh);

  /**
   * This method is intended to be a comprehensive, one line replacement for
   * vtkRegressionTest and for the replay based testing using
   * vtkInteractorEventRecorder, greatly simplifying API and code
   * bloat. It scans the command line specified for the following :
   * - If a "--DisableReplay" is specified, it disables the testing
   * replay. This is particularly useful in enabling the user to
   * exercise the widgets. Typically the widgets are defined by the
   * testing replay, so the user misses out on playing around with the
   * widget definition behaviour.
   * - If a "--Record" is specified, it records the interactions into
   * a "vtkInteractorEventRecorder.log" file. This is useful when
   * creating the playback stream that is plugged into tests. The
   * file can be used to create a const char * variable for playback
   * or can copied into a location as a playback file.
   * - If a "--PlaybackFile filename is specified,the provided file
   * contains the events and is passed to the event recorder.

   * Typical usage in a test for a VTK widget that needs playback
   * testing / recording is :

   * const char TestFooWidgetLog[] = {
   * ....
   * };

   * int TestFooWidget( int argc, char *argv[] )
   * {
   * ...
   * return vtkTesting::InteractorEventLoop(
   * argc, argv, iren,
   * TestFooWidgetLog );
   * }

   * In tests that playback events from a file:
   * TestFooEventLog.txt stored in  ../Data/Input/TestFooEventLog.txt
   * The CMakeLists.txt file should contain:

   * set(TestFoo_ARGS "--PlaybackFile"
   * "DATA{../Data/Input/TestFooEventLog.txt}")

   * and the API is
   * int TestFoo( int argc, char *argv[] )
   * {
   * ...
   * return vtkTesting::InteractorEventLoop( argc, argv, iren );
   * }

   * In tests where no playback is exercised, the API is simply

   * int TestFoo( int argc, char *argv[] )
   * {
   * ...
   * return vtkTesting::InteractorEventLoop( argc, argv, iren );
   * }
   */
  static int InteractorEventLoop(int argc, char *argv[],
                                 vtkRenderWindowInteractor *iren,
                                 const char *stream = NULL);

  //@{
  /**
   * Use the front buffer first for regression test comparisons. By
   * default use back buffer first, then try the front buffer if the
   * test fails when comparing to the back buffer.
   */
  vtkBooleanMacro(FrontBuffer, int);
  vtkGetMacro(FrontBuffer, int);
  void SetFrontBuffer(int frontBuffer);
  //@}

  /**
   * Perform the test and return the result. Delegates to
   * RegressionTestAndCaptureOutput, sending the output to cout.
   */
  virtual int RegressionTest(double thresh);

  /**
   * Perform the test and return the result. At the same time, write
   * the output to the output stream os. Includes timing information
   * in the output.
   */
  virtual int RegressionTestAndCaptureOutput(double thresh, ostream &os);

  /**
   * Perform the test and return the result. At the same time, write
   * the output to the output stream os. This method is nearly the
   * same as RegressionTestAndCaptureOutput, but does not include
   * timing information in the output.
   */
  virtual int RegressionTest(double thresh,ostream &os);

  //@{
  /**
   * Perform the test and return result. The test image will be read from the
   * png file at pngFileName.
   */
  virtual int RegressionTest(const std::string &pngFileName, double thresh);
  virtual int RegressionTest(const std::string &pngFileName,
                             double thresh, ostream& os);
  //@}

  //@{
  /**
   * Compare the image with the valid image.
   */
  virtual int RegressionTest(vtkAlgorithm* imageSource, double thresh);
  virtual int RegressionTest(vtkAlgorithm* imageSource, double thresh,
                             ostream& os);
  //@}

  /**
   * Compute the average L2 norm between all point data data arrays
   * of types float and double present in the data sets "dsA" and "dsB"
   * (this includes instances of vtkPoints) Compare the result of
   * each L2 comutation to "tol".
   */
  int CompareAverageOfL2Norm(vtkDataSet *pdA, vtkDataSet *pdB, double tol);

  /**
   * Compute the average L2 norm between two data arrays "daA" and "daB"
   * and compare against "tol".
   */
  int CompareAverageOfL2Norm(vtkDataArray *daA, vtkDataArray *daB, double tol);

  //@{
  /**
   * Set and get the render window that will be used for regression testing.
   */
  virtual void SetRenderWindow(vtkRenderWindow* rw);
  vtkGetObjectMacro(RenderWindow, vtkRenderWindow);
  //@}

  //@{
  /**
   * Set/Get the name of the valid image file
   */
  vtkSetStringMacro(ValidImageFileName);
  const char *GetValidImageFileName();
  //@}

  //@{
  /**
   * Get the image difference.
   */
  vtkGetMacro(ImageDifference, double);
  //@}

  //@{
  /**
   * Pass the command line arguments into this class to be processed. Many of
   * the Get methods such as GetValidImage and GetBaselineRoot rely on the
   * arguments to be passed in prior to retrieving these values. Just call
   * AddArgument for each argument that was passed into the command line
   */
  void AddArgument(const char *argv);
  void AddArguments(int argc,const char **argv);
  void AddArguments(int argc, char **argv);
  //@}

  /**
   * Search for a specific argument by name and return its value
   * (assumed to be the next on the command tail). Up to caller
   * to delete the returned string.
   */
  char *GetArgument(const char *arg);

  /**
   * This method delete all arguments in vtkTesting, this way you can reuse
   * it in a loop where you would have multiple testing.
   */
  void CleanArguments();

  //@{
  /**
   * Get some parameters from the command line arguments, env, or defaults
   */
  const char *GetDataRoot();
  vtkSetStringMacro(DataRoot);
  //@}

  //@{
  /**
   * Get some parameters from the command line arguments, env, or defaults
   */
  const char *GetTempDirectory();
  vtkSetStringMacro(TempDirectory);
  //@}

  /**
   * Is a valid image specified on the command line areguments?
   */
  int IsValidImageSpecified();

  /**
   * Is the interactive mode specified?
   */
  int IsInteractiveModeSpecified();

  /**
   * Is some arbitrary user flag ("-X", "-Z" etc) specified
   */
  int IsFlagSpecified(const char *flag);

  //@{
  /**
   * Number of pixels added as borders to avoid problems with
   * window decorations added by some window managers.
   */
  vtkSetMacro(BorderOffset, int);
  vtkGetMacro(BorderOffset, int);
  //@}

  //@{
  /**
   * Get/Set verbosity level. A level of 0 is quiet.
   */
  vtkSetMacro(Verbose, int);
  vtkGetMacro(Verbose, int);
  //@}

protected:
  vtkTesting();
  ~vtkTesting() VTK_OVERRIDE;

  static char* IncrementFileName(const char* fname, int count);
  static int LookForFile(const char* newFileName);

  int FrontBuffer;
  vtkRenderWindow* RenderWindow;
  char* ValidImageFileName;
  double ImageDifference;
  char *TempDirectory;
  int BorderOffset;
  int Verbose;

  std::vector<std::string> Args;

  char *DataRoot;
  double StartWallTime;
  double StartCPUTime;

private:
  vtkTesting(const vtkTesting&) VTK_DELETE_FUNCTION;
  void operator=(const vtkTesting&) VTK_DELETE_FUNCTION;
};

#endif


