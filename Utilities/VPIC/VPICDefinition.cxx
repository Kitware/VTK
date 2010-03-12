
#include "VPICDefinition.h"
#include <stdio.h> // fread

/////////////////////////////////////////////////////////////////////////
//
// Read in the requested number of characters
//
/////////////////////////////////////////////////////////////////////////

string readString(FILE* filePtr, int size)
{
   char* buffer = new char[size + 1];
   fread(buffer, sizeof(char), size, filePtr);
   buffer[size] = '\0';

   // Make sure string has legal values
   if (isalnum(buffer[0]) == 0)
      buffer[0] = '\0';
   for (int i = 1; i < size; i++)
      if (isprint(buffer[i]) == 0)
         buffer[i] = '\0';

   string retString = buffer;
   delete [] buffer;
   return retString;
}

/////////////////////////////////////////////////////////////////////////
//
// Read in the number of items from the file pointer and
// byte swap if necessary
//
/////////////////////////////////////////////////////////////////////////

void readData(
        bool littleEndian,
        unsigned short* data, 
        unsigned long dataSize, 
        unsigned long dataCount, 
        FILE* fp)
{
   // Read all the data from the file
   fread(data, dataSize, dataCount, fp);

   if (littleEndian != true) {

      // Byte swap each integer
      char* dataPtr = (char*) data;
      char temp;
      for (int item = 0; item < dataCount; item++) {

         // Do a byte-by-byte swap, reversing the order.
         for (int i = 0; i < dataSize / 2; i++) {
            temp = dataPtr[i];
            dataPtr[i] = dataPtr[dataSize - 1 - i];
            dataPtr[dataSize - 1 - i] = temp;
         }
         dataPtr += WORDSIZE;
      }
   }
}

/////////////////////////////////////////////////////////////////////////
//
// Read in the number of items from the file pointer and
// byte swap if necessary
//
/////////////////////////////////////////////////////////////////////////

void readData(
        bool littleEndian,
        int* data, 
        unsigned long dataSize, 
        unsigned long dataCount, 
        FILE* fp)
{
   // Read all the data from the file
   fread(data, dataSize, dataCount, fp);

   if (littleEndian != true) {

      // Byte swap each integer
      char* dataPtr = (char*) data;
      char temp;
      for (int item = 0; item < dataCount; item++) {

         // Do a byte-by-byte swap, reversing the order.
         for (int i = 0; i < dataSize / 2; i++) {
            temp = dataPtr[i];
            dataPtr[i] = dataPtr[dataSize - 1 - i];
            dataPtr[dataSize - 1 - i] = temp;
         }
         dataPtr += WORDSIZE;
      }
   }
}

/////////////////////////////////////////////////////////////////////////
//
// Read in the number of items from the file pointer and
// byte swap if necessary
//
/////////////////////////////////////////////////////////////////////////

void readData(
        bool littleEndian,
        float* data,
        unsigned long dataSize,
        unsigned long dataCount,
        FILE* fp)
{
   // Read all the data from the file
   fread(data, dataSize, dataCount, fp);

   if (littleEndian != true) {

      // Byte swap each float
      char* dataPtr = (char*) data;
      char temp;
      for (int item = 0; item < dataCount; item++) {

         // Do a byte-by-byte swap, reversing the order.
         for (int i = 0; i < dataSize / 2; i++) {
            temp = dataPtr[i];
            dataPtr[i] = dataPtr[dataSize - 1 - i];
            dataPtr[dataSize - 1 - i] = temp;
         }
         dataPtr += WORDSIZE;
      }
   }
}

/////////////////////////////////////////////////////////////////////////
//
// Read in the number of items from the file pointer and
// byte swap if necessary
//
/////////////////////////////////////////////////////////////////////////

void readData(
        bool littleEndian,
        double* data,
        unsigned long dataSize,
        unsigned long dataCount,
        FILE* fp)
{
   // Read all the data from the file
   fread(data, dataSize, dataCount, fp);

   if (littleEndian != true) {

      // Byte swap each float
      char* dataPtr = (char*) data;
      char temp;
      for (int item = 0; item < dataCount; item++) {

         // Do a byte-by-byte swap, reversing the order.
         for (int i = 0; i < dataSize / 2; i++) {
            temp = dataPtr[i];
            dataPtr[i] = dataPtr[dataSize - 1 - i];
            dataPtr[dataSize - 1 - i] = temp;
         }
         dataPtr += WORDSIZE;
      }
   }
}

//////////////////////////////////////////////////////////////////////////////
//
// Greatest common divisor
//
//////////////////////////////////////////////////////////////////////////////

int GCD(int a, int b)
{
   if (b == 0)
      return a;
   else
      return GCD(b, a % b);
}
