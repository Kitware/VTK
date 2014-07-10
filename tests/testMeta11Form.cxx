#include <stdio.h>
#include <fstream>
#include <ctype.h>

#include <metaForm.h>

int main(int, char * [])
  {
  MetaForm tObj;

  tObj.InitializeEssential();
  tObj.FileName("testForm.txt");
  tObj.Comment("TestForm");
  tObj.FormTypeName("Form");

  // Add user's defined fields
  int myarray[3];
  myarray[0]=1;
  myarray[1]=2;
  myarray[2]=3;
  tObj.AddUserField("MyName", MET_STRING, strlen("Julien"), "Julien");
  tObj.AddUserField("MyArray", MET_INT_ARRAY,3,myarray);

  int i;
  float myMatrix[4];
  for(i=0; i<4; i++)
    {
    myMatrix[i] = (float)i;
    }
  tObj.AddUserField("MyMatrix", MET_FLOAT_MATRIX,2,myMatrix);

  tObj.PrintInfo();
  tObj.Write();

  tObj.Clear();
  tObj.ClearUserFields();

  tObj.AddUserField("MyName", MET_STRING, strlen("default"), "default");
  tObj.AddUserField("MyArray", MET_INT_ARRAY, 3, myarray);
  tObj.AddUserField("MyMatrix", MET_FLOAT_MATRIX, 2, myMatrix);

  METAIO_STREAM::cout << "Read: " << METAIO_STREAM::endl;
  tObj.Read();
  METAIO_STREAM::cout << "PrintInfo: " << METAIO_STREAM::endl;
  tObj.PrintInfo();

  METAIO_STREAM::cout << "Check fields: " << METAIO_STREAM::endl;
  char* name = static_cast<char*>(tObj.GetUserField("MyName"));
  if(!strcmp(name,"Julien"))
  {
    METAIO_STREAM::cout << "MyName: FAIL" << METAIO_STREAM::endl;
    return 0;
  }
  METAIO_STREAM::cout << "MyName: " << name << METAIO_STREAM::endl;

  int* array = static_cast<int*>(tObj.GetUserField("MyArray"));

  for(i=0;i<3;i++)
  {
    if(array[i] != i+1)
    {
      METAIO_STREAM::cout << "MyArray: FAIL" << METAIO_STREAM::endl;
      return 0;
    }
  }
  METAIO_STREAM::cout << "MyArray: PASS" << METAIO_STREAM::endl;

  float* matrix = static_cast<float*>(tObj.GetUserField("MyMatrix"));
  for(i=0; i<4; i++)
  {
    if(matrix[i] != i)
    {
      METAIO_STREAM::cout << "MyMatrix: FAIL" << METAIO_STREAM::endl;
    }
  }
  METAIO_STREAM::cout << "MyMatrix: PASS" << METAIO_STREAM::endl;

  METAIO_STREAM::cout << "PASSED!" << METAIO_STREAM::endl;

  tObj.Clear();
  tObj.ClearUserFields();

  tObj.FileName("testObject2.txt");
  tObj.InitializeEssential();
  tObj.PrintInfo();
  tObj.Write();
  tObj.Clear();

  tObj.Read();
  tObj.PrintInfo();

  return 1;
  }
