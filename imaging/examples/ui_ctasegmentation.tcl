# Simple viewer for images.
#make interface
wm withdraw .
toplevel .ctan  
wm title .ctan {CTA Segmentation}
wm resizable .ctan 0 0
#wm withdraw .ctan
# Interface to Load Data ...
frame .ctan.dd -relief ridge -borderwidth 2;
button .ctan.dd.b1 -text "Display Original Images" -command DData

frame .ctan.dd.slice
button .ctan.dd.slice.up -text "Slice Up" -command SliceUp
button .ctan.dd.slice.down -text "Slice Down" -command SliceDown

frame .ctan.dd.wl
frame .ctan.dd.wl.f1;
label .ctan.dd.wl.f1.windowLabel -text Window;
scale .ctan.dd.wl.f1.window -from 1 -to 2000 -orient horizontal -command SetWindow
frame .ctan.dd.wl.f2;
label .ctan.dd.wl.f2.levelLabel -text Level;
scale .ctan.dd.wl.f2.level -from 1 -to 2000 -orient horizontal -command SetLevel

# Interface for SubSampling ...
frame  .ctan.cta0 -relief ridge -borderwidth 2;
frame  .ctan.cta0.label
label  .ctan.cta0.label.l1 -text "SubSampling Using:";
frame  .ctan.cta0.process
radiobutton .ctan.cta0.process.minflag -text "Min" -value 0 -command SetMinimumFlag;
radiobutton .ctan.cta0.process.maxflag -text "Max"  -value 1 -command SetMaximumFlag;
radiobutton .ctan.cta0.process.meanflag -text "Mean"  -value 2 -command SetMeanFlag;
radiobutton .ctan.cta0.process.medianflag -text "Median" -value 3 -command SetMedianFlag;
button .ctan.cta0.b1 -text "Display Images" -command DSI
 
# Interface for Thresholding ...
frame .ctan.cta1 -relief ridge -borderwidth 2;
frame .ctan.cta1.group

label .ctan.cta1.group.l1 -text "Lower TH & Upper TH";
entry .ctan.cta1.group.lowerth -width 4
entry .ctan.cta1.group.upperth -width 4 
button .ctan.cta1.b1 -text "Display Images" -command DTHI

# Interface for Connectivity ...
frame .ctan.cta2 -relief ridge -borderwidth 2;
frame .ctan.cta2.group
frame .ctan.cta2.process
label .ctan.cta2.group.l1 -text "TH-%TH of Max";
entry .ctan.cta2.group.threshold -width 4 
entry .ctan.cta2.group.plth -width 4
radiobutton .ctan.cta2.process.six -text "6 Connected" -value 5 -command SetSix;
radiobutton .ctan.cta2.process.twentysix -text "26 Connected"  -value 6 -command SetTwentySix;
button .ctan.cta2.b1 -text "Display Images" -command DCI

# interface for Markboundary
frame .ctan.cta3 -relief ridge -borderwidth 2;
frame .ctan.cta3.group
label .ctan.cta3.group.l1 -text "Boundary Thickness";
entry .ctan.cta3.group.st -width 4
button .ctan.cta3.b1 -text "Display Images" -command DMBI

# Adaptive Median ...
# Interface for Thresholding ...
frame .ctan.cta4 -relief ridge -borderwidth 2;
frame .ctan.cta4.group

label .ctan.cta4.group.l1 -text "Adaptive Filter";
entry .ctan.cta4.group.xkernel -width 4
entry .ctan.cta4.group.ykernel -width 4
entry .ctan.cta4.group.zkernel -width 4
frame  .ctan.cta4.process
radiobutton .ctan.cta4.process.minamflag -text "Min" -value 7 -command SetMinAMFlag;
radiobutton .ctan.cta4.process.maxamflag -text "Max"  -value 8 -command SetMaxAMFlag;
radiobutton .ctan.cta4.process.meanamflag -text "Mean"  -value 9 -command SetMeanAMFlag;
radiobutton .ctan.cta4.process.medamflag -text "Median" -value 10 -command SetMedAMFlag;
button .ctan.cta4.b1 -text "Display Images" -command DAMI
button .ctan.update -text "Update" -command Update
button .ctan.exit -text "Exit" -command Exit


.ctan.dd.wl.f1.window set 500
.ctan.dd.wl.f2.level set 1100
# pack frames togther ...
pack .ctan.dd .ctan.cta0 .ctan.cta1 .ctan.cta2 .ctan.cta3 .ctan.cta4 -side top

# packing Display Data frame
pack .ctan.dd.slice .ctan.dd.wl -side bottom
pack .ctan.dd.b1 -side top -expand 1 -in .ctan.dd  -fill both -padx 5 -pady 5;
pack .ctan.dd -side top -expand 1 -in .ctan -fill both -padx 5 -pady 5;
pack .ctan.dd.slice.up .ctan.dd.slice.down -side right ;
pack .ctan.dd.wl.f1 .ctan.dd.wl.f2  -side top ;
pack .ctan.dd.wl.f1.windowLabel .ctan.dd.wl.f1.window -side left
pack .ctan.dd.wl.f2.levelLabel .ctan.dd.wl.f2.level -side left

# packing Subsampling frame 
pack .ctan.cta0 -side top
pack .ctan.cta0.label .ctan.cta0.process -side top
pack .ctan.cta0.label.l1  -side top -fill none -expand 0;
pack .ctan.cta0.process.minflag .ctan.cta0.process.maxflag .ctan.cta0.process.meanflag \
     .ctan.cta0.process.medianflag -side left -fill both -expand 1;
pack  .ctan.cta0.b1 \
      -side bottom -expand 1 -in .ctan.cta0 -fill both -padx 5 -pady 5;
pack .ctan.cta0 -expand 1 -in .ctan -fill both -padx 5 -pady 5;

# packing Thresholding frame 
pack .ctan.cta1.group -side top
pack .ctan.cta1.group.l1 -in .ctan.cta1.group -side top -fill none -expand 0;

pack .ctan.cta1.group.lowerth .ctan.cta1.group.upperth -padx 3 -pady 2 \
	-in .ctan.cta1.group  -side left -fill both -expand 1;
pack  .ctan.cta1.b1 \
      -side bottom -expand 1 -in .ctan.cta1 -fill both -padx 5 -pady 5;
pack .ctan.cta1 -expand 1 -in .ctan -fill both -padx 5 -pady 5;

# packing Connectivity frame 
pack .ctan.cta2 -side top
pack .ctan.cta2.group .ctan.cta2.process -side top
pack .ctan.cta2.group.l1 -in .ctan.cta2.group -side left -fill none -expand 0;
pack .ctan.cta2.group.threshold .ctan.cta2.group.plth -padx 3 -pady 2 \
	-in .ctan.cta2.group -side left -fill both -expand 1;
pack .ctan.cta2.process.six .ctan.cta2.process.twentysix -in .ctan.cta2.process \
	-side left -fill both -expand 1;
pack  .ctan.cta2.b1 \
      -side bottom -expand 1 -in .ctan.cta2 -fill both -padx 5 -pady 5;
pack .ctan.cta2 -expand 1 -in .ctan -fill both -padx 5 -pady 5;

# packing Mark Boundary frame 
pack .ctan.cta3 -side top
pack .ctan.cta3.group -side top
pack .ctan.cta3.group.l1 -in .ctan.cta3.group -side top -fill none -expand 0;
pack .ctan.cta3.group.st -padx 3 -pady 2 \
	-in .ctan.cta3.group -side left -fill both -expand 1;
pack  .ctan.cta3.b1 \
      -side bottom -expand 1 -in .ctan.cta3 -fill both -padx 5 -pady 5;
pack .ctan.cta3 -expand 1 -in .ctan -fill both -padx 5 -pady 5;

# Packing Adaptive Median frame...
pack .ctan.cta4 -side top
pack .ctan.cta4.group  .ctan.cta4.process -side top
pack .ctan.cta4.group.l1 -in .ctan.cta4.group -side top -fill none -expand 0;
pack .ctan.cta4.group.xkernel .ctan.cta4.group.ykernel .ctan.cta4.group.zkernel  \
        -padx 3 -pady 2  -in .ctan.cta4.group -side left -fill both -expand 1;
pack .ctan.cta4.process.minamflag .ctan.cta4.process.maxamflag  \
     .ctan.cta4.process.meanamflag .ctan.cta4.process.medamflag \
      -side left -fill both -expand 1;
pack  .ctan.cta4.b1 \
      -side bottom -expand 1 -in .ctan.cta4 -fill both -padx 5 -pady 5;
pack .ctan.cta4 -expand 1 -in .ctan -fill both -padx 5 -pady 5;

pack  .ctan.exit \
      -side bottom -expand 1 -in .ctan -fill both -padx 5 -pady 5;
pack  .ctan.update \
      -side bottom -expand 1 -in .ctan -fill both -padx 5 -pady 5;


.ctan.cta1.group.lowerth insert 0 "1050";
.ctan.cta1.group.upperth insert 0 "1350"
bind .ctan.cta1.group.lowerth <Return> { Threshold }
bind .ctan.cta1.group.upperth <Return> { Threshold }

.ctan.cta2.group.threshold insert 0 "1350";
.ctan.cta2.group.plth insert 0 "90";

bind .ctan.cta2.group.threshold <Return> { ConnTH }
bind .ctan.cta2.group.plth      <Return> { ConnTH }

.ctan.cta3.group.st insert 0 "1";
bind .ctan.cta3.group.st        <Return> { SetST }

.ctan.cta4.group.xkernel insert 0 "1";
.ctan.cta4.group.ykernel insert 0 "1";
.ctan.cta4.group.zkernel insert 0 "1";
bind .ctan.cta4.group.xkernel   <Return> { AMedian }
bind .ctan.cta4.group.ykernel   <Return> { AMedian }
bind .ctan.cta4.group.zkernel   <Return> { AMedian }

proc DData {} {
   global viewer
   global region   
   viewer SetInput [reader GetOutput];
   viewer Render;
}
proc DSI {} {
   global viewer
   global region   
   [ss GetOutput] UpdateRegion region;
   viewer SetInput [region GetOutput];
   viewer Render;
}
proc DTHI {} {
   global viewer
   global region   
   [thresh2 GetOutput] UpdateRegion region;
   viewer SetInput [region GetOutput];
   viewer Render;
}
proc DCI {} {
   global viewer
   global region   
   [connect GetOutput] UpdateRegion region;
   viewer SetInput [region GetOutput];
   viewer Render;
}
proc DMBI {} {
   global viewer
   global region   
   [mb GetOutput] UpdateRegion region;
   viewer SetInput [region GetOutput];
   viewer Render;
}
proc DAMI {} {
   global viewer
   global region   
   [af GetOutput] UpdateRegion region;
   viewer SetInput [region GetOutput];
   viewer Render;
}
proc Update {} {
   global viewer
   global region 
   thresh2 ThresholdByLower [.ctan.cta1.group.lowerth get];
   thresh  ThresholdByUpper [.ctan.cta1.group.upperth get];
   set plsvlevel  [expr [.ctan.cta2.group.plth get]/100.0];
   connect SetPLevelSeedValue $plsvlevel;
   connect SetThreshold [.ctan.cta2.group.threshold get];
   mb SetKernelRadius [.ctan.cta3.group.st get] ;
   set x1 [.ctan.cta4.group.xkernel get];
   set y1 [.ctan.cta4.group.ykernel get];
   set z1 [.ctan.cta4.group.zkernel get];
   af SetKernelDimensions $x1 $y1 $z1;
   [af GetOutput] UpdateRegion region;
   mip SetInput [ region GetOutput];
   mag SetInput [ mip GetOutput];
   viewer SetInput [mag GetOutput];
   viewer Render;
}
proc Exit {} {
   exit;
}
proc Threshold {} {
   global viewer
   global region   
   thresh2 ThresholdByLower [.ctan.cta1.group.lowerth get];
   thresh  ThresholdByUpper [.ctan.cta1.group.upperth get];
   [af GetOutput] UpdateRegion region;
   viewer SetInput [mag GetOutput];
   viewer Render;
}
proc ConnTH {} {
   global  viewer
   global  region
   set plsvlevel  [expr [.ctan.cta2.group.plth get]/100.0];
   puts $plsvlevel;
   global viewer
   global region   
   connect SetPLevelSeedValue $plsvlevel;
   connect SetThreshold [.ctan.cta2.group.threshold get];
   [af GetOutput] UpdateRegion region;
   viewer SetInput [mag GetOutput];
   viewer Render;
}
proc SetST {} {
   global viewer
   global region   
   mb SetKernelRadius [.ctan.cta3.group.st get] ;
   [af GetOutput] UpdateRegion region;
   viewer SetInput [mag GetOutput];
   viewer Render;
}
proc AMedian {} {
   global viewer
   global region   
   set x1 [.ctan.cta4.group.xkernel get];
   set y1 [.ctan.cta4.group.ykernel get];
   set z1 [.ctan.cta4.group.zkernel get];
   af SetKernelDimensions $x1 $y1 $z1;
   [af GetOutput] UpdateRegion region;
   viewer SetInput [mag GetOutput];
   viewer Render;
}
proc SetMinimumFlag {} {
   global viewer
   global region   
   ss MinimumOn; ss MaximumOff; ss MeanOff;ss MedianOff;
   [af GetOutput] UpdateRegion region;
   viewer Render;
}
proc SetMaximumFlag {} {
   global viewer
   global region   
   ss MinimumOff; ss  MaximumOn; ss MeanOff; ss MedianOff;
   [af GetOutput] UpdateRegion region;
   viewer Render;
}
proc SetMeanFlag {} {
   global viewer
   global region   
   ss MinimumOff; ss MaximumOff; ss MeanOn; ss MedianOff;
   [af GetOutput] UpdateRegion region;
   viewer Render;
}
proc SetMedianFlag {} {
   global viewer
   global region   
   ss MinimumOff; ss MaximumOff; ss MeanOff; ss MedianOn;
   [af GetOutput] UpdateRegion region;
   viewer Render;
}

proc SetSix {} {
   global viewer
   global region 
   connect SetNeighbors 6;
   [af GetOutput] UpdateRegion region;
   viewer SetInput [mag GetOutput];
   viewer Render;
}
proc SetTwentySix {} {
   global viewer
   global region   
   connect SetNeighbors 26;
   [af GetOutput] UpdateRegion region;
   viewer SetInput [mag GetOutput];
   viewer Render;
}
proc SetMinAMFlag {} {
   global viewer
   global region   
   af MinimumOn; af MaximumOff; af MeanOff;af MedianOff;
   [af GetOutput] UpdateRegion region;
   viewer Render;
}
proc SetMaxAMFlag {} {
   global viewer
   global region   
   af MinimumOff; af  MaximumOn; af MeanOff; af MedianOff;
   [af GetOutput] UpdateRegion region;
   viewer Render;
}
proc SetMeanAMFlag {} {
   global viewer
   global region   
   af MinimumOff; af MaximumOff; af MeanOn; af MedianOff;
   [af GetOutput] UpdateRegion region;
   viewer Render;
}
proc SetMedAMFlag {} {
   global viewer
   global region   
   af MinimumOff; af MaximumOff; af MeanOff; af MedianOn;
   [af GetOutput] UpdateRegion region;
   viewer Render;
}

proc SliceUp {} {
   global sliceNumber viewer
   if {$sliceNumber < 93} {set sliceNumber [expr $sliceNumber + 1]}
   puts $sliceNumber
   viewer SetCoordinate2 $sliceNumber;
   viewer Render;
}

proc SliceDown {} {
   global sliceNumber viewer
   if {$sliceNumber > 0} {set sliceNumber [expr $sliceNumber - 1]}
   puts $sliceNumber
   viewer SetCoordinate2 $sliceNumber;
   viewer Render;
}

proc SetWindow window {
   global viewer
   viewer SetColorWindow $window;
   viewer Render;
}

proc SetLevel level {
   global viewer
   viewer SetColorLevel $level;
   viewer Render;
}

puts "Done";


#$renWin Render
#wm withdraw .








