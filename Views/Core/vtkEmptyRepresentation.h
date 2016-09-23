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
/**
 * @class   vtkEmptyRepresentation
 *
 *
*/

#ifndef vtkEmptyRepresentation_h
#define vtkEmptyRepresentation_h

#include "vtkViewsCoreModule.h" // For export macro
#include "vtkDataRepresentation.h"
#include "vtkSmartPointer.h" // For SP ivars

class vtkConvertSelectionDomain;

class VTKVIEWSCORE_EXPORT vtkEmptyRepresentation : public vtkDataRepresentation
{
public:
  static vtkEmptyRepresentation* New();
  vtkTypeMacro(vtkEmptyRepresentation, vtkDataRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent);

  /**
   * Since this representation has no inputs, override superclass
   * implementation with one that ignores "port" and "conn" and still allows it
   * to have an annotation output.
   */
  virtual vtkAlgorithmOutput* GetInternalAnnotationOutputPort()
    { return this->GetInternalAnnotationOutputPort(0); }
  virtual vtkAlgorithmOutput* GetInternalAnnotationOutputPort(int port)
    { return this->GetInternalAnnotationOutputPort(port, 0); }
  virtual vtkAlgorithmOutput* GetInternalAnnotationOutputPort(int port, int conn);

protected:
  vtkEmptyRepresentation();
  ~vtkEmptyRepresentation();

private:
  vtkEmptyRepresentation(const vtkEmptyRepresentation&) VTK_DELETE_FUNCTION;
  void operator=(const vtkEmptyRepresentation&) VTK_DELETE_FUNCTION;

  vtkSmartPointer<vtkConvertSelectionDomain> ConvertDomains;

};

#endif

