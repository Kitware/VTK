/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolyDataStreamer.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPolyDataStreamer - Stream appends input pieces to the output.
// .SECTION Description
// vtkPolyDataStreamer initiates streaming by requesting pieces from its 
// single input it appends these pieces it to the requested output.
// Note that since vtkPolyDataStreamer uses an append filter, all the
// polygons generated have to be kept in memory before rendering. If
// these do not fit in the memory, it is possible to make the vtkPolyDataMapper
// stream. Since the mapper will render each piece separately, all the
// polygons do not have to stored in memory.
// .SECTION Note
// The output may be slightly different if the pipeline does not handle 
// ghost cells properly (i.e. you might see seames between the pieces).
// .SECTION See Also
// vtkAppendFilter

#ifndef __vtkPolyDataStreamer_h
#define __vtkPolyDataStreamer_h

#include "vtkPolyDataAlgorithm.h"

class VTK_GRAPHICS_EXPORT vtkPolyDataStreamer : public vtkPolyDataAlgorithm
{
public:
  static vtkPolyDataStreamer *New();

  vtkTypeMacro(vtkPolyDataStreamer,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the number of pieces to divide the problem into.
  void SetNumberOfStreamDivisions(int num);
  vtkGetMacro(NumberOfStreamDivisions,int);
  
  // Description:
  // By default, this option is off.  When it is on, cell scalars are generated
  // based on which piece they are in.
  vtkSetMacro(ColorByPiece, int);
  vtkGetMacro(ColorByPiece, int);
  vtkBooleanMacro(ColorByPiece, int);


protected:
  vtkPolyDataStreamer();
  ~vtkPolyDataStreamer();
  
  // Append the pieces.
  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  int RequestUpdateExtent(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  
  int NumberOfStreamDivisions;
  int ColorByPiece;
private:
  vtkPolyDataStreamer(const vtkPolyDataStreamer&);  // Not implemented.
  void operator=(const vtkPolyDataStreamer&);  // Not implemented.
};

#endif
