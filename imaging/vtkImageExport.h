/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageExport.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to David G. Gobbi who developed this class.


Copyright (c) 1993-1999 Ken Martin, Will Schroeder, Bill Lorensen.

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
// .NAME vtkImageExport - Writes images to files.
// .SECTION Description
// vtkImageExport writes images to files with any data type. The data type of
// the file is the same scalar type as the input.  The dimensionality
// determines whether the data will be written in one or multiple files.
// This class is used as the superclass of most image writing classes 
// such as vtkBMPExport etc. It supports streaming.

#ifndef __vtkImageExport_h
#define __vtkImageExport_h

#include <iostream.h>
#include <fstream.h>
#include "vtkProcessObject.h"
#include "vtkImageFlip.h"

class VTK_EXPORT vtkImageExport : public vtkProcessObject
{
public:
  vtkImageExport();
  ~vtkImageExport();
  static vtkImageExport *New() {return new vtkImageExport;};
  const char *GetClassName() {return "vtkImageExport";};
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Get the number of bytes required (for safety checks, etc)
  int GetDataMemorySize();

  // Description:
  // Get the (x,y,z) index dimensions of the data 
  // (warning: C arrays are indexed like this: array[z][y][x]) 
  void GetDataDimensions(int *ptr);
  int *GetDataDimensions() { 
    this->GetDataDimensions(this->DataDimensions);
    return this->DataDimensions; }

  // Description:
  // Get the number of scalar components of the data
  int GetDataNumberOfScalarComponents() {
    this->GetInput()->UpdateInformation();
    return this->GetInput()->GetNumberOfScalarComponents(); };

  // Description: 
  // Get misc. information about the data
  int *GetDataExtent() {   
    this->GetInput()->UpdateInformation();
    return this->GetInput()->GetWholeExtent(); };
  void GetDataExtent(int *ptr) {   
    this->GetInput()->UpdateInformation();
    this->GetInput()->GetWholeExtent(ptr); };
  float *GetDataSpacing() { 
    this->GetInput()->UpdateInformation();
    return this->GetInput()->GetSpacing(); };
  void GetDataSpacing(float *ptr) { 
    this->GetInput()->UpdateInformation();
    this->GetInput()->GetSpacing(ptr); };
  float *GetDataOrigin() { 
    this->GetInput()->UpdateInformation();
    return this->GetInput()->GetOrigin(); };
  void GetDataOrigin(float *ptr) { 
    this->GetInput()->UpdateInformation();
    this->GetInput()->GetOrigin(ptr); };
  int GetDataScalarType() {
    this->GetInput()->UpdateInformation();
    return this->GetInput()->GetScalarType(); };

  // Description:
  // Set/Get the input object from the image pipeline.
  void SetInput(vtkImageData *input);
  vtkImageData *GetInput();

  // Description:
  // Set/Get whether the data goes to the exported memory starting 
  // in the lower left corner or upper left corner.
  vtkBooleanMacro(ImageLowerLeft, int);
  vtkGetMacro(ImageLowerLeft, int);
  vtkSetMacro(ImageLowerLeft, int);
  
  // Description:
  // The main interface: export to the memory pointed to
  virtual void Export(void *);

  // Description:
  // An alternative to Export(): Use with caution. 
  // Get a pointer to the image memory
  // (the pointer might be different each time this is called)
  void *GetPointerToData();

protected:
  vtkImageFlip *ImageFlip;

  int ImageLowerLeft;
  int DataDimensions[3];

  void FinalExport(vtkImageData *data, int extent[6],
		   void **output);
  void RecursiveExport(int axis, vtkImageData *data,
		       void **output);
};

#endif


