/*=========================================================================

  Program:   ParaView
  Module:    vtkXMLUniformGridAMRReader.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkXMLUniformGridAMRReader
 * @brief   Reader for amr datasets (vtkOverlappingAMR
 * or vtkNonOverlappingAMR).
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

#include "vtkIOXMLModule.h" // For export macro
#include "vtkXMLCompositeDataReader.h"
#include "vtkSmartPointer.h" // needed for vtkSmartPointer.

class vtkOverlappingAMR;
class vtkUniformGridAMR;

class VTKIOXML_EXPORT vtkXMLUniformGridAMRReader :
  public vtkXMLCompositeDataReader
{
public:
  static vtkXMLUniformGridAMRReader* New();
  vtkTypeMacro(vtkXMLUniformGridAMRReader,vtkXMLCompositeDataReader);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
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
  //@}

protected:
  vtkXMLUniformGridAMRReader();
  ~vtkXMLUniformGridAMRReader() VTK_OVERRIDE;

  /**
   * This method is used by CanReadFile() to check if the reader can read an XML
   * with the primary element with the given name. Default implementation
   * compares the name with the text returned by this->GetDataSetName().
   * Overridden to support all AMR types.
   */
  int CanReadFileWithDataType(const char* dsname) VTK_OVERRIDE;

  /**
   * Read the top-level element from the file.  This is always the
   * VTKFile element.
   * Overridden to read the "type" information specified in the XML. The "type"
   * attribute helps us identify the output data type.
   */
  int ReadVTKFile(vtkXMLDataElement* eVTKFile) VTK_OVERRIDE;

  /**
   * Read the meta-data from the AMR from the file. Note that since
   * ReadPrimaryElement() is only called when the filename changes, we are
   * technically not supporting time-varying AMR datasets in this format right
   * now.
   */
  int ReadPrimaryElement(vtkXMLDataElement* ePrimary) VTK_OVERRIDE;

  /**
   * Overridden to create an output data object based on the type in the file.
   * Since this reader can handle all subclasses of vtkUniformGrid, we need to
   * check in the file to decide what type to create.
   */
  int RequestDataObject(vtkInformation *request,
    vtkInformationVector **inputVector, vtkInformationVector *outputVector) VTK_OVERRIDE;

  /**
   * Overridden to put vtkOverlappingAMR in the pipeline if
   * available/applicable.
   */
  int RequestInformation(vtkInformation *request,
    vtkInformationVector **inputVector, vtkInformationVector *outputVector) VTK_OVERRIDE;

  // Get the name of the data set being read.
  const char* GetDataSetName() VTK_OVERRIDE;

  // Read the XML element for the subtree of a the composite dataset.
  // dataSetIndex is used to rank the leaf nodes in an inorder traversal.
  void ReadComposite(vtkXMLDataElement* element,
    vtkCompositeDataSet* composite, const char* filePath,
    unsigned int &dataSetIndex) VTK_OVERRIDE;

  // Read the vtkDataSet (a leaf) in the composite dataset.
  vtkDataSet* ReadDataset(vtkXMLDataElement* xmlElem, const char* filePath) VTK_OVERRIDE;

  vtkSmartPointer<vtkOverlappingAMR> Metadata;
  unsigned int MaximumLevelsToReadByDefault;

private:
  vtkXMLUniformGridAMRReader(const vtkXMLUniformGridAMRReader&) VTK_DELETE_FUNCTION;
  void operator=(const vtkXMLUniformGridAMRReader&) VTK_DELETE_FUNCTION;

  char* OutputDataType;
  vtkSetStringMacro(OutputDataType);
};

#endif
