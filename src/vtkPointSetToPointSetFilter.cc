/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPointSetToPointSetFilter.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1995 Ken Martin, Will Schroeder, Bill Lorensen.

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
#include "vtkPointSetToPointSetFilter.hh"
#include "vtkPolyData.hh"

vtkPointSetToPointSetFilter::vtkPointSetToPointSetFilter()
{
  // prevents dangling reference to PointSet
  this->PointSet = new vtkPolyData;
}

vtkPointSetToPointSetFilter::~vtkPointSetToPointSetFilter()
{
  this->PointSet->Delete();
}

vtkDataSet* vtkPointSetToPointSetFilter::MakeObject()
{
  vtkPointSetToPointSetFilter *o = new vtkPointSetToPointSetFilter();
  o->PointSet = this->PointSet;
  o->SetPoints(this->GetPoints());
  return o;
}

void vtkPointSetToPointSetFilter::Modified()
{
  this->vtkPointSet::Modified();
  this->vtkPointSetFilter::_Modified();
}

unsigned long int vtkPointSetToPointSetFilter::GetMTime()
{
  unsigned long dtime = this->vtkPointSet::GetMTime();
  unsigned long ftime = this->vtkPointSetFilter::_GetMTime();
  return (dtime > ftime ? dtime : ftime);
}

void vtkPointSetToPointSetFilter::DebugOn()
{
  vtkPointSet::DebugOn();
  vtkPointSetFilter::_DebugOn();
}

void vtkPointSetToPointSetFilter::DebugOff()
{
  vtkPointSet::DebugOff();
  vtkPointSetFilter::_DebugOff();
}

int vtkPointSetToPointSetFilter::GetDataReleased()
{
  return this->DataReleased;
}

void vtkPointSetToPointSetFilter::SetDataReleased(int flag)
{
  this->DataReleased = flag;
}

void vtkPointSetToPointSetFilter::Update()
{
  this->UpdateFilter();
}

void vtkPointSetToPointSetFilter::Initialize()
{
  if ( this->Input != NULL )
    {
    // copies input geometry to internal data set
    vtkDataSet *ds=this->Input->MakeObject();
    this->PointSet->Delete();
    this->PointSet = ds;
    }
  else
    {
    return;
    }
}

void vtkPointSetToPointSetFilter::ComputeBounds()
{
  if ( this->Points != NULL )
    {
    this->Points->ComputeBounds();
    float *bounds=this->Points->GetBounds();
    for (int i=0; i < 6; i++) this->Bounds[i] = bounds[i];
    }
};

void vtkPointSetToPointSetFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkPointSet::PrintSelf(os,indent);
  vtkPointSetFilter::_PrintSelf(os,indent);

  if ( this->PointSet )
    {
    os << indent << "PointSet: (" << this->PointSet << ")\n";
    os << indent << "PointSet type: " << this->PointSet->GetClassName() <<"\n";
    }
  else
    {
    os << indent << "PointSet: (none)\n";
    }
}
