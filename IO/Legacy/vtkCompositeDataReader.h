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
class vtkPartitionedDataSet;
class vtkPartitionedDataSetCollection;

class VTKIOLEGACY_EXPORT vtkCompositeDataReader : public vtkDataReader
{
public:
  static vtkCompositeDataReader* New();
  vtkTypeMacro(vtkCompositeDataReader, vtkDataReader);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Get the output of this reader.
   */
  vtkCompositeDataSet *GetOutput();
  vtkCompositeDataSet *GetOutput(int idx);
  void SetOutput(vtkCompositeDataSet *output);
  //@}

  /**
   * Actual reading happens here
   */
  int ReadMeshSimple(const std::string& fname,
                     vtkDataObject* output) override;

protected:
  vtkCompositeDataReader();
  ~vtkCompositeDataReader() override;

  vtkDataObject* CreateOutput(vtkDataObject* currentOutput) override;

  int FillOutputPortInformation(int, vtkInformation*) override;

  /**
   * Read the output type information.
   */
  int ReadOutputType();

  bool ReadCompositeData(vtkMultiPieceDataSet*);
  bool ReadCompositeData(vtkMultiBlockDataSet*);
  bool ReadCompositeData(vtkHierarchicalBoxDataSet*);
  bool ReadCompositeData(vtkOverlappingAMR*);
  bool ReadCompositeData(vtkPartitionedDataSet*);
  bool ReadCompositeData(vtkPartitionedDataSetCollection*);
  bool ReadCompositeData(vtkNonOverlappingAMR*);
  vtkDataObject* ReadChild();

private:
  vtkCompositeDataReader(const vtkCompositeDataReader&) = delete;
  void operator=(const vtkCompositeDataReader&) = delete;

};

#endif
