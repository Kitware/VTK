/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAbstractVolumeMapper.cxx
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
#include "vtkAbstractVolumeMapper.h"

#include "vtkDataSet.h"

vtkCxxRevisionMacro(vtkAbstractVolumeMapper, "1.2");

// Construct a vtkAbstractVolumeMapper 
vtkAbstractVolumeMapper::vtkAbstractVolumeMapper()
{
  this->Bounds[0] = this->Bounds[2] = this->Bounds[4] = -1.0;
  this->Bounds[1] = this->Bounds[3] = this->Bounds[5] = 1.0;
  this->Center[0] = this->Center[1] = this->Center[2] = 0.0;
}

vtkAbstractVolumeMapper::~vtkAbstractVolumeMapper()
{
}

void vtkAbstractVolumeMapper::Update()
{
  if ( this->GetDataSetInput() )
    {
    this->GetDataSetInput()->UpdateInformation();
    this->GetDataSetInput()->SetUpdateExtentToWholeExtent();
    this->GetDataSetInput()->Update();
    }
}

// Get the bounds for the input of this mapper as 
// (Xmin,Xmax,Ymin,Ymax,Zmin,Zmax).
float *vtkAbstractVolumeMapper::GetBounds()
{
  static float bounds[] = {-1.0,1.0, -1.0,1.0, -1.0,1.0};

  if ( ! this->GetDataSetInput() ) 
    {
    return bounds;
    }
  else
    {
    this->Update();
    this->GetDataSetInput()->GetBounds(this->Bounds);
    return this->Bounds;
    }
}

void vtkAbstractVolumeMapper::SetInput( vtkDataSet *vtkNotUsed(input) )
{
  vtkErrorMacro("Cannot set the input on the abstract volume mapper"
                " - must be set on a subclass" );
}

vtkDataSet *vtkAbstractVolumeMapper::GetDataSetInput()
{
  if (this->NumberOfInputs < 1)
    {
    return NULL;
    }
  return (vtkDataSet *)this->Inputs[0];
}


// Print the vtkAbstractVolumeMapper
void vtkAbstractVolumeMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

