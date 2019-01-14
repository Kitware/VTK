/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPDataSetReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPDataSetReader
 * @brief   Manages reading pieces of a data set.
 *
 * vtkPDataSetReader will read a piece of a file, it takes as input
 * a metadata file that lists all of the files in a data set.
*/

#ifndef vtkPDataSetReader_h
#define vtkPDataSetReader_h

#include "vtkIOParallelModule.h" // For export macro
#include "vtkDataSetAlgorithm.h"

class vtkDataSet;

class VTKIOPARALLEL_EXPORT vtkPDataSetReader : public vtkDataSetAlgorithm
{
public:
  void PrintSelf(ostream& os, vtkIndent indent) override;
  vtkTypeMacro(vtkPDataSetReader,vtkDataSetAlgorithm);
  static vtkPDataSetReader *New();

  //@{
  /**
   * This file to open and read.
   */
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);
  //@}

  //@{
  /**
   * This is set when UpdateInformation is called.
   * It shows the type of the output.
   */
  vtkGetMacro(DataType, int);
  //@}

  /**
   * Called to determine if the file can be read by the reader.
   */
  int CanReadFile(const char* filename);

protected:
  vtkPDataSetReader();
  ~vtkPDataSetReader() override;

  int RequestDataObject(vtkInformation* request,
                                vtkInformationVector** inputVector,
                                vtkInformationVector* outputVector) override;
  void ReadPVTKFileInformation(ifstream *fp,
                               vtkInformation* request,
                               vtkInformationVector** inputVector,
                               vtkInformationVector* outputVector);
  void ReadVTKFileInformation(vtkInformation* request,
    vtkInformationVector** inputVector,
    vtkInformationVector* outputVector);

  int RequestInformation(vtkInformation*,
                                 vtkInformationVector**,
                                 vtkInformationVector*) override;

  int RequestData(vtkInformation*,
                          vtkInformationVector**,
                          vtkInformationVector*) override;
  int PolyDataExecute(vtkInformation*,
                      vtkInformationVector**,
                      vtkInformationVector*);
  int UnstructuredGridExecute(vtkInformation*,
                              vtkInformationVector**,
                              vtkInformationVector*);
  int ImageDataExecute(vtkInformation*,
                       vtkInformationVector**,
                       vtkInformationVector*);
  int StructuredGridExecute(vtkInformation*,
                            vtkInformationVector**,
                            vtkInformationVector*);

  void CoverExtent(int ext[6], int *pieceMask);

  vtkDataSet *CheckOutput();
  void SetNumberOfPieces(int num);

  ifstream *OpenFile(const char *);

  int ReadXML(ifstream *file, char **block, char **param, char **value);
  int VTKFileFlag;
  int StructuredFlag;
  char *FileName;
  int DataType;
  int NumberOfPieces;
  char **PieceFileNames;
  int **PieceExtents;

private:
  vtkPDataSetReader(const vtkPDataSetReader&) = delete;
  void operator=(const vtkPDataSetReader&) = delete;
};

#endif
