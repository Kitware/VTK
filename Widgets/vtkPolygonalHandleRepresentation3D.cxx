/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolygonalHandleRepresentation3D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPolygonalHandleRepresentation3D.h"
#include "vtkPolyDataMapper.h"
#include "vtkActor.h"
#include "vtkCellPicker.h"
#include "vtkRenderer.h"
#include "vtkObjectFactory.h"
#include "vtkProperty.h"
#include "vtkCoordinate.h"
#include "vtkRenderWindow.h"
#include "vtkCamera.h"
#include "vtkMatrix4x4.h"
#include "vtkPointPlacer.h"

vtkStandardNewMacro(vtkPolygonalHandleRepresentation3D);

//----------------------------------------------------------------------
vtkPolygonalHandleRepresentation3D::vtkPolygonalHandleRepresentation3D()
{
  this->Offset[0] = 
  this->Offset[1] = 
  this->Offset[2] = 0.0;

  this->Actor = vtkActor::New();
  this->Actor->SetMapper(this->Mapper);
  this->Actor->SetProperty(this->Property);
  this->HandlePicker->AddPickList(this->Actor);
}

//-------------------------------------------------------------------------
void vtkPolygonalHandleRepresentation3D::SetWorldPosition(double p[3])
{
  if (!this->Renderer || !this->PointPlacer || 
                          this->PointPlacer->ValidateWorldPosition( p ))
    {
    this->HandleTransformMatrix->SetElement(0, 3, p[0] - this->Offset[0]);
    this->HandleTransformMatrix->SetElement(1, 3, p[1] - this->Offset[1]);
    this->HandleTransformMatrix->SetElement(2, 3, p[2] - this->Offset[2]);

    this->WorldPosition->SetValue( (*(this->HandleTransformMatrix))[0][3],
                                   (*(this->HandleTransformMatrix))[1][3],
                                   (*(this->HandleTransformMatrix))[2][3] );
    this->WorldPositionTime.Modified();
    }
}

//----------------------------------------------------------------------
void vtkPolygonalHandleRepresentation3D::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  
  os << indent << "Offset: (" << this->Offset[0] << "," 
     << this->Offset[1] << ")\n";
}

