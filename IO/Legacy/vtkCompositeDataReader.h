/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCompositeDataReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkCompositeDataReader
 * @brief   read vtkCompositeDataSet data file.
 *
 * @warning
 * This is an experimental format. Use XML-based formats for writing composite
 * datasets. Saving composite dataset in legacy VTK format is expected to change
 * in future including changes to the file layout.
*/

#ifndef vtkCompositeDataReader_h
#define vtkCompositeDataReader_h

#include "vtkIOLegacyModule.h" // For export macro
#include "vtkDataReader.h"

class vtkCompositeDataSet;
class vtkHierarchicalBoxDataSet;
class vtkMultiBlockDataSet;
class vtkMultiPieceDataSet;
class vtkNonOverlappingAMR;
class vtkOverlappingAMR;

class VTKIOLEGACY_EXPORT vtkCompositeDataReader : public vtkDataReader
{
public:
  static vtkCompositeDataReader* New();
  vtkTypeMacro(vtkCompositeDataReader, vtkDataReader);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * Get the output of this reader.
   */
  vtkCompositeDataSet *GetOutput();
  vtkCompositeDataSet *GetOutput(int idx);
  void SetOutput(vtkCompositeDataSet *output);
  //@}

protected:
  vtkCompositeDataReader();
  ~vtkCompositeDataReader() VTK_OVERRIDE;

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

  int FillOutputPortInformation(int, vtkInformation*) VTK_OVERRIDE;

  /**
   * Read the output type information.
   */
  int ReadOutputType();

  bool ReadCompositeData(vtkMultiPieceDataSet*);
  bool ReadCompositeData(vtkMultiBlockDataSet*);
  bool ReadCompositeData(vtkHierarchicalBoxDataSet*);
  bool ReadCompositeData(vtkOverlappingAMR*);
  bool ReadCompositeData(vtkNonOverlappingAMR*);
  vtkDataObject* ReadChild();

private:
  vtkCompositeDataReader(const vtkCompositeDataReader&) VTK_DELETE_FUNCTION;
  void operator=(const vtkCompositeDataReader&) VTK_DELETE_FUNCTION;

};

#endif
