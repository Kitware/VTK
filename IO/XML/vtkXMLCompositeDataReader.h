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
  void PrintSelf(ostream& os, vtkIndent indent) override;

  enum
  {
    Block,
    Interleave
  };

  /**
   * Set the strategy for assigning files to parallel readers. The default is
   * @a Block.
   *
   * Let @a X be the rank of a specific reader, and @a N be the number of
   * reader, then:
   * @arg @c Block Each processor is assigned a contiguous block of files,
   *      [@a X * @a N, ( @a X + 1) * @a N ).
   * @arg @c Interleave The files are interleaved across readers,
   * @a i * @a N + @a X.
   * @{
   */
  vtkSetClampMacro(PieceDistribution, int, Block, Interleave)
  vtkGetMacro(PieceDistribution, int)
  /**@}*/

  //@{
  /**
   * Get the output data object for a port on this algorithm.
   */
  vtkCompositeDataSet* GetOutput();
  vtkCompositeDataSet* GetOutput(int);
  //@}

protected:
  vtkXMLCompositeDataReader();
  ~vtkXMLCompositeDataReader() override;

  // Get the name of the data set being read.
  const char* GetDataSetName() override;

  // Returns the primary element pass to ReadPrimaryElement().
  vtkXMLDataElement* GetPrimaryElement();

  void ReadXMLData() override;
  int ReadPrimaryElement(vtkXMLDataElement* ePrimary) override;

  // Setup the output with no data available.  Used in error cases.
  void SetupEmptyOutput() override;

  int FillOutputPortInformation(int, vtkInformation* info) override;

  // Create a default executive.
  vtkExecutive* CreateDefaultExecutive() override;

  // Find the path to this file in case the internal files are
  // specified as relative paths.
  std::string GetFilePath();

  std::string GetFileNameFromXML(vtkXMLDataElement *xmlElem,
                                 const std::string &filePath);

  vtkXMLReader* GetReaderOfType(const char* type);
  vtkXMLReader* GetReaderForFile(const std::string &filename);

  int RequestInformation(vtkInformation*,
                                 vtkInformationVector**,
                                 vtkInformationVector*) override;

  void SyncDataArraySelections(vtkXMLReader *accum,
                               vtkXMLDataElement *xmlElem,
                               const std::string &filePath);

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

  // Read the vtkDataObject (a leaf) in the composite dataset.
  virtual vtkDataObject* ReadDataObject(vtkXMLDataElement* xmlElem, const char* filePath);

  // Counts "DataSet" elements in the subtree.
  unsigned int CountLeaves(vtkXMLDataElement* elem);

  /**
   * Given the inorder index for a leaf node, this method tells if the current
   * process should read the dataset.
   */
  int ShouldReadDataSet(unsigned int datasetIndex);

  bool DataSetIsValidForBlockStrategy(unsigned int datasetIndex);
  bool DataSetIsValidForInterleaveStrategy(unsigned int datasetIndex);

private:
  vtkXMLCompositeDataReader(const vtkXMLCompositeDataReader&) = delete;
  void operator=(const vtkXMLCompositeDataReader&) = delete;

  int PieceDistribution;

  vtkXMLCompositeDataReaderInternals* Internal;
};

#endif
