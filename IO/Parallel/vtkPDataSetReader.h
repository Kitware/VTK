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
// .NAME vtkPDataSetReader - Manages reading pieces of a data set.
// .SECTION Description
// vtkPDataSetReader will read a piece of a file, it takes as input
// a metadata file that lists all of the files in a data set.


#ifndef vtkPDataSetReader_h
#define vtkPDataSetReader_h

#include "vtkIOParallelModule.h" // For export macro
#include "vtkDataSetAlgorithm.h"

class vtkDataSet;

class VTKIOPARALLEL_EXPORT vtkPDataSetReader : public vtkDataSetAlgorithm
{
public:
  void PrintSelf(ostream& os, vtkIndent indent);
  vtkTypeMacro(vtkPDataSetReader,vtkDataSetAlgorithm);
  static vtkPDataSetReader *New();

  // Description:
  // This file to open and read.
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);

  // Description:
  // This is set when UpdateInformation is called.
  // It shows the type of the output.
  vtkGetMacro(DataType, int);

  // Description:
  // Called to determine if the file can be read by the reader.
  int CanReadFile(const char* filename);

protected:
  vtkPDataSetReader();
  ~vtkPDataSetReader();

  virtual int RequestDataObject(vtkInformation* request,
                                vtkInformationVector** inputVector,
                                vtkInformationVector* outputVector);
  void ReadPVTKFileInformation(ifstream *fp,
                               vtkInformation* request,
                               vtkInformationVector** inputVector,
                               vtkInformationVector* outputVector);
  void ReadVTKFileInformation(ifstream *fp,
                               vtkInformation* request,
                               vtkInformationVector** inputVector,
                               vtkInformationVector* outputVector);

  virtual int RequestInformation(vtkInformation*,
                                 vtkInformationVector**,
                                 vtkInformationVector*);

  virtual int RequestData(vtkInformation*,
                          vtkInformationVector**,
                          vtkInformationVector*);
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

//BTX
  ifstream *OpenFile(const char *);
//ETX
  int ReadXML(ifstream *file, char **block, char **param, char **value);
  void SkipFieldData(ifstream *file);

  int VTKFileFlag;
  int StructuredFlag;
  char *FileName;
  int DataType;
  int NumberOfPieces;
  char **PieceFileNames;
  int **PieceExtents;

private:
  vtkPDataSetReader(const vtkPDataSetReader&); // Not implemented
  void operator=(const vtkPDataSetReader&); // Not implemented
};

#endif
