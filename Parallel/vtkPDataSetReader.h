/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPDataSetReader.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPDataSetReader - Manages writing pieces of a data set.
// .SECTION Description
// vtkPDataSetReader will write a piece of a file, and will also create
// a metadata file that lists all of the files in a data set.


#ifndef __vtkPDataSetReader_h
#define __vtkPDataSetReader_h

#include "vtkSource.h"

class vtkDataSet;

class VTK_PARALLEL_EXPORT vtkPDataSetReader : public vtkSource
{
public:
  void PrintSelf(ostream& os, vtkIndent indent);
  vtkTypeRevisionMacro(vtkPDataSetReader,vtkSource);
  static vtkPDataSetReader *New();
  
  // Description:
  // This file to open and read.
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);

  // Description:
  // The output of this reader depends on the file choosen.
  // You cannot get the output until the filename is set.
  void SetOutput(vtkDataSet *output);
  virtual vtkDataSet *GetOutput();

  // Description:
  // We need to define this so that the output gets created.
  virtual void Update();

  // Description:
  // This is set when UpdateInformation is called. 
  // It shows the type of the output.
  vtkGetMacro(DataType, int);
  
  // Description:
  // This method can be used to find out the type of output expected without
  // needing to read the whole file.
  virtual int ReadOutputType();

protected:
  vtkPDataSetReader();
  ~vtkPDataSetReader();
  vtkPDataSetReader(const vtkPDataSetReader&);
  void operator=(const vtkPDataSetReader&);

  virtual void ExecuteInformation();
  void ReadPVTKFileInformation(ifstream *fp);
  void ReadVTKFileInformation(ifstream *fp);

  virtual void Execute();
  void PolyDataExecute();
  void UnstructuredGridExecute();
  void ImageDataExecute();
  void StructuredGridExecute();

  void CoverExtent(int ext[6], int *pieceMask);

  vtkDataSet *CheckOutput();
  void SetNumberOfPieces(int num);

//BTX
  ifstream *OpenFile();
//ETX
  int ReadXML(ifstream *file, char **block, char **param, char **value);

  int VTKFileFlag;
  int StructuredFlag;
  char *FileName;
  int DataType;
  int NumberOfPieces;
  char **PieceFileNames;
  int **PieceExtents;
};

#endif
