// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Kitware, Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkXMLMultiBlockDataReader
 * @brief   Reader for multi-block datasets
 *
 * vtkXMLMultiBlockDataReader reads the VTK XML multi-block data file
 * format. XML multi-block data files are meta-files that point to a list
 * of serial VTK XML files. When reading in parallel, it will distribute
 * sub-blocks among processor. If the number of sub-blocks is less than
 * the number of processors, some processors will not have any sub-blocks
 * for that block. If the number of sub-blocks is larger than the
 * number of processors, each processor will possibly have more than
 * 1 sub-block.
 */

#ifndef vtkXMLMultiBlockDataReader_h
#define vtkXMLMultiBlockDataReader_h

#include "vtkIOXMLModule.h"   // For export macro
#include "vtkWrappingHints.h" // For VTK_MARSHALAUTO
#include "vtkXMLCompositeDataReader.h"

#include <set>    // For std::set
#include <vector> // For std::vector

VTK_ABI_NAMESPACE_BEGIN
class vtkDataAssembly;

class VTKIOXML_EXPORT VTK_MARSHALAUTO vtkXMLMultiBlockDataReader : public vtkXMLCompositeDataReader
{
public:
  static vtkXMLMultiBlockDataReader* New();
  vtkTypeMacro(vtkXMLMultiBlockDataReader, vtkXMLCompositeDataReader);
  void PrintSelf(ostream& os, vtkIndent indent) override;

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
   * The default is "/" to maintain backwards compatibility
   *
   * @sa vtkDataAssembly::SelectNodes
   */
  bool AddSelector(const char* selector);
  void ClearSelectors();
  void SetSelector(const char* selector);
  ///@}

  ///@{
  /**
   * API to access selectors.
   */
  int GetNumberOfSelectors() const;
  const char* GetSelector(int index) const;
  ///@}

protected:
  vtkXMLMultiBlockDataReader();
  ~vtkXMLMultiBlockDataReader() override;

  // Get the name of the data set being read.
  const char* GetDataSetName() override;

  int FillOutputPortInformation(int, vtkInformation* info) override;

  // Read the XML element for the subtree of a composite dataset.
  // dataSetIndex is used to rank the leaf nodes in an inorder traversal.
  void ReadComposite(vtkXMLDataElement* element, vtkCompositeDataSet* composite,
    const char* filePath, unsigned int& dataSetIndex) override;

  // Reads file version < 1.0.
  virtual void ReadVersion0(vtkXMLDataElement* element, vtkCompositeDataSet* composite,
    const char* filePath, unsigned int& dataSetIndex);

  void PrepareToCreateMetaData(vtkXMLDataElement* ePrimary) override;

  /**
   * Read the meta-data from the Multiblock from the file.
   */
  void CreateMetaData(vtkXMLDataElement* ePrimary) override;

  /**
   * Recursively synchronize the data array selection of the reader for the file specified in the
   * XML element.
   */
  void SyncCompositeDataArraySelections(vtkCompositeDataSet* composite, vtkXMLDataElement* element,
    const std::string& filePath) override;

private:
  vtkXMLMultiBlockDataReader(const vtkXMLMultiBlockDataReader&) = delete;
  void operator=(const vtkXMLMultiBlockDataReader&) = delete;

  virtual int FillMetaData(vtkCompositeDataSet* composite, vtkXMLDataElement* element);

  void ReadCompositeInternal(vtkXMLDataElement* element, vtkCompositeDataSet* composite,
    const char* filePath, unsigned int& dataSetIndex, unsigned int& compositeIndex);

  bool IsBlockSelected(unsigned int compositeIndex);

  int AssemblyTag = 0;
  vtkNew<vtkDataAssembly> Assembly;
  std::set<std::string> Selectors;
  std::vector<unsigned int> SelectedCompositeIds;
  bool DistributePiecesInMultiPieces;
};

VTK_ABI_NAMESPACE_END
#endif
