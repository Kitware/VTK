/*  Author: Kenneth Leiter
	E-mail: kenneth.leiter@us.army.mil

	Writes values to XdmfArrays and then tries to read them.  Compares the values for equality and
	tests the accuracy of some convenience functions.
*/

#include <Xdmf.h>
#include <XdmfArray.h>

int main( int argc, const char* argv[] )
{
	XdmfArray * intArray = new XdmfArray();
	intArray->SetNumberType(XDMF_INT64_TYPE);
	if (strcmp(intArray->GetNumberTypeAsString(), "XDMF_INT64_TYPE") != 0) return -1;
	intArray->SetShapeFromString("3 3");
	if (strcmp(intArray->GetShapeAsString(), "3 3") != 0) return -1;
	if (intArray->GetNumberOfElements() != 9) return -1;
	long toWrite [9] = {0, 1, 3, 4, 500, -5000, 500000, 9223372036854775807, -9223372036854775807};
	for (int i=0; i<9; i++)
	{
		intArray->SetValueFromInt64(i,toWrite[i]);
	}
	for (int i=0; i<9; i++)
	{
		if(intArray->GetValueAsInt64(i) != toWrite[i]) return -1;
	}
	if (intArray->GetMaxAsInt64() != 9223372036854775807) return -1;
	if (intArray->GetMinAsInt64() != -9223372036854775807) return -1;
	delete intArray;

	XdmfArray * floatArray = new XdmfArray();
	floatArray->SetNumberType(XDMF_FLOAT64_TYPE);
	if (floatArray->GetNumberType() != XDMF_FLOAT64_TYPE) return -1;
	floatArray->SetShapeFromString("2 2 2");
	if (strcmp(floatArray->GetShapeAsString(), "2 2 2") != 0) return -1;
	if (floatArray->GetNumberOfElements() != 8) return -1;
	double floatToWrite [8] = {0, -1, 1100.256, 1.1, 1000.50 , 5.6234567, -60.2, 60.25659};
	floatArray->SetValues(0, floatToWrite, 8, 1, 1);
	for (int i=0; i<8; i++)
	{
		if(floatArray->GetValueAsFloat64(i) != floatToWrite[i]) return -1;
	}
	if (floatArray->GetMaxAsFloat64() != 1100.256) return -1;
	if (floatArray->GetMinAsFloat64() != -60.2) return -1;
	delete floatArray;

	XdmfArray * opArray = new XdmfArray();
	opArray->SetNumberType(XDMF_INT32_TYPE);
	if (strcmp(opArray->GetNumberTypeAsString(), "XDMF_INT32_TYPE") != 0) return -1;
	opArray->SetNumberOfElements(10);
	if (strcmp(opArray->GetShapeAsString(), "10") != 0) return -1;
	if (opArray->GetNumberOfElements() != 10) return -1;
	opArray->Generate(0, 9);
	for (int i=0; i<opArray->GetNumberOfElements(); i++)
	{
		if(opArray->GetValueAsInt32(i) != i) return -1;
	}

	delete opArray;

	return 0;
}
