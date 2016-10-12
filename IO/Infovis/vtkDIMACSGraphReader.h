/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDIMACSGraphReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/

/**
 * @class   vtkDIMACSGraphReader
 * @brief   reads vtkGraph data from a DIMACS
 * formatted file
 *
 *
 * vtkDIMACSGraphReader is a source object that reads vtkGraph data files
 * from a DIMACS format.
 *
 * The reader has special handlers for max-flow and graph coloring problems,
 * which are specified in the problem line as 'max' and 'edge' respectively.
 * Other graphs are treated as generic DIMACS files.
 *
 * DIMACS formatted files consist of lines in which the first character in
 * in column 0 specifies the type of the line.
 *
 * Generic DIMACS files have the following line types:
 * - problem statement line : p graph num_verts num_edges
 * - node line (optional)   : n node_id node_weight
 * - edge line              : a src_id trg_id edge_weight
 * - alternate edge format  : e src_id trg_id edge_weight
 * - comment lines          : c I am a comment line
 * ** note, there should be one and only one problem statement line per file.
 *
 *
 * DIMACS graphs are undirected and nodes are numbered 1..n
 *
 * See webpage for additional formatting details.
 * -  http://dimacs.rutgers.edu/Challenges/
 * -  http://www.dis.uniroma1.it/~challenge9/format.shtml
 *
 * @sa
 * vtkDIMACSGraphWriter
 *
*/

#ifndef vtkDIMACSGraphReader_h
#define vtkDIMACSGraphReader_h

#include "vtkIOInfovisModule.h" // For export macro
#include "vtkGraphAlgorithm.h"
#include "vtkStdString.h" // For string API

class VTKIOINFOVIS_EXPORT vtkDIMACSGraphReader : public vtkGraphAlgorithm
{

public:

  static vtkDIMACSGraphReader *New();
  vtkTypeMacro(vtkDIMACSGraphReader, vtkGraphAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  //@{
  /**
   * The DIMACS file name.
   */
  vtkGetStringMacro(FileName);
  vtkSetStringMacro(FileName);
  //@}

  //@{
  /**
   * Vertex attribute array name
   */
  vtkGetStringMacro(VertexAttributeArrayName);
  vtkSetStringMacro(VertexAttributeArrayName);
  //@}

  //@{
  /**
   * Edge attribute array name
   */
  vtkGetStringMacro(EdgeAttributeArrayName);
  vtkSetStringMacro(EdgeAttributeArrayName);
  //@}

protected:

  vtkDIMACSGraphReader();
  ~vtkDIMACSGraphReader();

  virtual int RequestData(vtkInformation *,
                          vtkInformationVector **,
                          vtkInformationVector *);

  int buildGenericGraph(vtkGraph     * output,
                        vtkStdString & defaultVertexAttrArrayName,
                        vtkStdString & defaultEdgeAttrArrayName);

  int buildColoringGraph(vtkGraph * output);
  int buildMaxflowGraph(vtkGraph * output);

  /**
   * Creates directed or undirected output based on Directed flag.
   */
  virtual int RequestDataObject(vtkInformation*,
                                vtkInformationVector** inputVector,
                                vtkInformationVector* outputVector);

  int ReadGraphMetaData();

private:

  bool   fileOk;
  bool   Directed;
  char * FileName;
  char * VertexAttributeArrayName;
  char * EdgeAttributeArrayName;

  int numVerts;
  int numEdges;
  vtkStdString dimacsProblemStr;

  vtkDIMACSGraphReader(const vtkDIMACSGraphReader&) VTK_DELETE_FUNCTION;
  void operator=(const vtkDIMACSGraphReader&) VTK_DELETE_FUNCTION;
};

#endif // vtkDIMACSGraphReader_h
