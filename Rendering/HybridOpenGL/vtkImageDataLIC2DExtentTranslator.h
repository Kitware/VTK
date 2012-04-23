/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageDataLIC2DExtentTranslator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkImageDataLIC2DExtentTranslator
// .SECTION Description

#ifndef __vtkImageDataLIC2DExtentTranslator_h
#define __vtkImageDataLIC2DExtentTranslator_h

#include "vtkRenderingHybridOpenGLModule.h" // For export macro
#include "vtkExtentTranslator.h"
#include "vtkWeakPointer.h" // needed for vtkWeakPointer.

class vtkImageDataLIC2D;

class VTKRENDERINGHYBRIDOPENGL_EXPORT vtkImageDataLIC2DExtentTranslator
  : public vtkExtentTranslator
{
public:
  static vtkImageDataLIC2DExtentTranslator* New();
  vtkTypeMacro(vtkImageDataLIC2DExtentTranslator, vtkExtentTranslator);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the vtkImageDataLIC2D algorithm for which this extent translator is
  // being used.
  void SetAlgorithm(vtkImageDataLIC2D*);
  vtkImageDataLIC2D* GetAlgorithm();

  void SetInputExtentTranslator(vtkExtentTranslator*);
  vtkGetObjectMacro(InputExtentTranslator, vtkExtentTranslator);

  vtkSetVector6Macro(InputWholeExtent, int);
  vtkGetVector6Macro(InputWholeExtent, int);

  virtual int PieceToExtentThreadSafe(int piece, int numPieces,
                                     int ghostLevel, int *wholeExtent,
                                     int *resultExtent, int splitMode,
                                     int byPoints);

//BTX
protected:
  vtkImageDataLIC2DExtentTranslator();
  ~vtkImageDataLIC2DExtentTranslator();

  int InputWholeExtent[6];
  vtkExtentTranslator* InputExtentTranslator;
  vtkWeakPointer<vtkImageDataLIC2D> Algorithm;
private:
  vtkImageDataLIC2DExtentTranslator(const vtkImageDataLIC2DExtentTranslator&); // Not implemented.
  void operator=(const vtkImageDataLIC2DExtentTranslator&); // Not implemented.
//ETX
};

#endif
