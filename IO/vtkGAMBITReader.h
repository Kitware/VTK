/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGAMBITReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkGAMBITReader - reads a dataset in Fluent GAMBIT Neutral File format
// .SECTION Description
// vtkGAMBITReader creates an unstructured grid dataset. It reads ASCII
// files stored in GAMBIT neutral format, with optional data stored at the nodes
// or at the cells of the model. A cell-based fielddata stores the material
// id.

// .SECTION Thanks
// Thanks to Jean M. Favre (CSCS, Swiss Center for Scientific Computing) who 
// developed this class.
// Please address all comments to Jean Favre (jfavre at cscs.ch)

// .SECTION See Also
// vtkAVSucdReader

#ifndef __vtkGAMBITReader_h
#define __vtkGAMBITReader_h

#include "vtkUnstructuredGridSource.h"

class vtkDoubleArray;
class VTK_IO_EXPORT vtkGAMBITReader : public vtkUnstructuredGridSource
{
public:
  static vtkGAMBITReader *New();
  vtkTypeRevisionMacro(vtkGAMBITReader,vtkUnstructuredGridSource);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify file name of GAMBITReader datafile to read
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);

  // Description:
  // Get the total number of cells.
  vtkGetMacro(NumberOfCells,int);

  // Description:
  // Get the total number of nodes.
  vtkGetMacro(NumberOfNodes,int);

  // Description:
  // Get the number of data components at the nodes and cells.
  vtkGetMacro(NumberOfNodeFields,int);
  vtkGetMacro(NumberOfCellFields,int);

protected:
  vtkGAMBITReader();
  ~vtkGAMBITReader();
  void ExecuteInformation();
  void Execute();

  char *FileName;

  int NumberOfNodes;
  int NumberOfCells;
  int NumberOfNodeFields;
  int NumberOfCellFields;
  int NumberOfElementGroups;
  int NumberOfBoundaryConditionSets;
  int NumberOfCoordinateDirections;
  int NumberOfVelocityComponents;
  ifstream *FileStream;

  //BTX
  enum GAMBITCellType
  {
    EDGE    = 1,
    QUAD    = 2,
    TRI     = 3,
    BRICK   = 4,
    PRISM   = 5,
    TETRA   = 6,
    PYRAMID = 7
  };
  //ETX

private:
  void ReadFile();
  void ReadGeometry();
  void ReadNodeData();
  void ReadCellData();

  void ReadXYZCoords(vtkDoubleArray *coords);

  void ReadCellConnectivity();
  void ReadMaterialTypes();
  void ReadBoundaryConditionSets();

  vtkGAMBITReader(const vtkGAMBITReader&);  // Not implemented.
  void operator=(const vtkGAMBITReader&);  // Not implemented.
};

#endif
