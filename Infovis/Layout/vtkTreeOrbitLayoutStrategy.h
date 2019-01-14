/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTreeOrbitLayoutStrategy.h

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/
/**
 * @class   vtkTreeOrbitLayoutStrategy
 * @brief   hierarchical orbital layout
 *
 *
 * Assigns points to the nodes of a tree to an orbital layout. Each parent
 * is orbited by its children, recursively.
 *
 * @par Thanks:
 * Thanks to the galaxy for inspiring this layout strategy.
*/

#ifndef vtkTreeOrbitLayoutStrategy_h
#define vtkTreeOrbitLayoutStrategy_h

#include "vtkInfovisLayoutModule.h" // For export macro
#include "vtkGraphLayoutStrategy.h"

class vtkPoints;
class vtkTree;


class VTKINFOVISLAYOUT_EXPORT vtkTreeOrbitLayoutStrategy : public vtkGraphLayoutStrategy
{
public:
  static vtkTreeOrbitLayoutStrategy *New();

  vtkTypeMacro(vtkTreeOrbitLayoutStrategy, vtkGraphLayoutStrategy);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Perform the orbital layout.
   */
  void Layout() override;

  //@{
  /**
   * The spacing of orbital levels. Levels near zero give more space
   * to levels near the root, while levels near one (the default)
   * create evenly-spaced levels. Levels above one give more space
   * to levels near the leaves.
   */
  vtkSetMacro(LogSpacingValue, double);
  vtkGetMacro(LogSpacingValue, double);
  //@}

  //@{
  /**
   * The spacing of leaves.  Levels near one evenly space leaves
   * with no gaps between subtrees.  Levels near zero creates
   * large gaps between subtrees.
   */
  vtkSetClampMacro(LeafSpacing, double, 0.0, 1.0);
  vtkGetMacro(LeafSpacing, double);
  //@}

  //@{
  /**
   * This is a magic number right now. Controls the radius
   * of the child layout, all of this should be fixed at
   * some point with a more logical layout. Defaults to .5 :)
   */
  vtkSetMacro(ChildRadiusFactor, double);
  vtkGetMacro(ChildRadiusFactor, double);
  //@}

protected:
  vtkTreeOrbitLayoutStrategy();
  ~vtkTreeOrbitLayoutStrategy() override;

  void OrbitChildren(vtkTree *t, vtkPoints *p, vtkIdType parent, double radius);

  double LogSpacingValue;
  double LeafSpacing;
  double ChildRadiusFactor;

private:

  vtkTreeOrbitLayoutStrategy(const vtkTreeOrbitLayoutStrategy&) = delete;
  void operator=(const vtkTreeOrbitLayoutStrategy&) = delete;
};

#endif

