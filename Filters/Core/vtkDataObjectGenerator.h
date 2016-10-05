/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataObjectGenerator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkDataObjectGenerator
 * @brief   produces simple (composite or atomic) data
 * sets for testing.
 *
 * vtkDataObjectGenerator parses a string and produces dataobjects from the
 * dataobject template names it sees in the string. For example, if the string
 * contains "ID1" the generator will create a vtkImageData. "UF1", "RG1",
 * "SG1", "PD1", and "UG1" will produce vtkUniformGrid, vtkRectilinearGrid,
 * vtkStructuredGrid, vtkPolyData and vtkUnstructuredGrid respectively.
 * "PD2" will produce an alternate vtkPolyData. You
 * can compose composite datasets from the atomic ones listed above
 * by placing them within one of the two composite dataset identifiers
 * - "MB{}" or "HB[]". "MB{ ID1 PD1 MB{} }" for example will create a
 * vtkMultiBlockDataSet consisting of three blocks: image data, poly data,
 * multi-block (empty). Hierarchical Box data sets additionally require
 * the notion of groups, declared within "()" braces, to specify AMR depth.
 * "HB[ (UF1)(UF1)(UF1) ]" will create a vtkHierarchicalBoxDataSet representing
 * an octree that is three levels deep, in which the firstmost cell in each level
 * is refined.
*/

#ifndef vtkDataObjectGenerator_h
#define vtkDataObjectGenerator_h

#include "vtkFiltersCoreModule.h" // For export macro
#include "vtkDataObjectAlgorithm.h"

class vtkInternalStructureCache;

class VTKFILTERSCORE_EXPORT vtkDataObjectGenerator
: public vtkDataObjectAlgorithm
{
 public:
  static vtkDataObjectGenerator *New();
  vtkTypeMacro(vtkDataObjectGenerator,vtkDataObjectAlgorithm);
  void PrintSelf(ostream &os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * The string that will be parsed to specify a dataobject structure.
   */
  vtkSetStringMacro(Program);
  vtkGetStringMacro(Program);
  //@}

protected:
  vtkDataObjectGenerator();
  ~vtkDataObjectGenerator() VTK_OVERRIDE;

  int RequestData(vtkInformation *req,
                  vtkInformationVector **inV,
                  vtkInformationVector *outV) VTK_OVERRIDE;
  int RequestDataObject(vtkInformation *req,
                  vtkInformationVector **inV,
                  vtkInformationVector *outV) VTK_OVERRIDE;
  int RequestInformation(vtkInformation *req,
                  vtkInformationVector **inV,
                  vtkInformationVector *outV) VTK_OVERRIDE;
  int RequestUpdateExtent(vtkInformation *req,
                  vtkInformationVector **inV,
                  vtkInformationVector *outV) VTK_OVERRIDE;

  //the string to parse to create a structure
  char *Program;
  //a record of the structure
  vtkInternalStructureCache *Structure;

  //Helper for RequestDataObject
  vtkDataObject *
    CreateOutputDataObjects(vtkInternalStructureCache *structure);
  //Helper for RequestData
  vtkDataObject *
    FillOutputDataObjects(vtkInternalStructureCache *structure,
                          int level,
                          int stripe=0);

  //to determine which composite data stripe to fill in
  vtkIdType Rank;
  vtkIdType Processors;

  //create the templated atomic data sets
  void MakeImageData1(vtkDataSet *ds);
  void MakeImageData2(vtkDataSet *ds);
  void MakeUniformGrid1(vtkDataSet *ds);
  void MakeRectilinearGrid1(vtkDataSet *ds);
  void MakeStructuredGrid1(vtkDataSet *ds);
  void MakePolyData1(vtkDataSet *ds);
  void MakePolyData2(vtkDataSet *ds);
  void MakeUnstructuredGrid1(vtkDataSet *ds);
  void MakeUnstructuredGrid2(vtkDataSet *ds);
  void MakeUnstructuredGrid3(vtkDataSet *ds);
  void MakeUnstructuredGrid4(vtkDataSet *ds);

  //used to spatially separate sub data sets within composites
  double XOffset; //increases for each dataset index
  double YOffset; //increases for each sub data set
  double ZOffset; //increases for each group index

  //used to filling in point and cell values with unique Ids
  vtkIdType CellIdCounter;
  vtkIdType PointIdCounter;

  //assign point and cell values to each point and cell
  void MakeValues(vtkDataSet *ds);

private:
  vtkDataObjectGenerator(const vtkDataObjectGenerator&) VTK_DELETE_FUNCTION;
  void operator=(const vtkDataObjectGenerator&) VTK_DELETE_FUNCTION;
};

#endif
