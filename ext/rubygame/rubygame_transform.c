/*
 *  Surface rotation, zooming, and flipping functions.
 *
 *--
 *  Rubygame -- Ruby code and bindings to SDL to facilitate game creation
 *  Copyright (C) 2004-2005  John 'jacius' Croisant
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *++
 */

#include "rubygame.h"
#include "rubygame_surface.h"
#include "rubygame_transform.h"

void Rubygame_Init_Transform();

VALUE rbgm_transform_flip(int, VALUE*, VALUE);

#ifdef HAVE_SDL_ROTOZOOM_H

VALUE rbgm_transform_rotozoom(int, VALUE*, VALUE);
VALUE rbgm_transform_rotozoomsize(int, VALUE*, VALUE);

VALUE rbgm_transform_zoom(int, VALUE*, VALUE);
VALUE rbgm_transform_zoomsize(int, VALUE*, VALUE);

/*
 *  call-seq:
 *    rotozoom( angle, zoom, smooth )  ->  Surface
 *
 *  Return a rotated and/or zoomed version of the given surface. Note that
 *  rotating a Surface anything other than a multiple of 90 degrees will 
 *  cause the new surface to be larger than the original to accomodate the
 *  corners (which would otherwise extend beyond the surface).
 *
 *  If Rubygame was compiled with SDL_gfx-2.0.13 or greater, +zoom+ can be
 *  an Array of 2 Numerics for separate X and Y scaling. Also, it can be
 *  negative to indicate flipping horizontally or vertically.
 *
 *  Will raise SDLError if you attempt to use separate X and Y zoom factors
 *  or negative zoom factors with an unsupported version of SDL_gfx.
 *
 *  This method takes these arguments:
 *  angle::   degrees to rotate counter-clockwise (negative for clockwise).
 *  zoom::    scaling factor(s). A single positive Numeric, unless you have
 *            SDL_gfx-2.0.13 or greater (see above).
 *  smooth::  whether to anti-alias the new surface. This option can be
 *            omitted, in which case the surface will not be anti-aliased.
 *            If true, the new surface will be 32bit RGBA.
 */
VALUE rbgm_transform_rotozoom(int argc, VALUE *argv, VALUE self)
{
  SDL_Surface *src, *dst;
  double angle, zoomx, zoomy;
  int smooth = 0;

  if(argc < 3)
    rb_raise(rb_eArgError,"wrong number of arguments (%d for 3)",argc);

  /* argv[0], the source surface. */
  Data_Get_Struct(self,SDL_Surface,src);

  /* argv[1], the angle of rotation. */
  angle = NUM2DBL(argv[0]);

  /* Parsing of argv[2] is delayed until below, because its type
     affects which function we call. */

  /* argv[3] (optional), rotozoom smoothly? */
  if(argc > 2)
    smooth = argv[2];

  /* argv[1], the zoom factor(s) */
  if(TYPE(argv[1])==T_ARRAY)		/* if we got separate X and Y factors */
  {

#ifdef HAVE_ROTOZOOMXY
    /* Do the real function. */
    zoomx = NUM2DBL(rb_ary_entry(argv[1],0));
    zoomy = NUM2DBL(rb_ary_entry(argv[1],1));
    dst = rotozoomSurfaceXY(src, angle, zoomx, zoomy, smooth);
    if(dst == NULL)
      rb_raise(eSDLError,"Could not rotozoom surface: %s",SDL_GetError());
#else
    /* Raise SDLError. You should have checked first! */
    rb_raise(eSDLError,"Separate X/Y rotozoom scale factors is not supported by your version of SDL_gfx (%d,%d,%d). Please upgrade to 2.0.13 or later.", SDL_GFXPRIMITIVES_MAJOR, SDL_GFXPRIMITIVES_MINOR, SDL_GFXPRIMITIVES_MICRO);
    return Qnil;
#endif

  }
  /* If we got 1 zoom factor for both X and Y */
  else if(FIXNUM_P(argv[1]) || TYPE(argv[1])==T_FLOAT)
  {
    zoomx = NUM2DBL(argv[1]);
#ifndef HAVE_ROTOZOOMXY
    if(zoomx < 0)								/* negative zoom (for flipping) */
    {
      /* Raise SDLError. You should have checked first! */
      rb_raise(eSDLError,"Negative rotozoom scale factor is not supported by your version of SDL_gfx (%d,%d,%d). Please upgrade to 2.0.13 or later.", SDL_GFXPRIMITIVES_MAJOR, SDL_GFXPRIMITIVES_MINOR, SDL_GFXPRIMITIVES_MICRO);
    }
#endif
    dst = rotozoomSurface(src, angle, zoomx, smooth);
    if(dst == NULL)
      rb_raise(eSDLError,"Could not rotozoom surface: %s",SDL_GetError());
  }
  else
    rb_raise(rb_eArgError,"wrong zoom factor type (expected Array or Numeric)");

  return Data_Wrap_Struct(cSurface,0,SDL_FreeSurface,dst);
}

/*
 *  call-seq:
 *    rotozoom_size( size, angle, zoom )  ->  [width, height] or nil
 *
 *  Return the dimensions of the surface that would be returned if
 *  #rotozoom() were called on a Surface of the given size, with
 *  the same angle and zoom factors.
 *
 *  If Rubygame was compiled with SDL_gfx-2.0.13 or greater, +zoom+ can be
 *  an Array of 2 Numerics for separate X and Y scaling. Also, it can be
 *  negative to indicate flipping horizontally or vertically.
 *
 *  Will return +nil+ if you attempt to use separate X and Y zoom factors
 *  or negative zoom factors with an unsupported version of SDL_gfx.
 *
 *  This method takes these arguments:
 *  size::  an Array with the hypothetical Surface width and height (pixels)
 *  angle:: degrees to rotate counter-clockwise (negative for clockwise).
 *  zoom::  scaling factor(s). A single positive Numeric, unless you have
 *          SDL_gfx-2.0.13 or greater (see above).
 */
VALUE rbgm_transform_rzsize(int argc, VALUE *argv, VALUE module)
{
  int w,h, dstw,dsth;
  double angle, zoomx, zoomy;

  if(argc < 3)
    rb_raise(rb_eArgError,"wrong number of arguments (%d for 3)",argc);
  w = NUM2INT(rb_ary_entry(argv[0],0));
  h = NUM2INT(rb_ary_entry(argv[0],0));
  angle = NUM2DBL(argv[1]);

  if(TYPE(argv[2])==T_ARRAY)
  {
/* Separate X/Y rotozoom scaling was not supported prior to 2.0.13. */
/* Check if we have at least version 2.0.13 of SDL_gfxPrimitives */
#ifdef HAVE_ROTOZOOMXY
    /* Do the real function. */
    zoomx = NUM2DBL(rb_ary_entry(argv[1],0));
    zoomy = NUM2DBL(rb_ary_entry(argv[1],1));
    rotozoomSurfaceSizeXY(w, h, angle, zoomx, zoomy, &dstw, &dsth);

#else 
    /* Return nil, because it's not supported. */
    return Qnil;
#endif

  }
  else if(FIXNUM_P(argv[1]) || TYPE(argv[1])==T_FLOAT)
  {
    zoomx = NUM2DBL(argv[1]);
#ifndef HAVE_ROTOZOOMXY
    if(zoomx < 0)								/* negative zoom (for flipping) */
    {
			/* Return nil, because it's not supported. */
			return Qnil;
    }
#endif
    rotozoomSurfaceSize(w, h, angle, zoomx, &dstw, &dsth);
  }
  else
    rb_raise(rb_eArgError,"wrong zoom factor type (expected Array or Numeric)");


  /*   if(dstw == NULL || dsth == NULL)
     rb_raise(eSDLError,"Could not rotozoom surface: %s",SDL_GetError());*/
  return rb_ary_new3(2,INT2NUM(dstw),INT2NUM(dsth));

}

#if 0
VALUE rbgm_transform_rotozoomsize(int argc, VALUE *argv, VALUE module)
{
  int w,h, dstw,dsth;
  double angle, zoom;

  if(argc < 3)
    rb_raise(rb_eArgError,"wrong number of arguments (%d for 3)",argc);
  w = NUM2INT(rb_ary_entry(argv[0],0));
  h = NUM2INT(rb_ary_entry(argv[0],0));
  angle = NUM2DBL(argv[1]);
  zoom = NUM2DBL(argv[2]);

  rotozoomSurfaceSize(w, h, angle, zoom, &dstw, &dsth);
  return rb_ary_new3(2,INT2NUM(dstw),INT2NUM(dsth));
}
#endif

/* 
 *  call-seq:
 *     zoom(zoom, smooth)  ->  Surface
 *
 *  Return a zoomed version of the Surface.
 *
 *  This method takes these arguments:
 *  zoom::    the factor to scale by in both x and y directions, or an Array
 *            with separate x and y scale factors.
 *  smooth::  whether to anti-alias the new surface. This option can be
 *            omitted, in which case the surface will not be anti-aliased.
 *            If true, the new surface will be 32bit RGBA.
 */
VALUE rbgm_transform_zoom(int argc, VALUE *argv, VALUE self)
{
  SDL_Surface *src, *dst;
  double zoomx, zoomy;
  int smooth = 0;

  if(argc < 2)									/* smooth is optional */
    rb_raise(rb_eArgError,"wrong number of arguments (%d for 1)",argc);
  Data_Get_Struct(self,SDL_Surface,src);

  if(TYPE(argv[0])==T_ARRAY)
  {
    zoomx = NUM2DBL(rb_ary_entry(argv[0],0));
    zoomy = NUM2DBL(rb_ary_entry(argv[0],1));
  }
  else if(FIXNUM_P(argv[0]) || TYPE(argv[0])==T_FLOAT)
  {
    zoomx = NUM2DBL(argv[0]);
    zoomy = zoomx;
  }
  else
    rb_raise(rb_eArgError,"wrong zoom factor type (expected Array or Numeric)");

  if(argc > 1)
    smooth = argv[1];

  dst = zoomSurface(src,zoomx,zoomy,smooth);
  if(dst == NULL)
    rb_raise(eSDLError,"Could not rotozoom surface: %s",SDL_GetError());
  return Data_Wrap_Struct(cSurface,0,SDL_FreeSurface,dst);
}

/* 
 *  call-seq:
 *    zoom_size(size, zoom)  ->  [width, height]
 *
 *  Return the dimensions of the surface that would be returned if
 *  #zoom were called with a surface of the given size and zoom factors.
 *
 *  This method takes these arguments:
 *  size:: an Array with the hypothetical surface width and height (pixels)
 *  zoom:: the factor to scale by in both x and y directions, or an Array
 *         with separate x and y scale factors.
 */
VALUE rbgm_transform_zoomsize(int argc, VALUE *argv, VALUE module)
{
  int w,h, dstw,dsth;
  double zoomx, zoomy;

  if(argc < 3)
    rb_raise(rb_eArgError,"wrong number of arguments (%d for 3)",argc);
  w = NUM2INT(rb_ary_entry(argv[0],0));
  h = NUM2INT(rb_ary_entry(argv[0],0));

  if(TYPE(argv[1])==T_ARRAY)
  {
    zoomx = NUM2DBL(rb_ary_entry(argv[1],0));
    zoomy = NUM2DBL(rb_ary_entry(argv[1],1));
  }
  else if(FIXNUM_P(argv[1]) || TYPE(argv[1])==T_FLOAT)
  {
    zoomx = NUM2DBL(argv[1]);
    zoomy = zoomx;
  }
  else
    rb_raise(rb_eArgError,"wrong zoom factor type (expected Array or Numeric)");

  zoomSurfaceSize(w, h,  zoomx, zoomy, &dstw, &dsth);
  return rb_ary_new3(2,INT2NUM(dstw),INT2NUM(dsth));
}

#endif /* HAVE_SDL_ROTOZOOM_H */

/* --
 * Borrowed from Pygame:
 * ++
 */
static SDL_Surface* newsurf_fromsurf(SDL_Surface* surf, int width, int height)
{
  SDL_Surface* newsurf;

  if(surf->format->BytesPerPixel <= 0 || surf->format->BytesPerPixel > 4)
    rb_raise(eSDLError,"unsupported Surface bit depth for transform");

  newsurf = SDL_CreateRGBSurface(surf->flags, width, height, surf->format->BitsPerPixel,
        surf->format->Rmask, surf->format->Gmask, surf->format->Bmask, surf->format->Amask);
  if(!newsurf)
    rb_raise(eSDLError,"%s",SDL_GetError());

  /* Copy palette, colorkey, etc info */
  if(surf->format->BytesPerPixel==1 && surf->format->palette)
    SDL_SetColors(newsurf, surf->format->palette->colors, 0, surf->format->palette->ncolors);
  if(surf->flags & SDL_SRCCOLORKEY)
    SDL_SetColorKey(newsurf, (surf->flags&SDL_RLEACCEL)|SDL_SRCCOLORKEY, surf->format->colorkey);

  return newsurf;
}

/* 
 * call-seq:
 *    flip(horz, vert)  ->  Surface
 * 
 *  Flips the source surface horizontally (if +horz+ is true), vertically
 *  (if +vert+ is true), or both (if both are true). This operation is
 *  non-destructive; the original image can be perfectly reconstructed by
 *  flipping the resultant image again.
 *
 *  This operation does NOT require SDL_gfx.
 *
 *  A similar effect can (supposedly) be achieved by giving X or Y zoom
 *  factors of -1 to #rotozoom (only if compiled with SDL_gfx 2.0.13 or
 *  greater). Your mileage may vary.
 */
VALUE rbgm_transform_flip(int argc, VALUE *argv, VALUE self)
{
  SDL_Surface *surf, *newsurf;
  int xaxis, yaxis;

  int loopx, loopy;
  int pixsize, srcpitch, dstpitch;
  Uint8 *srcpix, *dstpix;

  xaxis = argv[0];
  yaxis = argv[1];

  if(argc < 2)
    rb_raise(rb_eArgError,"wrong number of arguments (%d for 2)",argc);
  Data_Get_Struct(self,SDL_Surface,surf);


  /* Borrowed from Pygame: */
  newsurf = newsurf_fromsurf(surf, surf->w, surf->h);
  if(!newsurf)
    return Qnil;

  pixsize = surf->format->BytesPerPixel;
  srcpitch = surf->pitch;
  dstpitch = newsurf->pitch;

  SDL_LockSurface(newsurf);

  srcpix = (Uint8*)surf->pixels;
  dstpix = (Uint8*)newsurf->pixels;

  if(!xaxis)
  {
    if(!yaxis)
    {
      for(loopy = 0; loopy < surf->h; ++loopy)
        memcpy(dstpix+loopy*dstpitch, srcpix+loopy*srcpitch, surf->w*surf->format->BytesPerPixel);
    }
    else
    {
      for(loopy = 0; loopy < surf->h; ++loopy)
        memcpy(dstpix+loopy*dstpitch, srcpix+(surf->h-1-loopy)*srcpitch, surf->w*surf->format->BytesPerPixel);
    }
  }
  else /*if (xaxis)*/
  {
    if(yaxis)
    {
      switch(surf->format->BytesPerPixel)
      {
      case 1:
        for(loopy = 0; loopy < surf->h; ++loopy) {
          Uint8* dst = (Uint8*)(dstpix+loopy*dstpitch);
          Uint8* src = ((Uint8*)(srcpix+(surf->h-1-loopy)*srcpitch)) + surf->w - 1;
          for(loopx = 0; loopx < surf->w; ++loopx)
            *dst++ = *src--;
        }break;
      case 2:
        for(loopy = 0; loopy < surf->h; ++loopy) {
          Uint16* dst = (Uint16*)(dstpix+loopy*dstpitch);
          Uint16* src = ((Uint16*)(srcpix+(surf->h-1-loopy)*srcpitch)) + surf->w - 1;
          for(loopx = 0; loopx < surf->w; ++loopx)
            *dst++ = *src--;
        }break;
      case 4:
        for(loopy = 0; loopy < surf->h; ++loopy) {
          Uint32* dst = (Uint32*)(dstpix+loopy*dstpitch);
          Uint32* src = ((Uint32*)(srcpix+(surf->h-1-loopy)*srcpitch)) + surf->w - 1;
          for(loopx = 0; loopx < surf->w; ++loopx)
            *dst++ = *src--;
        }break;
      case 3:
        for(loopy = 0; loopy < surf->h; ++loopy) {
          Uint8* dst = (Uint8*)(dstpix+loopy*dstpitch);
          Uint8* src = ((Uint8*)(srcpix+(surf->h-1-loopy)*srcpitch)) + surf->w*3 - 3;
          for(loopx = 0; loopx < surf->w; ++loopx)
          {
            dst[0] = src[0]; dst[1] = src[1]; dst[2] = src[2];
            dst += 3;
            src -= 3;
          }
        }break;
      }
    }
    else
    {
      switch(surf->format->BytesPerPixel)
      {
      case 1:
        for(loopy = 0; loopy < surf->h; ++loopy) {
          Uint8* dst = (Uint8*)(dstpix+loopy*dstpitch);
          Uint8* src = ((Uint8*)(srcpix+loopy*srcpitch)) + surf->w - 1;
          for(loopx = 0; loopx < surf->w; ++loopx)
            *dst++ = *src--;
        }break;
      case 2:
        for(loopy = 0; loopy < surf->h; ++loopy) {
          Uint16* dst = (Uint16*)(dstpix+loopy*dstpitch);
          Uint16* src = ((Uint16*)(srcpix+loopy*srcpitch)) + surf->w - 1;
          for(loopx = 0; loopx < surf->w; ++loopx)
            *dst++ = *src--;
        }break;
      case 4:
        for(loopy = 0; loopy < surf->h; ++loopy) {
          Uint32* dst = (Uint32*)(dstpix+loopy*dstpitch);
          Uint32* src = ((Uint32*)(srcpix+loopy*srcpitch)) + surf->w - 1;
          for(loopx = 0; loopx < surf->w; ++loopx)
            *dst++ = *src--;
        }break;
      case 3:
        for(loopy = 0; loopy < surf->h; ++loopy) {
          Uint8* dst = (Uint8*)(dstpix+loopy*dstpitch);
          Uint8* src = ((Uint8*)(srcpix+loopy*srcpitch)) + surf->w*3 - 3;
          for(loopx = 0; loopx < surf->w; ++loopx)
          {
            dst[0] = src[0]; dst[1] = src[1]; dst[2] = src[2];
            dst += 3;
            src -= 3;
          }
        }break;
      }
    }
  }

  SDL_UnlockSurface(newsurf);
  /* Thanks, Pygame :) */


  if(newsurf == NULL)
    rb_raise(eSDLError,"Could not flip surface: %s",SDL_GetError());
  return Data_Wrap_Struct(cSurface,0,SDL_FreeSurface,newsurf);
}


void Rubygame_Init_Transform()
{
/* Pretend to define Rubygame and Surface, so RDoc knows about them: */
#if 0
  mRubygame = rb_define_module("Rubygame");
	cSurface = rb_define_class_under(mRubygame,"Surface",rb_cObject);
#endif

  rb_define_method(cSurface,"flip",rbgm_transform_flip,-1);

#ifdef HAVE_SDL_ROTOZOOM_H

  rb_define_method(cSurface,"rotozoom",rbgm_transform_rotozoom,-1);
  rb_define_method(cSurface,"zoom",rbgm_transform_zoom,-1);

  rb_define_module_function(cSurface,"rotozoom_size",rbgm_transform_rzsize,-1);
  rb_define_module_function(cSurface,"zoom_size",rbgm_transform_zoomsize,-1);

#endif
}


