/*=========================================================================
  
  Program:   Visualization Toolkit
  Module:    vtkImageBlockWriter.h
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
// .NAME vtkImageBlockWriter - Breaks up image into blocks and save in files.
// .SECTION Description
// Experimenting with different file formats. This one saves an image in 
// multiple files.  I am allowing overlap between file for efficiency.

// .SECTION see also
// vtkImageBlockReader.

#ifndef __vtkImageBlockWriter_h
#define __vtkImageBlockWriter_h

#include "vtkProcessObject.h"
#include "vtkImageData.h"

class VTK_EXPORT vtkImageBlockWriter : public vtkProcessObject
{
public:
  static vtkImageBlockWriter *New();
  vtkTypeMacro(vtkImageBlockWriter,vtkProcessObject);
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
  vtkImageBlockWriter(const vtkImageBlockWriter&) {};
  void operator=(const vtkImageBlockWriter&) {};
  
  char *FilePattern;

  int Divisions[3];
  int Overlap;
};


#endif


