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

static char* hotkeyGetWindowTypeString( Hotkey* hk ) 
{
    char* pt = strchr(hk->action,'.');
    if( !pt )
	return 0;
    int len = pt - hk->action;
    static char buffer[HOTKEY_ACTION_MAX_SIZE+1];
    strncpy( buffer, hk->action, len );
    buffer[len] = '\0';
    return buffer;
}

/**
 * Does the hotkey 'hk' have the right window_type to trigger its
 * action on the window 'w'.
 */
static int hotkeyHasMatchingWindowTypeString( char* windowType, Hotkey* hk ) {
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

/**
 * Does the hotkey 'hk' have the right window_type to trigger its
 * action on the window 'w'.
 */
static int hotkeyHasMatchingWindowType( GWindow w, Hotkey* hk ) {
    char* windowType = GDrawGetWindowTypeName( w );
    return hotkeyHasMatchingWindowTypeString( windowType, hk );
}

static struct dlistnodeExternal*
hotkeyFindAllByStateAndKeysym( char* windowType, uint16 state, uint16 keysym ) {

    struct dlistnodeExternal* ret = 0;
    struct dlistnode* node = hotkeys;
    for( ; node; node=node->next ) {
	Hotkey* hk = (Hotkey*)node;
	printf("check hk:%s keysym:%d\n", hk->text, hk->keysym );
	if( hk->keysym ) {
	    if( keysym == hk->keysym ) {
		if( state == hk->state ) {
		    printf("event match! for hk:%s keysym:%d\n", hk->text, hk->keysym );
		    printf("event.state:%d hk.state:%d\n", state, hk->state );
		    if( hotkeyHasMatchingWindowTypeString( windowType, hk ) ) {
			printf("matching window type too for hk:%s keysym:%d\n", hk->text, hk->keysym );
			dlist_pushfront_external( &ret, hk );
		    }
		}
	    }
	}
    }
    return ret;
}


static Hotkey* hotkeyFindByStateAndKeysym( char* windowType, uint16 state, uint16 keysym ) {

    struct dlistnode* node = hotkeys;
    for( ; node; node=node->next ) {
	Hotkey* hk = (Hotkey*)node;
	printf("check hk:%s keysym:%d\n", hk->text, hk->keysym );
	if( hk->keysym ) {
	    if( keysym == hk->keysym ) {
		if( state == hk->state ) {
		    printf("event match! for hk:%s keysym:%d\n", hk->text, hk->keysym );
		    printf("event.state:%d hk.state:%d\n", state, hk->state );
		    if( hotkeyHasMatchingWindowTypeString( windowType, hk ) ) {
			printf("matching window type too for hk:%s keysym:%d\n", hk->text, hk->keysym );
			return hk;
		    }
		}
	    }
	}
    }
    return 0;
}


struct dlistnodeExternal* hotkeyFindAllByEvent( GWindow w, GEvent *event ) {
    if( event->u.chr.autorepeat )
	return 0;
    char* windowType = GDrawGetWindowTypeName( w );
    return hotkeyFindAllByStateAndKeysym( windowType,
					  event->u.chr.state,
					  event->u.chr.keysym );
}


Hotkey* hotkeyFindByEvent( GWindow w, GEvent *event ) {

    if( event->u.chr.autorepeat )
	return 0;

    char* windowType = GDrawGetWindowTypeName( w );
    return hotkeyFindByStateAndKeysym( windowType, event->u.chr.state, event->u.chr.keysym );
}

/**
 * Return the file name of the user defined hotkeys.
 * The return value must be freed by the caller.
 */
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

/**
 * Load all the hotkeys from the file at filename, marking them as userdefined
 * if isUserDefined is set.
 */
static void loadHotkeysFromFile( const char* filename, int isUserDefined ) 
{
    char line[1100];
    FILE* f = fopen(filename,"r");
    if( !f ) {
	fprintf(stderr,"Failed to open hotkey definition file: %s\n", filename );
	return;
    }
    fprintf(stderr,"loading hotkey definition file: %s\n", filename );

    while ( fgets(line,sizeof(line),f)!=NULL ) {
	int append = 0;
	
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
	char* action = line;
	if( line[0] == '+' ) {
	    append = 1;
	    action++;
	}
	
	
	Hotkey* hk = gcalloc(1,sizeof(Hotkey));
	strncpy( hk->action, action, HOTKEY_ACTION_MAX_SIZE );
	HotkeyParse( hk, keydefinition );

	// If we didn't get a hotkey (No Shortcut)
	// then we move along
	if( !hk->state && !hk->keysym ) {
	    free(hk);
	    continue;
	}
	
	// If we already have a binding for that hotkey combination
	// for this window, forget the old one. One combo = One action.
	if( !append ) {
	    Hotkey* oldkey = hotkeyFindByStateAndKeysym( hotkeyGetWindowTypeString(hk),
							 hk->state, hk->keysym );
	    if( oldkey ) {
		printf("have oldkey!\n");
		dlist_erase( &hotkeys, oldkey );
		free(oldkey);
	    }
	}
	
	
	hk->isUserDefined = isUserDefined;
	printf("3. state:%d keysym:%d\n", hk->state, hk->keysym );
	dlist_pushfront( &hotkeys, hk );
    }
    fclose(f);
}

/**
 * Load all the default hotkeys for this locale and then the users
 * ~/.Fontforge/hotkeys.
 */
void hotkeysLoad()
{
    char* p = 0;
    
    // FIXME: find out how to convert en_AU.UTF-8 that setlocale()
    //   gives to its fallback of en_GB
    char localefn[PATH_MAX+1];
    char* currentlocale = copy(setlocale(LC_MESSAGES, 0));
    snprintf(localefn,PATH_MAX,"%s/hotkeys/%s", SHAREDIR, currentlocale);
    loadHotkeysFromFile( localefn, false );
    while( p = strrchr( currentlocale, '.' )) {
	*p = '\0';
	printf("LOOP currentlocale:%s\n", currentlocale );
	snprintf(localefn,PATH_MAX,"%s/hotkeys/%s", SHAREDIR, currentlocale);
	loadHotkeysFromFile( localefn, false );
    }
    while( p = strrchr( currentlocale, '_' )) {
	*p = '\0';
	printf("LOOP currentlocale:%s\n", currentlocale );
	snprintf(localefn,PATH_MAX,"%s/hotkeys/%s", SHAREDIR, currentlocale);
	loadHotkeysFromFile( localefn, false );
    }
    
    free(currentlocale);
    
    char* fn = getHotkeyFilename();
    if( !fn ) {
	fprintf(stderr,"Can not work out where your hotkey definition file is!\n");
	return;
    }
    loadHotkeysFromFile( fn, true );
    free(fn);
}

static void hotkeysSaveCallback(Hotkey* hk,FILE* f) {
    if( hk->isUserDefined ) {
	fprintf( f, "%s:%s\n", hk->action, hk->text );
    }
}

/**
 * Save all the user defined hotkeys back to the users
 * ~/.Fontforge/hotkeys file.
 */
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


/**
 * Find a hotkey by the action. This is useful for menus to find out
 * what hotkey is currently bound to them. So if the user changes
 * file/open to be alt+j then the menu can adjust the hotkey is is
 * displaying to show the user what key they have assigned.
 */
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

