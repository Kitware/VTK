/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTransformPolyDataFilter.cxx
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
#include "vtkTransformPolyDataFilter.h"
#include "vtkNormals.h"
#include "vtkVectors.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkTransformPolyDataFilter* vtkTransformPolyDataFilter::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkTransformPolyDataFilter");
  if(ret)
    {
    return (vtkTransformPolyDataFilter*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkTransformPolyDataFilter;
}




vtkTransformPolyDataFilter::vtkTransformPolyDataFilter()
{
  this->Transform = NULL;
}

vtkTransformPolyDataFilter::~vtkTransformPolyDataFilter()
{
  this->SetTransform(NULL);
}

void vtkTransformPolyDataFilter::Execute()
{
  vtkPoints *inPts;
  vtkPoints *newPts;
  vtkVectors *inVectors, *inCellVectors;
  vtkVectors *newVectors=NULL, *newCellVectors=NULL;
  vtkNormals *inNormals, *inCellNormals;
  vtkNormals *newNormals=NULL, *newCellNormals=NULL;
  int numPts, numCells;
  vtkPolyData *input = this->GetInput();
  vtkPolyData *output = this->GetOutput();
  vtkPointData *pd=input->GetPointData(), *outPD=output->GetPointData();
  vtkCellData *cd=input->GetCellData(), *outCD=output->GetCellData();

  vtkDebugMacro(<<"Executing polygonal transformation");

  // Check input
  //
  if ( this->Transform == NULL )
    {
    vtkErrorMacro(<<"No transform defined!");
    return;
    }

  inPts = input->GetPoints();
  inVectors = pd->GetVectors();
  inNormals = pd->GetNormals();
  inCellVectors = cd->GetVectors();
  inCellNormals = cd->GetNormals();

  if ( !inPts )
    {
    vtkErrorMacro(<<"No input data");
    return;
    }

  numPts = inPts->GetNumberOfPoints();
  numCells = input->GetNumberOfCells();

  newPts = vtkPoints::New();
  newPts->Allocate(numPts);
  if ( inVectors ) 
    {
    newVectors = vtkVectors::New();
    newVectors->Allocate(numPts);
    }
  if ( inNormals ) 
    {
    newNormals = vtkNormals::New();
    newNormals->Allocate(numPts);
    }
  if ( inCellVectors ) 
    {
    newCellVectors = vtkVectors::New();
    newCellVectors->Allocate(numCells);
    }
  if ( inCellNormals ) 
    {
    newCellNormals = vtkNormals::New();
    newCellNormals->Allocate(numCells);
    }

  // Loop over all points, updating position
  //
  this->Transform->TransformPoints(inPts,newPts);
  this->UpdateProgress (.2);

  // Ditto for vectors and normals
  //
  if ( inVectors )
    {
    this->Transform->TransformVectors(inPts,newPts,
				      inVectors,newVectors);
    }

  this->UpdateProgress (.4);

  if ( inNormals )
    {
    this->Transform->TransformNormals(inPts,newPts,
				      inNormals,newNormals);
    }

  this->UpdateProgress (.6);

  if ( (inCellVectors || inCellNormals) )
    {
    vtkPoints *inCellPts = NULL;
    vtkPoints *outCellPts = NULL;

    // We have to create a set of cell points (consisting of the
    // first point in each cell) to calculate the transformation 
    // for the normals if the transform is nonlinear.
    
    // This treatment is accurate for perspective transformations,
    // and a fair approximation for other nonlinear transformations.

    if ( this->Transform->GetTransformType() & VTK_LINEAR_TRANSFORM
	!= VTK_LINEAR_TRANSFORM )
      {
      inCellPts = vtkPoints::New();
      inCellPts->Allocate(numCells);
      outCellPts = vtkPoints::New();
      outCellPts->Allocate(numCells);

      int i, pointId;
      for (i = 0; i < numCells; i++)
	{
	pointId = input->GetCell(i)->GetPointId(0);
	inCellPts->SetPoint(i,inPts->GetPoint(pointId));
	outCellPts->SetPoint(i,newPts->GetPoint(pointId));
	}
      }

    if ( inCellVectors )
      {
      this->Transform->TransformVectors(inCellPts,outCellPts,
					inCellVectors,newCellVectors);
      }

    this->UpdateProgress (.7);

    if ( inCellNormals )
      {
      this->Transform->TransformNormals(inCellPts,outCellPts,
					inCellNormals,newCellNormals);
      }

    if (inCellPts)
      {
      inCellPts->Delete();
      outCellPts->Delete();
      }
    }

  this->UpdateProgress (.8);

  // Update ourselves and release memory
  //
  output->SetPoints(newPts);
  newPts->Delete();

  output->SetVerts(input->GetVerts());
  output->SetLines(input->GetLines());
  output->SetPolys(input->GetPolys());
  output->SetStrips(input->GetStrips());

  if (newNormals)
    {
    outPD->SetNormals(newNormals);
    newNormals->Delete();
    }

  if (newVectors)
    {
    outPD->SetVectors(newVectors);
    newVectors->Delete();
    }

  if (newCellNormals)
    {
    outCD->SetNormals(newCellNormals);
    newCellNormals->Delete();
    }

  if (newCellVectors)
    {
    outCD->SetVectors(newCellVectors);
    newCellVectors->Delete();
    }

  outPD->PassNoReplaceData(pd);
  outCD->PassNoReplaceData(cd);
}

unsigned long vtkTransformPolyDataFilter::GetMTime()
{
  unsigned long mTime=this->MTime.GetMTime();
  unsigned long transMTime;

  if ( this->Transform )
    {
    transMTime = this->Transform->GetMTime();
    mTime = ( transMTime > mTime ? transMTime : mTime );
    }

  return mTime;
}

void vtkTransformPolyDataFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkPolyDataToPolyDataFilter::PrintSelf(os,indent);

  os << indent << "Transform: " << this->Transform << "\n";
}
