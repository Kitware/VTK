/*=========================================================================

  Program:   ParaView
  Module:    vtkPVGeometryFilter.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 2000-2001 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither the name of Kitware nor the names of any contributors may be used
   to endorse or promote products derived from this software without specific 
   prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkPVGeometryFilter.h"
#include "vtkGeometryFilter.h"
#include "vtkExtractEdges.h"
#include "vtkOutlineSource.h"
#include "vtkStructuredGridOutlineFilter.h"
#include "vtkObjectFactory.h"
#include "vtkCommand.h"


//------------------------------------------------------------------------------
vtkPVGeometryFilter* vtkPVGeometryFilter::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkPVGeometryFilter");
  if(ret)
    {
    return (vtkPVGeometryFilter*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkPVGeometryFilter;
}




//----------------------------------------------------------------------------
vtkPVGeometryFilter::vtkPVGeometryFilter ()
{
  this->OutlineFlag = 0;
}

//----------------------------------------------------------------------------
vtkPVGeometryFilter::~vtkPVGeometryFilter ()
{
}


//----------------------------------------------------------------------------
void vtkPVGeometryFilter::Execute()
{
  vtkDataSet *input = this->GetInput();

  if (input == NULL)
    {
    return;
    }

  if (input->IsA("vtkImageData"))
    {
    this->ImageDataExecute((vtkImageData*)input);
    return;
    }

  if (input->IsA("vtkStructuredGrid"))
    {
    this->StructuredGridExecute((vtkStructuredGrid*)input);
    return;
    }

  if (input->IsA("vtkRectilinearGrid"))
    {
    this->RectilinearGridExecute((vtkRectilinearGrid*)input);
    return;
    }

  if (input->IsA("vtkUnstructuredGrid"))
    {
    this->UnstructuredGridExecute((vtkUnstructuredGrid*)input);
    return;
    }

  this->vtkDataSetSurfaceFilter::Execute();
  return;
}



//----------------------------------------------------------------------------
void vtkPVGeometryFilter::ImageDataExecute(vtkImageData *input)
{
  float *spacing;
  float *origin;
  int *ext;
  float bounds[6];
  vtkPolyData *output = this->GetOutput();

  ext = input->GetWholeExtent();

  // If 2d then default to superclass behavior.
  if (ext[0] == ext[1] || ext[2] == ext[3] || ext[4] == ext[5])
    {
    this->vtkDataSetSurfaceFilter::Execute();
    this->OutlineFlag = 0;
    return;
    }  
  this->OutlineFlag = 1;

  //
  // Otherwise, let OutlineSource do all the work
  //
  
  if (output->GetUpdatePiece() == 0)
    {
    spacing = input->GetSpacing();
    origin = input->GetOrigin();
    
    bounds[0] = spacing[0] * ((float)ext[0]) + origin[0];
    bounds[1] = spacing[0] * ((float)ext[1]) + origin[0];
    bounds[2] = spacing[1] * ((float)ext[2]) + origin[1];
    bounds[3] = spacing[1] * ((float)ext[3]) + origin[1];
    bounds[4] = spacing[2] * ((float)ext[4]) + origin[2];
    bounds[5] = spacing[2] * ((float)ext[5]) + origin[2];

    vtkOutlineSource *outline = vtkOutlineSource::New();
    outline->SetBounds(bounds);
    outline->Update();

    output->SetPoints(outline->GetOutput()->GetPoints());
    output->SetLines(outline->GetOutput()->GetLines());
    outline->Delete();
    }

}




//----------------------------------------------------------------------------
void vtkPVGeometryFilter::StructuredGridExecute(vtkStructuredGrid *input)
{
  int *ext;
  vtkPolyData *output = this->GetOutput();

  ext = input->GetWholeExtent();

  // If 2d then default to superclass behavior.
  if (ext[0] == ext[1] || ext[2] == ext[3] || ext[4] == ext[5])
    {
    this->vtkDataSetSurfaceFilter::Execute();
    this->OutlineFlag = 0;
    return;
    }  
  this->OutlineFlag = 1;

  //
  // Otherwise, let Outline do all the work
  //
  

  vtkStructuredGridOutlineFilter *outline = vtkStructuredGridOutlineFilter::New();
  // Because of streaming, it is important to set the input and not copy it.
  outline->SetInput(input);
  outline->GetOutput()->SetUpdateNumberOfPieces(output->GetUpdateNumberOfPieces());
  outline->GetOutput()->SetUpdatePiece(output->GetUpdatePiece());
  outline->GetOutput()->SetUpdateGhostLevel(output->GetUpdateGhostLevel());
  outline->GetOutput()->Update();

  output->CopyStructure(outline->GetOutput());
  outline->Delete();
}


//----------------------------------------------------------------------------
void vtkPVGeometryFilter::RectilinearGridExecute(vtkRectilinearGrid *input)
{
  int *ext;
  float bounds[6];
  vtkPolyData *output = this->GetOutput();

  ext = input->GetWholeExtent();

  // If 2d then default to superclass behavior.
  if (ext[0] == ext[1] || ext[2] == ext[3] || ext[4] == ext[5])
    {
    this->vtkDataSetSurfaceFilter::Execute();
    this->OutlineFlag = 0;
    return;
    }  
  this->OutlineFlag = 1;

  //
  // Otherwise, let Outline do all the work
  //
  
  // Until we get a vtkRectilinearOutlineFilter.
  input->GetBounds(bounds);

  vtkOutlineSource *outline = vtkOutlineSource::New();
  outline->SetBounds(bounds);
  outline->Update();

  output->CopyStructure(outline->GetOutput());
  outline->Delete();
}


//----------------------------------------------------------------------------
void vtkPVGeometryFilter::UnstructuredGridExecute(vtkUnstructuredGrid *input)
{
  vtkIdType numCells, id;
  int type;
  vtkPolyData *output = this->GetOutput();
  
  this->OutlineFlag = 0;

  // Look through the input a see if it is 2D.
  // I know that having only part of the data may fool us, but so what.
  numCells = input->GetNumberOfCells();
  for (id = 0; id < numCells && this->OutlineFlag == 0; ++id)
    {
    type = input->GetCellType(id);
    if (type == VTK_TETRA || type == VTK_VOXEL || type == VTK_HEXAHEDRON
        || type == VTK_WEDGE || type == VTK_PYRAMID)
      {
      this->OutlineFlag = 1;
      } 
    }

  // If 2d then default to superclass behavior.
  if (this->OutlineFlag == 0)
    {
    this->vtkDataSetSurfaceFilter::Execute();
    return;
    }  


  vtkDataSetSurfaceFilter *surface = vtkDataSetSurfaceFilter::New();
  //vtkGeometryFilter *surface = vtkGeometryFilter::New();
  surface->SetInput(input);
  vtkExtractEdges *edges = vtkExtractEdges::New();
  edges->SetInput(surface->GetOutput());
  edges->GetOutput()->SetUpdateNumberOfPieces(output->GetUpdateNumberOfPieces());
  edges->GetOutput()->SetUpdatePiece(output->GetUpdatePiece());
  edges->GetOutput()->SetUpdateGhostLevel(output->GetUpdateGhostLevel());
  edges->GetOutput()->Update();

  output->CopyStructure(edges->GetOutput());
  surface->Delete();
  edges->Delete();
}
