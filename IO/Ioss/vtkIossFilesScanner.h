/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkIossFilesScanner.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkIossFilesScanner
 * @brief helper to scan files
 *
 * vtkIossReader supports specifying files in a variety of ways. This class
 * helps expand the chosen set of files to a complete set based on Ioss
 * conventions.
 */

#ifndef vtkIossFilesScanner_h
#define vtkIossFilesScanner_h

#include "vtkIOIossModule.h" // for export macros
#include "vtkObject.h"

#include <set>
#include <string>
#include <vector>

class VTKIOIOSS_EXPORT vtkIossFilesScanner : public vtkObject
{
public:
  static vtkIossFilesScanner* New();
  vtkTypeMacro(vtkIossFilesScanner, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Returns true if the file is a meta-file.
   */
  static bool IsMetaFile(const std::string& filename);

  /**
   * Parses the meta-file and returns a collection of files.
   */
  static std::set<std::string> GetFilesFromMetaFile(const std::string& filename);

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

protected:
  vtkIossFilesScanner();
  ~vtkIossFilesScanner() override;

private:
  vtkIossFilesScanner(const vtkIossFilesScanner&) = delete;
  void operator=(const vtkIossFilesScanner&) = delete;
};

#endif
// VTK-HeaderTest-Exclude: vtkIossFilesScanner.h
