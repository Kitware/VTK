/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMapper.cxx
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
#include "vtkMapper.hh"

// Description:
// Construct with initial range (0,1).
vtkMapper::vtkMapper()
{
  this->Input = NULL;
  this->Colors = NULL;

  this->StartRender = NULL;
  this->StartRenderArgDelete = NULL;
  this->StartRenderArg = NULL;
  this->EndRender = NULL;
  this->EndRenderArgDelete = NULL;
  this->EndRenderArg = NULL;

  this->LookupTable = NULL;

  this->ScalarsVisible = 1;
  this->ScalarRange[0] = 0.0; this->ScalarRange[1] = 1.0;

  this->SelfCreatedLookupTable = 0;
}

vtkMapper::~vtkMapper()
{
  if ( this->SelfCreatedLookupTable && this->LookupTable != NULL) 
    this->LookupTable->Delete();
  if ( this->Colors != NULL ) this->Colors->Delete();
}

// Description:
// Overload standard modified time function. If lookup table is modified,
// then this object is modified as well.
unsigned long vtkMapper::GetMTime()
{
  unsigned long mTime=this->MTime.GetMTime();
  unsigned long lutMTime;

  if ( this->LookupTable != NULL )
    {
    lutMTime = this->LookupTable->GetMTime();
    mTime = ( lutMTime > mTime ? lutMTime : mTime );
    }

  return mTime;
}

void vtkMapper::operator=(const vtkMapper& m)
{
  this->SetLookupTable(m.LookupTable);

  this->SetScalarsVisible(m.ScalarsVisible);
  this->SetScalarRange(m.ScalarRange[0], m.ScalarRange[1]);

  this->SetStartRender(m.StartRender,m.StartRenderArg);
  this->SetEndRender(m.EndRender,m.EndRenderArg);
}

vtkColorScalars *vtkMapper::GetColors()
{
  vtkPointData *pd;
  vtkScalars *scalars;
  int i, numPts;
  vtkColorScalars *colors;
  
  // make sure we have a lookup table
  if ( this->LookupTable == NULL ) this->CreateDefaultLookupTable();
  this->LookupTable->Build();

  //
  // create colors
  //
  numPts = this->Input->GetNumberOfPoints();
  if ( this->ScalarsVisible && (pd=this->Input->GetPointData()) && 
       (scalars=pd->GetScalars()) )
    {
    if ( strcmp(scalars->GetScalarType(),"ColorScalar") )
      {
      if ( this->Colors == NULL ) 
	{
	this->Colors = new vtkAPixmap(numPts);
	}
      else
	{
	int numColors=this->Colors->GetNumberOfColors();
	if ( numColors < numPts ) this->Colors->Allocate(numPts);
	}
      
      this->LookupTable->SetTableRange(this->ScalarRange);
      for (i=0; i < numPts; i++)
	{
	this->Colors->SetColor(i,this->LookupTable->
			       MapValue(scalars->GetScalar(i)));
	}
      colors = this->Colors;
      }
    else //color scalar
      {
      colors = (vtkColorScalars *)scalars;
      }
    }
  else
    {
    if ( this->Colors ) this->Colors->Delete();
    this->Colors = colors = NULL;
    }
  
  return colors;
}

// Description:
// Specify a function to be called before rendering process begins.
// Function will be called with argument provided.
void vtkMapper::SetStartRender(void (*f)(void *), void *arg)
{
  if ( f != this->StartRender || arg != this->StartRenderArg )
    {
    // delete the current arg if there is one and a delete meth
    if ((this->StartRenderArg)&&(this->StartRenderArgDelete))
      {
      (*this->StartRenderArgDelete)(this->StartRenderArg);
      }
    this->StartRender = f;
    this->StartRenderArg = arg;
    this->Modified();
    }
}

// Description:
// Specify a method to delete the user specified argument to the 
// StartRenderMethod. This is an optional capability.
void vtkMapper::SetStartRenderArgDelete(void (*f)(void *))
{
  if ( f != this->StartRenderArgDelete)
    {
    this->StartRenderArgDelete = f;
    this->Modified();
    }
}

// Description:
// Specify a method to delete the user specified argument to the 
// EndRenderMethod. This is an optional capability.
void vtkMapper::SetEndRenderArgDelete(void (*f)(void *))
{
  if ( f != this->EndRenderArgDelete)
    {
    this->EndRenderArgDelete = f;
    this->Modified();
    }
}

// Description:
// Specify a function to be called when rendering process completes.
// Function will be called with argument provided.
void vtkMapper::SetEndRender(void (*f)(void *), void *arg)
{
  if ( f != this->EndRender || arg != EndRenderArg )
    {
    // delete the current arg if there is one and a delete meth
    if ((this->EndRenderArg)&&(this->EndRenderArgDelete))
      {
      (*this->EndRenderArgDelete)(this->EndRenderArg);
      }
    this->EndRender = f;
    this->EndRenderArg = arg;
    this->Modified();
    }
}

// Description:
// Specify a lookup table for the mapper to use.
void vtkMapper::SetLookupTable(vtkLookupTable *lut)
{
  if ( this->LookupTable != lut ) 
    {
    if ( this->SelfCreatedLookupTable ) this->LookupTable->Delete();
    this->SelfCreatedLookupTable = 0;
    this->LookupTable = lut;
    this->Modified();
    }
}

vtkLookupTable *vtkMapper::GetLookupTable()
{
  if ( this->LookupTable == NULL ) this->CreateDefaultLookupTable();
  return this->LookupTable;
}

void vtkMapper::CreateDefaultLookupTable()
{
  if ( this->SelfCreatedLookupTable ) this->LookupTable->Delete();
  this->LookupTable = new vtkLookupTable;
  this->SelfCreatedLookupTable = 1;
}

float *vtkMapper::GetCenter()
{
  static float center[3];
  float *bounds;

  bounds = this->GetBounds();
  for (int i=0; i<3; i++) center[i] = (bounds[2*i+1] + bounds[2*i]) / 2.0;
  return center;
}

// Description:
// Update the network connected to this mapper.
void vtkMapper::Update()
{
  if ( this->Input )
    {
    this->Input->Update();
    }
}

void vtkMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkObject::PrintSelf(os,indent);

  if ( this->Input )
    {
    os << indent << "Input: (" << this->Input << ")\n";
    }
  else
    {
    os << indent << "Input: (none)\n";
    }

  os << indent << "Build Time: " <<this->BuildTime.GetMTime() << "\n";
  if ( this->StartRender )
    {
    os << indent << "Start Render method defined.\n";
    }
  else
    {
    os << indent << "No Start Render method.\n";
    }

  if ( this->EndRender )
    {
    os << indent << "End Render method defined.\n";
    }
  else
    {
    os << indent << "No End Render method.\n";
    }

  if ( this->LookupTable )
    {
    os << indent << "Lookup Table:\n";
    this->LookupTable->PrintSelf(os,indent.GetNextIndent());
    }
  else
    {
    os << indent << "Lookup Table: (none)\n";
    }
  os << indent << "Scalars Visible: " 
    << (this->ScalarsVisible ? "On\n" : "Off\n");

  float *range = this->GetScalarRange();
  os << indent << "Scalar Range: (" << range[0] << ", " << range[1] << ")\n";
}
