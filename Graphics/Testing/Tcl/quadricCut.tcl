package require vtk
package require vtkinteraction

set solidTexture "255  255"
set clearTexture  "255  0"
set edgeTexture  "0  255"

proc makeBooleanTexture {caseNumber resolution thickness} {
  global solidTexture clearTexture edgeTexture
  vtkBooleanTexture booleanTexture$caseNumber

  booleanTexture$caseNumber SetXSize  $resolution 
  booleanTexture$caseNumber SetYSize  $resolution 
  booleanTexture$caseNumber SetThickness  $thickness 

  switch  $caseNumber  {
      0 {
	  eval booleanTexture$caseNumber SetInIn  $solidTexture 
	  eval booleanTexture$caseNumber SetOutIn  $solidTexture 
	  eval booleanTexture$caseNumber SetInOut  $solidTexture 
	  eval booleanTexture$caseNumber SetOutOut  $solidTexture 
	  eval booleanTexture$caseNumber SetOnOn  $solidTexture 
	  eval booleanTexture$caseNumber SetOnIn  $solidTexture 
	  eval booleanTexture$caseNumber SetOnOut  $solidTexture 
	  eval booleanTexture$caseNumber SetInOn  $solidTexture 
	  eval booleanTexture$caseNumber SetOutOn  $solidTexture
      }

      1 {	
	  eval booleanTexture$caseNumber SetInIn  $clearTexture 
	  eval booleanTexture$caseNumber SetOutIn  $solidTexture 
	  eval booleanTexture$caseNumber SetInOut  $solidTexture 
	  eval booleanTexture$caseNumber SetOutOut  $solidTexture 
	  eval booleanTexture$caseNumber SetOnOn  $edgeTexture 
	  eval booleanTexture$caseNumber SetOnIn  $edgeTexture 
	  eval booleanTexture$caseNumber SetOnOut  $solidTexture 
	  eval booleanTexture$caseNumber SetInOn  $edgeTexture 
	  eval booleanTexture$caseNumber SetOutOn  $solidTexture 
      }

      2 {
	  eval booleanTexture$caseNumber SetInIn  $solidTexture 
	  eval booleanTexture$caseNumber SetOutIn  $clearTexture 
	  eval booleanTexture$caseNumber SetInOut  $solidTexture 
	  eval booleanTexture$caseNumber SetOutOut  $solidTexture 
	  eval booleanTexture$caseNumber SetOnOn  $edgeTexture 
	  eval booleanTexture$caseNumber SetOnIn  $edgeTexture 
	  eval booleanTexture$caseNumber SetOnOut  $solidTexture 
	  eval booleanTexture$caseNumber SetInOn  $solidTexture 
	  eval booleanTexture$caseNumber SetOutOn  $edgeTexture 
      }

      3 {
	  eval booleanTexture$caseNumber SetInIn  $clearTexture 
	  eval booleanTexture$caseNumber SetOutIn  $clearTexture 
	  eval booleanTexture$caseNumber SetInOut  $solidTexture 
	  eval booleanTexture$caseNumber SetOutOut  $solidTexture 
	  eval booleanTexture$caseNumber SetOnOn  $edgeTexture 
	  eval booleanTexture$caseNumber SetOnIn  $clearTexture 
	  eval booleanTexture$caseNumber SetOnOut  $solidTexture 
	  eval booleanTexture$caseNumber SetInOn  $edgeTexture 
	  eval booleanTexture$caseNumber SetOutOn  $edgeTexture 
      }

      4 {
	  eval booleanTexture$caseNumber SetInIn  $solidTexture 
	  eval booleanTexture$caseNumber SetOutIn  $solidTexture 
	  eval booleanTexture$caseNumber SetInOut  $clearTexture 
	  eval booleanTexture$caseNumber SetOutOut  $solidTexture 
	  eval booleanTexture$caseNumber SetOnOn  $edgeTexture 
	  eval booleanTexture$caseNumber SetOnIn  $solidTexture 
	  eval booleanTexture$caseNumber SetOnOut  $edgeTexture 
	  eval booleanTexture$caseNumber SetInOn  $edgeTexture 
	  eval booleanTexture$caseNumber SetOutOn  $solidTexture 
      }

      5 {
	  eval booleanTexture$caseNumber SetInIn  $clearTexture 
	  eval booleanTexture$caseNumber SetOutIn  $solidTexture 
	  eval booleanTexture$caseNumber SetInOut  $clearTexture 
	  eval booleanTexture$caseNumber SetOutOut  $solidTexture 
	  eval booleanTexture$caseNumber SetOnOn  $edgeTexture 
	  eval booleanTexture$caseNumber SetOnIn  $edgeTexture 
	  eval booleanTexture$caseNumber SetOnOut  $edgeTexture 
	  eval booleanTexture$caseNumber SetInOn  $clearTexture 
	  eval booleanTexture$caseNumber SetOutOn  $solidTexture 
      }

      6 {
	  eval booleanTexture$caseNumber SetInIn  $solidTexture 
	  eval booleanTexture$caseNumber SetOutIn  $clearTexture 
	  eval booleanTexture$caseNumber SetInOut  $clearTexture 
	  eval booleanTexture$caseNumber SetOutOut  $solidTexture 
	  eval booleanTexture$caseNumber SetOnOn  $edgeTexture 
	  eval booleanTexture$caseNumber SetOnIn  $edgeTexture 
	  eval booleanTexture$caseNumber SetOnOut  $edgeTexture 
	  eval booleanTexture$caseNumber SetInOn  $edgeTexture 
	  eval booleanTexture$caseNumber SetOutOn  $edgeTexture 
      }

      7 {
	  eval booleanTexture$caseNumber SetInIn  $clearTexture 
	  eval booleanTexture$caseNumber SetOutIn  $clearTexture 
	  eval booleanTexture$caseNumber SetInOut  $clearTexture 
	  eval booleanTexture$caseNumber SetOutOut  $solidTexture 
	  eval booleanTexture$caseNumber SetOnOn  $edgeTexture 
	  eval booleanTexture$caseNumber SetOnIn  $clearTexture 
	  eval booleanTexture$caseNumber SetOnOut  $edgeTexture 
	  eval booleanTexture$caseNumber SetInOn  $clearTexture 
	  eval booleanTexture$caseNumber SetOutOn  $edgeTexture 
      }

      8 {
	  eval booleanTexture$caseNumber SetInIn  $solidTexture 
	  eval booleanTexture$caseNumber SetOutIn  $solidTexture 
	  eval booleanTexture$caseNumber SetInOut  $solidTexture 
	  eval booleanTexture$caseNumber SetOutOut  $clearTexture 
	  eval booleanTexture$caseNumber SetOnOn  $edgeTexture 
	  eval booleanTexture$caseNumber SetOnIn  $solidTexture 
	  eval booleanTexture$caseNumber SetOnOut  $edgeTexture 
	  eval booleanTexture$caseNumber SetInOn  $solidTexture 
	  eval booleanTexture$caseNumber SetOutOn  $edgeTexture 
      }

      9 {
	  eval booleanTexture$caseNumber SetInIn  $clearTexture 
	  eval booleanTexture$caseNumber SetInOut  $solidTexture 
	  eval booleanTexture$caseNumber SetOutIn  $solidTexture 
	  eval booleanTexture$caseNumber SetOutOut  $clearTexture 
	  eval booleanTexture$caseNumber SetOnOn  $edgeTexture 
	  eval booleanTexture$caseNumber SetOnIn  $edgeTexture 
	  eval booleanTexture$caseNumber SetOnOut  $edgeTexture 
	  eval booleanTexture$caseNumber SetInOn  $edgeTexture 
	  eval booleanTexture$caseNumber SetOutOn  $edgeTexture 
      }

      10 {
	  eval booleanTexture$caseNumber SetInIn  $solidTexture 
	  eval booleanTexture$caseNumber SetInOut  $solidTexture 
	  eval booleanTexture$caseNumber SetOutIn  $clearTexture 
	  eval booleanTexture$caseNumber SetOutOut  $clearTexture 
	  eval booleanTexture$caseNumber SetOnOn  $edgeTexture 
	  eval booleanTexture$caseNumber SetOnIn  $edgeTexture 
	  eval booleanTexture$caseNumber SetOnOut  $edgeTexture 
	  eval booleanTexture$caseNumber SetInOn  $solidTexture 
	  eval booleanTexture$caseNumber SetOutOn  $clearTexture 
      }

      11 {
	  eval booleanTexture$caseNumber SetInIn  $clearTexture 
	  eval booleanTexture$caseNumber SetInOut  $solidTexture 
	  eval booleanTexture$caseNumber SetOutIn  $clearTexture 
	  eval booleanTexture$caseNumber SetOutOut  $clearTexture 
	  eval booleanTexture$caseNumber SetOnOn  $edgeTexture 
	  eval booleanTexture$caseNumber SetOnIn  $clearTexture 
	  eval booleanTexture$caseNumber SetOnOut  $edgeTexture 
	  eval booleanTexture$caseNumber SetInOn  $edgeTexture 
	  eval booleanTexture$caseNumber SetOutOn  $clearTexture 
      }

      12 {
	  eval booleanTexture$caseNumber SetInIn  $solidTexture 
	  eval booleanTexture$caseNumber SetInOut  $clearTexture 
	  eval booleanTexture$caseNumber SetOutIn  $solidTexture 
	  eval booleanTexture$caseNumber SetOutOut  $clearTexture 
	  eval booleanTexture$caseNumber SetOnOn  $edgeTexture 
	  eval booleanTexture$caseNumber SetOnIn  $solidTexture 
	  eval booleanTexture$caseNumber SetOnOut  $clearTexture 
	  eval booleanTexture$caseNumber SetInOn  $edgeTexture 
	  eval booleanTexture$caseNumber SetOutOn  $edgeTexture 
      }

      13 {
	  eval booleanTexture$caseNumber SetInIn  $clearTexture 
	  eval booleanTexture$caseNumber SetInOut  $clearTexture 
	  eval booleanTexture$caseNumber SetOutIn  $solidTexture 
	  eval booleanTexture$caseNumber SetOutOut  $clearTexture 
	  eval booleanTexture$caseNumber SetOnOn  $edgeTexture 
	  eval booleanTexture$caseNumber SetOnIn  $edgeTexture 
	  eval booleanTexture$caseNumber SetOnOut  $clearTexture 
	  eval booleanTexture$caseNumber SetInOn  $clearTexture 
	  eval booleanTexture$caseNumber SetOutOn  $edgeTexture 
      }


      14 {
	  eval booleanTexture$caseNumber SetInIn  $solidTexture 
	  eval booleanTexture$caseNumber SetInOut  $clearTexture 
	  eval booleanTexture$caseNumber SetOutIn  $clearTexture 
	  eval booleanTexture$caseNumber SetOutOut  $clearTexture 
	  eval booleanTexture$caseNumber SetOnOn  $edgeTexture 
	  eval booleanTexture$caseNumber SetOnIn  $edgeTexture 
	  eval booleanTexture$caseNumber SetOnOut  $clearTexture 
	  eval booleanTexture$caseNumber SetInOn  $edgeTexture 
	  eval booleanTexture$caseNumber SetOutOn  $clearTexture 
      }

      15 {
	  eval booleanTexture$caseNumber SetInIn  $clearTexture 
	  eval booleanTexture$caseNumber SetInOut  $clearTexture 
	  eval booleanTexture$caseNumber SetOutIn  $clearTexture 
	  eval booleanTexture$caseNumber SetOutOut  $clearTexture 
	  eval booleanTexture$caseNumber SetOnOn  $clearTexture 
	  eval booleanTexture$caseNumber SetOnIn  $clearTexture 
	  eval booleanTexture$caseNumber SetOnOut  $clearTexture 
	  eval booleanTexture$caseNumber SetInOn  $clearTexture 
	  eval booleanTexture$caseNumber SetOutOn  $clearTexture 
      }
  }
  return booleanTexture$caseNumber
}

set positions(0) "-4 4 0"
set positions(1) "-2 4 0"
set positions(2) "0 4 0"
set positions(3) "2 4 0"
set positions(4) "-4 2 0"
set positions(5) "-2 2 0"
set positions(6) "0 2 0"
set positions(7) "2 2 0"
set positions(8) "-4 0 0"
set positions(9) "-2 0 0"
set positions(10) "0 0 0"
set positions(11) "2 0 0"
set positions(12) "-4 -2 0"
set positions(13) "-2 -2 0"
set positions(14) "0 -2 0"
set positions(15) "2 -2 0"

vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1 
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin 

  # define two elliptical cylinders
vtkQuadric quadric1
      quadric1 SetCoefficients  1 2 0 0 0 0 0 0 0 -.07 

vtkQuadric quadric2
      quadric2 SetCoefficients  2 1 0 0 0 0 0 0 0 -.07 

  # create a sphere for all to use
vtkSphereSource aSphere
    aSphere SetPhiResolution 50
    aSphere SetThetaResolution 50

  # create texture coordianates for all
vtkImplicitTextureCoords tcoords
      tcoords SetInput [aSphere GetOutput]
      tcoords SetRFunction quadric1 
      tcoords SetSFunction quadric2 

vtkDataSetMapper aMapper
      aMapper SetInput  [tcoords GetOutput]

  # create a mapper, sphere and texture map for each case
for  {set i 0} {$i < 16} {incr i} {
    vtkTexture aTexture$i 
    aTexture$i SetInput  [[eval makeBooleanTexture $i 256 1] GetOutput]
      aTexture$i InterpolateOff  
      aTexture$i RepeatOff   
    vtkActor anActor$i
      anActor$i SetMapper  aMapper 
      anActor$i SetTexture  aTexture$i 
    eval  anActor$i SetPosition   $positions($i)
      anActor$i SetScale  2.0  2.0  2.0 
    ren1 AddActor  anActor$i 
    }

ren1 SetBackground  0.4392 0.5020 0.5647 
[ren1 GetActiveCamera]   Zoom 1.4
renWin SetSize 500 500

# interact with data
renWin Render  
  
#renWin SetFileName "quadricCut.tcl.ppm" 
#renWin SaveImageAsPPM  

iren AddObserver UserEvent {wm deiconify .vtkInteract}
iren Initialize

# prevent the tk window from showing up then start the event loop
wm withdraw .
