/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStringTree.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

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
#include <string.h>
#include "vtkStringTree.h"




//============================================================================
// Methods for the actual tree object.
// TreeNode and Tree are almost interchangable.

//----------------------------------------------------------------------------
vtkStringTree::vtkStringTree()
{
}

//----------------------------------------------------------------------------
vtkStringTree::~vtkStringTree()
{
}

//----------------------------------------------------------------------------
void vtkStringTree::DeleteItem(void *item)
{
  delete [] (char *)(item);
}

//----------------------------------------------------------------------------
void vtkStringTree::PrintItem(ostream& os, vtkIndent indent, void *item)
{
  os << indent << "String: " << (char *)(item) << endl;
}



//----------------------------------------------------------------------------
void vtkStringTree::SetItem(char *str)
{
  char *copy;

  // copy the string
  copy = new char [strlen(str)];
  strcpy(copy, str);

  this->vtkTree::SetItem(copy);
}


//----------------------------------------------------------------------------
int vtkStringTree::AddNewNode(char *str)
{
  int idx;

  idx = this->vtkTree::AddNewNode();
  this->MoveToChild(idx);
  this->SetItem(str);
  this->MoveToParent();

  return idx;
}

  
//----------------------------------------------------------------------------
int vtkStringTree::AddNewLeaf(char *str)
{
  int idx;

  idx = this->vtkTree::AddNewLeaf();
  this->MoveToChild(idx);
  this->SetItem(str);
  this->MoveToParent();

  return idx;
}

  

