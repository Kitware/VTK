catch {load vtktcl}
# Simple viewer for images.
#make interface
wm withdraw .
toplevel .ctan  
wm title .ctan {CTA Segmentation}
wm resizable .ctan 0 0
#wm withdraw .ctan
global afilter con subs

# Interface to Load Data ...
frame .ctan.dd -relief ridge -borderwidth 3
button .ctan.dd.b0 -text "Load Parameters..." -command LData
button .ctan.dd.b1 -text "Display Original Images" -command DData


# Interface for SubSampling ...
frame  .ctan.cta0 -relief ridge -borderwidth 3
frame  .ctan.cta0.label
label  .ctan.cta0.label.l1 -text "SubSampling Using:"
frame  .ctan.cta0.process
radiobutton .ctan.cta0.process.minflag -text "Min"  -variable subs -value 0 -command SetMinimumFlag
radiobutton .ctan.cta0.process.maxflag -text "Max"  -variable subs -value 1 -command SetMaximumFlag
radiobutton .ctan.cta0.process.meanflag -text "Mean"  -variable subs -value 2 -command SetMeanFlag
radiobutton .ctan.cta0.process.medianflag -text "Median" -variable subs -value 3 -command SetMedianFlag
button .ctan.cta0.b1 -text "Display Images" -command DSI
 
# Interface for Thresholding ...
frame .ctan.cta1 -relief ridge -borderwidth 3
frame .ctan.cta1.group

label .ctan.cta1.group.l1 -text "Lower TH & Upper TH"
entry .ctan.cta1.group.lowerth -width 4
entry .ctan.cta1.group.upperth -width 4 
button .ctan.cta1.b1 -text "Display Images" -command DTHI

# Interface for Connectivity ...
frame .ctan.cta2 -relief ridge -borderwidth 3
frame .ctan.cta2.group
frame .ctan.cta2.process
label .ctan.cta2.group.l1 -text "TH-%TH of Max:"
entry .ctan.cta2.group.threshold -width 4 
entry .ctan.cta2.group.plth -width 4
radiobutton .ctan.cta2.process.six -text "6 Connected" -variable con  -value 5 -command SetSix
radiobutton .ctan.cta2.process.twentysix -text "26 Connected" -variable con  -value 6 -command SetTwentySix
button .ctan.cta2.b1 -text "Display Images" -command DCI

# interface for Markboundary
frame .ctan.cta3 -relief ridge -borderwidth 3
frame .ctan.cta3.group
label .ctan.cta3.group.l1 -text "Mark Boundary Radius"
entry .ctan.cta3.group.st -width 4
button .ctan.cta3.b1 -text "Display Images" -command DMBI

# Adaptive Filter ...
# Interface for Thresholding ...
frame .ctan.cta4 -relief ridge -borderwidth 3
frame .ctan.cta4.group

label .ctan.cta4.group.l1 -text "Adaptive Filter Radius"
entry .ctan.cta4.group.kernelradius -width 4
frame  .ctan.cta4.process
radiobutton .ctan.cta4.process.minamflag -text "Min" -variable afilter -value 7 -command SetMinAMFlag
radiobutton .ctan.cta4.process.maxamflag -text "Max" -variable afilter  -value 8 -command SetMaxAMFlag
radiobutton .ctan.cta4.process.meanamflag -text "Mean" -variable afilter  -value 9 -command SetMeanAMFlag
radiobutton .ctan.cta4.process.medamflag -text "Median" -variable afilter -value 10 -command SetMedAMFlag
button .ctan.cta4.b1 -text "Display Images" -command DAMI
button .ctan.composite -text "Display Composite Images" -command Composite
button .ctan.update -text "Update" -command Update

# Interface for MIP ...
frame .ctan.cta5 -relief ridge -borderwidth 3
frame .ctan.cta5.group
label .ctan.cta5.group.l1 -text "MIP Range:"
entry .ctan.cta5.group.lowerprj -width 4
entry .ctan.cta5.group.upperprj -width 4 
button .ctan.cta5.b1 -text "Display MIP" -command DMIP

button .ctan.wl -text "Window & Level" -command PopupWL
button .ctan.save -text "Save" -command Save
button .ctan.exit -text "Exit" -command Exit


# pack frames togther ...
pack .ctan.dd .ctan.cta0 .ctan.cta1 .ctan.cta2 .ctan.cta3 .ctan.cta4 .ctan.cta5 -side top

# packing Display Data frame
#pack .ctan.dd.slice .ctan.dd.wl -side bottom
pack .ctan.dd.b0 .ctan.dd.b1 -side top -expand 1 -in .ctan.dd  -fill both -padx 2 -pady 2
pack .ctan.dd -side top -expand 1 -in .ctan -fill both -padx 2 -pady 2

# packing Subsampling frame 
pack .ctan.cta0 -side top
pack .ctan.cta0.label .ctan.cta0.process -side top
pack .ctan.cta0.label.l1  -side top -fill none -expand 0
pack .ctan.cta0.process.minflag .ctan.cta0.process.maxflag .ctan.cta0.process.meanflag \
     .ctan.cta0.process.medianflag -side left -fill both -expand 1
pack  .ctan.cta0.b1 \
      -side bottom -expand 1 -in .ctan.cta0 -fill both -padx 2 -pady 2
pack .ctan.cta0 -expand 1 -in .ctan -fill both -padx 2 -pady 2

# packing Thresholding frame 
pack .ctan.cta1.group -side top
pack .ctan.cta1.group.l1 -in .ctan.cta1.group -side top 
pack .ctan.cta1.group.lowerth .ctan.cta1.group.upperth -padx 2 -pady 2 \
	-in .ctan.cta1.group  -side left -expand 1 -fill both 
pack  .ctan.cta1.b1 \
      -side bottom -expand 1 -in .ctan.cta1 -fill both -padx 2 -pady 2
pack .ctan.cta1 -expand 1 -in .ctan -fill both -padx 2 -pady 2

# packing Connectivity frame 
pack .ctan.cta2 -side top
pack .ctan.cta2.group .ctan.cta2.process -side top
pack .ctan.cta2.group.l1 -in .ctan.cta2.group -side left -fill none -expand 0
pack .ctan.cta2.group.threshold .ctan.cta2.group.plth -padx 2 -pady 2 \
	-in .ctan.cta2.group -side left -expand 1 -fill both
pack .ctan.cta2.process.six .ctan.cta2.process.twentysix -in .ctan.cta2.process \
	-side left -fill both -expand 1
pack  .ctan.cta2.b1 \
      -side bottom -expand 1 -in .ctan.cta2 -fill both -padx 2 -pady 2
pack .ctan.cta2 -expand 1 -in .ctan -fill both -padx 2 -pady 2

# packing Mark Boundary frame 
pack .ctan.cta3 -side top
pack .ctan.cta3.group -side top
pack .ctan.cta3.group.l1 -in .ctan.cta3.group -side left -fill none -expand 0
pack .ctan.cta3.group.st -padx 23 -pady 2 \
      -in .ctan.cta3.group -side left -fill both -expand 1
pack  .ctan.cta3.b1 \
      -side bottom -expand 1 -in .ctan.cta3 -fill both -padx 2 -pady 2
pack .ctan.cta3 -expand 1 -in .ctan -fill both -padx 2 -pady 2

# Packing Adaptive Median frame...
pack .ctan.cta4 -side top
pack .ctan.cta4.group  .ctan.cta4.process -side top
pack .ctan.cta4.group.l1 -in .ctan.cta4.group -side left -fill none -expand 0
pack .ctan.cta4.group.kernelradius \
        -padx 23 -pady 2  -in .ctan.cta4.group -side left -fill both -expand 1
pack .ctan.cta4.process.minamflag .ctan.cta4.process.maxamflag  \
     .ctan.cta4.process.meanamflag .ctan.cta4.process.medamflag \
      -side left -fill both -expand 1
pack  .ctan.cta4.b1 \
      -side bottom -expand 1 -in .ctan.cta4 -fill both -padx 2 -pady 2
pack .ctan.cta4 -expand 1 -in .ctan -fill both -padx 2 -pady 2
pack  .ctan.composite \
      -side top -expand 1 -in .ctan -fill both -padx 2 -pady 2

# packing MIP frame 
pack .ctan.cta5.group -side top
pack .ctan.cta5.group.l1 -in .ctan.cta5.group -side left -fill none -expand 0
pack .ctan.cta5.group.lowerprj .ctan.cta5.group.upperprj -padx 2 -pady 2 \
	-in .ctan.cta5.group  -side left -fill both -expand 1
pack  .ctan.cta5.b1 \
      -side bottom -expand 1 -in .ctan.cta5 -fill both -padx 2 -pady 2
pack  .ctan.cta5 -expand 1 -in .ctan -fill both -padx 2 -pady 2

pack  .ctan.update \
      -side top -expand 1 -in .ctan -fill both -padx 2 -pady 2
pack  .ctan.wl \
      -side top -expand 1 -in .ctan -fill both -padx 2 -pady 2
pack  .ctan.save \
      -side top -expand 1 -in .ctan -fill both -padx 2 -pady 2
pack  .ctan.exit \
      -side top -expand 1 -in .ctan -fill both -padx 2 -pady 2



#-----------------------------------------------------------------
global slicenumber numslices kernelradius thresholdvalue invalue
global lprojection uprojection
.ctan.cta1.group.lowerth insert 0 $invalue
.ctan.cta1.group.upperth insert 0 $thresholdvalue
bind .ctan.cta1.group.lowerth <Return> { Threshold }
bind .ctan.cta1.group.upperth <Return> { Threshold }

.ctan.cta2.group.threshold insert 0 $thresholdvalue
.ctan.cta2.group.plth insert 0 "90"

bind .ctan.cta2.group.threshold <Return> { ConnTH }
bind .ctan.cta2.group.plth      <Return> { ConnTH }

.ctan.cta3.group.st insert 0 $kernelradius
bind .ctan.cta3.group.st        <Return> { SetST }

.ctan.cta4.group.kernelradius insert 0 $kernelradius
bind .ctan.cta4.group.kernelradius <Return> { AMedian }

.ctan.cta5.group.lowerprj insert 0 $lprojection
.ctan.cta5.group.upperprj insert 0 $uprojection
bind .ctan.cta5.group.lowerprj <Return> { CHGMIP }
bind .ctan.cta5.group.upperprj <Return> { CHGMIP }

proc LData {} {
    wm deiconify .load_param
}
proc PopupWL {} {
  wm deiconify .wl
}
proc DData {} {
   global  slicenumber viewer1 level window
   viewer1 SetCoordinate2 $slicenumber
   viewer1 SetInput [reader GetOutput]
   viewer1 SetXOffset 0
   viewer1 SetYOffset 0
   viewer1 SetColorLevel $level
   viewer1 SetColorWindow $window
   viewer1 Render
}
proc DSI {} {
   global viewer1 slicenumber level window
   viewer1 SetInput [ss GetOutput]
   viewer1 SetXOffset 0
   viewer1 SetYOffset 0
   viewer1 SetColorLevel $level
   viewer1 SetColorWindow $window
   viewer1 Render
}
proc DTHI {} {
   global viewer1 slicenumber subx level window
   viewer1 SetCoordinate2 $slicenumber
   viewer1 SetInput [thresh2 GetOutput]; 
   set scale [expr 512/$subx]
   if { $scale == 512} { 
        viewer1 SetYOffset 0
   } else { viewer1 SetYOffset [expr 512/$subx];}
   viewer1 SetXOffset 0
   viewer1 SetColorLevel $level
   viewer1 SetColorWindow $window
   viewer1 Render
}
proc DCI {} {
   global viewer1 slicenumber subx
   viewer1 SetCoordinate2 $slicenumber
   viewer1 SetInput [connect GetOutput]
   set scale [expr 512/$subx]
   viewer1 SetYOffset 0
   if { $scale == 512} { 
      viewer1 SetXOffset 0; 
   } else { viewer1 SetXOffset [expr 512/$subx]; }
   viewer1 SetColorLevel 1
   viewer1 SetColorWindow 1
   viewer1 Render
}
proc DMBI {} {
   global viewer1 slicenumber subx
   viewer1 SetCoordinate2 $slicenumber
   viewer1 SetInput [mb GetOutput]
   set scale [expr 512/$subx]
   viewer1 SetYOffset 0
   if { $scale == 512} { 
      viewer1 SetXOffset 0; 
   } else { viewer1 SetXOffset [expr 512/$subx]; }
   viewer1 SetColorLevel 1
   viewer1 SetColorWindow 1
   viewer1 Render
}
proc DAMI {} {
   global viewer1 slicenumber subx level window
   viewer1 SetCoordinate2 $slicenumber
   viewer1 SetInput [af GetOutput]
   set scale [expr 512/$subx]
   if { $scale == 512} { 
      viewer1 SetXOffset 0; 
      viewer1 SetYOffset 0
   } else { viewer1 SetYOffset [expr 512/$subx]; viewer1 SetXOffset [expr 512/$subx]; }
   viewer1 SetColorLevel $level
   viewer1 SetColorWindow $window
   viewer1 Render
}
proc Update {} {
   global viewer thresh2 thresh connect mb af mip subx suby subz
   global numslices kernelradius window level
   puts "Updating : ..."
thresh2 ThresholdByLower [.ctan.cta1.group.lowerth get]
thresh  ThresholdByUpper [.ctan.cta1.group.upperth get]
   puts "Lower & Upper TH: [.ctan.cta1.group.lowerth get] [.ctan.cta1.group.upperth get] "
   set plsvlevel  [expr [.ctan.cta2.group.plth get]/100.0]
connect SetPLevelSeedValue $plsvlevel
connect SetThreshold [.ctan.cta2.group.threshold get]
   puts "Connectivity:TH & %MaxTH= [.ctan.cta2.group.threshold get] $plsvlevel"
   set kernelradius  [.ctan.cta3.group.st get]
mb SetKernelRadius $kernelradius
   puts "Boundary Thinkness: $kernelradius"
   set x1 [.ctan.cta4.group.kernelradius get]
   puts "Adaptive Kernel Radius = $x1"
af SetKernelDimensions $x1 $x1 $x1
   set subx   [.load_param.t1.group2.arx get]
   set suby   [.load_param.t1.group2.ary get]
   set subz   [.load_param.t1.group2.arz get]
   puts "Composite Filter with Magnification facotrs: $subx $suby $subz"
composite SetMagnificationFactors $subx $suby $subz
   puts "MIP Projection Range: $subz [expr $numslices - 1 - $subz]"
mip SetProjectionRange $subz [expr $numslices-1-$subz]
   puts "Rendering ..."
viewer SetInput [mip GetOutput]
   viewer SetColorLevel $level
   viewer SetColorWindow $window
viewer Render
}
proc Composite {} {
   global slicenumber viewer level window
   viewer SetCoordinate2 $slicenumber
   viewer SetInput [composite GetOutput]
   viewer SetXOffset 512
   viewer SetYOffset 0
   viewer SetColorLevel $level
   viewer SetColorWindow $window
   viewer Render
}
proc DMIP {} {
   global  slicenumber viewer level window
   viewer SetCoordinate2 0
   viewer SetInput [mip GetOutput]
   viewer SetXOffset 512
   viewer SetYOffset 0
   viewer SetColorLevel $level
   viewer SetColorWindow $window
   viewer Render
}
proc Save {} {
   global writer
   writer Write
}
proc Exit {} {
   exit
}
proc Threshold {} {
   global viewer connect thresh thresh2
   thresh2 ThresholdByLower [.ctan.cta1.group.lowerth get]
   thresh  ThresholdByUpper [.ctan.cta1.group.upperth get]
   .ctan.cta2.group.threshold delete 0 10
   .ctan.cta2.group.threshold insert 0 [.ctan.cta1.group.upperth get]
   connect SetThreshold     [.ctan.cta1.group.upperth get]
   viewer Render
}
proc ConnTH {} {
   global  viewer connect
   set plsvlevel  [expr [.ctan.cta2.group.plth get]/100.0]
   puts $plsvlevel
   connect SetPLevelSeedValue $plsvlevel
   connect SetThreshold [.ctan.cta2.group.threshold get]
   .ctan.cta1.group.upperth delete 0 10
   .ctan.cta1.group.upperth insert 0 [.ctan.cta2.group.threshold get]
   thresh  ThresholdByUpper [.ctan.cta2.group.threshold get]
   viewer Render
}
proc SetST {} {
   global viewer numslices kernelradius mb af
   set kernelradius  [.ctan.cta3.group.st get]
   mb SetKernelRadius $kernelradius
   .ctan.cta4.group.kernelradius delete 0 10
   .ctan.cta4.group.kernelradius insert 0 $kernelradius
   af SetKernelDimensions $kernelradius $kernelradius $kernelradius
   viewer Render
}
proc AMedian {} {
   global viewer af mb kernelradius
   set kernelradius [.ctan.cta4.group.kernelradius get]
   af SetKernelDimensions $kernelradius $kernelradius $kernelradius
   .ctan.cta3.group.st delete 0 10
   .ctan.cta3.group.st insert 0 $kernelradius
   mb SetKernelRadius $kernelradius
   viewer Render
}
proc SetMinimumFlag {} {
   global viewer ss
   ss MinimumOn; ss MaximumOff; ss MeanOff;ss MedianOff
   viewer Render
}
proc SetMaximumFlag {} {
   global viewer ss
   ss MinimumOff; ss  MaximumOn; ss MeanOff; ss MedianOff
   viewer Render
}
proc SetMeanFlag {} {
   global viewer ss
   ss MinimumOff; ss MaximumOff; ss MeanOn; ss MedianOff
   viewer Render
}
proc SetMedianFlag {} {
   global viewer ss
   ss MinimumOff; ss MaximumOff; ss MeanOff; ss MedianOn
   viewer Render
}

proc SetSix {} {
   global viewer connect
   connect SetNeighbors 6
   viewer Render
}
proc SetTwentySix {} {
   global viewer connect
   connect SetNeighbors 26
   viewer Render
}
proc SetMinAMFlag {} {
   global viewer af
   af MinimumOn; af MaximumOff; af MeanOff;af MedianOff
   viewer Render
}
proc SetMaxAMFlag {} {
   global viewer af
   af MinimumOff; af  MaximumOn; af MeanOff; af MedianOff
   viewer Render
}
proc SetMeanAMFlag {} {
   global viewer af
   af MinimumOff; af MaximumOff; af MeanOn; af MedianOff
   viewer Render
}
proc SetMedAMFlag {} {
   global viewer af
   af MinimumOff; af MaximumOff; af MeanOff; af MedianOn
   viewer Render
}

proc CHGMIP {} {
   global viewer lprojection uprojection 
   set lprojection [.ctan.cta5.group.lowerprj get]
   set uprojection [.ctan.cta5.group.upperprj get]
   mip SetProjectionRange  $lprojection $uprojection
   viewer Render
}

puts "Done"









