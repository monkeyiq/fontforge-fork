/* Copyright (C) 2012 by Ben Martin */
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

#include "hotkeys.h"
#include <locale.h>


struct dlistnode* hotkeys = 0;


static char *getHotkeyFilename(void) {
    static char *ret=NULL;
    char buffer[1025];

    if ( ret!=NULL )
	return( ret );
    if ( getPfaEditDir(buffer)==NULL )
	return( NULL );
    
    sprintf(buffer,"%s/hotkeys", getPfaEditDir(buffer));
    ret = copy(buffer);
    return( ret );
}

/**
 * Remove leading and trailing " " characters. N memory allocations
 * are performed, null is injected at the end of string and if there are leading
 * spaces the return value will be past them.
 */
char* trimspaces( char* line ) {
   while ( line[strlen(line)-1]==' ' )
	line[strlen(line)-1] = '\0';
   while( *line == ' ' )
	++line;
    return line;
}

static void loadHotkeysFromFile( const char* filename, int isUserDefined ) 
{
    char line[1100];
    FILE* f = fopen(filename,"r");
    if( !f ) {
	fprintf(stderr,"Failed to open hotkey definition file: %s\n", filename );
	return;
    }

    while ( fgets(line,sizeof(line),f)!=NULL ) {
	if ( *line=='#' )
	    continue;
	char* pt = strchr(line,':');
	if ( pt==NULL )
	    continue;
	*pt = '\0';
	char* keydefinition = pt+1;
	chomp( keydefinition );
	keydefinition = trimspaces( keydefinition );
	printf("2.accel:%s key__%s__\n", line, keydefinition );

	Hotkey* hk = gcalloc(1,sizeof(Hotkey));
	strncpy( hk->action, line, HOTKEY_ACTION_MAX_SIZE );
	HotkeyParse( hk, keydefinition );
	hk->isUserDefined = isUserDefined;
	printf("3. state:%d keysym:%d\n", hk->state, hk->keysym );
	dlist_pushfront( &hotkeys, hk );
    }
    fclose(f);
}

void hotkeysLoad()
{
    // FIXME: find out how to convert en_AU.UTF-8 that setlocale()
    //   gives to its fallback of en_GB
    char localefn[PATH_MAX+1];
    snprintf(localefn,PATH_MAX,"%s/hotkeys/%s",
	     SHAREDIR,setlocale(LC_MESSAGES, 0));
    loadHotkeysFromFile( localefn, false );

    char* fn = getHotkeyFilename();
    if( !fn ) {
	fprintf(stderr,"Can not work out where your hotkey definition file is!\n");
	return;
    }
    loadHotkeysFromFile( fn, true );
}

static void hotkeysSaveCallback(Hotkey* hk,FILE* f) {
    if( hk->isUserDefined ) {
	fprintf( f, "%s:%s\n", hk->action, hk->text );
    }
}


void hotkeysSave() {
    char* fn = "/tmp/hotkeys.out";
    if( !fn ) {
	fprintf(stderr,"Can not work out where your hotkey definition file is!\n");
	return;
    }
    FILE* f = fopen(fn,"w");
    if( !f ) {
	fprintf(stderr,"Failed to open your hotkey definition file for updates.\n");
	return;
    }
    printf("hotkeysSave() sz:%d\n", dlist_size( &hotkeys ));

    dlist_foreach_udata( &hotkeys, hotkeysSaveCallback, f );
    fclose(f);

    // FIXME: rename this over the old file.
}

    
char* hotkeysGetKeyDescriptionFromAction( char* action ) {
    struct dlistnode* node = hotkeys;
    for( ; node; node=node->next ) {
	Hotkey* hk = (Hotkey*)node;
	if(!strcmp(hk->action,action)) {
	    return hk->text;
	}
    }
    return 0;
}

static int hotkeyHasMatchingWindowType( GWindow w, Hotkey* hk ) {
    char* windowType = GDrawGetWindowTypeName( w );
    if( !windowType )
	return 0;
    char* pt = strchr(hk->action,'.');
    if( !pt )
	return 0;
    
    int len = pt - hk->action;
    if( strlen(windowType) < len )
	return 0;
    int rc = strncmp( windowType, hk->action, len );
    if( !rc )
	return 1;
    return 0;
}

Hotkey* hotkeyFindByAction( char* action ) {
    struct dlistnode* node = hotkeys;
    for( ; node; node=node->next ) {
	Hotkey* hk = (Hotkey*)node;
	if(!strcmp(hk->action,action)) {
	    return hk;
	}
    }
    return 0;
}


Hotkey* hotkeyFindByMenuPath( GWindow w, char* path ) {

    char* wt = GDrawGetWindowTypeName(w);
    if(!wt)
	return 0;

    char line[PATH_MAX+1];
    snprintf(line,PATH_MAX,"%s%s%s",wt, ".Menu.", path );
    return(hotkeyFindByAction(line));
}


    
Hotkey* hotkeyFindByEvent( GWindow w, GEvent *event ) {

    if( event->u.chr.autorepeat )
	return 0;
    
    struct dlistnode* node = hotkeys;
    for( ; node; node=node->next ) {
	Hotkey* hk = (Hotkey*)node;
	printf("check hk:%s keysym:%d\n", hk->text, hk->keysym );
	if( hk->keysym ) {
	    if( event->u.chr.keysym == hk->keysym ) {
		if( event->u.chr.state == hk->state ) {
		    printf("event match! for hk:%s keysym:%d\n", hk->text, hk->keysym );
		    printf("event.state:%d hk.state:%d\n",
			   event->u.chr.state,
			   hk->state );
		    if( hotkeyHasMatchingWindowType( w, hk ) ) {
			printf("matching window type too for hk:%s keysym:%d\n", hk->text, hk->keysym );
			return hk;
		    }
		}
	    }
	}
    }
    return 0;
}

char* hotkeyTextWithoutModifiers( char* hktext ) {
    if( !strcmp( hktext, "no shortcut" )
	|| !strcmp( hktext, "No shortcut" )
	|| !strcmp( hktext, "No Shortcut" ))
	return "";
    
    char* p = strrchr( hktext, '+' );
    if( !p )
	return hktext;
    return p+1;
}


