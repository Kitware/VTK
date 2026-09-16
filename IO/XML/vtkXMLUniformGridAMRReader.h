// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Kitware, Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkXMLUniformGridAMRReader
 * @brief   Reader for amr datasets (vtkOverlappingAMR or vtkNonOverlappingAMR).
 *
 * vtkXMLUniformGridAMRReader reads the VTK XML data files for all types of amr
 * datasets including vtkOverlappingAMR, vtkNonOverlappingAMR and the legacy
 * vtkHierarchicalBoxDataSet. The reader uses information in the file to
 * determine what type of dataset is actually being read and creates the
 * output-data object accordingly.
 *
 * This reader can only read files with version 1.1 or greater.
 * Older versions can be converted to the newer versions using
 * vtkXMLHierarchicalBoxDataFileConverter.
 */

#ifndef vtkXMLUniformGridAMRReader_h
#define vtkXMLUniformGridAMRReader_h

#include "vtkIOXMLModule.h"   // For export macro
#include "vtkSmartPointer.h"  // needed for vtkSmartPointer.
#include "vtkWrappingHints.h" // For VTK_MARSHALAUTO
#include "vtkXMLPartitionedDataSetCollectionReader.h"

#include <map> // std::map

VTK_ABI_NAMESPACE_BEGIN
class vtkOverlappingAMR;
class vtkUniformGridAMR;

class VTKIOXML_EXPORT VTK_MARSHALAUTO vtkXMLUniformGridAMRReader
  : public vtkXMLPartitionedDataSetCollectionReader
{
public:
  static vtkXMLUniformGridAMRReader* New();
  vtkTypeMacro(vtkXMLUniformGridAMRReader, vtkXMLPartitionedDataSetCollectionReader);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * This reader supports demand-driven heavy data reading i.e. downstream
   * pipeline can request specific blocks from the AMR using
   * vtkCompositeDataPipeline::UPDATE_COMPOSITE_INDICES() key in
   * RequestUpdateExtent() pass. However, when down-stream doesn't provide any
   * specific keys, the default behavior can be setup to read at-most N levels
   * by default. The number of levels read can be set using this method.
   * Set this to 0 to imply no limit. Default is 0.
   */
  vtkSetMacro(MaximumLevelsToReadByDefault, unsigned int);
  vtkGetMacro(MaximumLevelsToReadByDefault, unsigned int);
  ///@}

protected:
  vtkXMLUniformGridAMRReader();
  ~vtkXMLUniformGridAMRReader() override;

  // Get the name of the data set being read.
  const char* GetDataSetName() override;

  /**
   * This method is used by CanReadFile() to check if the reader can read an XML
   * with the primary element with the given name. Default implementation
   * compares the name with the text returned by this->GetDataSetName().
   * Overridden to support all AMR types.
   */
  int CanReadFileWithDataType(const char* dsname) override;

  /**
   * Read the top-level element from the file.  This is always the
   * VTKFile element.
   * Overridden to read the "type" information specified in the XML. The "type"
   * attribute helps us identify the output data type.
   */
  int ReadVTKFile(vtkXMLDataElement* eVTKFile) override;

  const char* GetXMLPartitionsName() override { return "Block"; }

  const char* GetXMLPartitionIndexName() override { return "level"; }

  /**
   * Read the meta-data from the AMR from the file.
   */
  void CreateMetaData(vtkXMLDataElement* ePrimary) override;

  /**
   * Synchronize the data array selection of the reader for the file specified in the XML element.
   */
  void SyncCompositeDataArraySelections(vtkCompositeDataSet* composite, vtkXMLDataElement* element,
    const std::string& filePath) override
  {
    this->Superclass::SyncCompositeDataArraySelections(composite, element, filePath);
  }

  /**
   * Overridden to create an output data object based on the type in the file.
   * Since this reader can handle all subclasses of vtkUniformGrid, we need to
   * check in the file to decide what type to create.
   */
  int RequestDataObject(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  /**
   * Given the composite id, this method tells if the block should be read
   */
  bool IsBlockSelected(unsigned int compositeIndex) override;

  /**
   * Given the data object class, return whether it is allowed.
   */
  bool CanReadDataObject(vtkDataObject* dataObject) override;

  // Read the vtkDataSet (a leaf) in the composite dataset.
  vtkDataObject* ReadDataObject(vtkXMLDataElement* xmlElem, const char* filePath) override;

  // Read the XML element for the subtree of a composite dataset.
  // dataSetIndex is used to rank the leaf nodes in an inorder traversal.
  void ReadComposite(vtkXMLDataElement* element, vtkCompositeDataSet* composite,
    const char* filePath, unsigned int& dataSetIndex) override;

private:
  vtkXMLUniformGridAMRReader(const vtkXMLUniformGridAMRReader&) = delete;
  void operator=(const vtkXMLUniformGridAMRReader&) = delete;

  bool HasBlockRequests = true;
  unsigned int MaximumLevelsToReadByDefault = 0;
  std::map<unsigned int, unsigned int> CompositeIdToLevel;

  // Hide Selection functionality since, AMR only support levels
  vtkDataAssembly* GetAssembly() override { return nullptr; }
  int GetAssemblyTag() VTK_FUTURE_CONST override { return 0; }
  bool AddSelector(const char* vtkNotUsed(selector)) override { return false; }
  void ClearSelectors() override {}
  void SetSelector(const char* vtkNotUsed(selector)) override {}
  int GetNumberOfSelectors() const override { return 0; }
  const char* GetSelector(int vtkNotUsed(index)) const override { return nullptr; }

  char* OutputDataType = nullptr;
  vtkSetStringMacro(OutputDataType);
};

VTK_ABI_NAMESPACE_END
#endif
