/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkLinearSelector.h

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkLinearSelector
 * @brief   select cells intersecting a line (possibly broken)
 *
 *
 * This filter takes a vtkCompositeDataSet as input and a line segment as parameter.
 * It outputs a vtkSelection identifying all the cells intersecting the given line segment.
 *
 * @par Thanks:
 * This class has been initially developed in the frame of CEA's Love visualization software development <br>
 * CEA/DIF - Commissariat a l'Energie Atomique, Centre DAM Ile-De-France <br>
 * BP12, F-91297 Arpajon, France. <br>
 * Modified and integrated into VTK, Kitware SAS 2012
 * This class was implemented by Thierry Carrard, Charles Pignerol, and Philippe Pebay.
*/

#ifndef vtkLinearSelector_h
#define vtkLinearSelector_h

#include "vtkFiltersSelectionModule.h" // For export macro
#include "vtkSelectionAlgorithm.h"

class vtkAlgorithmOutput;
class vtkDataSet;
class vtkDoubleArray;
class vtkIdTypeArray;
class vtkPoints;

class VTKFILTERSSELECTION_EXPORT vtkLinearSelector : public vtkSelectionAlgorithm
{
 public:
  vtkTypeMacro(vtkLinearSelector,vtkSelectionAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  static vtkLinearSelector* New();

  //@{
  /**
   * Set/Get starting point of intersecting segment
   */
  vtkSetVector3Macro(StartPoint,double);
  vtkGetVectorMacro(StartPoint,double,3);
  //@}

  //@{
  /**
   * Set/Get end point of intersecting segment
   */
  vtkSetVector3Macro(EndPoint,double);
  vtkGetVectorMacro(EndPoint,double,3);
  //@}

  //@{
  /**
   * Set/Get the list of points defining the intersecting broken line
   */
  virtual void SetPoints(vtkPoints*);
  vtkGetObjectMacro(Points,vtkPoints);
  //@}

  //@{
  /**
   * Set/Get tolerance to be used by intersection algorithm
   */
  vtkSetMacro(Tolerance,double);
  vtkGetMacro(Tolerance,double);
  //@}

  //@{
  /**
   * Set/Get whether lines vertice are included in selection
   */
  vtkSetMacro(IncludeVertices,bool);
  vtkGetMacro(IncludeVertices,bool);
  vtkBooleanMacro(IncludeVertices,bool);
  //@}

  //@{
  /**
   * Set/Get relative tolerance for vertex elimination
   */
  vtkSetClampMacro(VertexEliminationTolerance,double,0.,.1 );
  vtkGetMacro(VertexEliminationTolerance,double);
  //@}

 protected:
  vtkLinearSelector();
  virtual ~vtkLinearSelector();

  virtual int FillInputPortInformation(int port, vtkInformation *info);

  virtual int RequestData(vtkInformation *request,
                          vtkInformationVector **inputVector,
                          vtkInformationVector *outputVector);

  /**
   * The main routine that iterates over cells and looks for those that
   * intersect at least one of the segments of interest
   */
  void SeekIntersectingCells(vtkDataSet* input, vtkIdTypeArray* outIndices);

 private:
  vtkLinearSelector(const vtkLinearSelector&) VTK_DELETE_FUNCTION;
  void operator =(const vtkLinearSelector&) VTK_DELETE_FUNCTION;

  //@{
  /**
   * Start and end point of the intersecting line segment
   * NB: These are used if and only if Points is null.
   */
  double StartPoint[3];
  double EndPoint[3];
  //@}

  /**
   * The list of points defining the intersecting broken line
   * NB: The Start/EndPoint definition of a single line segment is used by default
   */
  vtkPoints* Points;

  /**
   * Tolerance to be used by intersection algorithm
   */
  double Tolerance;

  /**
   * Decide whether lines vertice are included in selection
   * Default: true
   */
  bool IncludeVertices;

  //@{
  /**
   * Relative tolerance for vertex elimination
   * Default: 1e-6
   */
  double VertexEliminationTolerance;
};
  //@}


#endif // vtkLinearSelector_h
