/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractTensorComponents.cxx
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
#include "vtkExtractTensorComponents.h"
#include "vtkMath.h"
#include "vtkFloatScalars.h"
#include "vtkFloatVectors.h"
#include "vtkFloatNormals.h"
#include "vtkFloatTCoords.h"

// Description:
// Construct object to extract nothing and to not pass tensor data
// through the pipeline.
vtkExtractTensorComponents::vtkExtractTensorComponents()
{
  this->PassTensorsToOutput = 0;

  this->ExtractScalars = 0;
  this->ExtractVectors = 0;
  this->ExtractNormals = 0;
  this->ExtractTCoords = 0;

  this->ScalarComponents[0] = this->ScalarComponents[1] = 0;

  this->VectorComponents[0] = 0; this->VectorComponents[1] = 0;
  this->VectorComponents[2] = 1; this->VectorComponents[3] = 0;
  this->VectorComponents[4] = 2; this->VectorComponents[5] = 0;

  this->NormalizeNormals = 1;
  this->NormalComponents[0] = 0; this->NormalComponents[1] = 1;
  this->NormalComponents[2] = 1; this->NormalComponents[3] = 1;
  this->NormalComponents[4] = 2; this->NormalComponents[5] = 1;

  this->NumberOfTCoords = 2;
  this->TCoordComponents[0] = 0; this->TCoordComponents[1] = 2;
  this->TCoordComponents[2] = 1; this->TCoordComponents[3] = 2;
  this->TCoordComponents[4] = 2; this->TCoordComponents[5] = 2;
}

// Extract data from tensors.
//
void vtkExtractTensorComponents::Execute()
{
  //
  // Initialize
  //
  vtkDebugMacro(<<"Extracting vector components!");

}

void vtkExtractTensorComponents::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkDataSetToDataSetFilter::PrintSelf(os,indent);

  os << indent << "Pass Tensors To Output: " << (this->PassTensorsToOutput ? "On\n" : "Off\n");

  os << indent << "Extract Scalars: " << (this->ExtractScalars ? "On\n" : "Off\n");
  os << indent << "Scalar Components: \n";
  os << indent << "  (row,column)0: (" 
     << this->ScalarComponents[0] << ", " << this->ScalarComponents[1] << ")\n";

  os << indent << "Extract Vectors: " << (this->ExtractVectors ? "On\n" : "Off\n");
  os << indent << "Vector Components: \n";
  os << indent << "  (row,column)0: (" 
     << this->VectorComponents[0] << ", " << this->VectorComponents[1] << ")\n";
  os << indent << "  (row,column)1: (" 
     << this->VectorComponents[2] << ", " << this->VectorComponents[3] << ")\n";
  os << indent << "  (row,column)2: (" 
     << this->VectorComponents[4] << ", " << this->VectorComponents[5] << ")\n";

  os << indent << "Extract Normals: " << (this->ExtractNormals ? "On\n" : "Off\n");
  os << indent << "Normalize Normals: " << (this->NormalizeNormals ? "On\n" : "Off\n");
  os << indent << "Normal Components: \n";
  os << indent << "  (row,column)0: (" 
     << this->NormalComponents[0] << ", " << this->NormalComponents[1] << ")\n";
  os << indent << "  (row,column)1: (" 
     << this->NormalComponents[2] << ", " << this->NormalComponents[3] << ")\n";
  os << indent << "  (row,column)2: (" 
     << this->NormalComponents[4] << ", " << this->NormalComponents[5] << ")\n";

  os << indent << "Extract TCoords: " << (this->ExtractTCoords ? "On\n" : "Off\n");
  os << indent << "Number Of TCoords: (" << this->NumberOfTCoords << ")\n";
  os << indent << "TCoord Components: \n";
  os << indent << "  (row,column)0: (" 
     << this->TCoordComponents[0] << ", " << this->TCoordComponents[1] << ")\n";
  os << indent << "  (row,column)1: (" 
     << this->TCoordComponents[2] << ", " << this->TCoordComponents[3] << ")\n";
  os << indent << "  (row,column)2: (" 
     << this->TCoordComponents[4] << ", " << this->TCoordComponents[5] << ")\n";

}
