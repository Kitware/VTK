/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractGrid.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkExtractStructuredGridHelper - helper for extracting/sub-sampling
//  structured datasets.
//
// .SECTION Description
// vtkExtractStructuredGridHelper provides some common functionality that is
// used by filters that extract and sub-sample structured data. Specifically,
// it provides functionality for calculating the mapping from the input extent
// of each process to the output extent.
//
// .SECTION See Also
// vtkExtractGrid vtkExtractVOI vtkExtractRectilinearGrid

#ifndef __vtkExtractStructuredGridHelper_h
#define __vtkExtractStructuredGridHelper_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkObject.h"

// Forward declarations
class vtkCellData;
class vtkPointData;
class vtkPoints;

namespace vtk
{
namespace detail
{

struct vtkIndexMap;

} // END namespace detail
} // END namespace vtk

class VTKCOMMONDATAMODEL_EXPORT vtkExtractStructuredGridHelper :
  public vtkObject
{
public:
  static vtkExtractStructuredGridHelper *New();
  vtkTypeMacro(vtkExtractStructuredGridHelper,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Get & Set Macros
  vtkGetVector6Macro(OutputWholeExtent,int);

  // Description:
  // \brief Initializes the index map.
  // \param voi the extent of the volume of interest
  // \param wholeExt the whole extent of the domain
  // \param smapleRate the sampling rate
  // \param includeBoundary indicates whether to include the boundary or not.
  void Initialize(int voi[6], int wholeExt[6], int sampleRate[3],
                  bool includeBoundary);

  // Description:
  // \brief Returns the size along a given dimension
  // \param dim the dimension in query
  // \pre dim >= 0 && dim < 3
  int GetSize(const int dim);

  // Description
  // \brief Returns the ith index along the given dimension
  // \param dim the dimension in query
  // \param i the index of the point in query
  // \pre dim >= 0 && dim < 3
  // \pre i > = 0 && i < this->GetSize( dim )
  int GetMapping(const int dim, const int i);

  // Description:
  // \brief Returns the begin & end extent that intersects with the VOI
  // \param inExt the input extent
  // \param voi the volume of interest
  // \param begin the begin extent
  // \param end the end extent
  void ComputeBeginAndEnd(int inExt[6], int voi[6], int begin[3], int end[3]);

  // Description:
  // \brief Copies the points & point data to the output.
  // \param inExt the input grid extent.
  // \param outExt the output grid extent.
  // \param pd pointer to the input point data.
  // \param inpnts pointer to the input points, or NULL if uniform grid.
  // \param outPD point to the output point data.
  // \param outpnts pointer to the output points, or NULL if uniform grid.
  // \param useMapping indicates whether to use the index mapping or not.
  // \pre pd != NULL.
  // \pre outPD != NULL.
  void CopyPointsAndPointData( int inExt[6], int outExt[6],
                    vtkPointData* pd, vtkPoints* inpnts,
                    vtkPointData* outPD, vtkPoints* outpnts,
                    bool useMapping=true);

  // Description:
  // \brief Copies the cell data to the output.
  // \param inExt the input grid extent.
  // \param outExt the output grid extent.
  // \param cd the input cell data.
  // \param outCD the output cell data.
  // \param useMapping indicates whether to use the index mapping or not.
  // \pre cd != NULL.
  // \pre outCD != NULL.
  void CopyCellData( int inExt[6], int outExt[6],
               vtkCellData* cd, vtkCellData* outCD,
               bool useMapping=true);

protected:
  vtkExtractStructuredGridHelper();
  ~vtkExtractStructuredGridHelper();

  int OutputWholeExtent[6];
  vtk::detail::vtkIndexMap* IndexMap;

  // Description:
  // \brief Invalidates the output extent.
  void Invalidate();

private:
  vtkExtractStructuredGridHelper(const vtkExtractStructuredGridHelper&); // Not implemented.
  void operator=(const vtkExtractStructuredGridHelper&); // Not implemented.
};

#endif /* VTKEXTRACTSTRUCTUREDGRIDHELPER_H_ */
