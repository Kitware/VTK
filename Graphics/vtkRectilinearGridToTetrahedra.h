/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRectilinearGridToTetrahedra.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkRectilinearGridToTetrahedra - create a Tetrahedral mesh from a RectilinearGrid
// .SECTION Description
// vtkRectilinearGridToTetrahedra forms a mesh of Tetrahedra from a
// vtkRectilinearGrid.  The tetrahedra can be 5 per cell, 6 per cell,
// or a mixture of 5 or 12 per cell. The resulting mesh is consistent,
// meaning that there are no edge crossings and that each tetrahedron
// face is shared by two tetrahedra, except those tetrahedra on the
// boundary. All tetrahedra are right handed.
//
// Note that 12 tetrahedra per cell means adding a point in the 
// center of the cell.
//
// In order to subdivide some cells into 5 and some cells into 12 tetrahedra:
// SetTetraPerCellTo5And12();
// Set the Scalars of the Input RectilinearGrid to be 5 or 12
// depending on what you want per cell of the RectilinearGrid.
//
// If you set RememberVoxelId, the scalars of the tetrahedron
// will be set to the Id of the Cell in the RectilinearGrid from which
// the tetrahedron came.
//
// .SECTION Thanks
//    This class was developed by Samson J. Timoner of the 
//    MIT Artificial Intelligence Laboratory
//
// .SECTION See Also
//    vtkDelaunay3D

#ifndef __vtkRectilinearGridToTetrahedra_h
#define __vtkRectilinearGridToTetrahedra_h

// ways to create the mesh from voxels
#define VTK_VOXEL_TO_12_TET      12
#define VTK_VOXEL_TO_5_TET        5
#define VTK_VOXEL_TO_6_TET        6
#define VTK_VOXEL_TO_5_AND_12_TET -1

#include "vtkUnstructuredGridSource.h"
class vtkRectilinearGrid;
class vtkCharArray;
class vtkIdList;
class vtkCellArray;
class vtkPoints;

class VTK_GRAPHICS_EXPORT vtkRectilinearGridToTetrahedra : public vtkUnstructuredGridSource
{
public:
  vtkTypeRevisionMacro(vtkRectilinearGridToTetrahedra,vtkUnstructuredGridSource);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Form 5 Tetrahedra per cube. Do not RememberVoxelId.
  static vtkRectilinearGridToTetrahedra *New();

  // Description:
  // Set the method to divide each cell (voxel) in the RectilinearGrid
  // into tetrahedra.
  void SetTetraPerCellTo5()      {SetTetraPerCell(VTK_VOXEL_TO_5_TET);};
  void SetTetraPerCellTo6()      {SetTetraPerCell(VTK_VOXEL_TO_6_TET);};
  void SetTetraPerCellTo12()     {SetTetraPerCell(VTK_VOXEL_TO_12_TET);};
  void SetTetraPerCellTo5And12() {SetTetraPerCell(VTK_VOXEL_TO_5_AND_12_TET);};
  vtkSetMacro(TetraPerCell,int);
  vtkGetMacro(TetraPerCell,int);

  // Description:
  // Should the tetrahedra have scalar data
  // indicating which Voxel they came from in the vtkRectilinearGrid?
  vtkSetMacro(RememberVoxelId,int);
  vtkGetMacro(RememberVoxelId,int);
  vtkBooleanMacro(RememberVoxelId,int);

  // Description:
  // This function for convenience for creating a Rectilinear Grid
  // If Spacing does not fit evenly into extent, the last cell will
  // have a different width (or height or depth).
  // If Extent[i]/Spacing[i] is within tol of an integer, then
  // assume the programmer meant an integer for direction i.
  void SetInput(const float Extent[3], const float Spacing[3],
                const float tol=0.001);
  // Description:
  // This version of the function for the wrappers
  void SetInput(const float ExtentX, 
                const float ExtentY,
                const float ExtentZ, 
                const float SpacingX,
                const float SpacingY,
                const float SpacingZ,
                const float tol=0.001);

  // Description:
  // Set / get the input data or filter.
  // If TetraPerCell is VTK_VOXEL_TO_5_and_12_TET
  // then the input must have scalars, each set to 5 or 12
  // to specify how to subdivide each cell.
  void SetInput(vtkRectilinearGrid *input);
  vtkRectilinearGrid *GetInput();

protected:
  vtkRectilinearGridToTetrahedra();
  ~vtkRectilinearGridToTetrahedra() {};

  void Execute();

  int RememberVoxelId;
  int TetraPerCell;

 private:
  vtkRectilinearGridToTetrahedra(const vtkRectilinearGridToTetrahedra&); // Not implemented.

  void operator=(const vtkRectilinearGridToTetrahedra&); // Not implemented.

//BTX
  // Description:
  // Determine how to Divide each cell (voxel) in the RectilinearGrid
  // Overwrites VoxelSubdivisionType with flipping information for forming the mesh
  static void DetermineGridDivisionTypes(vtkRectilinearGrid *RectGrid, 
                                         vtkCharArray *VoxelSubdivisionType,
                                         const int &TetraPerCell);

  // Description:
  // Take the grid and make it into a tetrahedral mesh.
  static void GridToTetMesh(vtkRectilinearGrid *RectGrid, 
                            vtkCharArray *VoxelSubdivisionType,
                            const int &TetraPerCell,
                            const int &RememberVoxelId,
                            vtkUnstructuredGrid *TetMesh);

  // Description:
  // Take a voxel and make tetrahedra out of it.
  // Add the resulting tetrahedra to TetraMesh. Also, should new
  // points need to be created, add them to NodeList.
  // Note that vtkIdList may be changed during this process (a point added).
  static int TetrahedralizeVoxel(vtkIdList *VoxelCorners,
                                 const int &DivisionType,
                                 vtkPoints *NodeList,
                                 vtkCellArray *TetList);

  // Description:
  // Helper Function for TetrahedraizeVoxel
  // Adds a center point in the middle of the voxel
  static inline void TetrahedralizeAddCenterPoint(vtkIdList *VoxelCorners,
                                                  vtkPoints *NodeList);
//ETX
};

#endif /* __vtkRectilinearGridToTetrahedra_h */

