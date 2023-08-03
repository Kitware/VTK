// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkFDSReader_h
#define vtkFDSReader_h

#include "vtkDataAssembly.h" // For vtkDataAssembly
#include "vtkIOFDSModule.h"  // For export macro
#include "vtkNew.h"          // For vtkNew
#include "vtkPartitionedDataSetCollectionAlgorithm.h"
#include "vtkResourceStream.h" // For vtkResourceStream
#include "vtkSmartPointer.h"   // For vtkSmartPointer

#include <memory> // For std::unique_ptr
#include <set>    // For std::set
#include <string> // For std::string

VTK_ABI_NAMESPACE_BEGIN

class vtkFDSDeviceVisitor;

/**
 * @class vtkFDSReader
 *
 * TODO
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
  vtkSetSmartPointerMacro(Stream, vtkResourceStream);
  vtkGetSmartPointerMacro(Stream, vtkResourceStream);
  ///@}

  /**
   * TODO
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

protected:
  vtkFDSReader();
  ~vtkFDSReader() override;

  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkFDSReader(const vtkFDSReader&) = delete;
  void operator=(const vtkFDSReader&) = delete;

  vtkSmartPointer<vtkResourceStream> Open();

  int AssemblyTag = 0;
  std::string FileName;
  std::set<std::string> Selectors;

  vtkNew<vtkDataAssembly> Assembly;

  vtkSmartPointer<vtkResourceStream> Stream;

  struct vtkInternals;
  std::shared_ptr<vtkInternals> Internals;

  friend class vtkFDSDeviceVisitor;
  friend class vtkFDSGridVisitor;
};

VTK_ABI_NAMESPACE_END

#endif
