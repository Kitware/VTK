/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataSetGradient.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkDataSetGradient - computes scalar field gradient
//
// .SECTION Description
// vtkDataSetGradient Computes per cell gradient of point scalar field
// or per point gradient of cell scalar field.
//
// .SECTION Thanks
// This file is part of the generalized Youngs material interface reconstruction algorithm contributed by
// CEA/DIF - Commissariat a l'Energie Atomique, Centre DAM Ile-De-France <br>
// BP12, F-91297 Arpajon, France. <br>
// Implementation by Thierry Carrard (CEA)


#ifndef __vtkDataSetGradient_h
#define __vtkDataSetGradient_h

#include <vtkObjectFactory.h>
#include <vtkSetGet.h>
#include <vtkDataSetAlgorithm.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>

class VTK_GRAPHICS_EXPORT vtkDataSetGradient : public vtkDataSetAlgorithm
{
   public:

      static vtkDataSetGradient* New();
      vtkTypeRevisionMacro(vtkDataSetGradient,vtkDataSetAlgorithm);
      ~vtkDataSetGradient();
      
     // Description:
     // Set/Get the name of computed vector array.
      vtkSetStringMacro(ResultArrayName);
      vtkGetStringMacro(ResultArrayName);

   protected:
      vtkDataSetGradient ();

      virtual int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

      char* ResultArrayName;

   private:

     //! Unimplemented copy constructor
      vtkDataSetGradient (const vtkDataSetGradient &);

     //! Unimplemented operator
      vtkDataSetGradient & operator= (const vtkDataSetGradient &);
} ;

#endif /* VTK_DATA_SET_GRADIENT_H */
