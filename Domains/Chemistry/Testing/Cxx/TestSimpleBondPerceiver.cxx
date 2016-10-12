/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkMolecule.h"
#include "vtkNew.h"
#include "vtkSimpleBondPerceiver.h"

int TestSimpleBondPerceiver(int , char *[])
{
  vtkIdType numBonds;
  vtkNew<vtkMolecule> mol;
  vtkNew<vtkSimpleBondPerceiver> bonder;
  bonder->SetInputData(mol.GetPointer());
//  bonder->DebugOn();

  // First try out the render test molecule:
  mol->AppendAtom(8,  3.0088731969,  1.1344098673,  0.9985902874);
  mol->AppendAtom(8, -0.2616286966,  2.7806709534,  0.7027800226);
  mol->AppendAtom(6, -2.0738607910,  1.2298524695,  0.3421802228);
  mol->AppendAtom(6, -1.4140240045,  0.1045928523,  0.0352265378);
  mol->AppendAtom(6,  0.0000000000,  0.0000000000,  0.0000000000);
  mol->AppendAtom(6,  1.2001889412,  0.0000000000,  0.0000000000);
  mol->AppendAtom(6, -1.4612030913,  2.5403617582,  0.6885503164);
  mol->AppendAtom(6,  2.6528126498,  0.1432895796,  0.0427014196);
  mol->AppendAtom(1, -3.1589178142,  1.2268537165,  0.3536340040);
  mol->AppendAtom(1, -1.9782163251, -0.7930325394, -0.1986937306);
  mol->AppendAtom(1,  3.0459155564,  0.4511167867, -0.9307386568);
  mol->AppendAtom(1,  3.1371551056, -0.7952192984,  0.3266426961);
  mol->AppendAtom(1,  2.3344947615,  1.8381683043,  0.9310726537);
  mol->AppendAtom(1, -2.1991803919,  3.3206134015,  0.9413825084);

  bonder->Update();
  numBonds = bonder->GetOutput()->GetNumberOfBonds();

  if (numBonds != 13)
  {
    cout << "Number of bonds in test molecule: "
         << numBonds << " (should be 13)\n";
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
