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
/**
 * @class   vtkGAMBITReader
 * @brief   reads a dataset in Fluent GAMBIT neutral file format
 *
 * vtkGAMBITReader creates an unstructured grid dataset. It reads ASCII files
 * stored in GAMBIT neutral format, with optional data stored at the nodes or
 * at the cells of the model. A cell-based fielddata stores the material id.
 *
 * @par Thanks:
 * Thanks to Jean M. Favre (CSCS, Swiss Center for Scientific Computing) who
 * developed this class.
 * Please address all comments to Jean Favre (jfavre at cscs.ch)
 *
 * @sa
 * vtkAVSucdReader
*/

#ifndef vtkGAMBITReader_h
#define vtkGAMBITReader_h

#include "vtkIOGeometryModule.h" // For export macro
#include "vtkUnstructuredGridAlgorithm.h"

class vtkDoubleArray;
class VTKIOGEOMETRY_EXPORT vtkGAMBITReader : public vtkUnstructuredGridAlgorithm
{
public:
  static vtkGAMBITReader *New();
  vtkTypeMacro(vtkGAMBITReader,vtkUnstructuredGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * Specify the file name of the GAMBIT data file to read.
   */
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);
  //@}

  //@{
  /**
   * Get the total number of cells. The number of cells is only valid after a
   * successful read of the data file is performed.
   */
  vtkGetMacro(NumberOfCells,int);
  //@}

  //@{
  /**
   * Get the total number of nodes. The number of nodes is only valid after a
   * successful read of the data file is performed.
   */
  vtkGetMacro(NumberOfNodes,int);
  //@}

  //@{
  /**
   * Get the number of data components at the nodes and cells.
   */
  vtkGetMacro(NumberOfNodeFields,int);
  vtkGetMacro(NumberOfCellFields,int);
  //@}

protected:
  vtkGAMBITReader();
  ~vtkGAMBITReader() VTK_OVERRIDE;
  int RequestInformation(vtkInformation *, vtkInformationVector **, vtkInformationVector *) VTK_OVERRIDE;
  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *) VTK_OVERRIDE;

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

private:
  void ReadFile(vtkUnstructuredGrid *output);
  void ReadGeometry(vtkUnstructuredGrid *output);
  void ReadNodeData(vtkUnstructuredGrid *output);
  void ReadCellData(vtkUnstructuredGrid *output);

  void ReadXYZCoords(vtkDoubleArray *coords);

  void ReadCellConnectivity(vtkUnstructuredGrid *output);
  void ReadMaterialTypes(vtkUnstructuredGrid *output);
  void ReadBoundaryConditionSets(vtkUnstructuredGrid *output);

  vtkGAMBITReader(const vtkGAMBITReader&) VTK_DELETE_FUNCTION;
  void operator=(const vtkGAMBITReader&) VTK_DELETE_FUNCTION;
};

#endif
