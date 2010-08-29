/*=========================================================================

  Program: Visualization Toolkit
  Module: vtkProStarReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE. See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkProStarReader - Reads geometry in proSTAR (STARCD) file format.
// .SECTION Description
// vtkProStarReader creates an unstructured grid dataset.
// It reads .cel/.vrt files stored in proSTAR (STARCD) ASCII format.
//
// .SECTION Thanks
// Reader written by Mark Olesen
//

#ifndef __vtkProStarReader_h
#define __vtkProStarReader_h

#include "vtkUnstructuredGridAlgorithm.h"

class VTK_IO_EXPORT vtkProStarReader : public vtkUnstructuredGridAlgorithm
{
public:
  static vtkProStarReader *New();
  vtkTypeMacro(vtkProStarReader,vtkUnstructuredGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify the file name prefix of the cel/vrt files to read.
  // The reader will try to open FileName.cel and FileName.vrt files.
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);

  // Description:
  // The proSTAR files are often in millimeters.
  // Specify an alternative scaling factor.
  vtkSetClampMacro(ScaleFactor, double, 0, VTK_DOUBLE_MAX);
  vtkGetMacro(ScaleFactor, double);

  // Description:
  // The type of material represented by the cell
  enum cellType
  {
    starcdFluidType = 1,
    starcdSolidType = 2,
    starcdBaffleType = 3,
    starcdShellType = 4,
    starcdLineType = 5,
    starcdPointType = 6
  };

  // Description:
  // The primitive cell shape
  enum shapeType
  {
    starcdPoint = 1,
    starcdLine = 2,
    starcdShell = 3,
    starcdHex = 11,
    starcdPrism = 12,
    starcdTet = 13,
    starcdPyr = 14,
    starcdPoly = 255
  };

protected:
  vtkProStarReader();
  ~vtkProStarReader();

  int RequestInformation
    (vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  int RequestData
    (vtkInformation *, vtkInformationVector **, vtkInformationVector *);

  // Description:
  // The name of the file to be read.  If it has a .cel, .vrt, or .inp
  // extension it will be truncated and later appended when reading
  // the appropriate files.  Otherwise those extensions will be appended
  // to FileName when opening the files.
  char *FileName;

  // Description:
  // The coordinates are multiplied by ScaleFactor when setting them.
  // The default value is 1.
  double ScaleFactor;

private:
  //
  // Internal Classes/Structures
  //
  struct idMapping;

  FILE* OpenFile(const char *ext);

  bool ReadVrtFile(vtkUnstructuredGrid *output, idMapping& pointMapping);
  bool ReadCelFile(vtkUnstructuredGrid *output, const idMapping& pointMapping);

  vtkProStarReader(const vtkProStarReader&); // Not implemented.
  void operator=(const vtkProStarReader&); // Not implemented.
};
#endif
