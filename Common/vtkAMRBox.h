/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAMRBox.h
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
// .NAME vtkAMRBox - represents a 3D uniform region in space
// .SECTION Description
// vtkAMRBox is similar to Chombo's Box. It represents a 3D
// region by storing indices for two corners (LoCorner, HiCorner).
// A few utility methods are provided.

#ifndef __vtkAMRBox_h
#define __vtkAMRBox_h

#include "vtkObject.h"

class VTK_COMMON_EXPORT vtkAMRBox
{
public:
  // public for quick access
  int LoCorner[3];
  int HiCorner[3];

  vtkAMRBox() 
    {
      for(int i=0; i<3; i++)
        {
        this->LoCorner[i] = this->HiCorner[i] = 0;
        }
    }

  vtkAMRBox(int dimensionality, int* loCorner, int* hiCorner) 
    {
      this->LoCorner[2] = this->HiCorner[2] = 0;
      memcpy(this->LoCorner, loCorner, dimensionality*sizeof(int));
      memcpy(this->HiCorner, hiCorner, dimensionality*sizeof(int));
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
        this->LoCorner[i] = this->LoCorner[i]*refinement;
        this->HiCorner[i] = this->HiCorner[i]*refinement;
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

};

struct vtkAMRLevelInformation
{
  unsigned int Level;
  unsigned int DataSetId;
  vtkAMRBox Box;
};


#endif






