/*=========================================================================

  Program:   ParaView
  Module:    vtkXMLPartitionedDataSetReader.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkXMLPartitionedDataSetReader
 * @brief   Reader for partitioned datasets
 *
 * vtkXMLPartitionedDataSetReader reads the VTK XML partitioned dataset file
 * format. XML partitioned dataset files are meta-files that point to a list
 * of serial VTK XML files. When reading in parallel, it will distribute
 * sub-blocks among processors. If the number of sub-blocks is less than
 * the number of processors, some processors will not have any sub-blocks
 * for that block. If the number of sub-blocks is larger than the
 * number of processors, each processor will possibly have more than
 * 1 sub-block.
*/

#ifndef vtkXMLPartitionedDataSetReader_h
#define vtkXMLPartitionedDataSetReader_h

#include "vtkIOXMLModule.h" // For export macro
#include "vtkXMLCompositeDataReader.h"

class vtkMultiBlockDataSet;

class VTKIOXML_EXPORT vtkXMLPartitionedDataSetReader : public vtkXMLCompositeDataReader
{
public:
  static vtkXMLPartitionedDataSetReader* New();
  vtkTypeMacro(vtkXMLPartitionedDataSetReader,vtkXMLCompositeDataReader);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkXMLPartitionedDataSetReader();
  ~vtkXMLPartitionedDataSetReader() override;

  // Read the XML element for the subtree of a the composite dataset.
  // dataSetIndex is used to rank the leaf nodes in an inorder traversal.
  void ReadComposite(vtkXMLDataElement* element,
    vtkCompositeDataSet* composite, const char* filePath,
    unsigned int &dataSetIndex) override;

  // Get the name of the data set being read.
  const char* GetDataSetName() override;

  int FillOutputPortInformation(int, vtkInformation* info) override;

private:
  vtkXMLPartitionedDataSetReader(const vtkXMLPartitionedDataSetReader&) = delete;
  void operator=(const vtkXMLPartitionedDataSetReader&) = delete;
};

#endif
