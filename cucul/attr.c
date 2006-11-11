/*
 *  libcucul      Canvas for ultrafast compositing of Unicode letters
 *  Copyright (c) 2002-2006 Sam Hocevar <sam@zoy.org>
 *                All Rights Reserved
 *
 *  $Id$
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the Do What The Fuck You Want To
 *  Public License, Version 2, as published by Sam Hocevar. See
 *  http://sam.zoy.org/wtfpl/COPYING for more details.
 */

/*
 *  This file contains functions for attribute management and colourspace
 *  conversions.
 */

#include "config.h"
#include "common.h"

#if defined(HAVE_ERRNO_H)
#   include <errno.h>
#endif

#include "cucul.h"
#include "cucul_internals.h"

static uint8_t nearest_ansi(uint16_t);

/** \brief Get the text attribute at the given coordinates.
 *
 *  Get the internal \e libcucul attribute value of the character at the
 *  given coordinates. The attribute value has 32 significant bits,
 *  organised as follows from MSB to LSB:
 *  - 3 bits for the background alpha
 *  - 4 bits for the background red component
 *  - 4 bits for the background green component
 *  - 3 bits for the background blue component
 *  - 3 bits for the foreground alpha
 *  - 4 bits for the foreground red component
 *  - 4 bits for the foreground green component
 *  - 3 bits for the foreground blue component
 *  - 4 bits for the bold, italics, underline and blink flags
 *
 *  If the coordinates are outside the canvas boundaries, the current
 *  attribute is returned.
 *
 *  This function never fails.
 *
 *  \param cv A handle to the libcucul canvas.
 *  \param x X coordinate.
 *  \param y Y coordinate.
 *  \return The requested attribute.
 */
unsigned long int cucul_get_attr(cucul_canvas_t *cv, int x, int y)
{
    if(x < 0 || x >= (int)cv->width || y < 0 || y >= (int)cv->height)
        return (unsigned long int)cv->curattr;

    return (unsigned long int)cv->attrs[x + y * cv->width];
}

/** \brief Set the default character attribute.
 *
 *  Set the default character attribute for drawing. Attributes define
 *  foreground and background colour, transparency, bold, italics and
 *  underline styles, as well as blink. String functions such as
 *  caca_printf() and graphical primitive functions such as caca_draw_line()
 *  will use this attribute.
 *
 *  The value of \e attr is either:
 *  - a 32-bit integer as returned by cucul_get_attr(), in which case it
 *    also contains colour information,
 *  - a combination (bitwise OR) of style values (\e CUCUL_UNDERLINE,
 *    \e CUCUL_BLINK, \e CUCUL_BOLD and \e CUCUL_ITALICS), in which case
 *    setting the attribute does not modify the current colour information.
 *
 *  To retrieve the current attribute value, use cucul_get_attr(-1,-1).
 *
 *  If an error occurs, -1 is returned and \b errno is set accordingly:
 *  - \c EINVAL The attribute value is out of the 32-bit range.
 *
 *  \param cv A handle to the libcucul canvas.
 *  \param attr The requested attribute value.
 *  \return 0 in case of success, -1 if an error occurred.
 */
int cucul_set_attr(cucul_canvas_t *cv, unsigned long int attr)
{
    if(sizeof(unsigned long int) > sizeof(uint32_t) && attr > 0xffffffff)
    {
#if defined(HAVE_ERRNO_H)
        errno = EINVAL;
#endif
        return -1;
    }

    if(attr < 0x00000010)
        attr = (cv->curattr & 0xfffffff0) | attr;

    cv->curattr = attr;

    return 0;
}

/** \brief Set the character attribute at the given coordinates.
 *
 *  Set the character attribute, without changing the character's value. If
 *  the character at the given coordinates is a fullwidth character, both
 *  cells' attributes are replaced.
 *
 *  The value of \e attr is either:
 *  - a 32-bit integer as returned by cucul_get_attr(), in which case it
 *    also contains colour information,
 *  - a combination (bitwise OR) of style values (\e CUCUL_UNDERLINE,
 *    \e CUCUL_BLINK, \e CUCUL_BOLD and \e CUCUL_ITALICS), in which case
 *    setting the attribute does not modify the current colour information.
 *
 *  If an error occurs, -1 is returned and \b errno is set accordingly:
 *  - \c EINVAL The attribute value is out of the 32-bit range.
 *
 *  \param cv A handle to the libcucul canvas.
 *  \param x X coordinate.
 *  \param y Y coordinate.
 *  \param attr The requested attribute value.
 *  \return 0 in case of success, -1 if an error occurred.
 */
int cucul_putattr(cucul_canvas_t *cv, int x, int y, unsigned long int attr)
{
    uint32_t *curattr, *curchar;

    if(sizeof(unsigned long int) > sizeof(uint32_t) && attr > 0xffffffff)
    {
#if defined(HAVE_ERRNO_H)
        errno = EINVAL;
#endif
        return -1;
    }

    if(x < 0 || x >= (int)cv->width || y < 0 || y >= (int)cv->height)
        return 0;

    curchar = cv->chars + x + y * cv->width;
    curattr = cv->attrs + x + y * cv->width;

    if(curattr[0] < 0x00000010)
        curattr[0] = (cv->curattr & 0xfffffff0) | curattr[0];

    if(x && curchar[0] == CUCUL_MAGIC_FULLWIDTH)
        curattr[-1] = curattr[0];
    else if(x + 1 < (int)cv->width && curchar[1] == CUCUL_MAGIC_FULLWIDTH)
        curattr[1] = curattr[0];

    return 0;
}

/** \brief Set the default colour pair for text (ANSI version).
 *
 *  Set the default ANSI colour pair for text drawing. String functions such
 *  as caca_printf() and graphical primitive functions such as caca_draw_line()
 *  will use these attributes.
 *
 *  Color values are those defined in cucul.h, such as CUCUL_RED
 *  or CUCUL_TRANSPARENT.
 *
 *  If an error occurs, 0 is returned and \b errno is set accordingly:
 *  - \c EINVAL At least one of the colour values is invalid.
 *
 *  \param cv A handle to the libcucul canvas.
 *  \param fg The requested ANSI foreground colour.
 *  \param bg The requested ANSI background colour.
 *  \return 0 in case of success, -1 if an error occurred.
 */
int cucul_set_color_ansi(cucul_canvas_t *cv, unsigned char fg, unsigned char bg)
{
    uint32_t attr;

    if(fg > 0x20 || bg > 0x20)
    {
#if defined(HAVE_ERRNO_H)
        errno = EINVAL;
#endif
        return -1;
    }

    attr = ((uint32_t)(bg | 0x40) << 18) | ((uint32_t)(fg | 0x40) << 4);
    cv->curattr = (cv->curattr & 0x0000000f) | attr;

    return 0;
}

/** \brief Set the default colour pair for text (truecolor version).
 *
 *  Set the default ARGB colour pair for text drawing. String functions such
 *  as caca_printf() and graphical primitive functions such as caca_draw_line()
 *  will use these attributes.
 *
 *  Colors are 16-bit ARGB values, each component being coded on 4 bits. For
 *  instance, 0xf088 is solid dark cyan (A=15 R=0 G=8 B=8), and 0x8fff is
 *  white with 50% alpha (A=8 R=15 G=15 B=15).
 *
 *  If an error occurs, 0 is returned and \b errno is set accordingly:
 *  - \c EINVAL At least one of the colour values is invalid.
 *
 *  \param cv A handle to the libcucul canvas.
 *  \param fg The requested ARGB foreground colour.
 *  \param bg The requested ARGB background colour.
 *  \return 0 in case of success, -1 if an error occurred.
 */
int cucul_set_color_argb(cucul_canvas_t *cv, unsigned int fg, unsigned int bg)
{
    uint32_t attr;

    if(fg > 0xffff || bg > 0xffff)
    {
#if defined(HAVE_ERRNO_H)
        errno = EINVAL;
#endif
        return -1;
    }

    if(fg < 0x100)
        fg += 0x100;

    if(bg < 0x100)
        bg += 0x100;

    fg = ((fg >> 1) & 0x7ff) | ((fg >> 13) << 11);
    bg = ((bg >> 1) & 0x7ff) | ((bg >> 13) << 11);

    attr = ((uint32_t)bg << 18) | ((uint32_t)fg << 4);
    cv->curattr = (cv->curattr & 0x0000000f) | attr;

    return 0;
}

/** \brief Get ANSI foreground information from attribute.
 *
 *  Get the ANSI foreground colour value for a given attribute. The returned
 *  value is either one of the \e CUCUL_RED, \e CUCUL_BLACK etc. predefined
 *  colours, or the special value \e CUCUL_DEFAULT meaning the media's
 *  default foreground value, or the special value \e CUCUL_TRANSPARENT.
 *
 *  If the attribute has ARGB colours, the nearest colour is returned.
 *
 *  This function never fails. If the attribute value is outside the expected
 *  32-bit range, higher order bits are simply ignored.
 *
 *  \param attr The requested attribute value.
 *  \return The corresponding ANSI foreground value.
 */
unsigned char cucul_attr_to_ansi_fg(unsigned long int attr)
{
    return nearest_ansi(((uint16_t)attr >> 4) & 0x3fff);
}

/** \brief Get ANSI background information from attribute.
 *
 *  Get the ANSI background colour value for a given attribute. The returned
 *  value is either one of the \e CUCUL_RED, \e CUCUL_BLACK etc. predefined
 *  colours, or the special value \e CUCUL_DEFAULT meaning the media's
 *  default background value, or the special value \e CUCUL_TRANSPARENT.
 *
 *  If the attribute has ARGB colours, the nearest colour is returned.
 *
 *  This function never fails. If the attribute value is outside the expected
 *  32-bit range, higher order bits are simply ignored.
 *
 *  \param attr The requested attribute value.
 *  \return The corresponding ANSI background value.
 */
unsigned char cucul_attr_to_ansi_bg(unsigned long int attr)
{
    return nearest_ansi(attr >> 18);
}

/*
 * XXX: the following functions are local
 */

/* RGB colours for the ANSI palette. There is no real standard, so we
 * use the same values as gnome-terminal. The 7th colour (brown) is a bit
 * special: 0xfa50 instead of 0xfaa0. */
static const uint16_t ansitab16[16] =
{
    0xf000, 0xf00a, 0xf0a0, 0xf0aa, 0xfa00, 0xfa0a, 0xfa50, 0xfaaa,
    0xf555, 0xf55f, 0xf5f5, 0xf5ff, 0xff55, 0xff5f, 0xfff5, 0xffff,
};

/* Same table, except on 14 bits (3-4-4-3) */
static const uint16_t ansitab14[16] =
{
    0x3800, 0x3805, 0x3850, 0x3855, 0x3d00, 0x3d05, 0x3d28, 0x3d55,
    0x3aaa, 0x3aaf, 0x3afa, 0x3aff, 0x3faa, 0x3faf, 0x3ffa, 0x3fff,
};

static uint8_t nearest_ansi(uint16_t argb14)
{
    unsigned int i, best, dist;

    if(argb14 < (0x10 | 0x40))
        return argb14 ^ 0x40;

    if(argb14 == (CUCUL_DEFAULT | 0x40) || argb14 == (CUCUL_TRANSPARENT | 0x40))
        return argb14 ^ 0x40;

    if(argb14 < 0x0fff) /* too transparent */
        return CUCUL_TRANSPARENT;

    best = CUCUL_DEFAULT;
    dist = 0x3fff;
    for(i = 0; i < 16; i++)
    {
        unsigned int d = 0;
        int a, b;

        a = (ansitab14[i] >> 7) & 0xf;
        b = (argb14 >> 7) & 0xf;
        d += (a - b) * (a - b);

        a = (ansitab14[i] >> 3) & 0xf;
        b = (argb14 >> 3) & 0xf;
        d += (a - b) * (a - b);

        a = (ansitab14[i] << 1) & 0xf;
        b = (argb14 << 1) & 0xf;
        d += (a - b) * (a - b);

        if(d < dist)
        {
            dist = d;
            best = i;
        }
    }

    return best;
}

uint8_t _cucul_attr_to_ansi8(uint32_t attr)
{
    uint8_t fg = nearest_ansi((attr >> 4) & 0x3fff);
    uint8_t bg = nearest_ansi(attr >> 18);

    if(fg == CUCUL_DEFAULT || fg == CUCUL_TRANSPARENT)
        fg = CUCUL_LIGHTGRAY;

    if(bg == CUCUL_DEFAULT || bg == CUCUL_TRANSPARENT)
        bg = CUCUL_BLACK;

    return fg | (bg << 4);
}

uint16_t _cucul_attr_to_rgb12fg(uint32_t attr)
{
    uint16_t fg = (attr >> 4) & 0x3fff;

    if(fg < (0x10 | 0x40))
        return ansitab16[fg ^ 0x40] & 0x0fff;

    if(fg == (CUCUL_DEFAULT | 0x40))
        return ansitab16[CUCUL_LIGHTGRAY] & 0x0fff;

    if(fg == (CUCUL_TRANSPARENT | 0x40))
        return ansitab16[CUCUL_LIGHTGRAY] & 0x0fff;

    return (fg << 1) & 0x0fff;
}

uint16_t _cucul_attr_to_rgb12bg(uint32_t attr)
{
    uint16_t bg = attr >> 18;

    if(bg < (0x10 | 0x40))
        return ansitab16[bg ^ 0x40] & 0x0fff;

    if(bg == (CUCUL_DEFAULT | 0x40))
        return ansitab16[CUCUL_BLACK] & 0x0fff;

    if(bg == (CUCUL_TRANSPARENT | 0x40))
        return ansitab16[CUCUL_BLACK] & 0x0fff;

    return (bg << 1) & 0x0fff;
}

#define RGB12TO24(i) \
   (((uint32_t)((i & 0xf00) >> 8) * 0x110000) \
  | ((uint32_t)((i & 0x0f0) >> 4) * 0x001100) \
  | ((uint32_t)(i & 0x00f) * 0x000011))

uint32_t _cucul_attr_to_rgb24fg(uint32_t attr)
{
    return RGB12TO24(_cucul_attr_to_rgb12fg(attr));
}

uint32_t _cucul_attr_to_rgb24bg(uint32_t attr)
{
    return RGB12TO24(_cucul_attr_to_rgb12bg(attr));
}

void _cucul_attr_to_argb4(uint32_t attr, uint8_t argb[8])
{
    uint16_t fg = (attr >> 4) & 0x3fff;
    uint16_t bg = attr >> 18;

    if(bg < (0x10 | 0x40))
        bg = ansitab16[bg ^ 0x40];
    else if(bg == (CUCUL_DEFAULT | 0x40))
        bg = ansitab16[CUCUL_BLACK];
    else if(bg == (CUCUL_TRANSPARENT | 0x40))
        bg = 0x0fff;
    else
        bg = ((bg << 2) & 0xf000) | ((bg << 1) & 0x0fff);

    argb[0] = bg >> 12;
    argb[1] = (bg >> 8) & 0xf;
    argb[2] = (bg >> 4) & 0xf;
    argb[3] = bg & 0xf;

    if(fg < (0x10 | 0x40))
        fg = ansitab16[fg ^ 0x40];
    else if(fg == (CUCUL_DEFAULT | 0x40))
        fg = ansitab16[CUCUL_LIGHTGRAY];
    else if(fg == (CUCUL_TRANSPARENT | 0x40))
        fg = 0x0fff;
    else
        fg = ((fg << 2) & 0xf000) | ((fg << 1) & 0x0fff);

    argb[4] = fg >> 12;
    argb[5] = (fg >> 8) & 0xf;
    argb[6] = (fg >> 4) & 0xf;
    argb[7] = fg & 0xf;
}

