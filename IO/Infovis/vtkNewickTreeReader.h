/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkNewickTreeReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkNewickTreeReader
 * @brief   read vtkTree from Newick formatted file
 *
 * vtkNewickTreeReader is a source object that reads Newick tree format
 * files.
 * The output of this reader is a single vtkTree data object.
 * The superclass of this class, vtkDataReader, provides many methods for
 * controlling the reading of the data file, see vtkDataReader for more
 * information.
 * @par Thanks:
 * This class is adapted from code originally written by Yu-Wei Wu.
 * @sa
 * vtkTree vtkDataReader
*/

#ifndef vtkNewickTreeReader_h
#define vtkNewickTreeReader_h

#include "vtkIOInfovisModule.h" // For export macro
#include "vtkDataReader.h"

class vtkDoubleArray;
class vtkMutableDirectedGraph;
class vtkStringArray;
class vtkTree;

class VTKIOINFOVIS_EXPORT vtkNewickTreeReader : public vtkDataReader
{
public:
  static vtkNewickTreeReader *New();
  vtkTypeMacro(vtkNewickTreeReader,vtkDataReader);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * Get the output of this reader.
   */
  vtkTree *GetOutput();
  vtkTree *GetOutput(int idx);
  void SetOutput(vtkTree *output);
  int ReadNewickTree(const char * buffer, vtkTree & tree);
  //@}

protected:
  vtkNewickTreeReader();
  ~vtkNewickTreeReader() VTK_OVERRIDE;

  int RequestData(vtkInformation *, vtkInformationVector **,
                          vtkInformationVector *) VTK_OVERRIDE;

  // Since the Outputs[0] has the same UpdateExtent format
  // as the generic DataObject we can copy the UpdateExtent
  // as a default behavior.
  int RequestUpdateExtent(vtkInformation *, vtkInformationVector **,
                                  vtkInformationVector *) VTK_OVERRIDE;

  int FillOutputPortInformation(int, vtkInformation*) VTK_OVERRIDE;
  void CountNodes(const char * buffer, vtkIdType *numNodes);
  vtkIdType BuildTree(char *buffer, vtkMutableDirectedGraph *g,
    vtkDoubleArray *weights, vtkStringArray *names, vtkIdType parent);
private:
  vtkNewickTreeReader(const vtkNewickTreeReader&) VTK_DELETE_FUNCTION;
  void operator=(const vtkNewickTreeReader&) VTK_DELETE_FUNCTION;
};

#endif
