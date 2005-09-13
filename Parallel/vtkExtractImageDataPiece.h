/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractImageDataPiece.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkExtractImageDataPiece - Takes in ImageData and extracts a
// region within, producing another ImageData.

#ifndef __vtkExtractImageDataPiece_h
#define __vtkExtractImageDataPiece_h

#include "vtkImageAlgorithm.h"

class VTK_PARALLEL_EXPORT vtkExtractImageDataPiece : public vtkImageAlgorithm
{
public:
  static vtkExtractImageDataPiece *New();
  vtkTypeRevisionMacro(vtkExtractImageDataPiece, vtkImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);
    
protected:
  vtkExtractImageDataPiece() {};
  ~vtkExtractImageDataPiece() {};

  virtual int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  virtual int RequestUpdateExtent(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
   
private:
  vtkExtractImageDataPiece(const vtkExtractImageDataPiece&);  // Not implemented.
  void operator=(const vtkExtractImageDataPiece&);  // Not implemented.
};

#endif
