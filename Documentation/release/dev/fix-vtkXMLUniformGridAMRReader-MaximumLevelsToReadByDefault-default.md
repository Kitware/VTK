## Fix wrong default value of vtkXMLUniformGridAMRReader::MaximumLevelsToReadByDefault

vtkXMLUniformGridAMRReader::MaximumLevelsToReadByDefault used to be 1 by default, it should have been
0 (infinite) as documented.
