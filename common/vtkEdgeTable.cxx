/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkEdgeTable.cxx
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
#include "vtkEdgeTable.hh"

// Description:
// Instantiate object based on maximum point id.
vtkEdgeTable::vtkEdgeTable(int numPoints)
{
  if ( numPoints < 1 ) numPoints = 1;

  this->Table = new vtkIdList *[numPoints];
  for (int i=0; i < numPoints; i++) this->Table[i] = NULL;
  this->TableSize = numPoints;

  this->Position[0] = 0;
  this->Position[1] = -1;
}

vtkEdgeTable::~vtkEdgeTable()
{
  if ( this->Table )
    {
    for (int i=0; i < this->TableSize; i++)
      {
      if ( this->Table[i] )
        {
        delete this->Table[i];
        }
      }
    delete [] this->Table;
    }
}

// Description:
// Return non-zero if edge (p1,p2) is an edge; otherwise 0.
int vtkEdgeTable::IsEdge(int p1, int p2)
{
  int index, search;

  if ( p1 < p2 )
    {
    index = p1;
    search = p2;
    }
  else
    {
    index = p2;
    search = p1;
    }

  if ( this->Table[index] == NULL ) 
    {
    return 0;
    }
  else
    {
    return this->Table[index]->IsId(search);
    }
}

// Description:
// Insert the edge (p1,p2) into the table. It is the user's responsibility to
// check if the edge has already been inserted.
void vtkEdgeTable::InsertEdge(int p1, int p2)
{
  int index, search;

  if ( p1 < p2 )
    {
    index = p1;
    search = p2;
    }
  else
    {
    index = p2;
    search = p1;
    }

  if ( this->Table[index] == NULL ) 
    {
    this->Table[index] = new vtkIdList(6,12);
    }

  this->Table[index]->InsertNextId(search);
}

// Description:
// Intialize traversal of edges in table.
void vtkEdgeTable::InitTraversal()
{
  this->Position[0] = 0;
  this->Position[1] = -1;
}

// Description:
// Traverse list of edges in table. Return the edge as (p1,p2), where p1 and p2
// are point id's. Method return value is zero if list is exhausted; non-zero
// otherwise. The value of p1 is guaranteed to be <= p2.
int vtkEdgeTable::GetNextEdge(int &p1, int &p2)
{
  for ( ; this->Position[0] < this->TableSize; 
  this->Position[0]++, this->Position[1]=(-1) )
    {
    if ( this->Table[this->Position[0]] != NULL && 
    ++this->Position[1] < this->Table[this->Position[0]]->GetNumberOfIds() )
      {
      p1 = this->Position[0];
      p2 = this->Table[this->Position[0]]->GetId(this->Position[1]);
      return 1;
      }
    }

  return 0;
}
