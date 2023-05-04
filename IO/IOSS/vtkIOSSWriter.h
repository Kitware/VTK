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
#include "vtkIOSSReader.h"   // For vtkIOSSReader::EntityType
#include "vtkNew.h"          // For vtkNew
#include "vtkWriter.h"

#include <memory> // for std::unique_ptr
#include <set>    // for std::set
#include <string> // for std::string

VTK_ABI_NAMESPACE_BEGIN
class vtkDataArraySelection;
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

  using EntityType = vtkIOSSReader::EntityType;

  ///@{
  /**
   * Choose which fields to write. If this is true, then only the
   * arrays selected will be written. If this is false, then all arrays will be
   * written.
   *
   * The default is false.
   */
  vtkSetMacro(ChooseFieldsToWrite, bool);
  vtkGetMacro(ChooseFieldsToWrite, bool);
  vtkBooleanMacro(ChooseFieldsToWrite, bool);
  ///@}

  /////////////////////////////////////////////////////////////////////////////////////////
  //                                  Generic Entity API                                 //
  /////////////////////////////////////////////////////////////////////////////////////////
  ///@{
  /**
   * API to set entity selectors. Multiple selectors can be added using
   * `AddSelector`. The order in which selectors are specified is not preserved
   * and has no impact on the result.
   *
   * `AddSelector` returns true if the selector was added, false if the selector
   * was already specified and hence not added.
   *
   * @sa vtkDataAssembly::SelectNodes
   */
  bool AddSelector(EntityType entity, const char* selector);
  void ClearSelectors(EntityType entity);
  ///@}

  /**
   * Convenience method to set a single entity selector.
   * This clears any other existing selectors.
   */
  void SetSelector(EntityType entity, const char* selector);

  ///@{
  /**
   * API to access entity selectors.
   */
  int GetNumberOfSelectors(EntityType entity) const;
  const char* GetSelector(EntityType entity, int index) const;
  std::set<std::string> GetSelectors(EntityType entity) const;
  ///@}

  /**
   * Get the selection object for the given entity type. This can be used to
   * select which fields to write.
   */
  vtkDataArraySelection* GetFieldSelection(EntityType type);
  /////////////////////////////////////////////////////////////////////////////////////////

  /////////////////////////////////////////////////////////////////////////////////////////
  //                                   Node Block API                                    //
  /////////////////////////////////////////////////////////////////////////////////////////
  /**
   * Returns the field selection object for the element block arrays.
   */
  vtkDataArraySelection* GetNodeBlockFieldSelection()
  {
    return this->GetFieldSelection(EntityType::NODEBLOCK);
  }
  /////////////////////////////////////////////////////////////////////////////////////////

  /////////////////////////////////////////////////////////////////////////////////////////
  //                                  Edge Block API                                     //
  /////////////////////////////////////////////////////////////////////////////////////////
  ///@{
  /**
   * Add/Clear/Set/Get edge block selectors
   */
  bool AddEdgeBlockSelector(const char* selector)
  {
    return this->AddSelector(EntityType::EDGEBLOCK, selector);
  }
  void ClearEdgeBlockSelectors() { this->ClearSelectors(EntityType::EDGEBLOCK); }
  void SetEdgeBlockSelector(const char* selector)
  {
    this->SetSelector(EntityType::EDGEBLOCK, selector);
  }
  int GetNumberOfEdgeBlockSelectors() const
  {
    return this->GetNumberOfSelectors(EntityType::EDGEBLOCK);
  }
  const char* GetEdgeBlockSelector(int index) const
  {
    return this->GetSelector(EntityType::EDGEBLOCK, index);
  }
  std::set<std::string> GetEdgeBlockSelectors() const
  {
    return this->GetSelectors(EntityType::EDGEBLOCK);
  }
  ///@}

  /**
   * Returns the field selection object for the edge block arrays.
   */
  vtkDataArraySelection* GetEdgeBlockFieldSelection()
  {
    return this->GetFieldSelection(EntityType::EDGEBLOCK);
  }
  /////////////////////////////////////////////////////////////////////////////////////////

  /////////////////////////////////////////////////////////////////////////////////////////
  //                                  Face Block API                                     //
  /////////////////////////////////////////////////////////////////////////////////////////
  ///@{
  /**
   * Add/Clear/Set/Get face block selectors
   */
  bool AddFaceBlockSelector(const char* selector)
  {
    return this->AddSelector(EntityType::FACEBLOCK, selector);
  }
  void ClearFaceBlockSelectors() { this->ClearSelectors(EntityType::FACEBLOCK); }
  void SetFaceBlockSelector(const char* selector)
  {
    this->SetSelector(EntityType::FACEBLOCK, selector);
  }
  int GetNumberOfFaceBlockSelectors() const
  {
    return this->GetNumberOfSelectors(EntityType::FACEBLOCK);
  }
  const char* GetFaceBlockSelector(int index) const
  {
    return this->GetSelector(EntityType::FACEBLOCK, index);
  }
  std::set<std::string> GetFaceBlockSelectors() const
  {
    return this->GetSelectors(EntityType::FACEBLOCK);
  }
  ///@}

  /**
   * Returns the field selection object for the face block arrays.
   */
  vtkDataArraySelection* GetFaceBlockFieldSelection()
  {
    return this->GetFieldSelection(EntityType::FACEBLOCK);
  }
  /////////////////////////////////////////////////////////////////////////////////////////

  /////////////////////////////////////////////////////////////////////////////////////////
  //                                 Element Block API                                   //
  /////////////////////////////////////////////////////////////////////////////////////////
  ///@{
  /**
   * Add/Clear/Set/Get element block selectors
   */
  bool AddElementBlockSelector(const char* selector)
  {
    return this->AddSelector(EntityType::ELEMENTBLOCK, selector);
  }
  void ClearElementBlockSelectors() { this->ClearSelectors(EntityType::ELEMENTBLOCK); }
  void SetElementBlockSelector(const char* selector)
  {
    this->SetSelector(EntityType::ELEMENTBLOCK, selector);
  }
  int GetNumberOfElementBlockSelectors() const
  {
    return this->GetNumberOfSelectors(EntityType::ELEMENTBLOCK);
  }
  const char* GetElementBlockSelector(int index) const
  {
    return this->GetSelector(EntityType::ELEMENTBLOCK, index);
  }
  std::set<std::string> GetElementBlockSelectors() const
  {
    return this->GetSelectors(EntityType::ELEMENTBLOCK);
  }
  ///@}

  /**
   * Returns the field selection object for the element block arrays.
   */
  vtkDataArraySelection* GetElementBlockFieldSelection()
  {
    return this->GetFieldSelection(EntityType::ELEMENTBLOCK);
  }
  /////////////////////////////////////////////////////////////////////////////////////////

  /////////////////////////////////////////////////////////////////////////////////////////
  //                                 Node Set API                                        //
  /////////////////////////////////////////////////////////////////////////////////////////
  ///@{
  /**
   * Add/Clear/Set/Get node set selectors
   */
  bool AddNodeSetSelector(const char* selector)
  {
    return this->AddSelector(EntityType::NODESET, selector);
  }
  void ClearNodeSetSelectors() { this->ClearSelectors(EntityType::NODESET); }
  void SetNodeSetSelector(const char* selector)
  {
    this->SetSelector(EntityType::NODESET, selector);
  }
  int GetNumberOfNodeSetSelectors() const
  {
    return this->GetNumberOfSelectors(EntityType::NODESET);
  }
  const char* GetNodeSetSelector(int index) const
  {
    return this->GetSelector(EntityType::NODESET, index);
  }
  std::set<std::string> GetNodeSetSelectors() const
  {
    return this->GetSelectors(EntityType::NODESET);
  }
  ///@}

  /**
   * Returns the field selection object for the node set arrays.
   */
  vtkDataArraySelection* GetNodeSetFieldSelection()
  {
    return this->GetFieldSelection(EntityType::NODESET);
  }
  /////////////////////////////////////////////////////////////////////////////////////////

  /////////////////////////////////////////////////////////////////////////////////////////
  //                                 Edge Set API                                        //
  /////////////////////////////////////////////////////////////////////////////////////////
  ///@{
  /**
   * Add/Clear/Set/Get edge set selectors
   */
  bool AddEdgeSetSelector(const char* selector)
  {
    return this->AddSelector(EntityType::SIDESET, selector);
  }
  void ClearEdgeSetSelectors() { this->ClearSelectors(EntityType::SIDESET); }
  void SetEdgeSetSelector(const char* selector)
  {
    this->SetSelector(EntityType::SIDESET, selector);
  }
  int GetNumberOfEdgeSetSelectors() const
  {
    return this->GetNumberOfSelectors(EntityType::SIDESET);
  }
  const char* GetEdgeSetSelector(int index) const
  {
    return this->GetSelector(EntityType::SIDESET, index);
  }
  std::set<std::string> GetEdgeSetSelectors() const
  {
    return this->GetSelectors(EntityType::SIDESET);
  }
  ///@}

  /**
   * Returns the field selection object for the edge set arrays.
   */
  vtkDataArraySelection* GetEdgeSetFieldSelection()
  {
    return this->GetFieldSelection(EntityType::SIDESET);
  }
  /////////////////////////////////////////////////////////////////////////////////////////

  /////////////////////////////////////////////////////////////////////////////////////////
  //                                 Face Set API                                        //
  /////////////////////////////////////////////////////////////////////////////////////////
  ///@{
  /**
   * Add/Clear/Set/Get edge set selectors
   */
  bool AddFaceSetSelector(const char* selector)
  {
    return this->AddSelector(EntityType::SIDESET, selector);
  }
  void ClearFaceSetSelectors() { this->ClearSelectors(EntityType::SIDESET); }
  void SetFaceSetSelector(const char* selector)
  {
    this->SetSelector(EntityType::SIDESET, selector);
  }
  int GetNumberOfFaceSetSelectors() const
  {
    return this->GetNumberOfSelectors(EntityType::SIDESET);
  }
  const char* GetFaceSetSelector(int index) const
  {
    return this->GetSelector(EntityType::SIDESET, index);
  }
  std::set<std::string> GetFaceSetSelectors() const
  {
    return this->GetSelectors(EntityType::SIDESET);
  }
  ///@}

  /**
   * Returns the field selection object for the edge set arrays.
   */
  vtkDataArraySelection* GetFaceSetFieldSelection()
  {
    return this->GetFieldSelection(EntityType::SIDESET);
  }
  /////////////////////////////////////////////////////////////////////////////////////////

  /////////////////////////////////////////////////////////////////////////////////////////
  //                               Element Set API                                       //
  /////////////////////////////////////////////////////////////////////////////////////////
  ///@{
  /**
   * Add/Clear/Set/Get element set selectors
   */
  bool AddElementSetSelector(const char* selector)
  {
    return this->AddSelector(EntityType::SIDESET, selector);
  }
  void ClearElementSetSelectors() { this->ClearSelectors(EntityType::SIDESET); }
  void SetElementSetSelector(const char* selector)
  {
    this->SetSelector(EntityType::SIDESET, selector);
  }
  int GetNumberOfElementSetSelectors() const
  {
    return this->GetNumberOfSelectors(EntityType::SIDESET);
  }
  const char* GetElementSetSelector(int index) const
  {
    return this->GetSelector(EntityType::SIDESET, index);
  }
  std::set<std::string> GetElementSetSelectors() const
  {
    return this->GetSelectors(EntityType::SIDESET);
  }
  ///@}

  /**
   * Returns the field selection object for the element set arrays.
   */
  vtkDataArraySelection* GetElementSetFieldSelection()
  {
    return this->GetFieldSelection(EntityType::SIDESET);
  }
  /////////////////////////////////////////////////////////////////////////////////////////

  /////////////////////////////////////////////////////////////////////////////////////////
  //                                 Side Set API                                        //
  /////////////////////////////////////////////////////////////////////////////////////////
  ///@{
  /**
   * Add/Clear/Set/Get side set selectors
   */
  bool AddSideSetSelector(const char* selector)
  {
    return this->AddSelector(EntityType::SIDESET, selector);
  }
  void ClearSideSetSelectors() { this->ClearSelectors(EntityType::SIDESET); }
  void SetSideSetSelector(const char* selector)
  {
    this->SetSelector(EntityType::SIDESET, selector);
  }
  int GetNumberOfSideSetSelectors() const
  {
    return this->GetNumberOfSelectors(EntityType::SIDESET);
  }
  const char* GetSideSetSelector(int index) const
  {
    return this->GetSelector(EntityType::SIDESET, index);
  }
  std::set<std::string> GetSideSetSelectors() const
  {
    return this->GetSelectors(EntityType::SIDESET);
  }
  ///@}

  /**
   * Returns the field selection object for the side set arrays.
   */
  vtkDataArraySelection* GetSideSetFieldSelection()
  {
    return this->GetFieldSelection(EntityType::SIDESET);
  }
  /////////////////////////////////////////////////////////////////////////////////////////

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
  bool ChooseFieldsToWrite;
  bool RemoveGhosts;
  bool OffsetGlobalIds;
  bool PreserveOriginalIds;
  bool WriteQAAndInformationRecords;
  double DisplacementMagnitude;
  int TimeStepRange[2];
  int TimeStepStride;

  std::set<std::string> Selectors[EntityType::NUMBER_OF_ENTITY_TYPES];
  vtkNew<vtkDataArraySelection> FieldSelection[EntityType::NUMBER_OF_ENTITY_TYPES];
};
VTK_ABI_NAMESPACE_END

#endif
