/*
 * background.sl -- imager shader to put a solid background color behind
 *                  an image.
 *
 * DESCRIPTION:
 *   Puts a solid background color in all the pixels which do not contain
 *   foreground objects.
 * 
 * PARAMETERS:
 *   bgcolor - the color of the background
 *
 * AUTHOR: Larry Gritz
 *
 * HISTORY:
 *    16 June 1994 - written by Larry Gritz
 *
 * last modified  16 June 1994 by Larry Gritz
 */



imager background (color bgcolor = color(0,0,1))
{
  Ci += (1-alpha) * bgcolor;
  Oi = 1;
  alpha = 1;
}
