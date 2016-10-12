/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBiomTableReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkBiomTableReader
 * @brief   read vtkTable from a .biom input file
 *
 * vtkBiomTableReader is a source object that reads ASCII biom data files.
 * The output of this reader is a single vtkTable data object.
 * @sa
 * vtkTable vtkTableReader vtkDataReader
*/

#ifndef vtkBiomTableReader_h
#define vtkBiomTableReader_h

#include "vtkIOInfovisModule.h" // For export macro
#include "vtkTableReader.h"

class vtkTable;
class vtkVariant;

class VTKIOINFOVIS_EXPORT vtkBiomTableReader : public vtkTableReader
{
public:
  static vtkBiomTableReader *New();
  vtkTypeMacro(vtkBiomTableReader,vtkTableReader);
  void PrintSelf(ostream& os, vtkIndent indent);

  //@{
  /**
   * Get the output of this reader.
   */
  vtkTable *GetOutput();
  vtkTable *GetOutput(int idx);
  void SetOutput(vtkTable *output);
  //@}

protected:
  vtkBiomTableReader();
  ~vtkBiomTableReader();

  virtual int RequestData(vtkInformation *, vtkInformationVector **,
                          vtkInformationVector *);

  // Since the Outputs[0] has the same UpdateExtent format
  // as the generic DataObject we can copy the UpdateExtent
  // as a default behavior.
  virtual int RequestUpdateExtent(vtkInformation *, vtkInformationVector **,
                                  vtkInformationVector *);

  virtual int FillOutputPortInformation(int, vtkInformation*);
  void ParseShape();
  void ParseDataType();
  void ParseSparseness();
  void InitializeData();
  void FillData(vtkVariant v);
  void ParseSparseData();
  void ParseDenseData();
  void InsertValue(int row, int col, std::string value);
  void ParseId();
  void ParseColumns();
  void ParseRows();

private:
  std::string FileContents;
  int NumberOfRows;
  int NumberOfColumns;
  int DataType;
  bool Sparse;
  vtkBiomTableReader(const vtkBiomTableReader&) VTK_DELETE_FUNCTION;
  void operator=(const vtkBiomTableReader&) VTK_DELETE_FUNCTION;
};

#endif
