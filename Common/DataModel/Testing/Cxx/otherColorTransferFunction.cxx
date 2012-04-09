/*=========================================================================

  Program:   Visualization Toolkit
  Module:    otherColorTransferFunction.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME 
// .SECTION Description
// this program tests the ColorTransferFunction

#include "vtkColorTransferFunction.h"
#include "vtkDebugLeaks.h"

#include <vtksys/ios/sstream>

int Test(ostream& strm)
{
  int i, j, k;
  vtkColorTransferFunction *ctf1 = vtkColorTransferFunction::New();

  // actual test
  strm << "Test vtkColorTransferFunction Start" << endl;

  ctf1->AddRGBPoint (0.0, 1, 0, 0);
  ctf1->AddHSVPoint (1.0, 1, 1, .6);
  ctf1->AddRGBSegment (2.0, 1, 1, 1, 10.0, 0, 0, 0);
  ctf1->AddHSVSegment (11.0, 1, 1, 6, 15.0, .1, .2, .3);
  strm << *ctf1;

  double rgb[3];
  ctf1->GetColor(.5, rgb);
  strm << "GetColor(.5) = " 
    << rgb[0] << ", " << rgb[1] << ", " << rgb[2] << endl;

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

  double table[3][256];

  ctf1->GetTable(0, 15, 256, &table[0][0]);
  strm << "GetTable(0, 15, 256, &table[0][0])" << endl;
  for (i = 0; i < 256; i++)
    {
      for (j = 0; j < 3; j++)
        {
        strm << table[j][i] << " ";
        }
      strm << endl;
    }

  strm << "BuildFunctionFrom(0, 15, 256, &table[0][0])" << endl;
  vtkColorTransferFunction *ctf2 = vtkColorTransferFunction::New();
  ctf2->BuildFunctionFromTable(0, 15, 256, &table[0][0]);

  ctf2->SetColorSpaceToRGB();
  ctf2->GetTable(0,15,512);

  ctf2->SetColorSpaceToHSV();
  ctf2->GetTable(0,15,512);

  ctf1->DeepCopy(ctf2);
  strm << "ctf1->DeepCopy(ctf2)" << endl;
  strm << *ctf1;
  
  ctf1->RemovePoint (10);
  strm << *ctf1;

  ctf1->RemoveAllPoints();
  strm << *ctf1;

  char *cData = new char[128];
  unsigned char *ucData = new unsigned char[128];
  short *sData = new short[128];
  unsigned short *usData = new unsigned short[128];
  int *iData = new int[128];
  unsigned int *uiData = new unsigned int[128];
  long *lData = new long[128];
  unsigned long *ulData = new unsigned long[128];
  float *fData = new float[128];
  double *dData = new double[128];

  for (k = 0; k < 128; k++)
    {
    *(cData+k) = static_cast<char>(static_cast<float>(k)/255.0);
    *(ucData+k) = static_cast<unsigned char>(static_cast<float>(k)/255.0);
    *(sData+k) = static_cast<short>(static_cast<float>(k)/255.0);
    *(usData+k) = static_cast<unsigned short>(static_cast<float>(k)/255.0);
    *(iData+k) = static_cast<int>(static_cast<float>(k)/255.0);
    *(uiData+k) = static_cast<unsigned int>(static_cast<float>(k)/255.0);
    *(lData+k) = static_cast<long>(static_cast<float>(k)/255.0);
    *(ulData+k) = static_cast<unsigned long>(static_cast<float>(k)/255.0);
    *(fData+k) = static_cast<float>(static_cast<float>(k)/255.0);
    *(dData+k) = static_cast<double>(static_cast<float>(k)/255.0);
    }

  unsigned char *ucResult = new unsigned char[128*4];
  for (k = 1; k <= 4; k++)
    {
    ctf2->MapScalarsThroughTable2((void *) cData, ucResult, VTK_CHAR, 128, 1, k);
    ctf2->MapScalarsThroughTable2((void *) ucData, ucResult, VTK_UNSIGNED_CHAR, 128, 1, k);
    ctf2->MapScalarsThroughTable2((void *) sData, ucResult, VTK_SHORT, 128, 1, k);
    ctf2->MapScalarsThroughTable2((void *) usData, ucResult, VTK_UNSIGNED_SHORT, 128, 1, k);
    ctf2->MapScalarsThroughTable2((void *) iData, ucResult, VTK_INT, 128, 1, k);
    ctf2->MapScalarsThroughTable2((void *) uiData, ucResult, VTK_UNSIGNED_INT, 128, 1, k);
    ctf2->MapScalarsThroughTable2((void *) lData, ucResult, VTK_LONG, 128, 1, k);
    ctf2->MapScalarsThroughTable2((void *) ulData, ucResult, VTK_UNSIGNED_LONG, 128, 1, k);
    ctf2->MapScalarsThroughTable2((void *) fData, ucResult, VTK_FLOAT, 128, 1, k);
    ctf2->MapScalarsThroughTable2((void *) dData, ucResult, VTK_DOUBLE, 128, 1, k);
    }

  ctf1->Delete();
  ctf2->Delete();

  delete []ucResult;
  delete []cData;
  delete []ucData;
  delete []sData;
  delete []usData;
  delete []iData;
  delete []uiData;
  delete []lData;
  delete []ulData;
  delete []fData;
  delete []dData;

  strm << "Test vtkColorTransferFunction End" << endl;
  return 0;
}


int otherColorTransferFunction(int, char *[])
{
  vtksys_ios::ostringstream vtkmsg_with_warning_C4701; 
  return Test(vtkmsg_with_warning_C4701);
} 
