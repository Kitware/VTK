/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTestingInteractor.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyrgight notice for more information.

=========================================================================*/
/**
 * @class   vtkTestingInteractor
 * @brief   A RenderWindowInteractor for testing
 *
 * Provides a Start() method that passes arguments to a test for
 * regression testing and returns. This permits programs that
 * run as tests to exit gracefully during the test run without needing
 * interaction.
 * @sa
 * vtkTestingObjectFactory
*/

#ifndef vtkTestingInteractor_h
#define vtkTestingInteractor_h

#include "vtkTestingRenderingModule.h" // For export macro
#include "vtkRenderWindowInteractor.h"
#include "vtkObjectFactoryCollection.h" // Generated object overrides

#include <string> // STL Header; Required for string

class VTKTESTINGRENDERING_EXPORT vtkTestingInteractor : public vtkRenderWindowInteractor
{
public:
  /**
   * Standard object factory instantiation method.
   */
  static vtkTestingInteractor* New();

  //@{
  /**
   * Type and printing information.
   */
  vtkTypeMacro(vtkTestingInteractor,vtkRenderWindowInteractor);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;
  //@}

  void Start() VTK_OVERRIDE;

  static int         TestReturnStatus;  // Return status of the test
  static double      ErrorThreshold;    // Error Threshold
  static std::string ValidBaseline;     // Name of the Baseline image
  static std::string TempDirectory;     // Location of Testing/Temporary
  static std::string DataDirectory;     // Location of VTKData

protected:
  vtkTestingInteractor() {}

private:
  vtkTestingInteractor(const vtkTestingInteractor&) VTK_DELETE_FUNCTION;
  void operator=(const vtkTestingInteractor&) VTK_DELETE_FUNCTION;

};

#endif
