/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPerturbCoincidentVertices.h

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
// .NAME vtkPerturbCoincidentVertices - remove vertices of a vtkGraph with 
//    degree zero.
//
// .SECTION Description

#ifndef __vtkPerturbCoincidentVertices_h
#define __vtkPerturbCoincidentVertices_h

#include "vtkGraphAlgorithm.h"
#include "vtkSmartPointer.h" // for ivars

class vtkCoincidentPoints;
class vtkDataSet;

class VTK_INFOVIS_EXPORT vtkPerturbCoincidentVertices : public vtkGraphAlgorithm
{
public:
  static vtkPerturbCoincidentVertices* New();
  vtkTypeRevisionMacro(vtkPerturbCoincidentVertices,vtkGraphAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkPerturbCoincidentVertices();
  ~vtkPerturbCoincidentVertices();

  int RequestData(
    vtkInformation*, 
    vtkInformationVector**, 
    vtkInformationVector*);

  vtkSmartPointer<vtkCoincidentPoints> CoincidentPoints;
  
private:
  vtkPerturbCoincidentVertices(const vtkPerturbCoincidentVertices&); // Not implemented
  void operator=(const vtkPerturbCoincidentVertices&);   // Not implemented
};

#endif

