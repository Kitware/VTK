/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGraphReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkGraphReader
 * @brief   read vtkGraph data file
 *
 * vtkGraphReader is a source object that reads ASCII or binary
 * vtkGraph data files in vtk format. (see text for format details).
 * The output of this reader is a single vtkGraph data object.
 * The superclass of this class, vtkDataReader, provides many methods for
 * controlling the reading of the data file, see vtkDataReader for more
 * information.
 * @warning
 * Binary files written on one system may not be readable on other systems.
 * @sa
 * vtkGraph vtkDataReader vtkGraphWriter
*/

#ifndef vtkGraphReader_h
#define vtkGraphReader_h

#include "vtkIOLegacyModule.h" // For export macro
#include "vtkDataReader.h"

class vtkGraph;

class VTKIOLEGACY_EXPORT vtkGraphReader : public vtkDataReader
{
public:
  static vtkGraphReader *New();
  vtkTypeMacro(vtkGraphReader,vtkDataReader);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * Get the output of this reader.
   */
  vtkGraph *GetOutput();
  vtkGraph *GetOutput(int idx);
  void SetOutput(vtkGraph *output);
  //@}

protected:
  vtkGraphReader();
  ~vtkGraphReader() VTK_OVERRIDE;

  enum GraphType
  {
    UnknownGraph,
    DirectedGraph,
    UndirectedGraph,
    Molecule
  };

  int RequestData(vtkInformation *, vtkInformationVector **,
                          vtkInformationVector *) VTK_OVERRIDE;

  // Override ProcessRequest to handle request data object event
  int ProcessRequest(vtkInformation *, vtkInformationVector **,
                             vtkInformationVector *) VTK_OVERRIDE;

  // Since the Outputs[0] has the same UpdateExtent format
  // as the generic DataObject we can copy the UpdateExtent
  // as a default behavior.
  int RequestUpdateExtent(vtkInformation *, vtkInformationVector **,
                                  vtkInformationVector *) VTK_OVERRIDE;

  // Create output (a directed or undirected graph).
  virtual int RequestDataObject(vtkInformation *, vtkInformationVector **,
                                vtkInformationVector *);

  // Read beginning of file to determine whether the graph is directed.
  virtual int ReadGraphType(GraphType &type);


  int FillOutputPortInformation(int, vtkInformation*) VTK_OVERRIDE;
private:
  vtkGraphReader(const vtkGraphReader&) VTK_DELETE_FUNCTION;
  void operator=(const vtkGraphReader&) VTK_DELETE_FUNCTION;
};

#endif
