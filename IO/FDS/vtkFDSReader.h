// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkFDSReader_h
#define vtkFDSReader_h

#include "vtkDataAssembly.h" // For vtkDataAssembly
#include "vtkIOFDSModule.h"  // For export macro
#include "vtkNew.h"          // For vtkNew
#include "vtkPartitionedDataSetCollectionAlgorithm.h"
#include "vtkSmartPointer.h" // For vtkSmartPointer

#include <memory> // For std::unique_ptr
#include <set>    // For std::set
#include <string> // For std::string
#include <vector> // For std::vector

VTK_ABI_NAMESPACE_BEGIN

class vtkResourceStream;
/**
 * @class vtkFDSReader
 *
 * A reader for the Fire Dynamics Simulator (FDS) output data.
 *
 * This class reads in the `.smv` file and uses the meta-data to identify the other
 * files to read automatically. It outputs a `vtkPartitionedDataSetCollection`
 * containing 5 groups: Grids, Devices, HRR, Slices and Boundaries. Each group
 * contains data sets with the expected values for users of the FDS code.
 *
 * FDS & SMV specifications : https://pages.nist.gov/fds-smv/manuals.html
 */
class VTKIOFDS_EXPORT vtkFDSReader : public vtkPartitionedDataSetCollectionAlgorithm
{
public:
  static vtkFDSReader* New();
  vtkTypeMacro(vtkFDSReader, vtkPartitionedDataSetCollectionAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Specifies the name of the .smv file to be loaded.
   */
  vtkSetMacro(FileName, std::string);
  vtkGetMacro(FileName, std::string);
  ///@}

  ///@{
  /**
   * Set/Get the stream from which to read the .smv file.
   * If Stream is not nullptr, it will be used in priority from FileName
   */
  virtual void SetStream(vtkResourceStream* stream);
  virtual vtkResourceStream* GetStream();
  ///@}

  /**
   * Get the data full data assembly associated with the input
   */
  vtkGetNewMacro(Assembly, vtkDataAssembly);

  /**
   * Whenever the assembly is changed, this tag gets changed. Note, users should
   * not assume that this is monotonically increasing but instead simply rely on
   * its value to determine if the assembly may have changed since last time.
   *
   * It is set to 0 whenever there's no valid assembly available.
   */
  vtkGetMacro(AssemblyTag, int);

  ///@{
  /**
   * API to set selectors. Multiple selectors can be added using `AddSelector`.
   * The order in which selectors are specified is not preserved and has no
   * impact on the result.
   *
   * `AddSelector` returns true if the selector was added, false if the selector
   * was already specified and hence not added.
   *
   * @sa vtkDataAssembly::SelectNodes
   */
  bool AddSelector(const char* selector);
  void ClearSelectors();
  ///@}

  ///@{
  /**
   * Set the absolute tolerance under which two time values are considered identical
   */
  vtkGetMacro(TimeTolerance, double);
  vtkSetMacro(TimeTolerance, double);
  ///@}

protected:
  vtkFDSReader();
  ~vtkFDSReader() override;

  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkFDSReader(const vtkFDSReader&) = delete;
  void operator=(const vtkFDSReader&) = delete;

  bool ParseVIEWTIMES();
  bool ParseGRID(const std::vector<int>& baseNodes);
  bool ParseCSVF(const std::vector<int>& baseNodes);
  bool ParseDEVICE(const std::vector<int>& baseNodes);

  /**
   * Parse the slices section. Data can be cell-centered (SLCC) or not (SLCF)
   */
  bool ParseSLCFSLCC(const std::vector<int>& baseNodes, bool cellCentered);

  /**
   * Parse the boundary section. Data can be cell-centered (BNDC) or not (BNDF)
   */
  bool ParseBNDFBNDC(bool cellCentered);

  vtkSmartPointer<vtkResourceStream> Open();
  std::string SanitizeName(const std::string& name);

  int AssemblyTag = 0;
  std::string FileName;
  std::set<std::string> Selectors;

  vtkNew<vtkDataAssembly> Assembly;

  double TimeTolerance = 1e-5;

  struct vtkInternals;
  std::shared_ptr<vtkInternals> Internals;
};

VTK_ABI_NAMESPACE_END

#endif
