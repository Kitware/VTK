/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLPartitionedDataSetCollectionWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkXMLPartitionedDataSetCollectionWriter
 * @brief   writer for vtkPartitionedDataSetCollection.
 *
 * vtkXMLPartitionedDataSetCollectionWriter is a vtkXMLCompositeDataWriter
 * subclass to handle vtkPartitionedDataSetCollection.
*/

#ifndef vtkXMLPartitionedDataSetCollectionWriter_h
#define vtkXMLPartitionedDataSetCollectionWriter_h

#include "vtkIOXMLModule.h" // For export macro
#include "vtkXMLCompositeDataWriter.h"

class VTKIOXML_EXPORT vtkXMLPartitionedDataSetCollectionWriter : public vtkXMLCompositeDataWriter
{
public:
  static vtkXMLPartitionedDataSetCollectionWriter* New();
  vtkTypeMacro(vtkXMLPartitionedDataSetCollectionWriter, vtkXMLCompositeDataWriter);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Get the default file extension for files written by this writer.
   */
  const char* GetDefaultFileExtension() override
    { return "vtpc"; }

protected:
  vtkXMLPartitionedDataSetCollectionWriter();
  ~vtkXMLPartitionedDataSetCollectionWriter() override;

  int FillInputPortInformation(int port, vtkInformation* info) override;

  // Internal method called recursively to create the xml tree for the children
  // of compositeData.
  int WriteComposite(vtkCompositeDataSet* compositeData,
    vtkXMLDataElement* parent, int &writerIdx) override;

private:
  vtkXMLPartitionedDataSetCollectionWriter(const vtkXMLPartitionedDataSetCollectionWriter&) = delete;
  void operator=(const vtkXMLPartitionedDataSetCollectionWriter&) = delete;

};

#endif
