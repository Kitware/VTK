FTGL 1.32
April 23 2002

DESCRIPTION:

FTGL is a free open source library to enable developers to use arbitrary
fonts in their OpenGL (www.opengl.org)  applications.
Unlike other OpenGL font libraries FTGL uses standard font file formats
so doesn't need a preprocessing step to convert the high quality font data
into a lesser quality, proprietary format.
FTGL uses the Freetype (www.freetype.org) font library to open and 'decode'
the fonts. It then takes that output and stores it in a format most efficient
for OpenGL rendering.

Rendering modes supported are
- Bit maps
- Antialiased Pix maps
- Texture maps
- Outlines
- Polygon meshes
- Extruded polygon meshes

FTGL is designed to be used in commercial quality software. It has been
written with performance, robustness and simplicity in mind.

USAGE:

  FTGLPixmapFont font;
  
  font.Open( "Fonts:Arial");
  font.FaceSize( 72);
  
  font.render( "Hello World!");

This library was inspired by gltt, Copyright (C) 1998-1999 Stephane Rehel
(http://gltt.sourceforge.net)
Bezier curve code contributed by Jed Soane.
Demo, Linux port, extrusion code and gltt maintainance by Gerard Lanois
Linux port by Matthias Kretz
Windows port by Max Rheiner & Ellers
Bug fixes by Robert Osfield, Marcelo E. Magallon, Markku Rontu

Please contact me if you have any suggestions, feature requests, or problems.

Henry Maddocks
henryj@paradise.net.nz
http://homepages.paradise.net.nz/henryj/



//==============================================================================

Version 2??????
My initial design of FTGL was in 2 distinct parts...the freetype stuff and
the FTGL stuff. The freetype side contained wrappers for the freetype stuff
(surprise) and the ftgl side handled all the opengl stuff. All communication
was done via FTFace <-> FTFont. This felt right from a design point of view
because conceptually it made sense, it was clean, simple and it insulated
FTGL from changes in freetype. Up to version 1.3 I have rigidly stuck to
this 'rule'. Unfortunately this has been at the expense of the code. This
became most evident when dealing with char maps. Common sense would argue
that charmaps and the glyph container are intimately related, but because
of the 'rule' the communication path between them is...
FTGlyphContainer <-> FTFont <-> FTFace <-> FTCharMap
This is bollocks and has lead to some ugly code.
I am not about abandon the design completely, just the rule that says all
communication should be via FTFace <-> FTFont. I will still maintain
wrappers for freetype objects, but they will interface with ftgl in places
that make the most sense from a code efficiency point of view.

move glyph creation out of constructor, but load the freetype glyph and get
the metrics.
Change all dim stuff to float. Make my own floating point version of
FT_Vector.
Move Charmap to be owned by glyph container. See above
Try out cbloom sorted vector in charmap. faster than std::map?
Enable access to raw glyph data
State handling...
inline base class methods

Extreme Programming...


Things to think about...

The whole char size thing is major headache.
At the moment if you call font.CharSize( x) the glyph list is destroyed and
rebuilt, which will be really, really, really inefficient if you change sizes
often. Will the freetype cache stuff help? What about the new (FT 2.0.5)
FTSize public api.

When is the best time to construct the glyphList? After the call to Size(x)
is the earliest but what happens if the client doesn't set the char size?
Define a default size, check if glyphlist is valid in render function, if
not call size with default size.

good sites...
http://cgm.cs.mcgill.ca/~luc/
http://www.blackpawn.com/texts/lightmaps/default.html

glGetIntegerv( GL_TEXTURE_2D_BINDING_EXT, &activeTextureID);
should really check at run time.



Check that I do this properly..
============================

Dave Williss a ecrit :

Question:

If I do this...

    TT_New_Glyph(face, &glyph);
    for (i = 0 ; i < n ; ++i) {
        TT_Load_Glyph(instance, glyph, index[i], flags);
            ... use glyph...
    }

    TT_Done_Glyph(glyph)

Will I be leaking memory on each call to Load Glyph or
should I create and destroy the glyph handle for each call?
Seems terribily inefficient but to do that, but doing it as
above I seem to be leaking memory.


No, this is the correct behavior. Each call to TT_Load_Glyph
overwrites the previous content.. and this was designed on
purpose because the real content of a TT_Glyph object is
_really_ complex with TrueType, and you don't want to create
them on each glyph load..
