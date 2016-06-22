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
#ifndef _FF_SHARED_MENU_H
#define _FF_SHARED_MENU_H

#include <fontforge-config.h>

/**
 * Some windows might use a local MID_Foo to describe their menu. We
 * use SMID_Foo to indicate that it is a shared menu id so that both
 * identifiers can be in scope at once and not cause any issues.
 */
enum {
    SMID_24 = 2001,
    SMID_36,
    SMID_48,
    SMID_72,
    SMID_96,
    SMID_128,
    SMID_AntiAlias,
    SMID_Next,
    SMID_Prev,
    SMID_NextDef,
    SMID_PrevDef,
    SMID_ShowHMetrics,
    SMID_ShowVMetrics,
    SMID_Ligatures,
    SMID_KernPairs,
    SMID_AnchorPairs,
    SMID_FitToBbox,
    SMID_DisplaySubs,
    SMID_32x8,
    SMID_16x4,
    SMID_8x2,
    SMID_BitmapMag,
    SMID_Layers,
    SMID_FontInfo,
    SMID_CharInfo,
    SMID_Transform,
    SMID_Stroke,
    SMID_RmOverlap,
    SMID_Simplify,
    SMID_Correct,
    SMID_BuildAccent,
    SMID_AvailBitmaps,
    SMID_RegenBitmaps,
    SMID_Autotrace,
    SMID_Round,
    SMID_MergeFonts,
    SMID_InterpolateFonts,
    SMID_FindProblems,
    SMID_Embolden,
    SMID_Condense,
    SMID_ShowDependentRefs,
    SMID_AddExtrema,
    SMID_CleanupGlyph,
    SMID_TilePath,
    SMID_BuildComposite,
    SMID_NLTransform,
    SMID_Intersection,
    SMID_FindInter,
    SMID_Styles,
    SMID_SimplifyMore,
    SMID_ShowDependentSubs,
    SMID_DefaultATT,
    SMID_POV,
    SMID_BuildDuplicates,
    SMID_StrikeInfo,
    SMID_FontCompare,
    SMID_CanonicalStart,
    SMID_CanonicalContours,
    SMID_RemoveBitmaps,
    SMID_Validate,
    SMID_MassRename,
    SMID_Italic,
    SMID_SmallCaps,
    SMID_SubSup,
    SMID_ChangeXHeight,
    SMID_ChangeGlyph,
    SMID_SetColor,
    SMID_SetExtremumBound,
    SMID_Center,
    SMID_Thirds,
    SMID_SetWidth,
    SMID_SetLBearing,
    SMID_SetRBearing,
    SMID_SetVWidth,
    SMID_RmHKern,
    SMID_RmVKern,
    SMID_VKernByClass,
    SMID_VKernFromH,
    SMID_SetBearings,
    SMID_AutoHint,
    SMID_ClearHints,
    SMID_ClearWidthMD,
    SMID_AutoInstr,
    SMID_EditInstructions,
    SMID_Editfpgm,
    SMID_Editprep,
    SMID_ClearInstrs,
    SMID_HStemHist,
    SMID_VStemHist,
    SMID_BlueValuesHist,
    SMID_Editcvt,
    SMID_HintSubsPt,
    SMID_AutoCounter,
    SMID_DontAutoHint,
    SMID_RmInstrTables,
    SMID_Editmaxp,
    SMID_Deltas,
    SMID_OpenBitmap,
    SMID_OpenOutline,
    SMID_Revert,
    SMID_Recent,
    SMID_Print,
    SMID_ScriptMenu,
    SMID_RevertGlyph,
    SMID_RevertToBackup,
    SMID_GenerateTTC,
    SMID_OpenMetrics,
    SMID_ClearSpecialData,
    SMID_Cut,
    SMID_Copy,
    SMID_Paste,
    SMID_Delete,
    SMID_Clear,
    SMID_SelAll,
    SMID_CopyRef,
    SMID_UnlinkRef,
    SMID_Undo,
    SMID_Redo,
    SMID_CopyWidth,
    SMID_UndoFontLevel,
    SMID_AllFonts,
    SMID_DisplayedFont,
    SMID_CharName,
    SMID_RemoveUndoes,
    SMID_CopyFgToBg,
    SMID_ClearBackground,
    SMID_CopyLBearing,
    SMID_CopyRBearing,
    SMID_CopyVWidth,
    SMID_Join,
    SMID_PasteInto,
    SMID_SameGlyphAs,
    SMID_RplRef,
    SMID_PasteAfter,
    SMID_TTFInstr,
    SMID_CopyLookupData,
    SMID_CopyL2L,
    SMID_CorrectRefs,
    SMID_Convert2CID,
    SMID_Flatten,
    SMID_InsertFont,
    SMID_InsertBlank,
    SMID_CIDFontInfo,
    SMID_RemoveFromCID,
    SMID_ConvertByCMap,
    SMID_FlattenByCMap,
    SMID_ChangeSupplement,
    SMID_Reencode,
    SMID_ForceReencode,
    SMID_AddUnencoded,
    SMID_RemoveUnused,
    SMID_DetachGlyphs,
    SMID_DetachAndRemoveGlyphs,
    SMID_LoadEncoding,
    SMID_MakeFromFont,
    SMID_RemoveEncoding,
    SMID_DisplayByGroups,
    SMID_Compact,
    SMID_SaveNamelist,
    SMID_RenameGlyphs,
    SMID_NameGlyphs,
    SMID_HideNoGlyphSlots,
    SMID_CreateMM,
    SMID_MMInfo,
    SMID_MMValid,
    SMID_ChangeMMBlend,
    SMID_BlendToNew,
    SMID_ModifyComposition,
    SMID_BuildSyllables,
    SMID_CollabStart,
    SMID_CollabConnect,
    SMID_CollabDisconnect,
    SMID_CollabCloseLocalServer,
    SMID_CollabConnectToExplicitAddress,
    SMID_AddWordList,
    SMID_WordListNextLine,
    SMID_WordListPrevLine,
    SMID_CloseTab,

    SMID_FirstPt,
    SMID_FirstPtNextCont,
    SMID_NextPt,
    SMID_PrevPt,
    SMID_NextCP,
    SMID_PrevCP,
    SMID_Contours,
    SMID_SelPointAt,
    SMID_SelectAllPoints,
    SMID_SelectOpenContours,
    SMID_SelectAnchors,
    SMID_SelectWidth,
    SMID_SelectVWidth,
    SMID_SelectHM,

    SMID_GlyphLabel,
    SMID_scaleViewToFit,
    SMID_scaleViewOut,
    SMID_scaleViewIn,


    SMID_PtsNone,
    SMID_PtsTrue,
    SMID_PtsPost,
    SMID_PtsSVG,
    SMID_PtsPos,
    SMID_Former,

    SMID_ShowGridFit,
    SMID_ShowGridFitLiveUpdate,
    SMID_Bigger,
    SMID_Smaller,
    SMID_GridFitAA,
    SMID_GridFitOff,

    SMID_GlyphElements,
    SMID_HidePoints,
    SMID_HideControlPoints,
    SMID_ShowCPInfo,
    SMID_ShowAnchors,
    SMID_MarkExtrema,
    SMID_MarkPointsOfInflection,
    SMID_DraggingComparisonOutline,
    SMID_Fill,
    SMID_Preview,
    SMID_ShowSideBearings,
    SMID_ShowRefNames,
    SMID_ShowAlmostHV,
    SMID_ShowAlmostHVCurves,
    SMID_DefineAlmost,
    SMID_ShowHHints,
    SMID_ShowVHints,
    SMID_ShowDHints,
    SMID_ShowBlueValues,
    SMID_ShowFamilyBlues,
    SMID_ShowTabs,
    SMID_HideRulers,
    SMID_Tools,
    SMID_DockPalettes,
    SMID_GlyphWindow,
    SMID_Exclude,
    SMID_NameContour,
    SMID_MakeLine,
    SMID_MakeArc,
    SMID_MakeParallel,
    SMID_Clockwise,
    SMID_Counter,
    SMID_ReverseDir,
    SMID_AcceptableExtrema,

    SMID_Debug,
    SMID_ClearHStem,
    SMID_ClearVStem,
    SMID_ClearDStem,
    SMID_ClearInstr,
    SMID_AddHHint,
    SMID_AddVHint,
    SMID_AddDHint,
    SMID_ReviewHints,
    SMID_CreateHHint,
    SMID_CreateVHint,
    SMID_InsertText,
    SMID_AnchorGlyph,
    SMID_AnchorControl,
    SMID_GlyphSelfIntersects,
    SMID_CheckSelf,
    SMID_GenerateUFO,
    SMID_GenerateGraphic,
    SMID_CopyGridFit,
    SMID_FindReplace,

    SMID_GetInfo,
    SMID_Curve,
    SMID_HVCurve,
    SMID_Corner,
    SMID_Tangent,
    SMID_MakeFirst,
    SMID_AddAnchor,
    SMID_RoundToCluster,
    SMID_Average,
    SMID_SpacePts,
    SMID_SpaceRegion,

    SMID_SpiroG4,
    SMID_SpiroG2,
    SMID_SpiroCorner,
    SMID_SpiroLeft,
    SMID_SpiroRight,
    SMID_SpiroMakeFirst,
    SMID_NamePoint,
    SMID_ClipPath,
    SMID_InsertPtOnSplineAt,
    SMID_CenterCP,
    SMID_ImplicitPt,
    SMID_NoImplicitPt,
    SMID_ReplaceChar,    

    SMID_InsertCharA,
    SMID_ShowGrid,
    SMID_HideGrid,
    SMID_PartialGrid,
    SMID_HideGridWhenMoving,
    SMID_KernOnly,
    SMID_WidthOnly,
    SMID_ZoomIn,
    SMID_ZoomOut,

    SMID_Fit,
    SMID_First,
    SMID_Earlier,
    SMID_Last,
    SMID_Later,
    SMID_MMAll,
    SMID_MMNone,
    SMID_ShowDebugChanges,

    SMID_SelectAll,
    SMID_SelectInvert,
    SMID_SelectNone,
    
    SMID_Warnings = 3000
};

#include "gdraw.h"
#include "views.h"


GMenuItem2 sharedmenu_exportlist[10];
GMenuItem2 sharedmenu_file[100];
void sharedmenu_file_check(GWindow, struct gmenuitem*, GEvent *);

GMenuItem2 sharedmenu_edit[100];
void sharedmenu_edit_check(GWindow, struct gmenuitem*, GEvent*);

GMenuItem2 sharedmenu_font[100];
void sharedmenu_font_check(GWindow, struct gmenuitem*, GEvent*);

GMenuItem2 sharedmenu_metrics[100];
void sharedmenu_metrics_check(GWindow, struct gmenuitem*, GEvent*);

GMenuItem2 sharedmenu_view[100];
void sharedmenu_view_check(GWindow, struct gmenuitem*, GEvent*);

GMenuItem2 sharedmenu_path[100];
void sharedmenu_path_check(GWindow, struct gmenuitem*, GEvent*);

GMenuItem2 sharedmenu_help[100];
void sharedmenu_help_check(GWindow, struct gmenuitem*, GEvent*);

GMenuItem2 sharedmenu_glyph[100];
void sharedmenu_glyph_check(GWindow, struct gmenuitem*, GEvent*);

GMenuItem2 sharedmenu_point[100];
void sharedmenu_point_check(GWindow, struct gmenuitem*, GEvent*);

GMenuItem2 sharedmenu_collab[100];
void sharedmenu_collab_check(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e));

GMenuItem2 sharedmenu_window[100];

void sharedmenu_update_menus_at_init( CommonView* cc, GMenuItem2* mblist, int mblist_extensions_idx );



#ifdef BUILD_COLLAB
#define SHAREDMENU_COLLAB_LINE \
    { { (unichar_t *) N_("C_ollaborate"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'W' }, \
	    H_("Collaborate|No Shortcut"), sharedmenu_collab, sharedmenu_collab_check, NULL, 0 },
#else
#define SHAREDMENU_COLLAB_LINE
#endif


#define SHAREDMENU_SETVTABLE(VFNAME,FNAME) \
    cv->b.m_commonView.m_sharedmenu_funcs.VFNAME = sm_##FNAME;

#endif
