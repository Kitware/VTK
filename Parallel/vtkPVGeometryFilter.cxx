/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVGeometryFilter.cxx
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
#include "vtkPVGeometryFilter.h"
#include "vtkGeometryFilter.h"
#include "vtkExtractEdges.h"
#include "vtkOutlineSource.h"
#include "vtkStructuredGridOutlineFilter.h"
#include "vtkStripper.h"
#include "vtkObjectFactory.h"
#include "vtkCommand.h"

vtkCxxRevisionMacro(vtkPVGeometryFilter, "1.7");
vtkStandardNewMacro(vtkPVGeometryFilter);

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
    // I think this filter is misbehaving.
    this->GetOutput()->CheckAttributes();
    return;
    }
  if (input->IsA("vtkPolyData"))
    {
    vtkPolyData *inPd = vtkPolyData::SafeDownCast(input);
    vtkPolyData *out = this->GetOutput(); 
    if (this->UseStrips)
      {
      vtkPolyData *inCopy = vtkPolyData::New();
      vtkStripper *stripper = vtkStripper::New();
      inCopy->ShallowCopy(inPd);
      stripper->SetInput(inCopy);
      stripper->Update();
      out->CopyStructure(stripper->GetOutput());
      out->GetPointData()->ShallowCopy(stripper->GetOutput()->GetPointData());
      out->GetCellData()->ShallowCopy(stripper->GetOutput()->GetCellData());
      inCopy->Delete();
      stripper->Delete();
      return;
      }
    else
      {
      out->ShallowCopy(inPd);
      return;
      }
    }

  // We are not stripping unstructured grids ...
  this->GetInput()->CheckAttributes();
  this->vtkDataSetSurfaceFilter::Execute();
  // I think this filter is misbehaving.
  this->GetOutput()->CheckAttributes();
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

//----------------------------------------------------------------------------
void vtkPVGeometryFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  if (this->OutlineFlag)
    {
    os << indent << "OutlineFlag: On\n";
    }
  else
    {
    os << indent << "OutlineFlag: Off\n";
    }
}
