/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageShortWriter.h
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
// .NAME vtkImageShortWriter - Writes no header short files.
// .SECTION Description
// vtkImageShortWriter writes no header short files. Format can either be
// unsigned or signed, and byte swapped of not byte swapped.
// The writer will stream to get its input if the InputMemoryLimit 
// is set small.


#ifndef __vtkImageShortWriter_h
#define __vtkImageShortWriter_h

#include <iostream.h>
#include <fstream.h>
#include "vtkObject.h"
#include "vtkImageCache.h"
#include "vtkStructuredPointsToImage.h"
#include "vtkImageRegion.h"

class VTK_EXPORT vtkImageShortWriter : public vtkObject
{
public:
  vtkImageShortWriter();
  ~vtkImageShortWriter();
  static vtkImageShortWriter *New() {return new vtkImageShortWriter;};
  char *GetClassName() {return "vtkImageShortWriter";};
  void PrintSelf(ostream& os, vtkIndent indent);
  
  void SetFilePrefix(char *filePrefix);
  void SetFilePattern(char *filePattern);
  
  // Description:
  // Set/Get the input object from the image pipeline.
  vtkSetObjectMacro(Input,vtkImageCache);
  vtkGetObjectMacro(Input,vtkImageCache);
  void SetInput(vtkStructuredPoints *spts)
    {this->SetInput(spts->GetStructuredPointsToImage()->GetOutput());}
  // Description:
  // Set/Get the flag that tells the object to save the whole image or not.
  vtkSetMacro(WholeImage,int);
  vtkGetMacro(WholeImage,int);
  vtkBooleanMacro(WholeImage,int);

  // Set/Get the extent to save explicitely.
  void SetExtent(int dim, int *extent);
  vtkImageSetExtentMacro(Extent);
  void GetExtent(int dim, int *extent);
  vtkImageGetExtentMacro(Extent);


  // Description:
  // Set/Get the coordinate system which determines how extent are interpreted.
  // Note: This does not yet change the order of the structured points!
  void SetAxes(int dim, int *axes);
  vtkImageSetMacro(Axes,int);
  void GetAxes(int dim, int *axes);
  vtkImageGetMacro(Axes,int);
  int *GetAxes() {return this->Axes;};  

  // Description:
  // Get the order of the axes to split while streaming.
  void GetSplitOrder(int dim, int *axes);
  vtkImageGetMacro(SplitOrder,int);
  int *GetSplitOrder() {return this->SplitOrder;};  
  
  // Description:
  // This object will stream to keep the input regions below this limit.
  vtkSetMacro(InputMemoryLimit,int);
  vtkGetMacro(InputMemoryLimit,int);
  
  // Description:
  // Determine whether shorts are saved in signed or unsigned binary formats.
  vtkSetMacro(Signed,int);
  vtkGetMacro(Signed,int);
  vtkBooleanMacro(Signed,int);
  
  // Description:
  // Determine whether buinary format has swapped bytes.
  vtkSetMacro(SwapBytes,int);
  vtkGetMacro(SwapBytes,int);
  vtkBooleanMacro(SwapBytes,int);
  
  // Description:
  // The main interface which triggers the writer to start.
  void Write();

  // Public for templated function
  char *FileName;
  int Signed;
  int SwapBytes;

protected:
  vtkImageCache *Input;
  int WholeImage;
  int Extent[VTK_IMAGE_EXTENT_DIMENSIONS];
  int Axes[VTK_IMAGE_DIMENSIONS];
  char *FilePrefix;
  char *FilePattern;
  int SplitOrder[VTK_IMAGE_DIMENSIONS];
  int InputMemoryLimit;

  void WriteRegion(vtkImageRegion *region);
  void WriteRegionData(vtkImageRegion *region);
  void WriteRegion2D(vtkImageRegion *region);
};

#endif


