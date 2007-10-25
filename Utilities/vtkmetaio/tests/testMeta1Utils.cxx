#include <stdio.h>
#include <fstream>
#include <ctype.h>


#include <metaUtils.h>

int main(int argc, char **argv)
  {

  if(MET_SystemByteOrderMSB())
    {
    METAIO_STREAM::cout << "MET_SYSTEM_BYTE_ORDER_MSB = TRUE" << METAIO_STREAM::endl;
    }
  else
    {
    METAIO_STREAM::cout << "MET_SYSTEM_BYTE_ORDER_MSB = FALSE" << METAIO_STREAM::endl;
    }
  unsigned short x = 256;
  METAIO_STREAM::cout << "MET_ByteSwapShort: ";
  if(MET_ByteOrderSwapShort(x) != 1)
    METAIO_STREAM::cout << "FAILED: 256->" << MET_ByteOrderSwapShort(x) << METAIO_STREAM::endl;
  else
    METAIO_STREAM::cout << "PASSED" << METAIO_STREAM::endl;
  x = 1;
  METAIO_STREAM::cout << "MET_ByteSwapShort: ";
  if(MET_ByteOrderSwapShort(x) != 256)
    METAIO_STREAM::cout << "FAILED: 1->" << MET_ByteOrderSwapShort(x) << METAIO_STREAM::endl;
  else
    METAIO_STREAM::cout << "PASSED" << METAIO_STREAM::endl;

  MET_ValueEnumType metType = MET_USHORT;
  MET_ValueEnumType tmpMetType = MET_USHORT;
  char tmpString[80];
  sprintf(tmpString, "MET_USHORT");
  METAIO_STREAM::cout << "MET_StringToType: ";
  MET_StringToType(tmpString, &tmpMetType);
  if(tmpMetType != metType)
    METAIO_STREAM::cout << "FAILED" << METAIO_STREAM::endl;
  else
    METAIO_STREAM::cout << "PASSED" << METAIO_STREAM::endl;

  METAIO_STREAM::cout << "MET_TypeToString: ";
  MET_TypeToString(MET_USHORT, tmpString);
  if(strcmp(tmpString, "MET_USHORT"))
    METAIO_STREAM::cout << "FAILED" << METAIO_STREAM::endl;
  else
    METAIO_STREAM::cout << "PASSED" << METAIO_STREAM::endl;

  int n;
  METAIO_STREAM::cout << "MET_SizeOfType: ";
  MET_SizeOfType(MET_USHORT, &n);
  if(2 != n)
    METAIO_STREAM::cout << "FAILED" << METAIO_STREAM::endl;
  else
    METAIO_STREAM::cout << "PASSED" << METAIO_STREAM::endl;

  char **wordArray;
  MET_StringToWordArray("This is a test", &n, &wordArray);
  METAIO_STREAM::cout << "MET_StringToWordArray: N: ";
  if(n != 4)
    METAIO_STREAM::cout << "FAILED" << METAIO_STREAM::endl;
  else
    METAIO_STREAM::cout << "PASSED" << METAIO_STREAM::endl;
  METAIO_STREAM::cout << "MET_StringToWordArray: 1: ";
  if(strcmp(wordArray[0], "This"))
    METAIO_STREAM::cout << "FAILED" << METAIO_STREAM::endl;
  else
    METAIO_STREAM::cout << "PASSED" << METAIO_STREAM::endl;
  METAIO_STREAM::cout << "MET_StringToWordArray: 2: ";
  if(strcmp(wordArray[1], "is"))
    METAIO_STREAM::cout << "FAILED" << METAIO_STREAM::endl;
  else
    METAIO_STREAM::cout << "PASSED" << METAIO_STREAM::endl;
  METAIO_STREAM::cout << "MET_StringToWordArray: 3: ";
  if(strcmp(wordArray[2], "a"))
    METAIO_STREAM::cout << "FAILED" << METAIO_STREAM::endl;
  else
    METAIO_STREAM::cout << "PASSED" << METAIO_STREAM::endl;
  METAIO_STREAM::cout << "MET_StringToWordArray: 4: ";
  if(strcmp(wordArray[3], "test"))
    METAIO_STREAM::cout << "FAILED" << METAIO_STREAM::endl;
  else
    METAIO_STREAM::cout << "PASSED" << METAIO_STREAM::endl;

  char fName[80];
  sprintf(fName, "this/is/a/test.com");

  METAIO_STREAM::cout << "MET_GetFilePathTest: ";
  MET_GetFilePath(fName, tmpString);
  if(strcmp(tmpString, "this/is/a/"))
    METAIO_STREAM::cout << "FAILED" << METAIO_STREAM::endl;
  else
    METAIO_STREAM::cout << "PASSED" << METAIO_STREAM::endl;

  int tmpI;
  METAIO_STREAM::cout << "MET_GetFileSuffixPtr: ";
  MET_GetFileSuffixPtr(fName, &tmpI);
  if(fName[tmpI] != 'c')
    {
    METAIO_STREAM::cout << "FAILED" << METAIO_STREAM::endl;
    METAIO_STREAM::cout << &(fName[tmpI]) << METAIO_STREAM::endl;
    }
  else
    METAIO_STREAM::cout << "PASSED" << METAIO_STREAM::endl;

  METAIO_STREAM::cout << "MET_SetFileSuffix: ";
  MET_SetFileSuffix(fName, ".net");
  if(strcmp(fName, "this/is/a/test.net"))
    METAIO_STREAM::cout << "FAILED" << METAIO_STREAM::endl;
  else
    METAIO_STREAM::cout << "PASSED" << METAIO_STREAM::endl;

  METAIO_STREAM::ofstream fout;
  fout.open("testMetaFileUtils.txt", METAIO_STREAM::ios::out);

  MET_FieldRecordType * mF;
  METAIO_STL::vector<MET_FieldRecordType *> mFields;
  
  mF = new MET_FieldRecordType;
  MET_InitWriteField(mF, "NDims", MET_UCHAR, 2);
  mFields.push_back(mF);

  float vTmp[10];
  vTmp[0] = 0.5;
  vTmp[1] = 0.75;
  mF = new MET_FieldRecordType;
  MET_InitWriteField(mF, "ElementSize", MET_FLOAT_ARRAY, 2, vTmp);
  mFields.push_back(mF);

  char s[80];
  strcpy(s, "X-AXIS Y-AXIS");
  mF = new MET_FieldRecordType;
  MET_InitWriteField(mF, "DirNames", MET_STRING, strlen(s), s);
  mFields.push_back(mF);

  mF = new MET_FieldRecordType;
  MET_InitWriteField(mF, "END", MET_NONE);
  mF->terminateRead = true;
  mFields.push_back(mF);

  MET_Write(fout, &mFields);

  MET_WriteFieldToFile(fout, "Beyond", MET_STRING, 4, "True");
  MET_WriteFieldToFile(fout, "Extra", MET_USHORT, 1);

  fout.flush();

  METAIO_STL::vector<MET_FieldRecordType *>::iterator fieldIter;
  for(fieldIter=mFields.begin(); fieldIter!=mFields.end(); fieldIter++)
    delete *fieldIter;
  mFields.clear();

  //
  //
  //
  METAIO_STREAM::ifstream fin;
  fin.open("testMetaFileUtils.txt", METAIO_STREAM::ios::in);

  mF = new MET_FieldRecordType;
  MET_InitReadField(mF, "NDims", MET_INT);
  mFields.push_back(mF);

  mF = new MET_FieldRecordType;
  MET_InitReadField(mF, "ElementSize", MET_FLOAT_ARRAY, true, 0);
  mFields.push_back(mF);

  mF = new MET_FieldRecordType;
  MET_InitReadField(mF, "DirNames", MET_STRING);
  mFields.push_back(mF);

  mF = new MET_FieldRecordType;
  MET_InitReadField(mF, "END", MET_NONE);
  mF->terminateRead = true;
  mFields.push_back(mF);

  METAIO_STREAM::cout << "MET_Read: ";
  if(!MET_Read(fin, &mFields))
    METAIO_STREAM::cout << "FAILED" << METAIO_STREAM::endl;
  else
    METAIO_STREAM::cout << "PASSED" << METAIO_STREAM::endl;

  fieldIter = mFields.begin();
  int nDims = 0;
  if((*fieldIter)->defined)
    {
    nDims = (int)((*fieldIter)->value[0]);
    if(nDims != 2)
      METAIO_STREAM::cout << "nDims not equal to 2" << METAIO_STREAM::endl;
    else
      METAIO_STREAM::cout << "nDims: Passed" << METAIO_STREAM::endl;
    }
  else
    METAIO_STREAM::cout << "nDims not defined" << METAIO_STREAM::endl;

  float eSize[2];
  fieldIter++;
  if((*fieldIter)->defined)
    {
    eSize[0] = (*fieldIter)->value[0];
    eSize[1] = (*fieldIter)->value[1];
    if(eSize[0] != 0.5 || eSize[1] != 0.75)
      METAIO_STREAM::cout << "ElementSizes are wrong: " << eSize[0] << ", " << eSize[1] << METAIO_STREAM::endl;
    else
      METAIO_STREAM::cout << "ElementSizes: Passed" << METAIO_STREAM::endl;
    }
  else
    METAIO_STREAM::cout << "ElementSize not defined" << METAIO_STREAM::endl;

  int nNames;
  char **names;
  fieldIter++;
  if((*fieldIter)->defined)
    {
    MET_StringToWordArray((char *)((*fieldIter)->value), &nNames, &names);
    if(nNames != 2)
      METAIO_STREAM::cout << "nNames wrong : " << nNames << METAIO_STREAM::endl;
    else
      if(strcmp(names[0], "X-AXIS") || strcmp(names[1], "Y-AXIS"))
        METAIO_STREAM::cout << "names wrong : _" << names[0] << "_, _" << names[1] << "_" << METAIO_STREAM::endl;
      else
        METAIO_STREAM::cout << "Names: Passed" << METAIO_STREAM::endl;
    }
  else
    METAIO_STREAM::cout << "DirNames not defined" << METAIO_STREAM::endl;

  return 1;
  }
