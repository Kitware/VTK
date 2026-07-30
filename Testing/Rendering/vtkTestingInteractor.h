// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
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

#include "vtkDeprecation.h"             // For deprecation macros
#include "vtkObjectFactoryCollection.h" // Generated object overrides
#include "vtkRenderWindowInteractor.h"
#include "vtkSmartPointer.h"           // For vtkSmartPointer
#include "vtkTestingRenderingModule.h" // For export macro

#include <string> // STL Header; Required for string

VTK_ABI_NAMESPACE_BEGIN
class vtkMultiProcessController;

class VTKTESTINGRENDERING_EXPORT vtkTestingInteractor : public vtkRenderWindowInteractor
{
public:
  /**
   * Standard object factory instantiation method.
   */
  static vtkTestingInteractor* New();

  ///@{
  /**
   * Type and printing information.
   */
  vtkTypeMacro(vtkTestingInteractor, vtkRenderWindowInteractor);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  void Start() override;

  static void SetTestReturnStatus(int status);
  static int GetTestReturnStatus();

  static void SetErrorThreshold(double threshold);
  static double GetErrorThreshold();

  static void SetValidBaseline(std::string baseline);
  static std::string const& GetValidBaseline();

  static void SetTempDirectory(std::string dir);
  static std::string const& GetTempDirectory();

  static void SetDataDirectory(std::string dir);
  static std::string const& GetDataDirectory();

  VTK_DEPRECATED_IN_9_8_0("Use `SetTestReturnStatus`/`GetTestReturnStatus`")
  static int TestReturnStatus; // Return status of the test
  VTK_DEPRECATED_IN_9_8_0("Use `SetErrorThreshold`/`GetErrorThreshold`")
  static double ErrorThreshold; // Error Threshold
  VTK_DEPRECATED_IN_9_8_0("Use `SetValidBaseline`/`GetValidBaseline`")
  static std::string ValidBaseline; // Name of the Baseline image
  VTK_DEPRECATED_IN_9_8_0("Use `SetTempDirectory`/`GetTempDirectory`")
  static std::string TempDirectory; // Location of Testing/Temporary
  VTK_DEPRECATED_IN_9_8_0("Use `SetDataDirectory`/`GetDataDirectory`")
  static std::string DataDirectory; // Location of VTKData

  ///@{
  /**
   * Get/Set the controller in an MPI environment.
   */
  vtkMultiProcessController* GetController() const;
  void SetController(vtkMultiProcessController* controller);
  ///@}

protected:
  /**
   * The constructor sets up the `Controller` if MPI has been initialized.
   */
  vtkTestingInteractor();

  vtkSmartPointer<vtkMultiProcessController> Controller;

private:
  vtkTestingInteractor(const vtkTestingInteractor&) = delete;
  void operator=(const vtkTestingInteractor&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
