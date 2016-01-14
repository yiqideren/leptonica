/*====================================================================*
 -  Copyright (C) 2001 Leptonica.  All rights reserved.
 -  This software is distributed in the hope that it will be
 -  useful, but with NO WARRANTY OF ANY KIND.
 -  No author or distributor accepts responsibility to anyone for the
 -  consequences of using this software, or for whether it serves any
 -  particular purpose or works at all, unless he or she says so in
 -  writing.  Everyone is granted permission to copy, modify and
 -  redistribute this source code, for commercial or non-commercial
 -  purposes, with the following restrictions: (1) the origin of this
 -  source code must not be misrepresented; (2) modified versions must
 -  be plainly marked as such; and (3) this notice may not be removed
 -  or altered from any source or modified source distribution.
 *====================================================================*/

/*
 *  pix2.c
 *
 *    This file has these basic operations:
 *
 *      (1) Get and set of image pixels, pad pixels, border pixels,
 *          and color components for RGB
 *      (2) Endian byte swaps
 *
 *      Pixel poking
 *           l_int32     pixGetPixel()
 *           l_int32     pixSetPixel()
 *           l_int32     pixClearPixel()
 *           l_int32     pixFlipPixel()
 *           void        setPixelLow()
 *
 *      Full image clear/set/set-to-arbitrary-value
 *           l_int32     pixClearAll()
 *           l_int32     pixSetAll()
 *           l_int32     pixSetAllArbitrary()
 *           l_int32     pixSetPadBits()
 *           l_int32     pixSetPadBitsBand()
 *
 *      Set border pixels to arbitrary value
 *           l_int32     pixSetOrClearBorder()
 *           l_int32     pixSetBorderVal()
 *
 *      Add and remove border
 *           PIX        *pixAddBorder()
 *           PIX        *pixRemoveBorder()
 *           PIX        *pixAddBorderGeneral()
 *           PIX        *pixRemoveBorderGeneral()
 *
 *      Color sample setting and extraction
 *           PIX        *pixCreateRGBImage()
 *           PIX        *pixGetRGBComponent()
 *           l_int32     pixSetRGBComponent()
 *           l_int32     composeRGBPixel()
 *           l_int32     pixGetRGBLine()
 *
 *      Conversion between big and little endians
 *           PIX        *pixEndianByteSwapNew()
 *           l_int32     pixEndianByteSwap()
 *           l_int32     pixEndianTwoByteSwap()
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "allheaders.h"

static const l_uint32 rmask32[] = {0x0,
    0x00000001, 0x00000003, 0x00000007, 0x0000000f,
    0x0000001f, 0x0000003f, 0x0000007f, 0x000000ff,
    0x000001ff, 0x000003ff, 0x000007ff, 0x00000fff,
    0x00001fff, 0x00003fff, 0x00007fff, 0x0000ffff,
    0x0001ffff, 0x0003ffff, 0x0007ffff, 0x000fffff,
    0x001fffff, 0x003fffff, 0x007fffff, 0x00ffffff,
    0x01ffffff, 0x03ffffff, 0x07ffffff, 0x0fffffff,
    0x1fffffff, 0x3fffffff, 0x7fffffff, 0xffffffff};



/*-------------------------------------------------------------*
 *                         Pixel poking                        *
 *-------------------------------------------------------------*/
/*!
 *  pixGetPixel()
 *
 *      Input:  pix
 *              (x,y) pixel coords
 *              &val (<return> pixel value)
 *      Return: 0 if OK; 1 on error
 */
l_int32
pixGetPixel(PIX       *pix,
            l_int32    x,
            l_int32    y,
            l_uint32  *pval)
{
l_int32    w, h, d, wpl, val;
l_uint32  *line, *data;

    PROCNAME("pixGetPixel");

    if (!pix)
        return ERROR_INT("pix not defined", procName, 1);
    if (!pval)
        return ERROR_INT("pval not defined", procName, 1);
    *pval = 0;

    w = pixGetWidth(pix);
    h = pixGetHeight(pix);
    if (x < 0 || x >= w)
        return ERROR_INT("x out of bounds", procName, 1);
    if (y < 0 || y >= h)
        return ERROR_INT("y out of bounds", procName, 1);

    wpl = pixGetWpl(pix);
    data = pixGetData(pix);
    line = data + y * wpl;

    d = pixGetDepth(pix);
    switch (d)
    {
    case 1:
        val = GET_DATA_BIT(line, x);
        break;
    case 2:
        val = GET_DATA_DIBIT(line, x);
        break;
    case 4:
        val = GET_DATA_QBIT(line, x);
        break;
    case 8:
        val = GET_DATA_BYTE(line, x);
        break;
    case 16:
        val = GET_DATA_TWO_BYTES(line, x);
        break;
    case 32:
        val = line[x];
        break;
    default:
        return ERROR_INT("depth must be in {1,2,4,8,16,32} bpp", procName, 1);
    }

    *pval = val;
    return 0;
}


/*!
 *  pixSetPixel()
 *
 *      Input:  pix
 *              (x,y) pixel coords
 *              val (value to be inserted)
 *      Return: 0 if OK; 1 on error
 *
 *  Note: the input value is not checked for overflow, and
 *        the sign bit (if any) is ignored.
 */
l_int32
pixSetPixel(PIX      *pix,
            l_int32   x,
            l_int32   y,
            l_uint32  val)
{
l_int32    w, h, d, wpl;
l_uint32  *line, *data;

    PROCNAME("pixSetPixel");

    if (!pix)
        return ERROR_INT("pix not defined", procName, 1);

    pixGetDimensions(pix, &w, &h, NULL);
    if (x < 0 || x >= w)
        return ERROR_INT("x out of bounds", procName, 1);
    if (y < 0 || y >= h)
        return ERROR_INT("y out of bounds", procName, 1);

    data = pixGetData(pix);
    wpl = pixGetWpl(pix);
    line = data + y * wpl;

    d = pixGetDepth(pix);
    switch (d)
    {
    case 1:
        if (val)
            SET_DATA_BIT(line, x);
        else
            CLEAR_DATA_BIT(line, x);
        break;
    case 2:
        SET_DATA_DIBIT(line, x, val);
        break;
    case 4:
        SET_DATA_QBIT(line, x, val);
        break;
    case 8:
        SET_DATA_BYTE(line, x, val);
        break;
    case 16:
        SET_DATA_TWO_BYTES(line, x, val);
        break;
    case 32:
        line[x] = val;
        break;
    default:
        return ERROR_INT("depth must be in {1,2,4,8,16,32} bpp", procName, 1);
    }

    return 0;
}



/*!
 *  pixClearPixel()
 *
 *      Input:  pix
 *              (x,y) pixel coords
 *      Return: 0 if OK; 1 on error.
 */
l_int32
pixClearPixel(PIX     *pix,
              l_int32  x,
              l_int32  y)
{
l_int32    w, h, d, wpl;
l_uint32  *line, *data;

    PROCNAME("pixClearPixel");

    if (!pix)
        return ERROR_INT("pix not defined", procName, 1);

    pixGetDimensions(pix, &w, &h, NULL);
    if (x < 0 || x >= w)
        return ERROR_INT("x out of bounds", procName, 1);
    if (y < 0 || y >= h)
        return ERROR_INT("y out of bounds", procName, 1);

    wpl = pixGetWpl(pix);
    data = pixGetData(pix);
    line = data + y * wpl;

    d = pixGetDepth(pix);
    switch (d)
    {
    case 1:
        CLEAR_DATA_BIT(line, x);
        break;
    case 2:
        CLEAR_DATA_DIBIT(line, x);
        break;
    case 4:
        CLEAR_DATA_QBIT(line, x);
        break;
    case 8:
        SET_DATA_BYTE(line, x, 0);
        break;
    case 16:
        SET_DATA_TWO_BYTES(line, x, 0);
        break;
    case 32:
        line[x] = 0;
        break;
    default:
        return ERROR_INT("depth must be in {1,2,4,8,16,32} bpp", procName, 1);
    }

    return 0;
}


/*!
 *  pixFlipPixel()
 *
 *      Input:  pix
 *              (x,y) pixel coords
 *      Return: 0 if OK; 1 on error
 */
l_int32
pixFlipPixel(PIX     *pix,
             l_int32  x,
             l_int32  y)
{
l_int32    w, h, d, wpl;
l_uint32   val;
l_uint32  *line, *data;

    PROCNAME("pixFlipPixel");

    if (!pix)
        return ERROR_INT("pix not defined", procName, 1);

    pixGetDimensions(pix, &w, &h, NULL);
    if (x < 0 || x >= w)
        return ERROR_INT("x out of bounds", procName, 1);
    if (y < 0 || y >= h)
        return ERROR_INT("y out of bounds", procName, 1);

    data = pixGetData(pix);
    wpl = pixGetWpl(pix);
    line = data + y * wpl;

    d = pixGetDepth(pix);
    switch (d)
    {
    case 1:
        val = GET_DATA_BIT(line, x);
        if (val)
            CLEAR_DATA_BIT(line, x);
        else
            SET_DATA_BIT(line, x);
        break;
    case 2:
        val = GET_DATA_DIBIT(line, x);
        val ^= 0x3;
        SET_DATA_DIBIT(line, x, val);
        break;
    case 4:
        val = GET_DATA_QBIT(line, x);
        val ^= 0xf;
        SET_DATA_QBIT(line, x, val);
        break;
    case 8:
        val = GET_DATA_BYTE(line, x);
        val ^= 0xff;
        SET_DATA_BYTE(line, x, val);
        break;
    case 16:
        val = GET_DATA_TWO_BYTES(line, x);
        val ^= 0xffff;
        SET_DATA_TWO_BYTES(line, x, val);
        break;
    case 32:
        val = line[x] ^ 0xffffffff;
        line[x] = val;
        break;
    default:
        return ERROR_INT("depth must be in {1,2,4,8,16,32} bpp", procName, 1);
    }

    return 0;
}


/*!
 *  setPixelLow()
 *
 *      Input:  line (ptr to beginning of line),
 *              x (pixel location in line)
 *              depth (bpp)
 *              val (to be inserted)
 *      Return: void
 *
 *  Note: input variables are not checked
 */
void
setPixelLow(l_uint32  *line,
            l_int32    x,
            l_int32    depth,
            l_uint32   val)
{
    switch (depth)
    {
    case 1:
        if (val)
            SET_DATA_BIT(line, x);
        else
            CLEAR_DATA_BIT(line, x);
        break;
    case 2:
        SET_DATA_DIBIT(line, x, val);
        break;
    case 4:
        SET_DATA_QBIT(line, x, val);
        break;
    case 8:
        SET_DATA_BYTE(line, x, val);
        break;
    case 16:
        SET_DATA_TWO_BYTES(line, x, val);
        break;
    case 32:
        line[x] = val;
        break;
    default:
        fprintf(stderr, "illegal depth in setPixelLow()\n");
    }

    return;
}



/*-------------------------------------------------------------*
 *     Full image clear/set/set-to-arbitrary-value/invert      *
 *-------------------------------------------------------------*/
/*!
 *  pixClearAll()
 *
 *      Input:  pix
 *      Return: 0 if OK, 1 on error
 *
 *  Action: clears all data to 0
 */
l_int32
pixClearAll(PIX  *pix)
{
    PROCNAME("pixClearAll");

    if (!pix)
        return ERROR_INT("pix not defined", procName, 1);

    pixRasterop(pix, 0, 0, pixGetWidth(pix), pixGetHeight(pix),
                PIX_CLR, NULL, 0, 0);
    return 0;
}


/*!
 *  pixSetAll()
 *
 *      Input:  pix
 *      Return: 0 if OK, 1 on error
 *
 *  Action: sets all data to 1
 */
l_int32
pixSetAll(PIX  *pix)
{
    PROCNAME("pixSetAll");

    if (!pix)
        return ERROR_INT("pix not defined", procName, 1);

    pixRasterop(pix, 0, 0, pixGetWidth(pix), pixGetHeight(pix),
                PIX_SET, NULL, 0, 0);
    return 0;
}


/*!
 *  pixSetAllArbitrary()
 *
 *      Input:  pix
 *              val  (value to set all pixels)
 *      Return: 0 if OK; 1 on error
 */
l_int32
pixSetAllArbitrary(PIX      *pix,
                   l_uint32  val)
{
l_int32    i, j, w, h, d, wpl, npix;
l_uint32   maxval, wordval;
l_uint32  *data, *line;

    PROCNAME("pixSetAllArbitrary");

    if (!pix)
        return ERROR_INT("pix not defined", procName, 1);

    pixGetDimensions(pix, &w, &h, &d);
    if (d == 32)
        maxval = 0xffffffff;
    else
        maxval = (1 << d) - 1;
    if (val < 0) {
        L_WARNING("invalid pixel value; set to 0", procName);
        val = 0;
    }
    if (val > maxval) {
        L_WARNING_INT("invalid pixel val; set to maxval = %d",
                      procName, maxval);
        val = maxval;
    }

        /* Set up word to tile with */
    wordval = 0;
    npix = 32 / d;    /* number of pixels per 32 bit word */
    for (j = 0; j < npix; j++)
        wordval |= (val << (j * d));

    wpl = pixGetWpl(pix);
    data = pixGetData(pix);
    for (i = 0; i < h; i++) {
        line = data + i * wpl;
        for (j = 0; j < wpl; j++) {
            *(line + j) = wordval;
        }
    }

    return 0;
}


/*!
 *  pixSetPadBits()
 *
 *      Input:  pix (1, 2, 4, 8, 16, 32 bpp)
 *              val  (0 or 1)
 *      Return: 0 if OK; 1 on error
 *
 *  Notes:
 *      (1) The pad bits are the bits that expand each scanline to a
 *          multiple of 32 bits.  They are usually not used in
 *          image processing operations.  When boundary conditions
 *          are important, as in seedfill, they must be set properly.
 *      (2) This sets the value of the pad bits (if any) in the last
 *          32-bit word in each scanline.
 *      (3) For 32 bpp pix, there are no pad bits, so this is a no-op.
 */
l_int32
pixSetPadBits(PIX     *pix,
              l_int32  val)
{
l_int32    i, w, h, d, wpl, endbits, fullwords;
l_uint32   mask;
l_uint32  *data, *pword;

    PROCNAME("pixSetPadBits");

    if (!pix)
        return ERROR_INT("pix not defined", procName, 1);

    pixGetDimensions(pix, &w, &h, &d);
    if (d == 32)  /* no padding exists for 32 bpp */
        return 0;  

    data = pixGetData(pix);
    wpl = pixGetWpl(pix);
    endbits = 32 - ((w * d) % 32);
    if (endbits == 32)  /* no partial word */
        return 0;
    fullwords = w * d / 32;

    mask = rmask32[endbits];
    if (val == 0)
        mask = ~mask;

    for (i = 0; i < h; i++) {
        pword = data + i * wpl + fullwords;
        if (val == 0) /* clear */
            *pword = *pword & mask;
        else  /* set */
            *pword = *pword | mask;
    }

    return 0;
}


/*!
 *  pixSetPadBitsBand()
 *
 *      Input:  pix (1, 2, 4, 8, 16, 32 bpp)
 *              by  (starting y value of band)
 *              bh  (height of band)
 *              val  (0 or 1)
 *      Return: 0 if OK; 1 on error
 *
 *  Notes:
 *      (1) The pad bits are the bits that expand each scanline to a
 *          multiple of 32 bits.  They are usually not used in
 *          image processing operations.  When boundary conditions
 *          are important, as in seedfill, they must be set properly.
 *      (2) This sets the value of the pad bits (if any) in the last
 *          32-bit word in each scanline, within the specified
 *          band of raster lines.
 *      (3) For 32 bpp pix, there are no pad bits, so this is a no-op.
 */
l_int32
pixSetPadBitsBand(PIX     *pix,
                  l_int32  by,
                  l_int32  bh,
                  l_int32  val)
{
l_int32    i, w, h, d, wpl, endbits, fullwords;
l_uint32   mask;
l_uint32  *data, *pword;

    PROCNAME("pixSetPadBitsBand");

    if (!pix)
        return ERROR_INT("pix not defined", procName, 1);

    pixGetDimensions(pix, &w, &h, &d);
    if (d == 32)  /* no padding exists for 32 bpp */
        return 0;  

    if (by < 0)
        by = 0;
    if (by >= h)
        return ERROR_INT("start y not in image", procName, 1);
    if (by + bh > h)
        bh = h - by;

    data = pixGetData(pix);
    wpl = pixGetWpl(pix);
    endbits = 32 - ((w * d) % 32);
    if (endbits == 32)  /* no partial word */
        return 0;
    fullwords = w * d / 32;

    mask = rmask32[endbits];
    if (val == 0)
        mask = ~mask;

    for (i = by; i < by + bh; i++) {
        pword = data + i * wpl + fullwords;
        if (val == 0) /* clear */
            *pword = *pword & mask;
        else  /* set */
            *pword = *pword | mask;
    }

    return 0;
}


/*-------------------------------------------------------------*
 *                       Set border pixels                     *
 *-------------------------------------------------------------*/
/*!
 *  pixSetOrClearBorder()
 *
 *      Input:  pixs (all depths)
 *              leftpix, rightpix, toppix, bottompix
 *              operation (PIX_SET or PIX_CLR)
 *      Return: 0 if OK; 1 on error
 *
 *  Notes:
 *      (1) The border region is defined to be the region in the
 *          image within a specific distance of each edge.  Here, we
 *          allow the pixels within a specified distance of each
 *          edge to be set independently.  This either sets or
 *          clears all pixels in the border region.
 *      (2) For binary images, use PIX_SET for black and PIX_CLR for white.
 *      (3) For grayscale or color images, use PIX_SET for white
 *          and PIX_CLR for black.
 */
l_int32
pixSetOrClearBorder(PIX     *pixs,
                    l_int32  leftpix,
                    l_int32  rightpix,
                    l_int32  toppix,
                    l_int32  bottompix,
                    l_int32  op)
{
l_int32  w, h;

    PROCNAME("pixSetOrClearBorder");

    if (!pixs)
        return ERROR_INT("pixs not defined", procName, 1);
    if (op != PIX_SET && op != PIX_CLR)
        return ERROR_INT("op must be PIX_SET or PIX_CLR", procName, 1);

    pixGetDimensions(pixs, &w, &h, NULL);
    pixRasterop(pixs, 0, 0, leftpix, h, op, NULL, 0, 0);
    pixRasterop(pixs, w - rightpix, 0, rightpix, h, op, NULL, 0, 0);
    pixRasterop(pixs, 0, 0, w, toppix, op, NULL, 0, 0);
    pixRasterop(pixs, 0, h - bottompix, w, bottompix, op, NULL, 0, 0);

    return 0;
}


/*!
 *  pixSetBorderVal()
 *
 *      Input:  pixs (8 or 32 bpp)
 *              leftpix, rightpix, toppix, bottompix
 *              val (value to set at each border pixel)
 *      Return: 0 if OK; 1 on error
 *
 *  Notes:
 *      (1) The border region is defined to be the region in the
 *          image within a specific distance of each edge.  Here, we
 *          allow the pixels within a specified distance of each
 *          edge to be set independently.  This sets the pixels
 *          in the border region to the given input value.
 *      (2) For efficiency, use pixSetOrClearBorder() if
 *          you're setting the border to either black or white.
 *      (3) If d != 32, the input value should be masked off
 *          to the appropriate number of least significant bits.
 *      (4) The code is easily generalized for 2, 4 or 16 bpp.
 */
l_int32
pixSetBorderVal(PIX      *pixs,
                l_int32   leftpix,
                l_int32   rightpix,
                l_int32   toppix,
                l_int32   bottompix,
                l_uint32  val)
{
l_int32    w, h, d, wpls, i, j, bstart, rstart;
l_uint32  *datas, *lines;

    PROCNAME("pixSetBorderVal");

    if (!pixs)
        return ERROR_INT("pixs not defined", procName, 1);
    pixGetDimensions(pixs, &w, &h, &d);
    if (d != 8 && d != 32)
        return ERROR_INT("depth must be 8 or 32 bpp", procName, 1);

    datas = pixGetData(pixs);
    wpls = pixGetWpl(pixs);
    if (d == 8) {
        val &= 0xff;
        for (i = 0; i < toppix; i++) {
            lines = datas + i * wpls;
            for (j = 0; j < w; j++)
                SET_DATA_BYTE(lines, j, val);
        }
        rstart = w - rightpix;
        bstart = h - bottompix;
        for (i = toppix; i < bstart; i++) {
            lines = datas + i * wpls;
            for (j = 0; j < leftpix; j++)
                SET_DATA_BYTE(lines, j, val);
            for (j = rstart; j < w; j++)
                SET_DATA_BYTE(lines, j, val);
        }
        for (i = bstart; i < h; i++) {
            lines = datas + i * wpls;
            for (j = 0; j < w; j++)
                SET_DATA_BYTE(lines, j, val);
        }
    }
    else {   /* d == 32 */
        for (i = 0; i < toppix; i++) {
            lines = datas + i * wpls;
            for (j = 0; j < w; j++)
                *(lines + j) = val;
        }
        rstart = w - rightpix;
        bstart = h - bottompix;
        for (i = toppix; i < bstart; i++) {
            lines = datas + i * wpls;
            for (j = 0; j < leftpix; j++)
                *(lines + j) = val;
            for (j = rstart; j < w; j++)
                *(lines + j) = val;
        }
        for (i = bstart; i < h; i++) {
            lines = datas + i * wpls;
            for (j = 0; j < w; j++)
                *(lines + j) = val;
        }
    }

    return 0;
}
        

/*-------------------------------------------------------------*
 *                     Add and remove border                   *
 *-------------------------------------------------------------*/
/*!
 *  pixAddBorder()
 *
 *      Input:  pix
 *              npix (number of pixels to be added to each side)
 *              val  (value of added border pixels)
 *      Return: pix with the input pix centered, or null on error.
 *
 *  Notes:
 *      (1) For binary images:
 *             white:  val = 0
 *             black:  val = 1
 *      (2) For grayscale images:
 *             white:  val = 2 ** d - 1
 *             black:  val = 0
 *      (3) For rgb color images:
 *             white:  val = 0xffffff00
 *             black:  val = 0
 */
PIX *
pixAddBorder(PIX      *pixs,
             l_int32   npix,
             l_uint32  val)
{
l_int32  ws, hs, wd, hd, d;
PIX     *pixd;

    PROCNAME("pixAddBorder");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);

    if (npix == 0)
        return pixClone(pixs);

    pixGetDimensions(pixs, &ws, &hs, &d);
    wd = ws + 2 * npix;
    hd = hs + 2 * npix;
    if ((pixd = pixCreate(wd, hd, d)) == NULL)
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    pixCopyResolution(pixd, pixs);
    pixCopyColormap(pixd, pixs);

    pixSetAllArbitrary(pixd, val);   /* a little extra writing ! */

    pixRasterop(pixd, npix, npix, ws, hs, PIX_SRC, pixs, 0, 0);

    return pixd;
}


/*!
 *  pixRemoveBorder()
 *
 *      Input:  pixs
 *              npix (number to be removed from each of the 4 sides)
 *      Return: pixd, or null on error
 */
PIX *
pixRemoveBorder(PIX     *pixs,
                l_int32  npix)
{
l_int32  ws, hs, wd, hd, d;
PIX     *pixd;

    PROCNAME("pixRemoveBorder");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);

    if (npix == 0)
        return pixClone(pixs);

    pixGetDimensions(pixs, &ws, &hs, &d);
    wd = ws - 2 * npix;
    hd = hs - 2 * npix;
    if (wd <= 0)
        return (PIX *)ERROR_PTR("width must be > 0", procName, NULL);
    if (hd <= 0)
        return (PIX *)ERROR_PTR("height must be > 0", procName, NULL);
    if ((pixd = pixCreate(wd, hd, d)) == NULL)
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    pixCopyResolution(pixd, pixs);
    pixCopyColormap(pixd, pixs);

        /* Rasterop from the center */
    pixRasterop(pixd, 0, 0, wd, hd, PIX_SRC, pixs, npix, npix);
    
    return pixd;
}


/*!
 *  pixAddBorderGeneral()
 *
 *      Input:  pix
 *              leftpix, rightpix, toppix, bottompix  (number of pixels
 *                   to be added to each side)
 *              val   (value of added border pixels)
 *      Return: pix with the input pix placed properly, or null on error
 *
 *  Notes:
 *      (1) For binary images:
 *             white:  val = 0
 *             black:  val = 1
 *      (2) For grayscale images:
 *             white:  val = 2 ** d - 1
 *             black:  val = 0
 *      (3) For rgb color images:
 *             white:  val = 0xffffff00
 *             black:  val = 0
 */
PIX *
pixAddBorderGeneral(PIX      *pixs,
                    l_int32   leftpix,
                    l_int32   rightpix,
                    l_int32   toppix,
                    l_int32   bottompix,
                    l_uint32  val)
{
l_int32  ws, hs, wd, hd, d;
PIX     *pixd;

    PROCNAME("pixAddBorderGeneral");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);

    pixGetDimensions(pixs, &ws, &hs, &d);
    wd = ws + leftpix + rightpix;
    hd = hs + toppix + bottompix;
    if ((pixd = pixCreate(wd, hd, d)) == NULL)
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    pixCopyResolution(pixd, pixs);
    pixCopyColormap(pixd, pixs);

    pixSetAllArbitrary(pixd, val);   /* a little extra writing ! */

    pixRasterop(pixd, leftpix, toppix, ws, hs, PIX_SRC, pixs, 0, 0);

    return pixd;
}


/*!
 *  pixRemoveBorderGeneral()
 *
 *      Input:  pixs
 *              leftpix, rightpix, toppix, bottompix  (number of pixels
 *                   to be removed from each side)
 *      Return: pixd (with pixels removed around border), or null on error
 */
PIX *
pixRemoveBorderGeneral(PIX     *pixs,
                       l_int32  leftpix,
                       l_int32  rightpix,
                       l_int32  toppix,
                       l_int32  bottompix)
{
l_int32  ws, hs, wd, hd, d;
PIX     *pixd;

    PROCNAME("pixRemoveBorderGeneral");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);

    pixGetDimensions(pixs, &ws, &hs, &d);
    wd = ws - leftpix - rightpix;
    hd = hs - toppix - bottompix;
    if (wd <= 0)
        return (PIX *)ERROR_PTR("width must be > 0", procName, NULL);
    if (hd <= 0)
        return (PIX *)ERROR_PTR("height must be > 0", procName, NULL);
    if ((pixd = pixCreate(wd, hd, d)) == NULL)
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    pixCopyResolution(pixd, pixs);
    pixCopyColormap(pixd, pixs);

    pixRasterop(pixd, 0, 0, wd, hd, PIX_SRC, pixs, leftpix, toppix);
    
    return pixd;
}


/*-------------------------------------------------------------*
 *                Color sample setting and extraction          *
 *-------------------------------------------------------------*/
/*!
 *  pixCreateRGBImage()
 *
 *      Input:  8 bpp red pix
 *              8 bpp green pix
 *              8 bpp blue pix
 *      Return: 32 bpp pix, interleaved with 4 samples/pixel,
 *              or null on error
 *
 *  Notes:
 *      (1) the 4th byte, sometimes called the "alpha channel",
 *          and which is often used for blending between different
 *          images, is left with 0 value.
 *      (2) see Note (4) in pix.h for details on storage of
 *          8-bit samples within each 32-bit word.
 */
PIX *
pixCreateRGBImage(PIX  *pixr,
                  PIX  *pixg,
                  PIX  *pixb)
{
l_int32  w, h;
PIX     *pixd;

    PROCNAME("pixCreateRGBImage");

    if (!pixr)
        return (PIX *)ERROR_PTR("pixr not defined", procName, NULL);
    if (!pixg)
        return (PIX *)ERROR_PTR("pixg not defined", procName, NULL);
    if (!pixb)
        return (PIX *)ERROR_PTR("pixb not defined", procName, NULL);
    if (pixGetDepth(pixr) != 8)
        return (PIX *)ERROR_PTR("pixr not 8 bpp", procName, NULL);
    if (pixGetDepth(pixg) != 8)
        return (PIX *)ERROR_PTR("pixg not 8 bpp", procName, NULL);
    if (pixGetDepth(pixb) != 8)
        return (PIX *)ERROR_PTR("pixb not 8 bpp", procName, NULL);
 
    pixGetDimensions(pixr, &w, &h, NULL);
    if (w != pixGetWidth(pixg) || w != pixGetWidth(pixb))
        return (PIX *)ERROR_PTR("widths not the same", procName, NULL);
    if (h != pixGetHeight(pixg) || h != pixGetHeight(pixb))
        return (PIX *)ERROR_PTR("heights not the same", procName, NULL);

    if ((pixd = pixCreate(w, h, 32)) == NULL)
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    pixCopyResolution(pixd, pixr);
    pixSetRGBComponent(pixd, pixr, COLOR_RED);
    pixSetRGBComponent(pixd, pixg, COLOR_GREEN);
    pixSetRGBComponent(pixd, pixb, COLOR_BLUE);

    return pixd;
}


/*!
 *  pixGetRGBComponent()
 *
 *      Input:  pixs  (32 bpp)
 *              color  (one of {COLOR_RED, COLOR_GREEN, COLOR_BLUE,
 *                      L_ALPHA_CHANNEL})
 *      Return: pixd, the selected 8 bpp component image of the
 *              input 32 bpp image, or null on error
 *
 *  Notes:
 *      (1) The alpha channel (in the 4th byte of each RGB pixel)
 *          is not used in leptonica.
 */
PIX *
pixGetRGBComponent(PIX     *pixs,
                   l_int32  color)
{
l_uint8    srcbyte;
l_uint32  *lines, *lined;
l_uint32  *datas, *datad;
l_int32    i, j, w, h;
l_int32    wpls, wpld;
PIX           *pixd;

    PROCNAME("pixGetRGBComponent");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (pixGetDepth(pixs) != 32)
        return (PIX *)ERROR_PTR("pixs not 32 bpp", procName, NULL);
    if (color != COLOR_RED && color != COLOR_GREEN &&
        color != COLOR_BLUE && color != L_ALPHA_CHANNEL)
        return (PIX *)ERROR_PTR("invalid color", procName, NULL);

    pixGetDimensions(pixs, &w, &h, NULL);
    if ((pixd = pixCreate(w, h, 8)) == NULL)
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    pixCopyResolution(pixd, pixs);
    wpls = pixGetWpl(pixs);
    wpld = pixGetWpl(pixd);
    datas = pixGetData(pixs);
    datad = pixGetData(pixd);

    for (i = 0; i < h; i++) {
        lines = datas + i * wpls;
        lined = datad + i * wpld;
        for (j = 0; j < w; j++) {
            srcbyte = GET_DATA_BYTE(lines + j, color);
            SET_DATA_BYTE(lined, j, srcbyte);
        }
    }

    return pixd;
}


/*!
 *  pixSetRGBComponent()
 *
 *      Input:  pixd  (32 bpp)
 *              pixs  (8 bpp)
 *              color  (one of {COLOR_RED, COLOR_GREEN, COLOR_BLUE,
 *                      L_ALPHA_CHANNEL})
 *      Return: 0 if OK; 1 on error
 *
 *  Notes:
 *      (1) This places the 8 bpp pixel in pixs into the
 *          specified color component (properly interleaved) in pixd.
 *      (2) The alpha channel component is not used in leptonica.
 */
l_int32
pixSetRGBComponent(PIX     *pixd,
                   PIX     *pixs,
                   l_int32  color)
{
l_uint8    srcbyte;
l_int32    i, j, w, h;
l_int32    wpls, wpld;
l_uint32  *lines, *lined;
l_uint32  *datas, *datad;

    PROCNAME("pixSetRGBComponent");

    if (!pixd)
        return ERROR_INT("pixd not defined", procName, 1);
    if (!pixs)
        return ERROR_INT("pixs not defined", procName, 1);

    if (pixGetDepth(pixd) != 32)
        return ERROR_INT("pixd not 32 bpp", procName, 1);
    if (pixGetDepth(pixs) != 8)
        return ERROR_INT("pixs not 8 bpp", procName, 1);
    if (color != COLOR_RED && color != COLOR_GREEN &&
        color != COLOR_BLUE && color != L_ALPHA_CHANNEL)
        return ERROR_INT("invalid color", procName, 1);
    pixGetDimensions(pixs, &w, &h, NULL);
    if (w != pixGetWidth(pixd) || h != pixGetHeight(pixd))
        return ERROR_INT("sizes not commensurate", procName, 1);

    datas = pixGetData(pixs);
    datad = pixGetData(pixd);
    wpls = pixGetWpl(pixs);
    wpld = pixGetWpl(pixd);
    for (i = 0; i < h; i++) {
        lines = datas + i * wpls;
        lined = datad + i * wpld;
        for (j = 0; j < w; j++) {
            srcbyte = GET_DATA_BYTE(lines, j);
            SET_DATA_BYTE(lined + j, color, srcbyte);
        }
    }

    return 0;
}


/*!
 *  composeRGBPixel()
 *
 *      Input:  rval, gval, bval
 *              &rgbpixel  (<return> 32-bit pixel)
 *      Return: 0 if OK; 1 on error
 */
l_int32
composeRGBPixel(l_int32    rval,
                l_int32    gval,
                l_int32    bval,
                l_uint32  *ppixel)
{
    PROCNAME("composeRGBPixel");

    if (!ppixel)
        return ERROR_INT("&pixel not defined", procName, 1);

    *ppixel = 0;  /* want the alpha byte to be 0 */
    SET_DATA_BYTE(ppixel, COLOR_RED, rval);
    SET_DATA_BYTE(ppixel, COLOR_GREEN, gval);
    SET_DATA_BYTE(ppixel, COLOR_BLUE, bval);
    return 0;
}


/*!
 *  pixGetRGBLine()
 *
 *      Input:  pixs  (32 bpp)
 *              row
 *              bufr  (array of red samples; size w bytes)
 *              bufg  (array of green samples; size w bytes)
 *              bufb  (array of blue samples; size w bytes)
 *      Return: 0 if OK; 1 on error
 *
 *  Notes:
 *      (1) This puts rgb components from the input line in pixs
 *          into the given buffers.
 */
l_int32
pixGetRGBLine(PIX      *pixs,
              l_int32   row,
              l_uint8  *bufr,
              l_uint8  *bufg,
              l_uint8  *bufb)
{
l_uint32  *lines;
l_int32    j, w, h;
l_int32    wpls;

    PROCNAME("pixGetRGBLine");

    if (!pixs)
        return ERROR_INT("pixs not defined", procName, 1);
    if (pixGetDepth(pixs) != 32)
        return ERROR_INT("pixs not 32 bpp", procName, 1);
    if (!bufr || !bufg || !bufb)
        return ERROR_INT("buffer not defined", procName, 1);

    pixGetDimensions(pixs, &w, &h, NULL);
    if (row < 0 || row >= h)
        return ERROR_INT("row out of bounds", procName, 1);
    wpls = pixGetWpl(pixs);
    lines = pixGetData(pixs) + row * wpls;

    for (j = 0; j < w; j++) {
        bufr[j] = GET_DATA_BYTE(lines + j, COLOR_RED);
        bufg[j] = GET_DATA_BYTE(lines + j, COLOR_GREEN);
        bufb[j] = GET_DATA_BYTE(lines + j, COLOR_BLUE);
    }

    return 0;
}


/*-------------------------------------------------------------*
 *                    Pixel endian conversion                  *
 *-------------------------------------------------------------*/
/*!
 *  pixEndianByteSwapNew()
 *
 *      Input:  pixs
 *      Return: pixd, or null on error
 *
 *  Notes:
 *      (1) This is used to convert the data in a pix to a
 *          serialized byte buffer in raster order, and, for RGB,
 *          in order RGBA.  This requires flipping bytes within
 *          each 32-bit word for little-endian platforms, because the
 *          words have a MSB-to-the-left rule, whereas byte raster-order
 *          requires the left-most byte in each word to be byte 0.
 *          For big-endians, no swap is necessary, so this returns a clone.
 *      (2) Unlike pixEndianByteSwap(), which swaps the bytes in-place,
 *          this returns a new pix (or a clone).  We provide this
 *          because often when serialization is done, the source
 *          pix needs to be restored to canonical little-endian order,
 *          and this requires a second byte swap.  In such a situation,
 *          it is twice as fast to make a new pix in big-endian order,
 *          use it, and destroy it.
 */
PIX *
pixEndianByteSwapNew(PIX  *pixs)
{
l_uint32  *datas, *datad;
l_int32    i, j, h, wpl;
l_uint32   word;
PIX       *pixd;

    PROCNAME("pixEndianByteSwapNew");
        
#ifdef L_BIG_ENDIAN

    return pixClone(pixs);

#else   /* L_LITTLE_ENDIAN */

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);

    datas = pixGetData(pixs);
    wpl = pixGetWpl(pixs);
    h = pixGetHeight(pixs);
    pixd = pixCreateTemplate(pixs);
    datad = pixGetData(pixd);
    for (i = 0; i < h; i++) {
        for (j = 0; j < wpl; j++, datas++, datad++) {
            word = *datas;
            *datad = (word >> 24) |
                    ((word >> 8) & 0x0000ff00) |
                    ((word << 8) & 0x00ff0000) |
                    (word << 24);
        }
    }

    return pixd;

#endif   /* L_BIG_ENDIAN */

}


/*!
 *  pixEndianByteSwap()
 *
 *      Input:  pix
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This is used on little-endian platforms to swap
 *          the bytes within a word; bytes 0 and 3 are swapped,
 *          and bytes 1 and 2 are swapped.
 *      (2) This is required for little-endians in situations
 *          where we convert from a serialized byte order that is
 *          in raster order, as one typically has in file formats,
 *          to one with MSB-to-the-left in each 32-bit word, or v.v.
 *          See pix.h for a description of the canonical format
 *          (MSB-to-the left) that is used for both little-endian
 *          and big-endian platforms.   For big-endians, the
 *          MSB-to-the-left word order has the bytes in raster
 *          order when serialized, so no byte flipping is required.
 */
l_int32
pixEndianByteSwap(PIX  *pix)
{
l_uint32  *data;
l_int32    i, j, h, wpl;
l_uint32   word;

    PROCNAME("pixEndianByteSwap");
        
#ifdef L_BIG_ENDIAN

    return 0;

#else   /* L_LITTLE_ENDIAN */

    if (!pix)
        return ERROR_INT("pix not defined", procName, 1);

    data = pixGetData(pix);
    wpl = pixGetWpl(pix);
    h = pixGetHeight(pix);
    for (i = 0; i < h; i++) {
        for (j = 0; j < wpl; j++, data++) {
            word = *data;
            *data = (word >> 24) |
                    ((word >> 8) & 0x0000ff00) |
                    ((word << 8) & 0x00ff0000) |
                    (word << 24);
        }
    }

    return 0;

#endif   /* L_BIG_ENDIAN */

}


/*!
 *  pixEndianTwoByteSwap()
 *
 *      Input:  pix
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This is used on little-endian platforms to swap the
 *          2-byte entities within a 32-bit word.
 *      (2) This is equivalent to a full byte swap, as performed
 *          by pixEndianByteSwap(), followed by byte swaps in
 *          each of the 16-bit entities separately.
 */
l_int32
pixEndianTwoByteSwap(PIX  *pix)
{
l_uint32  *data;
l_int32    i, j, h, wpl;
l_uint32   word;

    PROCNAME("pixEndianTwoByteSwap");
        
#ifdef L_BIG_ENDIAN

    return 0;

#else   /* L_LITTLE_ENDIAN */

    if (!pix)
        return ERROR_INT("pix not defined", procName, 1);

    data = pixGetData(pix);
    wpl = pixGetWpl(pix);
    h = pixGetHeight(pix);
    for (i = 0; i < h; i++) {
        for (j = 0; j < wpl; j++, data++) {
            word = *data;
            *data = ((word << 16) & 0xffff0000) |
                    ((word >> 16) & 0x0000ffff);
        }
    }

    return 0;

#endif   /* L_BIG_ENDIAN */

}

