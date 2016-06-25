/* Copyright (C) 2000-2012 by George Williams */
/* Copyright (C) 2016      by Ben Martin */
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
#include "sharedmenu.h"
#include "fontview.h"
#include "charview.h"
#include "metricsview.h"
#include "fontforge.h"
#include "classtypeui.h"
#include "fontforgeui.h"
#include "groups.h"  // group_root
#include "collabclientui.h"

 extern int palettes_docked;

#include <unistd.h>


#define MENUITEM_LINE  { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 1, 0, 0, 0, '\0' }, NULL, NULL, NULL, 0, 0 },
#define UN_(x) (unichar_t *) N_(x)
#define GI(x)  (GImage *)(x)
#define MENUBODY_DEFAULT          NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0
#define MENUBODY_DEFAULT_DISABLED NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 1, 1, 0, 0, 0, 0, 1, 1, 0
#define MENUBODYC_DEFAULT          COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0
#define MENUBODYC_DEFAULT_DISABLED COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 1, 1, 0, 0, 0, 0, 1, 1, 0
#define MENUBODYD_DEFAULT          COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0


unichar_t* script_menu_names[SCRIPT_MENU_MAX];
char*      script_filenames [SCRIPT_MENU_MAX];

// needed for walking fontviews, should have a visitor for that,
// but one thing at a time.
extern FontView *fv_list;
extern int onlycopydisplayed;

#define MENU_CHECK_VARIABLES				\
    CommonView*  cc = tryObtainGDataCommonView( gw );   \
    CharView*    cv = tryObtainGDataCharView( gw );     \
    FontView*    fv = tryObtainGDataFontView( gw );     \
    MetricsView* mv = tryObtainGDataMetricsView( gw );	\
    int isCharView  = (cv!=0);				\
    int isFontView  = (cv==0 && mv==0);                 \
    int isMetricsView = (mv != 0 );


#define MAKETRAMP(FNAME) \
static void sm_##FNAME(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) { \
    CommonView* cc = tryObtainGDataCommonView( gw ); \
    if( cc->m_sharedmenu_funcs.FNAME ) \
	cc->m_sharedmenu_funcs.FNAME( cc ); \
    } \
static int sm_avail_##FNAME(CommonView* cc) {				\
    return( cc->m_sharedmenu_funcs.FNAME != 0 );			\
}

int SCToPos( SplineChar* sc ) 
{
    int pos = sc->unicodeenc;
    return pos;
}

static void setAllDisabled( struct gmenuitem *mi, int v )
{
    for ( mi = mi->sub; mi->ti.text!=NULL || mi->ti.line ; ++mi ) {
	mi->ti.disabled = v;
    }
}


/****/
    
    MAKETRAMP(dialogLoadWordList);
	




void sharedmenu_file_check(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e))
{
    MENU_CHECK_VARIABLES;
    
    FontView *fvs;
    int in_modal = (fv->b.container!=NULL && fv->b.container->funcs->is_modal);

    for ( mi = mi->sub; mi && (mi->ti.text!=NULL || mi->ti.line); ++mi ) {
	switch ( mi->mid ) {
	  case SMID_GenerateTTC:
	      for ( fvs=fv_list; fvs!=NULL; fvs=(FontView *) (fvs->b.next) ) {
		  if ( fvs!=fv )
		      break;
	      }
	      mi->ti.disabled = fvs==NULL;
	      break;
	  case SMID_Revert:
	      mi->ti.disabled = fv->b.sf->origname==NULL || fv->b.sf->new;
	      break;
	  case SMID_RevertToBackup:
	      /* We really do want to use filename here and origname above */
	      mi->ti.disabled = true;
	      if ( fv->b.sf->filename!=NULL ) {
		  if ( fv->b.sf->backedup == bs_dontknow ) {
		      char *buf = malloc(strlen(fv->b.sf->filename)+20);
		      strcpy(buf,fv->b.sf->filename);
		      if ( fv->b.sf->compression!=0 )
			  strcat(buf,compressors[fv->b.sf->compression-1].ext);
		      strcat(buf,"~");
		      if ( access(buf,F_OK)==0 )
			  fv->b.sf->backedup = bs_backedup;
		      else
			  fv->b.sf->backedup = bs_not;
		      free(buf);
		  }
		  if ( fv->b.sf->backedup == bs_backedup )
		      mi->ti.disabled = false;
	      }
	      break;
	  case SMID_Recent:
	      mi->ti.disabled = !RecentFilesAny();
	      break;
	  case SMID_ScriptMenu:
	      mi->ti.disabled = script_menu_names[0]==NULL;
	      break;
	  case SMID_Print:
	      mi->ti.disabled = fv->b.sf->onlybitmaps || in_modal;
	      break;
	  case SMID_AddWordList:
	      mi->ti.disabled = isFontView;
	      break;
	  case SMID_CloseTab:
	      mi->ti.disabled = 1;
	      if( cv ) {
		  mi->ti.disabled = cv->tabs==NULL || cv->former_cnt<=1;
	      }
	      break;
	}
    }
}

static GMenuItem2 dummyitem[] = {
    { { UN_("Font|_New"), NULL, MENUBODYC_DEFAULT, 'N' }, NULL, NULL, NULL, 0, 0 },
    GMENUITEM2_EMPTY
};

GMenuItem2 sharedmenu_exportlist[] = {
    { { UN_("_Font..."),       GI("filegenerate.png"), MENUBODYC_DEFAULT, 'F' },
                               H_("Font...|No Shortcut"), NULL, NULL, FVMenuGenerate, 0 },
    { { UN_("_Mac Family..."), GI("filegeneratefamily.png"), MENUBODYC_DEFAULT, 'M' },
                               H_("Mac Family...|No Shortcut"), NULL, NULL, FVMenuGenerateFamily, 0 },
    { { UN_("_TTC..."),        GI("filegeneratefamily.png"), MENUBODYC_DEFAULT, 'T' },
                               H_("TTC...|No Shortcut"), NULL, NULL, FVMenuGenerateTTC, SMID_GenerateTTC },
    { { UN_("_UFO..."),        GI("filegeneratefamily.png"), MENUBODYC_DEFAULT, 'T' },
                               H_("UFO...|No Shortcut"), NULL, NULL, FVMenuGenerateUFO, SMID_GenerateUFO },
    { { UN_("_Graphic..."),    GI("filegeneratefamily.png"), MENUBODYC_DEFAULT, 'T' },
                               H_("Graphic...|No Shortcut"), NULL, NULL, FVMenuGenerateGraphic, SMID_GenerateGraphic },	    
    GMENUITEM2_EMPTY
};


GMenuItem2 sharedmenu_file[] = {
    { { UN_("Font|_New"),    GI("filenew.png"),    MENUBODYC_DEFAULT, 'N' }, H_("New|No Shortcut"), NULL, NULL, MenuNew, 0 },
    { { UN_("_Open..."),     GI("fileopen.png"),   MENUBODYC_DEFAULT, 'O' }, H_("Open...|No Shortcut"), NULL, NULL, FVMenuOpen, 0 },
    { { UN_("Open Recen_t"), GI("filerecent.png"), MENUBODYC_DEFAULT, 't' }, H_("Open Recent|No Shortcut"), dummyitem, MenuRecentBuild, NULL, SMID_Recent },
    MENUITEM_LINE
    { { UN_("_Save"),        GI("filesave.png"),   MENUBODYC_DEFAULT, 'S' }, H_("Save|No Shortcut"), NULL, NULL, FVMenuSave, 0 },
    { { UN_("S_ave as..."),  GI("filesaveas.png"), MENUBODYC_DEFAULT, 'a' }, H_("Save as...|No Shortcut"), NULL, NULL, FVMenuSaveAs, 0 },
    { { UN_("Export"),       GI("fileexport.png"), MENUBODYC_DEFAULT, 'E' }, H_("Export|No Shortcut"), sharedmenu_exportlist, NULL, NULL, 0 },
    MENUITEM_LINE
    { { UN_("Import..."),    GI("fileimport.png"), MENUBODYC_DEFAULT, 'I' }, H_("Import...|No Shortcut"), NULL, NULL, FVMenuImport, 0 },
    MENUITEM_LINE
    { { UN_("_Merge Fonts..."), GI("elementmergefonts.png"), MENUBODYC_DEFAULT, 'M' }, H_("Merge Fonts...|No Shortcut"), NULL, NULL, FVMenuMergeFonts, SMID_MergeFonts },
    { { UN_("Load _Word List..."), (GImage *) 0,  MENUBODYC_DEFAULT_DISABLED, '\0' }, H_("Load Word List...|No Shortcut"), NULL, NULL, sm_dialogLoadWordList, SMID_AddWordList },
    MENUITEM_LINE
    { { UN_("Print..."),     GI("fileprint.png"), MENUBODYC_DEFAULT, 'P' }, H_("Print|No Shortcut"), NULL, NULL, FVMenuPrint, SMID_Print },
    MENUITEM_LINE
//    { { UN_("_Themes Editor..."), GI("menuempty.png"), MENUBODYC_DEFAULT, 'e' }, H_("Themes Editor...|No Shortcut"), NULL, NULL, MenuXRes, 0 },
    { { UN_("Preferences..."),GI("fileprefs.png"), MENUBODYC_DEFAULT, 'e' }, H_("Preferences...|No Shortcut"), NULL, NULL, MenuPrefs, 0 },
    MENUITEM_LINE
    { { UN_("C_lose Tab"),   GI("menuempty.png"), COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 1, 1, 0, 0, 0, 0, 1, 1, 0, 'C' }, H_("Close Tab|No Shortcut"), NULL, NULL, FVMenuCloseTab, SMID_CloseTab },
    { { UN_("_Close"),       GI("fileclose.png"), MENUBODYC_DEFAULT, 'C' }, H_("Close|No Shortcut"), NULL, NULL, FVMenuClose, 0 },
    { { UN_("Quit"),         GI("filequit.png"),  MENUBODYC_DEFAULT, 'Q' }, H_("Quit|Ctl+Q"), /* WARNING: THIS BINDING TO PROPERLY INITIALIZE KEYBOARD INPUT */
      NULL, NULL, FVMenuExit, 0 },
    GMENUITEM2_EMPTY
};

/*********************************************************************************/
/*********************************************************************************/
/*********************************************************************************/
/*********************************************************************************/



static void sm_copyWidth(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e)) {
    CommonView* cc = tryObtainGDataCommonView( gw );
    enum undotype undotype = mi->mid==SMID_CopyWidth?ut_width:
	mi->mid==SMID_CopyVWidth?ut_vwidth:
	mi->mid==SMID_CopyLBearing?ut_lbearing:
	ut_rbearing;

    SplineFont* sf = tryObtainCastSplineFont( cc );

    if ( mi->mid==SMID_CopyVWidth && sf && !sf->hasvmetrics )
	return;
    if( cc->m_sharedmenu_funcs.copyWidth )
	cc->m_sharedmenu_funcs.copyWidth( cc, undotype );
}



MAKETRAMP(copyRef);
MAKETRAMP(copyLookupData);
MAKETRAMP(copyFgBg);
MAKETRAMP(copyL2L);

static GMenuItem2 sharedmenu_edit_copylist[] = {
    { { UN_("R_eference"), GI("editcopyref.png"), MENUBODYC_DEFAULT, 'o' }, H_("Reference|No Shortcut"), NULL, NULL, sm_copyRef, SMID_CopyRef },
    { { UN_("Lookup Data"), GI("editcopylookupdata.png"), MENUBODYC_DEFAULT, 'o' }, H_("Lookup Data|No Shortcut"), NULL, NULL, sm_copyLookupData, SMID_CopyLookupData },
    { { UN_("_Width"), GI("editcopywidth.png"), MENUBODYC_DEFAULT, 'W' }, H_("Width|No Shortcut"), NULL, NULL, sm_copyWidth, SMID_CopyWidth },
    { { UN_("_VWidth"), GI("editcopyvwidth.png"), MENUBODYC_DEFAULT, 'V' }, H_("VWidth|No Shortcut"), NULL, NULL, sm_copyWidth, SMID_CopyVWidth },
    { { UN_("L_Bearing"), GI("editcopylbearing.png"), MENUBODYC_DEFAULT, 'p' }, H_("LBearing|No Shortcut"), NULL, NULL, sm_copyWidth, SMID_CopyLBearing },
    { { UN_("R_Bearing"), GI("editcopyrbearing.png"), MENUBODYC_DEFAULT, 'g' }, H_("RBearing|No Shortcut"), NULL, NULL, sm_copyWidth, SMID_CopyRBearing },
	{ { UN_("F_g To Bg"), GI("editcopyfg2bg.png"), MENUBODYC_DEFAULT, 'F' }, H_("Fg To Bg|No Shortcut"), NULL, NULL, sm_copyFgBg, SMID_CopyFgToBg },
    { { UN_("L_ayer To Layer"), GI("editcopylayer2layer.png"), MENUBODYC_DEFAULT, 'F' }, H_("Layer To Layer|No Shortcut"), NULL, NULL, sm_copyL2L, SMID_CopyL2L },
    { { UN_("Copy Gri_d Fit"), GI("menuempty.png"), MENUBODYC_DEFAULT, '\0' }, H_("Copy Grid Fit|No Shortcut"), NULL, NULL, CVMenuCopyGridFit, SMID_CopyGridFit },
    GMENUITEM2_EMPTY
};

MAKETRAMP(pasteInto);
MAKETRAMP(pasteAfter);

static GMenuItem2 sharedmenu_edit_pastelist[] = {
    { { UN_("Into"), GI("editpasteinto.png"), MENUBODYC_DEFAULT, '\0' }, H_("Into|No Shortcut"), NULL, NULL, sm_pasteInto, SMID_PasteInto },
    { { UN_("After"), GI("editpasteafter.png"), MENUBODYC_DEFAULT, '\0' }, H_("After|No Shortcut"), NULL, NULL, sm_pasteAfter, SMID_PasteAfter },
    GMENUITEM2_EMPTY
};

MAKETRAMP(clearBackground);
MAKETRAMP(join);

static GMenuItem2 sharedmenu_edit_clearlist[] = {
    { { UN_("Background"), GI("editclearback.png"), MENUBODYC_DEFAULT, 'B' }, H_("Background|No Shortcut"), NULL, NULL, sm_clearBackground, SMID_ClearBackground },
     { { (unichar_t *) N_("Special Data"), GI("menuempty.png"), MENUBODYC_DEFAULT, 'R' }, H_("Clear Special Data|No Shortcut"), NULL, NULL, FVMenuClearSpecialData, SMID_ClearSpecialData },
    GMENUITEM2_EMPTY
};


MAKETRAMP(revertToFile);
MAKETRAMP(revertToBackup);
MAKETRAMP(revertGlyphs);
	
static GMenuItem2 sharedmenu_edit_revertlist[] = {
    { { UN_("to File on Disk"),  MENUBODY_DEFAULT, 'B' }, H_("Background|No Shortcut"), NULL, NULL, sm_revertToFile, SMID_Revert },
    { { UN_("to Backup"),        MENUBODY_DEFAULT, 'B' }, H_("Background|No Shortcut"), NULL, NULL, sm_revertToBackup, SMID_RevertToBackup },
    { { UN_("Glyph"),            MENUBODY_DEFAULT, 'B' }, H_("Background|No Shortcut"), NULL, NULL, sm_revertGlyphs, SMID_RevertGlyph },        
    GMENUITEM2_EMPTY
};

static void sharedmenu_edit_revertlist_check(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e))
{
    MENU_CHECK_VARIABLES;
    int pos = FVAnyCharSelected(fv);
    if( isMetricsView )
    {
	setAllDisabled(mi,1);
	return;
    }
	
    
    for ( mi = mi->sub; mi->ti.text!=NULL || mi->ti.line ; ++mi ) {
	switch ( mi->mid ) {
	  case SMID_Revert:
	      mi->ti.disabled = fv->b.sf->origname==NULL || fv->b.sf->new;
	      break;
	  case SMID_RevertGlyph:
	      mi->ti.disabled = fv->b.sf->origname==NULL || fv->b.sf->new;
	      if( isFontView && pos == -1 )
		  mi->ti.disabled = 1;
	      break;
	  case SMID_RevertToBackup:
	      /* We really do want to use filename here and origname above */
	      mi->ti.disabled = true;
	      if ( fv->b.sf->filename!=NULL ) {
		  if ( fv->b.sf->backedup == bs_dontknow ) {
		      char *buf = malloc(strlen(fv->b.sf->filename)+20);
		      strcpy(buf,fv->b.sf->filename);
		      if ( fv->b.sf->compression!=0 )
			  strcat(buf,compressors[fv->b.sf->compression-1].ext);
		      strcat(buf,"~");
		      if ( access(buf,F_OK)==0 )
			  fv->b.sf->backedup = bs_backedup;
		      else
			  fv->b.sf->backedup = bs_not;
		      free(buf);
		  }
		  if ( fv->b.sf->backedup == bs_backedup )
		      mi->ti.disabled = false;
	      }
	      break;
	}
    }
}


static void sm_selectbyColor(GWindow gw, struct gmenuitem *mi, GEvent *e) {
    CommonView* cc = tryObtainGDataCommonView( gw );
    Color col = (Color) (intpt) (mi->ti.userdata);
    int merge = SelMergeType(e);
    if( cc->m_sharedmenu_funcs.selectbyColor )
	cc->m_sharedmenu_funcs.selectbyColor( cc, merge, col );
}

static GMenuItem2 sclist[] = {
    { { UN_("Color|Choose..."), (GImage *)"colorwheel.png", COLOR_DEFAULT, COLOR_DEFAULT, (void *) -10, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, '\0' }, H_("Color Choose|No Shortcut"), NULL, NULL, sm_selectbyColor, 0 },
    { { (unichar_t *)  N_("Color|Default"), &def_image, COLOR_DEFAULT, COLOR_DEFAULT, (void *) COLOR_DEFAULT, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, '\0' }, H_("Default|No Shortcut"), NULL, NULL, sm_selectbyColor, 0 },
    { { NULL, &white_image, COLOR_DEFAULT, COLOR_DEFAULT, (void *) 0xffffff, NULL, 0, 1, 0, 0, 0, 0, 0, 0, 0, '\0' }, NULL, NULL, NULL, sm_selectbyColor, 0 },
    { { NULL, &red_image, COLOR_DEFAULT, COLOR_DEFAULT, (void *) 0xff0000, NULL, 0, 1, 0, 0, 0, 0, 0, 0, 0, '\0' }, NULL, NULL, NULL, sm_selectbyColor, 0 },
    { { NULL, &green_image, COLOR_DEFAULT, COLOR_DEFAULT, (void *) 0x00ff00, NULL, 0, 1, 0, 0, 0, 0, 0, 0, 0, '\0' }, NULL, NULL, NULL, sm_selectbyColor, 0 },
    { { NULL, &blue_image, COLOR_DEFAULT, COLOR_DEFAULT, (void *) 0x0000ff, NULL, 0, 1, 0, 0, 0, 0, 0, 0, 0, '\0' }, NULL, NULL, NULL, sm_selectbyColor, 0 },
    { { NULL, &yellow_image, COLOR_DEFAULT, COLOR_DEFAULT, (void *) 0xffff00, NULL, 0, 1, 0, 0, 0, 0, 0, 0, 0, '\0' }, NULL, NULL, NULL, sm_selectbyColor, 0 },
    { { NULL, &cyan_image, COLOR_DEFAULT, COLOR_DEFAULT, (void *) 0x00ffff, NULL, 0, 1, 0, 0, 0, 0, 0, 0, 0, '\0' }, NULL, NULL, NULL, sm_selectbyColor, 0 },
    { { NULL, &magenta_image, COLOR_DEFAULT, COLOR_DEFAULT, (void *) 0xff00ff, NULL, 0, 1, 0, 0, 0, 0, 0, 0, 0, '\0' }, NULL, NULL, NULL, sm_selectbyColor, 0 },
    GMENUITEM2_EMPTY
};

static void sharedmenu_select_check(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e)) {
    MENU_CHECK_VARIABLES;

    for ( mi = mi->sub; mi->ti.text!=NULL || mi->ti.line ; ++mi ) {
	switch ( mi->mid ) {
	  case SMID_SelectAll:
	  case SMID_SelectInvert:
	      mi->ti.disabled = false;
	      if( isMetricsView )
		  mi->ti.disabled = true;
	      break;
	  case SMID_SelectNone:
	      mi->ti.disabled = false;
	      break;
	  default:
	      mi->ti.disabled = !isCharView;
	      break;
	}
    }
    
}




MAKETRAMP(selectAll);
MAKETRAMP(invertSelection);
MAKETRAMP(deselectAll);


#define MAKETRAMP_MERGETYPE(FNAME) \
static void sm_##FNAME(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent* e) { \
    CommonView* cc = tryObtainGDataCommonView( gw );			\
    int merge = SelMergeType(e);					\
    if( cc->m_sharedmenu_funcs.FNAME )					\
	cc->m_sharedmenu_funcs.FNAME( cc, merge );			\
}
MAKETRAMP_MERGETYPE(selectByName);
MAKETRAMP_MERGETYPE(selectByScript);
MAKETRAMP_MERGETYPE(selectWorthOutputting);
MAKETRAMP_MERGETYPE(glyphsRefs);
MAKETRAMP_MERGETYPE(glyphsSplines);
MAKETRAMP_MERGETYPE(glyphsBoth);
MAKETRAMP_MERGETYPE(glyphsWhite);
MAKETRAMP_MERGETYPE(selectChanged);
MAKETRAMP_MERGETYPE(selectHintingNeeded);
MAKETRAMP_MERGETYPE(selectAutohintable);

MAKETRAMP(selectByPST);

MAKETRAMP(selectFirstPoint);
MAKETRAMP(selectFirstPointNextContour);
MAKETRAMP(selectNextPoint);
MAKETRAMP(selectPrevPoint);
MAKETRAMP(selectNextControlPoint);
MAKETRAMP(selectPrevControlPoint);
MAKETRAMP(selectContours);
MAKETRAMP(selectPointAt);
MAKETRAMP(selectAllPoints);
MAKETRAMP(selectOpenContours);
MAKETRAMP(selectAnchors);
MAKETRAMP(selectWidth);
MAKETRAMP(selectVWidth);
MAKETRAMP(selectHM);


#define MENUBODY_DEFAULT          NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0
#define MENUBODY_DEFAULT_DISABLED NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 1, 1, 0, 0, 0, 0, 1, 1, 0

static GMenuItem2 sharedmenu_select[] = {
    { { UN_("_All"),    MENUBODY_DEFAULT, 'A' }, H_("Select All|No Shortcut"), NULL, NULL, sm_selectAll, SMID_SelectAll },
    { { UN_("_Invert"), MENUBODY_DEFAULT, 'I' }, H_("Invert Selection|No Shortcut"), NULL, NULL, sm_invertSelection, SMID_SelectInvert },
    { { UN_("_None"),   MENUBODY_DEFAULT, 'o' }, H_("Deselect All|Escape"), NULL, NULL, sm_deselectAll, SMID_SelectNone },
    MENUITEM_LINE
    { { UN_("All _Points & Refs"), MENUBODY_DEFAULT, 'P' }, H_("Select All Points & Refs|No Shortcut"), NULL, NULL, sm_selectAllPoints, SMID_SelectAllPoints },
    { { UN_("_First Point"), MENUBODY_DEFAULT, 'F' }, H_("First Point|No Shortcut"), NULL, NULL, sm_selectFirstPoint, SMID_FirstPt },
    { { UN_("First P_oint, Next Contour"), MENUBODY_DEFAULT, 'F' }, H_("First Point, Next Contour|No Shortcut"), NULL, NULL, sm_selectFirstPointNextContour, SMID_FirstPtNextCont },
    { { UN_("_Next Point"), MENUBODY_DEFAULT, 'N' }, H_("Next Point|No Shortcut"), NULL, NULL, sm_selectNextPoint, SMID_NextPt },
    { { UN_("_Prev Point"), MENUBODY_DEFAULT, 'P' }, H_("Prev Point|No Shortcut"), NULL, NULL, sm_selectPrevPoint, SMID_PrevPt },
    { { UN_("Ne_xt Control Point"), MENUBODY_DEFAULT, 'x' }, H_("Next Control Point|No Shortcut"), NULL, NULL, sm_selectNextControlPoint, SMID_NextCP },
    { { UN_("P_rev Control Point"), MENUBODY_DEFAULT, 'r' }, H_("Prev Control Point|No Shortcut"), NULL, NULL, sm_selectPrevControlPoint, SMID_PrevCP },
    { { UN_("Points on Selected _Contours"), MENUBODY_DEFAULT, 'r' }, H_("Points on Selected Contours|No Shortcut"), NULL, NULL, sm_selectContours, SMID_Contours },
    { { UN_("Point A_t"), MENUBODY_DEFAULT, 'r' }, H_("Point At|No Shortcut"), NULL, NULL, sm_selectPointAt, SMID_SelPointAt },
    { { UN_("Points Affected by HM"), MENUBODY_DEFAULT, 'V' }, H_("Select Points Affected by HM|No Shortcut"), NULL, NULL, sm_selectHM, SMID_SelectHM },

    MENUITEM_LINE
    
    { { UN_("Open Contours"),      MENUBODY_DEFAULT, 'P' }, H_("Select Open Contours|No Shortcut"), NULL, NULL, sm_selectOpenContours, SMID_SelectOpenContours },
    { { UN_("Anc_hors"),           MENUBODY_DEFAULT, 'c' }, H_("Select Anchors|No Shortcut"), NULL, NULL, sm_selectAnchors, SMID_SelectAnchors },
    { { UN_("_Width"),                    MENUBODY_DEFAULT, '\0' }, H_("Width|No Shortcut"), NULL, NULL, sm_selectWidth, SMID_SelectWidth },
    { { UN_("_VWidth"),                   MENUBODY_DEFAULT, '\0' }, H_("VWidth|No Shortcut"), NULL, NULL, sm_selectVWidth, SMID_SelectVWidth },
    
    /* MENUITEM_LINE */
    /* MENUITEM_LINE */
    /* { { UN_("Select by _Color"), MENUBODY_DEFAULT, '\0' }, H_("Select by Color|No Shortcut"), sclist, NULL, NULL, 0 }, */
    /* { { UN_("Select by _Wildcard..."), MENUBODY_DEFAULT, '\0' }, H_("Select by Wildcard...|No Shortcut"), NULL, NULL, sm_selectByName, 0 }, */
    /* { { UN_("Select by _Script..."), MENUBODY_DEFAULT, '\0' }, H_("Select by Script...|No Shortcut"), NULL, NULL, sm_selectByScript, 0 }, */
    /* MENUITEM_LINE */
    /* { { UN_("_Glyphs Worth Outputting"), MENUBODY_DEFAULT, '\0' }, H_("Glyphs Worth Outputting|No Shortcut"), NULL,NULL, sm_selectWorthOutputting, 0 }, */
    /* { { UN_("Glyphs with only _References"), MENUBODY_DEFAULT, '\0' }, H_("Glyphs with only References|No Shortcut"), NULL,NULL, sm_glyphsRefs, 0 }, */
    /* { { UN_("Glyphs with only S_plines"), MENUBODY_DEFAULT, '\0' }, H_("Glyphs with only Splines|No Shortcut"), NULL,NULL, sm_glyphsSplines, 0 }, */
    /* { { UN_("Glyphs with both"), MENUBODY_DEFAULT, '\0' }, H_("Glyphs with both|No Shortcut"), NULL,NULL, sm_glyphsBoth, 0 }, */
    /* { { UN_("W_hitespace Glyphs"), MENUBODY_DEFAULT, '\0' }, H_("Whitespace Glyphs|No Shortcut"), NULL,NULL, sm_glyphsWhite, 0 }, */
    /* { { UN_("_Changed Glyphs"), MENUBODY_DEFAULT, '\0' }, H_("Changed Glyphs|No Shortcut"), NULL,NULL, sm_selectChanged, 0 }, */
    /* { { UN_("_Hinting Needed"), MENUBODY_DEFAULT, '\0' }, H_("Hinting Needed|No Shortcut"), NULL,NULL, sm_selectHintingNeeded, 0 }, */
    /* { { UN_("Autohinta_ble"), MENUBODY_DEFAULT, '\0' }, H_("Autohintable|No Shortcut"), NULL,NULL, sm_selectAutohintable, 0 }, */

    /* MENUITEM_LINE */
    /* { { UN_("Selec_t By Lookup Subtable..."), MENUBODY_DEFAULT, 'T' }, H_("Select By Lookup Subtable...|No Shortcut"), NULL, NULL, sm_selectByPST, 0 }, */
    /* MENUITEM_LINE */
    
    /* { { UN_("Hold [Shift] key to merge"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 1, 1, 0, 0, 0, 0, 1, 1, 0, '\0' }, NULL, NULL, NULL, 0, 0 }, */
    /* { { UN_("Hold [Control] key to restrict"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 1, 1, 0, 0, 0, 0, 1, 1, 0, '\0' }, NULL, NULL, NULL, 0, 0 }, */

    /* MENUITEM_LINE */
   
    GMENUITEM2_EMPTY
};

MAKETRAMP(undo);
MAKETRAMP(redo);
MAKETRAMP(cut);
MAKETRAMP(copy);
MAKETRAMP(paste);
MAKETRAMP(clear);
MAKETRAMP(delete);
MAKETRAMP(undoFontLevel);
MAKETRAMP(removeUndoes);


GMenuItem2 sharedmenu_edit[] = {
    { { UN_("_Undo"),   GI("editundo.png"), MENUBODYC_DEFAULT, 'U' }, H_("Undo|No Shortcut"), NULL, NULL, sm_undo, SMID_Undo },
    { { UN_("_Redo"),   GI("editredo.png"), MENUBODYC_DEFAULT, 'R' }, H_("Redo|No Shortcut"), NULL, NULL, sm_redo, SMID_Redo},
    MENUITEM_LINE
    { { UN_("Cu_t"),    GI("editcut.png"), MENUBODYC_DEFAULT, 't' }, H_("Cut|No Shortcut"), NULL, NULL, sm_cut, SMID_Cut },
    { { UN_("_Copy"),   GI("editcopy.png"), MENUBODYC_DEFAULT, 'C' }, H_("Copy|No Shortcut"), NULL, NULL, sm_copy, SMID_Copy },
    { { UN_("_Paste"),  GI("editpaste.png"), MENUBODYC_DEFAULT, 'P' }, H_("Paste|No Shortcut"), NULL, NULL, sm_paste, SMID_Paste },
    { { UN_("_Delete"), GI("editdelete.png"), MENUBODYC_DEFAULT, 'P' }, H_("Paste|No Shortcut"), NULL, NULL, sm_delete, SMID_Delete },    
    { { UN_("C_lear"),  GI("editclear.png"), MENUBODYC_DEFAULT, 'l' }, H_("Clear|No Shortcut"), NULL, NULL, sm_clear, SMID_Clear },
    MENUITEM_LINE
    { { UN_("Copy"),    GI("editcopy.png"), MENUBODYC_DEFAULT, '\0' }, H_("Copy Menu|No Shortcut"), sharedmenu_edit_copylist, NULL, NULL, 0 },
    { { UN_("Paste"),   GI("editpaste.png"), MENUBODYC_DEFAULT, '\0' }, H_("Paste Menu|No Shortcut"), sharedmenu_edit_pastelist, NULL, NULL, 0 },
    { { UN_("Clear"),   GI("editclear.png"), MENUBODYC_DEFAULT, '\0' }, H_("Clear Menu|No Shortcut"), sharedmenu_edit_clearlist, NULL, NULL, 0 },
    MENUITEM_LINE
    { { UN_("Select"),  GI("editselect.png"), MENUBODYC_DEFAULT, 'S' }, H_("Select|No Shortcut"), sharedmenu_select, sharedmenu_select_check, NULL, 0 },
    { { UN_("_Deselect All"), NULL, MENUBODYC_DEFAULT, 'o' }, H_("Deselect All|Escape"), NULL, NULL, sm_deselectAll, SMID_SelectNone },
    { { UN_("Invert Selection"), NULL, MENUBODYC_DEFAULT, 'I' }, H_("Invert Selection|No Shortcut"), NULL, NULL, sm_invertSelection, 0 },
    MENUITEM_LINE
    { { UN_("Undo Fontlevel"), GI("editundo.png"), MENUBODYC_DEFAULT, 'U' }, H_("Undo Fontlevel|No Shortcut"), NULL, NULL, sm_undoFontLevel, SMID_UndoFontLevel },
    { { UN_("Remo_ve Undoes"), GI("editrmundoes.png"), MENUBODYC_DEFAULT, 'e' }, H_("Remove Undoes|No Shortcut"), NULL, NULL, sm_removeUndoes, SMID_RemoveUndoes },
    { { UN_("Revert"), (GImage *) 0, MENUBODYC_DEFAULT, 'e' }, H_("Revert"), sharedmenu_edit_revertlist, sharedmenu_edit_revertlist_check, NULL, SMID_RevertMenu },

 { { (unichar_t *) N_("F_ind / Replace..."), GI("editfind.png"), MENUBODYC_DEFAULT, 'i' }, H_("Find / Replace...|No Shortcut"), NULL, NULL, FVMenuFindRpl, SMID_FindReplace },
    
    GMENUITEM2_EMPTY
};

void disable_non_invokable( struct gmenuitem *mi ) 
{
    for ( mi = mi->sub; mi->ti.text!=NULL || mi->ti.line ; ++mi ) {
	if( !mi->invoke ) {
	    mi->ti.disabled = 1;
	}
    }
}

void sharedmenu_edit_check(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e))
{
    MENU_CHECK_VARIABLES;

    int anypoints=0, anyrefs=0, anyimages=0, anyanchor=0;

    if( cv ) 
	CVAnySel(cv,&anypoints,&anyrefs,&anyimages,&anyanchor);
    
    int pos = FVAnyCharSelected(fv);
    int i, gid;
    int not_pasteable = pos==-1 ||
	(!CopyContainsSomething() &&
#ifndef _NO_LIBPNG
	 !GDrawSelectionHasType(fv->gw,sn_clipboard,"image/png") &&
#endif
	 !GDrawSelectionHasType(fv->gw,sn_clipboard,"image/svg+xml") &&
	 !GDrawSelectionHasType(fv->gw,sn_clipboard,"image/svg-xml") &&
	 !GDrawSelectionHasType(fv->gw,sn_clipboard,"image/svg") &&
	 !GDrawSelectionHasType(fv->gw,sn_clipboard,"image/bmp") &&
	 !GDrawSelectionHasType(fv->gw,sn_clipboard,"image/eps") &&
	 !GDrawSelectionHasType(fv->gw,sn_clipboard,"image/ps"));
    RefChar *base = CopyContainsRef(fv->b.sf);
    int base_enc = base!=NULL ? fv->b.map->backmap[base->orig_pos] : -1;

    int nothingSelected = 1;
    if( cv )
	nothingSelected = !anypoints && !anyrefs && !anyimages && !anyanchor;
    else
	nothingSelected = pos==-1;

    
//    disable_non_invokable( mi );
    
    for ( mi = mi->sub; mi->ti.text!=NULL || mi->ti.line ; ++mi ) {
	switch ( mi->mid ) {
	  case SMID_RevertMenu:
	      mi->ti.disabled = isMetricsView;
	      break;
	  case SMID_FindReplace:
	      mi->ti.disabled = !isFontView;
	      break;
	  case SMID_Paste:
	  case SMID_PasteInto:
	      mi->ti.disabled = not_pasteable;
	      break;
	  case SMID_PasteAfter:
	      mi->ti.disabled = not_pasteable || pos<0;
	      break;
	  case SMID_SameGlyphAs:
	      mi->ti.disabled = not_pasteable || base==NULL || fv->b.cidmaster!=NULL ||
		  base_enc==-1 ||
		  fv->b.selected[base_enc];	/* Can't be self-referential */
	      break;
	  case SMID_Cut:
	  case SMID_Clear:
	  case SMID_Join:
	  case SMID_Copy:
	  case SMID_Delete:
	  case SMID_CopyWidth:
	  case SMID_CopyLBearing:
	  case SMID_CopyRBearing:
	  case SMID_CopyRef:
	  case SMID_UnlinkRef:
	  case SMID_RemoveUndoes:
	  case SMID_CopyFgToBg:
	  case SMID_CopyL2L:
	      mi->ti.disabled = nothingSelected;
	      break;
	  case SMID_RplRef:
	  case SMID_CorrectRefs:
	      mi->ti.disabled = pos==-1 || fv->b.cidmaster!=NULL || fv->b.sf->multilayer;
	      break;
	  case SMID_CopyLookupData:
	      mi->ti.disabled = pos==-1 || (fv->b.sf->gpos_lookups==NULL && fv->b.sf->gsub_lookups==NULL);
	      break;
	  case SMID_CopyVWidth:
	      mi->ti.disabled = pos==-1 || !fv->b.sf->hasvmetrics;
	      break;
	  case SMID_ClearBackground:
	      mi->ti.disabled = true;
	      if ( pos!=-1 && !( onlycopydisplayed && fv->filled!=fv->show )) {
		  for ( i=0; i<fv->b.map->enccount; ++i )
		      if ( fv->b.selected[i] && (gid = fv->b.map->map[i])!=-1 &&
			   fv->b.sf->glyphs[gid]!=NULL )
			  if ( fv->b.sf->glyphs[gid]->layers[ly_back].images!=NULL ||
			       fv->b.sf->glyphs[gid]->layers[ly_back].splines!=NULL ) {
			      mi->ti.disabled = false;
			      break;
			  }
	      }
	      break;
	  case SMID_Undo:
	      if( cv )
	      {
		  mi->ti.disabled = cv->b.layerheads[cv->b.drawmode]->undoes==NULL;
	      }
	      else
	      {
		  for ( i=0; i<fv->b.map->enccount; ++i )
		      if ( fv->b.selected[i] && (gid = fv->b.map->map[i])!=-1 &&
			   fv->b.sf->glyphs[gid]!=NULL )
			  if ( fv->b.sf->glyphs[gid]->layers[fv->b.active_layer].undoes!=NULL )
			      break;
		  mi->ti.disabled = i==fv->b.map->enccount;
	      }
	      break;
	  case SMID_Redo:
	      if( cv )
	      {
		  mi->ti.disabled = cv->b.layerheads[cv->b.drawmode]->redoes==NULL;
	      }
	      else
	      {
		  for ( i=0; i<fv->b.map->enccount; ++i )
		      if ( fv->b.selected[i] && (gid = fv->b.map->map[i])!=-1 &&
			   fv->b.sf->glyphs[gid]!=NULL )
			  if ( fv->b.sf->glyphs[gid]->layers[fv->b.active_layer].redoes!=NULL )
			      break;
		  mi->ti.disabled = i==fv->b.map->enccount;
	      }
	      break;
	  case SMID_UndoFontLevel:
	      mi->ti.disabled = dlist_isempty( (struct dlistnode **)&fv->b.sf->undoes );
	      break;
	}
    }
}


/*********************************************************************************/
/*********************************************************************************/
/*********************************************************************************/
/*********************************************************************************/
/*********************************************************************************/


static void validlistcheck(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e)) {
    MENU_CHECK_VARIABLES;
    
    int anychars = FVAnyCharSelected(fv);

    for ( mi = mi->sub; mi->ti.text!=NULL || mi->ti.line ; ++mi ) {
	switch ( mi->mid ) {
	  case SMID_FindProblems:
	      if( !isFontView )
		  mi->ti.disabled = 0;
	      else
		  mi->ti.disabled = anychars==-1;
	  break;
	  case SMID_Validate:
	    mi->ti.disabled = fv->b.sf->strokedfont || fv->b.sf->multilayer;
	  break;
        }
    }
}

MAKETRAMP(dialogFindProblems);

static GMenuItem2 validlist[] = {
    { { UN_("Find Pr_oblems..."), GI("elementfindprobs.png"), MENUBODYC_DEFAULT, 'o' }, H_("Find Problems...|No Shortcut"),
                                  NULL, NULL, sm_dialogFindProblems, SMID_FindProblems },
    { { UN_("_Validate..."),      GI("elementvalidate.png"), MENUBODYC_DEFAULT, 'o' }, H_("Validate...|No Shortcut"),
                                  NULL, NULL, FVMenuValidate, SMID_Validate },
    MENUITEM_LINE
    { { UN_("Set E_xtremum Bound..."), GI("menuempty.png"), MENUBODYC_DEFAULT, 'o' }, H_("Set Extremum bound...|No Shortcut"), 
                                       NULL, NULL, FVMenuSetExtremumBound, SMID_SetExtremumBound },
    GMENUITEM2_EMPTY
};


static void enlistcheck(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e)) {
    FontView* fv = tryObtainGDataFontView( gw );
    int i, gid;
    SplineFont *sf = fv->b.sf;
    EncMap *map = fv->b.map;
    int anyglyphs = false;

    for ( i=map->enccount-1; i>=0 ; --i )
	if ( fv->b.selected[i] && (gid=map->map[i])!=-1 )
	    anyglyphs = true;

    for ( mi = mi->sub; mi->ti.text!=NULL || mi->ti.line ; ++mi ) {
	switch ( mi->mid ) {
	  case SMID_Compact:
	    mi->ti.checked = fv->b.normal!=NULL;
	  break;
	case SMID_HideNoGlyphSlots:
	    break;
	  case SMID_Reencode: case SMID_ForceReencode:
	    mi->ti.disabled = fv->b.cidmaster!=NULL;
	  break;
	  case SMID_DetachGlyphs: case SMID_DetachAndRemoveGlyphs:
	    mi->ti.disabled = !anyglyphs;
	  break;
	  case SMID_RemoveUnused:
	    gid = map->enccount>0 ? map->map[map->enccount-1] : -1;
	    mi->ti.disabled = gid!=-1 && SCWorthOutputting(sf->glyphs[gid]);
	  break;
	  case SMID_MakeFromFont:
	    mi->ti.disabled = fv->b.cidmaster!=NULL || map->enccount>1024 || map->enc!=&custom;
	  break;
	  case SMID_RemoveEncoding:
	  break;
	  case SMID_DisplayByGroups:
	    mi->ti.disabled = fv->b.cidmaster!=NULL || group_root==NULL;
	  break;
	  case SMID_NameGlyphs:
	    mi->ti.disabled = fv->b.normal!=NULL || fv->b.cidmaster!=NULL;
	  break;
	  case SMID_RenameGlyphs: case SMID_SaveNamelist:
	    mi->ti.disabled = fv->b.cidmaster!=NULL;
	  break;
	}
    }
}

static GMenuItem2 emptymenu[] = {
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 1, 0, 0, 0, '\0' }, NULL, NULL, NULL, 0, 0},
    GMENUITEM2_EMPTY
};

static GMenuItem2 enlist[] = {
    { { UN_("_Compact"),            NULL, MENUBODYC_DEFAULT_DISABLED, 'C' }, H_("Compact|No Shortcut"),
                                    NULL, NULL, FVMenuCompact, SMID_Compact },
    { { UN_("_Force Encoding"),     NULL, MENUBODYC_DEFAULT, 'C' }, H_("Force Encoding|No Shortcut"),
                                    emptymenu, FVForceEncodingMenuBuild, NULL, SMID_ForceReencode },
    MENUITEM_LINE
    { { UN_("Add E_ncoding Name..."), NULL, MENUBODYC_DEFAULT, 'C' }, H_("Add Encoding Name...|No Shortcut"),
                                      NULL, NULL, FVMenuAddEncodingName, 0 },
    { { UN_("_Load Encoding..."),     NULL, MENUBODYC_DEFAULT, 'C' }, H_("Load Encoding...|No Shortcut"),
                                      NULL, NULL, FVMenuLoadEncoding, SMID_LoadEncoding },
    { { UN_("Ma_ke From Font..."),    NULL, MENUBODYC_DEFAULT, 'C' }, H_("Make From Font...|No Shortcut"),
                                      NULL, NULL, FVMenuMakeFromFont, SMID_MakeFromFont },
    { { UN_("Remove En_coding..."),   NULL, MENUBODYC_DEFAULT, 'C' }, H_("Remove Encoding...|No Shortcut"),
                                      NULL, NULL, FVMenuRemoveEncoding, SMID_RemoveEncoding },
    GMENUITEM2_EMPTY
};



static GMenuItem2 mmlist[] = {
/* GT: Here (and following) MM means "MultiMaster" */
    { { UN_("_Create..."),      NULL, MENUBODYC_DEFAULT, 'I' }, H_("Create MM...|No Shortcut"),
                                NULL, NULL, FVMenuCreateMM, SMID_CreateMM },
    { { UN_("_Info..."),        NULL, MENUBODYC_DEFAULT, 'I' }, H_("MM Info...|No Shortcut"),
                                NULL, NULL, FVMenuMMInfo, SMID_MMInfo },
    { { UN_("_Validity Check"), NULL, MENUBODYC_DEFAULT, 'I' }, H_("MM Validity Check|No Shortcut"),
                                NULL, NULL, FVMenuMMValid, SMID_MMValid },
    { { UN_("_Blend to New Font..."),      NULL, MENUBODYC_DEFAULT, 'I' }, H_("Blend to New Font...|No Shortcut"),
                                           NULL, NULL, FVMenuBlendToNew, SMID_BlendToNew },
    { { UN_("Change Default _Weights..."), NULL, MENUBODYC_DEFAULT, 'I' }, H_("MM Change Default Weights...|No Shortcut"),
                                           NULL, NULL, FVMenuChangeMMBlend, SMID_ChangeMMBlend },
    GMENUITEM2_EMPTY,
};

static void mmlistcheck(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e)) {
    FontView* fv = tryObtainGDataFontView( gw );
    int i, base, j;
    MMSet *mm = fv->b.sf->mm;
    SplineFont *sub;
    GMenuItem2 *mml;

    for ( i=0; mmlist[i].mid!=SMID_ChangeMMBlend; ++i );
    base = i+2;
    if ( mm==NULL )
	mml = mmlist;
    else {
	mml = calloc(base+mm->instance_count+2,sizeof(GMenuItem2));
	memcpy(mml,mmlist,sizeof(mmlist));
	mml[base-1].ti.fg = mml[base-1].ti.bg = COLOR_DEFAULT;
	mml[base-1].ti.line = true;
	for ( j = 0, i=base; j<mm->instance_count+1; ++i, ++j ) {
	    if ( j==0 )
		sub = mm->normal;
	    else
		sub = mm->instances[j-1];
	    mml[i].ti.text = uc_copy(sub->fontname);
	    mml[i].ti.checkable = true;
	    mml[i].ti.checked = sub==fv->b.sf;
	    mml[i].ti.userdata = sub;
	    mml[i].invoke = FVMenuShowSubFont;
	    mml[i].ti.fg = mml[i].ti.bg = COLOR_DEFAULT;
	}
    }
    GMenuItemArrayFree(mi->sub);
    mi->sub = GMenuItem2ArrayCopy(mml,NULL);
    if ( mml!=mmlist ) {
	for ( i=base; mml[i].ti.text!=NULL; ++i )
	    free( mml[i].ti.text);
	free(mml);
    }

    for ( mi = mi->sub; mi->ti.text!=NULL || mi->ti.line ; ++mi ) {
	switch ( mi->mid ) {
	  case SMID_CreateMM:
	    mi->ti.disabled = false;
	  break;
	  case SMID_MMInfo: case SMID_MMValid: case SMID_BlendToNew:
	    mi->ti.disabled = mm==NULL;
	  break;
	  case SMID_ChangeMMBlend:
	    mi->ti.disabled = mm==NULL || mm->apple;
	  break;
	}
    }
}


static GMenuItem2 cdlist[] = {
    { { UN_("_Convert to CID"),  NULL, MENUBODYC_DEFAULT, 'C' }, H_("Convert to CID|No Shortcut"),
                                 NULL, NULL, FVMenuConvert2CID, SMID_Convert2CID },
    { { UN_("Convert By C_Map"), NULL, MENUBODYC_DEFAULT, 'C' }, H_("Convert By CMap|No Shortcut"),
                                 NULL, NULL, FVMenuConvertByCMap, SMID_ConvertByCMap },
    MENUITEM_LINE
    { { UN_("_Flatten"),         NULL, MENUBODYC_DEFAULT, 'F' }, H_("Flatten|No Shortcut"),
                                 NULL, NULL, FVMenuFlatten, SMID_Flatten },
    { { UN_("Fl_attenByCMap"),   NULL, MENUBODYC_DEFAULT, 'F' }, H_("Flatten by CMap|No Shortcut"),
                                 NULL, NULL, FVMenuFlattenByCMap, SMID_FlattenByCMap },
    MENUITEM_LINE
    { { UN_("Insert F_ont..."),  NULL, MENUBODYC_DEFAULT, 'o' }, H_("Insert Font...|No Shortcut"),
                                 NULL, NULL, FVMenuInsertFont, SMID_InsertFont },
    { { UN_("Insert _Blank"),    NULL, MENUBODYC_DEFAULT, 'B' }, H_("Insert Blank|No Shortcut"),
                                 NULL, NULL, FVMenuInsertBlank, SMID_InsertBlank },
    { { UN_("_Remove Font"),     NULL, MENUBODYC_DEFAULT, 'R' }, H_("Remove Font|No Shortcut"),
                                 NULL, NULL, FVMenuRemoveFontFromCID, SMID_RemoveFromCID },
    MENUITEM_LINE
    { { UN_("_Change Supplement..."), NULL, MENUBODYC_DEFAULT, 'I' }, H_("Change Supplement...|No Shortcut"),
                                      NULL, NULL, FVMenuChangeSupplement, SMID_ChangeSupplement },
    { { UN_("F_ont Info..."),         NULL, MENUBODYC_DEFAULT, 'I' }, H_("CID Font Info...|No Shortcut"),
                                      NULL, NULL, FVMenuCIDFontInfo, SMID_CIDFontInfo },
    GMENUITEM2_EMPTY, GMENUITEM2_EMPTY
};

static void cdlistcheck(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e)) {
    FontView* fv = tryObtainGDataFontView( gw );
    int i, base, j;
    SplineFont *sub, *cidmaster = fv->b.cidmaster;

    for ( i=0; cdlist[i].mid!=SMID_CIDFontInfo; ++i );
    base = i+2;
    for ( i=base; cdlist[i].ti.text!=NULL; ++i ) {
	free( cdlist[i].ti.text);
	cdlist[i].ti.text = NULL;
    }

    cdlist[base-1].ti.fg = cdlist[base-1].ti.bg = COLOR_DEFAULT;
    if ( cidmaster==NULL ) {
	cdlist[base-1].ti.line = false;
    } else {
	cdlist[base-1].ti.line = true;
	for ( j = 0, i=base;
		i<sizeof(cdlist)/sizeof(cdlist[0])-1 && j<cidmaster->subfontcnt;
		++i, ++j ) {
	    sub = cidmaster->subfonts[j];
	    cdlist[i].ti.text = uc_copy(sub->fontname);
	    cdlist[i].ti.checkable = true;
	    cdlist[i].ti.checked = sub==fv->b.sf;
	    cdlist[i].ti.userdata = sub;
	    cdlist[i].invoke = FVMenuShowSubFont;
	    cdlist[i].ti.fg = cdlist[i].ti.bg = COLOR_DEFAULT;
	}
    }
    GMenuItemArrayFree(mi->sub);
    mi->sub = GMenuItem2ArrayCopy(cdlist,NULL);

    for ( mi = mi->sub; mi->ti.text!=NULL || mi->ti.line ; ++mi ) {
	switch ( mi->mid ) {
	  case SMID_Convert2CID: case SMID_ConvertByCMap:
	    mi->ti.disabled = cidmaster!=NULL || fv->b.sf->mm!=NULL;
	  break;
	  case SMID_InsertFont: case SMID_InsertBlank:
	    /* OpenType allows at most 255 subfonts (PS allows more, but why go to the effort to make safe font check that? */
	    mi->ti.disabled = cidmaster==NULL || cidmaster->subfontcnt>=255;
	  break;
	  case SMID_RemoveFromCID:
	    mi->ti.disabled = cidmaster==NULL || cidmaster->subfontcnt<=1;
	  break;
	  case SMID_Flatten: case SMID_FlattenByCMap: case SMID_CIDFontInfo:
	  case SMID_ChangeSupplement:
	    mi->ti.disabled = cidmaster==NULL;
	  break;
	}
    }
}

MAKETRAMP(dialogCompareLayers);



GMenuItem2 sharedmenu_font[] = {
    
    { { UN_("_Font Info..."),         GI("elementfontinfo.png"), MENUBODYC_DEFAULT, 'F' },
                                      H_("Font Info...|No Shortcut"), NULL, NULL, FVMenuFontInfo, SMID_FontInfo },
    { { UN_("_Validation"),           GI("elementvalidate.png"), MENUBODYC_DEFAULT, 'I' },
                                      H_("Validation|No Shortcut"), validlist, validlistcheck, NULL, 0 },

    MENUITEM_LINE
    
    { { UN_("Interpo_late Fonts..."), GI("elementinterpolatefonts.png"), MENUBODYC_DEFAULT, 'p' },
                                      H_("Interpolate Fonts|No Shortcut"), NULL, NULL, FVMenuInterpFonts, SMID_InterpolateFonts },
    { { UN_("Compare Fonts..."),      GI("elementcomparefonts.png"), MENUBODYC_DEFAULT, 'p' },
                                      H_("Compare Fonts|No Shortcut"), NULL, NULL, FVMenuCompareFonts, SMID_FontCompare },
    { { UN_("Compare Layers..."),     GI("elementcomparelayers.png"), MENUBODYC_DEFAULT, 'p' },
                                      H_("Compare Layers|No Shortcut"), NULL, NULL, sm_dialogCompareLayers, 0 },

    MENUITEM_LINE
    

    { { UN_("E_ncoding"), NULL,  MENUBODYC_DEFAULT, 'V' }, H_("Encoding|No Shortcut"), enlist, enlistcheck, NULL, 0 },
    { { UN_("_Re Encode"), NULL, MENUBODYC_DEFAULT, 'E' }, H_("Reencode|No Shortcut"), emptymenu, FVEncodingMenuBuild, NULL, SMID_Reencode },
    
    MENUITEM_LINE

    { { UN_("Bitm_ap Strikes Available"), GI("elementbitmapsavail.png"), MENUBODYC_DEFAULT, 'A' },
                                          H_("Bitmap Strikes Available...|No Shortcut"), NULL, NULL, FVMenuBitmaps, SMID_AvailBitmaps },
    { { UN_("Regenerate _Bitmap Glyphs"), GI("elementregenbitmaps.png"), MENUBODYC_DEFAULT, 'B' },
                                          H_("Regenerate Bitmap Glyphs...|No Shortcut"), NULL, NULL, FVMenuBitmaps, SMID_RegenBitmaps },
    { { UN_("Remove Bitmap Glyphs"),      GI("elementremovebitmaps.png"), MENUBODYC_DEFAULT, '\0' },
                                          H_("Remove Bitmap Glyphs...|No Shortcut"), NULL, NULL, FVMenuBitmaps, SMID_RemoveBitmaps },

    MENUITEM_LINE
    
    { { UN_("M_ultiple Master (MM)"),     GI("mm.png"),  MENUBODYC_DEFAULT, 'I' },
                                          H_("Multiple Master (MM)|No Shortcut"), mmlist, mmlistcheck, NULL, 0 },
    { { UN_("C_omposite (CID)"),          GI("cid.png"), MENUBODYC_DEFAULT, '\0' },
                                          H_("Composite (CID)|No Shortcut"), cdlist, cdlistcheck, NULL, 0 },

    MENUITEM_LINE
    
    { { UN_("_ATT info..."),              NULL, MENUBODYC_DEFAULT, 'S' },
                                          H_("ATT info|No Shortcut"), NULL, NULL, FVMenuShowAtt, 0 },
    { { UN_("_MATH Info..."),             GI("elementmathinfo.png"), MENUBODYC_DEFAULT, '\0' },
                                          H_("MATH Info...|No Shortcut"), NULL, NULL, FVMenuMATHInfo, 0 },
    { { UN_("_BDF Info..."),              GI("elementbdfinfo.png"), MENUBODYC_DEFAULT, '\0' },
                                          H_("BDF Info...|No Shortcut"), NULL, NULL, FVMenuBDFInfo, SMID_StrikeInfo },
    
   GMENUITEM2_EMPTY
};


void sharedmenu_font_check(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e))
{
    printf("sharedmenu_font_check mi:%p\n",mi);
    if( !mi )
	return;
    printf("sharedmenu_font_check mi->sub:%p\n",mi->sub);
    
    FontView* fv = tryObtainGDataFontView( gw );
    int anychars = FVAnyCharSelected(fv);
    int gid;
    int anybuildable, anytraceable;
    int in_modal = (fv->b.container!=NULL && fv->b.container->funcs->is_modal);

    for ( mi = mi->sub; mi && (mi->ti.text!=NULL || mi->ti.line); ++mi ) {
	switch ( mi->mid ) {
	  case SMID_FontInfo:
	    mi->ti.disabled = in_modal;
	  break;
	  case SMID_CharInfo:
	    mi->ti.disabled = anychars<0 || (gid = fv->b.map->map[anychars])==-1 ||
		    (fv->b.cidmaster!=NULL && fv->b.sf->glyphs[gid]==NULL) ||
		    in_modal;
	  break;
	  case SMID_Transform:
	    mi->ti.disabled = anychars==-1;
	    /* some Transformations make sense on bitmaps now */
	  break;
	  case SMID_AddExtrema:
	    mi->ti.disabled = anychars==-1 || fv->b.sf->onlybitmaps;
	  break;
	  case SMID_Simplify:
	  case SMID_Stroke: case SMID_RmOverlap:
	    mi->ti.disabled = anychars==-1 || fv->b.sf->onlybitmaps;
	  break;
	  case SMID_Styles:
	    mi->ti.disabled = anychars==-1 || fv->b.sf->onlybitmaps;
	  break;
	  case SMID_Round: case SMID_Correct:
	    mi->ti.disabled = anychars==-1 || fv->b.sf->onlybitmaps;
	  break;
#ifdef FONTFORGE_CONFIG_TILEPATH
	  case SMID_TilePath:
	    mi->ti.disabled = anychars==-1 || fv->b.sf->onlybitmaps;
	  break;
#endif
	  case SMID_AvailBitmaps:
	    mi->ti.disabled = fv->b.sf->mm!=NULL;
	  break;
	  case SMID_RegenBitmaps: case SMID_RemoveBitmaps:
	    mi->ti.disabled = fv->b.sf->bitmaps==NULL || fv->b.sf->onlybitmaps ||
		    fv->b.sf->mm!=NULL;
	  break;
	  case SMID_BuildAccent:
	    anybuildable = false;
	    if ( anychars!=-1 ) {
		int i;
		for ( i=0; i<fv->b.map->enccount; ++i ) if ( fv->b.selected[i] ) {
		    SplineChar *sc=NULL, dummy;
		    gid = fv->b.map->map[i];
		    if ( gid!=-1 )
			sc = fv->b.sf->glyphs[gid];
		    if ( sc==NULL )
			sc = SCBuildDummy(&dummy,fv->b.sf,fv->b.map,i);
		    if ( SFIsSomethingBuildable(fv->b.sf,sc,fv->b.active_layer,false) ||
			    SFIsDuplicatable(fv->b.sf,sc)) {
			anybuildable = true;
		break;
		    }
		}
	    }
	    mi->ti.disabled = !anybuildable;
	  break;
	  case SMID_Autotrace:
	    anytraceable = false;
	    if ( FindAutoTraceName()!=NULL && anychars!=-1 ) {
		int i;
		for ( i=0; i<fv->b.map->enccount; ++i )
		    if ( fv->b.selected[i] && (gid = fv->b.map->map[i])!=-1 &&
			    fv->b.sf->glyphs[gid]!=NULL &&
			    fv->b.sf->glyphs[gid]->layers[ly_back].images!=NULL ) {
			anytraceable = true;
		break;
		    }
	    }
	    mi->ti.disabled = !anytraceable;
	  break;
	  case SMID_MergeFonts:
	    mi->ti.disabled = fv->b.sf->bitmaps!=NULL && fv->b.sf->onlybitmaps;
	  break;
	  case SMID_FontCompare:
	    mi->ti.disabled = !fv_list || fv_list->b.next==NULL;
	  break;
	  case SMID_InterpolateFonts:
	    mi->ti.disabled = fv->b.sf->onlybitmaps;
	  break;
	  case SMID_StrikeInfo:
	    mi->ti.disabled = fv->b.sf->bitmaps==NULL;
	  break;
	}
    }
}

/*********************************************************************************/
/*********************************************************************************/
/*********************************************************************************/
/*********************************************************************************/
/*********************************************************************************/

void MenuLicense(GWindow UNUSED(base), struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    help("http://fontforge.github.io/en-US/about/project/license/");
}

void MenuIndex(GWindow UNUSED(base), struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    help("IndexFS.html");
}

static void FVMenuContextualHelp(GWindow UNUSED(base), struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    help("fontview.html");
}

void MenuHelp(GWindow UNUSED(base), struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    help("overview.html");
}

void MenuAbout(GWindow UNUSED(base), struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    help("http://fontforge.github.io/");
}

void MenuVersion(GWindow UNUSED(base), struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    ShowAboutScreen();
}

void MenuFollow(GWindow UNUSED(base), struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    help("https://twitter.com/fontforge");
}

void MenuDWFF(GWindow UNUSED(base), struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    help("http://designwithfontforge.com/");
}

void MenuGH(GWindow UNUSED(base), struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    help("https://github.com/fontforge/fontforge/");
}


GMenuItem2 sharedmenu_help[] = {
    { { UN_("_About"), GI("helpabout.png"), MENUBODYC_DEFAULT, 'A' }, H_("About|No Shortcut"), NULL, NULL, MenuAbout, 0 },
    { { UN_("_License"), GI("menuempty.png"), MENUBODYC_DEFAULT, 'A' }, H_("License|No Shortcut"), NULL, NULL, MenuLicense, 0 },
    { { UN_("_Version"), GI("menuempty.png"), MENUBODYC_DEFAULT, 'I' }, H_("Version|No Shortcut"), NULL, NULL, MenuVersion, 0 },
    MENUITEM_LINE
    { { UN_("@Follow for Updates"), GI("helpindex.png"), MENUBODYC_DEFAULT, 'I' }, H_("Follow for Updates|No Shortcut"), NULL, NULL, MenuFollow, 0 },
    MENUITEM_LINE
    { { UN_("DesignWithFontForge.com"), GI("helphelp.png"), MENUBODYC_DEFAULT, 'H' }, H_("Help and Tutorials|F1"), NULL, NULL, MenuDWFF, 0 },
    MENUITEM_LINE
    { { UN_("GitHub.com/FontForge"), GI("menuempty.png"), MENUBODYC_DEFAULT, 'I' }, H_("Bugs, Questions and Requests|Shft+F1"), NULL, NULL, MenuGH, 0 },
    GMENUITEM2_EMPTY
};

void sharedmenu_help_check(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e))
{
    MENU_CHECK_VARIABLES;
    
}

/*********************************************************************************/
/*********************************************************************************/
/*********************************************************************************/
/*********************************************************************************/
/*********************************************************************************/

static void sm_SetWidth(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e)) {
    CommonView* cc = tryObtainGDataCommonView( gw );
    FontView* fv = tryObtainGDataFontView( gw );

    enum widthtype wtype =
	mi->mid==SMID_SetWidth    ? wt_width    :
	mi->mid==SMID_SetLBearing ? wt_lbearing :
	mi->mid==SMID_SetRBearing ? wt_rbearing :
	mi->mid==SMID_SetBearings ? wt_bearings :
	wt_vwidth;
    
    if ( mi->mid == SMID_SetVWidth && !fv->b.sf->hasvmetrics )
	return;
    
    if( cc->m_sharedmenu_funcs.setWidth )
	cc->m_sharedmenu_funcs.setWidth( cc, wtype );
}

static void sm_center(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e)) {
    CommonView* cc = tryObtainGDataCommonView( gw );
    int docenter = (mi->mid==SMID_Center);
    if( cc->m_sharedmenu_funcs.metricsCenter )
	cc->m_sharedmenu_funcs.metricsCenter( cc, docenter );
}

static void sm_KernByClasses(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    CommonView* cc = tryObtainGDataCommonView( gw );
    FontView* fv = tryObtainGDataFontView( gw );
    int layer = cc->m_sharedmenu_funcs.getActiveLayer(cc);
    ShowKernClasses(fv->b.sf,NULL,layer,false);
}

MAKETRAMP(kernPairCloseUp);
MAKETRAMP(removeKern);


static void sm_VKernByClasses(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    CommonView* cc = tryObtainGDataCommonView( gw );
    FontView* fv = tryObtainGDataFontView( gw );
    int layer = cc->m_sharedmenu_funcs.getActiveLayer(cc);
    ShowKernClasses(fv->b.sf,NULL,layer,true);
}

static void sm_VKernFromHKern(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    FontView* fv = tryObtainGDataFontView( gw );
    FVVKernFromHKern(&fv->b);
    
}

MAKETRAMP(removeVKern);

GMenuItem2 sharedmenu_metrics[] = {
    { { UN_("Set _Width..."), GI("metricssetwidth.png"), MENUBODYC_DEFAULT, 'W' }, H_("Set Width...|No Shortcut"), NULL, NULL, sm_SetWidth, SMID_SetWidth },
    { { UN_("Set _LBearing"), GI("metricssetlbearing.png"), MENUBODYC_DEFAULT, 'L' }, H_("Set LBearing...|No Shortcut"), NULL, NULL, sm_SetWidth, SMID_SetLBearing },
    { { UN_("Set _RBearing"), GI("metricssetrbearing.png"), MENUBODYC_DEFAULT, 'R' }, H_("Set RBearing...|No Shortcut"), NULL, NULL, sm_SetWidth, SMID_SetRBearing },
    { { UN_("Set Both Bearings"), GI("menuempty.png"), MENUBODYC_DEFAULT, 'R' }, H_("Set Both Bearings...|No Shortcut"), NULL, NULL, sm_SetWidth, SMID_SetBearings },
    
    MENUITEM_LINE
    
    { { UN_("_Center in Width"), GI("metricscenter.png"), MENUBODYC_DEFAULT, 'C' }, H_("Center in Width|No Shortcut"), NULL, NULL, sm_center, SMID_Center },
    { { UN_("_Thirds in Width"), GI("menuempty.png"), MENUBODYC_DEFAULT, 'T' }, H_("Thirds in Width|No Shortcut"), NULL, NULL, sm_center, SMID_Thirds },
    { { UN_("_Auto Width..."), NULL, MENUBODYC_DEFAULT, 'A' }, H_("Auto Width...|No Shortcut"), NULL, NULL, FVMenuAutoWidth, 0 },
    
    MENUITEM_LINE
    
    { { UN_("Ker_n By Classes"), NULL, MENUBODYC_DEFAULT, 'K' }, H_("Kern By Classes...|No Shortcut"), NULL, NULL, sm_KernByClasses, 0 },
    { { UN_("Kern Pair Closeup"), NULL, MENUBODYC_DEFAULT, 'P' }, H_("Kern Pair Closeup...|No Shortcut"), NULL, NULL, sm_kernPairCloseUp, 0 },
    { { UN_("Remove All Kern _Pairs"), NULL, MENUBODYC_DEFAULT, 'P' }, H_("Remove All Kern Pairs|No Shortcut"), NULL, NULL, sm_removeKern, SMID_RmHKern },
    
    MENUITEM_LINE
    
    { { UN_("VKern By Classes"), NULL, MENUBODYC_DEFAULT, 'K' }, H_("VKern By Classes...|No Shortcut"), NULL, NULL, sm_VKernByClasses, SMID_VKernByClass },
    { { UN_("VKern From HKern"), NULL, MENUBODYC_DEFAULT, 'P' }, H_("VKern From HKern|No Shortcut"), NULL, NULL, sm_VKernFromHKern, SMID_VKernFromH },
    { { UN_("Remove All VKern Pairs"), NULL, MENUBODYC_DEFAULT, 'P' }, H_("Remove All VKern Pairs|No Shortcut"), NULL, NULL, sm_removeVKern, SMID_RmVKern },

    MENUITEM_LINE
    
    { { UN_("Set _Vertical Advance..."), GI("metricssetvwidth.png"), COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 1, 1, 0, 0, 0, 0, 1, 1, 0, 'V' }, H_("Set Vertical Advance...|No Shortcut"), NULL, NULL, sm_SetWidth, SMID_SetVWidth },

    GMENUITEM2_EMPTY
};


void sharedmenu_metrics_check(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e)) {
    MENU_CHECK_VARIABLES;
    SplineChar* sc = cc->m_sharedmenu_funcs.getActiveSplineChar(cc);
    SplineFont* sf = tryObtainCastSplineFont( cc );

    for ( mi = mi->sub; mi->ti.text!=NULL || mi->ti.line ; ++mi ) {
	switch ( mi->mid ) {
	  case SMID_Center: case SMID_Thirds: case SMID_SetWidth:
	  case SMID_SetLBearing: case SMID_SetRBearing: case SMID_SetBearings:
	    mi->ti.disabled = !sc;
	  break;
	  case SMID_SetVWidth:
	    mi->ti.disabled = !sc || !sf->hasvmetrics;
	  break;
	  case SMID_VKernByClass:
	  case SMID_VKernFromH:
	  case SMID_RmVKern:
	    mi->ti.disabled = !sf->hasvmetrics;
	  break;
	}
    }
}



/*********************************************************************************/
/*********************************************************************************/
/*********************************************************************************/
/*********************************************************************************/
/*********************************************************************************/

static GMenuItem2 gllist[] = {
    { { UN_("_Glyph Image"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'K' }, H_("Glyph Image|No Shortcut"), NULL, NULL, FVMenuGlyphLabel, gl_glyph },
    { { UN_("_Name"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'K' }, H_("Name|No Shortcut"), NULL, NULL, FVMenuGlyphLabel, gl_name },
    { { UN_("_Unicode"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'L' }, H_("Unicode|No Shortcut"), NULL, NULL, FVMenuGlyphLabel, gl_unicode },
    { { UN_("_Encoding Hex"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'L' }, H_("Encoding Hex|No Shortcut"), NULL, NULL, FVMenuGlyphLabel, gl_encoding },
    GMENUITEM2_EMPTY
};

static void gllistcheck(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e)) {
    MENU_CHECK_VARIABLES;

    for ( mi = mi->sub; mi->ti.text!=NULL || mi->ti.line ; ++mi ) {
	mi->ti.checked = fv->glyphlabel == mi->mid;
    }
}

static void sm_numberPoints(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e)) {
    CommonView* cc = tryObtainGDataCommonView( gw );
    int mid = mi->mid;
    if( cc->m_sharedmenu_funcs.numberPoints )
	cc->m_sharedmenu_funcs.numberPoints( cc, mid );
}

static GMenuItem2 nplist[] = {
    { { UN_("PointNumbers|_None"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'K' }, H_("None|No Shortcut"), NULL, NULL, sm_numberPoints, SMID_PtsNone },
    { { UN_("_TrueType"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'A' }, H_("TrueType|No Shortcut"), NULL, NULL, sm_numberPoints, SMID_PtsTrue },
    { { (unichar_t *) NU_("_PostScript®"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'L' }, H_("PostScript|No Shortcut"), NULL, NULL, sm_numberPoints, SMID_PtsPost },
    { { UN_("_SVG"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'L' }, H_("SVG|No Shortcut"), NULL, NULL, sm_numberPoints, SMID_PtsSVG },
    { { UN_("P_ositions"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'L' }, H_("Positions|No Shortcut"), NULL, NULL, sm_numberPoints, SMID_PtsPos },
    GMENUITEM2_EMPTY
};


static void nplistcheck(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e)) {
    CharView* cv   = tryObtainGDataCharView( gw );
//    if( !cv )
    {
	for ( mi = mi->sub; mi->ti.text!=NULL || mi->ti.line ; ++mi ) {
	    mi->ti.disabled = 0;
	    mi->ti.checked = 0;
	}
	return;
    }
    SplineChar* sc = cv->b.sc;
    
    int order2 = cv->b.layerheads[cv->b.drawmode]->order2;
    int is_grid_layer = cv->b.drawmode == dm_grid;

    for ( mi = mi->sub; mi->ti.text!=NULL || mi->ti.line ; ++mi ) {
	switch ( mi->mid ) {
	  case SMID_PtsNone:
	    mi->ti.disabled = !order2 || is_grid_layer;
	    mi->ti.checked = (cv->showpointnumbers == 0);
	  break;
	  case SMID_PtsTrue:
	    mi->ti.disabled = !order2 || is_grid_layer;
	    mi->ti.checked = cv->showpointnumbers && order2;
	  break;
	  case SMID_PtsPost:
	    mi->ti.disabled = order2 || is_grid_layer;
	    mi->ti.checked = cv->showpointnumbers && !order2 && sc->numberpointsbackards;
	  break;
	  case SMID_PtsSVG:
	    mi->ti.disabled = order2 || is_grid_layer;
	    mi->ti.checked = cv->showpointnumbers && !order2 && !sc->numberpointsbackards;
	  break;
          case SMID_PtsPos:
	    mi->ti.disabled = is_grid_layer;
            mi->ti.checked = (cv->showpointnumbers == 2);
	}
    }
}

static GMenuItem2 gflist[] = {
    { { UN_("Show _Grid Fit..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 1, 1, 1, 0, 0, 0, 1, 1, 0, 'l' },
                                  H_("Show Grid Fit...|No Shortcut"), NULL, NULL, CVMenuShowGridFit, SMID_ShowGridFit },
    { { UN_("Show _Grid Fit (Live Update)..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 1, 1, 1, 0, 0, 0, 1, 1, 0, 'l' },
                                                H_("Show Grid Fit (Live Update)...|No Shortcut"),
                                                NULL, NULL, CVMenuShowGridFitLiveUpdate, SMID_ShowGridFitLiveUpdate },
    { { UN_("_Bigger Point Size"),              NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 1, 1, 0, 0, 0, 0, 1, 1, 0, 'B' },
                                                H_("Bigger Point Size|No Shortcut"),
                                                NULL, NULL, CVMenuChangePointSize, SMID_Bigger },
    { { UN_("_Smaller Point Size"),       NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 1, 1, 0, 0, 0, 0, 1, 1, 0, 'S' },
                                          H_("Smaller Point Size|No Shortcut"), NULL, NULL, CVMenuChangePointSize, SMID_Smaller },
    { { UN_("_Anti Alias"),               NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 1, 1, 1, 0, 0, 0, 1, 1, 0, 'L' },
                                          H_("Grid Fit Anti Alias|No Shortcut"), NULL, NULL, CVMenuChangePointSize, SMID_GridFitAA },
    { { UN_("_Off"),                      NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 1, 1, 0, 0, 0, 0, 1, 1, 0, 'S' },
                                          H_("Grid Fit Off|No Shortcut"), NULL, NULL, CVMenuChangePointSize, SMID_GridFitOff },
    GMENUITEM2_EMPTY
};

static void gflistcheck(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e)) {
    CharView* cv   = tryObtainGDataCharView( gw );
    SplineFont* sc = tryObtainCastSplineFont( cv );
    if( !cv )
    {
	setAllDisabled(mi,1);
	return;
    }

    for ( mi = mi->sub; mi->ti.text!=NULL || mi->ti.line ; ++mi ) {
	switch ( mi->mid ) {
	  case SMID_ShowGridFit:
	    mi->ti.disabled = !hasFreeType() || cv->dv!=NULL;
	    mi->ti.checked = cv->show_ft_results;
	  break;
	  case SMID_ShowGridFitLiveUpdate:
	    mi->ti.disabled = !hasFreeType() || cv->dv!=NULL;
	    mi->ti.checked = cv->show_ft_results_live_update;
	  break;
	  case SMID_Bigger:
	    mi->ti.disabled = !cv->show_ft_results;
	  break;
	  case SMID_Smaller:
	    mi->ti.disabled = !cv->show_ft_results || cv->ft_pointsizex<2 || cv->ft_pointsizey<2;
	  break;
	  case SMID_GridFitAA:
	    mi->ti.disabled = !cv->show_ft_results;
	    mi->ti.checked = cv->ft_depth==8;
	  break;
	  case SMID_GridFitOff:
	    mi->ti.disabled = !cv->show_ft_results;
	  break;
	}
    }
}

static GMenuItem2 vwelementlist[] = {
    { { UN_("_Point"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'o' }, H_("Point|No Shortcut"), NULL, NULL, CVMenuShowHide, SMID_HidePoints },
    { { UN_("Control Point"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, ')' }, H_("Control Point|No Shortcut"), NULL, NULL, CVMenuShowHideControlPoints, SMID_HideControlPoints },
    { { UN_("_Control Point Info"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'M' }, H_("Control Point Info|No Shortcut"), NULL, NULL, CVMenuShowCPInfo, SMID_ShowCPInfo },
    { { UN_("_Anchor"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'R' }, H_("Anchor|No Shortcut"), NULL, NULL, CVMenuShowHints, SMID_ShowAnchors },
    { { UN_("_Extrema"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'M' }, H_("Extrema|No Shortcut"), NULL, NULL, CVMenuMarkExtrema, SMID_MarkExtrema },
    { { UN_("Points of _Inflection"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'M' }, H_("Points of Inflection|No Shortcut"), NULL, NULL, CVMenuMarkPointsOfInflection, SMID_MarkPointsOfInflection },
    { { UN_("Dragging Comparison Outline"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'l' }, H_("Dragging Comparison Outline|No Shortcut"), NULL, NULL, CVMenuDraggingComparisonOutline, SMID_DraggingComparisonOutline },
    MENUITEM_LINE
    { { UN_("_Fill"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'l' }, H_("Fill|No Shortcut"), NULL, NULL, CVMenuFill, SMID_Fill },
    { { UN_("Previe_w"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'l' }, H_("Preview|No Shortcut"), NULL, NULL, CVMenuPreview, SMID_Preview },
    MENUITEM_LINE
    { { UN_("_Side Bearing"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'M' }, H_("Side Bearing|No Shortcut"), NULL, NULL, CVMenuShowSideBearings, SMID_ShowSideBearings },
    { { UN_("Reference Name"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'M' }, H_("Reference Name|No Shortcut"), NULL, NULL, CVMenuShowRefNames, SMID_ShowRefNames },
    { { UN_("Hori_zontal Metric Line"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'R' }, H_("Hori_zontal Metric Line|No Shortcut"), NULL, NULL, CVMenuShowHints, SMID_ShowHMetrics },
    { { UN_("Vertical _Metric Line"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'R' }, H_("Vertical Metric Line|No Shortcut"), NULL, NULL, CVMenuShowHints, SMID_ShowVMetrics },
    MENUITEM_LINE
    { { UN_("Almost Horizontal/Vertical Line"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'M' }, H_("Almost Horizontal/Vertical Line|No Shortcut"), NULL, NULL, CVMenuShowAlmostHV, SMID_ShowAlmostHV },
    { { UN_("Almost Horizontal/Vertical Curve"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'M' }, H_("Almost Horizontal/Vertical Curve|No Shortcut"), NULL, NULL, CVMenuShowAlmostHVCurves, SMID_ShowAlmostHVCurves },
    { { UN_("Define \"Almost\"..."), NULL, MENUBODYC_DEFAULT, 'M' }, H_("(Define \"Almost\")|No Shortcut"), NULL, NULL, CVMenuDefineAlmost, SMID_DefineAlmost },
    MENUITEM_LINE
    { { UN_("_Horizontal Hint"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'R' }, H_("Horizontal Hint|No Shortcut"), NULL, NULL, CVMenuShowHints, SMID_ShowHHints },
    { { UN_("_Vertical Hint"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'R' }, H_("Vertical Hint|No Shortcut"), NULL, NULL, CVMenuShowHints, SMID_ShowVHints },
    { { UN_("_Diagonal Hint"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'R' }, H_("Diagonal Hint|No Shortcut"), NULL, NULL, CVMenuShowHints, SMID_ShowDHints },
/* GT: You might not want to translate this, it's a keyword in PostScript font files */
    { { UN_("_BlueValue"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'R' }, H_("BlueValue|No Shortcut"), NULL, NULL, CVMenuShowHints, SMID_ShowBlueValues },
/* GT: You might not want to translate this, it's a keyword in PostScript font files */
    { { UN_("FamilyBl_ue"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'R' }, H_("Family Blue|No Shortcut"), NULL, NULL, CVMenuShowHints, SMID_ShowFamilyBlues },
    GMENUITEM2_EMPTY
};

MAKETRAMP(toggleShowTabs);
MAKETRAMP(toggleShowRulers);
MAKETRAMP(toggleShowPaletteTools);
MAKETRAMP(toggleShowPaletteLayers);
MAKETRAMP(toggleShowPaletteDocked);

static GMenuItem2 vwglyphlist[] = {
    { { UN_("_Glyph Tabs"),     NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'R' },
                                H_("Glyph Tabs|No Shortcut"), NULL, NULL, sm_toggleShowTabs, SMID_ShowTabs },
    { { UN_("_Ruler"),          NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'R' },
                                H_("Ruler|No Shortcut"), NULL, NULL, sm_toggleShowRulers, SMID_HideRulers },
    { { UN_("_Tool Palette"),   NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'T' },
                                H_("Tools|No Shortcut"), NULL, NULL, sm_toggleShowPaletteTools, SMID_Tools },
    { { UN_("_Layer Palette"),  NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'L' },
                                H_("Layers|No Shortcut"), NULL, NULL, sm_toggleShowPaletteLayers, SMID_Layers },
    { { UN_("_Docked Palette"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'D' },
                                H_("Docked Palette|No Shortcut"), NULL, NULL, sm_toggleShowPaletteDocked, SMID_DockPalettes },
    GMENUITEM2_EMPTY
};


static void vwglyphlistcheck(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e)) {
    MENU_CHECK_VARIABLES;

    if( !isCharView )
    {
	setAllDisabled(mi,1);
	return;
    }
    
    setAllDisabled(mi,0);
	
    for ( mi = mi->sub; mi->ti.text!=NULL || mi->ti.line ; ++mi ) {
	switch(mi->mid) {
	  case SMID_ShowTabs:
	      mi->ti.checked = cv->showtabs;
	      mi->ti.disabled = cv->former_cnt<=1;
	      break;
	  case SMID_HideRulers:
	      mi->ti.checked = cv->showrulers;
	      break;
	  case SMID_Tools:
	      mi->ti.checked = CVPaletteIsVisible(cv,1);
	      break;
	  case SMID_Layers:
	      mi->ti.checked = CVPaletteIsVisible(cv,0);
	      break;
	  case SMID_DockPalettes:
	      mi->ti.checked = palettes_docked;
	      break;
	}
    }
}


MAKETRAMP(scaleViewToFit);
MAKETRAMP(scaleViewOut);
MAKETRAMP(scaleViewIn);
MAKETRAMP(gotoChar);
MAKETRAMP(gotoCharNext);
MAKETRAMP(gotoCharPrev);
MAKETRAMP(gotoCharNextDefined);
MAKETRAMP(gotoCharPrevDefined);
MAKETRAMP(gotoCharFormer);
MAKETRAMP(wordlistNextLine);
MAKETRAMP(wordlistPrevLine);


GMenuItem2 sharedmenu_view[] = {
    { { UN_("_Fit"),      GI("viewfit.png"), COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 1, 1, 0, 0, 0, 0, 1, 1, 0, 'F' },
                          H_("Fit|No Shortcut"), NULL, NULL, sm_scaleViewToFit, SMID_scaleViewToFit }, 
    { { UN_("Z_oom out"), GI("viewzoomout.png"), COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 1, 1, 0, 0, 0, 0, 1, 1, 0, 'o' },
                          H_("Zoom out|No Shortcut"), NULL, NULL, sm_scaleViewOut, SMID_scaleViewOut },
    { { UN_("Zoom _in"),  GI("viewzoomin.png"), COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 1, 1, 0, 0, 0, 0, 1, 1, 0, 'i' },
                          H_("Zoom in|No Shortcut"), NULL, NULL, sm_scaleViewIn, SMID_scaleViewIn },

    MENUITEM_LINE

    { { UN_("_Goto..."),     GI("viewgoto.png"), MENUBODYC_DEFAULT, 'G' }, H_("Goto...|No Shortcut"),
                             NULL, NULL, sm_gotoChar, SMID_GotoChar },
    { { UN_("Gl_yph Label"), NULL, MENUBODYC_DEFAULT, 'b' }, H_("Glyph Label|No Shortcut"),
                             gllist, gllistcheck, NULL, SMID_GlyphLabel },

    MENUITEM_LINE

    { { UN_("Insert Glyph _After..."),   GI("viewinsertafter.png"), COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 1, 1, 0, 0, 0, 0, 1, 1, 0, 'C' },
                                         H_("Insert Glyph After...|No Shortcut"), NULL, NULL, MVMenuInsertChar, SMID_InsertCharA },
    { { UN_("Insert Glyph _Before..."),  GI("viewinsertbefore.png"), COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 1, 1, 0, 0, 0, 0, 1, 1, 0, 'B' },
                                         H_("Insert Glyph Before...|No Shortcut"), NULL, NULL, MVMenuInsertChar, SMID_InsertCharB },
    { { UN_("_Next Glyph"),         GI("viewnext.png"), MENUBODYC_DEFAULT, 'N' }, H_("Next Glyph|No Shortcut"),
                                    NULL, NULL, sm_gotoCharNext, SMID_Next },
    { { UN_("_Prev Glyph"),         GI("viewprev.png"), MENUBODYC_DEFAULT, 'P' }, H_("Prev Glyph|No Shortcut"),
                                    NULL, NULL, sm_gotoCharPrev, SMID_Prev },
    { { UN_("Next _Defined Glyph"), GI("viewnextdef.png"), MENUBODYC_DEFAULT, 'D' }, H_("Next Defined Glyph|No Shortcut"),
                                    NULL, NULL, sm_gotoCharNextDefined, SMID_NextDef },
    { { UN_("Prev Defined Gl_yph"), GI("viewprevdef.png"), MENUBODYC_DEFAULT, 'a' }, H_("Prev Defined Glyph|No Shortcut"),
                                    NULL, NULL, sm_gotoCharPrevDefined, SMID_PrevDef },
    { { UN_("Form_er Glyph"),       GI("viewformer.png"), COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 1, 1, 0, 0, 0, 0, 1, 1, 0, 'a' },
                                    H_("Former Glyph|No Shortcut"), NULL, NULL, sm_gotoCharFormer, SMID_Former },

    MENUITEM_LINE
    
    { { UN_("N_umber Points"),  NULL, MENUBODYC_DEFAULT, 'o' }, H_("Number Points|No Shortcut"),
                                nplist, nplistcheck, NULL, 0 },
    { { UN_("Grid Fi_t"),       NULL, MENUBODYC_DEFAULT, 'l' }, H_("Grid Fit|No Shortcut"),
                                gflist, gflistcheck, NULL, SMID_ShowGridFit },
    { { UN_("Glyph Elements"),  NULL, MENUBODYC_DEFAULT, '\0' }, H_("Glyph Elements|No Shortcut"),
                                vwelementlist, NULL, NULL, SMID_GlyphElements },
    { { UN_("Glyph Window"),    NULL, MENUBODYC_DEFAULT, '\0' }, H_("Glyph Window|No Shortcut"),
                                vwglyphlist, NULL, NULL, SMID_GlyphWindow },
    
    GMENUITEM2_LINE,
    
    { { UN_("Next _Line in Word List"),     NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 1, 1, 0, 0, 0, 0, 1, 1, 0, 'L' },
                                            H_("Next Line in Word List|No Shortcut"), NULL, NULL, sm_wordlistNextLine, SMID_WordListNextLine },
    { { UN_("Previous Line in _Word List"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 1, 1, 0, 0, 0, 0, 1, 1, 0, 'W' },
                                            H_("Previous Line in Word List|No Shortcut"), NULL, NULL, sm_wordlistPrevLine, SMID_WordListPrevLine },
    GMENUITEM2_EMPTY,
    GMENUITEM2_EMPTY
};

void sharedmenu_view_check(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e))
{
    MENU_CHECK_VARIABLES;
    SplineChar* sc = cc->m_sharedmenu_funcs.getActiveSplineChar(cc);
    SplineFont* sf = tryObtainCastSplineFont( cc );
    
    int anychars = FVAnyCharSelected(fv);
    int i, base;
    BDFFont *bdf;
    char buffer[50];
    int pos;
    SplineFont *master = sf->cidmaster ? sf->cidmaster : sf;
    EncMap *map = fv->b.map;
    OTLookup *otl;

    /* for ( i=0; sharedmenu_view[i].ti.text==NULL || strcmp((char *) sharedmenu_view[i].ti.text, _("Bitmap _Magnification..."))!=0; ++i ) */
    /* { */
    /* 	// nothing */
    /* } */
    
    /* base = i+1; */
    /* for ( i=base; sharedmenu_view[i].ti.text!=NULL; ++i ) { */
    /* 	free( sharedmenu_view[i].ti.text); */
    /* 	sharedmenu_view[i].ti.text = NULL; */
    /* } */

    /* sharedmenu_view[base-1].ti.disabled = true; */
    /* if ( master->bitmaps!=NULL ) { */
    /* 	for ( bdf = master->bitmaps, i=base; */
    /* 		i<sizeof(sharedmenu_view)/sizeof(sharedmenu_view[0])-1 && bdf!=NULL; */
    /* 		++i, bdf = bdf->next ) { */
    /* 	    if ( BDFDepth(bdf)==1 ) */
    /* 		sprintf( buffer, _("%d pixel bitmap"), bdf->pixelsize ); */
    /* 	    else */
    /* 		sprintf( buffer, _("%d@%d pixel bitmap"), */
    /* 			bdf->pixelsize, BDFDepth(bdf) ); */
    /* 	    sharedmenu_view[i].ti.text = (unichar_t *) utf82u_copy(buffer); */
    /* 	    sharedmenu_view[i].ti.checkable = true; */
    /* 	    sharedmenu_view[i].ti.checked = bdf==fv->show; */
    /* 	    sharedmenu_view[i].ti.userdata = bdf; */
    /* 	    sharedmenu_view[i].invoke = FVMenuShowBitmap; */
    /* 	    sharedmenu_view[i].ti.fg = sharedmenu_view[i].ti.bg = COLOR_DEFAULT; */
    /* 	    if ( bdf==fv->show ) */
    /* 		sharedmenu_view[base-1].ti.disabled = false; */
    /* 	} */
    /* } */
    /* GMenuItemArrayFree(mi->sub); */
    /* mi->sub = GMenuItem2ArrayCopy(sharedmenu_view,NULL); */

    for ( mi = mi->sub; mi->ti.text!=NULL || mi->ti.line ; ++mi ) {
	switch ( mi->mid ) {
	  case SMID_GlyphWindow:
	      mi->ti.disabled = isMetricsView;
	      break;
	  case SMID_InsertCharA:
	  case SMID_InsertCharB:
	      mi->ti.disabled = (!isMetricsView);
	      break;
	  case SMID_scaleViewToFit:
	      mi->ti.disabled = (!isCharView);
	      break;
	  case SMID_scaleViewOut:
	      mi->ti.disabled = (!isCharView);
	      break;
	  case SMID_scaleViewIn:
	      mi->ti.disabled = (!isCharView);
	      break;
	  case SMID_GotoChar:
	      mi->ti.disabled = isMetricsView;
	      break;
	  case SMID_Next: case SMID_Prev:
	    mi->ti.disabled = !sc;
	  break;
	  case SMID_NextDef:
	      mi->ti.disabled = 1;
	      if( sc )
	      {
		  pos = SCToPos( sc );
		  pos++;
		  if ( anychars<0 ) pos = map->enccount;
		  for ( ; pos<map->enccount &&
			    (map->map[pos]==-1 || !SCWorthOutputting(sf->glyphs[map->map[pos]]));
			++pos );
		  mi->ti.disabled = pos==map->enccount;
	      }
	      break;
	  case SMID_PrevDef:
	      mi->ti.disabled = 1;
	      if( sc )
	      {
		  pos = SCToPos( sc );
		  pos--;
		  for ( ; pos>=0 &&
			    (map->map[pos]==-1 || !SCWorthOutputting(sf->glyphs[map->map[pos]]));
			--pos )
		  {
		      // nothing
		  }
		  mi->ti.disabled = pos<0;
	      }
	      break;
	  case SMID_Former:
	      mi->ti.disabled = (!isCharView);
	      break;

	  case SMID_WordListNextLine:
	      mi->ti.disabled = (!isCharView);
	      break;
	  case SMID_WordListPrevLine:
	      mi->ti.disabled = (!isCharView);
	      break;

	      

    
	  case SMID_ShowGridFit:
	  case SMID_GlyphElements:
	      mi->ti.disabled = (cv == 0);
	      break;
	  case SMID_DisplaySubs: { SplineFont *_sf = sf;
	    mi->ti.checked = fv->cur_subtable!=NULL;
	    if ( _sf->cidmaster ) _sf = _sf->cidmaster;
	    for ( otl=_sf->gsub_lookups; otl!=NULL; otl=otl->next )
		if ( otl->lookup_type == gsub_single && otl->subtables!=NULL )
	    break;
	    mi->ti.disabled = otl==NULL;
	  } break;
	  case SMID_ShowHMetrics:
	  break;
	  case SMID_ShowVMetrics:
	    mi->ti.disabled = !sf->hasvmetrics;
	  break;
	  case SMID_32x8:
	    mi->ti.checked = (fv->rowcnt==8 && fv->colcnt==32);
	    mi->ti.disabled = fv->b.container!=NULL;
	  break;
	  case SMID_16x4:
	    mi->ti.checked = (fv->rowcnt==4 && fv->colcnt==16);
	    mi->ti.disabled = fv->b.container!=NULL;
	  break;
	  case SMID_8x2:
	    mi->ti.checked = (fv->rowcnt==2 && fv->colcnt==8);
	    mi->ti.disabled = fv->b.container!=NULL;
	  break;
	  case SMID_24:
	    mi->ti.checked = (fv->show!=NULL && fv->show==fv->filled && fv->show->pixelsize==24);
	    mi->ti.disabled = sf->onlybitmaps && fv->show!=fv->filled;
	  break;
	  case SMID_36:
	    mi->ti.checked = (fv->show!=NULL && fv->show==fv->filled && fv->show->pixelsize==36);
	    mi->ti.disabled = sf->onlybitmaps && fv->show!=fv->filled;
	  break;
	  case SMID_AntiAlias:
	    mi->ti.checked = (fv->show!=NULL && fv->show->clut!=NULL);
	    mi->ti.disabled = sf->onlybitmaps && fv->show!=fv->filled;
	  break;
	  case SMID_FitToBbox:
	    mi->ti.checked = (fv->show!=NULL && fv->show->bbsized);
	    mi->ti.disabled = sf->onlybitmaps && fv->show!=fv->filled;
	  break;
	  case SMID_Layers:
	    mi->ti.disabled = sf->layer_cnt<=2 || sf->multilayer;
	  break;
	  case SMID_GlyphLabel:
	      /* if( cv )  */
	      /* 	  mi->ti.disabled = 1; */
	      break;
	}
    }
}




/*********************************************************************************/
/*********************************************************************************/
/*********************************************************************************/
/*********************************************************************************/
/*********************************************************************************/

MAKETRAMP(dialogEmbolden);
MAKETRAMP(dialogItalic);
MAKETRAMP(dialogOblique);
MAKETRAMP(dialogCondenseExtend);
MAKETRAMP(dialogXHeight);
MAKETRAMP(dialogStemsCounters);
MAKETRAMP(dialogInline);
MAKETRAMP(dialogOutline);
MAKETRAMP(dialogShadow);
MAKETRAMP(dialogWireframe);
MAKETRAMP(dialogTransform);
MAKETRAMP(dialogPointOfViewProjection);
MAKETRAMP(dialogNonLinearTransform);
MAKETRAMP(overlapRemove);
MAKETRAMP(overlapIntersect);
MAKETRAMP(overlapExclude);
MAKETRAMP(overlapFindIntersections);
MAKETRAMP(simplify);
MAKETRAMP(simplifyMoreDialog);
MAKETRAMP(simplifyCleanup);
MAKETRAMP(simplifyCanonicalStartPoint);
MAKETRAMP(simplifyCanonicalContours);
MAKETRAMP(dialogExpandStroke);
MAKETRAMP(extremaAdd);


// fv=ok, cv=ok, mv-end ok
static GMenuItem2 eflist[] = {
    { { UN_("_Weight..."),  GI("styleschangeweight.png"), COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, true, 0, 0, 0, 0, 1, 1, 0, 'M' },
                            H_("Change Weight...|No Shortcut"), NULL, NULL, sm_dialogEmbolden, SMID_Embolden },
    { { UN_("_Italic..."),  GI("stylesitalic.png"), COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, true, 0, 0, 0, 0, 1, 1, 0, '\0' },
                            H_("Italic...|No Shortcut"), NULL, NULL, sm_dialogItalic, SMID_Italic },
    { { UN_("Obli_que..."), GI("stylesoblique.png"), COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, true, 0, 0, 0, 0, 1, 1, 0, 'M' },
                            H_("Oblique...|No Shortcut"), NULL, NULL, sm_dialogOblique, 0 },
    { { UN_("_Condense/Extend..."), GI("stylesextendcondense.png"), COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, true, 0, 0, 0, 0, 1, 1, 0, 'M' },
                                    H_("Condense...|No Shortcut"), NULL, NULL, sm_dialogCondenseExtend, SMID_Condense },
    { { UN_("_X-Height..."), GI("styleschangexheight.png"), COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, true, 0, 0, 0, 0, 1, 1, 0, '\0' },
                             H_("Change XHeight...|No Shortcut"), NULL, NULL, sm_dialogXHeight, SMID_ChangeXHeight },
    { { UN_("S_tems and Counters..."), GI("menuempty.png"), COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, true, 0, 0, 0, 0, 1, 1, 0, '\0' },
                                       H_("Change Glyph...|No Shortcut"), NULL, NULL, sm_dialogStemsCounters, SMID_ChangeGlyph },
    MENUITEM_LINE
    { { UN_("In_line..."),    GI("stylesinline.png"), COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, true, 0, 0, 0, 0, 1, 1, 0, 'O' },
                              H_("Inline|No Shortcut"), NULL, NULL, sm_dialogInline, SMID_DialogInline },
    { { UN_("_Outline..."),   GI("stylesoutline.png"), COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, true, 0, 0, 0, 0, 1, 1, 0, 'I' },
                              H_("Outline|No Shortcut"), NULL, NULL, sm_dialogOutline, SMID_DialogOutline },
    { { UN_("S_hadow..."),    GI("stylesshadow.png"), COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, true, 0, 0, 0, 0, 1, 1, 0, 'S' },
                              H_("Shadow|No Shortcut"), NULL, NULL, sm_dialogShadow, SMID_dialogShadow },
    { { UN_("_Wireframe..."), GI("styleswireframe.png"), COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, true, 0, 0, 0, 0, 1, 1, 0, 'W' },
                              H_("Wireframe|No Shortcut"), NULL, NULL, sm_dialogWireframe, SMID_dialogWireframe },
    GMENUITEM2_EMPTY
};

static void eflistcheck(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e))
{
    MENU_CHECK_VARIABLES;
    

    for ( mi = mi->sub; mi && (mi->ti.text!=NULL || mi->ti.line); ++mi ) {
	switch ( mi->mid ) {
	  case SMID_DialogInline:
	  case SMID_DialogOutline:
	  case SMID_dialogShadow:
	  case SMID_dialogWireframe:
	      mi->ti.disabled = 0;
	      break;
	  default:
	      if( isMetricsView )
		  mi->ti.disabled = 1;
	      else
		  mi->ti.disabled = 0;
	      break;
	}
    }
}


static GMenuItem2 trlist[] = {
    { { UN_("_Transform..."), GI("elementtransform.png"), MENUBODYC_DEFAULT, 'T' }, H_("Transform...|No Shortcut"),
	                      NULL, NULL, sm_dialogTransform, SMID_Transform },
    { { UN_("_Point of View Projection..."), NULL, MENUBODYC_DEFAULT, 'T' }, H_("Point of View Projection...|No Shortcut"),
                              NULL, NULL, sm_dialogPointOfViewProjection, SMID_POV },
    { { UN_("_Non Linear Transform..."), NULL, MENUBODYC_DEFAULT, 'T' }, H_("Non Linear Transform...|No Shortcut"),
                              NULL, NULL, sm_dialogNonLinearTransform, SMID_NLTransform },
    GMENUITEM2_EMPTY
};

static void trlistcheck(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e))
{
    MENU_CHECK_VARIABLES;
    SplineFont* sf = tryObtainCastSplineFont( cc );
    SplineChar* sc = cc->m_sharedmenu_funcs.getActiveSplineChar(cc);
    
    for ( mi = mi->sub; mi && (mi->ti.text!=NULL || mi->ti.line); ++mi ) {
	switch ( mi->mid ) {
	  case SMID_Transform:
	      mi->ti.disabled = !sc;
	      break;
	  default:
	      if( isMetricsView )
		  mi->ti.disabled = 1;
	      else
		  mi->ti.disabled = !sc || sf->onlybitmaps;
	      break;
	}
    }
}

static GMenuItem2 rmlist[] = {
    { { UN_("_Remove"),   GI("overlaprm.png"), COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, true, 0, 0, 0, 0, 1, 1, 0, 'R' },
                          H_("Remove Overlap|No Shortcut"), NULL, NULL, sm_overlapRemove, SMID_RmOverlap },
    { { UN_("_Intersect"),GI("overlapintersection.png"), COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, true, 0, 0, 0, 0, 1, 1, 0, 'I' },
                          H_("Intersect|No Shortcut"), NULL, NULL, sm_overlapIntersect, SMID_Intersection },
    { { UN_("_Exclude"),  GI("overlapexclude.png"), COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, true, 0, 0, 0, 0, 1, 1, 0, 'E' },
                          H_("Exclude|No Shortcut"), NULL, NULL, sm_overlapExclude, SMID_Exclude },
    { { UN_("_Find Intersections"), GI("overlapfindinter.png"), COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, true, 0, 0, 0, 0, 1, 1, 0, 'F' },
                          H_("Find Intersections|No Shortcut"), NULL, NULL, sm_overlapFindIntersections, SMID_FindInter },
    GMENUITEM2_EMPTY
};

static void rmlistcheck(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e))
{
    MENU_CHECK_VARIABLES;
    SplineFont* sf = tryObtainCastSplineFont( cc );
    SplineChar* sc = cc->m_sharedmenu_funcs.getActiveSplineChar(cc);
    
    for ( mi = mi->sub; mi && (mi->ti.text!=NULL || mi->ti.line); ++mi ) {
	switch ( mi->mid ) {
	  case SMID_RmOverlap:
	      mi->ti.disabled = !sc || fv->b.sf->onlybitmaps;
	  case SMID_Intersection:
	  case SMID_Exclude:
	  case SMID_FindInter:
	      mi->ti.disabled = 0;
	      break;
	}
    }
}

static GMenuItem2 smlist[] = {
    { { UN_("_Simplify"), GI("elementsimplify.png"), MENUBODYC_DEFAULT, 'S' }, H_("Simplify|No Shortcut"), NULL, NULL, sm_simplify, SMID_Simplify },
    { { UN_("Simplify _with settings..."), NULL, MENUBODYC_DEFAULT, 'M' }, H_("Simplify More...|No Shortcut"), NULL, NULL, sm_simplifyMoreDialog, SMID_SimplifyMore },
    { { UN_("_Cleanup"), NULL, MENUBODYC_DEFAULT, 'n' }, H_("Cleanup Glyph|No Shortcut"), NULL, NULL, sm_simplifyCleanup, SMID_CleanupGlyph },
    { { UN_("Canonical Start _Point"), NULL, MENUBODYC_DEFAULT, 'n' }, H_("Canonical Start Point|No Shortcut"), NULL, NULL, sm_simplifyCanonicalStartPoint, SMID_CanonicalStart },
    { { UN_("Canonical Path _Order"), NULL, MENUBODYC_DEFAULT, 'n' }, H_("Canonical Contours|No Shortcut"), NULL, NULL, sm_simplifyCanonicalContours, SMID_CanonicalContours },
    GMENUITEM2_EMPTY
};

static void smlistcheck(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e))
{
    MENU_CHECK_VARIABLES;
    SplineFont* sf = tryObtainCastSplineFont( cc );
    SplineChar* sc = cc->m_sharedmenu_funcs.getActiveSplineChar(cc);
    
    for ( mi = mi->sub; mi && (mi->ti.text!=NULL || mi->ti.line); ++mi ) {
	switch ( mi->mid ) {
	  case SMID_CanonicalStart:
	      if( isMetricsView ) {
		  mi->ti.disabled = 1;
	      }
	      else {
		  mi->ti.disabled = 0;
		  if( cv )
		      mi->ti.disabled = cv->b.layerheads[cv->b.drawmode]->splines==NULL ||
			  (cv->b.sc->inspiro && hasspiro());
	      }
	  case SMID_CanonicalContours:
	      if( isMetricsView ) {
		  mi->ti.disabled = 1;
	      }
	      else {
		  mi->ti.disabled = 0;
		  if( cv )
		      mi->ti.disabled = cv->b.layerheads[cv->b.drawmode]->splines==NULL ||
			  (cv->b.sc->inspiro && hasspiro());
	      }
	      
	      break;
	  default:
	      mi->ti.disabled = 0;
	      if( cv )
		  mi->ti.disabled = cv->b.layerheads[cv->b.drawmode]->splines==NULL;
	      break;
	}
    }
}


static GMenuItem2 interpolationlist[] = {
    { { UN_("Can Be _Interpolated"),  NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 1, 1, 1, 0, 0, 0, 1, 1, 0, 'T' }, H_("Can Be Interpolated|No Shortcut"), NULL, NULL, NULL, 0 },
    { { UN_("Can't _Be Interpolated"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 1, 1, 1, 0, 0, 0, 1, 1, 0, 'T' }, H_("Can't Be Interpolated|No Shortcut"), NULL, NULL, NULL, 0 },
  GMENUITEM2_EMPTY
};

GMenuItem2 sharedmenu_path[] = {
    { { UN_("_Name..."),   GI("pointsnamecontour.png"), MENUBODYC_DEFAULT, 'M' }, H_("Name Contour|No Shortcut"),
                           NULL, NULL, CVMenuNameContour, SMID_NameContour },
    MENUITEM_LINE
    { { UN_("_Join"),      GI("editjoin.png"), MENUBODYC_DEFAULT, 'J' }, H_("Join|No Shortcut"),
                           NULL, NULL, CVJoin, SMID_Join },
    { { UN_("Make _Line"), GI("pointsmakeline.png"), MENUBODYC_DEFAULT, 'M' }, H_("Make Line|No Shortcut"),
                           NULL, NULL, CVMenuMakeLine, SMID_MakeLine },
    { { UN_("Ma_ke Arc"),  GI("pointsmakearc.png"), MENUBODYC_DEFAULT, 'M' }, H_("Make Arc|No Shortcut"),
                           NULL, NULL, CVMenuMakeLine, SMID_MakeArc },
    MENUITEM_LINE
    { { UN_("Make _Parallel..."), NULL, MENUBODYC_DEFAULT, 'P' }, H_("Make Parallel...|No Shortcut"),
                           NULL, NULL, CVMenuMakeParallel, SMID_MakeParallel },
    MENUITEM_LINE
    { { UN_("Cloc_kwise"), GI("elementclockwise.png"), MENUBODYD_DEFAULT, 'o' }, H_("Clockwise|No Shortcut"),
                           NULL, NULL, CVMenuDir, SMID_Clockwise },
    { { UN_("Cou_nter Clockwise"),GI("elementanticlock.png"), MENUBODYD_DEFAULT, 'n' }, H_("Counter Clockwise|No Shortcut"),
                                  NULL, NULL, CVMenuDir, SMID_Counter },
    { { UN_("_Correct Direction"),GI("elementcorrectdir.png"), MENUBODYC_DEFAULT, 'D' }, H_("Correct Direction|No Shortcut"),
                                  NULL, NULL, CVMenuCorrectDir, SMID_Correct },
    { { UN_("Reverse Direction"), GI("menuempty.png"), MENUBODYC_DEFAULT, 'D' }, H_("Reverse Direction|No Shortcut"),
                                  NULL, NULL, CVMenuReverseDir, SMID_ReverseDir },
    { { UN_("Add E_xtrema"),      GI("elementaddextrema.png"), MENUBODYC_DEFAULT, 'x' }, H_("Add Extrema|No Shortcut"),
                                  NULL, NULL, sm_extremaAdd, SMID_AddExtrema },
    { { UN_("Acceptable _Extrema"), GI("menuempty.png"), MENUBODYD_DEFAULT, 'C' }, H_("Acceptable Extrema|No Shortcut"),
                                    NULL, NULL, CVMenuAcceptableExtrema, SMID_AcceptableExtrema },

    MENUITEM_LINE
    
    { { UN_("Modify"),     GI("elementstyles.png"), MENUBODYC_DEFAULT, '\0' }, H_("Styles|No Shortcut"),
                           eflist, eflistcheck, NULL, SMID_Styles },
    { { UN_("_Transform"), GI("elementtransform.png"), MENUBODYC_DEFAULT, 'T' }, H_("Transform|No Shortcut"),
                           trlist, trlistcheck, NULL, 0 },
    { { UN_("O_verlap"),   GI("overlaprm.png"), MENUBODYC_DEFAULT, 'O' }, H_("Overlap|No Shortcut"),
                           rmlist, rmlistcheck, NULL, SMID_RmOverlap },
    { { UN_("_Simplify"),  GI("elementsimplify.png"), MENUBODYC_DEFAULT, 'S' }, H_("Simplify|No Shortcut"),
                           smlist, smlistcheck, NULL, SMID_Simplify },
    
    MENUITEM_LINE

    { { UN_("_Expand Stroke..."), GI("elementexpandstroke.png"), MENUBODYC_DEFAULT, 'E' }, H_("Expand Stroke...|No Shortcut"),
                                  NULL, NULL, sm_dialogExpandStroke, SMID_Stroke },
    { { UN_("Compare Layers..."), GI("elementcomparelayers.png"), MENUBODYC_DEFAULT, 'u' }, H_("Compare Layers...|No Shortcut"),
                                  NULL, NULL, sm_dialogCompareLayers, 0 },

    MENUITEM_LINE
    
    { { UN_("Autot_race"), GI("elementautotrace.png"), MENUBODYC_DEFAULT, 'r' }, H_("Autotrace|No Shortcut"),
                           NULL, NULL, CVMenuAutotrace, SMID_Autotrace },
    MENUITEM_LINE
    { { UN_("Interpol_tion"), NULL, MENUBODYC_DEFAULT, '\0' }, H_("Interpolation|No Shortcut"),
                              interpolationlist, NULL, NULL, 0 },
	GMENUITEM2_EMPTY
};

void sharedmenu_path_check(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e))
{
    CommonView* cc = tryObtainGDataCommonView( gw );
    CharView* cv = tryObtainGDataCharView( gw );
    FontView* fv = tryObtainGDataFontView( gw );
    for ( mi = mi->sub; mi->ti.text!=NULL || mi->ti.line ; ++mi ) {
	switch ( mi->mid ) {
	  case SMID_NameContour:
	  case SMID_Join:
	  case SMID_MakeLine:
	  case SMID_MakeArc:
	  case SMID_MakeParallel:
	  case SMID_Clockwise:
	  case SMID_Counter:
	  case SMID_Correct:
	  case SMID_ReverseDir:
	  case SMID_AcceptableExtrema:
	  case SMID_Autotrace:
	      mi->ti.disabled = (cv == 0);
	      break;
	}
    }
    
}


/*********************************************************************************/
/*********************************************************************************/
/*********************************************************************************/
/*********************************************************************************/
/*********************************************************************************/

MAKETRAMP(dialogCharInfo);
MAKETRAMP(dialogKernPairs);
MAKETRAMP(dialogLigatures);
MAKETRAMP(accentBuild);
MAKETRAMP(compositeBuild);
MAKETRAMP(duplicateGlyphs);
MAKETRAMP(selectionClear);



static GMenuItem2 dummyall[] = {
    { { (unichar_t *) N_("All"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 1, 0, 0, 0, 0, 0, 1, 1, 0, 'K' }, H_("All|No Shortcut"), NULL, NULL, NULL, 0 },
    GMENUITEM2_EMPTY
};

static GMenuItem2 scollist[] = {
    { { UN_("Color|Choose..."), (GImage *)"colorwheel.png", COLOR_DEFAULT, COLOR_DEFAULT, (void *) -10, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, '\0' }, H_("Color Choose|No Shortcut"), NULL, NULL, FVMenuSetColor, 0 },
    { { (unichar_t *)  N_("Color|Default"), &def_image, COLOR_DEFAULT, COLOR_DEFAULT, (void *) COLOR_DEFAULT, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, '\0' }, H_("Default|No Shortcut"), NULL, NULL, FVMenuSetColor, 0 },
    { { NULL, &white_image,  COLOR_DEFAULT, COLOR_DEFAULT, (void *) 0xffffff, NULL, 0, 1, 0, 0, 0, 0, 0, 0, 0, '\0' }, NULL, NULL, NULL, FVMenuSetColor, 0 },
    { { NULL, &red_image,    COLOR_DEFAULT, COLOR_DEFAULT, (void *) 0xff0000, NULL, 0, 1, 0, 0, 0, 0, 0, 0, 0, '\0' }, NULL, NULL, NULL, FVMenuSetColor, 0 },
    { { NULL, &green_image,  COLOR_DEFAULT, COLOR_DEFAULT, (void *) 0x00ff00, NULL, 0, 1, 0, 0, 0, 0, 0, 0, 0, '\0' }, NULL, NULL, NULL, FVMenuSetColor, 0 },
    { { NULL, &blue_image,   COLOR_DEFAULT, COLOR_DEFAULT, (void *) 0x0000ff, NULL, 0, 1, 0, 0, 0, 0, 0, 0, 0, '\0' }, NULL, NULL, NULL, FVMenuSetColor, 0 },
    { { NULL, &yellow_image, COLOR_DEFAULT, COLOR_DEFAULT, (void *) 0xffff00, NULL, 0, 1, 0, 0, 0, 0, 0, 0, 0, '\0' }, NULL, NULL, NULL, FVMenuSetColor, 0 },
    { { NULL, &cyan_image,   COLOR_DEFAULT, COLOR_DEFAULT, (void *) 0x00ffff, NULL, 0, 1, 0, 0, 0, 0, 0, 0, 0, '\0' }, NULL, NULL, NULL, FVMenuSetColor, 0 },
    { { NULL, &magenta_image,COLOR_DEFAULT, COLOR_DEFAULT, (void *) 0xff00ff, NULL, 0, 1, 0, 0, 0, 0, 0, 0, 0, '\0' }, NULL, NULL, NULL, FVMenuSetColor, 0 },
    GMENUITEM2_EMPTY
};

static void scollistcheck(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e)) {
    MENU_CHECK_VARIABLES;
    int pos = FVAnyCharSelected(fv);

    for ( mi = mi->sub; mi && (mi->ti.text!=NULL || mi->ti.line); ++mi ) {
	if( !isFontView )
	    mi->ti.disabled = 1;
	else
	    mi->ti.disabled = (pos == -1 );
    }
}


MAKETRAMP(hintDoAutoHint);
MAKETRAMP(hintAutoSubs);
MAKETRAMP(hintAutoCounter);
MAKETRAMP(hintDontAutoHint);
MAKETRAMP(hintAutoInstr);
MAKETRAMP(hintEditInstructionsDialog);
MAKETRAMP(hintEditTable_fpgm);
MAKETRAMP(hintEditTable_prep);
MAKETRAMP(hintEditTable_maxp);
MAKETRAMP(hintEditTable_cvt);
MAKETRAMP(hintRemoveInstructionTables);
MAKETRAMP(hintSuggestDeltasDialog);
MAKETRAMP(hintClear);
MAKETRAMP(hintClearInstructions);
MAKETRAMP(histogramHStemDialog);
MAKETRAMP(histogramVStemDialog);
MAKETRAMP(histogramBlueValuesDialog);


static GMenuItem2 htlist[] = {
    { { UN_("Auto_Hint"),     GI("hintsautohint.png"), MENUBODYC_DEFAULT, 'H' },
                              H_("AutoHint|No Shortcut"), NULL, NULL, sm_hintDoAutoHint, SMID_AutoHint },
    { { UN_("Hint _Substitution Points"), GI("menuempty.png"), MENUBODYC_DEFAULT, 'H' },
                              H_("Hint Substitution Pts|No Shortcut"), NULL, NULL, sm_hintAutoSubs, SMID_HintSubsPt },
    { { UN_("Auto _Counter Hint"), GI("menuempty.png"), MENUBODYC_DEFAULT, 'H' },
                              H_("Auto Counter Hint|No Shortcut"), NULL, NULL, sm_hintAutoCounter, SMID_AutoCounter },
    { { UN_("_Don't AutoHint"), GI("hintsdontautohint.png"), MENUBODYC_DEFAULT, 'H' },
                              H_("Don't AutoHint|No Shortcut"), NULL, NULL, sm_hintDontAutoHint, SMID_DontAutoHint },

    MENUITEM_LINE
    
    { { UN_("Auto_Instr"), GI("menuempty.png"), MENUBODYC_DEFAULT, 'T' },
                             H_("AutoInstr|No Shortcut"), NULL, NULL, sm_hintAutoInstr, SMID_AutoInstr },
    { { UN_("_Edit Instructions..."), GI("menuempty.png"), MENUBODYC_DEFAULT, 'l' },
                             H_("Edit Instructions...|No Shortcut"), NULL, NULL, sm_hintEditInstructionsDialog, SMID_EditInstructions },

    { { UN_("Edit 'fpgm'..."), GI("menuempty.png"), MENUBODYC_DEFAULT, '\0' },
                             H_("Edit 'fpgm'...|No Shortcut"), NULL, NULL, sm_hintEditTable_fpgm, SMID_Editfpgm },
    { { UN_("Edit 'prep'..."), GI("menuempty.png"), MENUBODYC_DEFAULT, '\0' },
                             H_("Edit 'prep'...|No Shortcut"), NULL, NULL, sm_hintEditTable_prep, SMID_Editprep },
    { { UN_("Edit 'maxp'..."), GI("menuempty.png"), MENUBODYC_DEFAULT, '\0' },
                             H_("Edit 'maxp'...|No Shortcut"), NULL, NULL, sm_hintEditTable_maxp, SMID_Editmaxp },
    { { UN_("Edit 'cvt '..."), GI("menuempty.png"), MENUBODYC_DEFAULT, '\0' },
                             H_("Edit 'cvt '...|No Shortcut"), NULL, NULL, sm_hintEditTable_cvt,  SMID_Editcvt },
    { { UN_("Remove Instr Tables"), GI("menuempty.png"), MENUBODYC_DEFAULT, '\0' },
                             H_("Remove Instr Tables|No Shortcut"), NULL, NULL, sm_hintRemoveInstructionTables, SMID_RmInstrTables },
    { { UN_("S_uggest Deltas..."), GI("menuempty.png"), MENUBODYC_DEFAULT, 'l' },
                             H_("Suggest Deltas|No Shortcut"), NULL, NULL, sm_hintSuggestDeltasDialog, SMID_Deltas },

    MENUITEM_LINE
    
    { { UN_("_Clear Hints"), GI("hintsclearvstems.png"), MENUBODYC_DEFAULT, 'C' },
                             H_("Clear Hints|No Shortcut"), NULL, NULL, sm_hintClear, SMID_ClearHints },
    { { UN_("Clear Instructions"),  GI("menuempty.png"), MENUBODYC_DEFAULT, '\0' },
                             H_("Clear Instructions|No Shortcut"), NULL, NULL, sm_hintClearInstructions, SMID_ClearInstr },
    
    MENUITEM_LINE

    { { UN_("Histo _HStem"), NULL, MENUBODYC_DEFAULT, 'H' },
                             H_("HStem|No Shortcut"), NULL, NULL, sm_histogramHStemDialog, SMID_HStemHist },
    { { UN_("Histo _VStem"), NULL, MENUBODYC_DEFAULT, 'V' },
                             H_("VStem|No Shortcut"), NULL, NULL, sm_histogramVStemDialog, SMID_VStemHist },
    { { UN_("Histo BlueValues"), NULL, MENUBODYC_DEFAULT, 'B' },
                             H_("BlueValues|No Shortcut"), NULL, NULL, sm_histogramBlueValuesDialog, SMID_BlueValuesHist },
    
    GMENUITEM2_EMPTY
};


static void htlistcheck(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e)) {
    MENU_CHECK_VARIABLES;
    if( !isCharView )
	return;

    int cvlayer = CVLayer((CharViewBase *) cv);
    BasePoint *bp[4], unit;
    int multilayer = cv ? cv->b.sc->parent->multilayer : fv->b.sf->multilayer;
    int i=0, num = 0;
    for (i=0; i<4; i++) {bp[i]=NULL;}
    int anychars = FVAnyCharSelected(fv);

    if( isCharView )
	anychars = 1;
    if( isMetricsView )
	anychars = -1;

    num = CVNumForePointsSelected(cv,bp);

    for ( mi = mi->sub; mi->ti.text!=NULL || mi->ti.line ; ++mi ) {
	switch ( mi->mid ) {
	  case SMID_ClearHints:
	  case SMID_ClearWidthMD:
	  case SMID_ClearInstrs:
	      if( !isFontView )
		  mi->ti.disabled = 1;
	      else
		  mi->ti.disabled = anychars==-1;
	      break;
	  case SMID_HStemHist:
	  case SMID_VStemHist:
	  case SMID_BlueValuesHist:
	      mi->ti.disabled = !isFontView;
	      break;
	    
	    
	  case SMID_Editfpgm: case SMID_Editprep: case SMID_Editcvt: case SMID_Editmaxp:
	      if( isFontView )
		  mi->ti.disabled = !fv->b.sf->layers[fv->b.active_layer].order2 || multilayer;
	      else
		  mi->ti.disabled = 1;
	      break;
	    
	  case SMID_AutoHint:
	      if( isCharView )
		  mi->ti.disabled = cvlayer == ly_grid || multilayer;
	      else if( isMetricsView )
		  mi->ti.disabled = 1;
	      else
		  mi->ti.disabled = anychars==-1 || multilayer;
	  break;
	  case SMID_HintSubsPt:
	      if( isCharView )
		  mi->ti.disabled = multilayer ||
		      cv->b.layerheads[cv->b.drawmode]->order2 ||
		      cvlayer == ly_grid;
	      else if( isMetricsView )
		  mi->ti.disabled = 1;
	      else
	      {
		  mi->ti.disabled = fv->b.sf->layers[fv->b.active_layer].order2 || anychars==-1 || multilayer;
		  if ( fv->b.sf->mm!=NULL && fv->b.sf->mm->apple )
		      mi->ti.disabled = true;
	      }
	      
	  break;
	  case SMID_AutoCounter:
	      if( isFontView )
		  mi->ti.disabled = fv->b.sf->layers[fv->b.active_layer].order2 || anychars==-1 || multilayer;
	      else
		  mi->ti.disabled = multilayer;
	  break;
	  case SMID_DontAutoHint:
	      if( isFontView )
		  mi->ti.disabled = fv->b.sf->layers[fv->b.active_layer].order2 || anychars==-1 || multilayer;
	      else
		  mi->ti.disabled = cv->b.layerheads[cv->b.drawmode]->order2 || multilayer;
	      
	      if( isCharView )
		  mi->ti.checked = cv->b.sc->manualhints;
	  break;
	  case SMID_AutoInstr:
	  case SMID_EditInstructions:
	      if( isCharView )
	      {
		  mi->ti.disabled = multilayer ||
		      !cv->b.layerheads[cv->b.drawmode]->order2 ||
		      cvlayer == ly_grid;
	      }
	      else if( isMetricsView )
		  mi->ti.disabled = 1;
	      else
		  mi->ti.disabled = !fv->b.sf->layers[fv->b.active_layer].order2 || anychars==-1 || multilayer;
	  break;
	  case SMID_Debug:
	    mi->ti.disabled = multilayer ||
		!cv->b.layerheads[cv->b.drawmode]->order2 ||
		!hasFreeTypeDebugger();
	  break;
	  case SMID_Deltas:
	      if( isCharView )
	      {
		  mi->ti.disabled = multilayer ||
		      !cv->b.layerheads[cv->b.drawmode]->order2 ||
		      !hasFreeTypeDebugger();
	      }
	      else if( isMetricsView )
		  mi->ti.disabled = 1;
	      else
		  mi->ti.disabled = !fv->b.sf->layers[fv->b.active_layer].order2 || anychars==-1 || multilayer;
	      
	  break;
          case SMID_ClearHStem:
          case SMID_ClearVStem:
          case SMID_ClearDStem:
	    mi->ti.disabled = cvlayer == ly_grid;
	  break;
	  case SMID_ClearInstr:
	    mi->ti.disabled = cv->b.sc->ttf_instrs_len==0;
	  break;
	  case SMID_AddHHint:
	    mi->ti.disabled = num != 2 || bp[1]->y==bp[0]->y || multilayer;
	  break;
	  case SMID_AddVHint:
	    mi->ti.disabled = num != 2 || bp[1]->x==bp[0]->x || multilayer;
	  break;
	  case SMID_AddDHint:
	    mi->ti.disabled = num != 4 || !PointsDiagonalable( cv->b.sc->parent,bp,&unit ) || multilayer;
	  break;
          case SMID_CreateHHint:
          case SMID_CreateVHint:
	    mi->ti.disabled = cvlayer == ly_grid;
	  break;
	  case SMID_ReviewHints:
	    mi->ti.disabled = (cv->b.sc->hstem==NULL && cv->b.sc->vstem==NULL ) || multilayer;
	  break;
	  case SMID_RmInstrTables:
	    mi->ti.disabled = fv->b.sf->ttf_tables==NULL;
	  break;
	}
    }
}


static void fvhtlistcheck(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e)) {
    MENU_CHECK_VARIABLES;
    int anychars = FVAnyCharSelected(fv);
    int multilayer = fv->b.sf->multilayer;

    for ( mi = mi->sub; mi && (mi->ti.text!=NULL || mi->ti.line); ++mi ) {
	switch ( mi->mid ) {
	  case SMID_AutoHint:
	    mi->ti.disabled = anychars==-1 || multilayer;
	  break;
	  case SMID_HintSubsPt:
	    mi->ti.disabled = fv->b.sf->layers[fv->b.active_layer].order2 || anychars==-1 || multilayer;
	    if ( fv->b.sf->mm!=NULL && fv->b.sf->mm->apple )
		mi->ti.disabled = true;
	  break;
	  case SMID_AutoCounter: case SMID_DontAutoHint:
	    mi->ti.disabled = fv->b.sf->layers[fv->b.active_layer].order2 || anychars==-1 || multilayer;
	  break;
	  case SMID_AutoInstr: case SMID_EditInstructions: case SMID_Deltas:
	    mi->ti.disabled = !fv->b.sf->layers[fv->b.active_layer].order2 || anychars==-1 || multilayer;
	  break;
	  case SMID_RmInstrTables:
	    mi->ti.disabled = fv->b.sf->ttf_tables==NULL;
	  break;
	  case SMID_Editfpgm: case SMID_Editprep: case SMID_Editcvt: case SMID_Editmaxp:
	    mi->ti.disabled = !fv->b.sf->layers[fv->b.active_layer].order2 || multilayer;
	  break;
	  case SMID_ClearHints: case SMID_ClearWidthMD: case SMID_ClearInstrs:
	    mi->ti.disabled = anychars==-1;
	  break;
	}
    }
}

/* Builds up a menu containing all the anchor classes */
static void aplistbuild(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e)) {
    MENU_CHECK_VARIABLES;

    GMenuItemArrayFree(mi->sub);
    mi->sub = NULL;

    _aplistbuild(mi,fv->b.sf,FVMenuAnchorPairs);
}

static GMenuItem2 ap2list[] = {
    GMENUITEM2_EMPTY
};

static void ap2listbuild(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e)) {
    MENU_CHECK_VARIABLES;
    if( !isCharView )
	return;
    char buf[300];
    GMenuItem *sub;
    int k, cnt;
    AnchorPoint *ap;

    if ( mi->sub!=NULL ) {
	GMenuItemArrayFree(mi->sub);
	mi->sub = NULL;
    }

    for ( k=0; k<2; ++k ) {
	cnt = 0;
	for ( ap=cv->b.sc->anchor; ap!=NULL; ap=ap->next ) {
	    if ( k ) {
		if ( ap->type==at_baselig )
/* GT: In the next few lines the "%s" is the name of an anchor class, and the */
/* GT: rest of the string identifies the type of the anchor */
		    snprintf(buf,sizeof(buf), _("%s at ligature pos %d"), ap->anchor->name, ap->lig_index );
		else
		    snprintf(buf,sizeof(buf),
			ap->type==at_cexit ? _("%s exit"):
			ap->type==at_centry ? _("%s entry"):
			ap->type==at_mark ? _("%s mark"):
			    _("%s base"),ap->anchor->name );
		sub[cnt].ti.text = utf82u_copy(buf);
		sub[cnt].ti.userdata = ap;
		sub[cnt].ti.bg = sub[cnt].ti.fg = COLOR_DEFAULT;
		sub[cnt].invoke = CVMenuAnchorsAway;
	    }
	    ++cnt;
	}
	if ( !k )
	    sub = calloc(cnt+1,sizeof(GMenuItem));
    }
    mi->sub = sub;
}

static GMenuItem2 aplist[] = {
    { { UN_("_Detach"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 1, 1, 0, 0, 0, 0, 1, 1, 0, 'K' }, H_("Detach|No Shortcut"), NULL, NULL, NULL, 0 },
    GMENUITEM2_EMPTY
};

static void aplistcheck(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e)) {
    MENU_CHECK_VARIABLES;
    if( !isCharView )
	return;
    SplineChar *sc = cv->b.sc, **glyphs;
    SplineFont *sf = sc->parent;
    AnchorPoint *ap, *found;
    GMenuItem2 *mit;
    int cnt;

    found = NULL;
    for ( ap=sc->anchor; ap!=NULL; ap=ap->next ) {
	if ( ap->selected ) {
	    if ( found==NULL )
		found = ap;
	    else {
		/* Can't deal with two selected anchors */
		found = NULL;
		break;
	    }
	}
    }

    GMenuItemArrayFree(mi->sub);
    if ( found==NULL )
	glyphs = NULL;
    else
	glyphs = GlyphsMatchingAP(sf,found);
    if ( glyphs==NULL ) {
	mi->sub = GMenuItem2ArrayCopy(aplist,NULL);
	mi->sub->ti.disabled = (cv->apmine==NULL);
	return;
    }

    for ( cnt = 0; glyphs[cnt]!=NULL; ++cnt );
    mit = calloc(cnt+2,sizeof(GMenuItem2));
    mit[0] = aplist[0];
    mit[0].ti.text = (unichar_t *) copy( (char *) mit[0].ti.text );
    mit[0].ti.disabled = (cv->apmine==NULL);
    for ( cnt = 0; glyphs[cnt]!=NULL; ++cnt ) {
	mit[cnt+1].ti.text = (unichar_t *) copy(glyphs[cnt]->name);
	mit[cnt+1].ti.text_is_1byte = true;
	mit[cnt+1].ti.fg = mit[cnt+1].ti.bg = COLOR_DEFAULT;
	mit[cnt+1].ti.userdata = glyphs[cnt];
	mit[cnt+1].invoke = CVMenuAPAttachSC;
	if ( glyphs[cnt]==cv->apsc )
	    mit[cnt+1].ti.checked = mit[cnt+1].ti.checkable = true;
    }
    free(glyphs);
    mi->sub = GMenuItem2ArrayCopy(mit,NULL);
    GMenuItem2ArrayFree(mit);
}

static GMenuItem2 balist[] = {
    { { UN_("_Accented"),  GI("elementbuildaccent.png"), MENUBODYC_DEFAULT, 'u' }, H_("Build Accented Glyph|No Shortcut"),
	                   NULL, NULL, sm_accentBuild, SMID_BuildAccent },
    { { UN_("_Composite"), GI("elementbuildcomposite.png"), MENUBODYC_DEFAULT, 'B' }, H_("Build Composite Glyph|No Shortcut"),
                           NULL, NULL, sm_compositeBuild, SMID_BuildComposite },
    { { UN_("_Duplicate"), GI("menuempty.png"), MENUBODYC_DEFAULT, 'B' }, H_("Build Duplicate Glyph|No Shortcut"),
                           NULL, NULL, sm_duplicateGlyphs, SMID_BuildDuplicates },
#ifdef KOREAN
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 1, 0, 0, 0, '\0' }, NULL, NULL, NULL, 0, 0 }, /* line */
    { { (unichar_t *) _STR_ShowGrp, NULL, MENUBODYC_DEFAULT, 'B' }, NULL, NULL, NULL, FVMenuShowGroup },
#endif
    GMENUITEM2_EMPTY
};

static void balistcheck(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e)) {
    MENU_CHECK_VARIABLES;
    SplineFont* sf = tryObtainCastSplineFont( cc );
    int layer = cc->m_sharedmenu_funcs.getActiveLayer(cc);
    SplineChar* sc = cc->m_sharedmenu_funcs.getActiveSplineChar(cc);
	
    for ( mi = mi->sub; mi->ti.text!=NULL || mi->ti.line ; ++mi ) {
	switch ( mi->mid ) {
	  case SMID_BuildAccent:
	      mi->ti.disabled = !sc || !SFIsSomethingBuildable(sf,sc,layer,true);
	  break;
	  case SMID_BuildComposite:
	      mi->ti.disabled = !sc || !SFIsSomethingBuildable(sf,sc,layer,false);
	  break;
	  case SMID_BuildDuplicates:
	      if( isFontView )
	      {
		  int anybuildable = false;
		  int i, gid;
		  for ( i=0; i<fv->b.map->enccount; ++i ) if ( fv->b.selected[i] ) {
			  SplineChar *sc=NULL, dummy;
			  if ( (gid=fv->b.map->map[i])!=-1 )
			      sc = fv->b.sf->glyphs[gid];
			  if ( sc==NULL )
			      sc = SCBuildDummy(&dummy,fv->b.sf,fv->b.map,i);
			  if ( SFIsDuplicatable(fv->b.sf,sc)) {
			      anybuildable = true;
			      break;
			  }
		      }
		  mi->ti.disabled = !anybuildable;
	      }
	      else if( isMetricsView )
	      {
		  mi->ti.disabled = 1;
	      }
	      else
	      {
		  mi->ti.disabled = !sc || !SFIsDuplicatable(sf,sc);
	      }
	      break;

        }
    }
}

static GMenuItem2 glyphnameslist[] = {
    { { UN_("Rename..."), NULL, MENUBODYC_DEFAULT, 'C' }, H_("Rename Glyphs...|No Shortcut"), NULL, NULL, FVMenuRenameByNamelist, SMID_RenameGlyphs },
    { { UN_("Cre_ate Named..."), NULL, MENUBODYC_DEFAULT, 'C' }, H_("Create Named Glyphs...|No Shortcut"), NULL, NULL, FVMenuNameGlyphs, SMID_NameGlyphs },
    { { UN_("L_oad Namelist..."), NULL, MENUBODYC_DEFAULT, 'C' }, H_("Load Namelist...|No Shortcut"), NULL, NULL, FVMenuLoadNamelist, 0 },
    { { UN_("_Save Namelist of Font..."), NULL, MENUBODYC_DEFAULT, 'C' }, H_("Save Namelist of Font...|No Shortcut"), NULL, NULL, FVMenuMakeNamelist, SMID_SaveNamelist },

    GMENUITEM2_EMPTY
};

static void glyphnameslistcheck(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e)) {
    MENU_CHECK_VARIABLES;
}

MAKETRAMP(referenceShowDependentRefs);
MAKETRAMP(referenceUnlink);


static GMenuItem2 referenceslist[] = {

    { { UN_("Show"),    GI("menuempty.png"), MENUBODYC_DEFAULT, 'u' }, H_("References...|No Shortcut"),
                        NULL, NULL, sm_referenceShowDependentRefs, SMID_ShowDependentRefs }, 
    { { UN_("Replace"), GI("menuempty.png"), MENUBODYC_DEFAULT, 'i' }, H_("Replace with Reference|No Shortcut"),
                        NULL, NULL, FVMenuReplaceWithRef, SMID_RplRef },
    { { UN_("Correct"), GI("menuempty.png"), MENUBODYC_DEFAULT, 'i' }, H_("Correct References|No Shortcut"),
                        NULL, NULL, FVMenuCorrectRefs, SMID_CorrectRefs },
    { { UN_("Unlink"),  GI("menuempty.png"), MENUBODYC_DEFAULT, 'U' }, H_("Unlink Reference|No Shortcut"),
                        NULL, NULL, sm_referenceUnlink, SMID_UnlinkRef },
    GMENUITEM2_EMPTY
};

static void referenceslistcheck(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e)) {
    MENU_CHECK_VARIABLES;
    int i = FVAnyCharSelected(fv);
    int gid = i<0 ? -1 : fv->b.map->map[i];
    SplineChar* sc = cc->m_sharedmenu_funcs.getActiveSplineChar(cc);

    for ( mi = mi->sub; mi->ti.text!=NULL || mi->ti.line ; ++mi ) {
	switch ( mi->mid ) {
	  case SMID_ShowDependentRefs:
	      mi->ti.disabled = !sc || sc->dependents == NULL;
	  break;
	  case SMID_RplRef:
	  case SMID_CorrectRefs:
	      mi->ti.disabled = !sc || fv->b.cidmaster!=NULL || fv->b.sf->multilayer;
	      break;
	  case SMID_UnlinkRef:
	      mi->ti.disabled = isMetricsView || !sc;
	      break;
	  
	}
    }
    
}


     
GMenuItem2 sharedmenu_glyph[] = {
    { { UN_("_Glyph Info..."), GI("elementglyphinfo.png"), MENUBODYC_DEFAULT, 'I' }, H_("Glyph Info...|No Shortcut"),
                               NULL, NULL, sm_dialogCharInfo, SMID_CharInfo },
    { { UN_("Set _Color"),     NULL, MENUBODYC_DEFAULT, '\0' }, H_("Set Color|No Shortcut"),
                               scollist, scollistcheck, NULL, SMID_SetColor },
    
    MENUITEM_LINE

    { { UN_("Check Self-Intersection"), NULL, MENUBODYD_DEFAULT, 'o' }, H_("Clockwise|No Shortcut"),
                                        NULL, NULL, CVMenuCheckSelf, SMID_CheckSelf },
    { { UN_("Glyph Self-Intersects"),   NULL, MENUBODYD_DEFAULT, 'o' }, H_("Clockwise|No Shortcut"),
                                        NULL, NULL, CVMenuGlyphSelfIntersects, SMID_GlyphSelfIntersects },
    { { UN_("A_dd..."),                 NULL, MENUBODYC_DEFAULT, 'C' }, H_("Add...|No Shortcut"),
                                        NULL, NULL, FVMenuAddUnencoded, SMID_AddUnencoded },
    { { UN_("D_etach"),                 NULL, MENUBODYC_DEFAULT, 'C' }, H_("Detach|No Shortcut"),
                                        NULL, NULL, FVMenuDetachGlyphs, SMID_DetachGlyphs },
    { { UN_("R_emo_ve"),                NULL, MENUBODYC_DEFAULT, 'C' }, H_("Remove...|No Shortcut"),
                                        NULL, NULL, FVMenuDetachAndRemoveGlyphs, SMID_DetachAndRemoveGlyphs },
    { { UN_("Remove _Unused Slots"),    NULL, MENUBODYC_DEFAULT, 'C' }, H_("Remove Unused Slots|No Shortcut"),
                                        NULL, NULL, FVMenuRemoveUnused, SMID_RemoveUnused },
    { { UN_("Glyph Names"),             NULL, MENUBODYC_DEFAULT, 'H' }, H_("Hints|No Shortcut"),
                                        glyphnameslist, glyphnameslistcheck, NULL, 0 },
    { { UN_("Mass Glyph _Rename"),      GI("elementrenameglyph.png"), MENUBODYC_DEFAULT, '\0' }, H_("Mass Glyph Rename|No Shortcut"),
                                        NULL, NULL, FVMenuMassRename, SMID_MassRename },

    MENUITEM_LINE
    
    { { UN_("H_ints"),                  NULL, MENUBODYC_DEFAULT, 'H' }, H_("Hints|No Shortcut"),
                                        htlist, htlistcheck, NULL, SMID_HintsMenu },

    { { UN_("_Horizontal Baselines..."), GI("elementhbaselines.png"), MENUBODYC_DEFAULT, '\0' }, H_("Horizontal Baselines...|No Shortcut"),
                                         NULL, NULL, FVMenuBaseHoriz, 0 },
    { { UN_("_Vertical Baselines..."),   GI("elementvbaselines.png"), MENUBODYC_DEFAULT, '\0' }, H_("Vertical Baselines...|No Shortcut"),
                                         NULL, NULL, FVMenuBaseVert, 0 },
    { { UN_("_Justification..."),        GI("menuempty.png"), MENUBODYC_DEFAULT, '\0' }, H_("Justification...|No Shortcut"),
                                         NULL, NULL, FVMenuJustify, 0 },
    MENUITEM_LINE
    
    { { UN_("_Kern Pairs"),              NULL, MENUBODYC_DEFAULT, 'K' }, H_("Kern Pairs|No Shortcut"),
                                         NULL, NULL, sm_dialogKernPairs, SMID_KernPairs },
    { { UN_("_Anchored Pairs"),          NULL, MENUBODYC_DEFAULT, 'A' }, H_("Anchored Pairs|No Shortcut"),
                                         NULL, NULL, CVMenuAnchorPairs, SMID_AnchorPairs },

    { { UN_("_Anchor Control..."),       NULL, MENUBODYC_DEFAULT, 'V' }, H_("Anchor Control...|No Shortcut"),
                                         ap2list, ap2listbuild, NULL, SMID_AnchorControl },
    { { UN_("Anchor _Glyph at Point"),   NULL, MENUBODYC_DEFAULT, 'A' }, H_("Anchor Glyph at Point|No Shortcut"),
                                         aplist, aplistcheck, NULL, SMID_AnchorGlyph },
    { { UN_("_Ligatures..."),            NULL, MENUBODYC_DEFAULT, 'L' }, H_("Ligatures|No Shortcut"),
                                         NULL, NULL, sm_dialogLigatures, SMID_Ligatures },

    MENUITEM_LINE
    
    { { UN_("S_ame Glyph As"),       GI("elementrenameglyph.png"), COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 1, 1, 0, 0, 0, 0, 1, 1, 0, '\0' }, H_("Same Glyph As|No Shortcut"),
                                     NULL, NULL, FVMenuMassRename, SMID_MassRename },
    { { UN_("B_uild Glyph"),         GI("elementbuildaccent.png"), MENUBODYC_DEFAULT, 'u' }, H_("Build|No Shortcut"),
                                     balist, balistcheck, NULL, SMID_BuildAccent },

    MENUITEM_LINE
    { { UN_("References"),             NULL, MENUBODYC_DEFAULT, 'H' }, H_("Hints|No Shortcut"),
                                       referenceslist, referenceslistcheck, NULL, SMID_ReferencesMenu },
    { { UN_("S_how Substitutions..."), NULL, MENUBODYC_DEFAULT, 'B' }, H_("Substitutions...|No Shortcut"),
                                       NULL, NULL, CVMenuShowDependentSubs, SMID_ShowDependentSubs },

    MENUITEM_LINE
    
    { { UN_("Insert Text Outlines..."), NULL, MENUBODYC_DEFAULT, 'D' }, H_("Insert Text Outlines|No Shortcut"),
                                        NULL, NULL, CVMenuInsertText, SMID_InsertText },

    GMENUITEM2_EMPTY
};

void sharedmenu_glyph_check(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e))
{
    MENU_CHECK_VARIABLES;
    SplineChar* sc = cc->m_sharedmenu_funcs.getActiveSplineChar(cc);
    int gid=-1;
    SplineFont *sf = fv->b.sf;
    EncMap *map = fv->b.map;
    int anychars = FVAnyCharSelected(fv);
    int anyglyphs = false;
    int pos = FVAnyCharSelected(fv);
    int anykerns=0;
    int anyligs=0;
    int i=0;
    PST *pst=0;

    if ( sf->kerns ) anykerns=true;
    for ( i=0; i<fv->b.map->enccount; ++i ) if ( (gid = fv->b.map->map[i])!=-1 && sf->glyphs[gid]!=NULL ) {
	    for ( pst=sf->glyphs[gid]->possub; pst!=NULL; pst=pst->next ) {
		if ( pst->type==pst_ligature ) {
		    anyligs = true;
		    if ( anykerns )
			break;
		}
	    }
	    if ( sf->glyphs[gid]->kerns!=NULL ) {
		anykerns = true;
		if ( anyligs )
		    break;
	    }
	}

    i = FVAnyCharSelected(fv);
    if( fv )
	gid = i<0 ? -1 : fv->b.map->map[i];
    
    for ( i=map->enccount-1; i>=0 ; --i )
	if ( fv->b.selected[i] && (gid=map->map[i])!=-1 )
	    anyglyphs = true;

    
    for ( mi = mi->sub; mi && (mi->ti.text!=NULL || mi->ti.line); ++mi ) {
	switch ( mi->mid ) {
	  case SMID_HintsMenu:
	      mi->ti.disabled = isMetricsView;
	      break;
	  case SMID_CharInfo:
	      mi->ti.disabled = !sc;
	      break;
	  case SMID_AnchorControl:
	      if( !isCharView )
		  mi->ti.disabled = 1;
	      else
	      {
		  SplineChar* sc = cv->b.sc;
		  mi->ti.disabled = sc->anchor==NULL;
	      }
	      break;
	  case SMID_AnchorGlyph:
	      if( !isCharView )
		  mi->ti.disabled = 1;
	      else
	      {
		  SplineChar* sc = cv->b.sc;
		  if ( cv && cv->apmine!=NULL )
		      mi->ti.disabled = false;
		  else
		      mi->ti.disabled = sc->anchor==NULL;
	      }
	      break;
	  case SMID_KernPairs:
	      mi->ti.disabled = !anykerns;
	  break;
          case SMID_SetColor:
	      if( !isFontView )
		  mi->ti.disabled = 1;
	      else
		  mi->ti.disabled = (pos == -1 );
	      break;
          case SMID_InsertText:
	      mi->ti.disabled = !isCharView;
	      break;
	  case SMID_ShowDependentSubs:
	      if( !isCharView )
		  mi->ti.disabled = 1;
	      else
	      {
		  mi->ti.disabled = gid<0 || fv->b.sf->glyphs[gid]==NULL ||
		      !SCUsedBySubs(fv->b.sf->glyphs[gid]);
	      }
	      break;
    
	  case SMID_AnchorPairs:
	      if( !isCharView )
		  mi->ti.disabled = 1;
	      else
		  mi->ti.disabled = sf->anchor==NULL;
	      break;
    
	  case SMID_CheckSelf:
	  case SMID_GlyphSelfIntersects:
	      mi->ti.disabled = !isCharView;
	      break;
	  case SMID_DetachGlyphs:
	  case SMID_DetachAndRemoveGlyphs:
	      if( !isFontView )
		  mi->ti.disabled = true;
	      else 
		  mi->ti.disabled = !anyglyphs;
	      break;
	  case SMID_RemoveUnused:
	      if( !isFontView )
		  mi->ti.disabled = true;
	      else
	      {
		  gid = map->enccount>0 ? map->map[map->enccount-1] : -1;
		  mi->ti.disabled = gid!=-1 && SCWorthOutputting(sf->glyphs[gid]);
	      }
	      break;
	  case SMID_MassRename:
	      if( !isFontView )
		  mi->ti.disabled = true;
	      else 
		  mi->ti.disabled = anychars==-1;
	      break;
	}
    }
    
    
    
}



/*********************************************************************************/
/*********************************************************************************/
/*********************************************************************************/
/*********************************************************************************/
/*********************************************************************************/

static GMenuItem2 sharedmenu_point_spiro[] = {
    { { UN_("G4 _Curve"), (GImage *) "pointscurve.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'C' },
                          H_("G4 Curve|No Shortcut"), NULL, NULL, CVMenuPointType, SMID_SpiroG4 },
    { { UN_("_G2 Curve"), (GImage *) "pointsG2curve.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'o' },
                          H_("G2 Curve|No Shortcut"), NULL, NULL, CVMenuPointType, SMID_SpiroG2 },
    { { UN_("C_orner"),   (GImage *) "pointscorner.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'o' },
                          H_("Corner|No Shortcut"), NULL, NULL, CVMenuPointType, SMID_SpiroCorner },
    { { UN_("_Left Constraint"), (GImage *) "pointsspiroprev.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'T' },
                          H_("Prev Constraint|No Shortcut"), NULL, NULL, CVMenuPointType, SMID_SpiroLeft },
    { { UN_("_Right Constraint"), (GImage *) "pointsspironext.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'T' },
                          H_("Next Constraint|No Shortcut"), NULL, NULL, CVMenuPointType, SMID_SpiroRight },
    MENUITEM_LINE
/* GT: Make this (selected) point the first point in the glyph */
    { { UN_("_Make First"),   (GImage *) "menuempty.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'M' },
                              H_("Make First|No Shortcut"), NULL, NULL, CVMenuSpiroMakeFirst, SMID_SpiroMakeFirst },
    MENUITEM_LINE
    { { UN_("_Add Anchor"),   (GImage *) "pointsaddanchor.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'A' },
                              H_("Add Anchor|No Shortcut"), NULL, NULL, CVMenuAddAnchor, SMID_AddAnchor },
    MENUITEM_LINE
    { { UN_("_Name Point"),   (GImage *) "pointsnamepoint.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'M' },
                              H_("Name Point|No Shortcut"), NULL, NULL, CVMenuNamePoint, SMID_NamePoint },
    { { UN_("_Name Contour"), (GImage *) "pointsnamecontour.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'M' },
                              H_("Name Contour|No Shortcut"), NULL, NULL, CVMenuNameContour, SMID_NameContour },
    MENUITEM_LINE
    GMENUITEM2_EMPTY
};

GMenuItem2 sharedmenu_point[] = {
    { { UN_("P_oint Info..."), GI("elementgetinfo.png"), MENUBODYC_DEFAULT, 'I' }, H_("Get Info...|No Shortcut"),
                               NULL, NULL, CVMenuGetInfo, SMID_GetInfo },
    MENUITEM_LINE
    { { UN_("C_urve"),    GI("pointscurve.png"), MENUBODYD_DEFAULT, 'C' }, H_("Curve|No Shortcut"),
                          NULL, NULL, CVMenuPointType, SMID_Curve },
    { { UN_("H_VCurve"),  GI("pointshvcurve.png"), MENUBODYD_DEFAULT, 'o' }, H_("HVCurve|No Shortcut"),
                          NULL, NULL, CVMenuPointType, SMID_HVCurve },
    { { UN_("C_orner"),   GI("pointscorner.png"), MENUBODYD_DEFAULT, 'o' }, H_("Corner|No Shortcut"),
                          NULL, NULL, CVMenuPointType, SMID_Corner },
    { { UN_("T_angent"),  GI("pointstangent.png"), MENUBODYD_DEFAULT, 'T' }, H_("Tangent|No Shortcut"),
                          NULL, NULL, CVMenuPointType, SMID_Tangent },
    MENUITEM_LINE
    { { UN_("I_nsert Point"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 1, 1, 0, 0, 0, 0, 1, 1, 0, 'M' },
                              H_("Insert Point|No Shortcut"), NULL, NULL, NULL, 0 },
    { { UN_("M_ake First"),   NULL, MENUBODYC_DEFAULT, 'M' }, H_("Make First|No Shortcut"),
                              NULL, NULL, CVMenuMakeFirst , SMID_MakeFirst },
    MENUITEM_LINE
    { { UN_("A_dd Anchor"),   GI("pointsaddanchor.png"), MENUBODYC_DEFAULT, 'A' }, H_("Add Anchor|No Shortcut"),
                              NULL, NULL, CVMenuAddAnchor, SMID_AddAnchor },
    MENUITEM_LINE
    { { UN_("T_o Int"),       GI("elementround.png"), MENUBODYC_DEFAULT, 'I' }, H_("To Int|No Shortcut"),
                              NULL, NULL, CVMenuRound2Int, SMID_Round },
    { { UN_("T_o Hundredths"),NULL, MENUBODYC_DEFAULT, 'I' }, H_("To Hundredths|No Shortcut"),
                              NULL, NULL, CVMenuRound2Hundredths, 0 },
    { { UN_("C_luster"),      NULL, MENUBODYC_DEFAULT, 'I' }, H_("Cluster|No Shortcut"),
                              NULL, NULL, CVMenuCluster, SMID_RoundToCluster },
    MENUITEM_LINE
    { { UN_("A_lign Points"), NULL, MENUBODYC_DEFAULT, 'A' }, H_("Align Points|No Shortcut"),
                              NULL, NULL, CVMenuConstrain, SMID_Average },
    { { UN_("S_pace Points"), NULL, MENUBODYC_DEFAULT, 'S' }, H_("Space Points|No Shortcut"),
                              NULL, NULL, CVMenuConstrain, SMID_SpacePts },
    { { UN_("S_pace Regions..."), NULL, MENUBODYC_DEFAULT, 'R' }, H_("Space Regions...|No Shortcut"),
                                  NULL, NULL, CVMenuConstrain, SMID_SpaceRegion },
    MENUITEM_LINE
    { { UN_("T_ools"),  NULL, MENUBODYC_DEFAULT, 'M' }, NULL,
                        cvspirotoollist, cvtoollist_check, NULL, SMID_Tools },
    GMENUITEM2_EMPTY
};

void sharedmenu_point_check(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e))
{
    MENU_CHECK_VARIABLES;
    
    if( !isCharView )
    {
	setAllDisabled(mi,1);
	return;
    }

    int type = -2, cnt=0, ccp_cnt=0, spline_selected=0;
    int spirotype = -2, opencnt=0, spirocnt=0;
    SplinePointList *spl, *sel=NULL, *onlysel=NULL;
    Spline *spline, *first;
    SplinePoint *selpt=NULL;
    int notimplicit = -1;
    int acceptable = -1;
    uint16 junk;
    int i;
//    GMenuItem2 used_point = CVInSpiro(cv) ? sharedmenu_spiropoint : sharedmenu_point;
    GMenuItem2* ptlist = CVInSpiro(cv) ? sharedmenu_point_spiro : sharedmenu_point;
    AnchorPoint *ap;
    spiro_cp *cp;
    
    if ( cv->showing_spiro_pt_menu != (cv->b.sc->inspiro && hasspiro())) {
	GMenuItemArrayFree(mi->sub);
	mi->sub = GMenuItem2ArrayCopy(ptlist,&junk);
	cv->showing_spiro_pt_menu = cv->b.sc->inspiro && hasspiro();
    }
    for ( spl = cv->b.layerheads[cv->b.drawmode]->splines; spl!=NULL; spl = spl->next ) {
	first = NULL;
	if ( spl->first->selected ) {
	    sel = spl;
	    if ( onlysel==NULL || onlysel==spl ) onlysel = spl; else onlysel = (SplineSet *) (-1);
	    selpt = spl->first; ++cnt;
	    if ( type==-2 ) type = spl->first->pointtype;
	    else if ( type!=spl->first->pointtype ) type = -1;
	    if ( !spl->first->nonextcp && !spl->first->noprevcp && spl->first->prev!=NULL )
		++ccp_cnt;
	    if ( notimplicit==-1 ) notimplicit = spl->first->dontinterpolate;
	    else if ( notimplicit!=spl->first->dontinterpolate ) notimplicit = -2;
	}
	for ( spline=spl->first->next; spline!=NULL && spline!=first; spline = spline->to->next ) {
	    if ( spline->to->selected ) {
		if ( type==-2 ) type = spline->to->pointtype;
		else if ( type!=spline->to->pointtype ) type = -1;
		selpt = spline->to;
		if ( onlysel==NULL || onlysel==spl ) onlysel = spl; else onlysel = (SplineSet *) (-1);
		sel = spl; ++cnt;
		if ( !spline->to->nonextcp && !spline->to->noprevcp && spline->to->next!=NULL )
		    ++ccp_cnt;
		if ( notimplicit==-1 ) notimplicit = spline->to->dontinterpolate;
		else if ( notimplicit!=spline->to->dontinterpolate ) notimplicit = -2;
		if ( spline->from->selected )
		    ++spline_selected;
	    }
	    if ( spline->to->selected && spline->from->selected ) {
		if ( acceptable==-1 )
		    acceptable = spline->acceptableextrema;
		else if ( acceptable!=spline->acceptableextrema )
		    acceptable = -2;
	    }
	    if ( first == NULL ) first = spline;
	}
	for ( i=0; i<spl->spiro_cnt-1; ++i ) {
	    if ( SPIRO_SELECTED(&spl->spiros[i])) {
		int ty = spl->spiros[i].ty&0x7f;
		++spirocnt;
		if ( ty==SPIRO_OPEN_CONTOUR )
		    ++opencnt;
		else if ( spirotype==-2 )
		    spirotype = ty;
		else if ( spirotype!=ty )
		    spirotype = -1;
		if ( onlysel==NULL || onlysel==spl ) onlysel = spl; else onlysel = (SplineSet *) (-1);
	    }
	}
    }

    for ( mi = mi->sub; mi && (mi->ti.text!=NULL || mi->ti.line); ++mi ) {
	switch ( mi->mid ) {
	  case SMID_GetInfo:
	  {
	      SplinePoint *sp; SplineSet *spl; RefChar *ref; ImageList *img;
	      mi->ti.disabled = !CVOneThingSel(cv,&sp,&spl,&ref,&img,&ap,&cp);
	  }
	  break;
	  case SMID_Corner:
	      mi->ti.disabled = type==-2;
	      mi->ti.checked = type==pt_corner;
	      break;
	  case SMID_Tangent:
	      mi->ti.disabled = type==-2;
	      mi->ti.checked = type==pt_tangent;
	      break;
	  case SMID_Curve:
	      mi->ti.disabled = type==-2;
	      mi->ti.checked = type==pt_curve;
	      break;
	  case SMID_HVCurve:
	      mi->ti.disabled = type==-2;
	      mi->ti.checked = type==pt_hvcurve;
	      break;
	  case SMID_SpiroG4:
	      mi->ti.disabled = spirotype==-2;
	      mi->ti.checked = spirotype==SPIRO_G4;
	      break;
	  case SMID_SpiroG2:
	      mi->ti.disabled = spirotype==-2;
	      mi->ti.checked = spirotype==SPIRO_G2;
	      break;
	  case SMID_SpiroCorner:
	      mi->ti.disabled = spirotype==-2;
	      mi->ti.checked = spirotype==SPIRO_CORNER;
	      break;
	  case SMID_SpiroLeft:
	      mi->ti.disabled = spirotype==-2;
	      mi->ti.checked = spirotype==SPIRO_LEFT;
	      break;
	  case SMID_SpiroRight:
	      mi->ti.disabled = spirotype==-2;
	      mi->ti.checked = spirotype==SPIRO_RIGHT;
	      break;
	  case SMID_MakeFirst:
	      mi->ti.disabled = cnt!=1 || sel->first->prev==NULL || sel->first==selpt;
	      break;
	  case SMID_SpiroMakeFirst:
	      mi->ti.disabled = opencnt!=0 || spirocnt!=1;
	      break;
	  case SMID_MakeLine: case SMID_MakeArc:
	      mi->ti.disabled = cnt<2;
	      break;
	  case SMID_AcceptableExtrema:
	      mi->ti.disabled = acceptable<0;
	      mi->ti.checked = acceptable==1;
	      break;
	  case SMID_NamePoint:
	      mi->ti.disabled = onlysel==NULL || onlysel == (SplineSet *) -1;
	      break;
	  case SMID_NameContour:
	      mi->ti.disabled = onlysel==NULL || onlysel == (SplineSet *) -1;
	      break;
	  case SMID_ClipPath:
	      mi->ti.disabled = !cv->b.sc->parent->multilayer;
	      break;
	  case SMID_InsertPtOnSplineAt:
	      mi->ti.disabled = spline_selected!=1;
	      break;
	  case SMID_CenterCP:
	      mi->ti.disabled = ccp_cnt==0;
	      break;
	  case SMID_ImplicitPt:
	      mi->ti.disabled = !cv->b.layerheads[cv->b.drawmode]->order2;
	      mi->ti.checked = notimplicit==0;
	      break;
	  case SMID_NoImplicitPt:
	      mi->ti.disabled = !cv->b.layerheads[cv->b.drawmode]->order2;
	      mi->ti.checked = notimplicit==1;
	      break;
	  case SMID_AddAnchor:
	      mi->ti.disabled = cv->b.container!=NULL;
	      break;
	}
    }    
}

/*********************************************************************************/
/*********************************************************************************/
/*********************************************************************************/
/*********************************************************************************/
/*********************************************************************************/



GMenuItem2 sharedmenu_collab[] = {
#ifdef BUILD_COLLAB
    { { (unichar_t *) N_("_Start Session..."), NULL, MENUBODYC_DEFAULT, 'I' }, H_("Start Session...|No Shortcut"), NULL, NULL, FVMenuCollabStart, SMID_CollabStart },
    { { (unichar_t *) N_("_Connect to Session..."), NULL, MENUBODYC_DEFAULT, 'I' }, H_("Connect to Session...|No Shortcut"), NULL, NULL, FVMenuCollabConnect, SMID_CollabConnect },
    { { (unichar_t *) N_("_Connect to Session (ip:port)..."), NULL, MENUBODYC_DEFAULT, 'I' }, H_("Connect to Session (ip:port)...|No Shortcut"), NULL, NULL, FVMenuCollabConnectToExplicitAddress, SMID_CollabConnectToExplicitAddress },
    { { (unichar_t *) N_("_Disconnect"), NULL, MENUBODYC_DEFAULT, 'I' }, H_("Disconnect|No Shortcut"), NULL, NULL, FVMenuCollabDisconnect, SMID_CollabDisconnect },
    GMENUITEM2_LINE,
    { { (unichar_t *) N_("Close local server"), NULL, MENUBODYC_DEFAULT, 'I' }, H_("Close local server|No Shortcut"), NULL, NULL, FVMenuCollabCloseLocalServer, SMID_CollabCloseLocalServer },
#endif
    GMENUITEM2_EMPTY,
};

void sharedmenu_collab_check(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e))
{
#ifdef BUILD_COLLAB
    MENU_CHECK_VARIABLES;

    for ( mi = mi->sub; mi->ti.text!=NULL || mi->ti.line ; ++mi )
    {
	switch ( mi->mid )
	{
	case SMID_CollabDisconnect:
	{
	    enum collabState_t st = collabclient_getState( &fv->b );
	    mi->ti.disabled = ( st < cs_server );
	    break;
	}
	case SMID_CollabCloseLocalServer:
	    printf("can close local server: %d\n", collabclient_haveLocalServer() );
	    mi->ti.disabled = !collabclient_haveLocalServer();
	    break;
	}
    }
#endif
}

/*********************************************************************************/
/*********************************************************************************/
/*********************************************************************************/
/*********************************************************************************/
/*********************************************************************************/

MAKETRAMP(openWindowGlyph);
MAKETRAMP(openWindowBitmap);
MAKETRAMP(openWindowMetrics);

GMenuItem2 sharedmenu_window[] = {
    { { UN_("New G_lyph Window"),   MENUBODY_DEFAULT, 'u' }, H_("New Glyph Window|No Shortcut"),   NULL, NULL, sm_openWindowGlyph, 0 },
    { { UN_("New _Bitmap Window"),  MENUBODY_DEFAULT, 'B' }, H_("New Bitmap Window|No Shortcut"),  NULL, NULL, sm_openWindowBitmap, SMID_OpenBitmap },
    { { UN_("New _Metrics Window"), MENUBODY_DEFAULT, 'M' }, H_("New Metrics Window|No Shortcut"), NULL, NULL, sm_openWindowMetrics, 0 },
    MENUITEM_LINE
    { { UN_("Warnings"), MENUBODY_DEFAULT, 'M' }, H_("Warnings|No Shortcut"), NULL, NULL, _MenuWarnings, SMID_Warnings },
    MENUITEM_LINE
    GMENUITEM2_EMPTY
};


/*********************************************************************************/
/*********************************************************************************/
/*********************************************************************************/
/*********************************************************************************/
/*********************************************************************************/

void sharedmenu_update_menus_at_init( CommonView* cc, GMenuItem2* mblist, int mblist_extensions_idx )
{
    FontView* fv = tryObtainCastFontView( cc );
    CharView* cv = tryObtainCastCharView( cc );

    GMenuItem2 *pymenu = fvpy_menu;
    if( cv )
    {
	pymenu = cvpy_menu;
    }
    
#ifndef _NO_PYTHON
    if ( pymenu!=NULL )
	mblist[mblist_extensions_idx].ti.disabled = false;
    mblist[mblist_extensions_idx].sub = pymenu;
#endif
}


