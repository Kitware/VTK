/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageSeriesReader.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to C. Charles Law who developed this class.

Copyright (c) 1993-1995 Ken Martin, Will Schroeder, Bill Lorensen.

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
// .NAME vtkImageSeriesReader - Reads a series of 2D images.
// .SECTION Description
// vtkImageSeriesReader will read a volume from a series of 2D images.


#ifndef __vtkImageSeriesReader_h
#define __vtkImageSeriesReader_h

#include "vtkImageReader.h"

class VTK_EXPORT vtkImageSeriesReader : public vtkImageReader
{
public:
  vtkImageSeriesReader();
  ~vtkImageSeriesReader();
  static vtkImageSeriesReader *New() {
    cerr << "Please use vtkImageVolume16Reader instead of vtkImageSeriesReader.  The methods are the same, so a simple script change should do.";
    return new vtkImageSeriesReader;};
  char *GetClassName() {return "vtkImageSeriesReader";};
  void PrintSelf(ostream& os, vtkIndent indent);   

  void SetFilePrefix(char *filePrefix);
  void SetFilePattern(char *filePattern);

  // Description:
  // Get the number of the first image 
  // (i.e. do the files start with the number 0 or 1?).
  vtkSetMacro(First,int);
  vtkGetMacro(First,int);
  
  void SetImageRange(int start, int end);

  // Description:
  // This variable indicates how many dimensions are stored in a single file.
  // In most cases, 2D images are stored in a file, but color can add
  // a third dimension.
  vtkSetMacro(FileDimensionality, int);
  vtkGetMacro(FileDimensionality, int);
  
protected:
  
  char *FilePrefix;
  char *FilePattern;
  // The first image file has this index
  int First;

  // How are the dimensions split between file and series.
  // Number of dimensions within a single file.
  // The rest are split up in the series.
  int FileDimensionality;

  void Initialize();
  void Execute(vtkImageRegion *outRegion);    
};

#endif


