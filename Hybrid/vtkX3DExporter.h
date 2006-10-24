/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkX3DExporter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtX3DExporter - create an x3d file
// .SECTION Description

#ifndef __vtkX3DExporter_h
#define __vtkX3DExporter_h

#include "vtkExporter.h"

class vtkLight;
class vtkActor;
class vtkActor2D;
class vtkPoints;
class vtkDataArray;
class vtkUnsignedCharArray;


class  VTK_HYBRID_EXPORT vtkX3DExporter : public vtkExporter
{
public:
  static vtkX3DExporter *New();
  vtkTypeRevisionMacro(vtkX3DExporter,vtkExporter);
  void PrintSelf(ostream& os, vtkIndent indent);


  // Description:
  // Set/Get the output file name.
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);

  // Description:
  // Specify the Speed of navigation. Default is 4.
  vtkSetMacro(Speed,double);
  vtkGetMacro(Speed,double);
  
  
protected:
  vtkX3DExporter();
  ~vtkX3DExporter();  
  
  // Description:
  // Write data to output.
  void WriteData();

  void WriteALight(vtkLight *aLight, FILE *fp);
  void WriteAnActor(vtkActor *anActor, FILE *fp,int index);
  void WritePointData(vtkPoints *points, vtkDataArray *normals, 
                      vtkDataArray *tcoords, vtkUnsignedCharArray *colors, 
                      FILE *fp,int index);
  void WriteanTextActor2D(vtkActor2D *anTextActor2D, FILE *fp);          
            
  char *FileName;
  double Speed;
  
private:
  vtkX3DExporter(const vtkX3DExporter&); // Not implemented.
  void operator=(const vtkX3DExporter&); // Not implemented.
};


#endif
