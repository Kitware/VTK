/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkNewickTreeWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkNewickTreeWriter
 * @brief   write vtkTree data to Newick format.
 *
 * vtkNewickTreeWriter is writes a vtkTree to a Newick formatted file
 * or string.
*/

#ifndef vtkNewickTreeWriter_h
#define vtkNewickTreeWriter_h

#include "vtkIOInfovisModule.h" // For export macro
#include "vtkDataWriter.h"
#include "vtkStdString.h"       // For get/set ivars

class vtkTree;

class VTKIOINFOVIS_EXPORT vtkNewickTreeWriter : public vtkDataWriter
{
public:
  static vtkNewickTreeWriter *New();
  vtkTypeMacro(vtkNewickTreeWriter,vtkDataWriter);
  void PrintSelf(ostream& os, vtkIndent indent);

  //@{
  /**
   * Get the input to this writer.
   */
  vtkTree* GetInput();
  vtkTree* GetInput(int port);
  //@}

  //@{
  /**
   * Get/Set the name of the input's tree edge weight array.
   * This array must be part of the input tree's EdgeData.
   * The default name is "weight".  If this array cannot be
   * found, then no edge weights will be included in the
   * output of this writer.
   */
  vtkGetMacro(EdgeWeightArrayName, vtkStdString);
  vtkSetMacro(EdgeWeightArrayName, vtkStdString);
  //@}

  //@{
  /**
   * Get/Set the name of the input's tree node name array.
   * This array must be part of the input tree's VertexData.
   * The default name is "node name".  If this array cannot
   * be found, then no node names will be included in the
   * output of this writer.
   */
  vtkGetMacro(NodeNameArrayName, vtkStdString);
  vtkSetMacro(NodeNameArrayName, vtkStdString);
  //@}

protected:
  vtkNewickTreeWriter();
  ~vtkNewickTreeWriter() {}

  void WriteData();

  /**
   * Write one vertex.  This function calls itself recursively for
   * any children of the input vertex.
   */
  void WriteVertex(ostream *fp, vtkTree* const input, vtkIdType vertex);

  virtual int FillInputPortInformation(int port, vtkInformation *info);

  vtkStdString EdgeWeightArrayName;
  vtkStdString NodeNameArrayName;

  vtkAbstractArray *EdgeWeightArray;
  vtkAbstractArray *NodeNameArray;

private:
  vtkNewickTreeWriter(const vtkNewickTreeWriter&) VTK_DELETE_FUNCTION;
  void operator=(const vtkNewickTreeWriter&) VTK_DELETE_FUNCTION;
};

#endif
