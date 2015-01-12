/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPhyloXMLTreeWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPhyloXMLTreeWriter - write vtkTree data to PhyloXML format.
// .SECTION Description
// vtkPhyloXMLTreeWriter is writes a vtkTree to a PhyloXML formatted file
// or string.

#ifndef vtkPhyloXMLTreeWriter_h
#define vtkPhyloXMLTreeWriter_h

#include "vtkIOInfovisModule.h" // For export macro
#include "vtkXMLWriter.h"
#include "vtkSmartPointer.h"    // For SP ivars
#include "vtkStdString.h"       // For get/set ivars

class vtkStringArray;
class vtkTree;
class vtkXMLDataElement;

class VTKIOINFOVIS_EXPORT vtkPhyloXMLTreeWriter : public vtkXMLWriter
{
public:
  static vtkPhyloXMLTreeWriter *New();
  vtkTypeMacro(vtkPhyloXMLTreeWriter,vtkXMLWriter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get the input to this writer.
  vtkTree* GetInput();
  vtkTree* GetInput(int port);

  // Description:
  // Get the default file extension for files written by this writer.
  virtual const char* GetDefaultFileExtension();

  // Description:
  // Get/Set the name of the input's tree edge weight array.
  // This array must be part of the input tree's EdgeData.
  // The default name is "weight".  If this array cannot be
  // found, then no edge weights will be included in the
  // output of this writer.
  vtkGetMacro(EdgeWeightArrayName, vtkStdString);
  vtkSetMacro(EdgeWeightArrayName, vtkStdString);

  // Description:
  // Get/Set the name of the input's tree node name array.
  // This array must be part of the input tree's VertexData.
  // The default name is "node name".  If this array cannot
  // be found, then no node names will be included in the
  // output of this writer.
  vtkGetMacro(NodeNameArrayName, vtkStdString);
  vtkSetMacro(NodeNameArrayName, vtkStdString);

  // Description:
  // Do not include name the VertexData array in the PhyloXML output
  // of this writer.  Call this function once for each array that
  // you wish to ignore.
  void IgnoreArray(const char * arrayName);

protected:
  vtkPhyloXMLTreeWriter();
  ~vtkPhyloXMLTreeWriter() {}

  virtual int WriteData();

  virtual const char* GetDataSetName();
  virtual int StartFile();
  virtual int EndFile();

  // Description:
  // Check for an optional, tree-level element and write it out if it is
  // found.
  void WriteTreeLevelElement(vtkTree *input,
                             vtkXMLDataElement *rootElement,
                             const char *elementName,
                             const char *attributeName);

  // Description:
  // Search for any tree-level properties and write them out if they are found.
  void WriteTreeLevelProperties(vtkTree *input, vtkXMLDataElement *rootElement);

  // Description:
  // Convert one vertex to PhyloXML.  This function calls itself recursively
  // for any children of the input vertex.
  void WriteCladeElement(vtkTree* const input, vtkIdType vertex,
                   vtkXMLDataElement *parentElement);

  // Description:
  // Write the branch length attribute for the specified vertex.
  void WriteBranchLengthAttribute(vtkTree* const input, vtkIdType vertex,
                                  vtkXMLDataElement *element);

  // Description:
  // Write the name element for the specified vertex.
  void WriteNameElement(vtkIdType vertex, vtkXMLDataElement *element);

  // Description:
  // Write the confidence element for the specified vertex.
  void WriteConfidenceElement(vtkTree* const input, vtkIdType vertex,
                              vtkXMLDataElement *element);

  // Description:
  // Write the color element and its subelements (red, green, blue)
  // for the specified vertex.
  void WriteColorElement(vtkTree* const input, vtkIdType vertex,
                         vtkXMLDataElement *element);

  // Description:
  // Write a property element as a child of the specified vtkXMLDataElement.
  void WritePropertyElement(vtkAbstractArray *array, vtkIdType vertex,
                            vtkXMLDataElement *element);

  // Description:
  // Get the value of the requested attribute from the specified array's
  // vtkInformation.
  const char* GetArrayAttribute(vtkAbstractArray *array,
                                const char *attributeName);

  virtual int FillInputPortInformation(int port, vtkInformation *info);

  vtkInformation* InputInformation;

  vtkStdString EdgeWeightArrayName;
  vtkStdString NodeNameArrayName;

  vtkAbstractArray *EdgeWeightArray;
  vtkAbstractArray *NodeNameArray;
  vtkSmartPointer<vtkStringArray> Blacklist;

private:
  vtkPhyloXMLTreeWriter(const vtkPhyloXMLTreeWriter&);  // Not implemented.
  void operator=(const vtkPhyloXMLTreeWriter&);  // Not implemented.
};

#endif
