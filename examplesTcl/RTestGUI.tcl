# RTestGUI.tcl - the regression test GUI and associated methods.
# 	         Written by Will Schroeder
#
catch {load vtktcl}

# The following environment variables should be set upon entry into this script.
# (They are typically set by the application RTest.tcl.)
#
#    TCL_EXECUTABLE -       Where the Tcl/Tk executable is, defaults to
#                           the value returned by the Tcl command 
#                           [info nameofexecutable]
#    BIN_DIR -              Where to find this script and supporting Tcl
#                           stuff, dll's, etc. Default is "".
#    VTK_ROOT -             vtk root directory, 
#                           defaults to where abc/vtk/def is found in the
#                           current directory, minus /def (to yield abc/vtk).
#                           In last case, defaults to current drive.
#    VTK_VALID_IMAGE_PATH - where the valid images are, 
#                           defaults to $VTK_ROOT/vtkbaseline
#    VTK_RESULTS_PATH -     where to put regression test images, 
#                           defaults to "$VTK_ROOT/rtResults"
#    VTK_REGRESSION_LOG -   where to send log messages, 
#                           defaults to $VTK_RESULTS_PATH/rt.log
#    VTK_PLATFORM -         the OS of the computer, defaults to "" (i.e., generic)
#
####################################### Create top-level GUI
#
wm title . "VTK Regression Tester v1.0"
frame .mbar -relief raised -bd 2
pack .mbar -side top -fill x

menubutton .mbar.file -text File -menu .mbar.file.menu
menubutton .mbar.run -text Run -menu .mbar.run.menu
menubutton .mbar.edit -text Edit -menu .mbar.edit.menu
menubutton .mbar.options -text Options -menu .mbar.options.menu
menubutton .mbar.help -text Help -menu .mbar.help.menu
pack .mbar.file .mbar.run .mbar.edit .mbar.options -side left
pack .mbar.help -side right

menu .mbar.file.menu
    .mbar.file.menu add cascade -label "New Suite" \
	    -menu .mbar.file.menu.new
    menu .mbar.file.menu.new
	.mbar.file.menu.new add command -label "Add Single Test..." \
	      -command AddSingleTest
	.mbar.file.menu.new add command -label "Add List..." \
	      -command OpenSuite
	.mbar.file.menu.new add cascade -label "Add VTK Kits" \
	      -menu .mbar.file.menu.new.vtkadd
	.mbar.file.menu.new add command \
		-label "Add VTK Tests With Classname..." \
	      -command AddClassname
	set ContribAdded 0
	set GraphicsAdded 0
	set ImagingAdded 0
	set PatentedAdded 0
	menu .mbar.file.menu.new.vtkadd
	      .mbar.file.menu.new.vtkadd add checkbutton \
		      -variable ContribAdded -label Contrib -command AddContrib
	      .mbar.file.menu.new.vtkadd add checkbutton \
		      -variable GraphicsAdded -label Graphics -command AddGraphics
	      .mbar.file.menu.new.vtkadd add checkbutton \
		      -variable ImagingAdded -label Imaging -command AddImaging
	      .mbar.file.menu.new.vtkadd add checkbutton \
		      -variable PatentedAdded -label Patented -command AddPatented
	      .mbar.file.menu.new.vtkadd add command -command AddAll \
		    -label "All VTK Kits"
    .mbar.file.menu add command -label "Open Suite" \
	    -command OpenSuite
    .mbar.file.menu add command -label "Save Suite" \
	    -command SaveSuite
    .mbar.file.menu add command -label "Create Regression Image" \
	    -command CreateRegressionImage
    .mbar.file.menu add command -label Exit -command exit

menu .mbar.run.menu
    .mbar.run.menu add command -label "Test Files" \
          -command {StartTesting 0}
    .mbar.run.menu add command -label "Resume Testing Files" \
          -command {StartTesting 1}

menu .mbar.edit.menu
    .mbar.edit.menu add command -label "Remove Passed Tests" \
          -command RemovePassedTests
    .mbar.edit.menu add command -label "Remove Warning Tests" \
          -command RemoveWarningTests
    .mbar.edit.menu add command -label "Remove Failed Tests" \
          -command RemoveErrorTests
    .mbar.edit.menu add command -label "Remove Untested Tests" \
          -command RemoveUntestedTests
    .mbar.edit.menu add cascade -label "Remove VTK Kits" \
          -menu .mbar.edit.menu.vtkrem
    .mbar.edit.menu add command -label "Remove All Tests" \
          -command RemoveAllTests

menu .mbar.edit.menu.vtkrem
      .mbar.edit.menu.vtkrem add checkbutton -variable ContribAdded \
            -label Contrib -command RemoveContrib
      .mbar.edit.menu.vtkrem add checkbutton -variable GraphicsAdded \
            -label Graphics -command RemoveGraphics
      .mbar.edit.menu.vtkrem add checkbutton -variable ImagingAdded \
            -label Imaging -command RemoveImaging
      .mbar.edit.menu.vtkrem add checkbutton -variable PatentedAdded \
            -label Patented -command RemovePatented
      .mbar.edit.menu.vtkrem add command -command RemoveAll \
            -label "All VTK Kits"

set ImmediateBrowse 0
menu .mbar.options.menu
    .mbar.options.menu add checkbutton -variable ImmediateBrowse \
          -label "View Errors Immediately"

menu .mbar.help.menu
.mbar.help.menu add command -label {About...} -command About

# The selection list (which keeps track of the regression test suite)
frame .list
text .list.text -width 60 -height 24 -wrap none \
      -xscrollcommand {.list.xsbar set} -yscrollcommand {.list.ysbar set}
scrollbar .list.ysbar -orient vertical -command {.list.text yview}
scrollbar .list.xsbar -orient horizontal -command {.list.text xview}
grid .list.text .list.ysbar -sticky nsew
grid .list.xsbar -sticky nsew
grid columnconfigure .list 0 -weight 1
grid rowconfigure .list 0 -weight 1 
pack .list -side top -anchor nw -padx 3 -pady 3 -fill both -expand 1

# Status bar
frame .bottomF -relief sunken -borderwidth 3
label .bottomF.status -text "Idle" -borderwidth 0
button .bottomF.b -text " Test " -command {StartTesting 1}
pack .bottomF.status -side left -anchor w -expand 1 -fill both \
      -padx 0 -pady 0
pack .bottomF.b -side right -anchor w -padx 3 -pady 3
pack .bottomF  -side top -anchor w -fill x -padx 0 -pady 0

# Tags & bindings on list of tests
.list.text tag configure hilite -background gray
.list.text tag configure normal -foreground black
.list.text tag configure passed -foreground green
.list.text tag configure warning -foreground orange
.list.text tag configure error -foreground red

bind .list.text <Motion> {
   .list.text tag remove "hilite" 1.0 end
   .list.text tag add "hilite" "@%x,%y linestart" "@%x,%y lineend" 
}
bind .list.text <Any-Leave> {
   .list.text tag remove "hilite" 1.0 end
}

# Browse images
.list.text tag bind hilite <ButtonPress-1> {
   if { [winfo exist .rt] } {CloseBrowser}
   set file [.list.text get "@%x,%y linestart" "@%x,%y lineend"]
   InvokeBrowser $file
}

# Run a single test manually
.list.text tag bind hilite <ButtonPress-3> {
   global ImmediateBrowse

   set file [.list.text get "@%x,%y linestart" "@%x,%y lineend"]
   set dir [file dirname $file]
   set test [file tail $file]
   .bottomF.status configure -text "Testing: $test" -borderwidth 0
   update

  .list.text tag remove passed "@%x,%y linestart" "@%x,%y lineend"
  .list.text tag remove error "@%x,%y linestart" "@%x,%y lineend"
  .list.text tag remove warning "@%x,%y linestart" "@%x,%y lineend"

   #remove log
   catch {set errorFiles [glob $env(VTK_RESULTS_PATH)/*.*]}
   catch {file delete $errorFiles}
   set ret [RunRegressionTest $dir $test]

   if {$ret == 0} {
      set tag "passed"
   } elseif {$ret == 1} {
      set tag "error"
   } elseif {$ret == 2} {
      set tag "warning"
   } else {
      set tag "warning"
   }
   .list.text tag add $tag "@%x,%y linestart" "@%x,%y lineend"
   if { $ImmediateBrowse && $tag == "error" } {
      InvokeBrowser $line
   }

   .bottomF.status configure -text "Idle" -borderwidth 0
   update
}

########################## Procedures Follow ####################################
##

#### Create a regression test image
#
proc CreateRegressionImage {} {
    global env

    set types {
        {{VTK Tcl Test}                  {.tcl}        }
    }
    set filename [tk_getOpenFile -filetypes $types]

    # If a file was provided, execute it and place the image
    # in the appropriate directory.
    if { $filename != "" } {
	set file [file tail $filename]
	set rtFile [open $filename.rt w]
	puts $rtFile "catch {load vtktcl}"
	if {[string match \*graphics\* $filename]} {
	    set regressionImage $env(VTK_VALID_IMAGE_PATH)/graphics/$file.tif
	} elseif {[string match \*imaging\* $filename]} {
	    set regressionImage $env(VTK_VALID_IMAGE_PATH)/imaging/$file.tif
	} elseif {[string match \*patented\* $filename]} {
	    set regressionImage $env(VTK_VALID_IMAGE_PATH)/patented/$file.tif
	} elseif {[string match \*contrib\* $filename]} {
	    set regressionImage $env(VTK_VALID_IMAGE_PATH)/contrib/$file.tif
	} else {
	    set regressionImage $file.tif
	}
	puts $rtFile "set regressionImage $regressionImage"

	# source the example and set the input
	puts $rtFile "source $filename"
	puts $rtFile "vtkWindowToImageFilter w2if"
	puts $rtFile {
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
		    } else {
		       if {[info exists viewer]} {
			  w2if SetInput [\$viewer GetImageWindow]
		       }
		    }
		}
	    }

	    # capture the image
	    vtkTIFFWriter rttiffw
	    rttiffw SetInput [w2if GetOutput]
	    rttiffw SetFileName $regressionImage
	    rttiffw Write
	    exit
	}
	close $rtFile
	catch {exec $env(TCL_EXECUTABLE) $filename.rt}
	file delete $filename.rt
    }
}

#### Save a regression test suite
#
proc SaveSuite {} {
    set types {
        {{VTK Regression Tester}                  {.rt}        }
    }
    set filename [tk_getSaveFile -filetypes $types]

    # Write out file
    if { $filename != "" } {
       set rtFile [open $filename w]
       set index [.list.text index "end - 1 char"]
       set numLines [expr [lindex [split $index .] 0] - 1]
       for {set i 0} {$i < $numLines} {incr i} {
          set idx [expr $i + 1]
          set line [.list.text get $idx.0 $idx.end]
          if { $line != "" } {
             puts $rtFile $line
          }
       }
       close $rtFile
    }
}

# Open a regression test suite
#
proc OpenSuite {} {
    set types {
        {{VTK Regression Tester}                  {.rt}        }
    }
    set filename [tk_getOpenFile -filetypes $types]

    # Read from file
    if { $filename != "" } {
       set rtFile [open $filename r]
       set line start
       while { $line != "" } {
          set line [gets $rtFile]
          if { $line != "" } {
             .list.text insert end $line
             .list.text insert end "\n"
          }
       }
       close $rtFile
    }
}

# Add all VTK tests with classname given
#
proc AddClassname {} {
    if { ![winfo exists .addclass] } {
        # Build the GUI
        toplevel .addclass
        wm title .addclass "Add VTK tests with classname..."
        wm protocol .addclass WM_DELETE_WINDOW {wm withdraw .addclass}

        frame .addclass.f1
        label .addclass.f1.label -text "VTK Classname:"
        entry .addclass.f1.classname -width 50
        bind .addclass.f1.classname <Return> {ApplyClassname}
        pack .addclass.f1.label .addclass.f1.classname \
            -padx 3 -pady 3 -side top -expand t -fill both 

        frame .addclass.fb
        button .addclass.fb.apply -text Apply -command ApplyClassname
        button .addclass.fb.cancel -text Cancel -command CloseClassname
        pack .addclass.fb.apply .addclass.fb.cancel -side left \
                -expand 1 -fill x
        pack .addclass.f1 .addclass.fb -side top -fill both -expand 1
    } else {
        wm deiconify .addclass
    }
}


proc CloseClassname {} {
    wm withdraw .addclass
}

proc ApplyClassname {} {
    # Check to see if anyname was found
    set classname [.addclass.f1.classname get]
    if { $classname == "" } {
        wm withdraw .addclass
        return
    }

    # First load all the tests
    AddGraphics

    # For each test, delete if it doesn't contain a class
    set index [.list.text index "end - 1 char"]
    set numLines [expr [lindex [split $index .] 0] - 1]

    for {set i 0} {$i < $numLines} {} {
        set idx [expr $i + 1]
        set line [.list.text get $idx.0 $idx.end]

        # Search file for classname
        set vtkTest [open $line r]
        set foundOne [string match \*$classname\* [read $vtkTest]]
        close $vtkTest
        
        if { ! $foundOne } {
            .list.text delete $idx.0 [expr $idx + 1].0
            incr numLines -1
        } else {
            incr i
        }
   }
   wm withdraw .addclass
}

### Procedures to remove tests from current suite
#
proc RemoveTests {tagName} {
   set index [.list.text index "end - 1 char"]
   set numLines [expr [lindex [split $index .] 0] - 1]

   for {set i 0} {$i < $numLines} {} {
      set idx [expr $i + 1]
      set index [.list.text index $idx.end]
      set thisTag [.list.text tag names $idx.0]

      if { $thisTag == $tagName } {
         .list.text delete $idx.0 [expr $idx + 1].0
         incr numLines -1
      } else {
         incr i
      }
   }
}

proc RemovePassedTests {} {
   RemoveTests "passed"
}

proc RemoveWarningTests {} {
   RemoveTests "warning"
}

proc RemoveErrorTests {} {
   RemoveTests "error"
}

proc RemoveUntestedTests {} {
   RemoveTests ""
}

proc RemoveAllTests {} {
   global ContribAdded GraphicsAdded ImagingAdded PatentedAdded
   set ContribAdded 0
   set GraphicsAdded 0
   set ImagingAdded 0
   set PatentedAdded 0

   .list.text delete 1.0 end
}

### Add a single file to the current suite
#
proc AddSingleTest {} {
    set types {
        {{Tcl Regression Test}                  {.tcl}        }
    }
    set filename [tk_getOpenFile -filetypes $types]

    if { $filename != "" } {
       .list.text insert end $filename
       .list.text insert end "\n"
    }
}

#### Procedures to add tests to the current suite from the given directory
#
proc AddDirectory {path} {
   set files [RetrieveFiles $path "*.tcl"]
   set tests [split [.list.text get 1.0 end] "\n"]
   foreach file $files {
      if { [InList $tests $file] == 0 } {
         .list.text insert end $file
         .list.text insert end "\n"
      }
   }
}

proc AddContrib {} {
   global env ContribAdded
   AddDirectory $env(VTK_ROOT)/$env(VTK_DIR)/contrib/examplesTcl
   set ContribAdded 1
}

proc AddGraphics {} {
   global env GraphicsAdded
   AddDirectory $env(VTK_ROOT)/$env(VTK_DIR)/graphics/examplesTcl
   set GraphicsAdded 1
}

proc AddImaging {} {
   global env ImagingAdded
   AddDirectory $env(VTK_ROOT)/$env(VTK_DIR)/imaging/examplesTcl
   set ImagingAdded 1
}

proc AddPatented {} {
   global env PatentedAdded
   AddDirectory $env(VTK_ROOT)/$env(VTK_DIR)/patented/examplesTcl
   set PatentedAdded 1
}

proc AddAll {} {
   AddContrib
   AddGraphics
   AddImaging
   AddPatented
}

proc RetrieveFiles {path expr} {
   set files [lsort [glob $path/$expr]]
   return $files
}

# case insensitive search for membership in list
proc InList {list item} {
   foreach listItem $list {
      if { $listItem != "" && [regexp -nocase $listItem $item] == 1 } {
         return 1
      }
   }
   return 0
}

#### Procedures to remove tests from the current suite given a directory name
#
proc RemoveDirectory {path} {
   set files [RetrieveFiles $path "*.tcl"]
   set tests [split [.list.text get 1.0 end] "\n"]
   .list.text delete 1.0 end
   foreach test $tests {
      set search [InList $files $test]
      if { $test != "" && $search == 0 } {
         .list.text insert end $test
         .list.text insert end "\n"
      }
   }
}

proc RemoveContrib {} {
   global env ContribAdded
   RemoveDirectory $env(VTK_ROOT)/$env(VTK_DIR)/contrib/examplesTcl
   set ContribAdded 0
}

proc RemoveGraphics {} {
   global env GraphicsAdded
   RemoveDirectory $env(VTK_ROOT)/$env(VTK_DIR)/graphics/examplesTcl
   set GraphicsAdded 0
}

proc RemoveImaging {} {
   global env ImagingAdded
   RemoveDirectory $env(VTK_ROOT)/$env(VTK_DIR)/imaging/examplesTcl
   set ImagingAdded 0
}

proc RemovePatented {} {
   global env PatentedAdded
   RemoveDirectory $env(VTK_ROOT)/$env(VTK_DIR)/patented/examplesTcl
   set PatentedAdded 0
}

proc RemoveAll {} {
   RemoveContrib
   RemoveGraphics
   RemoveImaging
   RemovePatented
}

#### The testing process. It depends on the script 
#    graphics/examplesTcl/rtImageTclExamples.tcl.
#
vtkWindowToImageFilter w2if

proc StartTesting {resume} {
   global AbortTesting ImmediateBrowse env

   # Destroy regression log if it exists - it's created again later
   catch {set errorFiles [glob $env(VTK_RESULTS_PATH)/*.*]}
   catch {file delete $errorFiles}

   set AbortTesting 0
   .bottomF.b configure -text "Cancel" -command CancelTesting

   # Set up progress bar
   set index [.list.text index "end - 1 char"]
   set numLines [expr [lindex [split $index .] 0] - 1]

   set height [winfo height .bottomF.status]
   set width [winfo width .bottomF.status]

   if { ![winfo exists .bottomF.canvas] } {
      canvas .bottomF.canvas -height $height -width $width -borderwidth 0\
	    -highlightthickness 0
   } else {
      .bottomF.canvas configure -height $height -width $width
   }

   set BarId [.bottomF.canvas create rect 0 0 0 $height -fill #888]
   set TextId [.bottomF.canvas create text [expr $width/2] [expr $height/2] \
	   -anchor center -justify center -text Testing]
   pack forget .bottomF.status
   pack .bottomF.canvas -padx 0 -pady 0

   if { ! $resume } {
      .list.text tag remove passed 1.0 $numLines.end
      .list.text tag remove error 1.0 $numLines.end
      .list.text tag remove warning 1.0 $numLines.end
   }

   # Now start testing each example one at a time
   for {set i 0} {$i < $numLines && $AbortTesting == 0} {incr i} {
       set progress [expr double($i) / $numLines]
       set height [winfo height .bottomF.status]
       set width [winfo width .bottomF.status]

      # Get the test and describe it, skip previously testing things
       set idx [expr $i + 1]
       set line [.list.text get $idx.0 $idx.end]
       set tag [.list.text tag names $idx.0]
       if { $tag != "" } {continue}

       set dir [file dirname $line]
       set test [file tail $line]

       .bottomF.canvas delete $BarId
       .bottomF.canvas delete $TextId
       set BarId \
             [.bottomF.canvas create rect 0 0 [expr $progress*$width] $height \
             -fill #888]
       set TextId \
             [.bottomF.canvas create text [expr $width/2] [expr $height/2] \
             -anchor center -justify center -text "Testing:$test"]
       update

      # Run the test and document results
      .list.text see $idx.0
      set ret [RunRegressionTest $dir $test]
      if {$ret == 0} {
         set tag "passed"
      } elseif {$ret == 1} {
         set tag "error"
      } elseif {$ret == 2} {
         set tag "warning"
      } else {
         set tag "warning"
      }
      .list.text tag add $tag $idx.0 $idx.end
      if { $ImmediateBrowse && $tag == "error" } {
         InvokeBrowser $line
      }
      update
   }

   # Complete the process
   .bottomF.canvas delete $BarId
   .bottomF.canvas delete $TextId
   .bottomF.b configure -text " Test " -command {StartTesting 1}
   pack forget .bottomF.canvas
   pack .bottomF.status -side left -anchor w -expand 1 -fill both \
         -padx 0 -pady 0

   update
}

proc RunRegressionTest {dir test} {
   global errorCode env

   set errorCode "NONE"
   cd $dir
   catch {exec $env(TCL_EXECUTABLE) \
         $env(VTK_ROOT)/$env(VTK_DIR)/graphics/examplesTcl/rtImageTclExamples.tcl $test}

   if { $errorCode == "NONE" } {
      return 0
   } else {
      return [lindex $errorCode 2]
   }
}

proc CancelTesting {} {
   global AbortTesting
   set AbortTesting 1
}

################################## Image Browser #########################
### Used to view differences between images
###
# vtk objects to read images
# Create the viewing windows for regression testing
set ViewSize 250

proc CreatePipeline {} {
  # The valid image
  vtkTIFFReader readValid
      readValid ReleaseDataFlagOff
  vtkImageResample magValid
      magValid SetInput [readValid GetOutput]
      magValid InterpolateOn
      magValid SetDimensionality 2
  vtkImageViewer viewValid
      viewValid SetInput [magValid GetOutput]
      viewValid SetColorWindow 256
      viewValid SetColorLevel 127.5
      [viewValid GetImageWindow] DoubleBufferOn

# The generated image
  vtkTIFFReader readGenerated
      readGenerated ReleaseDataFlagOff
  vtkImageResample magGenerated
      magGenerated SetInput [readGenerated GetOutput]
      magGenerated InterpolateOn
      magGenerated SetDimensionality 2
  vtkImageViewer viewGenerated
      viewGenerated SetInput [magGenerated GetOutput]
      viewGenerated SetColorWindow 256
      viewGenerated SetColorLevel 127.5
      [viewGenerated GetImageWindow] DoubleBufferOn

# The error image
  vtkTIFFReader readError
      readError ReleaseDataFlagOff
  vtkImageResample magError
      magError SetInput [readError GetOutput]
      magError InterpolateOn
      magError SetDimensionality 2
  vtkImageViewer viewError
      viewError SetInput [magError GetOutput]
      viewError SetColorWindow 256
      viewError SetColorLevel 127.5
      [viewError GetImageWindow] DoubleBufferOn
}

proc CreateBrowser {valid generated error} {
   toplevel .rt
   wm title .rt "Compare Images (Valid image is far left)"
   wm protocol .rt WM_DELETE_WINDOW CloseBrowser
   frame .rt.f1 

   # The valid images
   if { $valid } {
      vtkTkImageViewerWidget .rt.f1.r1 -width 250 \
            -height 250 -iv viewValid

      pack .rt.f1.r1 -side left -padx 3 -pady 3 -fill both -expand t
      bind .rt.f1.r1 <Expose> {.rt.f1.r1 Render}
   }

   # The generated image
   if { $generated } {
      vtkTkImageViewerWidget .rt.f1.r2 -width 250 \
            -height 250 -iv viewGenerated

      pack .rt.f1.r2 -side left -padx 3 -pady 3 -fill both -expand t
      bind .rt.f1.r2 <Expose> {.rt.f1.r2 Render}
   }

   # The error image
   if { $error } {
      vtkTkImageViewerWidget .rt.f1.r3 -width 250 \
            -height 250 -iv viewError

      pack .rt.f1.r3 -side left -padx 3 -pady 3 -fill both -expand t
      bind .rt.f1.r3 <Expose> {.rt.f1.r3 Render}
   }

   # Bottom button and text (regression test string)
   frame .rt.bottom
   text .rt.bottom.text -height 1 -width 66 -wrap none
   pack .rt.bottom.text -side left -anchor w -padx 3 -pady 3
   button .rt.bottom.btn  -text Close -command CloseBrowser
   pack .rt.bottom.btn -side left -anchor w -fill x -padx 3 -pady 3

   pack .rt.f1  -fill both -expand t
   pack .rt.bottom  -fill x -expand t
}

# Search through possible locations for kitname
proc FindKitName {file} {
   set directory [file dirname $file]
   set split [split $directory "/\:"]
   set kitname [lindex $split [expr [llength $split] - 2 ]]
   return $kitname
}

# This brings up the images
proc InvokeBrowser {file} {
   global env BrowserOpen ViewSize

   #first see if image exists, and what its characteristics are
   set kitname [FindKitName $file]
   set valid [file tail $file]
   set validFile $env(VTK_VALID_IMAGE_PATH)/$kitname/$valid.tif
   if { [file exists $validFile] == 0 } {
      tk_messageBox -icon info -message "Valid image not available: $validFile"
      return 
   }

   # Create the data processing pipeline and set attributes
   CreatePipeline

   readValid SetFileName $validFile
   if { [file exists $validFile] == 1 } { 
      readValid SetFileName $validFile
      readValid Update
   } else {
      return
   }
   set dims [[readValid GetOutput] GetDimensions]

   # Now scale the image to fit
   set dimx [lindex $dims 0]
   set dimy [lindex $dims 1]
   if { $dimx > $dimy } {
      set scale [expr double($ViewSize) / $dimx]
   } else {
      set scale [expr double($ViewSize) / $dimy]
   }
   magValid SetAxisMagnificationFactor 0 $scale
   magValid SetAxisMagnificationFactor 1 $scale
   magGenerated SetAxisMagnificationFactor 0 $scale
   magGenerated SetAxisMagnificationFactor 1 $scale
   magError SetAxisMagnificationFactor 0 $scale
   magError SetAxisMagnificationFactor 1 $scale

   set generatedFile $env(VTK_RESULTS_PATH)/$valid.test.tif
   readGenerated SetFileName $generatedFile
   if { [file exists $generatedFile] == 1 } {
      readGenerated SetFileName $generatedFile
      readGenerated Update
      set generated 1
   } else {
      set generated 0
   }

   set errorFile $env(VTK_RESULTS_PATH)/$valid.error.tif
   readError SetFileName $errorFile
   if { [file exists $errorFile] == 1 } {
      readError SetFileName $errorFile
      readError Update
      set error 1
   } else {
      set error 0
   }

   set srange [[readError GetOutput] GetScalarRange]
   viewError SetColorLevel \
         [expr ([lindex $srange 0] + [lindex $srange 1]) / 2.0]
   viewError SetColorWindow [expr [lindex $srange 0] - [lindex $srange 1]]

   # Bring up the browser
   # Create the GUI
   CreateBrowser 1 $generated $error

   # Grab the error log entry
   set rtFile [open $env(VTK_REGRESSION_LOG) r]
   set nchar [gets $rtFile line]
   while { $nchar >= 0 && [string first $valid $line] < 0 } {
      set nchar [gets $rtFile line]
   }
   set line [string trim $line]
   .rt.bottom.text insert 1.0 $line
   close $rtFile

   # Don't want to return until browser is closed   
   update
   set BrowserOpen 1
   while { $BrowserOpen } {
      after 100
      update
   }
}

proc CloseBrowser {} {
   global BrowserOpen
   set BrowserOpen 0
   destroy .rt

   readValid Delete
   magValid Delete
   viewValid Delete

   readGenerated Delete
   magGenerated Delete
   viewGenerated Delete

   readError Delete
   magError Delete
   viewError Delete
}

######## Advertising: the About popup and initial spash screen ##########
###
proc About {} {
   global env

   toplevel .about
   wm title .about "VTK Regression Test Tool"
   wm protocol .about WM_DELETE_WINDOW CloseAbout
   frame .about.f1 -borderwidth 0
   image create photo ColorWheel -file $env(BIN_DIR)/RTest.gif
   label .about.f1.l -image ColorWheel -highlightthickness 0
   pack .about.f1.l -fill both -side top
   button .about.f1.b -text Close -command CloseAbout
   pack .about.f1.b -fill both -side top
   pack .about.f1
}

proc CloseAbout {} {
   destroy .about
}