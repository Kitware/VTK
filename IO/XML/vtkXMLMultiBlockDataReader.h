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

#include "vtkIOXMLModule.h" // For export macro
#include "vtkXMLCompositeDataReader.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkMultiBlockDataSet;

class VTKIOXML_EXPORT vtkXMLMultiBlockDataReader : public vtkXMLCompositeDataReader
{
public:
  static vtkXMLMultiBlockDataReader* New();
  vtkTypeMacro(vtkXMLMultiBlockDataReader, vtkXMLCompositeDataReader);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkXMLMultiBlockDataReader();
  ~vtkXMLMultiBlockDataReader() override;

  // Read the XML element for the subtree of a the composite dataset.
  // dataSetIndex is used to rank the leaf nodes in an inorder traversal.
  void ReadComposite(vtkXMLDataElement* element, vtkCompositeDataSet* composite,
    const char* filePath, unsigned int& dataSetIndex) override;

  // Reads file version < 1.0.
  virtual void ReadVersion0(vtkXMLDataElement* element, vtkCompositeDataSet* composite,
    const char* filePath, unsigned int& dataSetIndex);

  // Get the name of the data set being read.
  const char* GetDataSetName() override;

  int FillOutputPortInformation(int, vtkInformation* info) override;

  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  virtual int FillMetaData(vtkCompositeDataSet* metadata, vtkXMLDataElement* element,
    const std::string& filePath, unsigned int& dataSetIndex);

private:
  vtkXMLMultiBlockDataReader(const vtkXMLMultiBlockDataReader&) = delete;
  void operator=(const vtkXMLMultiBlockDataReader&) = delete;

  bool DistributePiecesInMultiPieces;
};

VTK_ABI_NAMESPACE_END
#endif
