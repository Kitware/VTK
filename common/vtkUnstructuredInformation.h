/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkUnstructuredInformation.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2000 Ken Martin, Will Schroeder, Bill Lorensen.

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
// .NAME vtkUnstructuredInformation - Only MaximumNumberOfPieces for now..
// .SECTION Description
// Note:  This object is under development an might change in the future.
// This class contains all the information specific to unstructured
// data sets.  The only thing it contians now is the
// mamximum number of pieces streaming can request.


#ifndef __vtkUnstructuredInformation_h
#define __vtkUnstructuredInformation_h

#include "vtkDataInformation.h"


class VTK_EXPORT vtkUnstructuredInformation : public vtkDataInformation
{
public:
  static vtkUnstructuredInformation *New();

  vtkTypeMacro(vtkUnstructuredInformation,vtkDataInformation);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Makes an empty similar type object.
  vtkDataInformation *MakeObject() 
    {return vtkUnstructuredInformation::New();}
  
  // Description:
  // Subclasses override this method, and try to be smart
  // if the types are different.
  void Copy(vtkDataInformation *in);

  // Description:
  // This is the maximum number of pieces that can be requested.
  // Requesting more than this value will give you no data.
  // This variable is going to be moved into vtkUnstructuredInformation.
  vtkSetMacro(MaximumNumberOfPieces, unsigned long);
  vtkGetMacro(MaximumNumberOfPieces, unsigned long);
  
  // Description:
  // This method is passed a ClassName and returns 1 if the object is
  // a subclass of the class arg.  It is an attempt at making a smarter copy.
  int GetClassCheck(char *className);
  
  // Description:
  // Serialization provided for the multi-process ports.
  void ReadSelf(istream& is);
  void WriteSelf(ostream& os);

protected:
  vtkUnstructuredInformation();
  ~vtkUnstructuredInformation() {};
  vtkUnstructuredInformation(vtkUnstructuredInformation&) {};
  void operator=(vtkUnstructuredInformation&) {};

  // This tells down stream filters the smallest resolution available 
  // for streaming/spliting.  Now this is sort of a whole extent
  // for unstructured data, and should not be part 
  // of the information of vtkDataObject...
  unsigned long MaximumNumberOfPieces;

};


#endif
