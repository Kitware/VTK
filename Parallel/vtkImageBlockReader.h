/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageBlockReader.h
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
// .NAME vtkImageBlockReader - Breaks up image into blocks and save in files.
// .SECTION Description
// Experimenting with different file formats. This one saves an image in 
// multiple files.  I am allowing overlap between file for efficiency.

// .SECTION see also
// vtkImageBlockReader.

#ifndef __vtkImageBlockReader_h
#define __vtkImageBlockReader_h

#include "vtkImageSource.h"


class VTK_PARALLEL_EXPORT vtkImageBlockReader : public vtkImageSource
{
public:
  static vtkImageBlockReader *New();
  vtkTypeRevisionMacro(vtkImageBlockReader,vtkImageSource);
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
  // Although this information could be gotten from the files, this is easy.
  vtkSetVector6Macro(WholeExtent, int);
  vtkGetVector6Macro(WholeExtent, int);

  // Description:
  // Although this information could be gotten from the files, this is easy.
  vtkSetMacro(NumberOfScalarComponents, int);  
  vtkGetMacro(NumberOfScalarComponents, int);  
  
  // Description:
  // Although this information could be gotten from the files, this is easy.
  vtkSetMacro(ScalarType, int);  
  vtkGetMacro(ScalarType, int);  
  
  // Description:
  // This printf pattern should take three integers, one for each axis.
  vtkSetStringMacro(FilePattern);
  vtkGetStringMacro(FilePattern);


  
protected:
  vtkImageBlockReader();
  ~vtkImageBlockReader();
  
  char *FilePattern;

  int WholeExtent[6];
  int NumberOfScalarComponents;
  int ScalarType;
  int Divisions[3];
  int Overlap;


  // Description:
  // Write the files.
  void Execute(vtkImageData *data);
  void ExecuteInformation();

  // This method computes the XYZExtents.
  void ComputeBlockExtents();
  void DeleteBlockExtents();

  // helper methods
  void Read(vtkImageData *data, int *ext);
  void ReadRemainder(vtkImageData *data, int *ext, int *doneExt);
  void ReadBlock(int xIdx, int yIdx, int zIdx, 
                 vtkImageData *data, int *ext);

  // Description:
  // Generate more than requested.  Called by the superclass before
  // an execute, and before output memory is allocated.
  void ModifyOutputUpdateExtent();

  // extents (min, max) of the divisions.
  int *XExtents;
  int *YExtents;
  int *ZExtents;
private:
  vtkImageBlockReader(const vtkImageBlockReader&);  // Not implemented.
  void operator=(const vtkImageBlockReader&);  // Not implemented.
};


#endif


