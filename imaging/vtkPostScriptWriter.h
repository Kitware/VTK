/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPostScriptWriter.h
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
// .NAME vtkPostScriptWriter - Writes an image as a PostScript file.
// .SECTION Description
// vtkPostScriptWriter writes an image as a PostScript file using some
// reasonable scalings and centerd on the page which is assumed to be
// about 8.5 by 11 inches. This is based loosely off of the code from
// pnmtops.c. Right now there aren't any real options.


#ifndef __vtkPostScriptWriter_h
#define __vtkPostScriptWriter_h

#include "vtkImageWriter.h"

class VTK_EXPORT vtkPostScriptWriter : public vtkImageWriter
{
public:
  static vtkPostScriptWriter *New() {return new vtkPostScriptWriter;};
  const char *GetClassName() {return "vtkPostScriptWriter";};

protected:
  vtkPostScriptWriter() {};
  ~vtkPostScriptWriter() {};

  virtual void WriteFile(ofstream *file, vtkImageData *data, int extent[6]);
  virtual void WriteFileHeader(ofstream *, vtkImageData *);
  virtual void WriteFileTrailer(ofstream *, vtkImageData *);
};

#endif


