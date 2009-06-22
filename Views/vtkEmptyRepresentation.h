/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkEmptyRepresentation.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/
// .NAME vtkEmptyRepresentation - 
//
// .SECTION Description

#ifndef __vtkEmptyRepresentation_h
#define __vtkEmptyRepresentation_h

#include "vtkDataRepresentation.h"
#include "vtkSmartPointer.h"

class vtkConvertSelectionDomain;

class VTK_VIEWS_EXPORT vtkEmptyRepresentation : public vtkDataRepresentation
{
public:
  static vtkEmptyRepresentation* New();
  vtkTypeRevisionMacro(vtkEmptyRepresentation, vtkDataRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Since this representation has no inputs, override superclass 
  // implementation with one that ignores "port" and "conn" and still allows it
  // to have an annotation output.
  virtual vtkAlgorithmOutput* GetInternalAnnotationOutputPort(int port, int conn);

protected:
  vtkEmptyRepresentation();
  ~vtkEmptyRepresentation();

private:
  vtkEmptyRepresentation(const vtkEmptyRepresentation&); // Not implemented
  void operator=(const vtkEmptyRepresentation&);   // Not implemented

//BTX
  vtkSmartPointer<vtkConvertSelectionDomain> ConvertDomains;
//ETX
};

#endif

