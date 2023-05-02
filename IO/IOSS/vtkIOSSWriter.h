/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkIOSSWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkIOSSWriter
 * @brief writer using IOSS
 *
 * vtkIOSSWriter is a writer to write datasets using IOSS library. Currently
 * this writer supports writing Exodus files. This writer is a work in progress
 * and currently only supports targeted use-cases. The writer will be
 * iteratively cleaned up and fixed to support all types of incoming datasets.
 */

#ifndef vtkIOSSWriter_h
#define vtkIOSSWriter_h

#include "vtkDeprecation.h"  // For VTK_DEPRECATED
#include "vtkIOIOSSModule.h" // For export macros
#include "vtkWriter.h"

#include <memory> // for std::unique_ptr

VTK_ABI_NAMESPACE_BEGIN
class vtkMultiProcessController;

class VTKIOIOSS_EXPORT vtkIOSSWriter : public vtkWriter
{
public:
  static vtkIOSSWriter* New();
  vtkTypeMacro(vtkIOSSWriter, vtkWriter);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Get/set the filename. When writing in a distributed environment, the
   * actual filename written out may be different.
   */
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);
  ///@}

  ///@{
  /**
   * Get/Set the active assembly to use. The chosen assembly is used
   * in combination with the selectors specified to determine which blocks
   * are to be extracted.
   *
   * The default is set to vtkDataAssemblyUtilities::HierarchyName().
   */
  vtkSetStringMacro(AssemblyName);
  vtkGetStringMacro(AssemblyName);
  ///@}

  /////////////////////////////////////////////////////////////////////////////////////////
  //                         Element Block Selectors API                                 //
  /////////////////////////////////////////////////////////////////////////////////////////
  ///@{
  /**
   * API to set element block selectors. Multiple selectors can be added using
   * `AddElementBlockSelector`. The order in which selectors are specified is not preserved
   * and has no impact on the result.
   *
   * `AddElementBlockSelector` returns true if the selector was added, false if the selector
   * was already specified and hence not added.
   *
   * @sa vtkDataAssembly::SelectNodes
   */
  bool AddElementBlockSelector(const char* selector);
  void ClearElementBlockSelectors();
  ///@}

  /**
   * Convenience method to set a single element block selector.
   * This clears any other existing selectors.
   */
  void SetElementBlockSelector(const char* selector);

  ///@{
  /**
   * API to access element block selectors.
   */
  int GetNumberOfElementBlockSelectors() const;
  const char* GetElementBlockSelector(int index) const;
  ///@}

  /////////////////////////////////////////////////////////////////////////////////////////
  //                         Node Set Selectors API                                      //
  /////////////////////////////////////////////////////////////////////////////////////////
  ///@{
  /**
   * API to set node set selectors. Multiple selectors can be added using
   * `AddNodeSetSelector`. The order in which selectors are specified is not preserved
   * and has no impact on the result.
   *
   * `AddNodeSetSelector` returns true if the selector was added, false if the selector
   * was already specified and hence not added.
   *
   * @sa vtkDataAssembly::SelectNodes
   */
  bool AddNodeSetSelector(const char* selector);
  void ClearNodeSetSelectors();
  ///@}

  /**
   * Convenience method to set a single node set selector.
   * This clears any other existing selectors.
   */
  void SetNodeSetSelector(const char* selector);

  ///@{
  /**
   * API to access node set selectors.
   */
  int GetNumberOfNodeSetSelectors() const;
  const char* GetNodeSetSelector(int index) const;
  ///@}

  /////////////////////////////////////////////////////////////////////////////////////////
  //                         Side Set Selectors API                                      //
  /////////////////////////////////////////////////////////////////////////////////////////
  ///@{
  /**
   * API to set side set selectors. Multiple selectors can be added using
   * `AddSideSetSelector`. The order in which selectors are specified is not preserved
   * and has no impact on the result.
   *
   * `AddSideSetSelector` returns true if the selector was added, false if the selector
   * was already specified and hence not added.
   *
   * @sa vtkDataAssembly::SelectNodes
   *
   * Default: /IOSS/side_sets
   */
  bool AddSideSetSelector(const char* selector);
  void ClearSideSetSelectors();
  ///@}

  /**
   * Convenience method to set a single side set selector.
   * This clears any other existing selectors.
   */
  void SetSideSetSelector(const char* selector);

  ///@{
  /**
   * API to access side set selectors.
   */
  int GetNumberOfSideSetSelectors() const;
  const char* GetSideSetSelector(int index) const;
  ///@}

  ///@{
  /**
   * Set/Get whether to write remove ghost cells from the input.
   *
   * The default is 1.
   */
  vtkSetMacro(RemoveGhosts, bool);
  vtkGetMacro(RemoveGhosts, bool);
  ///@}

  ///@{
  /**
   * Exodus wants global ids to start with 1, while VTK generally produces
   * global ids starting with 0. Set this to true (default false), if the global
   * ids are generated by VTK and hence start with 0. When writing to the output
   * file, they will be offset by 1 to ensure the ids are valid exodus ids.
   */
  vtkSetMacro(OffsetGlobalIds, bool);
  vtkGetMacro(OffsetGlobalIds, bool);
  vtkBooleanMacro(OffsetGlobalIds, bool);
  ///@}

  ///@{
  /**
   * If input is untransformed IOSS dataset, then the writer can preserve entity
   * group classifications, such as element blocks, side sets etc. The same is
   * not true if the input has been transformed e.g. through a clip filter. Thus
   * flag is used to indicate whether the input has valid element
   * classifications.
   */
  VTK_DEPRECATED_IN_9_3_0("PreserveInputEntityGroups is no longer needed.")
  void SetPreserveInputEntityGroups(bool) {}
  VTK_DEPRECATED_IN_9_3_0("PreserveInputEntityGroups is no longer needed.")
  bool GetPreserveInputEntityGroups() { return true; }
  VTK_DEPRECATED_IN_9_3_0("PreserveInputEntityGroups is no longer needed.")
  void PreserveInputEntityGroupsOn() {}
  VTK_DEPRECATED_IN_9_3_0("PreserveInputEntityGroups is no longer needed.")
  void PreserveInputEntityGroupsOff() {}
  ///@}

  ///@{
  /**
   * If input is transformed, e.g. through clipping, new element blocks may be created.
   * This flag can be used to indicate whether to preserve the original ids from blocks.
   *
   * The default is false.
   */
  vtkSetMacro(PreserveOriginalIds, bool);
  vtkGetMacro(PreserveOriginalIds, bool);
  vtkBooleanMacro(PreserveOriginalIds, bool);
  ///@}

  /////////////////////////////////////////////////////////////////////////////////////////
  //                                Global Fields API                                    //
  /////////////////////////////////////////////////////////////////////////////////////////
  ///@{
  /**
   * When set to true (default), the writer will write quality assurance and
   * information records.
   *
   * These records are not copied from the input, but they are generated by the
   * writer.
   */
  vtkSetMacro(WriteQAAndInformationRecords, bool);
  vtkGetMacro(WriteQAAndInformationRecords, bool);
  vtkBooleanMacro(WriteQAAndInformationRecords, bool);
  ///@}
  /////////////////////////////////////////////////////////////////////////////////////////

  ///@{
  /**
   * If input dataset has displacements pre-applied, setting the displacement
   * magnitude to non-zero ensures that the point coordinates in the dataset are
   * correctly transformed using the displacement field array, if present.
   *
   * Defaults to 1.0.
   */
  vtkSetClampMacro(DisplacementMagnitude, double, 0, VTK_DOUBLE_MAX);
  vtkGetMacro(DisplacementMagnitude, double);
  ///@}

  ///@{
  /**
   * A debugging variable, set this to non-zero positive number to save at most
   * the specified number of timesteps in a single file before starting a new
   * one. The writer may start new files (aka restarts) automatically if it
   * determines that the mesh has changed.
   *
   * Defaults to 0 i.e. unlimited timesteps per file.
   */
  VTK_DEPRECATED_IN_9_3_0("Use TimeStepRange/TimeStepStride instead.")
  void SetMaximumTimeStepsPerFile(int val)
  {
    this->SetTimeStepStride(1);
    this->SetTimeStepRange(0, val - 1);
  }
  VTK_DEPRECATED_IN_9_3_0("Use TimeStepRange/TimeStepStride instead.")
  int GetMaximumTimeStepsPerFile() { return this->TimeStepRange[1] + 1; }
  ///@}

  ///@{
  /**
   * `TimeStepRange` and `TimeStepStride` can be used to limit which timesteps will be written.
   *
   * If the range is invalid, i.e. `TimeStepRange[0] >= TimeStepRange[1]`, it's assumed
   * that no TimeStepRange overrides have been specified and both TimeStepRange and
   * TimeStepStride will be ignored. When valid, only the chosen subset of files
   * will be processed.
   */
  vtkSetVector2Macro(TimeStepRange, int);
  vtkGetVector2Macro(TimeStepRange, int);
  vtkSetClampMacro(TimeStepStride, int, 1, VTK_INT_MAX);
  vtkGetMacro(TimeStepStride, int);
  ///@}

  ///@{
  /**
   * Get/Set the controller to use when working in parallel. Initialized to
   * `vtkMultiProcessController::GetGlobalController` in the constructor.
   *
   * The controller is used to determine the upstream piece request in
   * RequestUpdateExtent.
   */
  void SetController(vtkMultiProcessController* controller);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);
  ///@}

protected:
  vtkIOSSWriter();
  ~vtkIOSSWriter() override;

  int FillInputPortInformation(int port, vtkInformation* info) override;
  vtkTypeBool ProcessRequest(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;
  int RequestInformation(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector);
  int RequestUpdateExtent(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector);
  int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  void WriteData() override;

private:
  vtkIOSSWriter(const vtkIOSSWriter&) = delete;
  void operator=(const vtkIOSSWriter&) = delete;

  class vtkInternals;
  std::unique_ptr<vtkInternals> Internals;

  vtkMultiProcessController* Controller;
  char* FileName;
  char* AssemblyName;
  bool RemoveGhosts;
  bool OffsetGlobalIds;
  bool PreserveOriginalIds;
  bool WriteQAAndInformationRecords;
  double DisplacementMagnitude;
  int TimeStepRange[2];
  int TimeStepStride;
};
VTK_ABI_NAMESPACE_END

#endif
