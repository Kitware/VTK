// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Kitware, Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkXMLPartitionedDataSetCollectionReader
 * @brief   Reader for partitioned dataset collections
 *
 * vtkXMLPartitionedDataSetCollectionReader reads the VTK XML partitioned
 * dataset collection file format. These are meta-files that point to a list
 * of serial VTK XML files. When reading in parallel, it will distribute
 * sub-blocks among processor. If the number of sub-blocks is less than
 * the number of processors, some processors will not have any sub-blocks
 * for that block. If the number of sub-blocks is larger than the
 * number of processors, each processor will possibly have more than
 * 1 sub-block.
 */

#ifndef vtkXMLPartitionedDataSetCollectionReader_h
#define vtkXMLPartitionedDataSetCollectionReader_h

#include "vtkIOXMLModule.h"   // For export macro
#include "vtkWrappingHints.h" // For VTK_MARSHALAUTO
#include "vtkXMLCompositeDataReader.h"

#include <set>    // For std::set
#include <vector> // For std::vector

VTK_ABI_NAMESPACE_BEGIN
class vtkDataAssembly;

class VTKIOXML_EXPORT VTK_MARSHALAUTO vtkXMLPartitionedDataSetCollectionReader
  : public vtkXMLCompositeDataReader
{
public:
  static vtkXMLPartitionedDataSetCollectionReader* New();
  vtkTypeMacro(vtkXMLPartitionedDataSetCollectionReader, vtkXMLCompositeDataReader);
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
  virtual bool AddSelector(const char* selector);
  virtual void ClearSelectors();
  virtual void SetSelector(const char* selector);
  ///@}

  ///@{
  /**
   * API to access selectors.
   */
  virtual int GetNumberOfSelectors() const;
  virtual const char* GetSelector(int index) const;
  ///@}

protected:
  vtkXMLPartitionedDataSetCollectionReader();
  ~vtkXMLPartitionedDataSetCollectionReader() override;

  // Read the XML element for the subtree of a composite dataset.
  // dataSetIndex is used to rank the leaf nodes in an inorder traversal.
  void ReadComposite(vtkXMLDataElement* element, vtkCompositeDataSet* composite,
    const char* filePath, unsigned int& dataSetIndex) override;

  // Get the name of the data set being read.
  const char* GetDataSetName() override;

  int FillOutputPortInformation(int, vtkInformation* info) override;

  virtual const char* GetXMLPartitionsName() { return "Partitions"; }

  virtual const char* GetXMLPartitionIndexName() { return "index"; }

  /**
   * Create the meta-data from the partitioned dataset collection from the file.
   */
  void CreateMetaData(vtkXMLDataElement* ePrimary) override;

  /**
   * Recursively synchronize the data array selection of the reader for the file specified in the
   * XML element.
   */
  void SyncCompositeDataArraySelections(vtkCompositeDataSet* composite, vtkXMLDataElement* element,
    const std::string& filePath) override;

  /**
   * Given the composite id, this method tells if the block should be read
   */
  virtual bool IsBlockSelected(unsigned int compositeIndex);

  /**
   * Given the data object class, return whether it is allowed.
   */
  virtual bool CanReadDataObject(vtkDataObject* dataObject);

private:
  vtkXMLPartitionedDataSetCollectionReader(
    const vtkXMLPartitionedDataSetCollectionReader&) = delete;
  void operator=(const vtkXMLPartitionedDataSetCollectionReader&) = delete;

  int AssemblyTag = 0;
  vtkNew<vtkDataAssembly> Assembly;
  std::set<std::string> Selectors;
  std::vector<unsigned int> SelectedCompositeIds;
};

VTK_ABI_NAMESPACE_END
#endif
