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
 *  fmorphauto.c
 *      
 *    Main call:
 *       l_int32             fmorphautogen()
 *
 *    Static helpers:
 *       static SARRAY      *sarrayMakeWplsCode()
 *       static SARRAY      *sarrayMakeInnerLoopDWACode()
 *       static char        *makeBarrelshiftString()
 *
 *
 *    This automatically generates dwa code for erosion and dilation.
 *    Here's a road map for how it all works.
 *
 *    (1) You generate an array (a SELA) of structuring elements (SELs).
 *        This can be done in several ways, including
 *           (a) calling the function selaAddBasic() for
 *               pre-compiled SELs
 *           (b) generating the SELA in code in line
 *           (c) reading in a SELA from file using selaRead()
 *
 *    (2) You call fmorphautogen() on this SELA.  This uses the
 *        text files morphtemplate1.txt and morphtemplate2.txt for
 *        building up the source code.  See the file
 *        prog/fmorphautogen.c for an example of how this is done.
 *        The output is written to files named fmorphgen.*.c
 *        and fmorphgenlow.*.c, where "*" is an integer that you
 *        input to this function.  That integer labels both
 *        the output files, as well as all the functions that
 *        are generated.  That way, using different integers,
 *        you can invoke fmorphautogen() any number of times
 *        to get functions that all have different names so that
 *        they can be linked into one program.
 *        
 *    (3) You copy the generated source code back to your src
 *        directory for compilation.  Put their names in the
 *        Makefile and recompile the libraries.  Check the Makefile
 *        to see in which libraries I have placed the example
 *        ones (which are named fmorphgen.1.c and fmorphgenlow.1.c).
 *
 *    (4) You make the library again, compiling in the code.
 *        For the example one I made, using the integer "1",
 *        you have a high-level interface in fmorphgen.1.c to the
 *        dwa erosion and dilation, using any of the SELs given there.
 *
 *    (5) In an application, you now use this interface.  Again
 *        for the example files generated, using integer "1":
 *
 *         PIX   *pixFMorphopGen_1(PIX *pixd, PIX *pixs,
 *                                 l_int32 operation, char *selname);
 *
 *        The operation is either L_MORPH_DILATE or L_MORPH_ERODE.
 *        The selname is one of the set that were defined
 *        as the name field of sels.  This set is listed at the
 *        beginning of the file fmorphgen.1.c.
 *        As an example, see the file prog/fmorphtest2.c, which
 *        verifies the correctness of the implementation by
 *        comparing the dwa result with that of full-image
 *        rasterops. 
 *
 */

#include <stdio.h>
#include <stdlib.h>

#include "allheaders.h"

#define   OUTROOT         "fmorphgen"
#define   OUTROOTLOW      "fmorphgenlow"

#define   TEMPLATE1       "morphtemplate1.txt"
#define   TEMPLATE2       "morphtemplate2.txt"

#define   NSTART1         0
#define   NSTOP1          23
#define   NSTART2         31
#define   NSTOP2          42
#define   NSTART3         44
#define   NSTOP3          96
#define   NSTART4         98
#define   NSTOP4          100 
#define   NSTART5         102
#define   NSTOP5          106

#define   NSTART6         0
#define   NSTOP6          28
#define   NSTART7         32
#define   NSTOP7          42
#define   NSTART8         44
#define   NSTOP8          53
#define   NSTART9         57
#define   NSTOP9          74
#define   NSTART10        77 
#define   NSTOP10         85
#define   NSTART11        89
#define   NSTOP11         95
#define   NSTART12        99
#define   NSTOP12         103


#define   BUFFER_SIZE     512

#define   PROTOARGS   "(l_uint32 *, l_int32, l_int32, l_int32, l_uint32 *, l_int32);"

static char * makeBarrelshiftString(l_int32 delx, l_int32 dely);
static SARRAY * sarrayMakeInnerLoopDWACode(SEL *sel, l_int32 index);
static SARRAY * sarrayMakeWplsCode(SEL *sel);

static  char   *wpldecls[] = {
            "l_int32              wpls2;",
            "l_int32              wpls2, wpls3;",
            "l_int32              wpls2, wpls3, wpls4;",
            "l_int32              wpls5;",
            "l_int32              wpls5, wpls6;",
            "l_int32              wpls5, wpls6, wpls7;",
            "l_int32              wpls5, wpls6, wpls7, wpls8;",
            "l_int32              wpls9;",
            "l_int32              wpls9, wpls10;",
            "l_int32              wpls9, wpls10, wpls11;",
            "l_int32              wpls9, wpls10, wpls11, wpls12;",
            "l_int32              wpls13;",
            "l_int32              wpls13, wpls14;",
            "l_int32              wpls13, wpls14, wpls15;",
            "l_int32              wpls13, wpls14, wpls15, wpls16;",
            "l_int32              wpls17;",
            "l_int32              wpls17, wpls18;",
            "l_int32              wpls17, wpls18, wpls19;",
            "l_int32              wpls17, wpls18, wpls19, wpls20;",
            "l_int32              wpls21;",
            "l_int32              wpls21, wpls22;",
            "l_int32              wpls21, wpls22, wpls23;",
            "l_int32              wpls21, wpls22, wpls23, wpls24;",
            "l_int32              wpls25;",
            "l_int32              wpls25, wpls26;",
            "l_int32              wpls25, wpls26, wpls27;",
            "l_int32              wpls25, wpls26, wpls27, wpls28;",
            "l_int32              wpls29;",
            "l_int32              wpls29, wpls30;",
            "l_int32              wpls29, wpls30, wpls31;"};

static  char   *wpldefs[] = {
            "    wpls2 = 2 * wpls;",
            "    wpls3 = 3 * wpls;",
            "    wpls4 = 4 * wpls;",
            "    wpls5 = 5 * wpls;",
            "    wpls6 = 6 * wpls;",
            "    wpls7 = 7 * wpls;",
            "    wpls8 = 8 * wpls;",
            "    wpls9 = 9 * wpls;",
            "    wpls10 = 10 * wpls;",
            "    wpls11 = 11 * wpls;",
            "    wpls12 = 12 * wpls;",
            "    wpls13 = 13 * wpls;",
            "    wpls14 = 14 * wpls;",
            "    wpls15 = 15 * wpls;",
            "    wpls16 = 16 * wpls;",
            "    wpls17 = 17 * wpls;",
            "    wpls18 = 18 * wpls;",
            "    wpls19 = 19 * wpls;",
            "    wpls20 = 20 * wpls;",
            "    wpls21 = 21 * wpls;",
            "    wpls22 = 22 * wpls;",
            "    wpls23 = 23 * wpls;",
            "    wpls24 = 24 * wpls;",
            "    wpls25 = 25 * wpls;",
            "    wpls26 = 26 * wpls;",
            "    wpls27 = 27 * wpls;",
            "    wpls28 = 28 * wpls;",
            "    wpls29 = 29 * wpls;",
            "    wpls30 = 30 * wpls;",
            "    wpls31 = 31 * wpls;"};

static char   *wplstrp[] = {"+ wpls", "+ wpls2", "+ wpls3", "+ wpls4",
                            "+ wpls5", "+ wpls6", "+ wpls7", "+ wpls8",
                            "+ wpls9", "+ wpls10", "+ wpls11", "+ wpls12",
                            "+ wpls13", "+ wpls14", "+ wpls15", "+ wpls16",
                            "+ wpls17", "+ wpls18", "+ wpls19", "+ wpls20",
                            "+ wpls21", "+ wpls22", "+ wpls23", "+ wpls24",
                            "+ wpls25", "+ wpls26", "+ wpls27", "+ wpls28",
                            "+ wpls29", "+ wpls30", "+ wpls31"};

static char   *wplstrm[] = {"- wpls", "- wpls2", "- wpls3", "- wpls4",
                            "- wpls5", "- wpls6", "- wpls7", "- wpls8",
                            "- wpls9", "- wpls10", "- wpls11", "- wpls12",
                            "- wpls13", "- wpls14", "- wpls15", "- wpls16",
                            "- wpls17", "- wpls18", "- wpls19", "- wpls20",
                            "- wpls21", "- wpls22", "- wpls23", "- wpls24",
                            "- wpls25", "- wpls26", "- wpls27", "- wpls28",
                            "- wpls29", "- wpls30", "- wpls31"};


/*!
 *  fmorphautogen()
 *
 *      Input:  sel array
 *              fileindex
 *      Return: 0 if OK; 1 on error
 *
 *  Action: this function writes two C source files to carry out
 *          dilation and erosion by the fast dwa method, using all
 *          sels in the input array.  The output filenames are
 *          composed using the fileindex.
 */
l_int32
fmorphautogen(SELA    *sela,
              l_int32  fileindex)
{
char    *sname, *filestr, *linestr, *fname;
char    *toplevelcall, *lowlevelcall1, *lowlevelcall2;
char    *lowleveldefine;
char     bigbuf[BUFFER_SIZE];
l_int32  i, j, nsels, nbytes;
SARRAY  *sa1, *sa2, *sa3, *sa4, *sa5, *sa6;
SEL     *sel;

    PROCNAME("fmorphautogen");

    if (!sela)
        return ERROR_INT("sela not defined", procName, 1);
    if (fileindex < 0)
        fileindex = 0;
    if ((nsels = selaGetCount(sela)) == 0)
        return ERROR_INT("no sels in sela", procName, 1);
    
    /* --------------------------------------------------------------*
     *                    generate data for first file               *
     * --------------------------------------------------------------*/

        /* make array of sel names */
    if ((sa1 = sarrayCreate(nsels)) == NULL)
        return ERROR_INT("sa1 not made", procName, 1);
    for (i = 0; i < nsels; i++) {
        if ((sel = selaGetSel(sela, i)) == NULL)
            return ERROR_INT("sel not returned", procName, 1);
        sname = selGetName(sel);
        sarrayAddString(sa1, sname, 1);
    }

/*    sarrayWriteStream(stderr, sa1); */

        /* get textlines from morphtemplate1.txt */
    if ((filestr = (char *)arrayRead(TEMPLATE1, &nbytes)) == NULL)
        return ERROR_INT("filestr not made", procName, 1);
    if ((sa2 = sarrayCreateLinesFromString(filestr, 1)) == NULL)
        return ERROR_INT("sa2 not made", procName, 1);
    FREE(filestr);

/*    sarrayWriteStream(stderr, sa2); */

        /* special function call strings */
    sprintf(bigbuf, "pixFMorphopGen_%d(PIX      *pixd,", fileindex);
    toplevelcall = stringNew(bigbuf);
    sprintf(bigbuf,
        "        fmorphopgen_low_%d(datad, w, h, wpld, datat, wpls, index);",
        fileindex);
    lowlevelcall1 = stringNew(bigbuf);
    sprintf(bigbuf,
        "        fmorphopgen_low_%d(datad, w, h, wpld, datas, wpls, index);",
        fileindex);
    lowlevelcall2 = stringNew(bigbuf);

        /* output to this sa */
    if ((sa3 = sarrayCreate(0)) == NULL)
        return ERROR_INT("sa3 not made", procName, 1);

        /* copyright notice and info header: lines 1-24  */
    for (i = NSTART1; i <= NSTOP1; i++) {
        if ((linestr = sarrayGetString(sa2, i, 1)) == NULL)
            return ERROR_INT("linestr not retrieved", procName, 1);
        sarrayAddString(sa3, linestr, 0);
    }
        
        /* static globals */
    sprintf(bigbuf, "static l_int32   NUM_SELS_GENERATED = %d;\n", nsels);
    sarrayAddString(sa3, bigbuf, 1);
    sprintf(bigbuf, "static char  *SEL_NAMES[] = {");
    sarrayAddString(sa3, bigbuf, 1);
    for (i = 0; i < nsels - 1; i++) {
        sprintf(bigbuf, "                             \"%s\",", sarrayGetString(sa1, i, 0));
        sarrayAddString(sa3, bigbuf, 1);
    }
    sprintf(bigbuf, "                             \"%s\"};\n", sarrayGetString(sa1, i, 0));
    sarrayAddString(sa3, bigbuf, 1);

        /* descriptive function header: lines 32-43 */
    for (i = NSTART2; i <= NSTOP2; i++) {
        if ((linestr = sarrayGetString(sa2, i, 1)) == NULL)
            return ERROR_INT("linestr not retrieved", procName, 1);
        sarrayAddString(sa3, linestr, 0);
    }

        /* incorporate first line of toplevel function call */
    sarrayAddString(sa3, toplevelcall, 0);

        /* next patch of function: lines 45-97 */
    for (i = NSTART3; i <= NSTOP3; i++) {
        if ((linestr = sarrayGetString(sa2, i, 1)) == NULL)
            return ERROR_INT("linestr not retrieved", procName, 1);
        sarrayAddString(sa3, linestr, 0);
    }

        /* incorporate first lowlevel function call */
    sarrayAddString(sa3, lowlevelcall1, 0);

        /* next patch of function: lines 99-101 */
    for (i = NSTART4; i <= NSTOP4; i++) {
        if ((linestr = sarrayGetString(sa2, i, 1)) == NULL)
            return ERROR_INT("linestr not retrieved", procName, 1);
        sarrayAddString(sa3, linestr, 0);
    }

        /* incorporate second lowlevel function call */
    sarrayAddString(sa3, lowlevelcall2, 0);

        /* rest of morphtemplate1.txt: lines 103-107 */
    for (i = NSTART5; i <= NSTOP5; i++) {
        if ((linestr = sarrayGetString(sa2, i, 1)) == NULL)
            return ERROR_INT("linestr not retrieved", procName, 1);
        sarrayAddString(sa3, linestr, 0);
    }

    /* --------------------------------------------------------------*
     *                       output to first file                    *
     * --------------------------------------------------------------*/

    if ((filestr = sarrayToString(sa3, 1)) == NULL)
        return ERROR_INT("filestr from sa3 not made", procName, 1);
    nbytes = strlen(filestr);
    sprintf(bigbuf, "%s.%d.c", OUTROOT, fileindex);
    arrayWrite(bigbuf, "w", filestr, nbytes);
    sarrayDestroy(&sa1);
    sarrayDestroy(&sa2);
    sarrayDestroy(&sa3);
    FREE(filestr);
        
    /* --------------------------------------------------------------*
     *                   generate data for second file               *
     * --------------------------------------------------------------*/

        /* get textlines from morphtemplate2.txt */
    if ((filestr = (char *)arrayRead(TEMPLATE2, &nbytes)) == NULL)
        return ERROR_INT("filestr not made", procName, 1);
    if ((sa1 = sarrayCreateLinesFromString(filestr, 1)) == NULL)
        return ERROR_INT("sa1 not made", procName, 1);
    FREE(filestr);

        /* make the static function names */
    if ((sa2 = sarrayCreate(2 * nsels)) == NULL)
        return ERROR_INT("sa2 not made", procName, 1);
    for (i = 0; i < nsels; i++) {
        sprintf(bigbuf, "fdilate_%d_%d", fileindex, i);
        sarrayAddString(sa2, bigbuf, 1);
        sprintf(bigbuf, "ferode_%d_%d", fileindex, i);
        sarrayAddString(sa2, bigbuf, 1);
    }

        /* make the static prototype strings */
    if ((sa3 = sarrayCreate(2 * nsels)) == NULL)
        return ERROR_INT("sa3 not made", procName, 1);
    for (i = 0; i < 2 * nsels; i++) {
        fname = sarrayGetString(sa2, i, 0);
        sprintf(bigbuf, "static void  %s%s", fname, PROTOARGS);
        sarrayAddString(sa3, bigbuf, 1);
    }

        /* make the dispatcher first line */
    sprintf(bigbuf, "fmorphopgen_low_%d(l_uint32  *datad,", fileindex);
    lowleveldefine = stringNew(bigbuf);

        /* output to this sa */
    if ((sa4 = sarrayCreate(0)) == NULL)
        return ERROR_INT("sa4 not made", procName, 1);

        /* copyright notice and info header: lines 1-29  */
    for (i = NSTART6; i <= NSTOP6; i++) {
        if ((linestr = sarrayGetString(sa1, i, 1)) == NULL)
            return ERROR_INT("linestr not retrieved", procName, 1);
        sarrayAddString(sa4, linestr, 0);
    }
        
        /* insert static protos */
    for (i = 0; i < 2 * nsels; i++) {
        if ((linestr = sarrayGetString(sa3, i, 1)) == NULL)
            return ERROR_INT("linestr not retrieved", procName, 1);
        sarrayAddString(sa4, linestr, 0);
    }
        
        /* function info header: lines 33-43  */
    for (i = NSTART7; i <= NSTOP7; i++) {
        if ((linestr = sarrayGetString(sa1, i, 1)) == NULL)
            return ERROR_INT("linestr not retrieved", procName, 1);
        sarrayAddString(sa4, linestr, 0);
    }
        
        /* incorporate first line of dispatcher */
    sarrayAddString(sa4, lowleveldefine, 0);

        /* beginning of function body: lines 45-56  */
    for (i = NSTART8; i <= NSTOP8; i++) {
        if ((linestr = sarrayGetString(sa1, i, 1)) == NULL)
            return ERROR_INT("linestr not retrieved", procName, 1);
        sarrayAddString(sa4, linestr, 0);
    }

        /* make the dispatcher code */
    for (i = 0; i < 2 * nsels; i++) {
        sprintf(bigbuf, "    case %d:", i);
        sarrayAddString(sa4, bigbuf, 1);
        sprintf(bigbuf, "        %s(datad, w, h, wpld, datas, wpls);",
               sarrayGetString(sa2, i, 0));
        sarrayAddString(sa4, bigbuf, 1);
        sarrayAddString(sa4, "        break;", 1);
    }

        /* intro to static function routines: lines 60-77  */
    for (i = NSTART9; i <= NSTOP9; i++) {
        if ((linestr = sarrayGetString(sa1, i, 1)) == NULL)
            return ERROR_INT("linestr not retrieved", procName, 1);
        sarrayAddString(sa4, linestr, 0);
    }

        /* do all the static functions */
    for (i = 0; i < 2 * nsels; i++) {

        sarrayAddString(sa4, "static void", 1);
        fname = sarrayGetString(sa2, i, 0);
        sprintf(bigbuf, "%s(l_uint32  *datad,", fname);
        sarrayAddString(sa4, bigbuf, 1);

            /* finish function header:  lines 80-88 */
        for (j = NSTART10; j <= NSTOP10; j++) {
            if ((linestr = sarrayGetString(sa1, j, 1)) == NULL)
                return ERROR_INT("linestr not retrieved", procName, 1);
            sarrayAddString(sa4, linestr, 0);
        }

            /* declare and define wplsN args, as necessary */
        if ((sel = selaGetSel(sela, i/2)) == NULL)
            return ERROR_INT("sel not returned", procName, 1);
        if ((sa5 = sarrayMakeWplsCode(sel)) == NULL) 
            return ERROR_INT("sa5 not made", procName, 1);
        sarrayConcatenate(sa4, sa5);
        sarrayDestroy(&sa5);
        
            /* start function loop definition:  lines 92-98 */
        for (j = NSTART11; j <= NSTOP11; j++) {
            if ((linestr = sarrayGetString(sa1, j, 1)) == NULL)
                return ERROR_INT("linestr not retrieved", procName, 1);
            sarrayAddString(sa4, linestr, 0);
        }

            /* insert barrel-op code for *dptr */
        if ((sa6 = sarrayMakeInnerLoopDWACode(sel, i)) == NULL)
            return ERROR_INT("sa6 not made", procName, 1);
        sarrayConcatenate(sa4, sa6);
        sarrayDestroy(&sa6);

            /* finish function loop definition:  lines 102-106 */
        for (j = NSTART12; j <= NSTOP12; j++) {
            if ((linestr = sarrayGetString(sa1, j, 1)) == NULL)
                return ERROR_INT("linestr not retrieved", procName, 1);
            sarrayAddString(sa4, linestr, 0);
        }

    }

    /* --------------------------------------------------------------*
     *                     output to second file                     *
     * --------------------------------------------------------------*/

    if ((filestr = sarrayToString(sa4, 1)) == NULL)
        return ERROR_INT("filestr from sa4 not made", procName, 1);
    nbytes = strlen(filestr);
    sprintf(bigbuf, "%s.%d.c", OUTROOTLOW, fileindex);
    arrayWrite(bigbuf, "w", filestr, nbytes);
    sarrayDestroy(&sa1);
    sarrayDestroy(&sa2);
    sarrayDestroy(&sa3);
    sarrayDestroy(&sa4);
    FREE(filestr);
        
    return 0;
}


/*--------------------------------------------------------------------------*
 *                            Helper code for sel                           *
 *--------------------------------------------------------------------------*/
/*!
 *  sarrayMakeWplsCode()
 */
static SARRAY *
sarrayMakeWplsCode(SEL  *sel)
{
l_int32  i, j, ymax, dely;
SARRAY  *sa;

    PROCNAME("sarrayMakeWplsCode");

    if (!sel)
        return (SARRAY *)ERROR_PTR("sel not defined", procName, NULL);

    ymax = 0;
    for (i = 0; i < sel->sy; i++) {
        for (j = 0; j < sel->sx; j++) {
            if (sel->data[i][j] == 1) {
                dely = L_ABS(i - sel->cy);
                ymax = L_MAX(ymax, dely);
            }
        }
    }
    if (ymax > 31) {
        L_WARNING("ymax > 31; truncating to 31", procName);
        ymax = 31;
    }

    if ((sa = sarrayCreate(0)) == NULL)
        return (SARRAY *)ERROR_PTR("sa not made", procName, NULL);

        /* declarations */
    if (ymax > 4)
        sarrayAddString(sa, wpldecls[2], 1);
    if (ymax > 8)
        sarrayAddString(sa, wpldecls[6], 1);
    if (ymax > 12)
        sarrayAddString(sa, wpldecls[10], 1);
    if (ymax > 16)
        sarrayAddString(sa, wpldecls[14], 1);
    if (ymax > 20)
        sarrayAddString(sa, wpldecls[18], 1);
    if (ymax > 24)
        sarrayAddString(sa, wpldecls[22], 1);
    if (ymax > 28)
        sarrayAddString(sa, wpldecls[26], 1);
    if (ymax > 1)
        sarrayAddString(sa, wpldecls[ymax - 2], 1);

    sarrayAddString(sa, "    ", 1);

        /* definitions */
    for (i = 2; i <= ymax; i++)
        sarrayAddString(sa, wpldefs[i - 2], 1);

    return sa;
}


/*!
 *  sarrayMakeInnerLoopDWACode()
 */
static SARRAY *
sarrayMakeInnerLoopDWACode(SEL     *sel,
                           l_int32  index)
{
char    *tstr, *string;
char     logicalor[] = "|";
char     logicaland[] = "&";
char     bigbuf[BUFFER_SIZE];
l_int32  i, j, optype, count, nfound, delx, dely;
SARRAY  *sa;

    PROCNAME("sarrayMakeInnerLoopDWACode");

    if (!sel)
        return (SARRAY *)ERROR_PTR("sel not defined", procName, NULL);

    if (index % 2 == 0) {
        optype = L_MORPH_DILATE;
        tstr = logicalor;
    }
    else {
        optype = L_MORPH_ERODE;
        tstr = logicaland;
    }

    count = 0;
    for (i = 0; i < sel->sy; i++) {
        for (j = 0; j < sel->sx; j++) {
            if (sel->data[i][j] == 1)
                count++;
        }
    }

    if ((sa = sarrayCreate(0)) == NULL)
        return (SARRAY *)ERROR_PTR("sa not made", procName, NULL);

    nfound = 0;
    for (i = 0; i < sel->sy; i++) {
        for (j = 0; j < sel->sx; j++) {
            if (sel->data[i][j] == 1) {
                nfound++;
                if (optype == L_MORPH_DILATE) {
                    dely = sel->cy - i;
                    delx = sel->cx - j;
                }
                else if (optype == L_MORPH_ERODE) {
                    dely = i - sel->cy;
                    delx = j - sel->cx;
                }
                if ((string = makeBarrelshiftString(delx, dely)) == NULL) {
                    L_WARNING("barrel shift string not made", procName);
                    continue;
                }
                if (count == 1)  /* just one item */
                    sprintf(bigbuf, "            *dptr = %s;", string);
                else if (nfound == 1)
                    sprintf(bigbuf, "            *dptr = %s %s", string, tstr);
                else if (nfound < count)
                    sprintf(bigbuf, "                    %s %s", string, tstr);
                else  /* nfound == count */
                    sprintf(bigbuf, "                    %s;", string);
                sarrayAddString(sa, bigbuf, 1);
                FREE(string);
            }
        }
    }

    return sa;
}


/*!
 *  makeBarrelshiftString()
 */
static char *
makeBarrelshiftString(l_int32  delx,    /* j - cx */
                      l_int32  dely)    /* i - cy */
{
l_int32  absx, absy;
char     bigbuf[BUFFER_SIZE];

    PROCNAME("makeBarrelshiftString");

    if (delx < -31 || delx > 31)
        return (char *)ERROR_PTR("delx out of bounds", procName, NULL);
    if (dely < -31 || dely > 31)
        return (char *)ERROR_PTR("dely out of bounds", procName, NULL);
    absx = L_ABS(delx);
    absy = L_ABS(dely);

    if ((delx == 0) && (dely == 0))
        sprintf(bigbuf, "(*sptr)");
    else if ((delx == 0) && (dely < 0))
        sprintf(bigbuf, "(*(sptr %s))", wplstrm[absy - 1]);
    else if ((delx == 0) && (dely > 0))
        sprintf(bigbuf, "(*(sptr %s))", wplstrp[absy - 1]);
    else if ((delx < 0) && (dely == 0))
        sprintf(bigbuf, "((*(sptr) >> %d) | (*(sptr - 1) << %d))",
              absx, 32 - absx);
    else if ((delx > 0) && (dely == 0))
        sprintf(bigbuf, "((*(sptr) << %d) | (*(sptr + 1) >> %d))",
              absx, 32 - absx);
    else if ((delx < 0) && (dely < 0))
        sprintf(bigbuf, "((*(sptr %s) >> %d) | (*(sptr %s - 1) << %d))",
              wplstrm[absy - 1], absx, wplstrm[absy - 1], 32 - absx);
    else if ((delx > 0) && (dely < 0))
        sprintf(bigbuf, "((*(sptr %s) << %d) | (*(sptr %s + 1) >> %d))",
              wplstrm[absy - 1], absx, wplstrm[absy - 1], 32 - absx);
    else if ((delx < 0) && (dely > 0))
        sprintf(bigbuf, "((*(sptr %s) >> %d) | (*(sptr %s - 1) << %d))",
              wplstrp[absy - 1], absx, wplstrp[absy - 1], 32 - absx);
    else  /*  ((delx > 0) && (dely > 0))  */
        sprintf(bigbuf, "((*(sptr %s) << %d) | (*(sptr %s + 1) >> %d))",
              wplstrp[absy - 1], absx, wplstrp[absy - 1], 32 - absx);
            
    return stringNew(bigbuf);
}
