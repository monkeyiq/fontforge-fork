/* Copyright (C) 2000-2012 by George Williams */
/*
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.

 * The name of the author may not be used to endorse or promote products
 * derived from this software without specific prior written permission.

 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include <fontforge-config.h>

#include "inc/gnetwork.h"
#include "collabclientui.h"
#include "collabclientpriv.h"

#include "fontforgeui.h"
#include "groups.h"
#include "psfont.h"
#include <gfile.h>
#include <gio.h>
#include <gresedit.h>
#include <ustring.h>
#include "../fontforge/ffglib.h"
#include <gkeysym.h>
#include <utype.h>
#include <chardata.h>
#include <gresource.h>
#include <math.h>
#include <unistd.h>

#include "gutils/unicodelibinfo.h"
#include "sfundo.h"
#include "sharedmenu.h"
#include "classtypeui.h"
#include "wordlistparser.h"
#include "fontview.h"

#if defined (__MINGW32__)
#include <windows.h>
#endif

#include "xvasprintf.h"


int OpenCharsInNewWindow = 0;
char *RecentFiles[RECENT_MAX] = { NULL };
int save_to_dir = 0;			/* use sfdir rather than sfd */
extern int onlycopydisplayed, copymetadata, copyttfinstr, add_char_to_name_list;
int home_char='A';
int compact_font_on_open=0;
int navigation_mask = 0;		/* Initialized in startui.c */

static char *fv_fontnames = MONO_UI_FAMILIES;
extern char* pref_collab_last_server_connected_to;
extern void python_call_onClosingFunctions();

#define	FV_LAB_HEIGHT	15

#ifdef BIGICONS
#define fontview_width 32
#define fontview_height 32
static unsigned char fontview_bits[] = {
   0x00, 0x00, 0x00, 0x00, 0xfe, 0xff, 0xff, 0xff, 0x02, 0x20, 0x80, 0x00,
   0x82, 0x20, 0x86, 0x08, 0x42, 0x21, 0x8a, 0x14, 0xc2, 0x21, 0x86, 0x04,
   0x42, 0x21, 0x8a, 0x14, 0x42, 0x21, 0x86, 0x08, 0x02, 0x20, 0x80, 0x00,
   0xaa, 0xaa, 0xaa, 0xaa, 0x02, 0x20, 0x80, 0x00, 0x82, 0xa0, 0x8f, 0x18,
   0x82, 0x20, 0x91, 0x24, 0x42, 0x21, 0x91, 0x02, 0x42, 0x21, 0x91, 0x02,
   0x22, 0x21, 0x8f, 0x02, 0xe2, 0x23, 0x91, 0x02, 0x12, 0x22, 0x91, 0x02,
   0x3a, 0x27, 0x91, 0x24, 0x02, 0xa0, 0x8f, 0x18, 0x02, 0x20, 0x80, 0x00,
   0xfe, 0xff, 0xff, 0xff, 0x02, 0x20, 0x80, 0x00, 0x42, 0x20, 0x86, 0x18,
   0xa2, 0x20, 0x8a, 0x04, 0xa2, 0x20, 0x86, 0x08, 0xa2, 0x20, 0x8a, 0x10,
   0x42, 0x20, 0x8a, 0x0c, 0x82, 0x20, 0x80, 0x00, 0x02, 0x20, 0x80, 0x00,
   0xaa, 0xaa, 0xaa, 0xaa, 0x02, 0x20, 0x80, 0x00};
#else
#define fontview2_width 16
#define fontview2_height 16
static unsigned char fontview2_bits[] = {
   0x00, 0x07, 0x80, 0x08, 0x40, 0x17, 0x40, 0x15, 0x60, 0x09, 0x10, 0x02,
   0xa0, 0x01, 0xa0, 0x00, 0xa0, 0x00, 0xa0, 0x00, 0x50, 0x00, 0x52, 0x00,
   0x55, 0x00, 0x5d, 0x00, 0x22, 0x00, 0x1c, 0x00};
#endif

extern int _GScrollBar_Width;

static int fv_fontsize = 11, fv_fs_init=0;
static Color fvselcol = 0xffff00, fvselfgcol=0x000000;
Color view_bgcol;
static Color fvglyphinfocol = 0xff0000;
static Color fvemtpyslotfgcol = 0xd08080;
static Color fvchangedcol = 0x000060;
static Color fvhintingneededcol = 0x0000ff;

int default_fv_showhmetrics=false, default_fv_showvmetrics=false,
	default_fv_glyphlabel = gl_glyph;
#define METRICS_BASELINE 0x0000c0
#define METRICS_ORIGIN	 0xc00000
#define METRICS_ADVANCE	 0x008000
FontView *fv_list=NULL;

static void AskAndMaybeCloseLocalCollabServers( void );





static void FV_ToggleCharChanged(SplineChar *sc) {
    int i, j;
    int pos;
    FontView *fv;

    for ( fv = (FontView *) (sc->parent->fv); fv!=NULL; fv=(FontView *) (fv->b.nextsame) ) {
	if ( fv->b.sf!=sc->parent )		/* Can happen in CID fonts if char's parent is not currently active */
    continue;
	if ( fv->v==NULL || fv->colcnt==0 )	/* Can happen in scripts */
    continue;
	for ( pos=0; pos<fv->b.map->enccount; ++pos ) if ( fv->b.map->map[pos]==sc->orig_pos ) {
	    i = pos / fv->colcnt;
	    j = pos - i*fv->colcnt;
	    i -= fv->rowoff;
 /* Normally we should be checking against fv->rowcnt (rather than <=rowcnt) */
 /*  but every now and then the WM forces us to use a window size which doesn't */
 /*  fit our expectations (maximized view) and we must be prepared for half */
 /*  lines */
	    if ( i>=0 && i<=fv->rowcnt ) {
		GRect r;
		r.x = j*fv->cbw+1; r.width = fv->cbw-1;
		r.y = i*fv->cbh+1; r.height = fv->lab_height-1;
		GDrawRequestExpose(fv->v,&r,false);
	    }
	}
    }
}

void FVMarkHintsOutOfDate(SplineChar *sc) {
    int i, j;
    int pos;
    FontView *fv;

    if ( sc->parent->onlybitmaps || sc->parent->multilayer || sc->parent->strokedfont )
return;
    for ( fv = (FontView *) (sc->parent->fv); fv!=NULL; fv=(FontView *) (fv->b.nextsame) ) {
	if ( fv->b.sf!=sc->parent )		/* Can happen in CID fonts if char's parent is not currently active */
    continue;
	if ( sc->layers[fv->b.active_layer].order2 )
    continue;
	if ( fv->v==NULL || fv->colcnt==0 )	/* Can happen in scripts */
    continue;
	for ( pos=0; pos<fv->b.map->enccount; ++pos ) if ( fv->b.map->map[pos]==sc->orig_pos ) {
	    i = pos / fv->colcnt;
	    j = pos - i*fv->colcnt;
	    i -= fv->rowoff;
 /* Normally we should be checking against fv->rowcnt (rather than <=rowcnt) */
 /*  but every now and then the WM forces us to use a window size which doesn't */
 /*  fit our expectations (maximized view) and we must be prepared for half */
 /*  lines */
	    if ( i>=0 && i<=fv->rowcnt ) {
		GRect r;
		r.x = j*fv->cbw+1; r.width = fv->cbw-1;
		r.y = i*fv->cbh+1; r.height = fv->lab_height-1;
		GDrawDrawLine(fv->v,r.x,r.y,r.x,r.y+r.height-1,fvhintingneededcol);
		GDrawDrawLine(fv->v,r.x+1,r.y,r.x+1,r.y+r.height-1,fvhintingneededcol);
		GDrawDrawLine(fv->v,r.x+r.width-1,r.y,r.x+r.width-1,r.y+r.height-1,fvhintingneededcol);
		GDrawDrawLine(fv->v,r.x+r.width-2,r.y,r.x+r.width-2,r.y+r.height-1,fvhintingneededcol);
	    }
	}
    }
}

static int FeatureTrans(FontView *fv, int enc) {
    SplineChar *sc;
    PST *pst;
    char *pt;
    int gid;

    if ( enc<0 || enc>=fv->b.map->enccount || (gid = fv->b.map->map[enc])==-1 )
return( -1 );
    if ( fv->cur_subtable==NULL )
return( gid );

    sc = fv->b.sf->glyphs[gid];
    if ( sc==NULL )
return( -1 );
    for ( pst = sc->possub; pst!=NULL; pst=pst->next ) {
	if (( pst->type == pst_substitution || pst->type == pst_alternate ) &&
		pst->subtable == fv->cur_subtable )
    break;
    }
    if ( pst==NULL )
return( -1 );
    pt = strchr(pst->u.subs.variant,' ');
    if ( pt!=NULL )
	*pt = '\0';
    gid = SFFindExistingSlot(fv->b.sf, -1, pst->u.subs.variant );
    if ( pt!=NULL )
	*pt = ' ';
return( gid );
}

static void FVDrawGlyph(GWindow pixmap, FontView *fv, int index, int forcebg ) {
    GRect box, old2;
    int feat_gid;
    SplineChar *sc;
    struct _GImage base;
    GImage gi;
    GClut clut;
    int i,j;
    int em = fv->b.sf->ascent+fv->b.sf->descent;
    int yorg = fv->magnify*(fv->show->ascent);

    i = index / fv->colcnt;
    j = index - i*fv->colcnt;
    i -= fv->rowoff;

    if ( index<fv->b.map->enccount && (fv->b.selected[index] || forcebg)) {
	box.x = j*fv->cbw+1; box.width = fv->cbw-1;
	box.y = i*fv->cbh+fv->lab_height+1; box.height = fv->cbw;
	GDrawFillRect(pixmap,&box,fv->b.selected[index] ? fvselcol : view_bgcol );
    }
    feat_gid = FeatureTrans(fv,index);
    sc = feat_gid!=-1 ? fv->b.sf->glyphs[feat_gid]: NULL;
    if ( !SCWorthOutputting(sc) ) {
	int x = j*fv->cbw+1, xend = x+fv->cbw-2;
	int y = i*fv->cbh+fv->lab_height+1, yend = y+fv->cbw-1;
	GDrawDrawLine(pixmap,x,y,xend,yend,fvemtpyslotfgcol);
	GDrawDrawLine(pixmap,x,yend,xend,y,fvemtpyslotfgcol);
    }
    if ( sc!=NULL ) {
	BDFChar *bdfc;

	if ( fv->show!=NULL && fv->show->piecemeal &&
		feat_gid!=-1 &&
		(feat_gid>=fv->show->glyphcnt || fv->show->glyphs[feat_gid]==NULL) &&
		fv->b.sf->glyphs[feat_gid]!=NULL )
	    BDFPieceMeal(fv->show,feat_gid);

	if ( fv->show!=NULL && feat_gid!=-1 &&
		feat_gid < fv->show->glyphcnt &&
		fv->show->glyphs[feat_gid]==NULL &&
		SCWorthOutputting(fv->b.sf->glyphs[feat_gid]) ) {
	    /* If we have an outline but no bitmap for this slot */
	    box.x = j*fv->cbw+1; box.width = fv->cbw-2;
	    box.y = i*fv->cbh+fv->lab_height+2; box.height = box.width+1;
	    GDrawDrawRect(pixmap,&box,0xff0000);
	    ++box.x; ++box.y; box.width -= 2; box.height -= 2;
	    GDrawDrawRect(pixmap,&box,0xff0000);
/* When reencoding a font we can find times where index>=show->charcnt */
	} else if ( fv->show!=NULL && feat_gid<fv->show->glyphcnt && feat_gid!=-1 &&
		fv->show->glyphs[feat_gid]!=NULL ) {
	    /* If fontview is set to display an embedded bitmap font (not a temporary font, */
	    /* rasterized specially for this purpose), then we can't use it directly, as bitmap */
	    /* glyphs may contain selections and references. So create a temporary copy of */
	    /* the glyph merging all such elements into a single bitmap */
	    bdfc = fv->show->piecemeal ?
		fv->show->glyphs[feat_gid] : BDFGetMergedChar( fv->show->glyphs[feat_gid] );

	    memset(&gi,'\0',sizeof(gi));
	    memset(&base,'\0',sizeof(base));
	    if ( bdfc->byte_data ) {
		gi.u.image = &base;
		base.image_type = it_index;
		if ( !fv->b.selected[index] )
		    base.clut = fv->show->clut;
		else {
		    int bgr=((fvselcol>>16)&0xff), bgg=((fvselcol>>8)&0xff), bgb= (fvselcol&0xff);
		    int fgr=((fvselfgcol>>16)&0xff), fgg=((fvselfgcol>>8)&0xff), fgb= (fvselfgcol&0xff);
		    int i;
		    memset(&clut,'\0',sizeof(clut));
		    base.clut = &clut;
		    clut.clut_len = fv->show->clut->clut_len;
		    for ( i=0; i<clut.clut_len; ++i ) {
			clut.clut[i] =
				COLOR_CREATE( bgr + (i*(fgr-bgr))/(clut.clut_len-1),
						bgg + (i*(fgg-bgg))/(clut.clut_len-1),
						bgb + (i*(fgb-bgb))/(clut.clut_len-1));
		    }
		}
		GDrawSetDither(NULL, false);	/* on 8 bit displays we don't want any dithering */
	    } else {
		memset(&clut,'\0',sizeof(clut));
		gi.u.image = &base;
		base.image_type = it_mono;
		base.clut = &clut;
		clut.clut_len = 2;
		clut.clut[0] = fv->b.selected[index] ? fvselcol : view_bgcol ;
		clut.clut[1] = fv->b.selected[index] ? fvselfgcol : 0 ;
	    }
	    base.trans = 0;
	    base.clut->trans_index = 0;

	    base.data = bdfc->bitmap;
	    base.bytes_per_line = bdfc->bytes_per_line;
	    base.width = bdfc->xmax-bdfc->xmin+1;
	    base.height = bdfc->ymax-bdfc->ymin+1;
	    box.x = j*fv->cbw; box.width = fv->cbw;
	    box.y = i*fv->cbh+fv->lab_height+1; box.height = box.width+1;
	    GDrawPushClip(pixmap,&box,&old2);
	    if ( !fv->b.sf->onlybitmaps && fv->show!=fv->filled &&
		    sc->layers[fv->b.active_layer].splines==NULL && sc->layers[fv->b.active_layer].refs==NULL &&
		    !sc->widthset &&
		    !(bdfc->xmax<=0 && bdfc->xmin==0 && bdfc->ymax<=0 && bdfc->ymax==0) ) {
		/* If we have a bitmap but no outline character... */
		GRect b;
		b.x = box.x+1; b.y = box.y+1; b.width = box.width-2; b.height = box.height-2;
		GDrawDrawRect(pixmap,&b,0x008000);
		++b.x; ++b.y; b.width -= 2; b.height -= 2;
		GDrawDrawRect(pixmap,&b,0x008000);
	    }
	    /* I assume that the bitmap image matches the bounding*/
	    /*  box. In some bitmap fonts the bitmap has white space on the*/
	    /*  right. This can throw off the centering algorithem */
	    if ( fv->magnify>1 ) {
		GDrawDrawImageMagnified(pixmap,&gi,NULL,
			j*fv->cbw+(fv->cbw-1-fv->magnify*base.width)/2,
			i*fv->cbh+fv->lab_height+1+fv->magnify*(fv->show->ascent-bdfc->ymax),
			fv->magnify*base.width,fv->magnify*base.height);
	    } else if ( (GDrawHasCairo(pixmap)&gc_alpha) && base.image_type==it_index ) {
		GDrawDrawGlyph(pixmap,&gi,NULL,
			j*fv->cbw+(fv->cbw-1-base.width)/2,
			i*fv->cbh+fv->lab_height+1+fv->show->ascent-bdfc->ymax);
	    } else
		GDrawDrawImage(pixmap,&gi,NULL,
			j*fv->cbw+(fv->cbw-1-base.width)/2,
			i*fv->cbh+fv->lab_height+1+fv->show->ascent-bdfc->ymax);
	    if ( fv->showhmetrics ) {
		int x1, x0 = j*fv->cbw+(fv->cbw-1-fv->magnify*base.width)/2- bdfc->xmin*fv->magnify;
		/* Draw advance width & horizontal origin */
		if ( fv->showhmetrics&fvm_origin )
		    GDrawDrawLine(pixmap,x0,i*fv->cbh+fv->lab_height+yorg-3,x0,
			    i*fv->cbh+fv->lab_height+yorg+2,METRICS_ORIGIN);
		x1 = x0 + fv->magnify*bdfc->width;
		if ( fv->showhmetrics&fvm_advanceat )
		    GDrawDrawLine(pixmap,x1,i*fv->cbh+fv->lab_height+1,x1,
			    (i+1)*fv->cbh-1,METRICS_ADVANCE);
		if ( fv->showhmetrics&fvm_advanceto )
		    GDrawDrawLine(pixmap,x0,(i+1)*fv->cbh-2,x1,
			    (i+1)*fv->cbh-2,METRICS_ADVANCE);
	    }
	    if ( fv->showvmetrics ) {
		int x0 = j*fv->cbw+(fv->cbw-1-fv->magnify*base.width)/2- bdfc->xmin*fv->magnify
			+ fv->magnify*fv->show->pixelsize/2;
		int y0 = i*fv->cbh+fv->lab_height+yorg;
		int yvw = y0 + fv->magnify*sc->vwidth*fv->show->pixelsize/em;
		if ( fv->showvmetrics&fvm_baseline )
		    GDrawDrawLine(pixmap,x0,i*fv->cbh+fv->lab_height+1,x0,
			    (i+1)*fv->cbh-1,METRICS_BASELINE);
		if ( fv->showvmetrics&fvm_advanceat )
		    GDrawDrawLine(pixmap,j*fv->cbw,yvw,(j+1)*fv->cbw,
			    yvw,METRICS_ADVANCE);
		if ( fv->showvmetrics&fvm_advanceto )
		    GDrawDrawLine(pixmap,j*fv->cbw+2,y0,j*fv->cbw+2,
			    yvw,METRICS_ADVANCE);
		if ( fv->showvmetrics&fvm_origin )
		    GDrawDrawLine(pixmap,x0-3,i*fv->cbh+fv->lab_height+yorg,x0+2,i*fv->cbh+fv->lab_height+yorg,METRICS_ORIGIN);
	    }
	    GDrawPopClip(pixmap,&old2);
	    if ( !fv->show->piecemeal ) BDFCharFree( bdfc );
	}
    }
}

static void FVToggleCharSelected(FontView *fv,int enc) {
    int i, j;

    if ( fv->v==NULL || fv->colcnt==0 )	/* Can happen in scripts */
return;

    i = enc / fv->colcnt;
    j = enc - i*fv->colcnt;
    i -= fv->rowoff;
 /* Normally we should be checking against fv->rowcnt (rather than <=rowcnt) */
 /*  but every now and then the WM forces us to use a window size which doesn't */
 /*  fit our expectations (maximized view) and we must be prepared for half */
 /*  lines */
    if ( i>=0 && i<=fv->rowcnt )
	FVDrawGlyph(fv->v,fv,enc,true);
}

static void FontViewRefreshAll(SplineFont *sf) {
    FontView *fv;
    for ( fv = (FontView *) (sf->fv); fv!=NULL; fv = (FontView *) (fv->b.nextsame) )
	if ( fv->v!=NULL )
	    GDrawRequestExpose(fv->v,NULL,false);
}

void FVDeselectAll(FontView *fv) {
    int i;

    for ( i=0; i<fv->b.map->enccount; ++i ) {
	if ( fv->b.selected[i] ) {
	    fv->b.selected[i] = false;
	    FVToggleCharSelected(fv,i);
	}
    }
    fv->sel_index = 0;
}

static void FVInvertSelection(FontView *fv) {
    int i;

    for ( i=0; i<fv->b.map->enccount; ++i ) {
	fv->b.selected[i] = !fv->b.selected[i];
	FVToggleCharSelected(fv,i);
    }
    fv->sel_index = 1;
}

static void FVSelectAll(FontView *fv) {
    int i;

    for ( i=0; i<fv->b.map->enccount; ++i ) {
	if ( !fv->b.selected[i] ) {
	    fv->b.selected[i] = true;
	    FVToggleCharSelected(fv,i);
	}
    }
    fv->sel_index = 1;
}

static void FVReselect(FontView *fv, int newpos) {
    int i;

    if ( newpos<0 ) newpos = 0;
    else if ( newpos>=fv->b.map->enccount ) newpos = fv->b.map->enccount-1;

    if ( fv->pressed_pos<fv->end_pos ) {
	if ( newpos>fv->end_pos ) {
	    for ( i=fv->end_pos+1; i<=newpos; ++i ) if ( !fv->b.selected[i] ) {
		fv->b.selected[i] = fv->sel_index;
		FVToggleCharSelected(fv,i);
	    }
	} else if ( newpos<fv->pressed_pos ) {
	    for ( i=fv->end_pos; i>fv->pressed_pos; --i ) if ( fv->b.selected[i] ) {
		fv->b.selected[i] = false;
		FVToggleCharSelected(fv,i);
	    }
	    for ( i=fv->pressed_pos-1; i>=newpos; --i ) if ( !fv->b.selected[i] ) {
		fv->b.selected[i] = fv->sel_index;
		FVToggleCharSelected(fv,i);
	    }
	} else {
	    for ( i=fv->end_pos; i>newpos; --i ) if ( fv->b.selected[i] ) {
		fv->b.selected[i] = false;
		FVToggleCharSelected(fv,i);
	    }
	}
    } else {
	if ( newpos<fv->end_pos ) {
	    for ( i=fv->end_pos-1; i>=newpos; --i ) if ( !fv->b.selected[i] ) {
		fv->b.selected[i] = fv->sel_index;
		FVToggleCharSelected(fv,i);
	    }
	} else if ( newpos>fv->pressed_pos ) {
	    for ( i=fv->end_pos; i<fv->pressed_pos; ++i ) if ( fv->b.selected[i] ) {
		fv->b.selected[i] = false;
		FVToggleCharSelected(fv,i);
	    }
	    for ( i=fv->pressed_pos+1; i<=newpos; ++i ) if ( !fv->b.selected[i] ) {
		fv->b.selected[i] = fv->sel_index;
		FVToggleCharSelected(fv,i);
	    }
	} else {
	    for ( i=fv->end_pos; i<newpos; ++i ) if ( fv->b.selected[i] ) {
		fv->b.selected[i] = false;
		FVToggleCharSelected(fv,i);
	    }
	}
    }
    fv->end_pos = newpos;
    if ( newpos>=0 && newpos<fv->b.map->enccount && (i = fv->b.map->map[newpos])!=-1 &&
	    fv->b.sf->glyphs[i]!=NULL &&
	    fv->b.sf->glyphs[i]->unicodeenc>=0 && fv->b.sf->glyphs[i]->unicodeenc<0x10000 )
	GInsCharSetChar(fv->b.sf->glyphs[i]->unicodeenc);
}

static void FVFlattenAllBitmapSelections(FontView *fv) {
    BDFFont *bdf;
    int i;

    for ( bdf = fv->b.sf->bitmaps; bdf!=NULL; bdf=bdf->next ) {
	for ( i=0; i<bdf->glyphcnt; ++i )
	    if ( bdf->glyphs[i]!=NULL && bdf->glyphs[i]->selection!=NULL )
		BCFlattenFloat(bdf->glyphs[i]);
    }
}

static int AskChanged(SplineFont *sf) {
    int ret;
    char *buts[4];
    char *filename, *fontname;

    if ( sf->cidmaster!=NULL )
	sf = sf->cidmaster;

    filename = sf->filename;
    fontname = sf->fontname;

    if ( filename==NULL && sf->origname!=NULL &&
	    sf->onlybitmaps && sf->bitmaps!=NULL && sf->bitmaps->next==NULL )
	filename = sf->origname;
    if ( filename==NULL ) filename = "untitled.sfd";
    filename = GFileNameTail(filename);
    buts[0] = _("_Save");
    buts[1] = _("_Don't Save");
    buts[2] = _("_Cancel");
    buts[3] = NULL;
    ret = gwwv_ask( _("Font changed"),(const char **) buts,0,2,_("Font %1$.40s in file %2$.40s has been changed.\nDo you want to save it?"),fontname,filename);
return( ret );
}

int _FVMenuGenerate(FontView *fv,int family,int layer) {
    if( layer == -1 )
	layer = fv->b.active_layer;
    
    FVFlattenAllBitmapSelections(fv);
    int rc = SFGenerateFont( fv->b.sf,layer, family,
			     fv->b.normal==NULL?fv->b.map:fv->b.normal );
    return rc;
}

void FVMenuGenerate(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    
    CommonView* cc = tryObtainGDataCommonView( gw );
    FontView* fv = tryObtainGDataFontView( gw );
    int layer = cc->m_sharedmenu_funcs.getActiveLayer(cc);
    _FVMenuGenerate(fv,gf_none,layer);
}

void FVMenuGenerateFamily(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    FontView* fv = tryObtainGDataFontView( gw );
    _FVMenuGenerate(fv,gf_macfamily,-1);
}

void FVMenuGenerateTTC(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    FontView* fv = tryObtainGDataFontView( gw );
    _FVMenuGenerate(fv,gf_ttc,-1);
}

void FVMenuGenerateUFO(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    CommonView* cc = tryObtainGDataCommonView( gw );
    FontView* fv = tryObtainGDataFontView( gw );
    int layer = cc->m_sharedmenu_funcs.getActiveLayer(cc);
    _FVMenuGenerate(fv,gf_none,layer);
}
void FVMenuGenerateGraphic(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    CommonView* cc = tryObtainGDataCommonView( gw );
    FontView* fv = tryObtainGDataFontView( gw );
    int layer = cc->m_sharedmenu_funcs.getActiveLayer(cc);
    _FVMenuGenerate(fv,gf_svg,layer);
}

extern int save_to_dir;

static int SaveAs_FormatChange(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_radiochanged ) {
	GGadget *fc = GWidgetGetControl(GGadgetGetWindow(g),1000);
	char *oldname = GGadgetGetTitle8(fc);
	int *_s2d = GGadgetGetUserData(g);
	int s2d = GGadgetIsChecked(g);
	char *pt, *newname = malloc(strlen(oldname)+8);
	strcpy(newname,oldname);
	pt = strrchr(newname,'.');
	if ( pt==NULL )
	    pt = newname+strlen(newname);
	strcpy(pt,s2d ? ".sfdir" : ".sfd" );
	GGadgetSetTitle8(fc,newname);
	save_to_dir = *_s2d = s2d;
	SavePrefs(true);
    }
return( true );
}


static enum fchooserret _FVSaveAsFilterFunc(GGadget *g,struct gdirentry *ent, const unichar_t *dir)
{
    char* n = u_to_c(ent->name);
    int ew = endswithi( n, "sfd" ) || endswithi( n, "sfdir" );
    if( ew )
	return fc_show;
    if( ent->isdir )
	return fc_show;
    return fc_hide;
}


int _FVMenuSaveAs(FontView *fv) {
    char *temp;
    char *ret;
    char *filename;
    int ok;
    int s2d = fv->b.cidmaster!=NULL ? fv->b.cidmaster->save_to_dir :
		fv->b.sf->mm!=NULL ? fv->b.sf->mm->normal->save_to_dir :
		fv->b.sf->save_to_dir;
    GGadgetCreateData gcd;
    GTextInfo label;

    if ( fv->b.cidmaster!=NULL && fv->b.cidmaster->filename!=NULL )
	temp=def2utf8_copy(fv->b.cidmaster->filename);
    else if ( fv->b.sf->mm!=NULL && fv->b.sf->mm->normal->filename!=NULL )
	temp=def2utf8_copy(fv->b.sf->mm->normal->filename);
    else if ( fv->b.sf->filename!=NULL )
	temp=def2utf8_copy(fv->b.sf->filename);
    else {
	SplineFont *sf = fv->b.cidmaster?fv->b.cidmaster:
		fv->b.sf->mm!=NULL?fv->b.sf->mm->normal:fv->b.sf;
	char *fn = sf->defbasefilename ? sf->defbasefilename : sf->fontname;
	temp = malloc((strlen(fn)+10));
	strcpy(temp,fn);
	if ( sf->defbasefilename!=NULL )
	    /* Don't add a default suffix, they've already told us what name to use */;
	else if ( fv->b.cidmaster!=NULL )
	    strcat(temp,"CID");
	else if ( sf->mm==NULL )
	    ;
	else if ( sf->mm->apple )
	    strcat(temp,"Var");
	else
	    strcat(temp,"MM");
	strcat(temp,save_to_dir ? ".sfdir" : ".sfd");
	s2d = save_to_dir;
    }

    memset(&gcd,0,sizeof(gcd));
    memset(&label,0,sizeof(label));
    gcd.gd.flags = s2d ? (gg_visible | gg_enabled | gg_cb_on) : (gg_visible | gg_enabled);
    label.text = (unichar_t *) _("Save as _Directory");
    label.text_is_1byte = true;
    label.text_in_resource = true;
    gcd.gd.label = &label;
    gcd.gd.handle_controlevent = SaveAs_FormatChange;
    gcd.data = &s2d;
    gcd.creator = GCheckBoxCreate;

    GFileChooserInputFilenameFuncType FilenameFunc = GFileChooserDefInputFilenameFunc;

#if defined(__MINGW32__)
    //
    // If they are "saving as" but there is no path, lets help
    // the poor user by starting someplace sane rather than in `pwd`
    //
    if( !GFileIsAbsolute(temp) )
    {
    	char* defaultSaveDir = GFileGetHomeDocumentsDir();
	printf("save-as:%s\n", temp );
    	char* temp2 = GFileAppendFile( defaultSaveDir, temp, 0 );
    	free(temp);
    	temp = temp2;
    }
#endif

    ret = GWidgetSaveAsFileWithGadget8(_("Save as..."),temp,0,NULL,
				       _FVSaveAsFilterFunc, FilenameFunc,
				       &gcd );
    free(temp);
    if ( ret==NULL )
return( 0 );
    filename = utf82def_copy(ret);
    free(ret);

    if(!(endswithi( filename, ".sfdir") || endswithi( filename, ".sfd")))
    {
	// they forgot the extension, so we force the default of .sfd
	// and alert them to the fact that we have done this and we
	// are not saving to a OTF, TTF, UFO formatted file

	char* extension = ".sfd";
	char* newpath = copyn( filename, strlen(filename) + strlen(".sfd") + 1 );
	strcat( newpath, ".sfd" );

	char* oldfn = GFileNameTail( filename );
	char* newfn = GFileNameTail( newpath );
	
	LogError( _("You tried to save with the filename %s but it was saved as %s. "),
		  oldfn, newfn );
	LogError( _("Please choose File/Generate Fonts to save to other formats."));

	free(filename);
	filename = newpath;
    }
    
    FVFlattenAllBitmapSelections(fv);
    fv->b.sf->compression = 0;
    ok = SFDWrite(filename,fv->b.sf,fv->b.map,fv->b.normal,s2d);
    if ( ok ) {
	SplineFont *sf = fv->b.cidmaster?fv->b.cidmaster:fv->b.sf->mm!=NULL?fv->b.sf->mm->normal:fv->b.sf;
	free(sf->filename);
	sf->filename = filename;
	sf->save_to_dir = s2d;
	free(sf->origname);
	sf->origname = copy(filename);
	sf->new = false;
	if ( sf->mm!=NULL ) {
	    int i;
	    for ( i=0; i<sf->mm->instance_count; ++i ) {
		free(sf->mm->instances[i]->filename);
		sf->mm->instances[i]->filename = filename;
		free(sf->mm->instances[i]->origname);
		sf->mm->instances[i]->origname = copy(filename);
		sf->mm->instances[i]->new = false;
	    }
	}
	SplineFontSetUnChanged(sf);
	FVSetTitles(fv->b.sf);
    } else
	free(filename);
return( ok );
}

void FVMenuSaveAs(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    FontView* fv = tryObtainGDataFontView( gw );
    _FVMenuSaveAs(fv);
}

static int IsBackupName(char *filename) {

    if ( filename==NULL )
return( false );
return( filename[strlen(filename)-1]=='~' );
}

int _FVMenuSave(FontView *fv) {
    int ret = 0;
    SplineFont *sf = fv->b.cidmaster?fv->b.cidmaster:
		    fv->b.sf->mm!=NULL?fv->b.sf->mm->normal:
			    fv->b.sf;

    if ( sf->filename==NULL || IsBackupName(sf->filename))
	ret = _FVMenuSaveAs(fv);
    else {
	FVFlattenAllBitmapSelections(fv);
	if ( !SFDWriteBak(sf->filename,sf,fv->b.map,fv->b.normal) )
	    ff_post_error(_("Save Failed"),_("Save Failed"));
	else {
	    SplineFontSetUnChanged(sf);
	    ret = true;
	}
    }
return( ret );
}

void FVMenuSave(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    FontView* fv = tryObtainGDataFontView( gw );
    _FVMenuSave(fv);
}

void _FVCloseWindows(FontView *fv) {
    int i, j;
    BDFFont *bdf;
    MetricsView *mv, *mnext;
    SplineFont *sf = fv->b.cidmaster?fv->b.cidmaster:fv->b.sf->mm!=NULL?fv->b.sf->mm->normal : fv->b.sf;

    PrintWindowClose();
    if ( fv->b.nextsame==NULL && fv->b.sf->fv==&fv->b && fv->b.sf->kcld!=NULL )
	KCLD_End(fv->b.sf->kcld);
    if ( fv->b.nextsame==NULL && fv->b.sf->fv==&fv->b && fv->b.sf->vkcld!=NULL )
	KCLD_End(fv->b.sf->vkcld);

    for ( i=0; i<sf->glyphcnt; ++i ) if ( sf->glyphs[i]!=NULL ) {
	CharView *cv, *next;
	for ( cv = (CharView *) (sf->glyphs[i]->views); cv!=NULL; cv = next ) {
	    next = (CharView *) (cv->b.next);
	    GDrawDestroyWindow(cv->gw);
	}
	if ( sf->glyphs[i]->charinfo )
	    CharInfoDestroy(sf->glyphs[i]->charinfo);
    }
    if ( sf->mm!=NULL ) {
	MMSet *mm = sf->mm;
	for ( j=0; j<mm->instance_count; ++j ) {
	    SplineFont *sf = mm->instances[j];
	    for ( i=0; i<sf->glyphcnt; ++i ) if ( sf->glyphs[i]!=NULL ) {
		CharView *cv, *next;
		for ( cv = (CharView *) (sf->glyphs[i]->views); cv!=NULL; cv = next ) {
		    next = (CharView *) (cv->b.next);
		    GDrawDestroyWindow(cv->gw);
		}
		if ( sf->glyphs[i]->charinfo )
		    CharInfoDestroy(sf->glyphs[i]->charinfo);
	    }
	    for ( mv=sf->metrics; mv!=NULL; mv = mnext ) {
		mnext = mv->next;
		GDrawDestroyWindow(mv->gw);
	    }
	}
    } else if ( sf->subfontcnt!=0 ) {
	for ( j=0; j<sf->subfontcnt; ++j ) {
	    for ( i=0; i<sf->subfonts[j]->glyphcnt; ++i ) if ( sf->subfonts[j]->glyphs[i]!=NULL ) {
		CharView *cv, *next;
		for ( cv = (CharView *) (sf->subfonts[j]->glyphs[i]->views); cv!=NULL; cv = next ) {
		    next = (CharView *) (cv->b.next);
		    GDrawDestroyWindow(cv->gw);
		if ( sf->subfonts[j]->glyphs[i]->charinfo )
		    CharInfoDestroy(sf->subfonts[j]->glyphs[i]->charinfo);
		}
	    }
	    for ( mv=sf->subfonts[j]->metrics; mv!=NULL; mv = mnext ) {
		mnext = mv->next;
		GDrawDestroyWindow(mv->gw);
	    }
	}
    } else {
	for ( mv=sf->metrics; mv!=NULL; mv = mnext ) {
	    mnext = mv->next;
	    GDrawDestroyWindow(mv->gw);
	}
    }
    for ( bdf = sf->bitmaps; bdf!=NULL; bdf=bdf->next ) {
	for ( i=0; i<bdf->glyphcnt; ++i ) if ( bdf->glyphs[i]!=NULL ) {
	    BitmapView *bv, *next;
	    for ( bv = bdf->glyphs[i]->views; bv!=NULL; bv = next ) {
		next = bv->next;
		GDrawDestroyWindow(bv->gw);
	    }
	}
    }
    if ( fv->b.sf->fontinfo!=NULL )
	FontInfoDestroy(fv->b.sf);
    if ( fv->b.sf->valwin!=NULL )
	ValidationDestroy(fv->b.sf);
    SVDetachFV(fv);
}

static int SFAnyChanged(SplineFont *sf) {
    if ( sf->mm!=NULL ) {
	MMSet *mm = sf->mm;
	int i;
	if ( mm->changed )
return( true );
	for ( i=0; i<mm->instance_count; ++i )
	    if ( sf->mm->instances[i]->changed )
return( true );
	/* Changes to the blended font aren't real (for adobe fonts) */
	if ( mm->apple && mm->normal->changed )
return( true );

return( false );
    } else
return( sf->changed );
}

static int _FVMenuClose(FontView *fv) {
    int i;
    SplineFont *sf = fv->b.cidmaster?fv->b.cidmaster:fv->b.sf;

    if ( !SFCloseAllInstrs(fv->b.sf) )
return( false );

    if ( fv->b.nextsame!=NULL || fv->b.sf->fv!=&fv->b ) {
	/* There's another view, can close this one with no problems */
    } else if ( SFAnyChanged(sf) ) {
	i = AskChanged(fv->b.sf);
	if ( i==2 )	/* Cancel */
return( false );
	if ( i==0 && !_FVMenuSave(fv))		/* Save */
return(false);
	else
	    SFClearAutoSave(sf);		/* if they didn't save it, remove change record */
    }
    _FVCloseWindows(fv);
    if ( sf->filename!=NULL )
	RecentFilesRemember(sf->filename);
    else if ( sf->origname!=NULL )
	RecentFilesRemember(sf->origname);
    GDrawDestroyWindow(fv->gw);
return( true );
}

void MenuNew(GWindow UNUSED(gw), struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    FontNew();
}

void FVMenuClose(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    CharView* cv = tryObtainGDataCharView( gw );
    if( cv )
    {
	if ( cv->b.container )
	    (cv->b.container->funcs->doClose)(cv->b.container);
	else
	    GDrawDestroyWindow(gw);
    }
    else
    {
	FontView* fv = tryObtainGDataFontView( gw );
    

	if ( fv->b.container )
	    (fv->b.container->funcs->doClose)(fv->b.container);
	else
	    _FVMenuClose(fv);
    }
}

static void FV_ReattachCVs(SplineFont *old,SplineFont *new) {
    int i, j, pos;
    CharView *cv, *cvnext;
    SplineFont *sub;

    for ( i=0; i<old->glyphcnt; ++i ) {
	if ( old->glyphs[i]!=NULL && old->glyphs[i]->views!=NULL ) {
	    if ( new->subfontcnt==0 ) {
		pos = SFFindExistingSlot(new,old->glyphs[i]->unicodeenc,old->glyphs[i]->name);
		sub = new;
	    } else {
		pos = -1;
		for ( j=0; j<new->subfontcnt && pos==-1 ; ++j ) {
		    sub = new->subfonts[j];
		    pos = SFFindExistingSlot(sub,old->glyphs[i]->unicodeenc,old->glyphs[i]->name);
		}
	    }
	    if ( pos==-1 ) {
		for ( cv=(CharView *) (old->glyphs[i]->views); cv!=NULL; cv = cvnext ) {
		    cvnext = (CharView *) (cv->b.next);
		    GDrawDestroyWindow(cv->gw);
		}
	    } else {
		for ( cv=(CharView *) (old->glyphs[i]->views); cv!=NULL; cv = cvnext ) {
		    cvnext = (CharView *) (cv->b.next);
		    CVChangeSC(cv,sub->glyphs[pos]);
		    cv->b.layerheads[dm_grid] = &new->grid;
		}
	    }
	    GDrawProcessPendingEvents(NULL);		/* Don't want to many destroy_notify events clogging up the queue */
	}
    }
}


void FVMenuClearSpecialData(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
    FVClearSpecialData((FontViewBase *) fv);
}

void MenuPrefs(GWindow UNUSED(base), struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    DoPrefs();
}

void MenuXRes(GWindow UNUSED(base), struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    DoXRes();
}

void MenuSaveAll(GWindow UNUSED(base), struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    FontView *fv;

    for ( fv = fv_list; fv!=NULL; fv = (FontView *) (fv->b.next) ) {
	if ( SFAnyChanged(fv->b.sf) && !_FVMenuSave(fv))
return;
    }
}

static void _MenuExit(void *UNUSED(junk)) {

    FontView *fv, *next;

    if( collabclient_haveLocalServer() )
    {
	AskAndMaybeCloseLocalCollabServers();
    }
#ifndef _NO_PYTHON
    python_call_onClosingFunctions();
#endif

    LastFonts_Save();
    for ( fv = fv_list; fv!=NULL; fv = next )
    {
	next = (FontView *) (fv->b.next);
	if ( !_FVMenuClose(fv))
	    return;
	if ( fv->b.nextsame!=NULL || fv->b.sf->fv!=&fv->b )
	{
	    GDrawSync(NULL);
	    GDrawProcessPendingEvents(NULL);
	}
    }
    GDrawSync(NULL);
    GDrawProcessPendingEvents(NULL);
    exit(0);
}

void FVMenuExit(GWindow UNUSED(base), struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    _MenuExit(NULL);
}

void MenuExit(GWindow UNUSED(base), struct gmenuitem *UNUSED(mi), GEvent *e) {
    if ( e==NULL )	/* Not from the menu directly, but a shortcut */
	_MenuExit(NULL);
    else
	DelayEvent(_MenuExit,NULL);
}

char *GetPostScriptFontName(char *dir, int mult) {
    unichar_t *ret;
    char *u_dir;
    char *temp;

    u_dir = def2utf8_copy(dir);
    ret = FVOpenFont(_("Open Font"), u_dir,mult);
    temp = u2def_copy(ret);

    free(ret);
return( temp );
}

void MergeKernInfo(SplineFont *sf,EncMap *map) {
#ifndef __Mac
    static char wild[] = "*.{afm,tfm,ofm,pfm,bin,hqx,dfont,feature,feat,fea}";
    static char wild2[] = "*.{afm,amfm,tfm,ofm,pfm,bin,hqx,dfont,feature,feat,fea}";
#else
    static char wild[] = "*";	/* Mac resource files generally don't have extensions */
    static char wild2[] = "*";
#endif
    char *ret = gwwv_open_filename(_("Merge Feature Info"),NULL,
	    sf->mm!=NULL?wild2:wild,NULL);
    char *temp;

    if ( ret==NULL )
return;				/* Cancelled */
    temp = utf82def_copy(ret);

    if ( !LoadKerningDataFromMetricsFile(sf,temp,map))
	ff_post_error(_("Load of Kerning Metrics Failed"),_("Failed to load kern data from %s"), temp);
    free(ret); free(temp);
}

static void FVMenuMergeKern(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
    MergeKernInfo(fv->b.sf,fv->b.map);
}

void _FVMenuOpen(FontView *fv) {
    char *temp;
    char *eod, *fpt, *file, *full;
    FontView *test; int fvcnt, fvtest;

    char* OpenDir = NULL, *DefaultDir = NULL, *NewDir = NULL;
#if defined(__MINGW32__)
    DefaultDir = copy(GFileGetHomeDocumentsDir()); //Default value
    if (fv && fv->b.sf && fv->b.sf->filename) {
        free(DefaultDir);
        DefaultDir = GFileDirNameEx(fv->b.sf->filename, true);
    }
#endif

    for ( fvcnt=0, test=fv_list; test!=NULL; ++fvcnt, test=(FontView *) (test->b.next) );
    do {
        if (NewDir != NULL) {
            if (OpenDir != DefaultDir) {
                free(OpenDir);
            }
            
            OpenDir = NewDir;
            NewDir = NULL;
        } else if (OpenDir != DefaultDir) {
            free(OpenDir);
            OpenDir = DefaultDir;
        }
        
        temp = GetPostScriptFontName(OpenDir,true);
        if ( temp==NULL )
            return;

        //Make a copy of the folder; may be needed later if opening fails.
        NewDir = GFileDirName(temp);
        if (!GFileExists(NewDir)) {
            free(NewDir);
            NewDir = NULL;
        }

        eod = strrchr(temp,'/');
        if (eod != NULL) {
            *eod = '\0';
            file = eod+1;
            
            if (*file) {
                do {
                    fpt = strstr(file,"; ");
                    if ( fpt!=NULL ) *fpt = '\0';
                    full = malloc(strlen(temp)+1+strlen(file)+1);
                    strcpy(full,temp); strcat(full,"/"); strcat(full,file);
                    ViewPostScriptFont(full,0);
                    file = fpt+2;
                    free(full);
                } while ( fpt!=NULL );
            }
        }
        free(temp);
        for ( fvtest=0, test=fv_list; test!=NULL; ++fvtest, test=(FontView *) (test->b.next) );
    } while ( fvtest==fvcnt );	/* did the load fail for some reason? try again */
    
    free( NewDir );
    free( OpenDir );
    if (OpenDir != DefaultDir) {
        free( DefaultDir );
    }
}

void FVMenuOpen(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    FontView* fv = tryObtainGDataFontView( gw );
    _FVMenuOpen(fv);
}





void FVMenuImport(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    FontView* fv = tryObtainGDataFontView( gw );
    int empty = fv->b.sf->onlybitmaps && fv->b.sf->bitmaps==NULL;
    BDFFont *bdf;
    FVImport(fv);
    if ( empty && fv->b.sf->bitmaps!=NULL ) {
	for ( bdf= fv->b.sf->bitmaps; bdf->next!=NULL; bdf = bdf->next );
	FVChangeDisplayBitmap((FontViewBase *) fv,bdf);
    }
}

static int FVSelCount(FontView *fv) {
    int i, cnt=0;

    for ( i=0; i<fv->b.map->enccount; ++i )
	if ( fv->b.selected[i] ) ++cnt;
    if ( cnt>10 ) {
	char *buts[3];
	buts[0] = _("_OK");
	buts[1] = _("_Cancel");
	buts[2] = NULL;
	if ( gwwv_ask(_("Many Windows"),(const char **) buts,0,1,_("This involves opening more than 10 windows.\nIs that really what you want?"))==1 )
return( false );
    }
return( true );
}


static void sm_openWindowGlyph(CommonView* self) {
    FontView* fv = tryObtainCastFontView( self );
    int i;
    SplineChar *sc;

    if ( !FVSelCount(fv))
	return;
    if ( fv->b.container!=NULL && fv->b.container->funcs->is_modal )
	return;
    
    for ( i=0; i<fv->b.map->enccount; ++i )
	if ( fv->b.selected[i] ) {
	    sc = FVMakeChar(fv,i);
	    CharViewCreate(sc,fv,i);
	}
}

static void sm_openWindowBitmap(CommonView* self) {
    FontView* fv = tryObtainCastFontView( self );
    int i;
    SplineChar *sc;

    if ( fv->b.cidmaster==NULL ? (fv->b.sf->bitmaps==NULL) : (fv->b.cidmaster->bitmaps==NULL) )
return;
    if ( fv->b.container!=NULL && fv->b.container->funcs->is_modal )
return;
    if ( !FVSelCount(fv))
return;
    for ( i=0; i<fv->b.map->enccount; ++i )
	if ( fv->b.selected[i] ) {
	    sc = FVMakeChar(fv,i);
	    if ( sc!=NULL )
		BitmapViewCreatePick(i,fv);
	}
}

void _MenuWarnings(GWindow UNUSED(gw), struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    ShowErrorWindow();
}

static void sm_openWindowMetrics(CommonView* self) {
    FontView* fv = tryObtainCastFontView( self );
    if ( fv->b.container!=NULL && fv->b.container->funcs->is_modal )
return;
    MetricsViewCreate(fv,NULL,fv->filled==fv->show?NULL:fv->show);
}

void FVMenuPrint(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    FontView* fv = tryObtainGDataFontView( gw );
    if( !fv )
	return;
    
    if ( fv->b.container!=NULL && fv->b.container->funcs->is_modal )
return;
    PrintFFDlg(fv,NULL,NULL);
}

#if !defined(_NO_FFSCRIPT) || !defined(_NO_PYTHON)
static void FVMenuExecute(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);

    ScriptDlg(fv,NULL);
}
#endif

void FVMenuFontInfo(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    FontView* fv = tryObtainGDataFontView( gw );
    if ( fv->b.container!=NULL && fv->b.container->funcs->is_modal )
return;
    FontMenuFontInfo(fv);
}

void FVMenuMATHInfo(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    FontView* fv = tryObtainGDataFontView( gw );
    SFMathDlg(fv->b.sf,fv->b.active_layer);
}

void FVMenuFindProblems(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    FontView* fv = tryObtainGDataFontView( gw );
    FindProblems(fv,NULL,NULL);
}

void FVMenuValidate(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    FontView* fv = tryObtainGDataFontView( gw );
    SFValidationWindow(fv->b.sf,fv->b.active_layer,ff_none);
}

void FVMenuSetExtremumBound(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    FontView* fv = tryObtainGDataFontView( gw );
    char buffer[40], *end, *ret;
    int val;

    sprintf( buffer, "%d", fv->b.sf->extrema_bound<=0 ?
	    (int) rint((fv->b.sf->ascent+fv->b.sf->descent)/100.0) :
	    fv->b.sf->extrema_bound );
    ret = gwwv_ask_string(_("Extremum bound..."),buffer,_("Adobe says that \"big\" splines should not have extrema.\nBut they don't define what big means.\nIf the distance between the spline's end-points is bigger than this value, then the spline is \"big\" to fontforge."));
    if ( ret==NULL )
return;
    val = (int) rint(strtod(ret,&end));
    if ( *end!='\0' )
	ff_post_error( _("Bad Number"),_("Bad Number") );
    else {
	fv->b.sf->extrema_bound = val;
	if ( !fv->b.sf->changed ) {
	    fv->b.sf->changed = true;
	    FVSetTitles(fv->b.sf);
	}
    }
    free(ret);
}


static void sm_dialogEmbolden(CommonView* self) {
    FontView* fv = tryObtainCastFontView( self );
    EmboldenDlg(fv,NULL);
}
static void sm_dialogItalic(CommonView* self) {
    FontView* fv = tryObtainCastFontView( self );
    ItalicDlg(fv,NULL);
}

static void FVMenuSmallCaps(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
    GlyphChangeDlg(fv,NULL,gc_smallcaps);
}
static void sm_dialogXHeight(CommonView* self) {
    FontView* fv = tryObtainCastFontView( self );
    ChangeXHeightDlg(fv,NULL);
}

static void sm_dialogStemsCounters(CommonView* self) {
    FontView* fv = tryObtainCastFontView( self );
    GlyphChangeDlg(fv,NULL,gc_generic);
}

static void FVMenuSubSup(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
    GlyphChangeDlg(fv,NULL,gc_subsuper);
    /*AddSubSupDlg(fv);*/
}


static void sm_dialogOblique(CommonView* self) {
    FontView* fv = tryObtainCastFontView( self );
    ObliqueDlg(fv,NULL);
}

static void sm_dialogCondenseExtend(CommonView* self) {
    FontView* fv = tryObtainCastFontView( self );
    CondenseExtendDlg(fv,NULL);
}


/* returns -1 if nothing selected, if exactly one char return it, -2 if more than one */
int FVAnyCharSelected(FontView *fv) {
    int i, val=-1;
    if( !fv )
	return -1;
    
    for ( i=0; i<fv->b.map->enccount; ++i ) {
	if ( fv->b.selected[i]) {
	    if ( val==-1 )
		val = i;
	    else
return( -2 );
	}
    }
return( val );
}

static int FVAllSelected(FontView *fv) {
    int i, any = false;
    /* Is everything real selected? */

    for ( i=0; i<fv->b.sf->glyphcnt; ++i ) if ( SCWorthOutputting(fv->b.sf->glyphs[i])) {
	if ( !fv->b.selected[fv->b.map->backmap[i]] )
return( false );
	any = true;
    }
return( any );
}

void FVMenuCopyFrom(GWindow UNUSED(gw), struct gmenuitem *mi, GEvent *UNUSED(e)) {
    /*FontView *fv = (FontView *) GDrawGetUserData(gw);*/

    if ( mi->mid==SMID_CharName )
	copymetadata = !copymetadata;
    else if ( mi->mid==SMID_TTFInstr )
	copyttfinstr = !copyttfinstr;
    else
	onlycopydisplayed = (mi->mid==SMID_DisplayedFont);
    SavePrefs(true);
}


static void sm_copy(CommonView* self) {
    FontView* fv = tryObtainCastFontView( self );
    if ( FVAnyCharSelected(fv)==-1 )
	return;
    FVCopy((FontViewBase *) fv,ct_fullcopy);
}

static void sm_copyLookupData(CommonView* self) {
    FontView* fv = tryObtainCastFontView( self );
    if ( FVAnyCharSelected(fv)==-1 )
	return;
    FVCopy((FontViewBase *) fv,ct_lookups);
}

static void sm_copyRef(CommonView* self) {
    FontView* fv = tryObtainCastFontView( self );
    if ( FVAnyCharSelected(fv)==-1 )
	return;
    FVCopy((FontViewBase *) fv,ct_reference);
}

static void sm_copyWidth( CommonView* self, enum undotype undotype )
{
    FontView* fv = tryObtainCastFontView( self );
    if ( FVAnyCharSelected(fv)==-1 )
	return;
    FVCopyWidth((FontViewBase *) fv, undotype );
    
}


//enum undotype undotype
void FVMenuCopyWidth(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e)) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);

    if ( FVAnyCharSelected(fv)==-1 )
return;
    if ( mi->mid==SMID_CopyVWidth && !fv->b.sf->hasvmetrics )
return;
    FVCopyWidth((FontViewBase *) fv,
		   mi->mid==SMID_CopyWidth?ut_width:
		   mi->mid==SMID_CopyVWidth?ut_vwidth:
		   mi->mid==SMID_CopyLBearing?ut_lbearing:
					 ut_rbearing);
}


static void sm_paste(CommonView* self) {
    FontView* fv = tryObtainCastFontView( self );
    if ( FVAnyCharSelected(fv)==-1 )
	return;
    PasteIntoFV((FontViewBase *) fv,false,NULL);
}

static void sm_pasteInto(CommonView* self) {
    FontView* fv = tryObtainCastFontView( self );
    if ( FVAnyCharSelected(fv)==-1 )
	return;
    PasteIntoFV((FontViewBase *) fv,true,NULL);
}

static void sm_pasteAfter(CommonView* self) {
    FontView* fv = tryObtainCastFontView( self );
    int pos = FVAnyCharSelected(fv);
    if ( pos<0 )
	return;
    PasteIntoFV(&fv->b,2,NULL);
}

static void FVMenuSameGlyphAs(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
    FVSameGlyphAs((FontViewBase *) fv);
    GDrawRequestExpose(fv->v,NULL,false);
}

static void sm_copyFgBg(CommonView* self) {
    FontView* fv = tryObtainCastFontView( self );
    FVCopyFgtoBg( (FontViewBase *) fv );
}

static void sm_copyL2L(CommonView* self) {
    FontView* fv = tryObtainCastFontView( self );
    FVCopyLayerToLayer( fv );
}

static void sm_dialogCompareLayers(CommonView* self) {
    FontView* fv = tryObtainCastFontView( self );
    FVCompareLayerToLayer( fv );
}
void FVMenuCompareL2L(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
    FVCompareLayerToLayer( fv );
}


static void sm_delete(CommonView* self) {
    FontView* fv = tryObtainCastFontView( self );
    FVClear( (FontViewBase *) fv );
}

static void sm_clear(CommonView* self) {
    FontView* fv = tryObtainCastFontView( self );
    FVClear( (FontViewBase *) fv );
}

static void sm_clearBackground(CommonView* self) {
    FontView* fv = tryObtainCastFontView( self );
    FVClearBackground( (FontViewBase *) fv );
}

static void sm_join(CommonView* self) {
    FontView* fv = tryObtainCastFontView( self );
    FVJoin( (FontViewBase *) fv );
}


static void sm_referenceUnlink(CommonView* self) {
    FontView* fv = tryObtainCastFontView( self );
    FVUnlinkRef( (FontViewBase *) fv );
}

static void sm_removeUndoes(CommonView* self) {
    FontView* fv = tryObtainCastFontView( self );
    SFRemoveUndoes(fv->b.sf,fv->b.selected,fv->b.map);
}

static void sm_undo(CommonView* self) {
    FontView* fv = tryObtainCastFontView( self );
    FVUndo((FontViewBase *) fv);
}

static void sm_redo(CommonView* self) {
    FontView* fv = tryObtainCastFontView( self );
    FVRedo((FontViewBase *) fv);
}

static void sm_undoFontLevel(CommonView* self) {
    FontView* fv = tryObtainCastFontView( self );
    FontViewBase * fvb = (FontViewBase *) fv;
    SplineFont *sf = fvb->sf;

    if( !sf->undoes )
	return;

    struct sfundoes *undo = sf->undoes;
    printf("font level undo msg:%s\n", undo->msg );
    SFUndoPerform( undo, sf );
    SFUndoRemoveAndFree( sf, undo );
}

static void sm_cut(CommonView* self) {
    FontView* fv = tryObtainCastFontView( self );
    FVCopy(&fv->b,ct_fullcopy);
    FVClear(&fv->b);
}



void FVMenuDeselectAll(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);

    FVDeselectAll(fv);
}

    /* Index array by merge_type(*4) + selection*2 + doit */
const uint8 mergefunc[] = {
/* mt_set */
	0, 1,
	0, 1,
/* mt_merge */
	0, 1,
	1, 1,
/* mt_restrict */
	0, 0,
	1, 0,
/* mt_and */
	0, 0,
	0, 1,
};

enum merge_type SelMergeType(GEvent *e) {
    if ( e->type!=et_mouseup )
return( mt_set );

return( ((e->u.mouse.state&ksm_shift)?mt_merge:0) |
	((e->u.mouse.state&ksm_control)?mt_restrict:0) );
}

static char *SubMatch(char *pattern, char *eop, char *name,int ignorecase) {
    char ch, *ppt, *npt, *ept, *eon;

    while ( pattern<eop && ( ch = *pattern)!='\0' ) {
	if ( ch=='*' ) {
	    if ( pattern[1]=='\0' )
return( name+strlen(name));
	    for ( npt=name; ; ++npt ) {
		if ( (eon = SubMatch(pattern+1,eop,npt,ignorecase))!= NULL )
return( eon );
		if ( *npt=='\0' )
return( NULL );
	    }
	} else if ( ch=='?' ) {
	    if ( *name=='\0' )
return( NULL );
	    ++name;
	} else if ( ch=='[' ) {
	    /* [<char>...] matches the chars
	     * [<char>-<char>...] matches any char within the range (inclusive)
	     * the above may be concattenated and the resultant pattern matches
	     *		anything thing which matches any of them.
	     * [^<char>...] matches any char which does not match the rest of
	     *		the pattern
	     * []...] as a special case a ']' immediately after the '[' matches
	     *		itself and does not end the pattern
	     */
	    int found = 0, not=0;
	    ++pattern;
	    if ( pattern[0]=='^' ) { not = 1; ++pattern; }
	    for ( ppt = pattern; (ppt!=pattern || *ppt!=']') && *ppt!='\0' ; ++ppt ) {
		ch = *ppt;
		if ( ppt[1]=='-' && ppt[2]!=']' && ppt[2]!='\0' ) {
		    char ch2 = ppt[2];
		    if ( (*name>=ch && *name<=ch2) ||
			    (ignorecase && islower(ch) && islower(ch2) &&
				    *name>=toupper(ch) && *name<=toupper(ch2)) ||
			    (ignorecase && isupper(ch) && isupper(ch2) &&
				    *name>=tolower(ch) && *name<=tolower(ch2))) {
			if ( !not ) {
			    found = 1;
	    break;
			}
		    } else {
			if ( not ) {
			    found = 1;
	    break;
			}
		    }
		    ppt += 2;
		} else if ( ch==*name || (ignorecase && tolower(ch)==tolower(*name)) ) {
		    if ( !not ) {
			found = 1;
	    break;
		    }
		} else {
		    if ( not ) {
			found = 1;
	    break;
		    }
		}
	    }
	    if ( !found )
return( NULL );
	    while ( *ppt!=']' && *ppt!='\0' ) ++ppt;
	    pattern = ppt;
	    ++name;
	} else if ( ch=='{' ) {
	    /* matches any of a comma separated list of substrings */
	    for ( ppt = pattern+1; *ppt!='\0' ; ppt = ept ) {
		for ( ept=ppt; *ept!='}' && *ept!=',' && *ept!='\0'; ++ept );
		npt = SubMatch(ppt,ept,name,ignorecase);
		if ( npt!=NULL ) {
		    char *ecurly = ept;
		    while ( *ecurly!='}' && ecurly<eop && *ecurly!='\0' ) ++ecurly;
		    if ( (eon=SubMatch(ecurly+1,eop,npt,ignorecase))!=NULL )
return( eon );
		}
		if ( *ept=='}' )
return( NULL );
		if ( *ept==',' ) ++ept;
	    }
	} else if ( ch==*name ) {
	    ++name;
	} else if ( ignorecase && tolower(ch)==tolower(*name)) {
	    ++name;
	} else
return( NULL );
	++pattern;
    }
return( name );
}

/* Handles *?{}[] wildcards */
static int WildMatch(char *pattern, char *name,int ignorecase) {
    char *eop = pattern + strlen(pattern);

    if ( pattern==NULL )
return( true );

    name = SubMatch(pattern,eop,name,ignorecase);
    if ( name==NULL )
return( false );
    if ( *name=='\0' )
return( true );

return( false );
}

static int SS_ScriptChanged(GGadget *g, GEvent *e) {

    if ( e->type==et_controlevent && e->u.control.subtype != et_textfocuschanged ) {
	char *txt = GGadgetGetTitle8(g);
	char buf[8];
	int i;
	extern GTextInfo scripts[];

	for ( i=0; scripts[i].text!=NULL; ++i ) {
	    if ( strcmp((char *) scripts[i].text,txt)==0 )
	break;
	}
	free(txt);
	if ( scripts[i].text==NULL )
return( true );
	buf[0] = ((intpt) scripts[i].userdata)>>24;
	buf[1] = ((intpt) scripts[i].userdata)>>16;
	buf[2] = ((intpt) scripts[i].userdata)>>8 ;
	buf[3] = ((intpt) scripts[i].userdata)    ;
	buf[4] = 0;
	GGadgetSetTitle8(g,buf);
    }
return( true );
}

static int SS_OK(GGadget *g, GEvent *e) {

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	int *done = GDrawGetUserData(GGadgetGetWindow(g));
	*done = 2;
    }
return( true );
}

static int SS_Cancel(GGadget *g, GEvent *e) {

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	int *done = GDrawGetUserData(GGadgetGetWindow(g));
	*done = true;
    }
return( true );
}

static int ss_e_h(GWindow gw, GEvent *event) {
    int *done = GDrawGetUserData(gw);

    switch ( event->type ) {
      case et_char:
return( false );
      case et_close:
	*done = true;
      break;
    }
return( true );
}

static void FVSelectByScript(FontView *fv,int merge) {
    int j, gid;
    SplineChar *sc;
    EncMap *map = fv->b.map;
    SplineFont *sf = fv->b.sf;
    extern GTextInfo scripts[];
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[10], *hvarray[21][2], *barray[8], boxes[3];
    GTextInfo label[10];
    int i,k;
    int done = 0, doit;
    char tagbuf[4];
    uint32 tag;
    const unichar_t *ret;
    int lc_k, uc_k, select_k;
    int only_uc=0, only_lc=0;

    LookupUIInit();

    memset(&wattrs,0,sizeof(wattrs));
    memset(&gcd,0,sizeof(gcd));
    memset(&label,0,sizeof(label));
    memset(&boxes,0,sizeof(boxes));

    wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_isdlg|wam_restrict;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.restrict_input_to_me = false;
    wattrs.is_dlg = 1;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.utf8_window_title = _("Select by Script");
    wattrs.is_dlg = true;
    pos.x = pos.y = 0;
    pos.width = 100;
    pos.height = 100;
    gw = GDrawCreateTopWindow(NULL,&pos,ss_e_h,&done,&wattrs);

    k = i = 0;

    gcd[k].gd.flags = gg_visible|gg_enabled ;
    gcd[k].gd.u.list = scripts;
    gcd[k].gd.handle_controlevent = SS_ScriptChanged;
    gcd[k++].creator = GListFieldCreate;
    hvarray[i][0] = &gcd[k-1]; hvarray[i++][1] = NULL;

    label[k].text = (unichar_t *) _("All glyphs");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_enabled|gg_visible|gg_utf8_popup|gg_cb_on;
    gcd[k].gd.popup_msg = (unichar_t *) _("Set the selection of the font view to all glyphs in the script.");
    gcd[k++].creator = GRadioCreate;
    hvarray[i][0] = &gcd[k-1]; hvarray[i++][1] = NULL;
    hvarray[i][0] = GCD_HPad10; hvarray[i++][1] = NULL;

    uc_k = k;
    label[k].text = (unichar_t *) _("Only upper case");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_enabled|gg_visible|gg_utf8_popup;
    gcd[k].gd.popup_msg = (unichar_t *) _("Set the selection of the font view to any upper case glyphs in the script.");
    gcd[k++].creator = GRadioCreate;
    hvarray[i][0] = &gcd[k-1]; hvarray[i++][1] = NULL;

    lc_k = k;
    label[k].text = (unichar_t *) _("Only lower case");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_enabled|gg_visible|gg_utf8_popup;
    gcd[k].gd.popup_msg = (unichar_t *) _("Set the selection of the font view to any lower case glyphs in the script.");
    gcd[k++].creator = GRadioCreate;
    hvarray[i][0] = &gcd[k-1]; hvarray[i++][1] = NULL;
    hvarray[i][0] = GCD_HPad10; hvarray[i++][1] = NULL;

    select_k = k;
    label[k].text = (unichar_t *) _("Select Results");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_enabled|gg_visible|gg_utf8_popup|gg_rad_startnew;
    gcd[k].gd.popup_msg = (unichar_t *) _("Set the selection of the font view to the glyphs\nwhich match");
    gcd[k++].creator = GRadioCreate;
    hvarray[i][0] = &gcd[k-1]; hvarray[i++][1] = NULL;

    label[k].text = (unichar_t *) _("Merge Results");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_enabled|gg_visible|gg_utf8_popup;
    gcd[k].gd.popup_msg = (unichar_t *) _("Expand the selection of the font view to include\nall the glyphs which match");
    gcd[k++].creator = GRadioCreate;
    hvarray[i][0] = &gcd[k-1]; hvarray[i++][1] = NULL;

    label[k].text = (unichar_t *) _("Restrict Selection");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_enabled|gg_visible|gg_utf8_popup;
    gcd[k].gd.popup_msg = (unichar_t *) _("Remove matching glyphs from the selection." );
    gcd[k++].creator = GRadioCreate;
    hvarray[i][0] = &gcd[k-1]; hvarray[i++][1] = NULL;

    label[k].text = (unichar_t *) _("Logical And with Selection");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_enabled|gg_visible|gg_utf8_popup;
    gcd[k].gd.popup_msg = (unichar_t *) _("Remove glyphs which do not match from the selection." );
    gcd[k++].creator = GRadioCreate;
    hvarray[i][0] = &gcd[k-1]; hvarray[i++][1] = NULL;
    gcd[k-4 + merge/4].gd.flags |= gg_cb_on;

    hvarray[i][0] = GCD_Glue; hvarray[i++][1] = NULL;

    label[k].text = (unichar_t *) _("_OK");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_visible|gg_enabled | gg_but_default;
    gcd[k].gd.handle_controlevent = SS_OK;
    gcd[k++].creator = GButtonCreate;

    label[k].text = (unichar_t *) _("_Cancel");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_visible|gg_enabled | gg_but_cancel;
    gcd[k].gd.handle_controlevent = SS_Cancel;
    gcd[k++].creator = GButtonCreate;

    barray[0] = barray[2] = barray[3] = barray[4] = barray[6] = GCD_Glue; barray[7] = NULL;
    barray[1] = &gcd[k-2]; barray[5] = &gcd[k-1];
    hvarray[i][0] = &boxes[2]; hvarray[i++][1] = NULL;
    hvarray[i][0] = NULL;

    memset(boxes,0,sizeof(boxes));
    boxes[0].gd.pos.x = boxes[0].gd.pos.y = 2;
    boxes[0].gd.flags = gg_enabled|gg_visible;
    boxes[0].gd.u.boxelements = hvarray[0];
    boxes[0].creator = GHVGroupCreate;

    boxes[2].gd.flags = gg_enabled|gg_visible;
    boxes[2].gd.u.boxelements = barray;
    boxes[2].creator = GHBoxCreate;

    GGadgetsCreate(gw,boxes);
    GHVBoxSetExpandableCol(boxes[2].ret,gb_expandgluesame);
    GHVBoxSetExpandableRow(boxes[0].ret,gb_expandglue);


    GHVBoxFitWindow(boxes[0].ret);

    GDrawSetVisible(gw,true);
    ret = NULL;
    while ( !done ) {
	GDrawProcessOneEvent(NULL);
	if ( done==2 ) {
	    ret = _GGadgetGetTitle(gcd[0].ret);
	    if ( *ret=='\0' ) {
		ff_post_error(_("No Script"),_("Please specify a script"));
		done = 0;
	    } else if ( u_strlen(ret)>4 ) {
		ff_post_error(_("Bad Script"),_("Scripts are 4 letter tags"));
		done = 0;
	    }
	}
    }
    memset(tagbuf,' ',4);
    if ( done==2 && ret!=NULL ) {
	tagbuf[0] = *ret;
	if ( ret[1]!='\0' ) {
	    tagbuf[1] = ret[1];
	    if ( ret[2]!='\0' ) {
		tagbuf[2] = ret[2];
		if ( ret[3]!='\0' )
		    tagbuf[3] = ret[3];
	    }
	}
    }
    merge = GGadgetIsChecked(gcd[select_k+0].ret) ? mt_set :
	    GGadgetIsChecked(gcd[select_k+1].ret) ? mt_merge :
	    GGadgetIsChecked(gcd[select_k+2].ret) ? mt_restrict :
						    mt_and;
    only_uc = GGadgetIsChecked(gcd[uc_k+0].ret);
    only_lc = GGadgetIsChecked(gcd[lc_k+0].ret);

    GDrawDestroyWindow(gw);
    if ( done==1 )
return;
    tag = (tagbuf[0]<<24) | (tagbuf[1]<<16) | (tagbuf[2]<<8) | tagbuf[3];

    for ( j=0; j<map->enccount; ++j ) if ( (gid=map->map[j])!=-1 && (sc=sf->glyphs[gid])!=NULL ) {
	doit = ( SCScriptFromUnicode(sc)==tag );
	if ( doit ) {
	    if ( only_uc && (sc->unicodeenc==-1 || sc->unicodeenc>0xffff ||
		    !isupper(sc->unicodeenc)) )
		doit = false;
	    else if ( only_lc && (sc->unicodeenc==-1 || sc->unicodeenc>0xffff ||
		    !islower(sc->unicodeenc)) )
		doit = false;
	}
	fv->b.selected[j] = mergefunc[ merge + (fv->b.selected[j]?2:0) + doit ];
    } else if ( merge==mt_set )
	fv->b.selected[j] = false;

    GDrawRequestExpose(fv->v,NULL,false);
}

static void sm_selectByScript( CommonView* self, int merge) {
    FontView* fv = tryObtainCastFontView( self );
    FVSelectByScript(fv,merge);
}

static void FVSelectColor(FontView *fv, uint32 col, int merge) {
    int i, doit;
    uint32 sccol;
    SplineChar **glyphs = fv->b.sf->glyphs;

    for ( i=0; i<fv->b.map->enccount; ++i ) {
	int gid = fv->b.map->map[i];
	sccol =  ( gid==-1 || glyphs[gid]==NULL ) ? COLOR_DEFAULT : glyphs[gid]->color;
	doit = sccol==col;
	fv->b.selected[i] = mergefunc[ merge + (fv->b.selected[i]?2:0) + doit ];
    }
    GDrawRequestExpose(fv->v,NULL,false);
}

static void sm_selectbyColor( CommonView* self, int merge, Color col ) {
    FontView* fv = tryObtainCastFontView( self );
    if ( (intpt) col == (intpt) -10 ) {
	struct hslrgb retcol, font_cols[6];
	retcol = GWidgetColor(_("Pick a color"),NULL,SFFontCols(fv->b.sf,font_cols));
	if ( !retcol.rgb )
return;
	col = (((int) rint(255.*retcol.r))<<16 ) |
		    (((int) rint(255.*retcol.g))<<8 ) |
		    (((int) rint(255.*retcol.b)) );
    }
    FVSelectColor(fv,col,merge);
}

int FVSelectByName(FontView *fv, char *ret, int merge) {
    int j, gid, doit;
    char *end;
    SplineChar *sc;
    EncMap *map = fv->b.map;
    SplineFont *sf = fv->b.sf;
    struct altuni *alt;

    if ( !merge )
	FVDeselectAll(fv);
    if (( *ret=='0' && ( ret[1]=='x' || ret[1]=='X' )) ||
	    ((*ret=='u' || *ret=='U') && ret[1]=='+' )) {
	int uni = (int) strtol(ret+2,&end,16);
	int vs= -2;
	if ( *end=='.' ) {
	    ++end;
	    if (( *end=='0' && ( end[1]=='x' || end[1]=='X' )) ||
		    ((*end=='u' || *end=='U') && end[1]=='+' ))
		end += 2;
	    vs = (int) strtoul(end,&end,16);
	}
	if ( *end!='\0' || uni<0 || uni>=0x110000 ) {
	    ff_post_error( _("Bad Number"),_("Bad Number") );
return( false );
	}
	for ( j=0; j<map->enccount; ++j ) if ( (gid=map->map[j])!=-1 && (sc=sf->glyphs[gid])!=NULL ) {
	    if ( vs==-2 ) {
		for ( alt=sc->altuni; alt!=NULL && (alt->unienc!=uni || alt->fid!=0); alt=alt->next );
	    } else {
		for ( alt=sc->altuni; alt!=NULL && (alt->unienc!=uni || alt->vs!=vs || alt->fid!=0); alt=alt->next );
	    }
	    doit = (sc->unicodeenc == uni && vs<0) || alt!=NULL;
	    fv->b.selected[j] = mergefunc[ merge + (fv->b.selected[j]?2:0) + doit ];
	} else if ( merge==mt_set )
	    fv->b.selected[j] = false;
    } else {
	for ( j=0; j<map->enccount; ++j ) if ( (gid=map->map[j])!=-1 && (sc=sf->glyphs[gid])!=NULL ) {
	    doit = WildMatch(ret,sc->name,false);
	    fv->b.selected[j] = mergefunc[ merge + (fv->b.selected[j]?2:0) + doit ];
	} else if ( merge==mt_set )
	    fv->b.selected[j] = false;
    }
    GDrawRequestExpose(fv->v,NULL,false);
    fv->sel_index = 1;
return( true );
}

static void sm_SelectByName( CommonView* self, int merge )
{
    FontView* fv = tryObtainCastFontView( self );
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[8], *hvarray[12][2], *barray[8], boxes[3];
    GTextInfo label[8];
    int done=0,k,i;

    memset(&wattrs,0,sizeof(wattrs));
    memset(&gcd,0,sizeof(gcd));
    memset(&label,0,sizeof(label));
    memset(&boxes,0,sizeof(boxes));

    wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_isdlg|wam_restrict;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.restrict_input_to_me = false;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.utf8_window_title = _("Select by Name");
    wattrs.is_dlg = false;
    pos.x = pos.y = 0;
    pos.width = 100;
    pos.height = 100;
    gw = GDrawCreateTopWindow(NULL,&pos,ss_e_h,&done,&wattrs);

    k = i = 0;

    label[k].text = (unichar_t *) _("Enter either a wildcard pattern (to match glyph names)\n or a unicode encoding like \"U+0065\".");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
    gcd[k].gd.popup_msg = (unichar_t *) _(
	"Unix style wildcarding is accepted:\n"
	"Most characters match themselves\n"
	"A \"?\" will match any single character\n"
	"A \"*\" will match an arbitrary number of characters (including none)\n"
	"An \"[abd]\" set of characters within square brackets will match any (single) character\n"
	"A \"{scmp,c2sc}\" set of strings within curly brackets will match any string\n"
	"So \"a.*\" would match \"a.\" or \"a.sc\" or \"a.swash\"\n"
	"While \"a.{scmp,c2sc}\" would match \"a.scmp\" or \"a.c2sc\"\n"
	"And \"a.[abd]\" would match \"a.a\" or \"a.b\" or \"a.d\"");
    gcd[k++].creator = GLabelCreate;
    hvarray[i][0] = &gcd[k-1]; hvarray[i++][1] = NULL;

    gcd[k].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
    gcd[k].gd.popup_msg = gcd[k-1].gd.popup_msg;
    gcd[k++].creator = GTextFieldCreate;
    hvarray[i][0] = &gcd[k-1]; hvarray[i++][1] = NULL;

    label[k].text = (unichar_t *) _("Select Results");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_enabled|gg_visible|gg_utf8_popup;
    gcd[k].gd.popup_msg = (unichar_t *) _("Set the selection of the font view to the glyphs\nwhich match");
    gcd[k++].creator = GRadioCreate;
    hvarray[i][0] = &gcd[k-1]; hvarray[i++][1] = NULL;

    label[k].text = (unichar_t *) _("Merge Results");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_enabled|gg_visible|gg_utf8_popup;
    gcd[k].gd.popup_msg = (unichar_t *) _("Expand the selection of the font view to include\nall the glyphs which match");
    gcd[k++].creator = GRadioCreate;
    hvarray[i][0] = &gcd[k-1]; hvarray[i++][1] = NULL;

    label[k].text = (unichar_t *) _("Restrict Selection");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_enabled|gg_visible|gg_utf8_popup;
    gcd[k].gd.popup_msg = (unichar_t *) _("Remove matching glyphs from the selection." );
    gcd[k++].creator = GRadioCreate;
    hvarray[i][0] = &gcd[k-1]; hvarray[i++][1] = NULL;

    label[k].text = (unichar_t *) _("Logical And with Selection");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_enabled|gg_visible|gg_utf8_popup;
    gcd[k].gd.popup_msg = (unichar_t *) _("Remove glyphs which do not match from the selection." );
    gcd[k++].creator = GRadioCreate;
    hvarray[i][0] = &gcd[k-1]; hvarray[i++][1] = NULL;
    gcd[k-4 + merge/4].gd.flags |= gg_cb_on;

    hvarray[i][0] = GCD_Glue; hvarray[i++][1] = NULL;

    label[k].text = (unichar_t *) _("_OK");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_visible|gg_enabled | gg_but_default;
    gcd[k].gd.handle_controlevent = SS_OK;
    gcd[k++].creator = GButtonCreate;

    label[k].text = (unichar_t *) _("_Cancel");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_visible|gg_enabled | gg_but_cancel;
    gcd[k].gd.handle_controlevent = SS_Cancel;
    gcd[k++].creator = GButtonCreate;

    barray[0] = barray[2] = barray[3] = barray[4] = barray[6] = GCD_Glue; barray[7] = NULL;
    barray[1] = &gcd[k-2]; barray[5] = &gcd[k-1];
    hvarray[i][0] = &boxes[2]; hvarray[i++][1] = NULL;
    hvarray[i][0] = NULL;

    memset(boxes,0,sizeof(boxes));
    boxes[0].gd.pos.x = boxes[0].gd.pos.y = 2;
    boxes[0].gd.flags = gg_enabled|gg_visible;
    boxes[0].gd.u.boxelements = hvarray[0];
    boxes[0].creator = GHVGroupCreate;

    boxes[2].gd.flags = gg_enabled|gg_visible;
    boxes[2].gd.u.boxelements = barray;
    boxes[2].creator = GHBoxCreate;

    GGadgetsCreate(gw,boxes);
    GHVBoxSetExpandableCol(boxes[2].ret,gb_expandgluesame);
    GHVBoxSetExpandableRow(boxes[0].ret,gb_expandglue);


    GHVBoxFitWindow(boxes[0].ret);

    GDrawSetVisible(gw,true);
    while ( !done ) {
	GDrawProcessOneEvent(NULL);
	if ( done==2 ) {
	    char *str = GGadgetGetTitle8(gcd[1].ret);
	    int merge = GGadgetIsChecked(gcd[2].ret) ? mt_set :
			GGadgetIsChecked(gcd[3].ret) ? mt_merge :
			GGadgetIsChecked(gcd[4].ret) ? mt_restrict :
						       mt_and;
	    int ret = FVSelectByName(fv,str,merge);
	    free(str);
	    if ( !ret )
		done = 0;
	}
    }
    GDrawDestroyWindow(gw);
}



static void sm_SelectWorthOutputting(CommonView* self, int merge ) 
{
    FontView* fv = tryObtainCastFontView( self );
    int i, gid, doit;
    EncMap *map = fv->b.map;
    SplineFont *sf = fv->b.sf;

    for ( i=0; i< map->enccount; ++i ) {
	doit = ( (gid=map->map[i])!=-1 && sf->glyphs[gid]!=NULL &&
		SCWorthOutputting(sf->glyphs[gid]) );
	fv->b.selected[i] = mergefunc[ merge + (fv->b.selected[i]?2:0) + doit ];
    }
    GDrawRequestExpose(fv->v,NULL,false);
}

static void sm_glyphsRefs(CommonView* self, int merge ) {
    FontView* fv = tryObtainCastFontView( self );
    int i, gid, doit;
    EncMap *map = fv->b.map;
    SplineFont *sf = fv->b.sf;
    int layer = fv->b.active_layer;

    for ( i=0; i< map->enccount; ++i ) {
	doit = ( (gid=map->map[i])!=-1 && sf->glyphs[gid]!=NULL &&
		sf->glyphs[gid]->layers[layer].refs!=NULL &&
		sf->glyphs[gid]->layers[layer].splines==NULL );
	fv->b.selected[i] = mergefunc[ merge + (fv->b.selected[i]?2:0) + doit ];
    }
    GDrawRequestExpose(fv->v,NULL,false);
}

static void sm_glyphsSplines(CommonView* self, int merge ) {
    FontView* fv = tryObtainCastFontView( self );
    int i, gid, doit;
    EncMap *map = fv->b.map;
    SplineFont *sf = fv->b.sf;
    int layer = fv->b.active_layer;

    for ( i=0; i< map->enccount; ++i ) {
	doit = ( (gid=map->map[i])!=-1 && sf->glyphs[gid]!=NULL &&
		sf->glyphs[gid]->layers[layer].refs==NULL &&
		sf->glyphs[gid]->layers[layer].splines!=NULL );
	fv->b.selected[i] = mergefunc[ merge + (fv->b.selected[i]?2:0) + doit ];
    }
    GDrawRequestExpose(fv->v,NULL,false);
}

static void sm_glyphsBoth(CommonView* self, int merge ) {
    FontView* fv = tryObtainCastFontView( self );
    int i, gid, doit;
    EncMap *map = fv->b.map;
    SplineFont *sf = fv->b.sf;
    int layer = fv->b.active_layer;

    for ( i=0; i< map->enccount; ++i ) {
	doit = ( (gid=map->map[i])!=-1 && sf->glyphs[gid]!=NULL &&
		sf->glyphs[gid]->layers[layer].refs!=NULL &&
		sf->glyphs[gid]->layers[layer].splines!=NULL );
	fv->b.selected[i] = mergefunc[ merge + (fv->b.selected[i]?2:0) + doit ];
    }
    GDrawRequestExpose(fv->v,NULL,false);
}

static void sm_glyphsWhite(CommonView* self, int merge ) {
    FontView* fv = tryObtainCastFontView( self );
    int i, gid, doit;
    EncMap *map = fv->b.map;
    SplineFont *sf = fv->b.sf;
    int layer = fv->b.active_layer;

    for ( i=0; i< map->enccount; ++i ) {
	doit = ( (gid=map->map[i])!=-1 && sf->glyphs[gid]!=NULL &&
		sf->glyphs[gid]->layers[layer].refs==NULL &&
		sf->glyphs[gid]->layers[layer].splines==NULL );
	fv->b.selected[i] = mergefunc[ merge + (fv->b.selected[i]?2:0) + doit ];
    }
    GDrawRequestExpose(fv->v,NULL,false);
}

static void sm_selectChanged(CommonView* self, int merge ) {
    FontView* fv = tryObtainCastFontView( self );
    int i, gid, doit;
    EncMap *map = fv->b.map;
    SplineFont *sf = fv->b.sf;

    for ( i=0; i< map->enccount; ++i ) {
	doit = ( (gid=map->map[i])!=-1 && sf->glyphs[gid]!=NULL && sf->glyphs[gid]->changed );
	fv->b.selected[i] = mergefunc[ merge + (fv->b.selected[i]?2:0) + doit ];
    }

    GDrawRequestExpose(fv->v,NULL,false);
}

static void sm_selectHintingNeeded(CommonView* self, int merge ) {
    FontView* fv = tryObtainCastFontView( self );
    int i, gid, doit;
    EncMap *map = fv->b.map;
    SplineFont *sf = fv->b.sf;
    int order2 = sf->layers[fv->b.active_layer].order2;

    for ( i=0; i< map->enccount; ++i ) {
	doit = ( (gid=map->map[i])!=-1 && sf->glyphs[gid]!=NULL &&
		((!order2 && sf->glyphs[gid]->changedsincelasthinted ) ||
		 ( order2 && sf->glyphs[gid]->layers[fv->b.active_layer].splines!=NULL &&
		     sf->glyphs[gid]->ttf_instrs_len<=0 ) ||
		 ( order2 && sf->glyphs[gid]->instructions_out_of_date )) );
	fv->b.selected[i] = mergefunc[ merge + (fv->b.selected[i]?2:0) + doit ];
    }
    GDrawRequestExpose(fv->v,NULL,false);
}

static void sm_selectAutohintable(CommonView* self, int merge ) {
    FontView* fv = tryObtainCastFontView( self );
    int i, gid, doit;
    EncMap *map = fv->b.map;
    SplineFont *sf = fv->b.sf;

    for ( i=0; i< map->enccount; ++i ) {
	doit = (gid=map->map[i])!=-1 && sf->glyphs[gid]!=NULL &&
		!sf->glyphs[gid]->manualhints;
	fv->b.selected[i] = mergefunc[ merge + (fv->b.selected[i]?2:0) + doit ];
    }
    GDrawRequestExpose(fv->v,NULL,false);
}

static void sm_selectByPST(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);

    FVSelectByPST(fv);
}

void FVMenuFindRpl(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);

    SVCreate(fv);
}

void FVMenuReplaceWithRef(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);

    FVReplaceOutlineWithReference(fv,.001);
}

void FVMenuCorrectRefs(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    FontViewBase *fv = (FontViewBase *) GDrawGetUserData(gw);

    FVCorrectReferences(fv);
}


static void sm_dialogCharInfo(CommonView* self) {
    FontView* fv = tryObtainCastFontView( self );
    int pos = FVAnyCharSelected(fv);
    if ( pos<0 )
	return;
    if ( fv->b.cidmaster!=NULL &&
	 (fv->b.map->map[pos]==-1 || fv->b.sf->glyphs[fv->b.map->map[pos]]==NULL ))
    {
	return;
    }
    
    SCCharInfo(SFMakeChar(fv->b.sf,fv->b.map,pos),fv->b.active_layer,fv->b.map,pos);
}
void FVMenuCharInfo(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    FontView* fv = tryObtainGDataFontView( gw );
    sm_dialogCharInfo((CommonView*)fv);
}


void FVMenuBDFInfo(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    FontView* fv = tryObtainGDataFontView( gw );
    if ( fv->b.sf->bitmaps==NULL )
return;
    if ( fv->show!=fv->filled )
	SFBdfProperties(fv->b.sf,fv->b.map,fv->show);
    else
	SFBdfProperties(fv->b.sf,fv->b.map,NULL);
}

void FVMenuBaseHoriz(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
    SplineFont *sf = fv->b.cidmaster == NULL ? fv->b.sf : fv->b.cidmaster;
    sf->horiz_base = SFBaselines(sf,sf->horiz_base,false);
    SFBaseSort(sf);
}

void FVMenuBaseVert(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
    SplineFont *sf = fv->b.cidmaster == NULL ? fv->b.sf : fv->b.cidmaster;
    sf->vert_base = SFBaselines(sf,sf->vert_base,true);
    SFBaseSort(sf);
}

void FVMenuJustify(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
    SplineFont *sf = fv->b.cidmaster == NULL ? fv->b.sf : fv->b.cidmaster;
    JustifyDlg(sf);
}

void FVMenuMassRename(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
    FVMassGlyphRename(fv);
}

static void FVSetColor(FontView *fv, uint32 col) {
    int i;

    for ( i=0; i<fv->b.map->enccount; ++i ) if ( fv->b.selected[i] ) {
	SplineChar *sc = SFMakeChar(fv->b.sf,fv->b.map,i);
	sc->color = col;
    }
    GDrawRequestExpose(fv->v,NULL,false);
}

void FVMenuSetColor(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e)) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
    Color col = (Color) (intpt) (mi->ti.userdata);
    if ( (intpt) mi->ti.userdata == (intpt) -10 ) {
	struct hslrgb retcol, font_cols[6];
	retcol = GWidgetColor(_("Pick a color"),NULL,SFFontCols(fv->b.sf,font_cols));
	if ( !retcol.rgb )
return;
	col = (((int) rint(255.*retcol.r))<<16 ) |
		    (((int) rint(255.*retcol.g))<<8 ) |
		    (((int) rint(255.*retcol.b)) );
    }
    FVSetColor(fv,col);
}


static void sm_referenceShowDependentRefs(CommonView* self) {
    FontView* fv = tryObtainCastFontView( self );
    int pos = FVAnyCharSelected(fv);
    SplineChar *sc;

    if ( pos<0 || fv->b.map->map[pos]==-1 )
return;
    sc = fv->b.sf->glyphs[fv->b.map->map[pos]];
    if ( sc==NULL || sc->dependents==NULL )
return;
    SCRefBy(sc);
}

/* static void FVMenuShowDependentRefs(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) { */
/*     FontView *fv = (FontView *) GDrawGetUserData(gw); */
/*     int pos = FVAnyCharSelected(fv); */
/*     SplineChar *sc; */

/*     if ( pos<0 || fv->b.map->map[pos]==-1 ) */
/* return; */
/*     sc = fv->b.sf->glyphs[fv->b.map->map[pos]]; */
/*     if ( sc==NULL || sc->dependents==NULL ) */
/* return; */
/*     SCRefBy(sc); */
/* } */

static void FVMenuShowDependentSubs(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
    int pos = FVAnyCharSelected(fv);
    SplineChar *sc;

    if ( pos<0 || fv->b.map->map[pos]==-1 )
return;
    sc = fv->b.sf->glyphs[fv->b.map->map[pos]];
    if ( sc==NULL )
return;
    SCSubBy(sc);
}

static int getorigin(void *UNUSED(d), BasePoint *base, int index) {
    /*FontView *fv = (FontView *) d;*/

    base->x = base->y = 0;
    switch ( index ) {
      case 0:		/* Character origin */
	/* all done */
      break;
      case 1:		/* Center of selection */
	/*CVFindCenter(cv,base,!CVAnySel(cv,NULL,NULL,NULL,NULL));*/
      break;
      default:
return( false );
    }
return( true );
}

static void FVDoTransform(FontView *fv) {
    enum transdlg_flags flags=tdf_enableback|tdf_enablekerns;
    if ( FVAnyCharSelected(fv)==-1 )
return;
    if ( FVAllSelected(fv))
	flags=tdf_enableback|tdf_enablekerns|tdf_defaultkerns;
    TransformDlgCreate(fv,FVTransFunc,getorigin,flags,cvt_none);
}


static void sm_dialogTransform(CommonView* self) {
    FontView* fv = tryObtainCastFontView( self );
    FVDoTransform(fv);
}
static void sm_dialogPointOfViewProjection(CommonView* self) {
    FontView* fv = tryObtainCastFontView( self );
    struct pov_data pov_data;
    if ( FVAnyCharSelected(fv)==-1 || fv->b.sf->onlybitmaps )
	return;
    if ( PointOfViewDlg(&pov_data,fv->b.sf,false)==-1 )
	return;
    FVPointOfView((FontViewBase *) fv,&pov_data);
}

static void sm_dialogNonLinearTransform(CommonView* self) {
    FontView* fv = tryObtainCastFontView( self );
    if ( FVAnyCharSelected(fv)==-1 )
return;
    NonLinearDlg(fv,NULL);
}

void FVMenuBitmaps(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e)) {
    FontView* fv = tryObtainGDataFontView( gw );
    BitmapDlg(fv,NULL,mi->mid==SMID_RemoveBitmaps?-1:(mi->mid==SMID_AvailBitmaps) );
}

static void sm_dialogExpandStroke(CommonView* self) {
    FontView* fv = tryObtainCastFontView( self );
    FVStroke(fv);
}

#  ifdef FONTFORGE_CONFIG_TILEPATH
static void FVMenuTilePath(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
    FVTile(fv);
}

static void FVMenuPatternTile(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
    FVPatternTile(fv);
}
#endif


static void sm_overlapRemove(CommonView* self) {
    FontView* fv = tryObtainCastFontView( self );
    if ( fv->b.sf->onlybitmaps )
	return;
    /* We know it's more likely that we'll find a problem in the overlap code */
    /*  than anywhere else, so let's save the current state against a crash */
    DoAutoSaves();
    FVOverlap(&fv->b, over_remove );
}
static void sm_overlapIntersect(CommonView* self) {
    FontView* fv = tryObtainCastFontView( self );
    if ( fv->b.sf->onlybitmaps )
	return;
    /* We know it's more likely that we'll find a problem in the overlap code */
    /*  than anywhere else, so let's save the current state against a crash */
    DoAutoSaves();
    FVOverlap(&fv->b, over_intersect );
}
static void sm_overlapFindIntersections(CommonView* self) {
    FontView* fv = tryObtainCastFontView( self );
    if ( fv->b.sf->onlybitmaps )
	return;
    /* We know it's more likely that we'll find a problem in the overlap code */
    /*  than anywhere else, so let's save the current state against a crash */
    DoAutoSaves();
    FVOverlap(&fv->b, over_findinter );
}



static void sm_dialogInline(CommonView* self) {
    FontView* fv = tryObtainCastFontView( self );
    OutlineDlg(fv,NULL,NULL,true);
}
static void sm_dialogOutline(CommonView* self) {
    FontView* fv = tryObtainCastFontView( self );
    OutlineDlg(fv,NULL,NULL,false);
}
static void sm_dialogShadow(CommonView* self) {
    FontView* fv = tryObtainCastFontView( self );
    ShadowDlg(fv,NULL,NULL,false);
}
static void sm_dialogWireframe(CommonView* self) {
    FontView* fv = tryObtainCastFontView( self );
    ShadowDlg(fv,NULL,NULL,true);
}

static void FVSimplify(FontView *fv,int type) {
    static struct simplifyinfo smpls[] = {
	    { sf_normal, 0, 0, 0, 0, 0, 0 },
	    { sf_normal,.75,.05,0,-1, 0, 0 },
	    { sf_normal,.75,.05,0,-1, 0, 0 }};
    struct simplifyinfo *smpl = &smpls[type+1];

    if ( smpl->linelenmax==-1 || (type==0 && !smpl->set_as_default)) {
	smpl->err = (fv->b.sf->ascent+fv->b.sf->descent)/1000.;
	smpl->linelenmax = (fv->b.sf->ascent+fv->b.sf->descent)/100.;
    }

    if ( type==1 ) {
	if ( !SimplifyDlg(fv->b.sf,smpl))
return;
	if ( smpl->set_as_default )
	    smpls[1] = *smpl;
    }
    _FVSimplify((FontViewBase *) fv,smpl);
}


static void sm_simplify(CommonView* self) {
    FontView* fv = tryObtainCastFontView( self );
    FVSimplify( fv,false );
}

static void sm_simplifyMoreDialog(CommonView* self) {
    FontView* fv = tryObtainCastFontView( self );
    FVSimplify( fv, true );
}

static void sm_simplifyCleanup(CommonView* self) {
    FontView* fv = tryObtainCastFontView( self );
    FVSimplify( fv,-1 );
}
static void sm_simplifyCanonicalStartPoint(CommonView* self) {
    FontView* fv = tryObtainCastFontView( self );
    FVCanonicalStart( (FontViewBase *)fv );
}
static void sm_simplifyCanonicalContours(CommonView* self) {
    FontView* fv = tryObtainCastFontView( self );
    FVCanonicalContours( (FontViewBase *)fv );
}

static void sm_extremaAdd(CommonView* self) {
    FontView* fv = tryObtainCastFontView( self );
    FVAddExtrema( (FontViewBase *)fv, false);
}

static void FVMenuCorrectDir(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
    FVCorrectDir((FontViewBase *) fv);
}

static void FVMenuRound2Int(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    FVRound2Int( (FontViewBase *) GDrawGetUserData(gw), 1.0 );
}

static void FVMenuRound2Hundredths(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    FVRound2Int( (FontViewBase *) GDrawGetUserData(gw),100.0 );
}

static void FVMenuCluster(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    FVCluster( (FontViewBase *) GDrawGetUserData(gw));
}

static void FVMenuAutotrace(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
    GCursor ct=0;

    if ( fv->v!=NULL ) {
	ct = GDrawGetCursor(fv->v);
	GDrawSetCursor(fv->v,ct_watch);
	GDrawSync(NULL);
	GDrawProcessPendingEvents(NULL);
    }
    FVAutoTrace(&fv->b,e!=NULL && (e->u.mouse.state&ksm_shift));
    if ( fv->v!=NULL )
	GDrawSetCursor(fv->v,ct);
}


static void sm_accentBuild(CommonView* self) {
    FontView* fv = tryObtainCastFontView( self );
    FVBuildAccent( (FontViewBase *)fv, true );
}

static void sm_compositeBuild(CommonView* self) {
    FontView* fv = tryObtainCastFontView( self );
    FVBuildAccent( (FontViewBase *)fv, false );
}



static void sm_duplicateGlyphs(CommonView* self) {
    FontView* fv = tryObtainCastFontView( self );
    FVBuildDuplicate( (FontViewBase *)fv);
}

static void sm_revertToFile(CommonView* self) {
    FontView* fv = tryObtainCastFontView( self );
    FVRevert((FontViewBase *)fv);
}
static void sm_revertToBackup(CommonView* self) {
    FontView* fv = tryObtainCastFontView( self );
    FVRevertBackup((FontViewBase *)fv);
}
static void sm_revertGlyphs(CommonView* self) {
    FontView* fv = tryObtainCastFontView( self );
    FVRevertGlyph((FontViewBase *) fv);
}


static void sm_selectionClear(CommonView* self) {
    FontView* fv = tryObtainCastFontView( self );
    FVDeselectAll(fv);
}

static void sm_selectionAddChar(CommonView* self, struct splinechar *sc ) {
    FontView* fv = tryObtainCastFontView( self );
    int pos = sc->unicodeenc;
    FVChangeChar(   fv, pos );
    FVScrollToChar( fv, pos );

}

void FVMenuShowGroup(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
#ifdef KOREAN
    ShowGroup( ((FontView *) GDrawGetUserData(gw))->sf );
#endif
}


#if HANYANG
static void FVMenuModifyComposition(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
    if ( fv->b.sf->rules!=NULL )
	SFModifyComposition(fv->b.sf);
}

static void FVMenuBuildSyllables(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
    if ( fv->b.sf->rules!=NULL )
	SFBuildSyllables(fv->b.sf);
}
#endif

void FVMenuCompareFonts(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    FontView* fv = tryObtainGDataFontView( gw );
    FontCompareDlg(fv);
}

void FVMenuMergeFonts(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    FontView* fv = tryObtainGDataFontView( gw );
    FVMergeFonts(fv);
}

void FVAddWordList(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e))
{
    CharView* cv = tryObtainGDataCharView( gw );
    if( cv ) {
	WordlistLoadToGTextInfo( cv->charselector, &cv->charselectoridx );
    }
}

void FVMenuCloseTab(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    CharView* cv = tryObtainGDataCharView( gw );
    if( cv ) {
	int pos, i;

	if ( cv->b.container || cv->tabs==NULL || cv->former_cnt<=1 )
	    return;
	pos = GTabSetGetSel(cv->tabs);
	free(cv->former_names[pos]);
	for ( i=pos+1; i<cv->former_cnt; ++i )
	    cv->former_names[i-1] = cv->former_names[i];
	--cv->former_cnt;
	GTabSetRemoveTabByPos(cv->tabs,pos);	/* This should send an event that the selection has changed */
	GTabSetRemetric(cv->tabs);
    }
}


void FVMenuInterpFonts(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    FontView* fv = tryObtainGDataFontView( gw );
    FVInterpolateFonts(fv);
}

static void FVShowInfo(FontView *fv);

void FVChangeChar(FontView *fv,int i) {

    if ( i!=-1 ) {
	FVDeselectAll(fv);
	fv->b.selected[i] = true;
	fv->sel_index = 1;
	fv->end_pos = fv->pressed_pos = i;
	FVToggleCharSelected(fv,i);
	FVScrollToChar(fv,i);
	FVShowInfo(fv);
    }
}

void FVScrollToChar(FontView *fv,int i) {

    if ( fv->v==NULL || fv->colcnt==0 )	/* Can happen in scripts */
return;

    if ( i!=-1 ) {
	if ( i/fv->colcnt<fv->rowoff || i/fv->colcnt >= fv->rowoff+fv->rowcnt ) {
	    fv->rowoff = i/fv->colcnt;
	    if ( fv->rowcnt>= 3 )
		--fv->rowoff;
	    if ( fv->rowoff+fv->rowcnt>=fv->rowltot )
		fv->rowoff = fv->rowltot-fv->rowcnt;
	    if ( fv->rowoff<0 ) fv->rowoff = 0;
	    GScrollBarSetPos(fv->vsb,fv->rowoff);
	    GDrawRequestExpose(fv->v,NULL,false);
	}
    }
}

static void FVScrollToGID(FontView *fv,int gid) {
    FVScrollToChar(fv,fv->b.map->backmap[gid]);
}

static void FV_ChangeGID(FontView *fv,int gid) {
    FVChangeChar(fv,fv->b.map->backmap[gid]);
}


static void _FVMenuChangeChar(FontView *fv,int mid ) {
    SplineFont *sf = fv->b.sf;
    EncMap *map = fv->b.map;
    int pos = FVAnyCharSelected(fv);

    if ( pos>=0 ) {
	if ( mid==SMID_Next )
	    ++pos;
	else if ( mid==SMID_Prev )
	    --pos;
	else if ( mid==SMID_NextDef ) {
	    for ( ++pos; pos<map->enccount &&
		    (map->map[pos]==-1 || !SCWorthOutputting(sf->glyphs[map->map[pos]]) ||
			(fv->show!=fv->filled && fv->show->glyphs[map->map[pos]]==NULL ));
		    ++pos );
	    if ( pos>=map->enccount ) {
		int selpos = FVAnyCharSelected(fv);
		char *iconv_name = map->enc->iconv_name ? map->enc->iconv_name :
			map->enc->enc_name;
		if ( strstr(iconv_name,"2022")!=NULL && selpos<0x2121 )
		    pos = 0x2121;
		else if ( strstr(iconv_name,"EUC")!=NULL && selpos<0xa1a1 )
		    pos = 0xa1a1;
		else if ( map->enc->is_tradchinese ) {
		    if ( strstrmatch(map->enc->enc_name,"HK")!=NULL &&
			    selpos<0x8140 )
			pos = 0x8140;
		    else
			pos = 0xa140;
		} else if ( map->enc->is_japanese ) {
		    if ( strstrmatch(iconv_name,"SJIS")!=NULL ||
			    (strstrmatch(iconv_name,"JIS")!=NULL && strstrmatch(iconv_name,"SHIFT")!=NULL )) {
			if ( selpos<0x8100 )
			    pos = 0x8100;
			else if ( selpos<0xb000 )
			    pos = 0xb000;
		    }
		} else if ( map->enc->is_korean ) {
		    if ( strstrmatch(iconv_name,"JOHAB")!=NULL ) {
			if ( selpos<0x8431 )
			    pos = 0x8431;
		    } else {	/* Wansung, EUC-KR */
			if ( selpos<0xa1a1 )
			    pos = 0xa1a1;
		    }
		} else if ( map->enc->is_simplechinese ) {
		    if ( strmatch(iconv_name,"EUC-CN")==0 && selpos<0xa1a1 )
			pos = 0xa1a1;
		}
		if ( pos>=map->enccount )
return;
	    }
	} else if ( mid==SMID_PrevDef ) {
	    for ( --pos; pos>=0 &&
		    (map->map[pos]==-1 || !SCWorthOutputting(sf->glyphs[map->map[pos]]) ||
			(fv->show!=fv->filled && fv->show->glyphs[map->map[pos]]==NULL ));
		    --pos );
	    if ( pos<0 )
return;
	}
    }
    if ( pos<0 ) pos = map->enccount-1;
    else if ( pos>= map->enccount ) pos = 0;
    if ( pos>=0 && pos<map->enccount )
	FVChangeChar(fv,pos);
}

static void sm_gotoCharNext( CommonView* self )
{
    FontView* fv = tryObtainCastFontView( self );
    _FVMenuChangeChar( fv, SMID_Next );
}

static void sm_gotoCharPrev( CommonView* self )
{
    FontView* fv = tryObtainCastFontView( self );
    _FVMenuChangeChar( fv, SMID_Prev );
}

static void sm_gotoCharNextDefined( CommonView* self )
{
    FontView* fv = tryObtainCastFontView( self );
    _FVMenuChangeChar( fv, SMID_NextDef );
}

static void sm_gotoCharPrevDefined( CommonView* self )
{
    FontView* fv = tryObtainCastFontView( self );
    _FVMenuChangeChar( fv, SMID_PrevDef );
}

static void sm_gotoCharFormer( CommonView* self )
{
    FontView* fv = tryObtainCastFontView( self );
    _FVMenuChangeChar( fv, SMID_Former );
}

/******************************/
/******************************/
/******************************/

typedef void (*visitAllCharViewsVisitor)( FontView* fv, CharView* cv, void* udata );
void visitAllCharViews( FontView* fv, visitAllCharViewsVisitor visitor, void* udata )
{
    SplineFont* sf = tryObtainCastSplineFont( fv );
    CharView* cv = 0;
    CharView* cvnext = 0;
    
    int i=0;
    for ( i=0; i<sf->glyphcnt; ++i ) {
	if ( sf->glyphs[i]!=NULL && sf->glyphs[i]->views!=NULL ) {
	    for ( cv=(CharView *) (sf->glyphs[i]->views); cv!=NULL; cv = cvnext ) {
		cvnext = (CharView *) (cv->b.next);
		visitor( fv, cv, udata );
	    }
	}
    }
}

/******************************************************************************/
/******************************************************************************/
/******************************************************************************/
/*** These degate the call over to each charview that is running for the font */
/******************************************************************************/

static void sm_numberPoints_visitor( FontView* fv, CharView* cv, void* udata ) {
    int mid = (int)udata;
    cv->b.m_commonView.m_sharedmenu_funcs.numberPoints( (CommonView*)cv, mid );     
}
static void sm_numberPoints(CommonView* self, int mid ) {
    FontView* fv = tryObtainCastFontView( self );
    visitAllCharViews( fv, sm_numberPoints_visitor, (void*)mid );    
}

#define SM_VISITOR_DELEGATE_TO_ALL_CHARVIEW( FNAME )			              \
    static void sm_ ## FNAME ## _visitor( FontView* fv, CharView* cv, void* udata ) { \
	if( cv->b.m_commonView.m_sharedmenu_funcs.FNAME )		\
	    cv->b.m_commonView.m_sharedmenu_funcs.FNAME( (CommonView*)cv ); \
    }									\
    static void sm_## FNAME( CommonView* self ) {			\
	FontView* fv = tryObtainCastFontView( self );			\
	visitAllCharViews( fv, sm_## FNAME ##_visitor, (void*)0 );	\
    }

SM_VISITOR_DELEGATE_TO_ALL_CHARVIEW( toggleShowTabs );
SM_VISITOR_DELEGATE_TO_ALL_CHARVIEW( toggleShowRulers );
SM_VISITOR_DELEGATE_TO_ALL_CHARVIEW( toggleShowPaletteTools );
SM_VISITOR_DELEGATE_TO_ALL_CHARVIEW( toggleShowPaletteLayers );
SM_VISITOR_DELEGATE_TO_ALL_CHARVIEW( toggleShowPaletteDocked );


    
/******************************************************************************/
/******************************************************************************/
/******************************************************************************/


void FVMenuChangeChar(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e)) {
    FontView* fv = tryObtainGDataFontView( gw );
    _FVMenuChangeChar(fv,mi->mid);
}

static void FVShowSubFont(FontView *fv,SplineFont *new) {
    MetricsView *mv, *mvnext;
    BDFFont *newbdf;
    int wascompact = fv->b.normal!=NULL;
    extern int use_freetype_to_rasterize_fv;

    for ( mv=fv->b.sf->metrics; mv!=NULL; mv = mvnext ) {
	/* Don't bother trying to fix up metrics views, just not worth it */
	mvnext = mv->next;
	GDrawDestroyWindow(mv->gw);
    }
    if ( wascompact ) {
	EncMapFree(fv->b.map);
	if (fv->b.map == fv->b.sf->map) { fv->b.sf->map = fv->b.normal; }
	fv->b.map = fv->b.normal;
	fv->b.normal = NULL;
	fv->b.selected = realloc(fv->b.selected,fv->b.map->enccount);
	memset(fv->b.selected,0,fv->b.map->enccount);
    }
    CIDSetEncMap((FontViewBase *) fv,new);
    if ( wascompact ) {
	fv->b.normal = EncMapCopy(fv->b.map);
	CompactEncMap(fv->b.map,fv->b.sf);
	FontViewReformatOne(&fv->b);
	FVSetTitle(&fv->b);
    }
    newbdf = SplineFontPieceMeal(fv->b.sf,fv->b.active_layer,fv->filled->pixelsize,72,
	    (fv->antialias?pf_antialias:0)|(fv->bbsized?pf_bbsized:0)|
		(use_freetype_to_rasterize_fv && !fv->b.sf->strokedfont && !fv->b.sf->multilayer?pf_ft_nohints:0),
	    NULL);
    BDFFontFree(fv->filled);
    if ( fv->filled == fv->show )
	fv->show = newbdf;
    fv->filled = newbdf;
    GDrawRequestExpose(fv->v,NULL,true);
}

static void sm_gotoChar(CommonView* self)
{
    FontView* fv = tryObtainCastFontView( self );
    int merge_with_selection = false;
    int pos = GotoChar(fv->b.sf,fv->b.map,&merge_with_selection);
    if ( fv->b.cidmaster!=NULL && pos!=-1 && !fv->b.map->enc->is_compact ) {
	SplineFont *cidmaster = fv->b.cidmaster;
	int k, hadk= cidmaster->subfontcnt;
	for ( k=0; k<cidmaster->subfontcnt; ++k ) {
	    SplineFont *sf = cidmaster->subfonts[k];
	    if ( pos<sf->glyphcnt && sf->glyphs[pos]!=NULL )
	break;
	    if ( pos<sf->glyphcnt )
		hadk = k;
	}
	if ( k==cidmaster->subfontcnt && pos>=fv->b.sf->glyphcnt )
	    k = hadk;
	if ( k!=cidmaster->subfontcnt && cidmaster->subfonts[k] != fv->b.sf )
	    FVShowSubFont(fv,cidmaster->subfonts[k]);
	if ( pos>=fv->b.sf->glyphcnt )
	    pos = -1;
    }
    if ( !merge_with_selection )
	FVChangeChar(fv,pos);
    else {
	if ( !fv->b.selected[pos] ) {
	    fv->b.selected[pos] = ++fv->sel_index;
	    FVToggleCharSelected(fv,pos);
	}
	fv->end_pos = fv->pressed_pos = pos;
	FVScrollToChar(fv,pos);
	FVShowInfo(fv);
    }
}

void FVMenuGotoChar(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    FontView* fv = tryObtainGDataFontView( gw );
    int merge_with_selection = false;
    int pos = GotoChar(fv->b.sf,fv->b.map,&merge_with_selection);
    if ( fv->b.cidmaster!=NULL && pos!=-1 && !fv->b.map->enc->is_compact ) {
	SplineFont *cidmaster = fv->b.cidmaster;
	int k, hadk= cidmaster->subfontcnt;
	for ( k=0; k<cidmaster->subfontcnt; ++k ) {
	    SplineFont *sf = cidmaster->subfonts[k];
	    if ( pos<sf->glyphcnt && sf->glyphs[pos]!=NULL )
	break;
	    if ( pos<sf->glyphcnt )
		hadk = k;
	}
	if ( k==cidmaster->subfontcnt && pos>=fv->b.sf->glyphcnt )
	    k = hadk;
	if ( k!=cidmaster->subfontcnt && cidmaster->subfonts[k] != fv->b.sf )
	    FVShowSubFont(fv,cidmaster->subfonts[k]);
	if ( pos>=fv->b.sf->glyphcnt )
	    pos = -1;
    }
    if ( !merge_with_selection )
	FVChangeChar(fv,pos);
    else {
	if ( !fv->b.selected[pos] ) {
	    fv->b.selected[pos] = ++fv->sel_index;
	    FVToggleCharSelected(fv,pos);
	}
	fv->end_pos = fv->pressed_pos = pos;
	FVScrollToChar(fv,pos);
	FVShowInfo(fv);
    }
}


static void sm_dialogLigatures(CommonView* self) {
    FontView* fv = tryObtainCastFontView( self );
    SFShowLigatures(fv->b.sf,NULL);
}


static void sm_dialogKernPairs(CommonView* self) {
    FontView* fv = tryObtainCastFontView( self );
    SFKernClassTempDecompose(fv->b.sf,false);
    SFShowKernPairs(fv->b.sf,NULL,NULL,fv->b.active_layer);
    SFKernCleanup(fv->b.sf,false);
}

void FVMenuAnchorPairs(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e)) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
    SFShowKernPairs(fv->b.sf,NULL,mi->ti.userdata,fv->b.active_layer);
}

void FVMenuShowAtt(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    FontView* fv = tryObtainGDataFontView( gw );
    ShowAtt(fv->b.sf,fv->b.active_layer);
}

static void FVMenuDisplaySubs(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);

    if ( fv->cur_subtable!=0 ) {
	fv->cur_subtable = NULL;
    } else {
	SplineFont *sf = fv->b.sf;
	OTLookup *otf;
	struct lookup_subtable *sub;
	int cnt, k;
	char **names = NULL;
	if ( sf->cidmaster ) sf=sf->cidmaster;
	for ( k=0; k<2; ++k ) {
	    cnt = 0;
	    for ( otf = sf->gsub_lookups; otf!=NULL; otf=otf->next ) {
		if ( otf->lookup_type==gsub_single ) {
		    for ( sub=otf->subtables; sub!=NULL; sub=sub->next ) {
			if ( names )
			    names[cnt] = sub->subtable_name;
			++cnt;
		    }
		}
	    }
	    if ( cnt==0 )
	break;
	    if ( names==NULL )
		names = malloc((cnt+3) * sizeof(char *));
	    else {
		names[cnt++] = "-";
		names[cnt++] = _("New Lookup Subtable...");
		names[cnt] = NULL;
	    }
	}
	sub = NULL;
	if ( names!=NULL ) {
	    int ret = gwwv_choose(_("Display Substitution..."), (const char **) names, cnt, 0,
		    _("Pick a substitution to display in the window."));
	    if ( ret!=-1 )
		sub = SFFindLookupSubtable(sf,names[ret]);
	    free(names);
	    if ( ret==-1 )
return;
	}
	if ( sub==NULL )
	    sub = SFNewLookupSubtableOfType(sf,gsub_single,NULL,fv->b.active_layer);
	if ( sub!=NULL )
	    fv->cur_subtable = sub;
    }
    GDrawRequestExpose(fv->v,NULL,false);
}

static void FVChangeDisplayFont(FontView *fv,BDFFont *bdf) {
    int samesize=0;
    int rcnt, ccnt;
    int oldr, oldc;
    int first_time = fv->show==NULL;

    if ( fv->v==NULL )			/* Can happen in scripts */
return;

    if ( fv->show!=bdf ) {
	oldc = fv->cbw*fv->colcnt;
	oldr = fv->cbh*fv->rowcnt;

	fv->show = bdf;
	fv->b.active_bitmap = bdf==fv->filled ? NULL : bdf;
	if ( fv->user_requested_magnify!=-1 )
	    fv->magnify=fv->user_requested_magnify;
	else if ( bdf->pixelsize<20 ) {
	    if ( bdf->pixelsize<=9 )
		fv->magnify = 3;
	    else
		fv->magnify = 2;
	    samesize = ( fv->show && fv->cbw == (bdf->pixelsize*fv->magnify)+1 );
	} else
	    fv->magnify = 1;
	if ( !first_time && fv->cbw == fv->magnify*bdf->pixelsize+1 )
	    samesize = true;
	fv->cbw = (bdf->pixelsize*fv->magnify)+1;
	fv->cbh = (bdf->pixelsize*fv->magnify)+1+fv->lab_height+1;
	fv->resize_expected = !samesize;
	ccnt = fv->b.sf->desired_col_cnt;
	rcnt = fv->b.sf->desired_row_cnt;
	if ((( bdf->pixelsize<=fv->b.sf->display_size || bdf->pixelsize<=-fv->b.sf->display_size ) &&
		 fv->b.sf->top_enc!=-1 /* Not defaulting */ ) ||
		bdf->pixelsize<=48 ) {
	    /* use the desired sizes */
	} else {
	    if ( bdf->pixelsize>48 ) {
		ccnt = 8;
		rcnt = 2;
	    } else if ( bdf->pixelsize>=96 ) {
		ccnt = 4;
		rcnt = 1;
	    }
	    if ( !first_time ) {
		if ( ccnt < oldc/fv->cbw )
		    ccnt = oldc/fv->cbw;
		if ( rcnt < oldr/fv->cbh )
		    rcnt = oldr/fv->cbh;
	    }
	}
	if ( samesize ) {
	    GDrawRequestExpose(fv->v,NULL,false);
	} else if ( fv->b.container!=NULL && fv->b.container->funcs->doResize!=NULL ) {
	    (fv->b.container->funcs->doResize)(fv->b.container,&fv->b,
		    ccnt*fv->cbw+1+GDrawPointsToPixels(fv->gw,_GScrollBar_Width),
		    rcnt*fv->cbh+1+fv->mbh+fv->infoh);
	} else {
	    GDrawResize(fv->gw,
		    ccnt*fv->cbw+1+GDrawPointsToPixels(fv->gw,_GScrollBar_Width),
		    rcnt*fv->cbh+1+fv->mbh+fv->infoh);
	}
    }
}

struct md_data {
    int done;
    int ish;
    FontView *fv;
};

static int md_e_h(GWindow gw, GEvent *e) {
    if ( e->type==et_close ) {
	struct md_data *d = GDrawGetUserData(gw);
	d->done = true;
    } else if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct md_data *d = GDrawGetUserData(gw);
	static int masks[] = { fvm_baseline, fvm_origin, fvm_advanceat, fvm_advanceto, -1 };
	int i, metrics;
	if ( GGadgetGetCid(e->u.control.g)==10 ) {
	    metrics = 0;
	    for ( i=0; masks[i]!=-1 ; ++i )
		if ( GGadgetIsChecked(GWidgetGetControl(gw,masks[i])))
		    metrics |= masks[i];
	    if ( d->ish )
		default_fv_showhmetrics = d->fv->showhmetrics = metrics;
	    else
		default_fv_showvmetrics = d->fv->showvmetrics = metrics;
	}
	d->done = true;
    } else if ( e->type==et_char ) {
return( false );
    }
return( true );
}

static void FVMenuShowMetrics(GWindow fvgw,struct gmenuitem *mi, GEvent *UNUSED(e)) {
    FontView *fv = (FontView *) GDrawGetUserData(fvgw);
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    struct md_data d;
    GGadgetCreateData gcd[7];
    GTextInfo label[6];
    int metrics = mi->mid==SMID_ShowHMetrics ? fv->showhmetrics : fv->showvmetrics;

    d.fv = fv;
    d.done = 0;
    d.ish = mi->mid==SMID_ShowHMetrics;

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_restrict;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.restrict_input_to_me = 1;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.utf8_window_title = d.ish?_("Show H. Metrics"):_("Show V. Metrics");
    pos.x = pos.y = 0;
    pos.width =GDrawPointsToPixels(NULL,GGadgetScale(170));
    pos.height = GDrawPointsToPixels(NULL,130);
    gw = GDrawCreateTopWindow(NULL,&pos,md_e_h,&d,&wattrs);

    memset(&label,0,sizeof(label));
    memset(&gcd,0,sizeof(gcd));

    label[0].text = (unichar_t *) _("Baseline");
    label[0].text_is_1byte = true;
    gcd[0].gd.label = &label[0];
    gcd[0].gd.pos.x = 8; gcd[0].gd.pos.y = 8;
    gcd[0].gd.flags = gg_enabled|gg_visible|(metrics&fvm_baseline?gg_cb_on:0);
    gcd[0].gd.cid = fvm_baseline;
    gcd[0].creator = GCheckBoxCreate;

    label[1].text = (unichar_t *) _("Origin");
    label[1].text_is_1byte = true;
    gcd[1].gd.label = &label[1];
    gcd[1].gd.pos.x = 8; gcd[1].gd.pos.y = gcd[0].gd.pos.y+16;
    gcd[1].gd.flags = gg_enabled|gg_visible|(metrics&fvm_origin?gg_cb_on:0);
    gcd[1].gd.cid = fvm_origin;
    gcd[1].creator = GCheckBoxCreate;

    label[2].text = (unichar_t *) _("Advance Width as a Line");
    label[2].text_is_1byte = true;
    gcd[2].gd.label = &label[2];
    gcd[2].gd.pos.x = 8; gcd[2].gd.pos.y = gcd[1].gd.pos.y+16;
    gcd[2].gd.flags = gg_enabled|gg_visible|gg_utf8_popup|(metrics&fvm_advanceat?gg_cb_on:0);
    gcd[2].gd.cid = fvm_advanceat;
    gcd[2].gd.popup_msg = (unichar_t *) _("Display the advance width as a line\nperpendicular to the advance direction");
    gcd[2].creator = GCheckBoxCreate;

    label[3].text = (unichar_t *) _("Advance Width as a Bar");
    label[3].text_is_1byte = true;
    gcd[3].gd.label = &label[3];
    gcd[3].gd.pos.x = 8; gcd[3].gd.pos.y = gcd[2].gd.pos.y+16;
    gcd[3].gd.flags = gg_enabled|gg_visible|gg_utf8_popup|(metrics&fvm_advanceto?gg_cb_on:0);
    gcd[3].gd.cid = fvm_advanceto;
    gcd[3].gd.popup_msg = (unichar_t *) _("Display the advance width as a bar under the glyph\nshowing the extent of the advance");
    gcd[3].creator = GCheckBoxCreate;

    label[4].text = (unichar_t *) _("_OK");
    label[4].text_is_1byte = true;
    label[4].text_in_resource = true;
    gcd[4].gd.label = &label[4];
    gcd[4].gd.pos.x = 20-3; gcd[4].gd.pos.y = GDrawPixelsToPoints(NULL,pos.height)-35-3;
    gcd[4].gd.pos.width = -1; gcd[4].gd.pos.height = 0;
    gcd[4].gd.flags = gg_visible | gg_enabled | gg_but_default;
    gcd[4].gd.cid = 10;
    gcd[4].creator = GButtonCreate;

    label[5].text = (unichar_t *) _("_Cancel");
    label[5].text_is_1byte = true;
    label[5].text_in_resource = true;
    gcd[5].gd.label = &label[5];
    gcd[5].gd.pos.x = -20; gcd[5].gd.pos.y = gcd[4].gd.pos.y+3;
    gcd[5].gd.pos.width = -1; gcd[5].gd.pos.height = 0;
    gcd[5].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
    gcd[5].creator = GButtonCreate;

    GGadgetsCreate(gw,gcd);

    GDrawSetVisible(gw,true);
    while ( !d.done )
	GDrawProcessOneEvent(NULL);
    GDrawDestroyWindow(gw);

    SavePrefs(true);
    GDrawRequestExpose(fv->v,NULL,false);
}

static void FV_ChangeDisplayBitmap(FontView *fv,BDFFont *bdf) {
    FVChangeDisplayFont(fv,bdf);
    if (fv->show != NULL) {
        fv->b.sf->display_size = fv->show->pixelsize;
    } else {
        fv->b.sf->display_size = 1;
    }
}

static void FVMenuSize(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e)) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
    int dspsize = fv->filled->pixelsize;
    int changedmodifier = false;
    extern int use_freetype_to_rasterize_fv;

    fv->magnify = 1;
    fv->user_requested_magnify = -1;
    if ( mi->mid == SMID_24 )
	default_fv_font_size = dspsize = 24;
    else if ( mi->mid == SMID_36 )
	default_fv_font_size = dspsize = 36;
    else if ( mi->mid == SMID_48 )
	default_fv_font_size = dspsize = 48;
    else if ( mi->mid == SMID_72 )
	default_fv_font_size = dspsize = 72;
    else if ( mi->mid == SMID_96 )
	default_fv_font_size = dspsize = 96;
    else if ( mi->mid == SMID_128 )
	default_fv_font_size = dspsize = 128;
    else if ( mi->mid == SMID_FitToBbox ) {
	default_fv_bbsized = fv->bbsized = !fv->bbsized;
	fv->b.sf->display_bbsized = fv->bbsized;
	changedmodifier = true;
    } else {
	default_fv_antialias = fv->antialias = !fv->antialias;
	fv->b.sf->display_antialias = fv->antialias;
	changedmodifier = true;
    }

    SavePrefs(true);
    if ( fv->filled!=fv->show || fv->filled->pixelsize != dspsize || changedmodifier ) {
	BDFFont *new, *old;
	old = fv->filled;
	new = SplineFontPieceMeal(fv->b.sf,fv->b.active_layer,dspsize,72,
	    (fv->antialias?pf_antialias:0)|(fv->bbsized?pf_bbsized:0)|
		(use_freetype_to_rasterize_fv && !fv->b.sf->strokedfont && !fv->b.sf->multilayer?pf_ft_nohints:0),
	    NULL);
	fv->filled = new;
	FVChangeDisplayFont(fv,new);
	BDFFontFree(old);
	fv->b.sf->display_size = -dspsize;
	if ( fv->b.cidmaster!=NULL ) {
	    int i;
	    for ( i=0; i<fv->b.cidmaster->subfontcnt; ++i )
		fv->b.cidmaster->subfonts[i]->display_size = -dspsize;
	}
    }
}

void FVSetUIToMatch(FontView *destfv,FontView *srcfv) {
    extern int use_freetype_to_rasterize_fv;

    if ( destfv->filled==NULL || srcfv->filled==NULL )
return;
    if ( destfv->magnify!=srcfv->magnify ||
	    destfv->user_requested_magnify!=srcfv->user_requested_magnify ||
	    destfv->bbsized!=srcfv->bbsized ||
	    destfv->antialias!=srcfv->antialias ||
	    destfv->filled->pixelsize != srcfv->filled->pixelsize ) {
	BDFFont *new, *old;
	destfv->magnify = srcfv->magnify;
	destfv->user_requested_magnify = srcfv->user_requested_magnify;
	destfv->bbsized = srcfv->bbsized;
	destfv->antialias = srcfv->antialias;
	old = destfv->filled;
	new = SplineFontPieceMeal(destfv->b.sf,destfv->b.active_layer,srcfv->filled->pixelsize,72,
	    (destfv->antialias?pf_antialias:0)|(destfv->bbsized?pf_bbsized:0)|
		(use_freetype_to_rasterize_fv && !destfv->b.sf->strokedfont && !destfv->b.sf->multilayer?pf_ft_nohints:0),
	    NULL);
	destfv->filled = new;
	FVChangeDisplayFont(destfv,new);
	BDFFontFree(old);
    }
}

static void FV_LayerChanged( FontView *fv ) {
    extern int use_freetype_to_rasterize_fv;
    BDFFont *new, *old;

    fv->magnify = 1;
    fv->user_requested_magnify = -1;

    old = fv->filled;
    new = SplineFontPieceMeal(fv->b.sf,fv->b.active_layer,fv->filled->pixelsize,72,
	(fv->antialias?pf_antialias:0)|(fv->bbsized?pf_bbsized:0)|
	    (use_freetype_to_rasterize_fv && !fv->b.sf->strokedfont && !fv->b.sf->multilayer?pf_ft_nohints:0),
	NULL);
    fv->filled = new;
    FVChangeDisplayFont(fv,new);
    fv->b.sf->display_size = -fv->filled->pixelsize;
    BDFFontFree(old);
}

static void FVMenuChangeLayer(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e)) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);

    fv->b.active_layer = mi->mid;
    fv->b.sf->display_layer = mi->mid;
    FV_LayerChanged(fv);
}

static void FVMenuMagnify(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
    int magnify = fv->user_requested_magnify!=-1 ? fv->user_requested_magnify : fv->magnify;
    char def[20], *end, *ret;
    int val;
    BDFFont *show = fv->show;

    sprintf( def, "%d", magnify );
    ret = gwwv_ask_string(_("Bitmap Magnification..."),def,_("Please specify a bitmap magnification factor."));
    if ( ret==NULL )
return;
    val = strtol(ret,&end,10);
    if ( val<1 || val>5 || *end!='\0' )
	ff_post_error( _("Bad Number"),_("Bad Number") );
    else {
	fv->user_requested_magnify = val;
	fv->show = fv->filled;
	fv->b.active_bitmap = NULL;
	FVChangeDisplayFont(fv,show);
    }
    free(ret);
}

static void FVMenuWSize(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e)) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
    int h,v;
    extern int default_fv_col_count, default_fv_row_count;

    if ( mi->mid == SMID_32x8 ) {
	h = 32; v=8;
    } else if ( mi->mid == SMID_16x4 ) {
	h = 16; v=4;
    } else {
	h = 8; v=2;
    }
    GDrawResize(fv->gw,
	    h*fv->cbw+1+GDrawPointsToPixels(fv->gw,_GScrollBar_Width),
	    v*fv->cbh+1+fv->mbh+fv->infoh);
    fv->b.sf->desired_col_cnt = default_fv_col_count = h;
    fv->b.sf->desired_row_cnt = default_fv_row_count = v;

    SavePrefs(true);
}

void FVMenuGlyphLabel(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e)) {
    FontView* fv = tryObtainGDataFontView( gw );

    default_fv_glyphlabel = fv->glyphlabel = mi->mid;

    GDrawRequestExpose(fv->v,NULL,false);

    SavePrefs(true);
}

void FVMenuShowBitmap(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e)) {
    FontView* fv = tryObtainGDataFontView( gw );
    BDFFont *bdf = mi->ti.userdata;

    FV_ChangeDisplayBitmap(fv,bdf);		/* Let's not change any of the others */
}

static void FV_ShowFilled(FontView *fv) {

    fv->magnify = 1;
    fv->user_requested_magnify = 1;
    if ( fv->show!=fv->filled )
	FVChangeDisplayFont(fv,fv->filled);
    fv->b.sf->display_size = -fv->filled->pixelsize;
    fv->b.active_bitmap = NULL;
}

static void sm_metricsCenter( CommonView* self, int docenter )
{
    FontView* fv = tryObtainCastFontView( self );
    FVMetricsCenter((FontViewBase*)fv,docenter);
}


static void FVMenuCenter(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e)) {
    FontViewBase *fv = (FontViewBase *) GDrawGetUserData(gw);
    FVMetricsCenter(fv,mi->mid==SMID_Center);
}

static void sm_setWidth( CommonView* self, enum widthtype wtype ) {
    FontView* fv = tryObtainCastFontView( self );
    if ( FVAnyCharSelected(fv)==-1 )
	return;
    if ( wtype == wt_vwidth && !fv->b.sf->hasvmetrics )
	return;
    FVSetWidth(fv,wtype);
}

static void FVMenuSetWidth(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e)) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);

    if ( FVAnyCharSelected(fv)==-1 )
return;
    if ( mi->mid == SMID_SetVWidth && !fv->b.sf->hasvmetrics )
return;
    FVSetWidth(fv,mi->mid==SMID_SetWidth   ?wt_width:
		  mi->mid==SMID_SetLBearing?wt_lbearing:
		  mi->mid==SMID_SetRBearing?wt_rbearing:
		  mi->mid==SMID_SetBearings?wt_bearings:
		  wt_vwidth);
}

void FVMenuAutoWidth(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);

    FVAutoWidth2(fv);
}

static void FVMenuKernByClasses(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
    ShowKernClasses(fv->b.sf,NULL,fv->b.active_layer,false);
    
}

static void FVMenuVKernByClasses(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);

    ShowKernClasses(fv->b.sf,NULL,fv->b.active_layer,true);
}

static void FVMenuRemoveKern(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);

    FVRemoveKerns(&fv->b);
}

static void FVMenuRemoveVKern(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);

    FVRemoveVKerns(&fv->b);
}

static void FVMenuKPCloseup(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
    int i;

    for ( i=0; i<fv->b.map->enccount; ++i )
	if ( fv->b.selected[i] )
    break;
    KernPairD(fv->b.sf,i==fv->b.map->enccount?NULL:
		    fv->b.map->map[i]==-1?NULL:
		    fv->b.sf->glyphs[fv->b.map->map[i]],NULL,fv->b.active_layer,
		    false);
}

static void FVMenuVKernFromHKern(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);

    FVVKernFromHKern(&fv->b);
}


static void sm_hintDoAutoHint(CommonView* self) {
    FontView* fv = tryObtainCastFontView( self );
    FVAutoHint( &fv->b );
}
static void sm_hintAutoSubs(CommonView* self) {
    FontView* fv = tryObtainCastFontView( self );
    FVAutoHintSubs( &fv->b );
}
static void sm_hintAutoCounter(CommonView* self) {
    FontView* fv = tryObtainCastFontView( self );
    FVAutoCounter( &fv->b );
}
static void sm_hintDontAutoHint(CommonView* self) {
    FontView* fv = tryObtainCastFontView( self );
    FVDontAutoHint( &fv->b );
}



static void sm_hintSuggestDeltasDialog(CommonView* self) {
    FontView* fv = tryObtainCastFontView( self );
    if ( !hasFreeTypeDebugger())
	return;
    DeltaSuggestionDlg(fv,NULL);
}


static void sm_hintAutoInstr(CommonView* self) {
    FontView* fv = tryObtainCastFontView( self );
    FVAutoInstr( &fv->b );
}

static void sm_hintEditInstructionsDialog(CommonView* self) {
    FontView* fv = tryObtainCastFontView( self );
    int index = FVAnyCharSelected(fv);
    SplineChar *sc;
    if ( index<0 )
	return;
    sc = SFMakeChar(fv->b.sf,fv->b.map,index);
    SCEditInstructions(sc);
}


static void sm_hintEditTable_fpgm(CommonView* self) {
    FontView* fv = tryObtainCastFontView( self );
    SFEditTable(fv->b.sf,CHR('f','p','g','m'));
}
static void sm_hintEditTable_prep(CommonView* self) {
    FontView* fv = tryObtainCastFontView( self );
    SFEditTable(fv->b.sf,CHR('p','r','e','p'));
}
static void sm_hintEditTable_maxp(CommonView* self) {
    FontView* fv = tryObtainCastFontView( self );
    SFEditTable(fv->b.sf,CHR('m','a','x','p'));
}
static void sm_hintEditTable_cvt(CommonView* self) {
    FontView* fv = tryObtainCastFontView( self );
    SFEditTable(fv->b.sf,CHR('c','v','t',' '));
}


static void sm_hintRemoveInstructionTables(CommonView* self) {
    FontView* fv = tryObtainCastFontView( self );
    TtfTablesFree(fv->b.sf->ttf_tables);
    fv->b.sf->ttf_tables = NULL;
    if ( !fv->b.sf->changed ) {
	fv->b.sf->changed = true;
	FVSetTitles(fv->b.sf);
    }
}

static void sm_hintClearInstructions(CommonView* self) {
    FontView* fv = tryObtainCastFontView( self );
    FVClearInstrs(&fv->b);
}

static void sm_hintClear(CommonView* self) {
    FontView* fv = tryObtainCastFontView( self );
    FVClearHints(&fv->b);
}

static void sm_histogramHStemDialog(CommonView* self) {
    FontView* fv = tryObtainCastFontView( self );
    SFHistogram( fv->b.sf, fv->b.active_layer, NULL,
		 FVAnyCharSelected(fv)!=-1?fv->b.selected:NULL,
		 fv->b.map, hist_hstem );
    
}
static void sm_histogramVStemDialog(CommonView* self) {
    FontView* fv = tryObtainCastFontView( self );
    SFHistogram( fv->b.sf, fv->b.active_layer, NULL,
		 FVAnyCharSelected(fv)!=-1?fv->b.selected:NULL,
		 fv->b.map, hist_vstem );
}
static void sm_histogramBlueValuesDialog(CommonView* self) {
    FontView* fv = tryObtainCastFontView( self );
    SFHistogram( fv->b.sf, fv->b.active_layer, NULL,
		 FVAnyCharSelected(fv)!=-1?fv->b.selected:NULL,
		 fv->b.map, hist_blues );
}



static void FontViewSetTitle(FontView *fv) {
    unichar_t *title, *ititle, *temp;
    char *file=NULL;
    char *enc;
    int len;

    if ( fv->gw==NULL )		/* In scripting */
return;

    const char* collabStateString = "";
    if( collabclient_inSessionFV( &fv->b )) {
	printf("collabclient_getState( fv ) %d %d\n",
	       fv->b.collabState, collabclient_getState( &fv->b ));
	collabStateString = collabclient_stateToString(collabclient_getState( &fv->b ));
    }

    enc = SFEncodingName(fv->b.sf,fv->b.normal?fv->b.normal:fv->b.map);
    len = strlen(fv->b.sf->fontname)+1 + strlen(enc)+6;
    if ( fv->b.normal ) len += strlen(_("Compact"))+1;
    if ( fv->b.cidmaster!=NULL ) {
	if ( (file = fv->b.cidmaster->filename)==NULL )
	    file = fv->b.cidmaster->origname;
    } else {
	if ( (file = fv->b.sf->filename)==NULL )
	    file = fv->b.sf->origname;
    }
    len += strlen(collabStateString);
    if ( file!=NULL )
	len += 2+strlen(file);
    title = malloc((len+1)*sizeof(unichar_t));
    uc_strcpy(title,"");

    if(*collabStateString) {
	uc_strcat(title, collabStateString);
	uc_strcat(title, " - ");
    }
    uc_strcat(title,fv->b.sf->fontname);
    if ( fv->b.sf->changed )
	uc_strcat(title,"*");
    if ( file!=NULL ) {
	uc_strcat(title,"  ");
	temp = def2u_copy(GFileNameTail(file));
	u_strcat(title,temp);
	free(temp);
    }
    uc_strcat(title, " (" );
    if ( fv->b.normal ) { utf82u_strcat(title,_("Compact")); uc_strcat(title," "); }
    uc_strcat(title,enc);
    uc_strcat(title, ")" );
    free(enc);

    ititle = uc_copy(fv->b.sf->fontname);
    GDrawSetWindowTitles(fv->gw,title,ititle);
    free(title);
    free(ititle);
}

void FVTitleUpdate(FontViewBase *fv)
{
    FontViewSetTitle( (FontView*)fv );
}

static void FontViewSetTitles(SplineFont *sf) {
    FontView *fv;

    for ( fv = (FontView *) (sf->fv); fv!=NULL; fv=(FontView *) (fv->b.nextsame))
	FontViewSetTitle(fv);
}

void FVMenuShowSubFont(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e)) {
    FontView* fv = tryObtainGDataFontView( gw );
    SplineFont *new = mi->ti.userdata;
    FVShowSubFont(fv,new);
}

void FVMenuConvert2CID(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    FontView* fv = tryObtainGDataFontView( gw );
    SplineFont *cidmaster = fv->b.cidmaster;
    struct cidmap *cidmap;

    if ( cidmaster!=NULL )
return;
    SFFindNearTop(fv->b.sf);
    cidmap = AskUserForCIDMap();
    if ( cidmap==NULL )
return;
    MakeCIDMaster(fv->b.sf,fv->b.map,false,NULL,cidmap);
    SFRestoreNearTop(fv->b.sf);
}

static enum fchooserret CMapFilter(GGadget *g,GDirEntry *ent,
	const unichar_t *dir) {
    enum fchooserret ret = GFileChooserDefFilter(g,ent,dir);
    char buf2[256];
    FILE *file;
    static char *cmapflag = "%!PS-Adobe-3.0 Resource-CMap";

    if ( ret==fc_show && !ent->isdir ) {
	int len = 3*(u_strlen(dir)+u_strlen(ent->name)+5);
	char *filename = malloc(len);
	u2def_strncpy(filename,dir,len);
	strcat(filename,"/");
	u2def_strncpy(buf2,ent->name,sizeof(buf2));
	strcat(filename,buf2);
	file = fopen(filename,"r");
	if ( file==NULL )
	    ret = fc_hide;
	else {
	    if ( fgets(buf2,sizeof(buf2),file)==NULL ||
		    strncmp(buf2,cmapflag,strlen(cmapflag))!=0 )
		ret = fc_hide;
	    fclose(file);
	}
	free(filename);
    }
return( ret );
}

void FVMenuConvertByCMap(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    FontView* fv = tryObtainGDataFontView( gw );
    SplineFont *cidmaster = fv->b.cidmaster;
    char *cmapfilename;

    if ( cidmaster!=NULL )
return;
    cmapfilename = gwwv_open_filename(_("Find an adobe CMap file..."),NULL,NULL,CMapFilter);
    if ( cmapfilename==NULL )
return;
    MakeCIDMaster(fv->b.sf,fv->b.map,true,cmapfilename,NULL);
    free(cmapfilename);
}

void FVMenuFlatten(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    FontView* fv = tryObtainGDataFontView( gw );
    SplineFont *cidmaster = fv->b.cidmaster;

    if ( cidmaster==NULL )
return;
    SFFlatten(cidmaster);
}

void FVMenuFlattenByCMap(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    FontView* fv = tryObtainGDataFontView( gw );
    SplineFont *cidmaster = fv->b.cidmaster;
    char *cmapname;

    if ( cidmaster==NULL )
return;
    cmapname = gwwv_open_filename(_("Find an adobe CMap file..."),NULL,NULL,CMapFilter);
    if ( cmapname==NULL )
return;
    SFFindNearTop(fv->b.sf);
    SFFlattenByCMap(cidmaster,cmapname);
    SFRestoreNearTop(fv->b.sf);
    free(cmapname);
}

void FVMenuInsertFont(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    FontView* fv = tryObtainGDataFontView( gw );
    SplineFont *cidmaster = fv->b.cidmaster;
    SplineFont *new;
    struct cidmap *map;
    char *filename;
    extern NameList *force_names_when_opening;

    if ( cidmaster==NULL || cidmaster->subfontcnt>=255 )	/* Open type allows 1 byte to specify the fdselect */
return;

    filename = GetPostScriptFontName(NULL,false);
    if ( filename==NULL )
return;
    new = LoadSplineFont(filename,0);
    free(filename);
    if ( new==NULL )
return;
    if ( new->fv == &fv->b )		/* Already part of us */
return;
    if ( new->fv != NULL ) {
	if ( ((FontView *) (new->fv))->gw!=NULL )
	    GDrawRaise( ((FontView *) (new->fv))->gw);
	ff_post_error(_("Please close font"),_("Please close %s before inserting it into a CID font"),new->origname);
return;
    }
    EncMapFree(new->map);
    if ( force_names_when_opening!=NULL )
	SFRenameGlyphsToNamelist(new,force_names_when_opening );

    map = FindCidMap(cidmaster->cidregistry,cidmaster->ordering,cidmaster->supplement,cidmaster);
    SFEncodeToMap(new,map);
    if ( !PSDictHasEntry(new->private,"lenIV"))
	PSDictChangeEntry(new->private,"lenIV","1");		/* It's 4 by default, in CIDs the convention seems to be 1 */
    new->display_antialias = fv->b.sf->display_antialias;
    new->display_bbsized = fv->b.sf->display_bbsized;
    new->display_size = fv->b.sf->display_size;
    FVInsertInCID((FontViewBase *) fv,new);
    CIDMasterAsDes(new);
}

void FVMenuInsertBlank(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    FontView* fv = tryObtainGDataFontView( gw );
    SplineFont *cidmaster = fv->b.cidmaster, *sf;
    struct cidmap *map;

    if ( cidmaster==NULL || cidmaster->subfontcnt>=255 )	/* Open type allows 1 byte to specify the fdselect */
return;
    map = FindCidMap(cidmaster->cidregistry,cidmaster->ordering,cidmaster->supplement,cidmaster);
    sf = SplineFontBlank(MaxCID(map));
    sf->glyphcnt = sf->glyphmax;
    sf->cidmaster = cidmaster;
    sf->display_antialias = fv->b.sf->display_antialias;
    sf->display_bbsized = fv->b.sf->display_bbsized;
    sf->display_size = fv->b.sf->display_size;
    sf->private = calloc(1,sizeof(struct psdict));
    PSDictChangeEntry(sf->private,"lenIV","1");		/* It's 4 by default, in CIDs the convention seems to be 1 */
    FVInsertInCID((FontViewBase *) fv,sf);
}

void FVMenuRemoveFontFromCID(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    FontView* fv = tryObtainGDataFontView( gw );
    char *buts[3];
    SplineFont *cidmaster = fv->b.cidmaster, *sf = fv->b.sf, *replace;
    int i;
    MetricsView *mv, *mnext;
    FontView *fvs;

    if ( cidmaster==NULL || cidmaster->subfontcnt<=1 )	/* Can't remove last font */
return;
    buts[0] = _("_Remove"); buts[1] = _("_Cancel"); buts[2] = NULL;
    if ( gwwv_ask(_("_Remove Font"),(const char **) buts,0,1,_("Are you sure you wish to remove sub-font %1$.40s from the CID font %2$.40s"),
	    sf->fontname,cidmaster->fontname)==1 )
return;

    for ( i=0; i<sf->glyphcnt; ++i ) if ( sf->glyphs[i]!=NULL ) {
	CharView *cv, *next;
	for ( cv = (CharView *) (sf->glyphs[i]->views); cv!=NULL; cv = next ) {
	    next = (CharView *) (cv->b.next);
	    GDrawDestroyWindow(cv->gw);
	}
    }
    GDrawProcessPendingEvents(NULL);
    for ( mv=fv->b.sf->metrics; mv!=NULL; mv = mnext ) {
	mnext = mv->next;
	GDrawDestroyWindow(mv->gw);
    }
    GDrawSync(NULL);
    GDrawProcessPendingEvents(NULL);
    /* Just in case... */
    GDrawSync(NULL);
    GDrawProcessPendingEvents(NULL);

    for ( i=0; i<cidmaster->subfontcnt; ++i )
	if ( cidmaster->subfonts[i]==sf )
    break;
    replace = i==0?cidmaster->subfonts[1]:cidmaster->subfonts[i-1];
    while ( i<cidmaster->subfontcnt-1 ) {
	cidmaster->subfonts[i] = cidmaster->subfonts[i+1];
	++i;
    }
    --cidmaster->subfontcnt;

    for ( fvs=(FontView *) (fv->b.sf->fv); fvs!=NULL; fvs=(FontView *) (fvs->b.nextsame) ) {
	if ( fvs->b.sf==sf )
	    CIDSetEncMap((FontViewBase *) fvs,replace);
    }
    FontViewReformatAll(fv->b.sf);
    SplineFontFree(sf);
}

void FVMenuCIDFontInfo(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    FontView* fv = tryObtainGDataFontView( gw );
    SplineFont *cidmaster = fv->b.cidmaster;

    if ( cidmaster==NULL )
return;
    FontInfo(cidmaster,fv->b.active_layer,-1,false);
}

void FVMenuChangeSupplement(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    FontView* fv = tryObtainGDataFontView( gw );
    SplineFont *cidmaster = fv->b.cidmaster;
    struct cidmap *cidmap;
    char buffer[20];
    char *ret, *end;
    int supple;

    if ( cidmaster==NULL )
return;
    sprintf(buffer,"%d",cidmaster->supplement);
    ret = gwwv_ask_string(_("Change Supplement..."),buffer,_("Please specify a new supplement for %.20s-%.20s"),
	    cidmaster->cidregistry,cidmaster->ordering);
    if ( ret==NULL )
return;
    supple = strtol(ret,&end,10);
    if ( *end!='\0' || supple<=0 ) {
	free(ret);
	ff_post_error( _("Bad Number"),_("Bad Number") );
return;
    }
    free(ret);
    if ( supple!=cidmaster->supplement ) {
	    /* this will make noises if it can't find an appropriate cidmap */
	cidmap = FindCidMap(cidmaster->cidregistry,cidmaster->ordering,supple,cidmaster);
	cidmaster->supplement = supple;
	FontViewSetTitle(fv);
    }
}

static SplineChar *FVFindACharInDisplay(FontView *fv) {
    int start, end, enc, gid;
    EncMap *map = fv->b.map;
    SplineFont *sf = fv->b.sf;
    SplineChar *sc;

    start = fv->rowoff*fv->colcnt;
    end = start + fv->rowcnt*fv->colcnt;
    for ( enc = start; enc<end && enc<map->enccount; ++enc ) {
	if ( (gid=map->map[enc])!=-1 && (sc=sf->glyphs[gid])!=NULL )
return( sc );
    }
return( NULL );
}

static void FVMenuReencode(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e)) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
    Encoding *enc = NULL;
    SplineChar *sc;

    sc = FVFindACharInDisplay(fv);
    enc = FindOrMakeEncoding(mi->ti.userdata);
    if ( enc==NULL ) {
	IError("Known encoding could not be found");
return;
    }
    FVReencode((FontViewBase *) fv,enc);
    if ( sc!=NULL ) {
	int enc = fv->b.map->backmap[sc->orig_pos];
	if ( enc!=-1 )
	    FVScrollToChar(fv,enc);
    }
}

static void FVMenuForceEncode(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e)) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
    Encoding *enc = NULL;
    int oldcnt = fv->b.map->enccount;

    enc = FindOrMakeEncoding(mi->ti.userdata);
    if ( enc==NULL ) {
	IError("Known encoding could not be found");
return;
    }
    SFForceEncoding(fv->b.sf,fv->b.map,enc);
    if ( oldcnt < fv->b.map->enccount ) {
	fv->b.selected = realloc(fv->b.selected,fv->b.map->enccount);
	memset(fv->b.selected+oldcnt,0,fv->b.map->enccount-oldcnt);
    }
    if ( fv->b.normal!=NULL ) {
	EncMapFree(fv->b.normal);
	if (fv->b.normal == fv->b.sf->map) { fv->b.sf->map = NULL; }
	fv->b.normal = NULL;
    }
    SFReplaceEncodingBDFProps(fv->b.sf,fv->b.map);
    FontViewSetTitle(fv);
    FontViewReformatOne(&fv->b);
}

static void FVMenuDisplayByGroups(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);

    DisplayGroups(fv);
}

static void FVMenuDefineGroups(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);

    DefineGroups(fv);
}

void FVMenuMMValid(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    FontView* fv = tryObtainGDataFontView( gw );
    MMSet *mm = fv->b.sf->mm;

    if ( mm==NULL )
return;
    MMValid(mm,true);
}

void FVMenuCreateMM(GWindow UNUSED(gw), struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    MMWizard(NULL);
}

void FVMenuMMInfo(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    FontView* fv = tryObtainGDataFontView( gw );
    MMSet *mm = fv->b.sf->mm;

    if ( mm==NULL )
return;
    MMWizard(mm);
}

void FVMenuChangeMMBlend(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    FontView* fv = tryObtainGDataFontView( gw );
    MMSet *mm = fv->b.sf->mm;

    if ( mm==NULL || mm->apple )
return;
    MMChangeBlend(mm,fv,false);
}

void FVMenuBlendToNew(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    FontView* fv = tryObtainGDataFontView( gw );
    MMSet *mm = fv->b.sf->mm;

    if ( mm==NULL )
return;
    MMChangeBlend(mm,fv,true);
}


#if HANYANG
static void hglistcheck(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e)) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);

    for ( mi = mi->sub; mi->ti.text!=NULL || mi->ti.line ; ++mi ) {
        if ( mi->mid==SMID_BuildSyllables || mi->mid==SMID_ModifyComposition )
	    mi->ti.disabled = fv->b.sf->rules==NULL;
    }
}

static GMenuItem2 hglist[] = {
    { { (unichar_t *) N_("_New Composition..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'N' }, H_("New Composition...|No Shortcut"), NULL, NULL, MenuNewComposition },
    { { (unichar_t *) N_("_Modify Composition..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'M' }, H_("Modify Composition...|No Shortcut"), NULL, NULL, FVMenuModifyComposition, SMID_ModifyComposition },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) N_("_Build Syllables"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'B' }, H_("Build Syllables|No Shortcut"), NULL, NULL, FVMenuBuildSyllables, SMID_BuildSyllables },
    { NULL }
};
#endif



void FVMenuAddUnencoded(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
    char *ret, *end;
    int cnt;

    ret = gwwv_ask_string(_("Add Encoding Slots..."),"1",fv->b.cidmaster?_("How many CID slots do you wish to add?"):_("How many unencoded glyph slots do you wish to add?"));
    if ( ret==NULL )
return;
    cnt = strtol(ret,&end,10);
    if ( *end!='\0' || cnt<=0 ) {
	free(ret);
	ff_post_error( _("Bad Number"),_("Bad Number") );
return;
    }
    free(ret);
    FVAddUnencoded((FontViewBase *) fv, cnt);
}

void FVMenuRemoveUnused(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
    FVRemoveUnused((FontViewBase *) fv);
}

void FVEncodingMenuBuild(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e)) {
    FontView* fv = tryObtainGDataFontView( gw );

    if ( mi->sub!=NULL ) {
	GMenuItemArrayFree(mi->sub);
	mi->sub = NULL;
    }
    mi->sub = GetEncodingMenu(FVMenuReencode,fv->b.map->enc);
}

void FVMenuCompact(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    FontView* fv = tryObtainGDataFontView( gw );
    SplineChar *sc;

    sc = FVFindACharInDisplay(fv);
    FVCompact((FontViewBase *) fv);
    if ( sc!=NULL ) {
	int enc = fv->b.map->backmap[sc->orig_pos];
	if ( enc!=-1 )
	    FVScrollToChar(fv,enc);
    }
}

void FVMenuDetachGlyphs(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
    FVDetachGlyphs((FontViewBase *) fv);
}

void FVMenuDetachAndRemoveGlyphs(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
    char *buts[3];

    buts[0] = _("_Remove");
    buts[1] = _("_Cancel");
    buts[2] = NULL;

    if ( gwwv_ask(_("Detach & Remove Glyphs"),(const char **) buts,0,1,_("Are you sure you wish to remove these glyphs? This operation cannot be undone."))==1 )
return;

    FVDetachAndRemoveGlyphs((FontViewBase *) fv);
}

void FVForceEncodingMenuBuild(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e)) {
    FontView* fv = tryObtainGDataFontView( gw );

    if ( mi->sub!=NULL ) {
	GMenuItemArrayFree(mi->sub);
	mi->sub = NULL;
    }
    mi->sub = GetEncodingMenu(FVMenuForceEncode,fv->b.map->enc);
}

void FVMenuAddEncodingName(GWindow UNUSED(gw), struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    char *ret;
    Encoding *enc;

    /* Search the iconv database for the named encoding */
    ret = gwwv_ask_string(_("Add Encoding Name..."),NULL,_("Please provide the name of an encoding in the iconv database which you want in the menu."));
    if ( ret==NULL )
return;
    enc = FindOrMakeEncoding(ret);
    if ( enc==NULL )
	ff_post_error(_("Invalid Encoding"),_("Invalid Encoding"));
    free(ret);
}

void FVMenuLoadEncoding(GWindow UNUSED(gw), struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    LoadEncodingFile();
}

void FVMenuMakeFromFont(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    FontView* fv = tryObtainGDataFontView( gw );
    (void) MakeEncoding(fv->b.sf,fv->b.map);
}

void FVMenuRemoveEncoding(GWindow UNUSED(gw), struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    RemoveEncoding();
}

void FVMenuMakeNamelist(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
    char buffer[1025];
    char *filename, *temp;
    FILE *file;

    snprintf(buffer, sizeof(buffer),"%s/%s.nam", getFontForgeUserDir(Config), fv->b.sf->fontname );
    temp = def2utf8_copy(buffer);
    filename = gwwv_save_filename(_("Make Namelist"), temp,"*.nam");
    free(temp);
    if ( filename==NULL )
return;
    temp = utf82def_copy(filename);
    file = fopen(temp,"w");
    free(temp);
    if ( file==NULL ) {
	ff_post_error(_("Namelist creation failed"),_("Could not write %s"), filename);
	free(filename);
return;
    }
    FVB_MakeNamelist((FontViewBase *) fv, file);
    fclose(file);
}

void FVMenuLoadNamelist(GWindow UNUSED(gw), struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    /* Read in a name list and copy it into the prefs dir so that we'll find */
    /*  it in the future */
    /* Be prepared to update what we've already got if names match */
    char buffer[1025];
    char *ret = gwwv_open_filename(_("Load Namelist"),NULL,
	    "*.nam",NULL);
    char *temp, *pt;
    char *buts[3];
    FILE *old, *new;
    int ch, ans;
    NameList *nl;

    if ( ret==NULL )
return;				/* Cancelled */
    temp = utf82def_copy(ret);
    pt = strrchr(temp,'/');
    if ( pt==NULL )
	pt = temp;
    else
	++pt;
    snprintf(buffer,sizeof(buffer),"%s/%s", getFontForgeUserDir(Config), pt);
    if ( access(buffer,F_OK)==0 ) {
	buts[0] = _("_Replace");
	buts[1] = _("_Cancel");
	buts[2] = NULL;
	ans = gwwv_ask( _("Replace"),(const char **) buts,0,1,_("A name list with this name already exists. Replace it?"));
	if ( ans==1 ) {
	    free(temp);
	    free(ret);
return;
	}
    }

    old = fopen( temp,"r");
    if ( old==NULL ) {
	ff_post_error(_("No such file"),_("Could not read %s"), ret );
	free(ret); free(temp);
return;
    }
    if ( (nl = LoadNamelist(temp))==NULL ) {
	ff_post_error(_("Bad namelist file"),_("Could not parse %s"), ret );
	free(ret); free(temp);
        fclose(old);
return;
    }
    free(ret); free(temp);
    if ( nl->uses_unicode ) {
	if ( nl->a_utf8_name!=NULL )
	    ff_post_notice(_("Non-ASCII glyphnames"),_("This namelist contains at least one non-ASCII glyph name, namely: %s"), nl->a_utf8_name );
	else
	    ff_post_notice(_("Non-ASCII glyphnames"),_("This namelist is based on a namelist which contains non-ASCII glyph names"));
    }

    new = fopen( buffer,"w");
    if ( new==NULL ) {
	ff_post_error(_("Create failed"),_("Could not write %s"), buffer );
        fclose(old);
return;
    }

    while ( (ch=getc(old))!=EOF )
	putc(ch,new);
    fclose(old);
    fclose(new);
}

void FVMenuRenameByNamelist(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
    char **namelists = AllNamelistNames();
    int i;
    int ret;
    NameList *nl;
    extern int allow_utf8_glyphnames;

    for ( i=0; namelists[i]!=NULL; ++i );
    ret = gwwv_choose(_("Rename by NameList"),(const char **) namelists,i,0,_("Rename the glyphs in this font to the names found in the selected namelist"));
    if ( ret==-1 )
return;
    nl = NameListByName(namelists[ret]);
    if ( nl==NULL ) {
	IError("Couldn't find namelist");
return;
    } else if ( nl!=NULL && nl->uses_unicode && !allow_utf8_glyphnames) {
	ff_post_error(_("Namelist contains non-ASCII names"),_("Glyph names should be limited to characters in the ASCII character set, but there are names in this namelist which use characters outside that range."));
return;
    }
    SFRenameGlyphsToNamelist(fv->b.sf,nl);
    GDrawRequestExpose(fv->v,NULL,false);
}

void FVMenuNameGlyphs(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
    /* Read a file containing a list of names, and add an unencoded glyph for */
    /*  each name */
    char buffer[33];
    char *ret = gwwv_open_filename(_("Load glyph names"),NULL, "*",NULL);
    char *temp, *pt;
    FILE *file;
    int ch;
    SplineChar *sc;
    FontView *fvs;

    if ( ret==NULL )
return;				/* Cancelled */
    temp = utf82def_copy(ret);

    file = fopen( temp,"r");
    if ( file==NULL ) {
	ff_post_error(_("No such file"),_("Could not read %s"), ret );
	free(ret); free(temp);
return;
    }
    pt = buffer;
    for (;;) {
	ch = getc(file);
	if ( ch!=EOF && !isspace(ch)) {
	    if ( pt<buffer+sizeof(buffer)-1 )
		*pt++ = ch;
	} else {
	    if ( pt!=buffer ) {
		*pt = '\0';
		sc = NULL;
		for ( fvs=(FontView *) (fv->b.sf->fv); fvs!=NULL; fvs=(FontView *) (fvs->b.nextsame) ) {
		    EncMap *map = fvs->b.map;
		    if ( map->enccount+1>=map->encmax )
			map->map = realloc(map->map,(map->encmax += 20)*sizeof(int));
		    map->map[map->enccount] = -1;
		    fvs->b.selected = realloc(fvs->b.selected,(map->enccount+1));
		    memset(fvs->b.selected+map->enccount,0,1);
		    ++map->enccount;
		    if ( sc==NULL ) {
			sc = SFMakeChar(fv->b.sf,map,map->enccount-1);
			free(sc->name);
			sc->name = copy(buffer);
			sc->comment = copy(".");	/* Mark as something for sfd file */
		    }
		    map->map[map->enccount-1] = sc->orig_pos;
		    map->backmap[sc->orig_pos] = map->enccount-1;
		}
		pt = buffer;
	    }
	    if ( ch==EOF )
    break;
	}
    }
    fclose(file);
    free(ret); free(temp);
    FontViewReformatAll(fv->b.sf);
}




void FVMenuCollabStart(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e))
{
#ifdef BUILD_COLLAB
    CommonView* cc = tryObtainGDataCommonView( gw );
    FontView* fv = tryObtainGDataFontView( gw );

    printf("connecting to server and sending initial SFD to it...\n");

    int port_default = 5556;
    int port = port_default;
    char address[IPADDRESS_STRING_LENGTH_T];
    if( !getNetworkAddress( address ))
    {
	snprintf( address, IPADDRESS_STRING_LENGTH_T-1,
		  "%s", HostPortPack( "127.0.0.1", port ));
    }
    else
    {
	snprintf( address, IPADDRESS_STRING_LENGTH_T-1,
		  "%s", HostPortPack( address, port ));
    }

    printf("host address:%s\n",address);

    char* res = gwwv_ask_string(
	"Starting Collab Server",
	address,
	"FontForge has determined that your computer can be accessed"
	" using the below address. Share that address with other people"
	" who you wish to collaborate with...\n\nPress OK to start the collaboration server...");

    if( res )
    {
	printf("res:%s\n", res );
	strncpy( address, res, IPADDRESS_STRING_LENGTH_T );
	HostPortUnpack( address, &port, port_default );

	printf("address:%s\n", address );
	printf("port:%d\n", port );

	void* cc = collabclient_new( address, port );
	fv->b.collabClient = cc;
	collabclient_sessionStart( cc, fv );
	printf("connecting to server...sent the sfd for session start.\n");
    }
#endif
}

static int collab_MakeChoicesArray( GHashTable* peers, char** choices, int choices_sz, int localOnly )
{
    GHashTableIter iter;
    gpointer key, value;
    int lastidx = 0;
    memset( choices, 0, sizeof(char*) * choices_sz );

    int i=0;
    int maxUserNameLength = 1;
    g_hash_table_iter_init (&iter, peers);
    for( i=0; g_hash_table_iter_next (&iter, &key, &value); i++ )
    {
	beacon_announce_t* ba = (beacon_announce_t*)value;
	maxUserNameLength = imax( maxUserNameLength, strlen(ba->username) );
    }

    g_hash_table_iter_init (&iter, peers);
    for( i=0; g_hash_table_iter_next (&iter, &key, &value); i++ )
    {
	beacon_announce_t* ba = (beacon_announce_t*)value;
	if( localOnly && !collabclient_isAddressLocal( ba->ip ))
	    continue;

	printf("user:%s\n", ba->username );
	printf("mach:%s\n", ba->machinename );

	char buf[101];
	if( localOnly )
	{
	    snprintf( buf, 100, "%s", ba->fontname );
	}
	else
	{
	    char format[50];
	    sprintf( format, "%s %%%d", "%s", maxUserNameLength );
	    strcat( format, "s %s");
	    snprintf( buf, 100, format, ba->fontname, ba->username, ba->machinename );
	}
	choices[i] = copy( buf );
	if( i >= choices_sz )
	    break;
	lastidx++;
    }

    return lastidx;
}

static beacon_announce_t* collab_getBeaconFromChoicesArray( GHashTable* peers, int choice, int localOnly )
{
    GHashTableIter iter;
    gpointer key, value;
    int i=0;

    g_hash_table_iter_init (&iter, peers);
    for( i=0; g_hash_table_iter_next (&iter, &key, &value); i++ )
    {
	beacon_announce_t* ba = (beacon_announce_t*)value;
	if( localOnly && !collabclient_isAddressLocal( ba->ip ))
	    continue;
	if( i != choice )
	    continue;
	return ba;
    }
    return 0;
}

void FVMenuCollabConnect(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e))
{
#ifdef BUILD_COLLAB
    
    CommonView* cc = tryObtainGDataCommonView( gw );
    FontView* fv = tryObtainGDataFontView( gw );

    printf("connecting to server...\n");

    {
	int choices_sz = 100;
	char* choices[101];
	memset( choices, 0, sizeof(choices));

	collabclient_trimOldBeaconInformation( 0 );
	GHashTable* peers = collabclient_getServersFromBeaconInfomration();
	int localOnly = 0;
	int max = collab_MakeChoicesArray( peers, choices, choices_sz, localOnly );
	int choice = gwwv_choose(_("Connect to Collab Server"),(const char **) choices, max,
				 0,_("Select a collab server to connect to..."));
	printf("you wanted %d\n", choice );
	if( choice <= max )
	{
	    beacon_announce_t* ba = collab_getBeaconFromChoicesArray( peers, choice, localOnly );

	    if( ba )
	    {
		int port = ba->port;
		char address[IPADDRESS_STRING_LENGTH_T];
		strncpy( address, ba->ip, IPADDRESS_STRING_LENGTH_T-1 );
		void* cc = collabclient_new( address, port );
		fv->b.collabClient = cc;
		collabclient_sessionJoin( cc, fv );
	    }
	}
    }

    printf("FVMenuCollabConnect(done)\n");
#endif
}

void FVMenuCollabConnectToExplicitAddress(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e))
{
#ifdef BUILD_COLLAB
    
    CommonView* cc = tryObtainGDataCommonView( gw );
    FontView* fv = tryObtainGDataFontView( gw );

    printf("********** connecting to server... explicit address... p:%p\n", pref_collab_last_server_connected_to);

    char* default_server = "localhost";
    if( pref_collab_last_server_connected_to ) {
	default_server = pref_collab_last_server_connected_to;
    }
    
    char* res = gwwv_ask_string(
    	"Connect to Collab Server",
    	default_server,
    	"Please enter the network location of the Collab server you wish to connect to...");
    if( res )
    {
	if( pref_collab_last_server_connected_to ) {
	    free( pref_collab_last_server_connected_to );
	}
	pref_collab_last_server_connected_to = copy( res );
	SavePrefs(true);
	
    	int port_default = 5556;
    	int port = port_default;
    	char address[IPADDRESS_STRING_LENGTH_T];
    	strncpy( address, res, IPADDRESS_STRING_LENGTH_T-1 );
    	HostPortUnpack( address, &port, port_default );

    	void* cc = collabclient_new( address, port );
    	fv->b.collabClient = cc;
    	collabclient_sessionJoin( cc, fv );
    }

    printf("FVMenuCollabConnect(done)\n");
    
#endif
}

void FVMenuCollabDisconnect(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e))
{
#ifdef BUILD_COLLAB
    
    CommonView* cc = tryObtainGDataCommonView( gw );
    FontView* fv = tryObtainGDataFontView( gw );
    collabclient_sessionDisconnect( &fv->b );
    
#endif
}

static void AskAndMaybeCloseLocalCollabServers()
{
#ifdef BUILD_COLLAB
    
    char *buts[3];
    buts[0] = _("_OK");
    buts[1] = _("_Cancel");
    buts[2] = NULL;

    int i=0;
    int choices_sz = 100;
    char* choices[101];
    collabclient_trimOldBeaconInformation( 0 );
    GHashTable* peers = collabclient_getServersFromBeaconInfomration();
    if( !peers )
	return;
    
    int localOnly = 1;
    int max = collab_MakeChoicesArray( peers, choices, choices_sz, localOnly );
    if( !max )
	return;

    char sel[101];
    memset( sel, 1, max );
    int choice = gwwv_choose_multiple(_("Close Collab Server(s)"),
				      (const char **) choices, sel, max,
				      buts, _("Select which servers you wish to close..."));

    int allServersSelected = 1;
    printf("you wanted %d\n", choice );
    for( i=0; i < max; i++ )
    {
	printf("sel[%d] is %d\n", i, sel[i] );
	beacon_announce_t* ba = collab_getBeaconFromChoicesArray( peers, choice, localOnly );

	if( sel[i] && ba )
	{
	    int port = ba->port;

	    if( sel[i] )
	    {
		FontViewBase* fv = FontViewFind( FontViewFind_byCollabBasePort, (void*)(intptr_t)port );
		if( fv )
		    collabclient_sessionDisconnect( fv );
		printf("CLOSING port:%d fv:%p\n", port, fv );
		collabclient_closeLocalServer( port );
	    }
	}
	else
	{
	    allServersSelected = 0;
	}
    }

    printf("allServersSelected:%d\n", allServersSelected );
    if( allServersSelected )
	collabclient_closeAllLocalServersForce();

#endif
}

void FVMenuCollabCloseLocalServer(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e))
{
    AskAndMaybeCloseLocalCollabServers();
}

#if defined(__MINGW32__)
//
// This is an imperfect implemenation of kill() for windows.
//
static int kill( int pid, int sig )
{
    HANDLE hHandle;
    hHandle = OpenProcess( PROCESS_ALL_ACCESS, 0, pid );
    TerminateProcess( hHandle, 0 );
}
#endif



void FVChar(FontView *fv, GEvent *event) {
    int i,pos, cnt, gid;
    extern int navigation_mask;

#if MyMemory
    if ( event->u.chr.keysym == GK_F2 ) {
	fprintf( stderr, "Malloc debug on\n" );
	__malloc_debug(5);
    } else if ( event->u.chr.keysym == GK_F3 ) {
	fprintf( stderr, "Malloc debug off\n" );
	__malloc_debug(0);
    }
#endif

    if ( event->u.chr.keysym=='s' &&
	    (event->u.chr.state&ksm_control) &&
	    (event->u.chr.state&ksm_meta) )
	MenuSaveAll(NULL,NULL,NULL);
    else if ( event->u.chr.keysym=='q' &&
	    (event->u.chr.state&ksm_control) &&
	    (event->u.chr.state&ksm_meta) )
	MenuExit(NULL,NULL,NULL);
    else if ( event->u.chr.keysym=='I' &&
	    (event->u.chr.state&ksm_shift) &&
	    (event->u.chr.state&ksm_meta) )
	FVMenuCharInfo(fv->gw,NULL,NULL);
    else if ( (event->u.chr.keysym=='[' || event->u.chr.keysym==']') &&
	    (event->u.chr.state&ksm_control) ) {
	_FVMenuChangeChar(fv,event->u.chr.keysym=='['?SMID_Prev:SMID_Next);
    } else if ( (event->u.chr.keysym=='{' || event->u.chr.keysym=='}') &&
	    (event->u.chr.state&ksm_control) ) {
	_FVMenuChangeChar(fv,event->u.chr.keysym=='{'?SMID_PrevDef:SMID_NextDef);
    } else if ( event->u.chr.keysym=='\\' && (event->u.chr.state&ksm_control) ) {
	/* European keyboards need a funky modifier to get \ */
	FVDoTransform(fv);
#if !defined(_NO_FFSCRIPT) || !defined(_NO_PYTHON)
    } else if ( isdigit(event->u.chr.keysym) && (event->u.chr.state&ksm_control) &&
	    (event->u.chr.state&ksm_meta) ) {
	/* The Script menu isn't always up to date, so we might get one of */
	/*  the shortcuts here */
	int index = event->u.chr.keysym-'1';
	if ( index<0 ) index = 9;
	if ( script_filenames[index]!=NULL )
	    ExecuteScriptFile((FontViewBase *) fv,NULL,script_filenames[index]);
#endif
    } else if ( event->u.chr.keysym == GK_Left ||
	    event->u.chr.keysym == GK_Tab ||
	    event->u.chr.keysym == GK_BackTab ||
	    event->u.chr.keysym == GK_Up ||
	    event->u.chr.keysym == GK_Right ||
	    event->u.chr.keysym == GK_Down ||
	    event->u.chr.keysym == GK_KP_Left ||
	    event->u.chr.keysym == GK_KP_Up ||
	    event->u.chr.keysym == GK_KP_Right ||
	    event->u.chr.keysym == GK_KP_Down ||
	    event->u.chr.keysym == GK_Home ||
	    event->u.chr.keysym == GK_KP_Home ||
	    event->u.chr.keysym == GK_End ||
	    event->u.chr.keysym == GK_KP_End ||
	    event->u.chr.keysym == GK_Page_Up ||
	    event->u.chr.keysym == GK_KP_Page_Up ||
	    event->u.chr.keysym == GK_Prior ||
	    event->u.chr.keysym == GK_Page_Down ||
	    event->u.chr.keysym == GK_KP_Page_Down ||
	    event->u.chr.keysym == GK_Next ) {
	int end_pos = fv->end_pos;
	/* We move the currently selected char. If there is none, then pick */
	/*  something on the screen */
	if ( end_pos==-1 )
	    end_pos = (fv->rowoff+fv->rowcnt/2)*fv->colcnt;
	switch ( event->u.chr.keysym ) {
	  case GK_Tab:
	    pos = end_pos;
	    do {
		if ( event->u.chr.state&ksm_shift )
		    --pos;
		else
		    ++pos;
		if ( pos>=fv->b.map->enccount ) pos = 0;
		else if ( pos<0 ) pos = fv->b.map->enccount-1;
	    } while ( pos!=end_pos &&
		    ((gid=fv->b.map->map[pos])==-1 || !SCWorthOutputting(fv->b.sf->glyphs[gid])));
	    if ( pos==end_pos ) ++pos;
	    if ( pos>=fv->b.map->enccount ) pos = 0;
	  break;
#if GK_Tab!=GK_BackTab
	  case GK_BackTab:
	    pos = end_pos;
	    do {
		--pos;
		if ( pos<0 ) pos = fv->b.map->enccount-1;
	    } while ( pos!=end_pos &&
		    ((gid=fv->b.map->map[pos])==-1 || !SCWorthOutputting(fv->b.sf->glyphs[gid])));
	    if ( pos==end_pos ) --pos;
	    if ( pos<0 ) pos = 0;
	  break;
#endif
	  case GK_Left: case GK_KP_Left:
	    pos = end_pos-1;
	  break;
	  case GK_Right: case GK_KP_Right:
	    pos = end_pos+1;
	  break;
	  case GK_Up: case GK_KP_Up:
	    pos = end_pos-fv->colcnt;
	  break;
	  case GK_Down: case GK_KP_Down:
	    pos = end_pos+fv->colcnt;
	  break;
	  case GK_End: case GK_KP_End:
	    pos = fv->b.map->enccount;
	  break;
	  case GK_Home: case GK_KP_Home:
	    pos = 0;
	    if ( fv->b.sf->top_enc!=-1 && fv->b.sf->top_enc<fv->b.map->enccount )
		pos = fv->b.sf->top_enc;
	    else {
		pos = SFFindSlot(fv->b.sf,fv->b.map,home_char,NULL);
		if ( pos==-1 ) pos = 0;
	    }
	  break;
	  case GK_Page_Up: case GK_KP_Page_Up:
#if GK_Prior!=GK_Page_Up
	  case GK_Prior:
#endif
	    pos = (fv->rowoff-fv->rowcnt+1)*fv->colcnt;
	  break;
	  case GK_Page_Down: case GK_KP_Page_Down:
#if GK_Next!=GK_Page_Down
	  case GK_Next:
#endif
	    pos = (fv->rowoff+fv->rowcnt+1)*fv->colcnt;
	  break;
	}
	if ( pos<0 ) pos = 0;
	if ( pos>=fv->b.map->enccount ) pos = fv->b.map->enccount-1;
	if ( event->u.chr.state&ksm_shift && event->u.chr.keysym!=GK_Tab && event->u.chr.keysym!=GK_BackTab ) {
	    FVReselect(fv,pos);
	} else {
	    FVDeselectAll(fv);
	    fv->b.selected[pos] = true;
	    FVToggleCharSelected(fv,pos);
	    fv->pressed_pos = pos;
	    fv->sel_index = 1;
	}
	fv->end_pos = pos;
	FVShowInfo(fv);
	FVScrollToChar(fv,pos);
    } else if ( event->u.chr.keysym == GK_Help ) {
	MenuHelp(NULL,NULL,NULL);	/* Menu does F1 */
    } else if ( event->u.chr.keysym == GK_Escape ) {
	FVDeselectAll(fv);
    } else if ( event->u.chr.chars[0]=='\r' || event->u.chr.chars[0]=='\n' ) {
	if ( fv->b.container!=NULL && fv->b.container->funcs->is_modal )
return;
	for ( i=cnt=0; i<fv->b.map->enccount && cnt<10; ++i ) if ( fv->b.selected[i] ) {
	    SplineChar *sc = SFMakeChar(fv->b.sf,fv->b.map,i);
	    if ( fv->show==fv->filled ) {
		CharViewCreate(sc,fv,i);
	    } else {
		BDFFont *bdf = fv->show;
		BitmapViewCreate(BDFMakeGID(bdf,sc->orig_pos),bdf,fv,i);
	    }
	    ++cnt;
	}
    } else if ( (event->u.chr.state&((GMenuMask()|navigation_mask)&~(ksm_shift|ksm_capslock)))==navigation_mask &&
	    event->type == et_char &&
	    event->u.chr.keysym!=0 &&
	    (event->u.chr.keysym<GK_Special/* || event->u.chr.keysym>=0x10000*/)) {
	SplineFont *sf = fv->b.sf;
	int enc = EncFromUni(event->u.chr.keysym,fv->b.map->enc);
	if ( enc==-1 ) {
	    for ( i=0; i<sf->glyphcnt; ++i ) {
		if ( sf->glyphs[i]!=NULL )
		    if ( sf->glyphs[i]->unicodeenc==event->u.chr.keysym )
	    break;
	    }
	    if ( i!=-1 )
		enc = fv->b.map->backmap[i];
	}
	if ( enc<fv->b.map->enccount && enc!=-1 )
	    FVChangeChar(fv,enc);
    }
}



static void FVWindowMenuBuild(GWindow gw, struct gmenuitem *mi, GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
    int anychars = FVAnyCharSelected(fv);
    struct gmenuitem *wmi;
    int in_modal = (fv->b.container!=NULL && fv->b.container->funcs->is_modal);

    WindowMenuBuild(gw,mi,e);
    for ( wmi = mi->sub; wmi->ti.text!=NULL || wmi->ti.line ; ++wmi ) {
	switch ( wmi->mid ) {
	  case SMID_OpenOutline:
	    wmi->ti.disabled = anychars==-1 || in_modal;
	  break;
	  case SMID_OpenBitmap:
	    wmi->ti.disabled = anychars==-1 || fv->b.sf->bitmaps==NULL || in_modal;
	  break;
	  case SMID_OpenMetrics:
	    wmi->ti.disabled = in_modal;
	  break;
	  case SMID_Warnings:
	    wmi->ti.disabled = ErrorWindowExists();
	  break;
	}
    }
}


GMenuItem fvpopupmenu[] = {
    { { (unichar_t *) N_("Glyph _Info..."), (GImage *) "elementglyphinfo.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'I' }, '\0', ksm_control, NULL, NULL, FVMenuCharInfo, SMID_CharInfo },
    GMENUITEM_LINE,
    { { (unichar_t *) N_("Color|Choose..."), (GImage *)"colorwheel.png", COLOR_DEFAULT, COLOR_DEFAULT, (void *) -10, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, '\0' }, '\0', ksm_control, NULL, NULL, FVMenuSetColor, 0 },
    { { (unichar_t *)  N_("Color|Default"), &def_image, COLOR_DEFAULT, COLOR_DEFAULT, (void *) COLOR_DEFAULT, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, '\0' }, '\0', ksm_control, NULL, NULL, FVMenuSetColor, 0 },
    { { NULL, &white_image, COLOR_DEFAULT, COLOR_DEFAULT, (void *) 0xffffff, NULL, 0, 1, 0, 0, 0, 0, 0, 0, 0, '\0' }, '\0', ksm_control, NULL, NULL, FVMenuSetColor, 0 },
    { { NULL, &red_image, COLOR_DEFAULT, COLOR_DEFAULT, (void *) 0xff0000, NULL, 0, 1, 0, 0, 0, 0, 0, 0, 0, '\0' }, '\0', ksm_control, NULL, NULL, FVMenuSetColor, 0 },
    { { NULL, &green_image, COLOR_DEFAULT, COLOR_DEFAULT, (void *) 0x00ff00, NULL, 0, 1, 0, 0, 0, 0, 0, 0, 0, '\0' }, '\0', ksm_control, NULL, NULL, FVMenuSetColor, 0 },
    { { NULL, &blue_image, COLOR_DEFAULT, COLOR_DEFAULT, (void *) 0x0000ff, NULL, 0, 1, 0, 0, 0, 0, 0, 0, 0, '\0' }, '\0', ksm_control, NULL, NULL, FVMenuSetColor, 0 },
    { { NULL, &yellow_image, COLOR_DEFAULT, COLOR_DEFAULT, (void *) 0xffff00, NULL, 0, 1, 0, 0, 0, 0, 0, 0, 0, '\0' }, '\0', ksm_control, NULL, NULL, FVMenuSetColor, 0 },
    { { NULL, &cyan_image, COLOR_DEFAULT, COLOR_DEFAULT, (void *) 0x00ffff, NULL, 0, 1, 0, 0, 0, 0, 0, 0, 0, '\0' }, '\0', ksm_control, NULL, NULL, FVMenuSetColor, 0 },
    { { NULL, &magenta_image, COLOR_DEFAULT, COLOR_DEFAULT, (void *) 0xff00ff, NULL, 0, 1, 0, 0, 0, 0, 0, 0, 0, '\0' }, '\0', ksm_control, NULL, NULL, FVMenuSetColor, 0 },
    GMENUITEM_EMPTY
};

//#define _NO_PYTHON

#define mblist_extensions_idx  7
#define MENUBODY_DEFAULT          NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0
#define MENUBODY_DEFAULT_DISABLED NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 1, 1, 0, 0, 0, 0, 1, 1, 0
#define UN_(x) (unichar_t *) N_(x)

static GMenuItem2 mblist[] = {
    { { UN_("_File"),    MENUBODY_DEFAULT, 'F' },  H_("File|No Shortcut"),    sharedmenu_file,   sharedmenu_file_check, NULL, 0 },
    { { UN_("_Edit"),    MENUBODY_DEFAULT, 'E' },  H_("Edit|No Shortcut"),    sharedmenu_edit,   sharedmenu_edit_check, NULL, 0 },
    { { UN_("_Font"),    MENUBODY_DEFAULT, 'l' },  H_("Font|No Shortcut"),    sharedmenu_font,   sharedmenu_font_check, NULL, 0 },
    { { UN_("_Glyph"),   MENUBODY_DEFAULT, 'l' },  H_("Glyph|No Shortcut"),   sharedmenu_glyph,  sharedmenu_glyph_check,NULL, 0 },
    { { UN_("_Path"),    MENUBODY_DEFAULT, '\0' }, H_("Path|No Shortcut"),    sharedmenu_path,   sharedmenu_path_check, NULL, 0 },
    { { UN_("_Point"),   MENUBODY_DEFAULT, '\0' }, H_("Point|No Shortcut"),   sharedmenu_point,  sharedmenu_point_check,NULL, 0 },
    { { UN_("_Metrics"), MENUBODY_DEFAULT, 'M' },  H_("Metrics|No Shortcut"), sharedmenu_metrics,sharedmenu_metrics_check,NULL, 0 },
    { { UN_("_Extensions"),  MENUBODY_DEFAULT_DISABLED, 'l' }, H_("Extensions|No Shortcut"), NULL, fvpy_tllistcheck, NULL, 0 },
    { { UN_("_View"),    MENUBODY_DEFAULT, 'V' }, H_("View|No Shortcut"),   sharedmenu_view,   sharedmenu_view_check, NULL, 0 },
    { { UN_("_Window"),  MENUBODY_DEFAULT, 'W' }, H_("Window|No Shortcut"), sharedmenu_window, FVWindowMenuBuild, NULL, 0 },
    SHAREDMENU_COLLAB_LINE
    { { UN_("_Help"),    MENUBODY_DEFAULT, 'H' }, H_("Help|No Shortcut"), sharedmenu_help, sharedmenu_help_check, NULL, 0 },
    GMENUITEM2_EMPTY
};

void FVRefreshChar(FontView *fv,int gid) {
    BDFChar *bdfc;
    int i, j, enc;
    MetricsView *mv;

    /* Can happen in scripts */ /* Can happen if we do an AutoHint when generating a tiny font for freetype context */
    if ( fv->v==NULL || fv->colcnt==0 || fv->b.sf->glyphs[gid]== NULL )
return;
    for ( fv=(FontView *) (fv->b.sf->fv); fv!=NULL; fv = (FontView *) (fv->b.nextsame) ) {
	if( !fv->colcnt )
	    continue;

	for ( mv=fv->b.sf->metrics; mv!=NULL; mv=mv->next )
	    MVRefreshChar(mv,fv->b.sf->glyphs[gid]);
	if ( fv->show==fv->filled )
	    bdfc = BDFPieceMealCheck(fv->show,gid);
	else
	    bdfc = fv->show->glyphs[gid];
	if ( bdfc==NULL )
	    bdfc = BDFPieceMeal(fv->show,gid);
	/* A glyph may be encoded in several places, all need updating */
	for ( enc = 0; enc<fv->b.map->enccount; ++enc ) if ( fv->b.map->map[enc]==gid ) {
	    i = enc / fv->colcnt;
	    j = enc - i*fv->colcnt;
	    i -= fv->rowoff;
	    if ( i>=0 && i<fv->rowcnt )
		FVDrawGlyph(fv->v,fv,enc,true);
	}
    }
}

void FVRegenChar(FontView *fv,SplineChar *sc) {
    struct splinecharlist *dlist;
    MetricsView *mv;

    if ( fv->v==NULL )			/* Can happen in scripts */
return;

    if ( sc->orig_pos<fv->filled->glyphcnt ) {
	BDFCharFree(fv->filled->glyphs[sc->orig_pos]);
	fv->filled->glyphs[sc->orig_pos] = NULL;
    }
    /* FVRefreshChar does NOT do this for us */
    for ( mv=fv->b.sf->metrics; mv!=NULL; mv=mv->next )
	MVRegenChar(mv,sc);

    FVRefreshChar(fv,sc->orig_pos);
#if HANYANG
    if ( sc->compositionunit && fv->b.sf->rules!=NULL )
	Disp_RefreshChar(fv->b.sf,sc);
#endif

    for ( dlist=sc->dependents; dlist!=NULL; dlist=dlist->next )
	FVRegenChar(fv,dlist->sc);
}

static void AddSubPST(SplineChar *sc,struct lookup_subtable *sub,char *variant) {
    PST *pst;

    pst = chunkalloc(sizeof(PST));
    pst->type = pst_substitution;
    pst->subtable = sub;
    pst->u.alt.components = copy(variant);
    pst->next = sc->possub;
    sc->possub = pst;
}

SplineChar *FVMakeChar(FontView *fv,int enc) {
    SplineFont *sf = fv->b.sf;
    SplineChar *base_sc = SFMakeChar(sf,fv->b.map,enc), *feat_sc = NULL;
    int feat_gid = FeatureTrans(fv,enc);

    if ( fv->cur_subtable==NULL )
return( base_sc );

    if ( feat_gid==-1 ) {
	int uni = -1;
	FeatureScriptLangList *fl = fv->cur_subtable->lookup->features;

	if ( base_sc->unicodeenc>=0x600 && base_sc->unicodeenc<=0x6ff &&
		fl!=NULL &&
		(fl->featuretag == CHR('i','n','i','t') ||
		 fl->featuretag == CHR('m','e','d','i') ||
		 fl->featuretag == CHR('f','i','n','a') ||
		 fl->featuretag == CHR('i','s','o','l')) ) {
	    uni = fl->featuretag == CHR('i','n','i','t') ? ArabicForms[base_sc->unicodeenc-0x600].initial  :
		  fl->featuretag == CHR('m','e','d','i') ? ArabicForms[base_sc->unicodeenc-0x600].medial   :
		  fl->featuretag == CHR('f','i','n','a') ? ArabicForms[base_sc->unicodeenc-0x600].final    :
		  fl->featuretag == CHR('i','s','o','l') ? ArabicForms[base_sc->unicodeenc-0x600].isolated :
		  -1;
	    feat_sc = SFGetChar(sf,uni,NULL);
	    if ( feat_sc!=NULL )
return( feat_sc );
	}
	feat_sc = SFSplineCharCreate(sf);
	feat_sc->unicodeenc = uni;
	if ( uni!=-1 ) {
	    feat_sc->name = malloc(8);
	    feat_sc->unicodeenc = uni;
	    sprintf( feat_sc->name,"uni%04X", uni );
	} else if ( fv->cur_subtable->suffix!=NULL ) {
	    feat_sc->name = malloc(strlen(base_sc->name)+strlen(fv->cur_subtable->suffix)+2);
	    sprintf( feat_sc->name, "%s.%s", base_sc->name, fv->cur_subtable->suffix );
	} else if ( fl==NULL ) {
	    feat_sc->name = strconcat(base_sc->name,".unknown");
	} else if ( fl->ismac ) {
	    /* mac feature/setting */
	    feat_sc->name = malloc(strlen(base_sc->name)+14);
	    sprintf( feat_sc->name,"%s.m%d_%d", base_sc->name,
		    (int) (fl->featuretag>>16),
		    (int) ((fl->featuretag)&0xffff) );
	} else {
	    /* OpenType feature tag */
	    feat_sc->name = malloc(strlen(base_sc->name)+6);
	    sprintf( feat_sc->name,"%s.%c%c%c%c", base_sc->name,
		    (int) (fl->featuretag>>24),
		    (int) ((fl->featuretag>>16)&0xff),
		    (int) ((fl->featuretag>>8)&0xff),
		    (int) ((fl->featuretag)&0xff) );
	}
	SFAddGlyphAndEncode(sf,feat_sc,fv->b.map,fv->b.map->enccount);
	AddSubPST(base_sc,fv->cur_subtable,feat_sc->name);
return( feat_sc );
    } else
return( base_sc );
}

/* we style some glyph names differently, see FVExpose() */
#define _uni_italic	0x2
#define _uni_vertical	(1<<2)
#define _uni_fontmax	(2<<2)

static GFont *FVCheckFont(FontView *fv,int type) {
    FontRequest rq;

    if ( fv->fontset[type]==NULL ) {
	memset(&rq,0,sizeof(rq));
	rq.utf8_family_name = fv_fontnames;
	rq.point_size = fv_fontsize;
	rq.weight = 400;
	rq.style = 0;
	if (type&_uni_italic)
	    rq.style |= fs_italic;
	if (type&_uni_vertical)
	    rq.style |= fs_vertical;
	fv->fontset[type] = GDrawInstanciateFont(fv->v,&rq);
    }
return( fv->fontset[type] );
}

extern unichar_t adobes_pua_alts[0x200][3];

static void do_Adobe_Pua(unichar_t *buf,int sob,int uni) {
    int i, j;

    for ( i=j=0; j<sob-1 && i<3; ++i ) {
	int ch = adobes_pua_alts[uni-0xf600][i];
	if ( ch==0 )
    break;
	if ( ch>=0xf600 && ch<=0xf7ff && adobes_pua_alts[ch-0xf600]!=0 ) {
	    do_Adobe_Pua(buf+j,sob-j,ch);
	    while ( buf[j]!=0 ) ++j;
	} else
	    buf[j++] = ch;
    }
    buf[j] = 0;
}

static void FVExpose(FontView *fv,GWindow pixmap, GEvent *event) {
    int i, j, y, width, gid;
    int changed;
    GRect old, old2, r;
    GClut clut;
    struct _GImage base;
    GImage gi;
    SplineChar dummy;
    int styles, laststyles=0;
    Color bg, def_fg;
    int fgxor;

    def_fg = GDrawGetDefaultForeground(NULL);
    memset(&gi,'\0',sizeof(gi));
    memset(&base,'\0',sizeof(base));
    if ( fv->show->clut!=NULL ) {
	gi.u.image = &base;
	base.image_type = it_index;
	base.clut = fv->show->clut;
	GDrawSetDither(NULL, false);
	base.trans = -1;
    } else {
	memset(&clut,'\0',sizeof(clut));
	gi.u.image = &base;
	base.image_type = it_mono;
	base.clut = &clut;
	clut.clut_len = 2;
	clut.clut[0] = view_bgcol;
    }

    GDrawSetFont(pixmap,fv->fontset[0]);
    GDrawSetLineWidth(pixmap,0);
    GDrawPushClip(pixmap,&event->u.expose.rect,&old);
    GDrawFillRect(pixmap,NULL,view_bgcol);
    for ( i=0; i<=fv->rowcnt; ++i ) {
	GDrawDrawLine(pixmap,0,i*fv->cbh,fv->width,i*fv->cbh,def_fg);
	GDrawDrawLine(pixmap,0,i*fv->cbh+fv->lab_height,fv->width,i*fv->cbh+fv->lab_height,0x808080);
    }
    for ( i=0; i<=fv->colcnt; ++i )
	GDrawDrawLine(pixmap,i*fv->cbw,0,i*fv->cbw,fv->height,def_fg);
    for ( i=event->u.expose.rect.y/fv->cbh; i<=fv->rowcnt &&
	    (event->u.expose.rect.y+event->u.expose.rect.height+fv->cbh-1)/fv->cbh; ++i ) for ( j=0; j<fv->colcnt; ++j ) {
	int index = (i+fv->rowoff)*fv->colcnt+j;
	SplineChar *sc;
	styles = 0;
	if ( index < fv->b.map->enccount && index!=-1 ) {
	    unichar_t buf[60]; char cbuf[8];
	    char utf8_buf[8];
	    int use_utf8 = false;
	    Color fg;
	    extern const int amspua[];
	    int uni;
	    struct cidmap *cidmap = NULL;
	    sc = (gid=fv->b.map->map[index])!=-1 ? fv->b.sf->glyphs[gid]: NULL;

	    if ( fv->b.cidmaster!=NULL )
		cidmap = FindCidMap(fv->b.cidmaster->cidregistry,fv->b.cidmaster->ordering,fv->b.cidmaster->supplement,fv->b.cidmaster);

	    if ( ( fv->b.map->enc==&custom && index<256 ) ||
		 ( fv->b.map->enc!=&custom && index<fv->b.map->enc->char_cnt ) ||
		 ( cidmap!=NULL && index<MaxCID(cidmap) ))
		fg = def_fg;
	    else
		fg = 0x505050;
	    if ( sc==NULL )
		sc = SCBuildDummy(&dummy,fv->b.sf,fv->b.map,index);
	    uni = sc->unicodeenc;
	    buf[0] = buf[1] = 0;
	    if ( fv->b.sf->uni_interp==ui_ams && uni>=0xe000 && uni<=0xf8ff &&
		    amspua[uni-0xe000]!=0 )
		uni = amspua[uni-0xe000];
	    switch ( fv->glyphlabel ) {
	      case gl_name:
		uc_strncpy(buf,sc->name,sizeof(buf)/sizeof(buf[0]));
	      break;
	      case gl_unicode:
		if ( sc->unicodeenc!=-1 ) {
		    sprintf(cbuf,"%04x",sc->unicodeenc);
		    uc_strcpy(buf,cbuf);
		} else
		    uc_strcpy(buf,"?");
	      break;
	      case gl_encoding:
		if ( fv->b.map->enc->only_1byte ||
			(fv->b.map->enc->has_1byte && index<256))
		    sprintf(cbuf,"%02x",index);
		else
		    sprintf(cbuf,"%04x",index);
		uc_strcpy(buf,cbuf);
	      break;
	      case gl_glyph:
		if ( uni==0xad )
		    buf[0] = '-';
		else if ( fv->b.sf->uni_interp==ui_adobe && uni>=0xf600 && uni<=0xf7ff &&
			adobes_pua_alts[uni-0xf600]!=0 ) {
		    use_utf8 = false;
		    do_Adobe_Pua(buf,sizeof(buf),uni);
		} else if ( uni>=0xe0020 && uni<=0xe007e ) {
		    buf[0] = uni-0xe0000;	/* A map of Ascii for language names */
#if HANYANG
		} else if ( sc->compositionunit ) {
		    if ( sc->jamo<19 )
			buf[0] = 0x1100+sc->jamo;
		    else if ( sc->jamo<19+21 )
			buf[0] = 0x1161 + sc->jamo-19;
		    else	/* Leave a hole for the blank char */
			buf[0] = 0x11a8 + sc->jamo-(19+21+1);
#endif
		} else if ( uni>0 && uni<unicode4_size ) {
		    char *pt = utf8_buf;
		    use_utf8 = true;
			*pt = '\0'; // We terminate the string in case the appendage (?) fails.
		    pt = utf8_idpb(pt,uni,0);
		    if (pt) *pt = '\0'; else fprintf(stderr, "Invalid Unicode alert.\n");
		} else {
		    char *pt = strchr(sc->name,'.');
		    buf[0] = '?';
		    fg = 0xff0000;
		    if ( pt!=NULL ) {
			int i, n = pt-sc->name;
			char *end;
			SplineFont *cm = fv->b.sf->cidmaster;
			if ( n==7 && sc->name[0]=='u' && sc->name[1]=='n' && sc->name[2]=='i' &&
				(i=strtol(sc->name+3,&end,16), end-sc->name==7))
			    buf[0] = i;
			else if ( n>=5 && n<=7 && sc->name[0]=='u' &&
				(i=strtol(sc->name+1,&end,16), end-sc->name==n))
			    buf[0] = i;
			else if ( cm!=NULL && (i=CIDFromName(sc->name,cm))!=-1 ) {
			    int uni;
			    uni = CID2Uni(FindCidMap(cm->cidregistry,cm->ordering,cm->supplement,cm),
				    i);
			    if ( uni!=-1 )
				buf[0] = uni;
			} else {
			    int uni;
			    *pt = '\0';
			    uni = UniFromName(sc->name,fv->b.sf->uni_interp,fv->b.map->enc);
			    if ( uni!=-1 )
				buf[0] = uni;
			    *pt = '.';
			}
			if ( strstr(pt,".vert")!=NULL )
			    styles = _uni_vertical;
			if ( buf[0]!='?' ) {
			    fg = def_fg;
			    if ( strstr(pt,".italic")!=NULL )
				styles = _uni_italic;
			}
		    } else if ( strncmp(sc->name,"hwuni",5)==0 ) {
			int uni=-1;
			sscanf(sc->name,"hwuni%x", (unsigned *) &uni );
			if ( uni!=-1 ) buf[0] = uni;
		    } else if ( strncmp(sc->name,"italicuni",9)==0 ) {
			int uni=-1;
			sscanf(sc->name,"italicuni%x", (unsigned *) &uni );
			if ( uni!=-1 ) { buf[0] = uni; styles=_uni_italic; }
			fg = def_fg;
		    } else if ( strncmp(sc->name,"vertcid_",8)==0 ||
			    strncmp(sc->name,"vertuni",7)==0 ) {
			styles = _uni_vertical;
		    }
		}
	      break;
	    }
	    r.x = j*fv->cbw+1; r.width = fv->cbw-1;
	    r.y = i*fv->cbh+1; r.height = fv->lab_height-1;
	    bg = view_bgcol;
	    fgxor = 0x000000;
	    changed = sc->changed;
	    if ( fv->b.sf->onlybitmaps && gid<fv->show->glyphcnt )
		changed = gid==-1 || fv->show->glyphs[gid]==NULL? false : fv->show->glyphs[gid]->changed;
	    if ( changed ||
		    sc->layers[ly_back].splines!=NULL || sc->layers[ly_back].images!=NULL ||
		    sc->color!=COLOR_DEFAULT ) {
		if ( sc->layers[ly_back].splines!=NULL || sc->layers[ly_back].images!=NULL ||
			sc->color!=COLOR_DEFAULT )
		    bg = sc->color!=COLOR_DEFAULT?sc->color:0x808080;
		if ( sc->changed ) {
		    fgxor = bg ^ fvchangedcol;
		    bg = fvchangedcol;
		}
		GDrawFillRect(pixmap,&r,bg);
	    }
	    if ( (!fv->b.sf->layers[fv->b.active_layer].order2 && sc->changedsincelasthinted ) ||
		     ( fv->b.sf->layers[fv->b.active_layer].order2 && sc->layers[fv->b.active_layer].splines!=NULL &&
			sc->ttf_instrs_len<=0 ) ||
		     ( fv->b.sf->layers[fv->b.active_layer].order2 && sc->instructions_out_of_date ) ) {
		Color hintcol = fvhintingneededcol;
		if ( fv->b.sf->layers[fv->b.active_layer].order2 && sc->instructions_out_of_date && sc->ttf_instrs_len>0 )
		    hintcol = 0xff0000;
		GDrawDrawLine(pixmap,r.x,r.y,r.x,r.y+r.height-1,hintcol);
		GDrawDrawLine(pixmap,r.x+1,r.y,r.x+1,r.y+r.height-1,hintcol);
		GDrawDrawLine(pixmap,r.x+2,r.y,r.x+2,r.y+r.height-1,hintcol);
		GDrawDrawLine(pixmap,r.x+r.width-1,r.y,r.x+r.width-1,r.y+r.height-1,hintcol);
		GDrawDrawLine(pixmap,r.x+r.width-2,r.y,r.x+r.width-2,r.y+r.height-1,hintcol);
		GDrawDrawLine(pixmap,r.x+r.width-3,r.y,r.x+r.width-3,r.y+r.height-1,hintcol);
	    }
	    if ( use_utf8 && sc->unicodeenc!=-1 &&
		/* Pango complains if we try to draw non characters */
		/* These two are guaranteed "NOT A UNICODE CHARACTER" in all planes */
		    ((sc->unicodeenc&0xffff)==0xfffe || (sc->unicodeenc&0xffff)==0xffff ||
		     (sc->unicodeenc>=0xfdd0 && sc->unicodeenc<=0xfdef) ||	/* noncharacters */
		     (sc->unicodeenc>=0xfe00 && sc->unicodeenc<=0xfe0f) ||	/* variation selectors */
		     (sc->unicodeenc>=0xe0110 && sc->unicodeenc<=0xe01ff) ||	/* variation selectors */
		/*  The surrogates in BMP aren't valid either */
		     (sc->unicodeenc>=0xd800 && sc->unicodeenc<=0xdfff))) {	/* surrogates */
		GDrawDrawLine(pixmap,r.x,r.y,r.x+r.width-1,r.y+r.height-1,0x000000);
		GDrawDrawLine(pixmap,r.x,r.y+r.height-1,r.x+r.width-1,r.y,0x000000);
	    } else if ( use_utf8 ) {
		GTextBounds size;
		if ( styles!=laststyles ) GDrawSetFont(pixmap,FVCheckFont(fv,styles));
		width = GDrawGetText8Bounds(pixmap,utf8_buf,-1,&size);
		if ( size.lbearing==0 && size.rbearing==0 ) {
		    utf8_buf[0] = 0xe0 | (0xfffd>>12);
		    utf8_buf[1] = 0x80 | ((0xfffd>>6)&0x3f);
		    utf8_buf[2] = 0x80 | (0xfffd&0x3f);
		    utf8_buf[3] = 0;
		    width = GDrawGetText8Bounds(pixmap,utf8_buf,-1,&size);
		}
		width = size.rbearing - size.lbearing+1;
		if ( width >= fv->cbw-1 ) {
		    GDrawPushClip(pixmap,&r,&old2);
		    width = fv->cbw-1;
		}
		if ( sc->unicodeenc<0x80 || sc->unicodeenc>=0xa0 ) {
		    y = i*fv->cbh+fv->lab_as+1;
		    /* move rotated glyph up a bit to center it */
		    if (styles&_uni_vertical)
			y -= fv->lab_as/2;
		    GDrawDrawText8(pixmap,j*fv->cbw+(fv->cbw-1-width)/2-size.lbearing,y,utf8_buf,-1,fg^fgxor);
		}
		if ( width >= fv->cbw-1 )
		    GDrawPopClip(pixmap,&old2);
		laststyles = styles;
	    } else {
		if ( styles!=laststyles ) GDrawSetFont(pixmap,FVCheckFont(fv,styles));
		width = GDrawGetTextWidth(pixmap,buf,-1);
		if ( width >= fv->cbw-1 ) {
		    GDrawPushClip(pixmap,&r,&old2);
		    width = fv->cbw-1;
		}
		if ( sc->unicodeenc<0x80 || sc->unicodeenc>=0xa0 ) {
		    y = i*fv->cbh+fv->lab_as+1;
		    /* move rotated glyph up a bit to center it */
		    if (styles&_uni_vertical)
			y -= fv->lab_as/2;
		    GDrawDrawText(pixmap,j*fv->cbw+(fv->cbw-1-width)/2,y,buf,-1,fg^fgxor);
		}
		if ( width >= fv->cbw-1 )
		    GDrawPopClip(pixmap,&old2);
		laststyles = styles;
	    }
	}
	FVDrawGlyph(pixmap,fv,index,false);
    }
    if ( fv->showhmetrics&fvm_baseline ) {
	for ( i=0; i<=fv->rowcnt; ++i )
	    GDrawDrawLine(pixmap,0,i*fv->cbh+fv->lab_height+fv->magnify*fv->show->ascent+1,fv->width,i*fv->cbh+fv->lab_height+fv->magnify*fv->show->ascent+1,METRICS_BASELINE);
    }
    GDrawPopClip(pixmap,&old);
    GDrawSetDither(NULL, true);
}

void FVDrawInfo(FontView *fv,GWindow pixmap, GEvent *event) {
    GRect old, r;
    Color bg = GDrawGetDefaultBackground(GDrawGetDisplayOfWindow(pixmap));
    Color fg = fvglyphinfocol;
    SplineChar *sc, dummy;
    SplineFont *sf = fv->b.sf;
    EncMap *map = fv->b.map;
    int gid, uni, localenc;
    GString *output = g_string_new( "" );
    gchar *uniname = NULL;

    if ( event->u.expose.rect.y+event->u.expose.rect.height<=fv->mbh ) {
        g_string_free( output, TRUE ); output = NULL;
	return;
    }

    GDrawSetFont(pixmap,fv->fontset[0]);
    GDrawPushClip(pixmap,&event->u.expose.rect,&old);

    r.x = 0; r.width = fv->width; r.y = fv->mbh; r.height = fv->infoh;
    GDrawFillRect(pixmap,&r,bg);
    if ( fv->end_pos>=map->enccount || fv->pressed_pos>=map->enccount ||
	    fv->end_pos<0 || fv->pressed_pos<0 )
	fv->end_pos = fv->pressed_pos = -1;	/* Can happen after reencoding */
    if ( fv->end_pos == -1 ) {
        g_string_free( output, TRUE ); output = NULL;
	GDrawPopClip(pixmap,&old);
	return;
    }

    localenc = fv->end_pos;
    if ( map->remap!=NULL ) {
	struct remap *remap = map->remap;
	while ( remap->infont!=-1 ) {
	    if ( localenc>=remap->infont && localenc<=remap->infont+(remap->lastenc-remap->firstenc) ) {
		localenc += remap->firstenc-remap->infont;
		break;
	    }
	    ++remap;
	}
    }
    g_string_printf( output, "%d (0x%x) ", localenc, localenc );

    sc = (gid=fv->b.map->map[fv->end_pos])!=-1 ? sf->glyphs[gid] : NULL;
    if ( fv->b.cidmaster==NULL || fv->b.normal==NULL || sc==NULL )
	SCBuildDummy(&dummy,sf,fv->b.map,fv->end_pos);
    else
	dummy = *sc;
    if ( sc==NULL ) sc = &dummy;
    uni = dummy.unicodeenc!=-1 ? dummy.unicodeenc : sc->unicodeenc;

    /* last resort at guessing unicode code point from partial name */
    if ( uni == -1 ) {
	char *pt = strchr( sc->name, '.' );
	if( pt != NULL ) {
	    gchar *buf = g_strndup( (const gchar *) sc->name, pt - sc->name );
	    uni = UniFromName( (char *) buf, fv->b.sf->uni_interp, map->enc );
	    g_free( buf );
	}
    }

    if ( uni != -1 )
	g_string_append_printf( output, "U+%04X", uni );
    else {
	output = g_string_append( output, "U+????" );
    }

    /* postscript name */
    g_string_append_printf( output, " \"%s\" ", sc->name );

    /* code point name or range name */
    if( uni != -1 ) {
	uniname = (gchar *) unicode_name( uni );
	if ( uniname == NULL ) {
	    uniname = g_strdup( UnicodeRange( uni ) );
	}
    }

    if ( uniname != NULL ) {
	output = g_string_append( output, uniname );
	g_free( uniname );
    }

    GDrawDrawText8( pixmap, 10, fv->mbh+fv->lab_as, output->str, -1, fg );
    g_string_free( output, TRUE ); output = NULL;
    GDrawPopClip( pixmap, &old );
    return;
}

static void FVShowInfo(FontView *fv) {
    GRect r;

    if ( fv->v==NULL )			/* Can happen in scripts */
return;

    r.x = 0; r.width = fv->width; r.y = fv->mbh; r.height = fv->infoh;
    GDrawRequestExpose(fv->gw,&r,false);
}

static void utf82u_annot_strncat(unichar_t *to, const char *from, int len) {
    register unichar_t ch;

    to += u_strlen(to);
    while ( (ch = utf8_ildb(&from)) != '\0' && --len>=0 ) {
	if ( ch=='\t' ) {
	    *(to++) = ' ';
	    ch = ' ';
	}
	*(to++) = ch;
    }
    *to = 0;
}

void SCPreparePopup(GWindow gw,SplineChar *sc,struct remap *remap, int localenc,
	int actualuni) {
/* This is for the popup which appears when you hover mouse over a character on main window */
    int upos=-1;
    char *msg, *msg_old;

    /* If a glyph is multiply mapped then the inbuild unicode enc may not be */
    /*  the actual one used to access the glyph */
    if ( remap!=NULL ) {
	while ( remap->infont!=-1 ) {
	    if ( localenc>=remap->infont && localenc<=remap->infont+(remap->lastenc-remap->firstenc) ) {
		localenc += remap->firstenc-remap->infont;
                break;
	    }
	    ++remap;
	}
    }

    if ( actualuni!=-1 )
	upos = actualuni;
    else if ( sc->unicodeenc!=-1 )
	upos = sc->unicodeenc;
#if HANYANG
    else if ( sc->compositionunit ) {
	if ( sc->jamo<19 )
	    upos = 0x1100+sc->jamo;
	else if ( sc->jamo<19+21 )
	    upos = 0x1161 + sc->jamo-19;
	else		/* Leave a hole for the blank char */
	    upos = 0x11a8 + sc->jamo-(19+21+1);
    }
#endif

    if ( upos == -1 ) {
	msg = xasprintf( "%u 0x%x U+???? \"%.25s\" ",
		localenc, localenc,
		(sc->name == NULL) ? "" : sc->name );
    } else {
	/* unicode name or range name */
	char *uniname = unicode_name( upos );
	if( uniname == NULL ) uniname = strdup( UnicodeRange( upos ) );
	msg = xasprintf ( "%u 0x%x U+%04X \"%.25s\" %.100s",
		localenc, localenc, upos,
		(sc->name == NULL) ? "" : sc->name, uniname );
	if ( uniname != NULL ) free( uniname ); uniname = NULL;

	/* annotation */
        char *uniannot = unicode_annot( upos );
        if( uniannot != NULL ) {
            msg_old = msg;
            msg = xasprintf("%s\n%s", msg_old, uniannot);
            free(msg_old);
            free( uniannot );
        }
    }

    /* user comments */
    if ( sc->comment!=NULL ) {
        msg_old = msg;
        msg = xasprintf("%s\n%s", msg_old, sc->comment);
        free(msg_old);
    }

    GGadgetPreparePopup8( gw, msg );
    free(msg);
}

static void noop(void *UNUSED(_fv)) {
}

static void *ddgencharlist(void *_fv,int32 *len) {
    int i,j,cnt, gid;
    FontView *fv = (FontView *) _fv;
    SplineFont *sf = fv->b.sf;
    EncMap *map = fv->b.map;
    char *data;

    for ( i=cnt=0; i<map->enccount; ++i ) if ( fv->b.selected[i] && (gid=map->map[i])!=-1 && sf->glyphs[gid]!=NULL )
	cnt += strlen(sf->glyphs[gid]->name)+1;
    data = malloc(cnt+1); data[0] = '\0';
    for ( cnt=0, j=1 ; j<=fv->sel_index; ++j ) {
	for ( i=cnt=0; i<map->enccount; ++i )
	    if ( fv->b.selected[i] && (gid=map->map[i])!=-1 && sf->glyphs[gid]!=NULL ) {
		strcpy(data+cnt,sf->glyphs[gid]->name);
		cnt += strlen(sf->glyphs[gid]->name);
		strcpy(data+cnt++," ");
	    }
    }
    if ( cnt>0 )
	data[--cnt] = '\0';
    *len = cnt;
return( data );
}

static void FVMouse(FontView *fv, GEvent *event) {
    int pos = (event->u.mouse.y/fv->cbh + fv->rowoff)*fv->colcnt + event->u.mouse.x/fv->cbw;
    int gid;
    int realpos = pos;
    SplineChar *sc, dummy;
    int dopopup = true;

    if ( event->type==et_mousedown )
	CVPaletteDeactivate();
    if ( pos<0 ) {
	pos = 0;
	dopopup = false;
    } else if ( pos>=fv->b.map->enccount ) {
	pos = fv->b.map->enccount-1;
	if ( pos<0 )		/* No glyph slots in font */
return;
	dopopup = false;
    }

    sc = (gid=fv->b.map->map[pos])!=-1 ? fv->b.sf->glyphs[gid] : NULL;
    if ( sc==NULL )
	sc = SCBuildDummy(&dummy,fv->b.sf,fv->b.map,pos);
    if ( event->type == et_mouseup && event->u.mouse.clicks==2 ) {
	if ( fv->pressed ) {
	    GDrawCancelTimer(fv->pressed);
	    fv->pressed = NULL;
	}
	if ( fv->b.container!=NULL && fv->b.container->funcs->is_modal )
return;
	if ( fv->cur_subtable!=NULL ) {
	    sc = FVMakeChar(fv,pos);
	    pos = fv->b.map->backmap[sc->orig_pos];
	}
	if ( sc==&dummy ) {
	    sc = SFMakeChar(fv->b.sf,fv->b.map,pos);
	    gid = fv->b.map->map[pos];
	}
	if ( fv->show==fv->filled ) {
	    SplineFont *sf = fv->b.sf;
	    gid = -1;
	    if ( !OpenCharsInNewWindow )
		for ( gid=sf->glyphcnt-1; gid>=0; --gid )
		    if ( sf->glyphs[gid]!=NULL && sf->glyphs[gid]->views!=NULL )
		break;
	    if ( gid!=-1 ) {
		CharView *cv = (CharView *) (sf->glyphs[gid]->views);
		printf("calling CVChangeSC() sc:%p %s\n", sc, sc->name );
		CVChangeSC(cv,sc);
		GDrawSetVisible(cv->gw,true);
		GDrawRaise(cv->gw);
	    } else
		CharViewCreate(sc,fv,pos);
	} else {
	    BDFFont *bdf = fv->show;
	    BDFChar *bc =BDFMakeGID(bdf,gid);
	    gid = -1;
	    if ( !OpenCharsInNewWindow )
		for ( gid=bdf->glyphcnt-1; gid>=0; --gid )
		    if ( bdf->glyphs[gid]!=NULL && bdf->glyphs[gid]->views!=NULL )
		break;
	    if ( gid!=-1 ) {
		BitmapView *bv = bdf->glyphs[gid]->views;
		BVChangeBC(bv,bc,true);
		GDrawSetVisible(bv->gw,true);
		GDrawRaise(bv->gw);
	    } else
		BitmapViewCreate(bc,bdf,fv,pos);
	}
    } else if ( event->type == et_mousemove ) {
	if ( dopopup )
	    SCPreparePopup(fv->v,sc,fv->b.map->remap,pos,sc==&dummy?dummy.unicodeenc: UniFromEnc(pos,fv->b.map->enc));
    }
    if ( event->type == et_mousedown ) {
	if ( fv->drag_and_drop ) {
	    GDrawSetCursor(fv->v,ct_mypointer);
	    fv->any_dd_events_sent = fv->drag_and_drop = false;
	}
	if ( !(event->u.mouse.state&ksm_shift) && event->u.mouse.clicks<=1 ) {
	    if ( !fv->b.selected[pos] )
		FVDeselectAll(fv);
	    else if ( event->u.mouse.button!=3 ) {
		fv->drag_and_drop = fv->has_dd_no_cursor = true;
		fv->any_dd_events_sent = false;
		GDrawSetCursor(fv->v,ct_prohibition);
		GDrawGrabSelection(fv->v,sn_drag_and_drop);
		GDrawAddSelectionType(fv->v,sn_drag_and_drop,"STRING",fv,0,sizeof(char),
			ddgencharlist,noop);
	    }
	}
	fv->pressed_pos = fv->end_pos = pos;
	FVShowInfo(fv);
	if ( !fv->drag_and_drop ) {
	    if ( !(event->u.mouse.state&ksm_shift))
		fv->sel_index = 1;
	    else if ( fv->sel_index<255 )
		++fv->sel_index;
	    if ( fv->pressed!=NULL ) {
		GDrawCancelTimer(fv->pressed);
		fv->pressed = NULL;
	    } else if ( event->u.mouse.state&ksm_shift ) {
		fv->b.selected[pos] = fv->b.selected[pos] ? 0 : fv->sel_index;
		FVToggleCharSelected(fv,pos);
	    } else if ( !fv->b.selected[pos] ) {
		fv->b.selected[pos] = fv->sel_index;
		FVToggleCharSelected(fv,pos);
	    }
	    if ( event->u.mouse.button==3 )
		GMenuCreatePopupMenuWithName(fv->v,event, "Popup", fvpopupmenu);
	    else
		fv->pressed = GDrawRequestTimer(fv->v,200,100,NULL);
	}
    } else if ( fv->drag_and_drop ) {
	GWindow othergw = GDrawGetPointerWindow(fv->v);

	if ( othergw==fv->v || othergw==fv->gw || othergw==NULL ) {
	    if ( !fv->has_dd_no_cursor ) {
		fv->has_dd_no_cursor = true;
		GDrawSetCursor(fv->v,ct_prohibition);
	    }
	} else {
	    if ( fv->has_dd_no_cursor ) {
		fv->has_dd_no_cursor = false;
		GDrawSetCursor(fv->v,ct_ddcursor);
	    }
	}
	if ( event->type==et_mouseup ) {
	    if ( pos!=fv->pressed_pos ) {
		GDrawPostDragEvent(fv->v,event,event->type==et_mouseup?et_drop:et_drag);
		fv->any_dd_events_sent = true;
	    }
	    fv->drag_and_drop = fv->has_dd_no_cursor = false;
	    GDrawSetCursor(fv->v,ct_mypointer);
	    if ( !fv->any_dd_events_sent )
		FVDeselectAll(fv);
	    fv->any_dd_events_sent = false;
	}
    } else if ( fv->pressed!=NULL ) {
	int showit = realpos!=fv->end_pos;
	FVReselect(fv,realpos);
	if ( showit )
	    FVShowInfo(fv);
	if ( event->type==et_mouseup ) {
	    GDrawCancelTimer(fv->pressed);
	    fv->pressed = NULL;
	}
    }
    if ( event->type==et_mouseup && dopopup )
	SCPreparePopup(fv->v,sc,fv->b.map->remap,pos,sc==&dummy?dummy.unicodeenc: UniFromEnc(pos,fv->b.map->enc));
    if ( event->type==et_mouseup )
	SVAttachFV(fv,2);
}

static void FVResize(FontView *fv, GEvent *event) {
    extern int default_fv_row_count, default_fv_col_count;
    GRect pos,screensize;
    int topchar;

    if ( fv->colcnt!=0 )
	topchar = fv->rowoff*fv->colcnt;
    else if ( fv->b.sf->top_enc!=-1 && fv->b.sf->top_enc<fv->b.map->enccount )
	topchar = fv->b.sf->top_enc;
    else {
	/* Position on 'A' (or whatever they ask for) if it exists */
	topchar = SFFindSlot(fv->b.sf,fv->b.map,home_char,NULL);
	if ( topchar==-1 ) {
	    for ( topchar=0; topchar<fv->b.map->enccount; ++topchar )
		if ( fv->b.map->map[topchar]!=-1 && fv->b.sf->glyphs[fv->b.map->map[topchar]]!=NULL )
	    break;
	    if ( topchar==fv->b.map->enccount )
		topchar = 0;
	}
    }
    if ( !event->u.resize.sized )
	/* WM isn't responding to my resize requests, so no point in trying */;
    else if ( (event->u.resize.size.width-
		GDrawPointsToPixels(fv->gw,_GScrollBar_Width)-1)%fv->cbw!=0 ||
	    (event->u.resize.size.height-fv->mbh-fv->infoh-1)%fv->cbh!=0 ) {
	int cc = (event->u.resize.size.width+fv->cbw/2-
		GDrawPointsToPixels(fv->gw,_GScrollBar_Width)-1)/fv->cbw;
	int rc = (event->u.resize.size.height-fv->mbh-fv->infoh-1)/fv->cbh;
	if ( cc<=0 ) cc = 1;
	if ( rc<=0 ) rc = 1;
	GDrawGetSize(GDrawGetRoot(NULL),&screensize);
	if ( cc*fv->cbw+GDrawPointsToPixels(fv->gw,_GScrollBar_Width)>screensize.width )
	    --cc;
	if ( rc*fv->cbh+fv->mbh+fv->infoh+10>screensize.height )
	    --rc;
	GDrawResize(fv->gw,
		cc*fv->cbw+1+GDrawPointsToPixels(fv->gw,_GScrollBar_Width),
		rc*fv->cbh+1+fv->mbh+fv->infoh);
	/* somehow KDE loses this event of mine so to get even the vague effect */
	/*  we can't just return */
/*return;*/
    }

    pos.width = GDrawPointsToPixels(fv->gw,_GScrollBar_Width);
    pos.height = event->u.resize.size.height-fv->mbh-fv->infoh;
    pos.x = event->u.resize.size.width-pos.width; pos.y = fv->mbh+fv->infoh;
    GGadgetResize(fv->vsb,pos.width,pos.height);
    GGadgetMove(fv->vsb,pos.x,pos.y);
    pos.width = pos.x; pos.x = 0;
    GDrawResize(fv->v,pos.width,pos.height);

    fv->width = pos.width; fv->height = pos.height;
    fv->colcnt = (fv->width-1)/fv->cbw;
    if ( fv->colcnt<1 ) fv->colcnt = 1;
    fv->rowcnt = (fv->height-1)/fv->cbh;
    if ( fv->rowcnt<1 ) fv->rowcnt = 1;
    fv->rowltot = (fv->b.map->enccount+fv->colcnt-1)/fv->colcnt;

    GScrollBarSetBounds(fv->vsb,0,fv->rowltot,fv->rowcnt);
    fv->rowoff = topchar/fv->colcnt;
    if ( fv->rowoff>=fv->rowltot-fv->rowcnt )
        fv->rowoff = fv->rowltot-fv->rowcnt;
    if ( fv->rowoff<0 ) fv->rowoff =0;
    GScrollBarSetPos(fv->vsb,fv->rowoff);
    GDrawRequestExpose(fv->gw,NULL,true);
    GDrawRequestExpose(fv->v,NULL,true);

    if ( fv->rowcnt!=fv->b.sf->desired_row_cnt || fv->colcnt!=fv->b.sf->desired_col_cnt ) {
	default_fv_row_count = fv->rowcnt;
	default_fv_col_count = fv->colcnt;
	fv->b.sf->desired_row_cnt = fv->rowcnt;
	fv->b.sf->desired_col_cnt = fv->colcnt;
	SavePrefs(true);
    }
}

static void FVTimer(FontView *fv, GEvent *event) {

    if ( event->u.timer.timer==fv->pressed ) {
	GEvent e;
	GDrawGetPointerPosition(fv->v,&e);
	if ( e.u.mouse.y<0 || e.u.mouse.y >= fv->height ) {
	    real dy = 0;
	    if ( e.u.mouse.y<0 )
		dy = -1;
	    else if ( e.u.mouse.y>=fv->height )
		dy = 1;
	    if ( fv->rowoff+dy<0 )
		dy = 0;
	    else if ( fv->rowoff+dy+fv->rowcnt > fv->rowltot )
		dy = 0;
	    fv->rowoff += dy;
	    if ( dy!=0 ) {
		GScrollBarSetPos(fv->vsb,fv->rowoff);
		GDrawScroll(fv->v,NULL,0,dy*fv->cbh);
	    }
	}
    } else if ( event->u.timer.timer==fv->resize ) {
	/* It's a delayed resize event (for kde which sends continuous resizes) */
	fv->resize = NULL;
	FVResize(fv,(GEvent *) (event->u.timer.userdata));
    } else if ( event->u.timer.userdata!=NULL ) {
	/* It's a delayed function call */
	void (*func)(FontView *) = (void (*)(FontView *)) (event->u.timer.userdata);
	func(fv);
    }
}

void FVDelay(FontView *fv,void (*func)(FontView *)) {
    GDrawRequestTimer(fv->v,100,0,(void *) func);
}

static int FVScroll(GGadget *g, GEvent *e) {
    FontView *fv = GGadgetGetUserData(g);
    int newpos = fv->rowoff;
    struct sbevent *sb = &e->u.control.u.sb;

    switch( sb->type ) {
      case et_sb_top:
        newpos = 0;
      break;
      case et_sb_uppage:
        newpos -= fv->rowcnt;
      break;
      case et_sb_up:
        --newpos;
      break;
      case et_sb_down:
        ++newpos;
      break;
      case et_sb_downpage:
        newpos += fv->rowcnt;
      break;
      case et_sb_bottom:
        newpos = fv->rowltot-fv->rowcnt;
      break;
      case et_sb_thumb:
      case et_sb_thumbrelease:
        newpos = sb->pos;
      break;
    }
    if ( newpos>fv->rowltot-fv->rowcnt )
        newpos = fv->rowltot-fv->rowcnt;
    if ( newpos<0 ) newpos =0;
    if ( newpos!=fv->rowoff ) {
	int diff = newpos-fv->rowoff;
	fv->rowoff = newpos;
	GScrollBarSetPos(fv->vsb,fv->rowoff);
	GDrawScroll(fv->v,NULL,0,diff*fv->cbh);
    }
return( true );
}

static int v_e_h(GWindow gw, GEvent *event) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);

    if (( event->type==et_mouseup || event->type==et_mousedown ) &&
	    (event->u.mouse.button>=4 && event->u.mouse.button<=7) ) {
return( GGadgetDispatchEvent(fv->vsb,event));
    }

    GGadgetPopupExternalEvent(event);
    switch ( event->type ) {
      case et_expose:
	GDrawSetLineWidth(gw,0);
	FVExpose(fv,gw,event);
      break;
      case et_char:
	if ( fv->b.container!=NULL )
	    (fv->b.container->funcs->charEvent)(fv->b.container,event);
	else
	    FVChar(fv,event);
      break;
      case et_mousemove: case et_mousedown: case et_mouseup:
	if ( event->type==et_mousedown )
	    GDrawSetGIC(gw,fv->gic,0,20);
	if ( fv->notactive && event->type==et_mousedown )
	    (fv->b.container->funcs->activateMe)(fv->b.container,&fv->b);
	FVMouse(fv,event);
      break;
      case et_timer:
	FVTimer(fv,event);
      break;
      case et_focus:
	  printf("fv.et_focus\n");
	if ( event->u.focus.gained_focus )
	    GDrawSetGIC(gw,fv->gic,0,20);
      break;
    }
return( true );
}

static void FontView_ReformatOne(FontView *fv) {
    FontView *fvs;

    if ( fv->v==NULL || fv->colcnt==0 )	/* Can happen in scripts */
return;

    GDrawSetCursor(fv->v,ct_watch);
    fv->rowltot = (fv->b.map->enccount+fv->colcnt-1)/fv->colcnt;
    GScrollBarSetBounds(fv->vsb,0,fv->rowltot,fv->rowcnt);
    if ( fv->rowoff>fv->rowltot-fv->rowcnt ) {
        fv->rowoff = fv->rowltot-fv->rowcnt;
	if ( fv->rowoff<0 ) fv->rowoff =0;
	GScrollBarSetPos(fv->vsb,fv->rowoff);
    }
    for ( fvs=(FontView *) (fv->b.sf->fv); fvs!=NULL; fvs=(FontView *) (fvs->b.nextsame) )
	if ( fvs!=fv && fvs->b.sf==fv->b.sf )
    break;
    GDrawRequestExpose(fv->v,NULL,false);
    GDrawSetCursor(fv->v,ct_pointer);
}

static void FontView_ReformatAll(SplineFont *sf) {
    BDFFont *new, *old, *bdf;
    FontView *fv;
    MetricsView *mvs;
    extern int use_freetype_to_rasterize_fv;

    if ( ((FontView *) (sf->fv))->v==NULL || ((FontView *) (sf->fv))->colcnt==0 )			/* Can happen in scripts */
return;

    for ( fv=(FontView *) (sf->fv); fv!=NULL; fv=(FontView *) (fv->b.nextsame) ) {
	GDrawSetCursor(fv->v,ct_watch);
	old = fv->filled;
				/* In CID fonts fv->b.sf may not be same as sf */
	new = SplineFontPieceMeal(fv->b.sf,fv->b.active_layer,fv->filled->pixelsize,72,
		(fv->antialias?pf_antialias:0)|(fv->bbsized?pf_bbsized:0)|
		    (use_freetype_to_rasterize_fv && !sf->strokedfont && !sf->multilayer?pf_ft_nohints:0),
		NULL);
	fv->filled = new;
	if ( fv->show==old )
	    fv->show = new;
	else {
	    for ( bdf=sf->bitmaps; bdf != NULL &&
		( bdf->pixelsize != fv->show->pixelsize || BDFDepth( bdf ) != BDFDepth( fv->show )); bdf=bdf->next );
	    if ( bdf != NULL ) fv->show = bdf;
	    else fv->show = new;
	}
	BDFFontFree(old);
	fv->rowltot = (fv->b.map->enccount+fv->colcnt-1)/fv->colcnt;
	GScrollBarSetBounds(fv->vsb,0,fv->rowltot,fv->rowcnt);
	if ( fv->rowoff>fv->rowltot-fv->rowcnt ) {
	    fv->rowoff = fv->rowltot-fv->rowcnt;
	    if ( fv->rowoff<0 ) fv->rowoff =0;
	    GScrollBarSetPos(fv->vsb,fv->rowoff);
	}
	GDrawRequestExpose(fv->v,NULL,false);
	GDrawSetCursor(fv->v,ct_pointer);
    }
    for ( mvs=sf->metrics; mvs!=NULL; mvs=mvs->next ) if ( mvs->bdf==NULL ) {
	BDFFontFree(mvs->show);
	mvs->show = SplineFontPieceMeal(sf,mvs->layer,mvs->ptsize,mvs->dpi,
		mvs->antialias?(pf_antialias|pf_ft_recontext):pf_ft_recontext,NULL);
	GDrawRequestExpose(mvs->gw,NULL,false);
    }
}

void FontViewRemove(FontView *fv) {
    if ( fv_list==fv )
	fv_list = (FontView *) (fv->b.next);
    else {
	FontView *n;
	for ( n=fv_list; n->b.next!=&fv->b; n=(FontView *) (n->b.next) );
	n->b.next = fv->b.next;
    }
    FontViewFree(&fv->b);
}

/**
 * In some cases fontview gets an et_selclear event when using copy
 * and paste on the OSX. So this guard lets us quietly ignore that
 * event when we have just done command+c or command+x.
 */
extern int osx_fontview_copy_cut_counter;

static FontView* ActiveFontView = 0;

static int fv_e_h(GWindow gw, GEvent *event) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);

    if (( event->type==et_mouseup || event->type==et_mousedown ) &&
	    (event->u.mouse.button>=4 && event->u.mouse.button<=7) ) {
return( GGadgetDispatchEvent(fv->vsb,event));
    }

    switch ( event->type ) {
      case et_focus:
	  if ( event->u.focus.gained_focus )
	  {
	      ActiveFontView = fv;
	  }
	  else
	  {
	  }
	  break;
      case et_selclear:
#ifdef __Mac
	  // For some reason command + c and command + x wants
	  // to send a clear to us, even if that key was pressed
	  // on a charview.
	  if( osx_fontview_copy_cut_counter )
	  {
	     osx_fontview_copy_cut_counter--;
	     break;
          }
//	  printf("fontview et_selclear\n");
#endif
	ClipboardClear();
      break;
      case et_expose:
	GDrawSetLineWidth(gw,0);
	FVDrawInfo(fv,gw,event);
      break;
      case et_resize:
	/* KDE sends a continuous stream of resize events, and gets very */
	/*  confused if I start resizing the window myself, try to wait for */
	/*  the user to finish before responding to resizes */
	if ( event->u.resize.sized || fv->resize_expected ) {
	    if ( fv->resize )
		GDrawCancelTimer(fv->resize);
	    fv->resize_event = *event;
	    fv->resize = GDrawRequestTimer(fv->v,300,0,(void *) &fv->resize_event);
	    fv->resize_expected = false;
	}
      break;
      case et_char:
	if ( fv->b.container!=NULL )
	    (fv->b.container->funcs->charEvent)(fv->b.container,event);
	else
	    FVChar(fv,event);
      break;
      case et_mousedown:
	GDrawSetGIC(gw,fv->gwgic,0,20);
	if ( fv->notactive )
	    (fv->b.container->funcs->activateMe)(fv->b.container,&fv->b);
      break;
      case et_close:
	FVMenuClose(gw,NULL,NULL);
      break;
      case et_create:
	fv->b.next = (FontViewBase *) fv_list;
	fv_list = fv;
      break;
      case et_destroy:
	if ( fv->qg!=NULL )
	    QGRmFontView(fv->qg,fv);
	FontViewRemove(fv);
      break;
    }
return( true );
}

static void FontViewOpenKids(FontView *fv) {
    int k, i;
    SplineFont *sf = fv->b.sf, *_sf;
#if defined(__Mac)
    int cnt= 0;
#endif

    if ( sf->cidmaster!=NULL )
	sf = sf->cidmaster;

    k=0;
    do {
	_sf = sf->subfontcnt==0 ? sf : sf->subfonts[k];
	for ( i=0; i<_sf->glyphcnt; ++i )
	    if ( _sf->glyphs[i]!=NULL && _sf->glyphs[i]->wasopen ) {
		_sf->glyphs[i]->wasopen = false;
#if defined(__Mac)
		/* If we open a bunch of charviews all at once on the mac, X11*/
		/*  crashes */ /* But opening one seems ok */
		if ( ++cnt==1 )
#endif
		CharViewCreate(_sf->glyphs[i],fv,-1);
	    }
	++k;
    } while ( k<sf->subfontcnt );
}

static FontView *__FontViewCreate(SplineFont *sf) {
    FontView *fv = calloc(1,sizeof(FontView));
    int i;
    int ps = sf->display_size<0 ? -sf->display_size :
	     sf->display_size==0 ? default_fv_font_size : sf->display_size;

    if ( ps>200 ) ps = 128;

    /* Filename != NULL if we opened an sfd file. Sfd files know whether */
    /*  the font is compact or not and should not depend on a global flag */
    /* If a font is new, then compaction will make it vanish completely */
    if ( sf->fv==NULL && compact_font_on_open && sf->filename==NULL && !sf->new ) {
	sf->compacted = true;
	for ( i=0; i<sf->subfontcnt; ++i )
	    sf->subfonts[i]->compacted = true;
    }
    fv->b.nextsame = sf->fv;
    fv->b.active_layer = sf->display_layer;
    sf->fv = (FontViewBase *) fv;
    if ( sf->mm!=NULL ) {
	sf->mm->normal->fv = (FontViewBase *) fv;
	for ( i = 0; i<sf->mm->instance_count; ++i )
	    sf->mm->instances[i]->fv = (FontViewBase *) fv;
    }
    if ( sf->subfontcnt==0 ) {
	fv->b.sf = sf;
	if ( fv->b.nextsame!=NULL ) {
	    fv->b.map = EncMapCopy(fv->b.nextsame->map);
	    fv->b.normal = fv->b.nextsame->normal==NULL ? NULL : EncMapCopy(fv->b.nextsame->normal);
	} else if ( sf->compacted ) {
	    fv->b.normal = sf->map;
	    fv->b.map = CompactEncMap(EncMapCopy(sf->map),sf);
	    sf->map = fv->b.map;
	} else {
	    fv->b.map = sf->map;
	    fv->b.normal = NULL;
	}
    } else {
	fv->b.cidmaster = sf;
	for ( i=0; i<sf->subfontcnt; ++i )
	    sf->subfonts[i]->fv = (FontViewBase *) fv;
	for ( i=0; i<sf->subfontcnt; ++i )	/* Search for a subfont that contains more than ".notdef" (most significant in .gai fonts) */
	    if ( sf->subfonts[i]->glyphcnt>1 ) {
		fv->b.sf = sf->subfonts[i];
	break;
	    }
	if ( fv->b.sf==NULL )
	    fv->b.sf = sf->subfonts[0];
	sf = fv->b.sf;
	if ( fv->b.nextsame==NULL ) { EncMapFree(sf->map); sf->map = NULL; }
	fv->b.map = EncMap1to1(sf->glyphcnt);
	if ( fv->b.nextsame==NULL ) { sf->map = fv->b.map; }
	if ( sf->compacted ) {
	    fv->b.normal = fv->b.map;
	    fv->b.map = CompactEncMap(EncMapCopy(fv->b.map),sf);
	    if ( fv->b.nextsame==NULL ) { sf->map = fv->b.map; }
	}
    }
    fv->b.selected = calloc(fv->b.map->enccount,sizeof(char));
    fv->user_requested_magnify = -1;
    fv->magnify = (ps<=9)? 3 : (ps<20) ? 2 : 1;
    fv->cbw = (ps*fv->magnify)+1;
    fv->cbh = (ps*fv->magnify)+1+fv->lab_height+1;
    fv->antialias = sf->display_antialias;
    fv->bbsized = sf->display_bbsized;
    fv->glyphlabel = default_fv_glyphlabel;

    fv->end_pos = -1;
#ifndef _NO_PYTHON
    PyFF_InitFontHook((FontViewBase *)fv);
#endif

    fv->pid_webfontserver = 0;

return( fv );
}

static int fontview_ready = false;

static void FontViewFinish() {
    if (!fontview_ready) return;
    mb2FreeGetText(mblist);
    mbFreeGetText(fvpopupmenu);
}

void FontViewFinishNonStatic() {
    FontViewFinish();
}



static void FontViewInit(void) {
    // static int done = false; // superseded by fontview_ready.

    if ( fontview_ready )
return;

    fontview_ready = true;

    mb2DoGetText(mblist);
    mbDoGetText(fvpopupmenu);
    atexit(&FontViewFinishNonStatic);

}

static struct resed fontview_re[] = {
    {N_("Glyph Info Color"), "GlyphInfoColor", rt_color, &fvglyphinfocol, N_("Color of the font used to display glyph information in the fontview"), NULL, { 0 }, 0, 0 },
    {N_("Empty Slot FG Color"), "EmptySlotFgColor", rt_color, &fvemtpyslotfgcol, N_("Color used to draw the foreground of empty slots"), NULL, { 0 }, 0, 0 },
    {N_("Selected BG Color"), "SelectedColor", rt_color, &fvselcol, N_("Color used to draw the background of selected glyphs"), NULL, { 0 }, 0, 0 },
    {N_("Selected FG Color"), "SelectedFgColor", rt_color, &fvselfgcol, N_("Color used to draw the foreground of selected glyphs"), NULL, { 0 }, 0, 0 },
    {N_("Changed Color"), "ChangedColor", rt_color, &fvchangedcol, N_("Color used to mark a changed glyph"), NULL, { 0 }, 0, 0 },
    {N_("Hinting Needed Color"), "HintingNeededColor", rt_color, &fvhintingneededcol, N_("Color used to mark glyphs that need hinting"), NULL, { 0 }, 0, 0 },
    {N_("Font Size"), "FontSize", rt_int, &fv_fontsize, N_("Size (in points) of the font used to display information and glyph labels in the fontview"), NULL, { 0 }, 0, 0 },
    {N_("Font Family"), "FontFamily", rt_stringlong, &fv_fontnames, N_("A comma separated list of font family names used to display small example images of glyphs over the user designed glyphs"), NULL, { 0 }, 0, 0 },
    RESED_EMPTY
};

static void FVCreateInnards(FontView *fv,GRect *pos) {
    GWindow gw = fv->gw;
    GWindowAttrs wattrs;
    GGadgetData gd;
    FontRequest rq;
    BDFFont *bdf;
    int as,ds,ld;
    extern int use_freetype_to_rasterize_fv;
    SplineFont *sf = fv->b.sf;

    fv->b.m_commonView.m_classType.m_type = TYPE_FONTVIEW;
    fv->lab_height = FV_LAB_HEIGHT-13+GDrawPointsToPixels(NULL,fv_fontsize);

    memset(&gd,0,sizeof(gd));
    gd.pos.y = pos->y; gd.pos.height = pos->height;
    gd.pos.width = GDrawPointsToPixels(gw,_GScrollBar_Width);
    gd.pos.x = pos->width;
    gd.u.sbinit = NULL;
    gd.flags = gg_visible|gg_enabled|gg_pos_in_pixels|gg_sb_vert;
    gd.handle_controlevent = FVScroll;
    fv->vsb = GScrollBarCreate(gw,&gd,fv);


    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_backcol;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.cursor = ct_pointer;
    wattrs.background_color = view_bgcol;
    fv->v = GWidgetCreateSubWindow(gw,pos,v_e_h,fv,&wattrs);
    GDrawSetVisible(fv->v,true);
    GDrawSetWindowTypeName(fv->v, "FontView");

    fv->gic   = GDrawCreateInputContext(fv->v,gic_root|gic_orlesser);
    fv->gwgic = GDrawCreateInputContext(fv->gw,gic_root|gic_orlesser);
    GDrawSetGIC(fv->v,fv->gic,0,20);
    GDrawSetGIC(fv->gw,fv->gic,0,20);

    fv->fontset = calloc(_uni_fontmax,sizeof(GFont *));
    memset(&rq,0,sizeof(rq));
    rq.utf8_family_name = fv_fontnames;
    rq.point_size = fv_fontsize;
    rq.weight = 400;
    fv->fontset[0] = GDrawInstanciateFont(gw,&rq);
    GDrawSetFont(fv->v,fv->fontset[0]);
    GDrawWindowFontMetrics(fv->v,fv->fontset[0],&as,&ds,&ld);
    fv->lab_as = as;
    fv->showhmetrics = default_fv_showhmetrics;
    fv->showvmetrics = default_fv_showvmetrics && sf->hasvmetrics;
    bdf = SplineFontPieceMeal(fv->b.sf,fv->b.active_layer,sf->display_size<0?-sf->display_size:default_fv_font_size,72,
	    (fv->antialias?pf_antialias:0)|(fv->bbsized?pf_bbsized:0)|
		(use_freetype_to_rasterize_fv && !sf->strokedfont && !sf->multilayer?pf_ft_nohints:0),
	    NULL);
    fv->filled = bdf;
    if ( sf->display_size>0 ) {
	for ( bdf=sf->bitmaps; bdf!=NULL && bdf->pixelsize!=sf->display_size ;
		bdf=bdf->next );
	if ( bdf==NULL )
	    bdf = fv->filled;
    }
    if ( sf->onlybitmaps && bdf==fv->filled && sf->bitmaps!=NULL )
	bdf = sf->bitmaps;
    fv->cbw = -1;
    FVChangeDisplayFont(fv,bdf);
}


#define MENUTRAMP(FNAME) \
    static void sm_##FNAME(CommonView* self ) {			\
	FontView* fv = tryObtainCastFontView( self );		\
	FNAME(fv);						\
    } 
MENUTRAMP(FVSelectAll);
MENUTRAMP(FVInvertSelection);
MENUTRAMP(FVDeselectAll)
MENUTRAMP(FVSelectByPST)



    /* // sharedmenu_edit_copylist */
    /* void (*copyWidth)( CommonView* self ); */

    /* // sharedmenu_edit_pastelist */
    /* void (*pasteInto)( CommonView* self ); */
    /* void (*pasteAfter)( CommonView* self ); */

    /* // sharedmenu_edit_clearlist */
    /* void (*clearBackground)( CommonView* self ); */
    /* void (*join)( CommonView* self ); */



static int sm_getActiveLayer( CommonView* self )
{
    FontView* fv = tryObtainCastFontView( self );
    return( fv->b.active_layer );
}

static void sm_kernPairCloseUp( CommonView* self )
{
    FontView* fv = tryObtainCastFontView( self );
    int i;

    for ( i=0; i<fv->b.map->enccount; ++i )
	if ( fv->b.selected[i] )
	    break;
    SplineChar *sc =
	i==fv->b.map->enccount?NULL:
	fv->b.map->map[i]==-1?NULL:
	fv->b.sf->glyphs[fv->b.map->map[i]];
    
    KernPairD( fv->b.sf, sc, NULL, fv->b.active_layer, false);    
}

static void sm_removeKern( CommonView* self )
{
    FontView* fv = tryObtainCastFontView( self );
    FVRemoveKerns(&fv->b);
}

static void sm_removeVKern( CommonView* self )
{
    FontView* fv = tryObtainCastFontView( self );
    FVRemoveVKerns(&fv->b);
}

static FontView *FontView_Create(SplineFont *sf, int hide) {
    FontView *fv = (FontView *) __FontViewCreate(sf);
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetData gd;
    GRect gsize;
    static GWindow icon = NULL;
    static int nexty=0;
    GRect size;

    FontViewInit();
    if ( icon==NULL ) {
#ifdef BIGICONS
	icon = GDrawCreateBitmap(NULL,fontview_width,fontview_height,fontview_bits);
#else
	icon = GDrawCreateBitmap(NULL,fontview2_width,fontview2_height,fontview2_bits);
#endif
    }

#define SETVTABLE(VFNAME,FNAME) \
    fv->b.m_commonView.m_sharedmenu_funcs.VFNAME = sm_##FNAME;

    // file menu
//    SETVTABLE(dialogLoadWordList,     dialogLoadWordList);
    
    SETVTABLE(selectAll,              FVSelectAll);
    SETVTABLE(invertSelection,        FVInvertSelection);
    SETVTABLE(deselectAll,            FVDeselectAll);
    SETVTABLE(selectByName,           SelectByName);
    SETVTABLE(selectByScript,         selectByScript);
    SETVTABLE(selectWorthOutputting,  SelectWorthOutputting);
    SETVTABLE(glyphsRefs,             glyphsRefs);
    SETVTABLE(glyphsSplines,          glyphsSplines);
    SETVTABLE(glyphsBoth,             glyphsBoth);
    SETVTABLE(glyphsWhite,            glyphsWhite);
    SETVTABLE(selectChanged,          selectChanged);
    SETVTABLE(selectHintingNeeded,    selectHintingNeeded);
    SETVTABLE(selectAutohintable,     selectAutohintable);
    SETVTABLE(selectByPST,            FVSelectByPST);
    SETVTABLE(selectbyColor,          selectbyColor);

    SETVTABLE(undo,           undo);
    SETVTABLE(redo,           redo);
    SETVTABLE(cut,            cut);
    SETVTABLE(copy,           copy);
    SETVTABLE(paste,          paste);
    SETVTABLE(delete,         delete);
    SETVTABLE(clear,          clear);
    SETVTABLE(undoFontLevel,  undoFontLevel);
    SETVTABLE(removeUndoes,   removeUndoes);

    // sharedmenu_edit_copylist 
    SETVTABLE(copyRef,        copyRef);
    SETVTABLE(copyLookupData, copyLookupData);
    SETVTABLE(copyWidth,      copyWidth);
    SETVTABLE(copyFgBg,       copyFgBg);
    SETVTABLE(copyL2L,        copyL2L);

    // sharedmenu_edit_pastelist 
    SETVTABLE(pasteInto,      pasteInto);
    SETVTABLE(pasteAfter,     pasteAfter);

    // sharedmenu_edit_clearlist 
    SETVTABLE(clearBackground,clearBackground);
    SETVTABLE(join,           join);

    SETVTABLE(setWidth,        setWidth);
    SETVTABLE(metricsCenter,   metricsCenter);
    SETVTABLE(kernPairCloseUp, kernPairCloseUp);
    SETVTABLE(removeKern,      removeKern);
    SETVTABLE(removeVKern,     removeVKern);


    SETVTABLE(gotoChar,            gotoChar);
    SETVTABLE(gotoCharNext,        gotoCharNext);
    SETVTABLE(gotoCharPrev,        gotoCharPrev);
    SETVTABLE(gotoCharNextDefined, gotoCharNextDefined );
    SETVTABLE(gotoCharPrevDefined, gotoCharPrevDefined);
//    SETVTABLE(gotoCharFormer,    gotoCharFormer);
    
    SETVTABLE(numberPoints,        numberPoints);


    SETVTABLE(toggleShowTabs,          toggleShowTabs);
    SETVTABLE(toggleShowRulers,        toggleShowRulers);
    SETVTABLE(toggleShowPaletteTools,  toggleShowPaletteTools);
    SETVTABLE(toggleShowPaletteLayers, toggleShowPaletteLayers);
    SETVTABLE(toggleShowPaletteDocked, toggleShowPaletteDocked);

    SETVTABLE(dialogEmbolden,          dialogEmbolden);
    SETVTABLE(dialogItalic,            dialogItalic);
    SETVTABLE(dialogOblique,           dialogOblique);
    SETVTABLE(dialogCondenseExtend,    dialogCondenseExtend);
    SETVTABLE(dialogXHeight,           dialogXHeight);
    SETVTABLE(dialogStemsCounters,     dialogStemsCounters);
    SETVTABLE(dialogInline,            dialogInline);
    SETVTABLE(dialogOutline,           dialogOutline);
    SETVTABLE(dialogShadow,            dialogShadow);
    SETVTABLE(dialogWireframe,         dialogWireframe);
    SETVTABLE(dialogTransform,         dialogTransform);
    SETVTABLE(dialogPointOfViewProjection, dialogPointOfViewProjection);
    SETVTABLE(dialogNonLinearTransform,    dialogNonLinearTransform);
    SETVTABLE(overlapRemove,           overlapRemove);
    SETVTABLE(overlapIntersect,        overlapIntersect);
//    SETVTABLE(overlapExclude,          overlapExclude);
    SETVTABLE(overlapFindIntersections,overlapFindIntersections);
    SETVTABLE(simplify,                simplify);
    SETVTABLE(simplifyMoreDialog,      simplifyMoreDialog);
    SETVTABLE(simplifyCleanup,         simplifyCleanup);
    SETVTABLE(simplifyCanonicalStartPoint, simplifyCanonicalStartPoint);
    SETVTABLE(simplifyCanonicalContours,   simplifyCanonicalContours);
    SETVTABLE(dialogExpandStroke,      dialogExpandStroke);
    SETVTABLE(dialogCompareLayers,     dialogCompareLayers);
    SETVTABLE(extremaAdd,              extremaAdd);

    SETVTABLE(dialogCharInfo,          dialogCharInfo);
    SETVTABLE(dialogKernPairs,         dialogKernPairs);
    SETVTABLE(dialogLigatures,         dialogLigatures);
    SETVTABLE(accentBuild,             accentBuild);
    SETVTABLE(compositeBuild,          compositeBuild);
    SETVTABLE(duplicateGlyphs,         duplicateGlyphs);

    SETVTABLE(revertToFile,            revertToFile);
    SETVTABLE(revertToBackup,          revertToBackup);
    SETVTABLE(revertGlyphs,            revertGlyphs);

    SETVTABLE(referenceShowDependentRefs, referenceShowDependentRefs);
    SETVTABLE(referenceUnlink,         referenceUnlink);
    

    // hint menu
    SETVTABLE(hintDoAutoHint,             hintDoAutoHint);
    SETVTABLE(hintAutoSubs,               hintAutoSubs);
    SETVTABLE(hintAutoCounter,            hintAutoCounter);
    SETVTABLE(hintDontAutoHint,           hintDontAutoHint);
    SETVTABLE(hintAutoInstr,              hintAutoInstr);
    SETVTABLE(hintEditInstructionsDialog, hintEditInstructionsDialog);
    SETVTABLE(hintEditTable_fpgm,         hintEditTable_fpgm);
    SETVTABLE(hintEditTable_prep,         hintEditTable_prep);
    SETVTABLE(hintEditTable_maxp,         hintEditTable_maxp);
    SETVTABLE(hintEditTable_cvt,          hintEditTable_cvt);
    SETVTABLE(hintRemoveInstructionTables,hintRemoveInstructionTables);
    SETVTABLE(hintSuggestDeltasDialog,    hintSuggestDeltasDialog);
    SETVTABLE(hintClear,                  hintClear);
    SETVTABLE(hintClearInstructions,      hintClearInstructions);
    SETVTABLE(histogramHStemDialog,       histogramHStemDialog);
    SETVTABLE(histogramVStemDialog,       histogramVStemDialog);
    SETVTABLE(histogramBlueValuesDialog,  histogramBlueValuesDialog);


    SETVTABLE(openWindowGlyph,            openWindowGlyph);
    SETVTABLE(openWindowBitmap,           openWindowBitmap);
    SETVTABLE(openWindowMetrics,          openWindowMetrics);

    

    
    // utility to unify some methods
    SETVTABLE(getActiveLayer,  getActiveLayer);
    SETVTABLE(selectionClear,  selectionClear);
    SETVTABLE(selectionAddChar,selectionAddChar);


    
    
    GDrawGetSize(GDrawGetRoot(NULL),&size);

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_icon;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.cursor = ct_pointer;
    wattrs.icon = icon;
    pos.width = sf->desired_col_cnt*fv->cbw+1;
    pos.height = sf->desired_row_cnt*fv->cbh+1;
    pos.x = size.width-pos.width-30; pos.y = nexty;
    nexty += 2*fv->cbh+50;
    if ( nexty+pos.height > size.height )
	nexty = 0;
    fv->gw = gw = GDrawCreateTopWindow(NULL,&pos,fv_e_h,fv,&wattrs);
    FontViewSetTitle(fv);
    GDrawSetWindowTypeName(fv->gw, "FontView");

    if ( !fv_fs_init ) {
	GResEditFind( fontview_re, "FontView.");
	view_bgcol = GResourceFindColor("View.Background",GDrawGetDefaultBackground(NULL));
	fv_fs_init = true;
    }

    memset(&gd,0,sizeof(gd));
    gd.flags = gg_visible | gg_enabled;
//    helplist[6].invoke = FVMenuContextualHelp;
    sharedmenu_update_menus_at_init( (CommonView*)fv, mblist, mblist_extensions_idx );
/* #ifndef _NO_PYTHON */
/*     if ( fvpy_menu!=NULL ) */
/* 	mblist[mblist_extensions_idx].ti.disabled = false; */
/*     mblist[mblist_extensions_idx].sub = fvpy_menu; */
/* #define CALLBACKS_INDEX 4 /\* FIXME: There has to be a better way than this. *\/ */
/* #else */
/* #define CALLBACKS_INDEX 3 /\* FIXME: There has to be a better way than this. *\/ */
/* #endif		/\* _NO_PYTHON *\/ */
/* #ifdef NATIVE_CALLBACKS */
/*     if ( fv_menu!=NULL ) */
/*        mblist[CALLBACKS_INDEX].ti.disabled = false; */
/*     mblist[CALLBACKS_INDEX].sub = fv_menu; */
/* #endif      /\* NATIVE_CALLBACKS *\/ */
    gd.u.menu2 = mblist;
    fv->mb = GMenu2BarCreate( gw, &gd, NULL);
    GGadgetGetSize(fv->mb,&gsize);
    fv->mbh = gsize.height;
    fv->infoh = 1+GDrawPointsToPixels(NULL,fv_fontsize);

    pos.x = 0; pos.y = fv->mbh+fv->infoh;
    FVCreateInnards(fv,&pos);

    if ( !hide ) {
	GDrawSetVisible(gw,true);
	FontViewOpenKids(fv);
    }
return( fv );
}

static FontView *FontView_Append(FontView *fv) {
    /* Normally fontviews get added to the fv list when their windows are */
    /*  created. but we don't create any windows here, so... */
    FontView *test;

    if ( fv_list==NULL ) fv_list = fv;
    else {
	for ( test = fv_list; test->b.next!=NULL; test=(FontView *) test->b.next );
	test->b.next = (FontViewBase *) fv;
    }
return( fv );
}

FontView *FontNew(void) {
return( FontView_Create(SplineFontNew(),false));
}

static void FontView_Free(FontView *fv) {
    int i;
    FontView *prev;
    FontView *fvs;

    if ( fv->b.sf == NULL )	/* Happens when usurping a font to put it into an MM */
	BDFFontFree(fv->filled);
    else if ( fv->b.nextsame==NULL && fv->b.sf->fv==&fv->b ) {
	EncMapFree(fv->b.map);
	if (fv->b.sf != NULL && fv->b.map == fv->b.sf->map) { fv->b.sf->map = NULL; }
	SplineFontFree(fv->b.cidmaster?fv->b.cidmaster:fv->b.sf);
	BDFFontFree(fv->filled);
    } else {
	EncMapFree(fv->b.map);
	if (fv->b.sf != NULL && fv->b.map == fv->b.sf->map) { fv->b.sf->map = NULL; }
	fv->b.map = NULL;
	for ( fvs=(FontView *) (fv->b.sf->fv), i=0 ; fvs!=NULL; fvs = (FontView *) (fvs->b.nextsame) )
	    if ( fvs->filled==fv->filled ) ++i;
	if ( i==1 )
	    BDFFontFree(fv->filled);
	if ( fv->b.sf->fv==&fv->b ) {
	    if ( fv->b.cidmaster==NULL )
		fv->b.sf->fv = fv->b.nextsame;
	    else {
		fv->b.cidmaster->fv = fv->b.nextsame;
		for ( i=0; i<fv->b.cidmaster->subfontcnt; ++i )
		    fv->b.cidmaster->subfonts[i]->fv = fv->b.nextsame;
	    }
	} else {
	    for ( prev = (FontView *) (fv->b.sf->fv); prev->b.nextsame!=&fv->b; prev=(FontView *) (prev->b.nextsame) );
	    prev->b.nextsame = fv->b.nextsame;
	}
    }
#ifndef _NO_FFSCRIPT
    DictionaryFree(fv->b.fontvars);
    free(fv->b.fontvars);
#endif
    free(fv->b.selected);
    free(fv->fontset);
#ifndef _NO_PYTHON
    PyFF_FreeFV(&fv->b);
#endif
    free(fv);
}

static int FontViewWinInfo(FontView *fv, int *cc, int *rc) {
    if ( fv==NULL || fv->colcnt==0 || fv->rowcnt==0 ) {
	*cc = 16; *rc = 4;
return( -1 );
    }

    *cc = fv->colcnt;
    *rc = fv->rowcnt;

return( fv->rowoff*fv->colcnt );
}

static FontViewBase *FVAny(void) { return (FontViewBase *) fv_list; }

static int  FontIsActive(SplineFont *sf) {
    FontView *fv;

    for ( fv=fv_list; fv!=NULL; fv=(FontView *) (fv->b.next) )
	if ( fv->b.sf == sf )
return( true );

return( false );
}

static SplineFont *FontOfFilename(const char *filename) {
    char buffer[1025];
    FontView *fv;

    GFileGetAbsoluteName((char *) filename,buffer,sizeof(buffer));
    for ( fv=fv_list; fv!=NULL ; fv=(FontView *) (fv->b.next) ) {
	if ( fv->b.sf->filename!=NULL && strcmp(fv->b.sf->filename,buffer)==0 )
return( fv->b.sf );
	else if ( fv->b.sf->origname!=NULL && strcmp(fv->b.sf->origname,buffer)==0 )
return( fv->b.sf );
    }
return( NULL );
}

static void FVExtraEncSlots(FontView *fv, int encmax) {
    if ( fv->colcnt!=0 ) {		/* Ie. scripting vs. UI */
	fv->rowltot = (encmax+1+fv->colcnt-1)/fv->colcnt;
	GScrollBarSetBounds(fv->vsb,0,fv->rowltot,fv->rowcnt);
    }
}

static void FV_BiggerGlyphCache(FontView *fv, int gidcnt) {
    if ( fv->filled!=NULL )
	BDFOrigFixup(fv->filled,gidcnt,fv->b.sf);
}

static void FontView_Close(FontView *fv) {
    if ( fv->gw!=NULL )
	GDrawDestroyWindow(fv->gw);
    else
	FontViewRemove(fv);
}


struct fv_interface gdraw_fv_interface = {
    (FontViewBase *(*)(SplineFont *, int)) FontView_Create,
    (FontViewBase *(*)(SplineFont *)) __FontViewCreate,
    (void (*)(FontViewBase *)) FontView_Close,
    (void (*)(FontViewBase *)) FontView_Free,
    (void (*)(FontViewBase *)) FontViewSetTitle,
    FontViewSetTitles,
    FontViewRefreshAll,
    (void (*)(FontViewBase *)) FontView_ReformatOne,
    FontView_ReformatAll,
    (void (*)(FontViewBase *)) FV_LayerChanged,
    FV_ToggleCharChanged,
    (int  (*)(FontViewBase *, int *, int *)) FontViewWinInfo,
    FontIsActive,
    FVAny,
    (FontViewBase *(*)(FontViewBase *)) FontView_Append,
    FontOfFilename,
    (void (*)(FontViewBase *,int)) FVExtraEncSlots,
    (void (*)(FontViewBase *,int)) FV_BiggerGlyphCache,
    (void (*)(FontViewBase *,BDFFont *)) FV_ChangeDisplayBitmap,
    (void (*)(FontViewBase *)) FV_ShowFilled,
    FV_ReattachCVs,
    (void (*)(FontViewBase *)) FVDeselectAll,
    (void (*)(FontViewBase *,int )) FVScrollToGID,
    (void (*)(FontViewBase *,int )) FVScrollToChar,
    (void (*)(FontViewBase *,int )) FV_ChangeGID,
    SF_CloseAllInstrs
};

extern GResInfo charview_ri;
static struct resed view_re[] = {
    {N_("Color|Background"), "Background", rt_color, &view_bgcol, N_("Background color for the drawing area of all views"), NULL, { 0 }, 0, 0 },
    RESED_EMPTY
};
GResInfo view_ri = {
    NULL, NULL,NULL, NULL,
    NULL,
    NULL,
    NULL,
    view_re,
    N_("View"),
    N_("This is an abstract class which defines common features of the\nFontView, CharView, BitmapView and MetricsView"),
    "View",
    "fontforge",
    false,
    0,
    NULL,
    GBOX_EMPTY,
    NULL,
    NULL,
    NULL
};

GResInfo fontview_ri = {
    &charview_ri, NULL,NULL, NULL,
    NULL,
    NULL,
    NULL,
    fontview_re,
    N_("FontView"),
    N_("This is the main fontforge window displaying a font"),
    "FontView",
    "fontforge",
    false,
    0,
    NULL,
    GBOX_EMPTY,
    NULL,
    NULL,
    NULL
};

/* ************************************************************************** */
/* ***************************** Embedded FontViews ************************* */
/* ************************************************************************** */

static void FVCopyInnards(FontView *fv,GRect *pos,int infoh,
	FontView *fvorig,GWindow dw, int def_layer, struct fvcontainer *kf) {

    fv->notactive = true;
    fv->gw = dw;
    fv->infoh = infoh;
    fv->b.container = kf;
    fv->rowcnt = 4; fv->colcnt = 16;
    fv->b.active_layer = def_layer;
    FVCreateInnards(fv,pos);
    memcpy(fv->b.selected,fvorig->b.selected,fv->b.map->enccount);
    fv->rowoff = (fvorig->rowoff*fvorig->colcnt)/fv->colcnt;
}

void KFFontViewInits(struct kf_dlg *kf,GGadget *drawable) {
    GGadgetData gd;
    GRect pos, gsize, sbsize;
    GWindow dw = GDrawableGetWindow(drawable);
    int infoh;
    int ps;
    FontView *fvorig = (FontView *) kf->sf->fv;

    FontViewInit();

    kf->dw = dw;

    memset(&gd,0,sizeof(gd));
    gd.flags = gg_visible | gg_enabled;
//    helplist[6].invoke = FVMenuContextualHelp;
    gd.u.menu2 = mblist;
    kf->mb = GMenu2BarCreate( dw, &gd, NULL);
    GGadgetGetSize(kf->mb,&gsize);
    kf->mbh = gsize.height;
    kf->guts = drawable;

    ps = kf->sf->display_size; kf->sf->display_size = -24;
    kf->first_fv = __FontViewCreate(kf->sf); kf->first_fv->b.container = (struct fvcontainer *) kf;
    kf->second_fv = __FontViewCreate(kf->sf); kf->second_fv->b.container = (struct fvcontainer *) kf;

    kf->infoh = infoh = 1+GDrawPointsToPixels(NULL,fv_fontsize);
    kf->first_fv->mbh = kf->mbh;
    pos.x = 0; pos.y = kf->mbh+infoh+kf->fh+4;
    pos.width = 16*kf->first_fv->cbw+1;
    pos.height = 4*kf->first_fv->cbh+1;

    GDrawSetUserData(dw,kf->first_fv);
    FVCopyInnards(kf->first_fv,&pos,infoh,fvorig,dw,kf->def_layer,(struct fvcontainer *) kf);
    pos.height = 4*kf->first_fv->cbh+1;		/* We don't know the real fv->cbh until after creating the innards. The size of the last window is probably wrong, we'll fix later */
    kf->second_fv->mbh = kf->mbh;
    kf->label2_y = pos.y + pos.height+2;
    pos.y = kf->label2_y + kf->fh + 2;
    GDrawSetUserData(dw,kf->second_fv);
    FVCopyInnards(kf->second_fv,&pos,infoh,fvorig,dw,kf->def_layer,(struct fvcontainer *) kf);

    kf->sf->display_size = ps;

    GGadgetGetSize(kf->second_fv->vsb,&sbsize);
    gsize.x = gsize.y = 0;
    gsize.width = pos.width + sbsize.width;
    gsize.height = pos.y+pos.height;
    GGadgetSetDesiredSize(drawable,NULL,&gsize);
}
/* ************************************************************************** */
/* ************************** Glyph Set from Selection ********************** */
/* ************************************************************************** */

struct gsd {
    struct fvcontainer base;
    FontView *fv;
    int done;
    int good;
    GWindow gw;
};

static void gs_activateMe(struct fvcontainer *UNUSED(fvc), FontViewBase *UNUSED(fvb)) {
    /*struct gsd *gs = (struct gsd *) fvc;*/
}

static void gs_charEvent(struct fvcontainer *fvc,void *event) {
    struct gsd *gs = (struct gsd *) fvc;
    FVChar(gs->fv,event);
}

static void gs_doClose(struct fvcontainer *fvc) {
    struct gsd *gs = (struct gsd *) fvc;
    gs->done = true;
}

#define CID_Guts	1000
#define CID_TopBox	1001

static void gs_doResize(struct fvcontainer *fvc, FontViewBase *UNUSED(fvb),
	int width, int height) {
    struct gsd *gs = (struct gsd *) fvc;
    /*FontView *fv = (FontView *) fvb;*/
    GRect size;

    memset(&size,0,sizeof(size));
    size.width = width; size.height = height;
    GGadgetSetDesiredSize(GWidgetGetControl(gs->gw,CID_Guts),
	    NULL,&size);
    GHVBoxFitWindow(GWidgetGetControl(gs->gw,CID_TopBox));
}

static struct fvcontainer_funcs glyphset_funcs = {
    fvc_glyphset,
    true,			/* Modal dialog. No charviews, etc. */
    gs_activateMe,
    gs_charEvent,
    gs_doClose,
    gs_doResize
};

static int GS_OK(GGadget *g, GEvent *e) {

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct gsd *gs = GDrawGetUserData(GGadgetGetWindow(g));
	gs->done = true;
	gs->good = true;
    }
return( true );
}

static int GS_Cancel(GGadget *g, GEvent *e) {
    struct gsd *gs;

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	gs = GDrawGetUserData(GGadgetGetWindow(g));
	gs->done = true;
    }
return( true );
}

static void gs_sizeSet(struct gsd *gs,GWindow dw) {
    GRect size, gsize;
    int width, height, y;
    int cc, rc, topchar;
    GRect subsize;
    FontView *fv = gs->fv;

    if ( gs->fv->vsb==NULL )
return;

    GDrawGetSize(dw,&size);
    GGadgetGetSize(gs->fv->vsb,&gsize);
    width = size.width - gsize.width;
    height = size.height - gs->fv->mbh - gs->fv->infoh;

    y = gs->fv->mbh + gs->fv->infoh;

    topchar = fv->rowoff*fv->colcnt;
    cc = (width-1) / fv->cbw;
    if ( cc<1 ) cc=1;
    rc = (height-1)/ fv->cbh;
    if ( rc<1 ) rc = 1;
    subsize.x = 0; subsize.y = 0;
    subsize.width = cc*fv->cbw + 1;
    subsize.height = rc*fv->cbh + 1;
    GDrawResize(fv->v,subsize.width,subsize.height);
    GDrawMove(fv->v,0,y);
    GGadgetMove(fv->vsb,subsize.width,y);
    GGadgetResize(fv->vsb,gsize.width,subsize.height);

    fv->colcnt = cc; fv->rowcnt = rc;
    fv->width = subsize.width; fv->height = subsize.height;
    fv->rowltot = (fv->b.map->enccount+fv->colcnt-1)/fv->colcnt;
    GScrollBarSetBounds(fv->vsb,0,fv->rowltot,fv->rowcnt);
    fv->rowoff = topchar/fv->colcnt;
    if ( fv->rowoff>=fv->rowltot-fv->rowcnt )
        fv->rowoff = fv->rowltot-fv->rowcnt;
    if ( fv->rowoff<0 ) fv->rowoff =0;
    GScrollBarSetPos(fv->vsb,fv->rowoff);

    GDrawRequestExpose(fv->v,NULL,true);
}

static int gs_sub_e_h(GWindow pixmap, GEvent *event) {
    FontView *active_fv;
    struct gsd *gs;

    if ( event->type==et_destroy )
return( true );

    active_fv = (FontView *) GDrawGetUserData(pixmap);
    gs = (struct gsd *) (active_fv->b.container);

    if (( event->type==et_mouseup || event->type==et_mousedown ) &&
	    (event->u.mouse.button>=4 && event->u.mouse.button<=7) ) {
return( GGadgetDispatchEvent(active_fv->vsb,event));
    }


    switch ( event->type ) {
      case et_expose:
	FVDrawInfo(active_fv,pixmap,event);
      break;
      case et_char:
	gs_charEvent(&gs->base,event);
      break;
      case et_mousedown:
return(false);
      break;
      case et_mouseup: case et_mousemove:
return(false);
      case et_resize:
        gs_sizeSet(gs,pixmap);
      break;
    }
return( true );
}

static int gs_e_h(GWindow gw, GEvent *event) {
    struct gsd *gs = GDrawGetUserData(gw);

    switch ( event->type ) {
      case et_close:
	gs->done = true;
      break;
      case et_char:
	FVChar(gs->fv,event);
      break;
    }
return( true );
}

char *GlyphSetFromSelection(SplineFont *sf,int def_layer,char *current) {
    struct gsd gs;
    GRect pos;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[5], boxes[3];
    GGadgetCreateData *varray[21], *buttonarray[8];
    GTextInfo label[5];
    int i,j,k,guts_row,gid,enc,len;
    char *ret, *rpt;
    SplineChar *sc;
    GGadget *drawable;
    GWindow dw;
    GGadgetData gd;
    GRect gsize, sbsize;
    int infoh, mbh;
    int ps;
    FontView *fvorig = (FontView *) sf->fv;
    GGadget *mb;
    char *start, *pt; int ch;

    FontViewInit();

    memset(&wattrs,0,sizeof(wattrs));
    memset(&gcd,0,sizeof(gcd));
    memset(&boxes,0,sizeof(boxes));
    memset(&label,0,sizeof(label));
    memset(&gs,0,sizeof(gs));

    gs.base.funcs = &glyphset_funcs;

    wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_isdlg|wam_restrict;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.restrict_input_to_me = true;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.utf8_window_title = _("Glyph Set by Selection") ;
    wattrs.is_dlg = true;
    pos.x = pos.y = 0;
    pos.width = 100;
    pos.height = 100;
    gs.gw = GDrawCreateTopWindow(NULL,&pos,gs_e_h,&gs,&wattrs);

    i = j = 0;

    guts_row = j/2;
    gcd[i].gd.flags = gg_enabled|gg_visible;
    gcd[i].gd.cid = CID_Guts;
    gcd[i].gd.u.drawable_e_h = gs_sub_e_h;
    gcd[i].creator = GDrawableCreate;
    varray[j++] = &gcd[i++]; varray[j++] = NULL;

    label[i].text = (unichar_t *) _("Select glyphs in the font view above.\nThe selected glyphs become your glyph class.");
    label[i].text_is_1byte = true;
    gcd[i].gd.label = &label[i];
    gcd[i].gd.flags = gg_enabled|gg_visible;
    gcd[i].creator = GLabelCreate;
    varray[j++] = &gcd[i++]; varray[j++] = NULL;

    gcd[i].gd.flags = gg_visible | gg_enabled | gg_but_default;
    label[i].text = (unichar_t *) _("_OK");
    label[i].text_is_1byte = true;
    label[i].text_in_resource = true;
    gcd[i].gd.label = &label[i];
    gcd[i].gd.handle_controlevent = GS_OK;
    gcd[i++].creator = GButtonCreate;

    gcd[i].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
    label[i].text = (unichar_t *) _("_Cancel");
    label[i].text_is_1byte = true;
    label[i].text_in_resource = true;
    gcd[i].gd.label = &label[i];
    gcd[i].gd.handle_controlevent = GS_Cancel;
    gcd[i++].creator = GButtonCreate;

    buttonarray[0] = GCD_Glue; buttonarray[1] = &gcd[i-2]; buttonarray[2] = GCD_Glue;
    buttonarray[3] = GCD_Glue; buttonarray[4] = &gcd[i-1]; buttonarray[5] = GCD_Glue;
    buttonarray[6] = NULL;
    boxes[2].gd.flags = gg_enabled|gg_visible;
    boxes[2].gd.u.boxelements = buttonarray;
    boxes[2].creator = GHBoxCreate;
    varray[j++] = &boxes[2]; varray[j++] = NULL; varray[j++] = NULL;

    boxes[0].gd.pos.x = boxes[0].gd.pos.y = 2;
    boxes[0].gd.flags = gg_enabled|gg_visible;
    boxes[0].gd.u.boxelements = varray;
    boxes[0].gd.cid = CID_TopBox;
    boxes[0].creator = GHVGroupCreate;

    GGadgetsCreate(gs.gw,boxes);

    GHVBoxSetExpandableRow(boxes[0].ret,guts_row);
    GHVBoxSetExpandableCol(boxes[2].ret,gb_expandgluesame);

    drawable = GWidgetGetControl(gs.gw,CID_Guts);
    dw = GDrawableGetWindow(drawable);

    memset(&gd,0,sizeof(gd));
    gd.flags = gg_visible | gg_enabled;
//    helplist[6].invoke = FVMenuContextualHelp;
    gd.u.menu2 = mblist;
    mb = GMenu2BarCreate( dw, &gd, NULL);
    GGadgetGetSize(mb,&gsize);
    mbh = gsize.height;

    ps = sf->display_size; sf->display_size = -24;
    gs.fv = __FontViewCreate(sf);

    infoh = 1+GDrawPointsToPixels(NULL,fv_fontsize);
    gs.fv->mbh = mbh;
    pos.x = 0; pos.y = mbh+infoh;
    pos.width = 16*gs.fv->cbw+1;
    pos.height = 4*gs.fv->cbh+1;

    GDrawSetUserData(dw,gs.fv);
    FVCopyInnards(gs.fv,&pos,infoh,fvorig,dw,def_layer,(struct fvcontainer *) &gs);
    pos.height = 4*gs.fv->cbh+1;	/* We don't know the real fv->cbh until after creating the innards. The size of the last window is probably wrong, we'll fix later */
    memset(gs.fv->b.selected,0,gs.fv->b.map->enccount);
    if ( current!=NULL && strcmp(current,_("{Everything Else}"))!=0 ) {
	int first = true;
	for ( start = current; *start==' '; ++start );
	while ( *start ) {
	    for ( pt=start; *pt!='\0' && *pt!=' '; ++pt );
	    ch = *pt; *pt='\0';
	    sc = SFGetChar(sf,-1,start);
	    *pt = ch;
	    if ( sc!=NULL && (enc = gs.fv->b.map->backmap[sc->orig_pos])!=-1 ) {
		gs.fv->b.selected[enc] = true;
		if ( first ) {
		    first = false;
		    gs.fv->rowoff = enc/gs.fv->colcnt;
		}
	    }
	    start = pt;
	    while ( *start==' ' ) ++start;
	}
    }
    sf->display_size = ps;

    GGadgetGetSize(gs.fv->vsb,&sbsize);
    gsize.x = gsize.y = 0;
    gsize.width = pos.width + sbsize.width;
    gsize.height = pos.y+pos.height;
    GGadgetSetDesiredSize(drawable,NULL,&gsize);

    GHVBoxFitWindow(boxes[0].ret);
    GDrawSetVisible(gs.gw,true);
    while ( !gs.done )
	GDrawProcessOneEvent(NULL);

    ret = rpt = NULL;
    if ( gs.good ) {
	for ( k=0; k<2; ++k ) {
	    len = 0;
	    for ( enc=0; enc<gs.fv->b.map->enccount; ++enc ) {
		if ( gs.fv->b.selected[enc] &&
			(gid=gs.fv->b.map->map[enc])!=-1 &&
			(sc = sf->glyphs[gid])!=NULL ) {
		    char *repr = SCNameUniStr( sc );
		    if ( ret==NULL )
			len += strlen(repr)+2;
		    else {
			strcpy(rpt,repr);
			rpt += strlen( repr );
			free(repr);
			*rpt++ = ' ';
		    }
		}
	    }
	    if ( k==0 )
		ret = rpt = malloc(len+1);
	    else if ( rpt!=ret && rpt[-1]==' ' )
		rpt[-1]='\0';
	    else
		*rpt='\0';
	}
    }
    FontViewFree(&gs.fv->b);
    GDrawSetUserData(gs.gw,NULL);
    GDrawSetUserData(dw,NULL);
    GDrawDestroyWindow(gs.gw);
return( ret );
}


/****************************************/
/****************************************/
/****************************************/

int FontViewFind_byXUID( FontViewBase* fv, void* udata )
{
    if( !fv || !fv->sf )
	return 0;
    return !strcmp( fv->sf->xuid, (char*)udata );
}

int FontViewFind_byXUIDConnected( FontViewBase* fv, void* udata )
{
    if( !fv || !fv->sf )
	return 0;
    return ( fv->collabState == cs_server || fv->collabState == cs_client )
	&& fv->sf->xuid
	&& !strcmp( fv->sf->xuid, (char*)udata );
}

int FontViewFind_byCollabPtr( FontViewBase* fv, void* udata )
{
    if( !fv || !fv->sf )
	return 0;
    return fv->collabClient == udata;
}

int FontViewFind_byCollabBasePort( FontViewBase* fv, void* udata )
{
    if( !fv || !fv->sf || !fv->collabClient )
	return 0;
    int port = (int)(intptr_t)udata;
    return port == collabclient_getBasePort( fv->collabClient );
}

int FontViewFind_bySplineFont( FontViewBase* fv, void* udata )
{
    if( !fv || !fv->sf )
	return 0;
    return fv->sf == udata;
}

static int FontViewFind_ActiveWindow( FontViewBase* fvb, void* udata )
{
    FontView* fv = (FontView*)fvb;
    return( fv->gw == udata || fv->v == udata );
}

FontViewBase* FontViewFindActive()
{
    return (FontViewBase*) ActiveFontView;
    /* GWindow w = GWindowGetCurrentFocusTopWindow(); */
    /* FontViewBase* ret = FontViewFind( FontViewFind_ActiveWindow, w ); */
    /* return ret; */
}



FontViewBase* FontViewFind( int (*testFunc)( FontViewBase*, void* udata ), void* udata )
{
    FontViewBase *fv;
    printf("FontViewFind(top) fv_list:%p\n", fv_list );
    for ( fv = (FontViewBase*)fv_list; fv!=NULL; fv=fv->next )
    {
	if( testFunc( fv, udata ))
	    return fv;
    }
    return 0;
}

FontView* FontViewFindUI( int (*testFunc)( FontViewBase*, void* udata ), void* udata )
{
    return (FontView*)FontViewFind( testFunc, udata );
}


/****************************************/
/****************************************/
/****************************************/

/* local variables: */
/* tab-width: 8     */
/* end:             */
