/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageBlockWriter.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkImageBlockWriter - Breaks up image into blocks and save in files.
// .SECTION Description
// Experimenting with different file formats. This one saves an image in 
// multiple files.  I am allowing overlap between file for efficiency.

// .SECTION see also
// vtkImageBlockReader.

#ifndef __vtkImageBlockWriter_h
#define __vtkImageBlockWriter_h

#include "vtkProcessObject.h"

class vtkImageData;

class VTK_PARALLEL_EXPORT vtkImageBlockWriter : public vtkProcessObject
{
public:
  static vtkImageBlockWriter *New();
  vtkTypeRevisionMacro(vtkImageBlockWriter,vtkProcessObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // The whole extent is broken up into this many divisions along each axis.
  vtkSetVector3Macro(Divisions, int);
  vtkGetVector3Macro(Divisions, int);

  // Description:
  // The number of points along any axis that belong to more than one piece.
  vtkSetMacro(Overlap, int);
  vtkGetMacro(Overlap, int);

  // Description:
  // This writer takes images as input.
  void SetInput(vtkImageData *input);
  vtkImageData *GetInput();
  
  // Description:
  // This printf pattern should take three integers, one for each axis.
  vtkSetStringMacro(FilePattern);
  vtkGetStringMacro(FilePattern);

  // Description:
  // Write the files.
  void Write();

  
protected:
  vtkImageBlockWriter();
  ~vtkImageBlockWriter();
  
  char *FilePattern;

  int Divisions[3];
  int Overlap;
private:
  vtkImageBlockWriter(const vtkImageBlockWriter&);  // Not implemented.
  void operator=(const vtkImageBlockWriter&);  // Not implemented.
};


#endif


