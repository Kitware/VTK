/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAMRBox.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkAMRBox - represents a 3D uniform region in space
// .SECTION Description
// vtkAMRBox is similar to Chombo's Box. It represents a 3D
// region by storing indices for two corners (LoCorner, HiCorner).
// A few utility methods are provided.

#ifndef __vtkAMRBox_h
#define __vtkAMRBox_h

#include "vtkObject.h"

template<int dimension>
struct vtkAMRBoxInitializeHelp;

// Helper to unroll the loop
template<int dimension>
void vtkAMRBoxInitialize(int *LoCorner, int *HiCorner, // member
                         const int *loCorner, const int *hiCorner, // local
                         vtkAMRBoxInitializeHelp<dimension>* = 0) // dummy parameter for vs6
  {
  for(int i=0; i<dimension; ++i)
    {
    LoCorner[i] = loCorner[i];
    HiCorner[i] = hiCorner[i];
    }
  for(int i=dimension; i<(3-dimension); ++i)
    {
    LoCorner[i] = 0;
    HiCorner[i] = 0;
    }
  }

class VTK_FILTERING_EXPORT vtkAMRBox
{
public:
  // public for quick access
  // Description:
  // cell position of the lower cell position and higher cell positon.
  // Eg. LoCorner = {0,0,0}, HiCorner = {0,0,0} is an AMRBox with 1 cell
  int LoCorner[3];
  int HiCorner[3];

  vtkAMRBox()
    {
    vtkAMRBoxInitialize<0>(this->LoCorner, this->HiCorner, 0, 0);
    }

  // \precondition dimensionality >= 2 && dimensionality <= 3
  vtkAMRBox(int dimensionality, const int* loCorner, const int* hiCorner)
    {
    switch(dimensionality)
      {
    case 2:
      vtkAMRBoxInitialize<2>(this->LoCorner, this->HiCorner,
                             loCorner, hiCorner);
      break;
    case 3:
      vtkAMRBoxInitialize<3>(this->LoCorner, this->HiCorner,
                             loCorner, hiCorner);
      break;
    default:
      vtkGenericWarningMacro( "Wrong dimensionality" );
      }
    }

  // Description:
  // Returns the number of cells (aka elements, zones etc.) in
  // the given region (for the specified refinement, see Coarsen()
  // and Refine() ).
  vtkIdType GetNumberOfCells()
    {
    vtkIdType numCells=1;
    for(int i=0; i<3; i++)
      {
      numCells *= HiCorner[i] - LoCorner[i] + 1;
      }
    return numCells;
    }

  // Description:
  // Modify LoCorner and HiCorner by coarsening with the given
  // refinement ratio.
  void Coarsen(int refinement)
    {
    for (int i=0; i<3; i++)
      {
      this->LoCorner[i] =
        ( this->LoCorner[i] < 0 ?
          -abs(this->LoCorner[i]+1)/refinement - 1 :
          this->LoCorner[i]/refinement );
      this->HiCorner[i] =
        ( this->HiCorner[i] < 0 ?
          -abs(this->HiCorner[i]+1)/refinement - 1 :
          this->HiCorner[i]/refinement );
      }
    }

  // Description:
  // Modify LoCorner and HiCorner by refining with the given
  // refinement ratio.
  void Refine(int refinement)
    {
    for (int i=0; i<3; i++)
      {
      this->LoCorner[i] = (this->LoCorner[i]+1)*refinement-1;
      this->HiCorner[i] = (this->HiCorner[i]+1)*refinement-1;
      }
    }

  // Description:
  // Returns non-zero if the box contains the cell with
  // given indices.
  int DoesContainCell(int i, int j, int k)
    {
    return
      i >= this->LoCorner[0] && i <= this->HiCorner[0] &&
      j >= this->LoCorner[1] && j <= this->HiCorner[1] &&
      k >= this->LoCorner[2] && k <= this->HiCorner[2];
    }

  // Description:
  // Returns non-zero if the box contains `box`
  int DoesContainBox(vtkAMRBox const & box) const
    {
    // return DoesContainCell(box.LoCorner) && DoesContainCell(box.HiCorner);
    return box.LoCorner[0] >= this->LoCorner[0]
        && box.LoCorner[1] >= this->LoCorner[1]
        && box.LoCorner[2] >= this->LoCorner[2]
        && box.HiCorner[0] <= this->HiCorner[0]
        && box.HiCorner[1] <= this->HiCorner[1]
        && box.HiCorner[2] <= this->HiCorner[2];
    }

  // Description:
  // Print LoCorner and HiCorner
  void Print(ostream &os)
    {
    os << "LoCorner: " << this->LoCorner[0] << "," << this->LoCorner[1]
      << "," << this->LoCorner[2] << "\n";
    os << "HiCorner: " << this->HiCorner[0] << "," << this->HiCorner[1]
      << "," << this->HiCorner[2] << "\n";
    }

  // Description:
  // Check if cell position is HiCorner
  bool IsHiCorner(const int pos[3]) const
    {
    return this->HiCorner[0] == pos[0]
        && this->HiCorner[1] == pos[1]
        && this->HiCorner[2] == pos[2];
    }

  // Description:
  // Check if cell position is LoCorner
  bool IsLoCorner(const int pos[3]) const
    {
    return this->LoCorner[0] == pos[0]
        && this->LoCorner[1] == pos[1]
        && this->LoCorner[2] == pos[2];
    }
};

#endif

