/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolyMapper.cxx
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
#include "vtkPolyMapper.hh"
#include "vtkPolyMapperDevice.hh"
#include "vtkRenderWindow.hh"

// Description:
// Construct mapper with vertices, lines, polygons, and triangle strips
// turned on.
vtkPolyMapper::vtkPolyMapper()
{
  this->Device = NULL;
}

vtkPolyMapper::~vtkPolyMapper()
{
  //delete internally created objects
  if ( this->Device != NULL ) this->Device->Delete();
}

void vtkPolyMapper::SetInput(vtkPolyData *in)
{
  if (in != this->Input )
    {
    vtkDebugMacro(<<" setting Input to " << (void *)in);
    this->Input = (vtkDataSet *) in;
    this->Modified();
    }
}

//
// Return bounding box of data
//
float *vtkPolyMapper::GetBounds()
{
  static float bounds[] = {-1.0,1.0, -1.0,1.0, -1.0,1.0};

  if ( ! this->Input ) 
    return bounds;
  else
    {
    this->Input->Update();
    return this->Input->GetBounds();
    }
}

//
// Receives from Actor -> maps data to primitives
//
void vtkPolyMapper::Render(vtkRenderer *ren, vtkActor *act)
{
  int numPts;
  vtkPolyData *input=(vtkPolyData *)this->Input;
  vtkColorScalars *colors;
//
// make sure that we've been properly initialized
//
  if ( input == NULL ) 
    {
    vtkErrorMacro(<< "No input!");
    return;
    }
  else
    {
    input->Update();
    numPts = input->GetNumberOfPoints();
    } 

  if ( this->LookupTable == NULL ) this->CreateDefaultLookupTable();
  this->LookupTable->Build();
  //
  // Now send data down to primitives and draw it
  //
  if ( this->GetMTime() > this->BuildTime || 
  input->GetMTime() > this->BuildTime || 
  this->LookupTable->GetMTime() > this->BuildTime )
    {
    colors = this->GetColors();

    if (!this->Device) 
      {
      this->Device = ren->GetRenderWindow()->MakePolyMapper();
      }
    this->Device->Build(input,colors);

    this->BuildTime.Modified();
    }

  // draw the primitives
  this->Device->Draw(ren,act);
}

