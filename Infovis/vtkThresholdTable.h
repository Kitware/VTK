/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkThresholdTable.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkThresholdTable - executes an sql query and retrieves results into a table
//
// .SECTION Description


#ifndef __vtkThresholdTable_h
#define __vtkThresholdTable_h

#include "vtkTableAlgorithm.h"
#include "vtkVariant.h" // For vtkVariant arguments

class VTK_INFOVIS_EXPORT vtkThresholdTable : public vtkTableAlgorithm
{
public:
  static vtkThresholdTable* New();
  vtkTypeRevisionMacro(vtkThresholdTable, vtkTableAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  //BTX
  enum {
    ACCEPT_LESS_THAN = 0,
    ACCEPT_GREATER_THAN = 1,
    ACCEPT_BETWEEN = 2,
    ACCEPT_OUTSIDE = 3
    };
  //ETX

  vtkSetClampMacro(Mode, int, 0, 3);
  vtkGetMacro(Mode, int);

  //BTX
  virtual void SetMinValue(vtkVariant v)
    {
    this->MinValue = v;
    this->Modified();
    }

  virtual vtkVariant GetMinValue()
    {
    return this->MinValue;
    }

  virtual void SetMaxValue(vtkVariant v)
    {
    this->MaxValue = v;
    this->Modified();
    }

  virtual vtkVariant GetMaxValue()
    {
    return this->MaxValue;
    }
  //ETX

  // Overloaded SetMinValue for use with VTK parallel server
  void SetMinValue(double v)
  {
    this->SetMinValue(vtkVariant(v));
  }

  // Overloaded SetMaxValue for use with VTK parallel server  
  void SetMaxValue(double v)
  {
    this->SetMaxValue(vtkVariant(v));
  }

protected:
  vtkThresholdTable();
  ~vtkThresholdTable();

  int RequestData(
    vtkInformation*, 
    vtkInformationVector**, 
    vtkInformationVector*);

  vtkVariant MinValue;
  vtkVariant MaxValue;
  int Mode;
    
private:
  vtkThresholdTable(const vtkThresholdTable&); // Not implemented
  void operator=(const vtkThresholdTable&);   // Not implemented
};

#endif

