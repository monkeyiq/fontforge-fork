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
#ifndef _FONTFORGEEXE_FONTVIEW_H
#define _FONTFORGEEXE_FONTVIEW_H

enum merge_type { mt_set=0, mt_merge=4, mt_or=mt_merge, mt_restrict=8, mt_and=12 };
enum merge_type SelMergeType(GEvent *e);

enum glyphlable { gl_glyph, gl_name, gl_unicode, gl_encoding };


void FVMenuSave(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e));
void FVMenuSaveAs(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e));
void FVMenuOpen(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e));
void FVMenuImport(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e));
void FVMenuMergeFonts(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e));
void FVAddWordList(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e));
void FVMenuPrint(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e));
void FVMenuCloseTab(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e));
void FVMenuClose(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e));
void FVMenuExit(GWindow UNUSED(base), struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e));
void FVMenuGenerate(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e));
void FVMenuGenerateTTC(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e));
void FVMenuGenerateFamily(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e));
void FVMenuCopyFgBg(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e));
void FVMenuCopyL2L(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e));
void FVMenuCompareL2L(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e));
void FVMenuPasteInto(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e));
void FVMenuPasteAfter(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e));
void FVMenuDeselectAll(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e));
void FVMenuInvertSelection(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e));
void FVMenuUndoFontLevel(GWindow gw,struct gmenuitem *mi,GEvent *e);
void FVMenuRemoveUndoes(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e));
void FVMenuCopyFrom(GWindow UNUSED(gw), struct gmenuitem *mi, GEvent *UNUSED(e));
void FVMenuCopyRef(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e));
void FVMenuCopyLookupData(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e));
void FVMenuCopyWidth(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e));
void FVMenuClearBackground(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e));
void FVMenuJoin(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e));
void FVMenuUnlinkRef(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e));
int  FVAnyCharSelected(FontView *fv);
void FVMenuGotoChar(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e));




void FVMenuFindProblems(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e));
void FVMenuValidate(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e));
void FVMenuSetExtremumBound(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e));
void FVMenuCompact(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e));
void FVForceEncodingMenuBuild(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e));
void FVMenuAddEncodingName(GWindow UNUSED(gw), struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e));
void FVMenuLoadEncoding(GWindow UNUSED(gw), struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e));
void FVMenuMakeFromFont(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e));
void FVMenuRemoveEncoding(GWindow UNUSED(gw), struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e));
void FVMenuCreateMM(GWindow UNUSED(gw), struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e));
void FVMenuMMInfo(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e));
void FVMenuMMValid(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e));
void FVMenuBlendToNew(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e));
void FVMenuChangeMMBlend(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e));
void FVMenuConvert2CID(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e));
void FVMenuConvertByCMap(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e));
void FVMenuFlatten(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e));
void FVMenuFlattenByCMap(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e));
void FVMenuInsertFont(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e));
void FVMenuInsertBlank(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e));
void FVMenuRemoveFontFromCID(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e));
void FVMenuChangeSupplement(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e));
void FVMenuCIDFontInfo(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e));
void FVMenuShowSubFont(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e));

void FVMenuFontInfo(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e));
void FVMenuInterpFonts(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e));
void FVMenuCompareFonts(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e));
void FVEncodingMenuBuild(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e));
void FVMenuBitmaps(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e));
void FVMenuShowAtt(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e));
void FVMenuMATHInfo(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e));
void FVMenuBDFInfo(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e));
void FVMenuAutoWidth(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e));
void FVMenuGlyphLabel(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e));


void FVMenuChangeChar(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e));
void FVMenuShowBitmap(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e));
void FVMenuCompareL2L(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e));


void FVMenuShowGroup(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e));

void FVMenuAutoHint(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e));
void FVMenuAutoHintSubs(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e));
void FVMenuAutoCounter(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e));
void FVMenuDontAutoHint(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e));
void FVMenuAutoInstr(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e));
void FVMenuEditInstrs(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e));
void FVMenuEditTable(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e));
void FVMenuRmInstrTables(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e));
void FVMenuDeltas(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e));
void FVMenuClearHints(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e));
void FVMenuClearInstrs(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e));

void FVMenuSetColor(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e));
void FVMenuAnchorPairs(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e));
void FVMenuMassRename(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e));
void FVMenuRemoveUnused(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e));
void FVMenuAddUnencoded(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e));
void FVMenuDetachGlyphs(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e));
void FVMenuDetachAndRemoveGlyphs(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e));

void FVMenuGenerateUFO(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e));
void FVMenuGenerateGraphic(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e));
void FVMenuClearSpecialData(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e));
void FVMenuFindRpl(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e));
void FVMenuNameGlyphs(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e));
void FVMenuRenameByNamelist(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e));
void FVMenuMakeNamelist(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e));
void FVMenuLoadNamelist(GWindow UNUSED(gw), struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e));
void FVMenuBaseVert(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e));
void FVMenuBaseHoriz(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e));
void FVMenuJustify(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e));
void FVMenuReplaceWithRef(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e));
void FVMenuCorrectRefs(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e));

// These become stubs when collab support is not compiled I thought it
// was an obvious pattern to leave the function as a no-op when collab
// is not available so that calling code doesn't have to worry about
// ifdef guards and can just call the blank function. I also reject
// the stipulation that calling a stub is very costly on modern
// hardware. If you think you have a very tight block then feel free
// to ifdef guard the call, but profile data might not be your friend. If profiling
// happens to show a performance drop for you then maybe we should consider
// #define FVMenuCollabStart(a,b,c) to nothing if build_collab is not set instead
// of polluting ifdefs all over the place.
void FVMenuCollabStart(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e));
void FVMenuCollabConnect(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e));
void FVMenuCollabConnectToExplicitAddress(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e));
void FVMenuCollabDisconnect(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e));
void FVMenuCollabCloseLocalServer(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e));





    

















    









    
































































#endif

