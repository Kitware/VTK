//
// Fragment shader for rendering a "confetti cannon"
// via a partcle system
//
// Author: Randi Rost
//
// Copyright (c) 2003-2004: 3Dlabs, Inc.
//
// See 3Dlabs-License.txt for license information
//

varying vec4 Color;

void main (void)
{
    gl_FragColor = Color;
}