/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCellArray.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1996 Ken Martin, Will Schroeder, Bill Lorensen.

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


=========================================================================*/
#include "vtkCellArray.h"

vtkCellArray::vtkCellArray()
{
  this->Ia = vtkIntArray::New();
  this->NumberOfCells = 0;
  this->Location = 0;
}

vtkCellArray::vtkCellArray(const int sz, const int ext)
{
  this->Ia = new vtkIntArray(sz,ext);
  this->NumberOfCells = 0;
  this->Location = 0;
}

vtkCellArray::vtkCellArray (const vtkCellArray& ca)
{
  this->Ia = vtkIntArray::New();
  this->NumberOfCells = ca.NumberOfCells;
  this->Location = 0;
  *(this->Ia) = *(ca.Ia);
}

vtkCellArray::~vtkCellArray()
{
  this->Ia->Delete();
}


// Description:
// Returns the size of the largest cell. The size is the number of points
// defining the cell.
int vtkCellArray::GetMaxCellSize()
{
  int i, npts=0, maxSize=0;

  for (i=0; i<this->Ia->GetMaxId(); i+=(npts+1))
    {
    if ( (npts=this->Ia->GetValue(i)) > maxSize )
      maxSize = npts;
    }
  return maxSize;
}
