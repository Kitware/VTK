/*==========================================================================

  Program: 
  Module:    otherColorTransferFunction.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  ==========================================================================*/

// .NAME 
// .SECTION Description
// this program tests the ColorTransferFunction

#include "vtkColorTransferFunction.h"
#include "vtkDebugLeaks.h"

void Test(ostream& strm)
{
  vtkColorTransferFunction *ctf1 = vtkColorTransferFunction::New();

  // actual test
  strm << "Test vtkColorTransferFunction Start" << endl;

  ctf1->AddRGBPoint (0.0, 1, 0, 0);
  ctf1->AddHSVPoint (1.0, 1, 1, .6);
  ctf1->AddRGBSegment (2.0, 1, 1, 1, 10.0, 0, 0, 0);
  ctf1->AddHSVSegment (11.0, 1, 1, 6, 15.0, .1, .2, .3);
  strm << *ctf1;

  strm << "GetColor(.5) = "
       << ctf1->GetColor(.5)[0] << ", "
       << ctf1->GetColor(.5)[1] << ", "
       << ctf1->GetColor(.5)[2] << endl;

  strm << "GetRedValue(.5) = " << ctf1->GetRedValue(.5) << endl;
  strm << "GetGreenValue(.5) = " << ctf1->GetGreenValue(.5) << endl;
  strm << "GetBlueValue(.5) = " << ctf1->GetBlueValue(.5) << endl;

  strm << "MapValue(12) = "
       << (int) ctf1->MapValue(12)[0] << ", "
       << (int) ctf1->MapValue(12)[1] << ", "
       << (int) ctf1->MapValue(12)[2] << endl;

  strm << "GetRange = "
       << ctf1->GetRange()[0] << "," 
       << ctf1->GetRange()[1] << endl;

  float table[3][256];

  ctf1->GetTable(0, 15, 256, &table[0][0]);
  strm << "GetTable(0, 15, 256, &table[0][0])" << endl;
  for (int i = 0; i < 256; i++)
    {
      for (int j = 0; j < 3; j++)
        {
        strm << table[j][i] << " ";
        }
      strm << endl;
    }

  strm << "BuildFunctionFrom(0, 15, 256, &table[0][0])" << endl;
  vtkColorTransferFunction *ctf2 = vtkColorTransferFunction::New();
  ctf2->BuildFunctionFromTable(0, 15, 256, &table[0][0]);

  ctf1->RemovePoint (10);
  strm << *ctf1;

  ctf1->RemoveAllPoints();
  strm << *ctf1;

  strm << "Test vtkColorTransferFunction End" << endl;
}


int main()
{
  vtkDebugLeaks::PromptUserOff();

  Test(cout);

  return 0;
} 
