#include <stdlib.h>
#include <stdio.h>
#include <libircclient.h>
#include <libirc_rfcnumeric.h>
#include <string.h>

#include "constants.h"

void doStuff(irc_session_t * session) {
    irc_cmd_msg( session, NICKOLAS_IRC_NICK, "Hi!" );
}

void handleConnection(irc_session_t * session, const char * event, const char * origin, const char ** params, unsigned int count) {
    printf("Connected callback!\n");
    printf(" %s %s: ", event, origin);
    for(size_t i=0; i<count; i++) {
        printf("%s ", params[i]);
    }
    printf("\n");
    doStuff(session);
}

void handleCode(irc_session_t * session, unsigned int event, const char * origin, const char ** params, unsigned int count) {
    if(250 <= event && event <= 266) {return;} // Ignore server stats
    if(event == 375 || event == 372 || event == 376) {return;} // ignore MOTD
    
    
    printf("RFC %d from %s:\t", event, origin);
    for(size_t i=0; i<count; i++) {
        printf(" %s", params[i]);
    }
    printf("\n");

}



int main(void) {
    // The IRC callbacks structure
    irc_callbacks_t callbacks;

    // Init it
    memset ( &callbacks, 0, sizeof(callbacks) );

    // Set up the mandatory events
    void (*connect_handler)(irc_session_t*, const char*, const char*, const char**, unsigned int);
    void (*code_handler)(irc_session_t*, unsigned int, const char*, const char**, unsigned int);

    connect_handler = &handleConnection;
    code_handler = &handleCode;

    callbacks.event_connect = connect_handler;
    callbacks.event_numeric = code_handler;

    // Set up the rest of events

    // Now create the session
    irc_session_t * session = irc_create_session( &callbacks );

    if ( !session ) {
        // Handle the error
        printf("No session!\n");
        return 1;
    }
    // else...
    irc_option_set( session, LIBIRC_OPTION_STRIPNICKS );
    if ( irc_connect (session, "irc.freenode.net", 6667, 0, NICKOLAS_IRC_INITIALNICK, NULL, NULL ) ) {
        printf("Failed to connect!\n");
        int errno =  irc_errno(session);
        printf("irc_connect failed with code %d: %s\n",errno, irc_strerror(errno));
        return 1;
    }
    else {
        printf("Connected sucessfully!\n");
    }

    if(irc_run(session)) { //nonzero return => error
        int errno =  irc_errno(session);
        printf("irc_run failed with code %d: %s\n",errno, irc_strerror(errno));
        return 1;
    }
    getchar();
}