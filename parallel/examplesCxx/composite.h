void vtkUCCompositeImagePair(float *localZdata, unsigned char* localPdata, 
			     float *remoteZdata, unsigned char* remotePdata, 
			     int total_pixels);
void vtkTreeComposite(vtkMultiProcessController *controller,
		      int numPixels,
		      float *localZdata, unsigned char* localPdata);

