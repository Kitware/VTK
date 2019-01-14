/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPhyloXMLTreeReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPhyloXMLTreeReader
 * @brief   read vtkTree from PhyloXML formatted file
 *
 * vtkPhyloXMLTreeReader is a source object that reads PhyloXML tree format
 * files.
 * The output of this reader is a single vtkTree data object.
 *
 *
 * @warning
 * This reader does not implement the entire PhyloXML specification.
 * It currently only supports the following tags:
 * phylogeny, name, description, confidence, property, clade, branch_length,
 * color, red, green, and blue.
 * This reader also only supports a single phylogeny per file.
 *
 * @sa
 * vtkTree vtkXMLReader vtkPhyloXMLTreeWriter
*/

#ifndef vtkPhyloXMLTreeReader_h
#define vtkPhyloXMLTreeReader_h

#include "vtkIOInfovisModule.h" // For export macro
#include "vtkSmartPointer.h"    // For SP ivar
#include "vtkXMLReader.h"

class vtkBitArray;
class vtkMutableDirectedGraph;
class vtkTree;
class vtkXMLDataElement;

class VTKIOINFOVIS_EXPORT vtkPhyloXMLTreeReader : public vtkXMLReader
{
public:
  static vtkPhyloXMLTreeReader *New();
  vtkTypeMacro(vtkPhyloXMLTreeReader,vtkXMLReader);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Get the output of this reader.
   */
  vtkTree *GetOutput();
  vtkTree *GetOutput(int idx);
  //@}

protected:
  vtkPhyloXMLTreeReader();
  ~vtkPhyloXMLTreeReader() override;

  /**
   * Read the input PhyloXML and populate our output vtkTree.
   */
  void ReadXMLData() override;

  /**
   * Read one particular XML element.  This method calls the more specific
   * methods (ReadCladeElement, ReadNameElement, etc) based on what type
   * of tag it encounters.
   */
  void ReadXMLElement(vtkXMLDataElement *element, vtkMutableDirectedGraph *g,
                      vtkIdType vertex);

  /**
   * Read a clade element.  This method does not parse the subelements of
   * the clade.  Instead, this task is handled by other methods of this class.
   * This method returns the vtkIdType of the newly created vertex in our
   * output vtkTree.
   */
  vtkIdType ReadCladeElement(vtkXMLDataElement *element,
                             vtkMutableDirectedGraph *g, vtkIdType parent);

  /**
   * Read a name and assign it to the specified vertex, or the whole tree
   * if vertex is -1.
   */
  void ReadNameElement(vtkXMLDataElement *element, vtkMutableDirectedGraph *g,
                       vtkIdType vertex);

  /**
   * Read the description for the tree.
   */
  void ReadDescriptionElement(vtkXMLDataElement *element,
                              vtkMutableDirectedGraph *g);

  /**
   * Read a property and assign it to our output vtkTree's VertexData for the
   * specified vertex.  If this property has not been encountered yet, this
   * method creates a new array and adds it to the VertexData.
   */
  void ReadPropertyElement(vtkXMLDataElement *element,
    vtkMutableDirectedGraph *g, vtkIdType vertex);

  /**
   * Read & store the branch length for this clade.  Branch length is defined
   * as the edge weight from this vertex to its parent.  Note that this
   * value can also be specified as an attribute of the clade element.
   */
  void ReadBranchLengthElement(vtkXMLDataElement *element,
    vtkMutableDirectedGraph *g, vtkIdType vertex);

  /**
   * Read confidence value and store it for the specified vertex, or the
   * whole tree is vertex is -1.
   */
  void ReadConfidenceElement(vtkXMLDataElement *element,
    vtkMutableDirectedGraph *g, vtkIdType vertex);

  /**
   * Read RGB color value for this vertex.  Note that this color is also
   * applied to all children of this vertex until a new value is specified.
   */
  void ReadColorElement(vtkXMLDataElement *element, vtkMutableDirectedGraph *g,
    vtkIdType vertex);

  /**
   * Assign the parent's branch color to child vertices where none is
   * otherwise specified.
   */
  void PropagateBranchColor(vtkTree *tree);

  /**
   * Count the number of vertices in the tree.
   */
  void CountNodes(vtkXMLDataElement *element);

  /**
   * Return a copy of the input string with all leading & trailing
   * whitespace removed.
   */
  std::string GetTrimmedString(const char *input);

  /**
   * Return the portion of the input string that occurs before the
   * first colon (:).
   */
  std::string GetStringBeforeColon(const char *input);

  /**
   * Return the portion of the input string that occurs after the
   * first colon (:).
   */
  std::string GetStringAfterColon(const char *input);

  int FillOutputPortInformation(int, vtkInformation*) override;
  const char* GetDataSetName() override;
  void SetOutput(vtkTree *output);
  void SetupEmptyOutput() override;

private:
  vtkIdType NumberOfNodes;
  bool HasBranchColor;
  vtkSmartPointer<vtkBitArray> ColoredVertices;
  vtkPhyloXMLTreeReader(const vtkPhyloXMLTreeReader&) = delete;
  void operator=(const vtkPhyloXMLTreeReader&) = delete;
};

#endif
