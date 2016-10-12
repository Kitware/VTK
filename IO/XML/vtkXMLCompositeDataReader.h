/*=========================================================================

  Program:   ParaView
  Module:    vtkXMLCompositeDataReader.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkXMLCompositeDataReader
 * @brief   Reader for multi-group datasets
 *
 * vtkXMLCompositeDataReader reads the VTK XML multi-group data file
 * format. XML multi-group data files are meta-files that point to a list
 * of serial VTK XML files. When reading in parallel, it will distribute
 * sub-blocks among processor. If the number of sub-blocks is less than
 * the number of processors, some processors will not have any sub-blocks
 * for that group. If the number of sub-blocks is larger than the
 * number of processors, each processor will possibly have more than
 * 1 sub-block.
*/

#ifndef vtkXMLCompositeDataReader_h
#define vtkXMLCompositeDataReader_h

#include "vtkIOXMLModule.h" // For export macro
#include "vtkXMLReader.h"

class vtkCompositeDataSet;
class vtkInformationIntegerKey;
class vtkInformationIntegerVectorKey;

struct vtkXMLCompositeDataReaderInternals;

class VTKIOXML_EXPORT vtkXMLCompositeDataReader : public vtkXMLReader
{
public:
  vtkTypeMacro(vtkXMLCompositeDataReader,vtkXMLReader);
  void PrintSelf(ostream& os, vtkIndent indent);

  //@{
  /**
   * Get the output data object for a port on this algorithm.
   */
  vtkCompositeDataSet* GetOutput();
  vtkCompositeDataSet* GetOutput(int);
  //@}

protected:
  vtkXMLCompositeDataReader();
  ~vtkXMLCompositeDataReader();

  // Get the name of the data set being read.
  virtual const char* GetDataSetName();

  // Returns the primary element pass to ReadPrimaryElement().
  vtkXMLDataElement* GetPrimaryElement();

  virtual void ReadXMLData();
  virtual int ReadPrimaryElement(vtkXMLDataElement* ePrimary);

  // Setup the output with no data available.  Used in error cases.
  virtual void SetupEmptyOutput();

  virtual int FillOutputPortInformation(int, vtkInformation* info);

  // Create a default executive.
  virtual vtkExecutive* CreateDefaultExecutive();

  vtkXMLReader* GetReaderOfType(const char* type);

  virtual int RequestInformation(vtkInformation*,
                                 vtkInformationVector**,
                                 vtkInformationVector*);



  // Adds a child data object to the composite parent. childXML is the XML for
  // the child data object need to obtain certain meta-data about the child.
  void AddChild(vtkCompositeDataSet* parent,
    vtkDataObject* child, vtkXMLDataElement* childXML);

  // Read the XML element for the subtree of a the composite dataset.
  // dataSetIndex is used to rank the leaf nodes in an inorder traversal.
  virtual void ReadComposite(vtkXMLDataElement* element,
    vtkCompositeDataSet* composite, const char* filePath,
    unsigned int &dataSetIndex)=0;

  // Read the vtkDataSet (a leaf) in the composite dataset.
  virtual vtkDataSet* ReadDataset(vtkXMLDataElement* xmlElem, const char* filePath);

  // Counts "DataSet" elements in the subtree.
  unsigned int CountLeaves(vtkXMLDataElement* elem);

  /**
   * Given the inorder index for a leaf node, this method tells if the current
   * process should read the dataset.
   */
  int ShouldReadDataSet(unsigned int datasetIndex);

private:
  vtkXMLCompositeDataReader(const vtkXMLCompositeDataReader&) VTK_DELETE_FUNCTION;
  void operator=(const vtkXMLCompositeDataReader&) VTK_DELETE_FUNCTION;

  vtkXMLCompositeDataReaderInternals* Internal;
};

#endif
