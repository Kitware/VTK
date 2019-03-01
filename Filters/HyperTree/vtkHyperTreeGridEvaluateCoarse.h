/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHyperTreeGridEvaluateCoarse.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkHyperTreeGridEvaluateCoarse
 * @brief   a partir des feuilles affecte une valeur a chaque maille coarse
 *          suivant une operation identifiee.
 *
 *
 * JB vtkHyperTreeGridEvaluateCoarse is a filter that takes as input an hyper tree
 * grid.
 * Chaque maille coarse se verra affecter a une valeur calculee a partir des
 * valeurs associees aux mailles filles et ceci en commencant par les mailles
 * coarse qui ont que des mailles feuilles.
 * La valeur que l'on affecte a une maille coarse est determinee a partir
 * des valeurs associees a ses mailles filles auxquelles on applique un des
 * operateurs predefinis identifie par un numero :
 *    #0    operateur don't change coarse value fast(default), just shallow copy
 *    #1    operateur don't change coarse value avec parcours qui ne fait rien
 *    #2    operateur min : la plus petite valeur des filles incluses dans le materiau
 *    #3    operateur max : la plus grande valeur des filles incluses dans le materiau
 *    #4    operateur sum : la somme des valeurs des filles incluses dans le materiau
 *    #5    operateur average: la somme des valeurs des filles incluses dans le materiau
 *          augmentee par celle des filles non incluses dans le materiau auquel a ete
 *          affecte la valeur par defaut (initialement a 0) divise par le nombre
 *          total de filles (HTG->GetNumberOfChildren())
 *    #6    operateur material_average : la somme des valeurs des filles incluses dans le materiau
 *          divisee par leur nombre.
 *    #7    operateur first son (elder child)
 *    #8    operateur splatting average: au lieu de diviser par (HTG->GetNumberOfChildren())
 *          qui vaut f^d on divise par f^(d-1). En effet, le splat des 8 childrens de 0.5
 *          produisent orthogonalement 4 splats de 1. Le splat de la maille mere doit donc donner
 *          la valeur de 1. et non celle de la moyenne qui vaut 0.5. Si cela s'avere vrai,
 *          le recalcul des coarses dans ce mode serait necessaire avant tout application du
 *          filtre de splatting.
 *
 *
 * @sa
 * vtkHyperTreeGrid vtkHyperTreeGridAlgorithm
 *
 * @par Thanks:
 * This class was written by Guenole Harel and Jacques-Bernard Lekien, 2016-18
 * This work was supported by Commissariat a l'Energie Atomique
 * CEA, DAM, DIF, F-91297 Arpajon, France.
*/

#ifndef vtkHyperTreeGridEvaluateCoarse_h
#define vtkHyperTreeGridEvaluateCoarse_h

#include "vtkFiltersHyperTreeModule.h" // For export macro
#include "vtkHyperTreeGridAlgorithm.h"

#include <vector> // For scratch storage.

class vtkBitArray;
class vtkHyperTreeGrid;
class vtkHyperTreeGridNonOrientedCursor;


class VTKFILTERSHYPERTREE_EXPORT vtkHyperTreeGridEvaluateCoarse : public vtkHyperTreeGridAlgorithm
{
public:
  enum {
     OPERATOR_DON_T_CHANGE_FAST = 0,
     OPERATOR_DON_T_CHANGE      = 1,
     OPERATOR_MIN               = 2,
     OPERATOR_MAX               = 3,
     OPERATOR_SUM               = 4,
     OPERATOR_AVERAGE           = 5,
     OPERATOR_MATERIAL_AVERAGE  = 6,
     OPERATOR_ELDER_CHILD       = 7,
     OPERATOR_SPLATTING_AVERAGE = 8
  };

  static vtkHyperTreeGridEvaluateCoarse* New();
  vtkTypeMacro( vtkHyperTreeGridEvaluateCoarse, vtkHyperTreeGridAlgorithm );
  void PrintSelf( ostream&, vtkIndent ) override;

  //@{
  /**
   * Set/Get operator
   */
  vtkSetMacro( Operator, unsigned int );
  vtkGetMacro( Operator, unsigned int );
  //@}

  //@{
  /**
   * Set/Get operator
   */
  vtkSetMacro( Default, double );
  //@}

protected:
  vtkHyperTreeGridEvaluateCoarse();
  ~vtkHyperTreeGridEvaluateCoarse() override;

  /**
   * Main routine to extract hyper tree grid levels
   */
  int ProcessTrees( vtkHyperTreeGrid*, vtkDataObject* ) override;

  //@{
  /**
   * Define default input and output port types
   */
  int FillOutputPortInformation( int, vtkInformation* ) override;
  //@}

  /**
   * Recursively descend into tree down to leaves
   */
  virtual void ProcessNode( vtkHyperTreeGridNonOrientedCursor* );

private:
  vtkHyperTreeGridEvaluateCoarse( const vtkHyperTreeGridEvaluateCoarse& ) = delete;
  void operator=( const vtkHyperTreeGridEvaluateCoarse& ) = delete;

  //@{
  /**
   * Define operator
   */
  virtual double EvalCoarse( const std::vector<double>& );

  virtual double Min( const std::vector<double>& );
  virtual double Max( const std::vector<double>& );
  virtual double Sum( const std::vector<double>& );
  virtual double Average( const std::vector<double>& );
  virtual double MaterialAverage( const std::vector<double>& );
  virtual double ElderChild( const std::vector<double>& );
  virtual double SplattingAverage( const std::vector<double>& );
  //@}

  int NbChilds;
  unsigned int Operator;

  double Default;

  unsigned int BranchFactor;
  unsigned int Dimension;
  unsigned int SplattingFactor;

  unsigned int NumberOfChildren;

  std::vector<vtkDataArray*> Arrays;

  vtkBitArray* Mask;
};

#endif // vtkHyperTreeGridEvaluateCoarse_h
