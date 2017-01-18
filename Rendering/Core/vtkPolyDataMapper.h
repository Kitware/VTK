/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolyDataMapper.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPolyDataMapper
 * @brief   map vtkPolyData to graphics primitives
 *
 * vtkPolyDataMapper is a class that maps polygonal data (i.e., vtkPolyData)
 * to graphics primitives. vtkPolyDataMapper serves as a superclass for
 * device-specific poly data mappers, that actually do the mapping to the
 * rendering/graphics hardware/software.
*/

#ifndef vtkPolyDataMapper_h
#define vtkPolyDataMapper_h

#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkMapper.h"
#include "vtkTexture.h" // used to include texture unit enum.

class vtkPolyData;
class vtkRenderer;
class vtkRenderWindow;

class VTKRENDERINGCORE_EXPORT vtkPolyDataMapper : public vtkMapper
{
public:
  static vtkPolyDataMapper *New();
  vtkTypeMacro(vtkPolyDataMapper, vtkMapper);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Implemented by sub classes. Actual rendering is done here.
   */
  virtual void RenderPiece(vtkRenderer *ren, vtkActor *act) = 0;

  /**
   * This calls RenderPiece (in a for loop if streaming is necessary).
   */
  void Render(vtkRenderer *ren, vtkActor *act) VTK_OVERRIDE;

  //@{
  /**
   * Specify the input data to map.
   */
  void SetInputData(vtkPolyData *in);
  vtkPolyData *GetInput();
  //@}

  //@{
  /**
   * Bring this algorithm's outputs up-to-date.
   */
  void Update(int port) VTK_OVERRIDE;
  void Update() VTK_OVERRIDE;
  int Update(int port, vtkInformationVector* requests) VTK_OVERRIDE;
  int Update(vtkInformation* requests) VTK_OVERRIDE;
  //@}

  //@{
  /**
   * If you want only a part of the data, specify by setting the piece.
   */
  vtkSetMacro(Piece, int);
  vtkGetMacro(Piece, int);
  vtkSetMacro(NumberOfPieces, int);
  vtkGetMacro(NumberOfPieces, int);
  vtkSetMacro(NumberOfSubPieces, int);
  vtkGetMacro(NumberOfSubPieces, int);
  //@}

  //@{
  /**
   * Set the number of ghost cells to return.
   */
  vtkSetMacro(GhostLevel, int);
  vtkGetMacro(GhostLevel, int);
  //@}

  /**
   * Return bounding box (array of six doubles) of data expressed as
   * (xmin,xmax, ymin,ymax, zmin,zmax).
   */
  double *GetBounds() VTK_OVERRIDE;
  void GetBounds(double bounds[6]) VTK_OVERRIDE
    { this->Superclass::GetBounds(bounds); }

  /**
   * Make a shallow copy of this mapper.
   */
  void ShallowCopy(vtkAbstractMapper *m);

  /**
   * Select a data array from the point/cell data
   * and map it to a generic vertex attribute.
   * vertexAttributeName is the name of the vertex attribute.
   * dataArrayName is the name of the data array.
   * fieldAssociation indicates when the data array is a point data array or
   * cell data array (vtkDataObject::FIELD_ASSOCIATION_POINTS or
   * (vtkDataObject::FIELD_ASSOCIATION_CELLS).
   * componentno indicates which component from the data array must be passed as
   * the attribute. If -1, then all components are passed.
   */
  virtual void MapDataArrayToVertexAttribute(
    const char* vertexAttributeName,
    const char* dataArrayName, int fieldAssociation, int componentno = -1);

  virtual void MapDataArrayToMultiTextureAttribute(
    int unit,
    const char* dataArrayName, int fieldAssociation, int componentno = -1);

  /**
   * Remove a vertex attribute mapping.
   */
  virtual void RemoveVertexAttributeMapping(const char* vertexAttributeName);

  /**
   * Remove all vertex attributes.
   */
  virtual void RemoveAllVertexAttributeMappings();

  /**
   * see vtkAlgorithm for details
   */
  int ProcessRequest(vtkInformation*,
                             vtkInformationVector**,
                             vtkInformationVector*) VTK_OVERRIDE;

protected:
  vtkPolyDataMapper();
  ~vtkPolyDataMapper() VTK_OVERRIDE {}

  /**
   * Called in GetBounds(). When this method is called, the consider the input
   * to be updated depending on whether this->Static is set or not. This method
   * simply obtains the bounds from the data-object and returns it.
   */
  virtual void ComputeBounds();

  int Piece;
  int NumberOfPieces;
  int NumberOfSubPieces;
  int GhostLevel;

  int FillInputPortInformation(int, vtkInformation*) VTK_OVERRIDE;

private:
  vtkPolyDataMapper(const vtkPolyDataMapper&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPolyDataMapper&) VTK_DELETE_FUNCTION;
};

#endif
