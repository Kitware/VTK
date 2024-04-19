// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkIOSSFilesScanner
 * @brief helper to scan files
 *
 * vtkIOSSReader supports specifying files in a variety of ways. This class
 * helps expand the chosen set of files to a complete set based on Ioss
 * conventions.
 */

#ifndef vtkIOSSFilesScanner_h
#define vtkIOSSFilesScanner_h

#include "vtkIOIOSSModule.h" // for export macros
#include "vtkObject.h"

#include <set>
#include <string>
#include <vector>

VTK_ABI_NAMESPACE_BEGIN
class VTKIOIOSS_EXPORT vtkIOSSFilesScanner : public vtkObject
{
public:
  static vtkIOSSFilesScanner* New();
  vtkTypeMacro(vtkIOSSFilesScanner, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Returns true if the file is a meta-file.
   */
  static bool IsMetaFile(VTK_FILEPATH const std::string& filename);

  /**
   * Parses the meta-file and returns a collection of files.
   */
  static std::set<std::string> GetFilesFromMetaFile(VTK_FILEPATH const std::string& filename);

  /**
   * Scans for related files.
   *
   * This searches for restarts, spatial partitions etc. using the Ioss/Exodus naming
   * conventions.
   *
   * `directoryListing`, if specified, is used instead of scanning the directories
   * containing the files in the `originalSet` (useful for testing).
   */
  static std::set<std::string> GetRelatedFiles(const std::set<std::string>& originalSet,
    const std::vector<std::string>& directoryListing = std::vector<std::string>());

  /**
   * Runs a bunch of tests for file pattern matching.
   */
  static bool DoTestFilePatternMatching();

protected:
  vtkIOSSFilesScanner();
  ~vtkIOSSFilesScanner() override;

private:
  vtkIOSSFilesScanner(const vtkIOSSFilesScanner&) = delete;
  void operator=(const vtkIOSSFilesScanner&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
// VTK-HeaderTest-Exclude: vtkIOSSFilesScanner.h
