/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataR.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1995 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.


=========================================================================*/
// .NAME vtkDataReader - helper class for objects that read vtk data files
// .SECTION Description
// vtkDataReader is a helper class that reads the vtk data file header and 
// point data (e.g., scalars, vectors, normals, etc) from a vtk data file. 
// See text for format.

#ifndef __vtkDataReader_hh
#define __vtkDataReader_hh

#include <stdio.h>
#include "Object.hh"
#include "PointSet.hh"

#define ASCII 1
#define BINARY 2


class vtkDataReader : public vtkObject
{
public:
  vtkDataReader();
  ~vtkDataReader();
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify file name of vtk data file to read.
  vtkSetStringMacro(Filename);
  vtkGetStringMacro(Filename);

  // Description:
  // Get the type of file (ASCII or BINARY)
  vtkGetMacro(FileType,int);

  // Description:
  // Set the name of the scalar data to extract. If not specified, first 
  // scalar data encountered is extracted.
  vtkSetStringMacro(ScalarsName);
  vtkGetStringMacro(ScalarsName);

  // Description:
  // Set the name of the vector data to extract. If not specified, first 
  // vector data encountered is extracted.
  vtkSetStringMacro(VectorsName);
  vtkGetStringMacro(VectorsName);

  // Description:
  // Set the name of the tensor data to extract. If not specified, first 
  // tensor data encountered is extracted.
  vtkSetStringMacro(TensorsName);
  vtkGetStringMacro(TensorsName);

  // Description:
  // Set the name of the normal data to extract. If not specified, first 
  // normal data encountered is extracted.
  vtkSetStringMacro(NormalsName);
  vtkGetStringMacro(NormalsName);

  // Description:
  // Set the name of the texture coordinate data to extract. If not specified,
  // first texture coordinate data encountered is extracted.
  vtkSetStringMacro(TCoordsName);
  vtkGetStringMacro(TCoordsName);

  // Description:
  // Set the name of the lookup table data to extract. If not specified, uses 
  // lookup table named by scalar. Otherwise, this specification supersedes.
  vtkSetStringMacro(LookupTableName);
  vtkGetStringMacro(LookupTableName);

  // Special methods
  char *LowerCase(char *);
  FILE *OpenVTKFile();
  int ReadHeader(FILE *fp);
  int ReadPointData(FILE *fp, vtkDataSet *ds, int numPts);
  int ReadPoints(FILE *fp, vtkPointSet *ps, int numPts);
  int ReadCells(FILE *fp, int size, int *data);
  void CloseVTKFile(FILE *fp);

protected:
  char *Filename;
  int FileType;

  char *ScalarsName;
  char *VectorsName;
  char *TensorsName;
  char *TCoordsName;
  char *NormalsName;
  char *LookupTableName;
  char *ScalarLut;

  vtkSetStringMacro(ScalarLut);
  vtkGetStringMacro(ScalarLut);

  int ReadScalarData(FILE *fp, vtkDataSet *ds, int numPts);
  int ReadVectorData(FILE *fp, vtkDataSet *ds, int numPts);
  int ReadNormalData(FILE *fp, vtkDataSet *ds, int numPts);
  int ReadTensorData(FILE *fp, vtkDataSet *ds, int numPts);
  int ReadCoScalarData(FILE *fp, vtkDataSet *ds, int numPts);
  int ReadLutData(FILE *fp, vtkDataSet *ds, int numPts);
  int ReadTCoordsData(FILE *fp, vtkDataSet *ds, int numPts);
};

#endif


