#include <stdio.h>
#include <fstream>
#include <ctype.h>

#include <metaArray.h>

int main(int, char * [])
  {
  MetaArray tObj;

  tObj.InitializeEssential(10, MET_DOUBLE, 1, NULL, true);
  tObj.FileName("testArray.mvh");
  tObj.Comment("TestArray");
  tObj.FormTypeName("Array");

  tObj.Length(10);
  tObj.ElementType(MET_DOUBLE);
  tObj.BinaryData(false);

  int i;
  for(i=0; i<10; i++)
    {
    tObj.ElementData(i, i*3.1415926);
    }
  tObj.PrintInfo();
  tObj.Write();

  tObj.BinaryData(true);
  tObj.FileName("testArray.mva");
  tObj.PrintInfo();
  tObj.Write();

  MetaArray tObj_ascii, tObj_binary;
  tObj_ascii.FileName("testArray.mvh");
  tObj_binary.FileName("testArray.mva");

  tObj_ascii.Read();
  tObj_binary.Read();

  tObj_ascii.PrintInfo();
  tObj_binary.PrintInfo();

  METAIO_STREAM::cout << "PASSED!" << METAIO_STREAM::endl;

  return 1;
  }
