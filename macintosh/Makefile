DARWIN_OBJ = \
vtkQuartzWindow.o \
vtkQuartzGLView.o \
vtkQuartzWindowController.o \
vtkQuartzBridge.o


vtkQuartzWindow.o : vtkQuartzWindow.m vtkQuartzWindow.h
	c++ -c vtkQuartzWindow.m -o vtkQuartzWindow.o

vtkQuartzGLView.o : vtkQuartzGLView.m vtkQuartzGLView.h
	c++ -c vtkQuartzGLView.m -o vtkQuartzGLView.o
        
vtkQuartzWindowController.o : vtkQuartzWindowController.m vtkQuartzWindowController.h
	c++ -c vtkQuartzWindowController.m -o vtkQuartzWindowController.o

vtkQuartzBridge.o : vtkQuartzBridge.m vtkQuartzBridge.h
	c++ -c vtkQuartzBridge.m -o vtkQuartzBridge.o

macObjC : vtkQuartzWindow.o vtkQuartzGLView.o\
	 vtkQuartzWindowController.o vtkQuartzBridge.o