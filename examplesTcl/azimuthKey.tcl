#
# sample script to generate a sequence of azimuthal changes
# to the current camera
#

set camera [ren1 GetActiveCamera]
KeyNew Azimuth $camera SetPosition

#
# define the Render proc for key framing
#
proc KeyRender {} {
    [ren1 GetLights] InitTraversal
    set light [[ren1 GetLights] GetNextItem]
    set camera [ren1 GetActiveCamera]
    $camera SetViewUp 0 -1 0
    $camera ComputeViewPlaneNormal
    $camera SetClippingRange 10 5000
    eval $light SetPosition [$camera GetPosition]
    eval $light SetFocalPoint [$camera GetFocalPoint]
    renWin Render
}

#
# define the key frames
#
KeyAdd Azimuth [$camera GetPosition]
$camera Azimuth 1
KeyAdd Azimuth [$camera GetPosition]
$camera Azimuth 5
KeyAdd Azimuth [$camera GetPosition]
for {set i 0} {$i <= 8} {incr i} {
  $camera Azimuth 10
  KeyAdd Azimuth [$camera GetPosition]
}
$camera Azimuth 5
KeyAdd Azimuth [$camera GetPosition]
$camera Azimuth 1
KeyAdd Azimuth [$camera GetPosition]

$camera Azimuth -1
KeyAdd Azimuth [$camera GetPosition]
$camera Azimuth -5
KeyAdd Azimuth [$camera GetPosition]
for {set i 0} {$i <= 8} {incr i} {
  $camera Azimuth -10
  KeyAdd Azimuth [$camera GetPosition]
}
$camera Azimuth -5
KeyAdd Azimuth [$camera GetPosition]
$camera Azimuth -1
KeyAdd Azimuth [$camera GetPosition]

KeyRun Azimuth 30
