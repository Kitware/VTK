/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSpatialRepFilter.cc
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
#include "vtkSpatialRepFilter.hh"

vtkSpatialRepFilter::vtkSpatialRepFilter()
{
  this->SpatialRep = NULL;
  this->Level = 0;
  this->TerminalNodesRequested = 0;

  this->Output = (vtkDataSet *) new vtkPolyData; //leaf representation
  this->Output->SetSource(this);
  for (int i=0; i <= VTK_MAX_SPATIALREP_LEVEL; i++) //intermediate representations
    {
    this->OutputList[i] = NULL;
    }
}

vtkSpatialRepFilter::~vtkSpatialRepFilter()
{
  if ( this->Output ) delete this->Output;
  for (int i=0; i <= Level; i++) //superclass deletes OutputList[0]
    {
    if ( this->OutputList[i] != NULL ) delete this->OutputList[i];
    }
}

vtkPolyData *vtkSpatialRepFilter::GetOutput()
{
  if ( !this->TerminalNodesRequested )
    {
    this->TerminalNodesRequested = 1;
    this->Modified();
    }
  return (vtkPolyData *)this->Output;
}

vtkPolyData *vtkSpatialRepFilter::GetOutput(int level)
{
  if ( level < 0 || !this->SpatialRep || 
  level > this->SpatialRep->GetMaxLevel() )
    {
    vtkErrorMacro(<<"Level requested is <0 or >= Locator's MaxLevel");
    return this->OutputList[0];
    }

  if ( this->OutputList[level] == NULL )
    {
    this->OutputList[level] = new vtkPolyData;
    this->OutputList[level]->SetSource(this);
    this->Modified(); //asking for new output
    }

  return this->OutputList[level];
}

void vtkSpatialRepFilter::ResetOutput()
{
  this->TerminalNodesRequested = 0;
  for ( int i=0; i <= VTK_MAX_SPATIALREP_LEVEL; i++)
    {
    if ( this->OutputList[i] != NULL ) delete this->OutputList[i];
    }
}
    

// Build OBB tree
void vtkSpatialRepFilter::Execute()
{
  vtkDebugMacro(<<"Building OBB representation");

  this->SpatialRep->SetDataSet(this->Input);
  this->SpatialRep->Update();
  this->Level = this->SpatialRep->GetLevel();

  vtkDebugMacro(<<"OBB deepest tree level: " << this->Level);
  this->GenerateOutput();
}

// Generate OBB representations at different requested levels.
void vtkSpatialRepFilter::GenerateOutput()
{
  int inputModified=(this->Input->GetMTime() > this->GetMTime() ? 1 : 0);
  int i;

  //
  // If input to filter is modified, have to update all levels of OBB
  //
  if ( inputModified )
    {
    for ( i=0; i <= this->Level; i++ )
      {
      if ( this->OutputList[i] != NULL ) this->OutputList[i]->Initialize();
      }
    }

  //
  // Loop over all requested levels generating new levels as necessary
  //
  for ( i=0; i <= Level; i++ )
    {
    if ( this->OutputList[i] != NULL &&
    this->OutputList[i]->GetNumberOfPoints() < 1 ) //compute OBB
      {
      this->SpatialRep->GenerateRepresentation(i, this->OutputList[i]);
      }
    }
  //
  // If terminal leafs requested, generate rep
  //
  if ( this->TerminalNodesRequested )
    {
    this->SpatialRep->GenerateRepresentation(-1, (vtkPolyData *)this->Output);
    }
}

void vtkSpatialRepFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkDataSetFilter::PrintSelf(os,indent);

  os << indent << "Level: " << this->Level << "\n";
}
