/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtent.h
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
// .NAME vtkExtent - Generalizes imaging extents into graphics.
// .SECTION Description
// Note:  This object is under development an might change in the future.
// vtkExtent contains information to specify update extents of vtkDataSets.
// This is a superclass. Two subclasses exist.  One for structured data,
// and one for unstructured data.  The extent object will also indicate
// whether the request is for points or cells.

#ifndef __vtkExtent_h
#define __vtkExtent_h

#include "vtkObject.h"
class vtkStructuredExtent;
class vtkUnstructuredExtent;


class VTK_EXPORT vtkExtent : public vtkObject
{
public:
  static vtkExtent *New();

  vtkTypeMacro(vtkExtent,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Copy information from one extent into another.
  // Subclasses override this method, and try to be smart
  // if the types are different.
  virtual void Copy(vtkExtent *in);


  // Description:
  // We can use streaming to processes series of data sets one at a time.
  vtkSetMacro(SeriesIndex, int);
  vtkGetMacro(SeriesIndex, int);
  
  // Description:
  // Serialization provided for the multi-process ports.
  virtual void ReadSelf(istream& is);
  virtual void WriteSelf(ostream& os);

protected:
  vtkExtent();
  ~vtkExtent() {};
  vtkExtent(const vtkExtent&) {};
  void operator=(const vtkExtent&) {};

  int SeriesIndex;
};


#endif
