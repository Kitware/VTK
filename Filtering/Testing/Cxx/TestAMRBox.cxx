/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestAMRBox.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkAMRBox.h"

int TestAMRBox(int , char *[])
{
  // Equality.
  {
  vtkAMRBox A(-8,-8,-8,-4,-4,-4);
  vtkAMRBox B(-8,-8,-8,-4,-4,-4);
  vtkAMRBox C(-8,-8,-8,-1,-1,-1);
  vtkAMRBox D;
  vtkAMRBox E(-8,-8,-4,-4);
  vtkAMRBox F(-8,-8,-4,-4);
  vtkAMRBox G(-12,-12,-4,-4);
  if ( !(A==A)
    || !(A==B)
    || A==C
    || A==D
    || D==A
    || A==E
    || E==A
    || !(E==E)
    || !(E==F)
    || E==G)
    {
    A.Print(cerr) << endl;
    B.Print(cerr) << endl;
    C.Print(cerr) << endl;
    D.Print(cerr) << endl;
    E.Print(cerr) << endl;
    F.Print(cerr) << endl;
    G.Print(cerr) << endl;
    cerr << "Failed testing operator===." << endl;
    return 1;
    }
  }
  // Coarsen then refine.
  {
  vtkAMRBox A0(-8,-8,-8,7,7,7);
  vtkAMRBox B0(-8,-8,-8,8,8,8); // can't coarsen
  vtkAMRBox C(-1,-1,-1,0,0,0);

  vtkAMRBox A1(A0);
  A1.Coarsen(8);      // ==C
  vtkAMRBox A2(A1);
  A2.Refine(8);       // ==A0

  vtkAMRBox B1(B0);
  B1.Coarsen(8);      // ==B0

  vtkAMRBox D0(-8,-8,7,7);
  vtkAMRBox E0(-8,-8,8,8); // can't coarsen
  vtkAMRBox F(-1,-1,0,0);

  vtkAMRBox D1(D0);
  D1.Coarsen(8);      // ==F
  vtkAMRBox D2(D1);
  D2.Refine(8);       // ==D0

  vtkAMRBox E1(E0);
  E1.Coarsen(8);      // ==E0

  if ( !(A1==C)
    || !(B1==B0)
    || !(A2==A0)
    || !(D1==F)
    || !(D2==D0)
    || !(E1==E0))
    {
    A0.Print(cerr) << endl;
    B0.Print(cerr) << endl;
    C.Print(cerr) << endl;
    A1.Print(cerr) << endl;
    A2.Print(cerr) << endl;
    B1.Print(cerr) << endl;
    D0.Print(cerr) << endl;
    E0.Print(cerr) << endl;
    F.Print(cerr) << endl;
    D1.Print(cerr) << endl;
    D2.Print(cerr) << endl;
    E1.Print(cerr) << endl;
    cerr << "Failed testing refine coarsened." << endl;
    return 1;
    }
  }
  // Refine then coarsen.
  {
  vtkAMRBox A0(-1,-1,-1,0,0,0);
  vtkAMRBox B(-8,-8,-8,7,7,7);

  vtkAMRBox A1(A0);
  A1.Refine(8);      // ==B
  vtkAMRBox A2(A1);
  A2.Coarsen(8);     // ==A0

  vtkAMRBox D0(-1,-1,0,0);
  vtkAMRBox E(-8,-8,7,7);

  vtkAMRBox D1(D0);
  D1.Refine(8);      // ==E
  vtkAMRBox D2(D1);
  D2.Coarsen(8);     // ==D0

  if ( !(A1==B)
    || !(A2==A0)
    || !(D1==E)
    || !(D2==D0))
    {
    A0.Print(cerr) << endl;
    B.Print(cerr) << endl;
    A1.Print(cerr) << endl;
    A2.Print(cerr) << endl;
    D0.Print(cerr) << endl;
    E.Print(cerr) << endl;
    D1.Print(cerr) << endl;
    D2.Print(cerr) << endl;
    cerr << "Failed testing coarsen refined." << endl;
    return 1;
    }
  }
  // Shift.
  {
  vtkAMRBox A(-2,-2,-2,2,2,2);
  vtkAMRBox B(A);
  B.Shift(100,0,0);
  B.Shift(0,100,0);
  B.Shift(0,0,100);
  B.Shift(-200,-200,-200);
  B.Shift(100,0,0);
  B.Shift(0,100,0);
  B.Shift(0,0,100); // ==A

  vtkAMRBox C(-2,-2,2,2);
  vtkAMRBox D(C);
  D.Shift(100,0,0);
  D.Shift(0,100,0);
  D.Shift(0,0,100);
  D.Shift(-200,-200,-200);
  D.Shift(100,0,0);
  D.Shift(0,100,0);
  D.Shift(0,0,100); // ==C

  if ( !(B==A)
    || !(D==C))
    {
    A.Print(cerr) << endl;
    B.Print(cerr) << endl;
    C.Print(cerr) << endl;
    D.Print(cerr) << endl;
    cerr << "Failed testing shift." << endl;
    return 1;
    }
  }
  // Grow/shrink.
  {
  vtkAMRBox A(-2,-2,-2,2,2,2);
  vtkAMRBox B(-4,-4,-4,4,4,4);
  vtkAMRBox A1(A);
  A1.Grow(2);       // ==B
  vtkAMRBox A2(A1);
  A2.Shrink(2);     // ==A

  vtkAMRBox C(-2,-2,2,2);
  vtkAMRBox D(-4,-4,4,4);
  vtkAMRBox C1(C);
  C1.Grow(2);       // ==D
  vtkAMRBox C2(C1);
  C2.Shrink(2);     // ==C

  if ( !(A2==A)
    || !(A1==B)
    || !(C2==C)
    || !(C1==D))
    {
    A.Print(cerr) << endl;
    B.Print(cerr) << endl;
    A1.Print(cerr) << endl;
    A2.Print(cerr) << endl;
    C.Print(cerr) << endl;
    D.Print(cerr) << endl;
    C1.Print(cerr) << endl;
    C2.Print(cerr) << endl;
    cerr << "Failed tetsing grow/shrink." << endl;
    return 1;
    }
  }
  // Intersect.
  {
  vtkAMRBox A(-4,-4,-4,4,4,4);
  vtkAMRBox B(-4,-4,-4,-1,-1,-1);
  vtkAMRBox C(1,1,1,4,4,4);
  vtkAMRBox AA(A);
  AA&=A;              // ==A
  vtkAMRBox AB(A);
  AB&=B;              // ==B
  vtkAMRBox BA(B);
  BA&=A;              // ==B
  vtkAMRBox AC(A);
  AC&=C;              // ==C
  vtkAMRBox CA(C);
  CA&=A;              // ==C
  vtkAMRBox BC(B); 
  BC&=C;              // ==NULL
  vtkAMRBox CB(C);
  CB&=B;              // ==NULL

  vtkAMRBox D(-4,-4,4,4);
  vtkAMRBox E(-4,-4,-1,-1);
  vtkAMRBox F(1,1,4,4);
  vtkAMRBox DD(D);
  DD&=D;              // ==D
  vtkAMRBox DE(D);
  DE&=E;              // ==E
  vtkAMRBox ED(E);
  ED&=D;              // ==E
  vtkAMRBox DF(D);
  DF&=F;              // ==F
  vtkAMRBox FD(F);
  FD&=D;              // ==F
  vtkAMRBox EF(E);
  EF&=F;              // ==NULL
  vtkAMRBox FE(F);
  FE&=E;              // ==NULL

  vtkAMRBox AD(A);
  AD&=D;              // ==NULL
  vtkAMRBox DA(D);
  DA&=A;              // ==NULL

  if (!(AA==A)
   || !(AB==B) || !(BA==AB)
   || !(AC==C) || !(AC==CA)
   || !BC.Empty() || !CB.Empty()
   || !(DD==D)
   || !(DE==E) || !(DE==ED)
   || !(DF==F) || !(DF==FD)
   || !EF.Empty() || !FE.Empty()
   || !(AD==A)
   || !(DA==D))
    {
    A.Print(cerr) << endl;
    B.Print(cerr) << endl;
    C.Print(cerr) << endl;
    AA.Print(cerr) << endl;
    AB.Print(cerr) << endl;
    BA.Print(cerr) << endl;
    AC.Print(cerr) << endl;
    CA.Print(cerr) << endl;
    BC.Print(cerr) << endl;
    CB.Print(cerr) << endl;
    D.Print(cerr) << endl;
    E.Print(cerr) << endl;
    F.Print(cerr) << endl;
    DD.Print(cerr) << endl;
    DE.Print(cerr) << endl;
    ED.Print(cerr) << endl;
    DF.Print(cerr) << endl;
    FD.Print(cerr) << endl;
    FE.Print(cerr) << endl;
    EF.Print(cerr) << endl;
    AD.Print(cerr) << endl;
    DA.Print(cerr) << endl;
    cerr << "Failed testing operator&=." << endl;
    return 1;
    }
  }
  return 0;
}




