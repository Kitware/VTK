/*=========================================================================
  
  Program:   Visualization Toolkit
  Module:    vtkImageBlockReader.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  
Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

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
// .NAME vtkImageBlockReader - Breaks up image into blocks and save in files.
// .SECTION Description
// Experimenting with different file formats. This one saves an image in 
// multiple files.  I am allowing overlap between file for efficiency.

// .SECTION see also
// vtkImageBlockReader.

#ifndef __vtkImageBlockReader_h
#define __vtkImageBlockReader_h

#include "vtkImageSource.h"


class VTK_EXPORT vtkImageBlockReader : public vtkImageSource
{
public:
  static vtkImageBlockReader *New() {return new vtkImageBlockReader;};
  const char *GetClassName() {return "vtkImageBlockReader";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // The whole extent is broken up into this many divisions along each axis.
  vtkSetVector3Macro(Divisions, int);
  vtkGetVector3Macro(Divisions, int);

  // Description:
  // The number of points along any axis that belong to more than one peice.
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
  vtkImageBlockReader(const vtkImageBlockReader&) {};
  void operator=(const vtkImageBlockReader&) {};
  
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
};


#endif


