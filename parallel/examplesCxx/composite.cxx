#include "vtkRenderWindow.h"
#include "vtkMultiProcessController.h"

//-------------------------------------------------------------------------
// Results are put in the local data.
void vtkUCCompositeImagePair(float *localZdata, unsigned char* localPdata, 
			     float *remoteZdata, unsigned char* remotePdata, 
			     int total_pixels) 
{
  int i,j;
  int pixel_data_size;

  pixel_data_size = 3;
  for (i = 0; i < total_pixels; i++) 
    {
    if (remoteZdata[i] < localZdata[i]) 
      {
      localZdata[i] = remoteZdata[i];
      for (j = 0; j < pixel_data_size; j++) 
	{
	localPdata[i*pixel_data_size+j] = remotePdata[i*pixel_data_size+j];
	}
      }
    }
}

#define vtkTCPow2(j) (1 << (j))


//----------------------------------------------------------------------------

void vtkTreeComposite(vtkMultiProcessController *controller,
		      int numPixels,
		      float *localZdata, unsigned char* localPdata)
{
  float *remoteZdata = new float[numPixels];
  unsigned char* remotePdata =  new unsigned char[3*numPixels];
  int pdata_size, zdata_size;
  int myId, numProcs;
  int i, id;

  myId = controller->GetLocalProcessId();
  numProcs = controller->GetNumberOfProcesses();

  zdata_size = numPixels;
  pdata_size = 3*numPixels;

  double doubleLogProcs = log((double)numProcs)/log((double)2);
  int logProcs = (int)doubleLogProcs;

  // not a power of 2 -- need an additional level
  if (doubleLogProcs != (double)logProcs) 
    {
    logProcs++;
    }

  for (i = 0; i < logProcs; i++) 
    {
    if ((myId % (int)vtkTCPow2(i)) == 0) 
      { // Find participants
      if ((myId % (int)vtkTCPow2(i+1)) < vtkTCPow2(i)) 
        {
	// receivers
	id = myId+vtkTCPow2(i);
	
	// only send or receive if sender or receiver id is valid
	// (handles non-power of 2 cases)
	if (id < numProcs) 
          {
	  controller->Receive(remoteZdata, zdata_size, id, 99);
	  controller->Receive((char *)remotePdata, pdata_size, id, 99);
	  
	  // notice the result is stored as the local data
	  vtkUCCompositeImagePair(localZdata, localPdata, 
				  remoteZdata, remotePdata, 
				  numPixels);
	  }
	}
      else 
	{
	id = myId-vtkTCPow2(i);
	if (id < numProcs) 
	  {
	  controller->Send(localZdata, zdata_size, id, 99);
	  controller->Send((char *)localPdata, pdata_size, id, 99);
	  }
	}
      }
    }

  delete[] remoteZdata;
  delete[] remotePdata;
}


