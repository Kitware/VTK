catch {load vtktcl}
#
# This is a regression test script for VTK.
#

# first find all the examples. they can be defined on command line or in
# current directory

if { $argv != ""} {
  set files $argv
  } else {
  set files [lsort [glob {[A-z]*.tcl}]]
}

# remove files that are not suitable for regression tests or simply don't work right now
set noTest {
   rt.tcl rtAll.tcl rib.tcl TkInteractor.tcl TkRenderWidget.tcl RenderWidget.tcl 
   rtExamples.tcl polyViewer.tcl KeyFrame.tcl cameraKey.tcl timing.tcl 
   Decimate.tcl assembly2.tcl connPineRoot.tcl aniIso.tcl deciHawa.tcl 
   deciPineRoot.tcl deleted.tcl mcTest.tcl viewMCubesFile.tcl vol.tcl
   volTkInteractor.tcl spikeColor.tcl tkwin.tcl 3dsToRIB.tcl

   Close.tcl ContinuousClose.tcl EnhanceEdges.tcl Gradient2D.tcl 
   HighPassComparison.tcl HybridMedianComparison.tcl IdealHighPass.tcl 
   ImageGradient.tcl LaplacianEdgeEnhance.tcl LaplacianSurfaceEnhance.tcl 
   ShotNoiseInclude.tcl Spectrum.tcl TestContinuousDilate3D.tcl
   TestContinuousErode3D.tcl TestDilateErode3D.tcl TestEuclideanToPolar.tcl 
   TestFeatureAnd.tcl TestHistogram.tcl TestHistogramEqualization.tcl
   TestHybridMedian2D.tcl TestIdealHighPass.tcl TestIdealLowPass.tcl
   TestLaplacian.tcl TestLogarithmicScale.tcl TestMIPFilter.tcl
   TestOpenClose3D.tcl TestRange3D.tcl TestSkeleton2D.tcl
   TestSobel2D.tcl TestSobel3D.tcl TestSubsample3D.tcl TestVariance3D.tcl
   TestWriter.tcl Timing.tcl VTKSpectrum.tcl WindowLevelInterface.tcl
   vtkImageInclude.tcl ContinuousClose2D.tcl TkViewer2.tcl Pad.tcl
}

for {set i 0} {$i < [llength $noTest]} {incr i} {
   if {[set pos [lsearch $files [lindex $noTest $i]]] != -1} {
      set files [lreplace $files $pos $pos ]
   }
}

# now do the tests
foreach afile $files {
   #
   # only tcl scripts with valid/ images are tested
   if {[catch {set channel [open "valid/$afile.ppm"]}] != 0 } {
      puts "WARNING: There is no valid image for $afile"
      continue
   } else {
      close $channel
   }
   
   vtkMath rtExMath
   rtExMath RandomSeed 6
   
   puts -nonewline "$afile took "
   puts -nonewline "[expr [lindex [time {source $afile} 1] 0] / 1000000.0] seconds "
   
   vtkWindowToImageFilter w2if
   
   # look for a renderWindow ImageWindow or ImageViewer
   # first check for some common names
   if {[info commands renWin] == "renWin"} {
      w2if SetInput renWin
   } else {
      if {[info commands viewer] == "viewer"} {
	 w2if SetInput [viewer GetImageWindow]
	 viewer Render
      } else {
	 if {[info commands imgWin] == "imgWin"} {
	    w2if SetInput imgWin
	    imgWin Render
	 }
      }
   }

   # run the event loop quickly to map any tkwidget windows
   wm withdraw .
   update

   if {[catch {set channel [open "valid/$afile.ppm"]}] != 0 } {
      puts "\nWARNING: Creating a valid image for $afile"
      vtkPNMWriter pnmw
      pnmw SetInput [w2if GetOutput]
      pnmw SetFileName "valid/$afile.ppm"
      pnmw Write
      vtkCommand DeleteAllObjects
      catch {destroy .top}
      continue
   }
   close $channel
   
   vtkPNMReader pnm
   pnm SetFileName "valid/$afile.ppm"
   
   vtkImageDifference imgDiff
   imgDiff SetInput [w2if GetOutput]
   imgDiff SetImage [pnm GetOutput]
   imgDiff Update

   # a test has to be off by at least ten pixels for us to care   
   if {[imgDiff GetThresholdedError] < 10.0} {
       puts "and Passed"
   } else {
       puts "but failed with an error of [imgDiff GetThresholdedError]"
         vtkPNMWriter pnmw
         pnmw SetInput [imgDiff GetOutput]
         pnmw SetFileName "$afile.error.ppm"
         pnmw Write
         vtkPNMWriter pnmw2
         pnmw2 SetInput [w2if GetOutput]
         pnmw2 SetFileName "$afile.test.ppm"
         pnmw2 Write
   }
   
   vtkCommand DeleteAllObjects
   catch {destroy .top}
}

exit