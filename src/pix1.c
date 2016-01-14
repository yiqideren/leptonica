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
 *  pix1.c
 *
 *    The pixN.c {N = 1,2,3} files are sorted by the type of operation.
 *    The primary functions in these files are:
 *
 *        pix1.c: constructors, destructors and field accessors
 *        pix2.c: pixel poking of image, pad and border pixels
 *        pix3.c: logical and mask ops; counting; histograms
 *
 *
 *    This file has the basic constructors, destructors and field accessors
 *
 *      Pix creation
 *          PIX          *pixCreate()
 *          PIX          *pixCreateNoInit()
 *          PIX          *pixCreateTemplate()
 *          PIX          *pixCreateTemplateNoInit()
 *          PIX          *pixCreateHeader()
 *          PIX          *pixClone()
 *
 *      Pix destruction
 *          void          pixDestroy()
 *          void          pixFree()
 *
 *      Pix copy
 *          PIX          *pixCopy()
 *          l_int32       pixCopyColormap()
 *          l_int32       pixSizesEqual()
 *
 *      Pix accessors
 *          l_int32       pixGetWidth()
 *          l_int32       pixSetWidth()
 *          l_int32       pixGetHeight()
 *          l_int32       pixSetHeight()
 *          l_int32       pixGetDepth()
 *          l_int32       pixSetDepth()
 *          l_int32       pixGetDimensions()
 *          l_int32       pixGetWpl()
 *          l_int32       pixSetWpl()
 *          l_int32       pixGetRefcount()
 *          l_int32       pixChangeRefcount()
 *          l_uint32      pixGetXRes()
 *          l_uint32      pixGetYRes()
 *          l_int32       pixSetXRes()
 *          l_int32       pixSetYRes()
 *          l_int32       pixCopyResolution()
 *          l_int32       pixScaleResolution()
 *          l_int32       pixGetInputFormat()
 *          l_int32       pixSetInputFormat()
 *          l_int32       pixCopyInputFormat()
 *          char         *pixGetText()
 *          l_int32       pixSetText()
 *          l_int32       pixAddText()
 *          l_int32       pixCopyText()
 *          l_int32       pixDestroyColormap()
 *          PIXCMAP      *pixGetColormap()
 *          l_int32       pixSetColormap()
 *          l_uint32     *pixGetData()
 *          l_int32       pixSetData()
 *
 *      Pix debug
 *          l_int32       pixPrintStreamInfo()
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "allheaders.h"


/*--------------------------------------------------------------------*
 *                              Pix Creation                          *
 *--------------------------------------------------------------------*/
/*!
 *  pixCreate()
 *
 *      Input:  width, height, depth
 *      Return: pixd (with data allocated and initialized to 0),
 *                    or null on error
 */
PIX *
pixCreate(l_int32  width,
          l_int32  height,
          l_int32  depth)
{
PIX       *pixd;

    PROCNAME("pixCreate");

    if ((pixd = pixCreateNoInit(width, height, depth)) == NULL)
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    memset(pixd->data, 0, 4 * pixd->wpl * pixd->h);
    return pixd;
}


/*!
 *  pixCreateNoInit()
 *
 *      Input:  width, height, depth
 *      Return: pixd (with data allocated but not initialized),
 *                    or null on error
 */
PIX *
pixCreateNoInit(l_int32  width,
                l_int32  height,
                l_int32  depth)
{
l_int32    wpl;
PIX       *pixd;
l_uint32  *data;

    PROCNAME("pixCreateNoInit");
    pixd = pixCreateHeader(width, height, depth);
    if (!pixd) return NULL;
    wpl = pixGetWpl(pixd);
    if ((data = (l_uint32 *)MALLOC(4 * wpl * height)) == NULL)
        return (PIX *)ERROR_PTR("MALLOC fail for data", procName, NULL);
    pixSetData(pixd, data);
    return pixd;
}


/*!
 *  pixCreateTemplate()
 *
 *      Input:  pixs
 *      Return: pixd, or null on error
 *
 *  Notes:
 *      (1) Makes a Pix of the same size as the input Pix, with the
 *          data array allocated and initialized to 0.
 *      (2) Copies the other fields, including colormap if it exists.
 */
PIX *
pixCreateTemplate(PIX  *pixs)
{
PIX     *pixd;

    PROCNAME("pixCreateTemplate");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);

    if ((pixd = pixCreateTemplateNoInit(pixs)) == NULL)
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    memset(pixd->data, 0, 4 * pixd->wpl * pixd->h);
    return pixd;
}


/*!
 *  pixCreateTemplateNoInit()
 *
 *      Input:  pixs
 *      Return: pixd, or null on error
 *
 *  Notes:
 *      (1) Makes a Pix of the same size as the input Pix, with
 *          the data array allocated but not initialized to 0.
 *      (2) Copies the other fields, including colormap if it exists.
 */
PIX *
pixCreateTemplateNoInit(PIX  *pixs)
{
l_int32  w, h, d;
PIX     *pixd;

    PROCNAME("pixCreateTemplateNoInit");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);

    pixGetDimensions(pixs, &w, &h, &d);
    if ((pixd = pixCreateNoInit(w, h, d)) == NULL)
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    pixCopyResolution(pixd, pixs);
    pixCopyColormap(pixd, pixs);
    pixCopyText(pixd, pixs);
    pixCopyInputFormat(pixd, pixs);

    return pixd;
}


/*!
 *  pixCreateHeader()
 *
 *      Input:  width, height, depth
 *      Return: pixd (with no data allocated), or null on error
 */
PIX *
pixCreateHeader(l_int32  width,
                l_int32  height,
                l_int32  depth)
{
l_int32    wpl;
PIX       *pixd;

    PROCNAME("pixCreateHeader");

    if ((depth != 1) && (depth != 2) && (depth != 4) && (depth != 8)
         && (depth != 16) && (depth != 24) && (depth != 32))
        return (PIX *)ERROR_PTR("depth must be {1, 2, 4, 8, 16, 24, 32}",
                                procName, NULL);
    if (width <= 0)
        return (PIX *)ERROR_PTR("width must be > 0", procName, NULL);
    if (height <= 0)
        return (PIX *)ERROR_PTR("height must be > 0", procName, NULL);

    if ((pixd = (PIX *)CALLOC(1, sizeof(PIX))) == NULL)
        return (PIX *)ERROR_PTR("CALLOC fail for pixd", procName, NULL);
    pixSetWidth(pixd, width);
    pixSetHeight(pixd, height);
    pixSetDepth(pixd, depth);
    wpl = (width * depth + 31) / 32;
    pixSetWpl(pixd, wpl);

    pixd->refcount = 1;
    pixd->informat = IFF_UNKNOWN;

    return pixd;
}


/*!
 *  pixClone()
 *
 *      Input:  pix
 *      Return: same pix (ptr), or null on error
 *
 *  Note: Why is this here?  We make a "clone", which
 *        is just another handle to an existing pix, because
 *        (1) images can be large and hence expensive to copy,
 *        (2) extra handles to a data structure that are made
 *        without some control are dangerous because if you
 *        have two (say), how do you know which one should be
 *        used to destroy the pix, and, conversely, if you
 *        destroy the pix, how do you remember that the other
 *        handle is no longer valid?  This is solved by a
 *        simple protocol: (1) whenever you want a new handle
 *        to an existing image, call pixClone(), which just
 *        bumps a ref count, and (2) always call pixDestroy()
 *        on ALL handles, which decrements the ref count,
 *        nulls the handle, and only destroys the pix when
 *        pixDestroy() has been called on all handles.
 */
PIX *
pixClone(PIX  *pixs)
{
    PROCNAME("pixClone");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    pixChangeRefcount(pixs, 1);

    return pixs;
}


/*--------------------------------------------------------------------*
 *                           Pix Destruction                          *
 *--------------------------------------------------------------------*/
/*!
 *  pixDestroy()
 *
 *      Input:  &pix <will be nulled>
 *      Return: void
 *
 *  Notes:
 *      (1) Decrements the ref count and, if 0, destroys the pix.
 *      (2) Always nulls the input ptr.
 */
void
pixDestroy(PIX  **ppix)
{
PIX  *pix;

    PROCNAME("pixDestroy");

    if (!ppix) {
        L_WARNING("ptr address is null!", procName);
        return;
    }

    if ((pix = *ppix) == NULL)
        return;

    pixFree(pix);
    *ppix = NULL;
    return;
}


/*!
 *  pixFree()
 *
 *      Input:  pix
 *      Return: void
 *
 *  Notes:
 *      (1) Decrements the ref count and, if 0, destroys the pix.
 */
void
pixFree(PIX *pix)
{
l_uint32  *data;
char      *text;

    if (!pix) return;

        /* Decrement the ref count.  If it is 0, destroy the pix. */
    pixChangeRefcount(pix, -1);
    if (pixGetRefcount(pix) <= 0) {
        if ((data = pixGetData(pix)))
            FREE(data);
        if ((text = pixGetText(pix)))
            FREE(text);
        pixDestroyColormap(pix);
        FREE(pix);
    }
    return;
}


/*-------------------------------------------------------------------------*
 *                                 Pix Copy                                *
 *-------------------------------------------------------------------------*/
/*!
 *  pixCopy()
 *
 *      Input:  pixd (<optional>)
 *              pixs
 *      Return: pixd, or null on error
 *
 *  Notes:
 *      (1) If pixd = NULL, this makes a new copy, with refcount of 1.
 *          If pixd != NULL, this makes sure pixs and pixd are the same
 *          size, and then copies the image data, leaving the refcounts
 *          of pixs and pixd unchanged.
 *      (2) This operation, like all others that may involve a pre-existing
 *          pixd, will side-effect any existing clones of pixd.
 */
PIX *
pixCopy(PIX  *pixd,   /* can be null */
        PIX  *pixs)
{
l_int32    bytes;
l_uint32  *datas, *datad;

    PROCNAME("pixCopy");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, pixd);
    if (pixs == pixd) {
        L_WARNING("pix copied to itself", procName);
        return pixd;
    }

        /* Total bytes in image data */
    bytes = 4 * pixGetWpl(pixs) * pixGetHeight(pixs);

        /* If we're making a new pix ... */
    if (!pixd) {
        if ((pixd = pixCreateTemplate(pixs)) == NULL)
            return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
        datas = pixGetData(pixs);
        datad = pixGetData(pixd);
        memcpy((char *)datad, (char *)datas, bytes);
        return pixd;
    }

        /* Check sizes */
    if (!pixSizesEqual(pixs, pixd))
        return (PIX *)ERROR_PTR("pix sizes not equal", procName, pixd);
    pixCopyColormap(pixd, pixs);
    pixCopyResolution(pixd, pixs);
    pixCopyInputFormat(pixd, pixs);
    pixCopyText(pixd, pixs);

        /* Copy the data */
    datas = pixGetData(pixs);
    datad = pixGetData(pixd);
    memcpy((char*)datad, (char*)datas, bytes);

    return pixd;
}


/*!
 *  pixCopyColormap()
 *
 *      Input:  src and dest Pix
 *      Return: 0 if OK, 1 on error
 */
l_int32
pixCopyColormap(PIX  *pixd,
                PIX  *pixs)
{
PIXCMAP  *cmaps, *cmapd;

    PROCNAME("pixCopyColormap");

    if (!pixs)
        return ERROR_INT("pixs not defined", procName, 1);
    if (!pixd)
        return ERROR_INT("pixd not defined", procName, 1);

    if ((cmaps = pixGetColormap(pixs)) == NULL)  /* not an error */
        return 0;

    if ((cmapd = pixcmapCopy(cmaps)) == NULL)
        return ERROR_INT("cmapd not made", procName, 1);
    pixSetColormap(pixd, cmapd);

    return 0;
}


/*!
 *  pixSizesEqual()
 *
 *      Input:  two Pix
 *      Return: 1 if the two Pix have same {h, w, d}; 0 otherwise.
 */
l_int32
pixSizesEqual(PIX  *pix1,
              PIX  *pix2)
{
    PROCNAME("pixSizesEqual");

    if (!pix1)
        return ERROR_INT("pix1 not defined", procName, 0);
    if (!pix2)
        return ERROR_INT("pix2 not defined", procName, 0);

    if ((pixGetWidth(pix1) != pixGetWidth(pix2)) ||
        (pixGetHeight(pix1) != pixGetHeight(pix2)) ||
        (pixGetDepth(pix1) != pixGetDepth(pix2)))
        return 0;
    else
        return 1;
}



/*--------------------------------------------------------------------*
 *                                Accessors                           *
 *--------------------------------------------------------------------*/
l_int32
pixGetWidth(PIX  *pix)
{
    PROCNAME("pixGetWidth");

    if (!pix)
        return ERROR_INT("pix not defined", procName, UNDEF);

    return pix->w;
}


l_int32
pixSetWidth(PIX     *pix,
            l_int32  width)
{
    PROCNAME("pixSetWidth");

    if (!pix)
        return ERROR_INT("pix not defined", procName, 1);
    if (width < 0) {
        pix->w = 0;
        return ERROR_INT("width must be >= 0", procName, 1);
    }

    pix->w = width;
    return 0;
}


l_int32
pixGetHeight(PIX  *pix)
{
    PROCNAME("pixGetHeight");

    if (!pix)
        return ERROR_INT("pix not defined", procName, UNDEF);

    return pix->h;
}


l_int32
pixSetHeight(PIX     *pix,
             l_int32  height)
{
    PROCNAME("pixSetHeight");

    if (!pix)
        return ERROR_INT("pix not defined", procName, 1);
    if (height < 0) {
        pix->h = 0;
        return ERROR_INT("h must be >= 0", procName, 1);
    }

    pix->h = height;
    return 0;
}


l_int32
pixGetDepth(PIX  *pix)
{
    PROCNAME("pixGetDepth");

    if (!pix)
        return ERROR_INT("pix not defined", procName, UNDEF);

    return pix->d;
}


l_int32
pixSetDepth(PIX     *pix,
            l_int32  depth)
{
    PROCNAME("pixSetDepth");

    if (!pix)
        return ERROR_INT("pix not defined", procName, 1);
    if (depth < 1)
        return ERROR_INT("d must be >= 1", procName, 1);

    pix->d = depth;
    return 0;
}


/*!
 *  pixGetDimensions()
 *
 *      Input:  pix
 *              &w, &h, &d (<optional return>; each can be null)
 *      Return: 0 if OK, 1 on error
 */
l_int32
pixGetDimensions(PIX      *pix,
                 l_int32  *pw,
                 l_int32  *ph,
                 l_int32  *pd)
{
    PROCNAME("pixGetDimensions");

    if (!pix)
        return ERROR_INT("pix not defined", procName, 1);
    if (pw) *pw = pix->w;
    if (ph) *ph = pix->h;
    if (pd) *pd = pix->d;
    return 0;
}


l_int32
pixGetWpl(PIX  *pix)
{
    PROCNAME("pixGetWpl");

    if (!pix)
        return ERROR_INT("pix not defined", procName, UNDEF);
    return pix->wpl;
}


l_int32
pixSetWpl(PIX     *pix,
          l_int32  wpl)
{
    PROCNAME("pixSetWpl");

    if (!pix)
        return ERROR_INT("pix not defined", procName, 1);

    pix->wpl = wpl;
    return 0;
}


l_int32
pixGetRefcount(PIX  *pix)
{
    PROCNAME("pixGetRefcount");

    if (!pix)
        return ERROR_INT("pix not defined", procName, UNDEF);
    return pix->refcount;
}


l_int32
pixChangeRefcount(PIX     *pix,
                  l_int32  delta)
{
    PROCNAME("pixChangeRefcount");

    if (!pix)
        return ERROR_INT("pix not defined", procName, 1);

    pix->refcount += delta;
    return 0;
}


l_uint32
pixGetXRes(PIX  *pix)
{
    PROCNAME("pixGetXRes");

    if (!pix)
        return ERROR_INT("pix not defined", procName, 0);
    return pix->xres;
}


l_uint32
pixGetYRes(PIX  *pix)
{
    PROCNAME("pixGetYRes");

    if (!pix)
        return ERROR_INT("pix not defined", procName, 0);
    return pix->yres;
}


l_int32
pixSetXRes(PIX      *pix,
           l_uint32  res)
{
    PROCNAME("pixSetXRes");

    if (!pix)
        return ERROR_INT("pix not defined", procName, 1);

    pix->xres = res;
    return 0;
}


l_int32
pixCopyResolution(PIX  *pixd,
                  PIX  *pixs)
{
    PROCNAME("pixCopyResolution");

    if (!pixs)
        return ERROR_INT("pixs not defined", procName, 1);
    if (!pixd)
        return ERROR_INT("pixd not defined", procName, 1);

    pixSetXRes(pixd, pixGetXRes(pixs));
    pixSetYRes(pixd, pixGetYRes(pixs));
    return 0;
}


l_int32
pixScaleResolution(PIX       *pix,
                   l_float32  xscale,
                   l_float32  yscale)
{
    PROCNAME("pixScaleResolution");

    if (!pix)
        return ERROR_INT("pix not defined", procName, 1);

    if (pix->xres != 0 && pix->yres != 0) {
        pix->xres = (l_uint32)(xscale * (l_float32)(pix->xres) + 0.5);
        pix->yres = (l_uint32)(yscale * (l_float32)(pix->yres) + 0.5);
    }
    return 0;
}


l_int32
pixSetYRes(PIX      *pix,
           l_uint32  res)
{
    PROCNAME("pixSetYRes");

    if (!pix)
        return ERROR_INT("pix not defined", procName, 1);

    pix->yres = res;
    return 0;
}


l_int32
pixGetInputFormat(PIX  *pix)
{
    PROCNAME("pixGetInputFormat");

    if (!pix)
        return ERROR_INT("pix not defined", procName, UNDEF);
    return pix->informat;
}


l_int32
pixSetInputFormat(PIX     *pix,
                  l_int32  informat)
{
    PROCNAME("pixSetInputFormat");

    if (!pix)
        return ERROR_INT("pix not defined", procName, 1);

    pix->informat = informat;
    return 0;
}


l_int32
pixCopyInputFormat(PIX  *pixd,
                   PIX  *pixs)
{
    PROCNAME("pixCopyInputFormat");

    if (!pixs)
        return ERROR_INT("pixs not defined", procName, 1);
    if (!pixd)
        return ERROR_INT("pixd not defined", procName, 1);

    pixSetInputFormat(pixd, pixGetInputFormat(pixs));
    return 0;
}


/*!
 *  pixGetText()
 *
 *      Input:  pix
 *      Return: ptr to existing text string
 *
 *  Notes:
 *      (1) The text string belongs to the pix.  The caller must
 *          NOT free it!
 */
char *
pixGetText(PIX  *pix)
{
    PROCNAME("pixGetText");

    if (!pix)
        return (char *)ERROR_PTR("pix not defined", procName, NULL);
    return pix->text;
}


/*!
 *  pixSetText()
 *
 *      Input:  pix
 *              textstring
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This removes any existing textstring and puts a copy of
 *          the input textstring there.
 */
l_int32
pixSetText(PIX         *pix,
           const char  *textstring)
{
    PROCNAME("pixSetText");

    if (!pix)
        return ERROR_INT("pix not defined", procName, 1);

    stringReplace(&pix->text, textstring);
    return 0;
}


/*!
 *  pixAddText()
 *
 *      Input:  pix
 *              textstring
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This adds the new textstring to any existing text.
 *      (2) Either or both the existing text and the new text
 *          string can be null.
 */
l_int32
pixAddText(PIX         *pix,
           const char  *textstring)
{
char  *newstring;

    PROCNAME("pixAddText");

    if (!pix)
        return ERROR_INT("pix not defined", procName, 1);

    newstring = stringJoin(pixGetText(pix), textstring);
    stringReplace(&pix->text, newstring);
    FREE(newstring);
    return 0;
}


l_int32
pixCopyText(PIX  *pixd,
            PIX  *pixs)
{
    PROCNAME("pixCopyText");

    if (!pixs)
        return ERROR_INT("pixs not defined", procName, 1);
    if (!pixd)
        return ERROR_INT("pixd not defined", procName, 1);

    pixSetText(pixd, pixGetText(pixs));
    return 0;
}


/*!
 *  pixDestroyColormap()
 *
 *      Input:  pix
 *      Return: 0 if OK, 1 on error
 */
l_int32
pixDestroyColormap(PIX  *pix)
{
PIXCMAP  *cmap;

    PROCNAME("pixDestroyColormap");

    if (!pix)
        return ERROR_INT("pix not defined", procName, 1);

    if ((cmap = pix->colormap) == NULL)
        return 0;

    pixcmapDestroy(&cmap);
    pix->colormap = NULL;
    return 0;
}


PIXCMAP *
pixGetColormap(PIX  *pix)
{
    PROCNAME("pixGetColormap");

    if (!pix)
        return (PIXCMAP *)ERROR_PTR("pix not defined", procName, NULL);
    return pix->colormap;
}


l_int32
pixSetColormap(PIX      *pix,
               PIXCMAP  *colormap)
{
    PROCNAME("pixSetColormap");

    if (!pix)
        return ERROR_INT("pix not defined", procName, 1);

    pixDestroyColormap(pix);
    pix->colormap = colormap;
    return 0;
}


l_uint32 *
pixGetData(PIX  *pix)
{
    PROCNAME("pixGetData");

    if (!pix)
        return (l_uint32 *)ERROR_PTR("pix not defined", procName, NULL);
    return pix->data;
}


l_int32
pixSetData(PIX       *pix,
           l_uint32  *data)
{
    PROCNAME("pixSetData");

    if (!pix)
        return ERROR_INT("pix not defined", procName, 1);

    pix->data = data;
    return 0;
}


/*--------------------------------------------------------------------*
 *                    Print output for debugging                      *
 *--------------------------------------------------------------------*/
/*!
 *  pixPrintStreamInfo()
 *
 *      Input:  stream
 *              pix
 *              text (an identifying string that is printed)
 *      Return: 0 if OK, 1 on error
 */
l_int32
pixPrintStreamInfo(FILE        *fp,
                   PIX         *pix,
                   const char  *text)
{
PIXCMAP  *cmap;

    PROCNAME("pixPrintStreamInfo");

    if (!fp)
        return ERROR_INT("fp not defined", procName, 1);
    if (!pix)
        return ERROR_INT("pix not defined", procName, 1);
    if (!text)
        return ERROR_INT("text not defined", procName, 1);

    fprintf(fp, "  Pix Info for %s:\n", text);
    fprintf(fp, "    width = %d, height = %d, depth = %d\n",
               pixGetWidth(pix), pixGetHeight(pix), pixGetDepth(pix));
    fprintf(fp, "    wpl = %d, data = %p, refcount = %d\n",
               pixGetWpl(pix), pixGetData(pix), pixGetRefcount(pix));
    if ((cmap = pixGetColormap(pix)) != NULL)
        pixcmapWriteStream(fp, cmap);
    else
        fprintf(fp, "    no colormap\n");

    return 0;
}
