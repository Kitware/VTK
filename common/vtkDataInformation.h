/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataInformation.h
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
// .NAME vtkDataInformation - Superclass of information objects.
// .SECTION Description
// Note:  This object is under development an might change in the future.
// This class and its sublcasses encapsulate the information associated
// with vtkData objects into there own objects.  The primary motivation 
// for this division is for pipeline ports between mutiple processes.

#ifndef __vtkDataInformation_h
#define __vtkDataInformation_h

#include "vtkObject.h"


class VTK_EXPORT vtkDataInformation : public vtkObject
{
public:
  static vtkDataInformation *New() {return new vtkDataInformation;};
  const char *GetClassName() {return "vtkDataInformation";}
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Subclasses override this method, and try to be smart
  // if the types are different.
  virtual void Copy(vtkDataInformation *in);

  // Description:
  // This is a special value that may not be considered "DataInformation".
  // it is just convenient to compute this value in the UpdateInformation 
  // calls.  This value represents the mamimum MTimes of all upstream
  // pipeline objects (not including this data object itself).
  vtkSetMacro(PipelineMTime, unsigned long);
  vtkGetMacro(PipelineMTime, unsigned long);

  // Description:
  // This is set to the estimated size of the data object (in Kb)
  // if the whole extent were updated.
  // Setting this value does not change MTime because this value
  // is automatically computed in 
  // vtkImageData::ComputeEstimatedWholeMemorySize.
  void SetEstimatedWholeMemorySize(unsigned long size)
    {this->EstimatedWholeMemorySize = size;};
  vtkGetMacro(EstimatedWholeMemorySize, unsigned long);
  
  // Description:
  // Locality is a measure of how many filters (in this same process)
  // are upstream of this filter.  Alternatively, it is a crude measure
  // of how long the processing should take to update our data.
  // It is used to sort Update requests in multiple input filters to
  // get the best possible parallel perfomance.
  vtkSetMacro(Locality, int);
  vtkGetMacro(Locality, int);
  
  // Description:
  // Sources that can generate a series of data objects can communicate
  // this down stream using this ivar.
  vtkSetMacro(SeriesLength, int);
  vtkGetMacro(SeriesLength, int);  
  
  // Description:
  // This method is passed a ClassName and returns 1 if the object is
  // a subclass of the class arg.  It is an attempt at making a smarter copy.
  virtual int GetClassCheck(char *className);
  
protected:
  
  vtkDataInformation();
  ~vtkDataInformation() {};

  // A guess at how much memory would be consumed by the data object
  // if the WholeExtent were updated.
  unsigned long EstimatedWholeMemorySize;
  // The Maximum MTime of all upstreamg filters and data objects.
  // This does not include the MTime of this data object.
  unsigned long PipelineMTime;
  // How many upstream filters are local to the process.
  // This will have to change to a float for Kens definition of locality.
  int Locality;  
  // Support for processing series of data sets.
  int SeriesLength;
};


#endif
