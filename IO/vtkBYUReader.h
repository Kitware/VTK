/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBYUReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkBYUReader - read MOVIE.BYU polygon files
// .SECTION Description
// vtkBYUReader is a source object that reads MOVIE.BYU polygon files.
// These files consist of a geometry file (.g), a scalar file (.s), a 
// displacement or vector file (.d), and a 2D texture coordinate file
// (.t).

#ifndef __vtkBYUReader_h
#define __vtkBYUReader_h

#include "vtkPolyDataAlgorithm.h"

class VTK_IO_EXPORT vtkBYUReader : public vtkPolyDataAlgorithm 
{
public:
  static vtkBYUReader *New();

  vtkTypeMacro(vtkBYUReader,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify name of geometry FileName.
  vtkSetStringMacro(GeometryFileName);
  vtkGetStringMacro(GeometryFileName);

  // Description:
  // Specify name of geometry FileName (alias).
  virtual void SetFileName(const char* f) { this->SetGeometryFileName(f); }
  virtual char* GetFileName() { return this->GetGeometryFileName(); }

  // Description:
  // Specify name of displacement FileName.
  vtkSetStringMacro(DisplacementFileName);
  vtkGetStringMacro(DisplacementFileName);

  // Description:
  // Specify name of scalar FileName.
  vtkSetStringMacro(ScalarFileName);
  vtkGetStringMacro(ScalarFileName);

  // Description:
  // Specify name of texture coordinates FileName.
  vtkSetStringMacro(TextureFileName);
  vtkGetStringMacro(TextureFileName);

  // Description:
  // Turn on/off the reading of the displacement file.
  vtkSetMacro(ReadDisplacement,int);
  vtkGetMacro(ReadDisplacement,int);
  vtkBooleanMacro(ReadDisplacement,int);
  
  // Description:
  // Turn on/off the reading of the scalar file.
  vtkSetMacro(ReadScalar,int);
  vtkGetMacro(ReadScalar,int);
  vtkBooleanMacro(ReadScalar,int);
  
  // Description:
  // Turn on/off the reading of the texture coordinate file.
  // Specify name of geometry FileName.
  vtkSetMacro(ReadTexture,int);
  vtkGetMacro(ReadTexture,int);
  vtkBooleanMacro(ReadTexture,int);

  // Description:
  // Set/Get the part number to be read.
  vtkSetClampMacro(PartNumber,int,1,VTK_LARGE_INTEGER);
  vtkGetMacro(PartNumber,int);

  // Description:
  // Returns 1 if this file can be read and 0 if the file cannot be read.
  // Because BYU files do not have anything in the header specifying the file
  // type, the result is not definitive.  Invalid files may still return 1
  // although a valid file will never return 0.
  static int CanReadFile(const char *filename);

protected:
  vtkBYUReader();
  ~vtkBYUReader();

  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  // This source does not know how to generate pieces yet.
  int ComputeDivisionExtents(vtkDataObject *output, 
                             int idx, int numDivisions);

  char *GeometryFileName;
  char *DisplacementFileName;
  char *ScalarFileName;
  char *TextureFileName;
  int ReadDisplacement;
  int ReadScalar;
  int ReadTexture;
  int PartNumber;

  void ReadGeometryFile(FILE *fp, int &numPts, vtkInformation *outInfo);
  void ReadDisplacementFile(int numPts, vtkInformation *outInfo);
  void ReadScalarFile(int numPts, vtkInformation *outInfo);
  void ReadTextureFile(int numPts, vtkInformation *outInfo);
private:
  vtkBYUReader(const vtkBYUReader&);  // Not implemented.
  void operator=(const vtkBYUReader&);  // Not implemented.
};

#endif
