/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataSetGradientPrecompute.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkDataSetGradientPrecompute 

// .SECTION Thanks
// <verbatim>
//
// This file is part of the generalized Youngs material interface reconstruction algorithm contributed by
// CEA/DIF - Commissariat a l'Energie Atomique, Centre DAM Ile-De-France
// BP12, F-91297 Arpajon, France
// 
// Implementation by Thierry Carrard (CEA)
//
// </verbatim>

#ifndef __vtkDataSetGradientPrecompute_h
#define __vtkDataSetGradientPrecompute_h

#include <vtkObjectFactory.h>
#include <vtkSetGet.h>
#include <vtkDataSetAlgorithm.h>

class VTK_GRAPHICS_EXPORT vtkDataSetGradientPrecompute : public vtkDataSetAlgorithm
{
   public:

      static vtkDataSetGradientPrecompute* New();
      vtkTypeRevisionMacro(vtkDataSetGradientPrecompute,vtkDataSetAlgorithm);
      ~vtkDataSetGradientPrecompute ();

   protected:
      vtkDataSetGradientPrecompute ();
      virtual int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

   private:
     //! Unimplemented copy constructor
      vtkDataSetGradientPrecompute (const vtkDataSetGradientPrecompute &);

     //! Unimplemented operator
      vtkDataSetGradientPrecompute & operator= (const vtkDataSetGradientPrecompute &);
} ;

#endif /* VTK_DATA_SET_GRADIENT_PRECOMPUTE_H */

