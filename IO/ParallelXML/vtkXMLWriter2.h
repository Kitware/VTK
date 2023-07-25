// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkXMLWriter2
 * @brief base class for new-style XML Writers
 *
 * vtkXMLWriter2 is designed to eventually replace vtkXMLWriter. This class and
 * its subclasses model a simpler design for the XML writers that does not rely
 * on complex class hierarchy which can cumbersome to develop and debug.
 *
 * vtkXMLWriter2 assumes parallel support. While MPI is not required,
 * vtkXMLWriter2 accesses vtkMultiProcessController and uses it to correctly
 * handle distributed execution. This avoids the need for having a parallel
 * variant of the writers as `vtkXMLWriter` hierarchy requires. This further
 * simplifies the development and debugging of these writers.
 *
 * A typical XML writer may generate multiple artifacts all of which should be
 * cleaned up if the write fails. vtkXMLWriter2 provides API that the subclasses
 * can use to register such artifacts. If `RequestData` returns failure, then
 * all such artifacts are cleaned up.
 */

#ifndef vtkXMLWriter2_h
#define vtkXMLWriter2_h

#include "vtkIOParallelXMLModule.h" // For export macro
#include "vtkXMLWriterBase.h"

#include <string>  // for std::string
#include <tuple>   // for std::tuple
#include <utility> // for std::pair
#include <vector>  // for std::vector

VTK_ABI_NAMESPACE_BEGIN
class vtkMultiProcessController;

class VTKIOPARALLELXML_EXPORT vtkXMLWriter2 : public vtkXMLWriterBase
{
public:
  vtkTypeMacro(vtkXMLWriter2, vtkXMLWriterBase);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Get/Set the controller to use. By default, initialized to
   * `vtkMultiProcessController::GetGlobalController`. The controller is used to
   * determine which piece to request from upstream.
   */
  void SetController(vtkMultiProcessController* controller);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);
  ///@}

  ///@{
  /**
   * Get/Set the number of ghost-levels to request from upstream pipeline.
   * Default is 0. Note, this does not mean the output file will have as many
   * ghost levels as requested. It's just a request. Whether the upstream
   * pipeline can satisfy the request totally depends on the upstream pipeline
   * itself.
   */
  vtkSetClampMacro(NumberOfGhostLevels, int, 0, VTK_INT_MAX);
  vtkGetMacro(NumberOfGhostLevels, int);
  ///@}

  /**
   * Overridden to handle requests.
   */
  vtkTypeBool ProcessRequest(
    vtkInformation* request, vtkInformationVector** inInfo, vtkInformationVector* outInfo) override;

protected:
  vtkXMLWriter2();
  ~vtkXMLWriter2() override;

  ///@{
  /**
   * Methods for various pipeline passes that can be overridden by subclasses to
   * do the work for the requested stage. Subclasses should always call the
   * superclass implementation to ensure the default set of tasks are executed
   * properly.
   */
  virtual int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*)
  {
    return 1;
  }
  virtual int RequestDataObject(vtkInformation*, vtkInformationVector**, vtkInformationVector*)
  {
    return 1;
  }
  virtual int RequestUpdateExtent(vtkInformation*, vtkInformationVector**, vtkInformationVector*);
  virtual int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) = 0;
  ///@}

  /**
   * Splits the file name into three parts:
   * 1. path: The directory containing the file.
   * 2. filename: The filename.
   * 3. artifacts-dir: Relative path to a directory to store secondary products for the file. This
   *    is typically path + filename-without-extension. If filename already has
   *    no extension, then a "_data" will be added.
   *
   * Both `filename` and `artifacts-dir` are relative to the `path`.
   */
  static std::tuple<std::string, std::string, std::string> SplitFileName(const std::string& fname);

  /**
   * This is used to store a list of generated artifacts as they are
   * written out. If `RequestData` returns 0 for whatever reason, all artifacts
   * logged using this API are removed thus ensuring that no partial results are
   * left behind.
   */
  void AddArtifact(const std::string& fname, bool isDir = false);

  /**
   * Same as AddArtifact except only executes on root node.
   */
  void AddRootArtifact(const std::string& fname, bool isDir = false);

  /**
   * Use this to create a directory. This function ensures that the directory is
   * created only in the root node to avoid all ranks attempting to create a
   * directory. The status is communicated to all ranks correctly to avoid
   * mismatched execution across ranks.
   */
  bool MakeDirectory(const std::string& dirname) const;

  /**
   * Helper method to do an exclusive scan using the summation operator.
   */
  static int ExclusiveScanSum(vtkMultiProcessController* controller, int value);

  /**
   * Utility function to gather a vector of strings on to the destination rank.
   * On all other ranks, empty vector is returned.
   */
  std::vector<std::string> Gather(vtkMultiProcessController* controller,
    const std::vector<std::string>& values, int destinationRank);

private:
  vtkXMLWriter2(const vtkXMLWriter2&) = delete;
  void operator=(const vtkXMLWriter2&) = delete;

  /**
   * Deletes all artifacts.
   */
  void DeleteArtifacts();

  vtkMultiProcessController* Controller;
  int NumberOfGhostLevels;
  std::vector<std::pair<std::string, bool>> Artifacts;
};

VTK_ABI_NAMESPACE_END
#endif
