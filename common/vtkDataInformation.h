/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataInformation.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2000 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

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
  static vtkDataInformation *New();
  vtkTypeMacro(vtkDataInformation,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Makes an empty similar type object.
  virtual vtkDataInformation *MakeObject() {return vtkDataInformation::New();}
  
  // Description:
  // Subclasses override this method, and try to be smart
  // if the types are different.
  virtual void Copy(vtkDataInformation *in);

  // Description:
  // This is a special value that may not be considered "DataInformation".
  // it is just convenient to compute this value in the UpdateInformation 
  // calls.  This value represents the mamimum MTimes of all upstream
  // pipeline objects (not including this data object itself).
  void SetPipelineMTime(unsigned long time) {this->PipelineMTime = time; }
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
  void SetLocality(float l) {this->Locality = l;};
  vtkGetMacro(Locality, float);
  
  // Description:
  // Sources that can generate a series of data objects can communicate
  // this down stream using this ivar.
  vtkSetMacro(SeriesLength, int);
  vtkGetMacro(SeriesLength, int);  
  
  // Description:
  // This method is passed a ClassName and returns 1 if the object is
  // a subclass of the class arg.  It is an attempt at making a smarter copy.
  virtual int GetClassCheck(char *className);

  // Description:
  // Serialization provided for the multi-process ports.
  virtual void ReadSelf(istream& is);
  virtual void WriteSelf(ostream& os);
  
protected:
  vtkDataInformation();
  ~vtkDataInformation() {};
  vtkDataInformation(vtkDataInformation&) {};
  void operator=(vtkDataInformation&) {};

  // A guess at how much memory would be consumed by the data object
  // if the WholeExtent were updated.
  unsigned long EstimatedWholeMemorySize;
  // The Maximum MTime of all upstreamg filters and data objects.
  // This does not include the MTime of this data object.
  unsigned long PipelineMTime;
  // How many upstream filters are local to the process.
  // This will have to change to a float for Kens definition of locality.
  float Locality;  
  // Support for processing series of data sets.
  int SeriesLength;
};


#endif
