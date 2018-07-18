#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <pcre.h>
#include <libircclient.h>
#include <libirc_rfcnumeric.h>

#include "linkedlist.h"

#define IRC_MAX_NICK_LENGTH 16

struct LinkedStringNode *nicks;

typedef enum {DISCONNECTED, CONNECTED_NOAUTH, CONNECTED_NONICKS, CONNECTED_PROCESSING, CONNECTED_DONE} nickolas_state;
nickolas_state state = DISCONNECTED;
pcre *nick_re;
int nick_extradelay = 0;
bool verbosemode = false;

char *NICKOLAS_IRC_NICK, *NICKOLAS_IRC_NICKSERV_PASSWORD;

void compileRegex() {
    const char *regex = "^(Nicks\\s+:\\s+)", *error;
    int erroffset;
    nick_re = pcre_compile(regex, 0, &error, &erroffset, 0);
    if(!nick_re) {
        printf("pcre_compile failed (offset: %d), %s\n", erroffset, error);
        exit(3);
    }
}

void doStuff(irc_session_t * session) {
    compileRegex();

    char command[11 + strlen(NICKOLAS_IRC_NICKSERV_PASSWORD) + strlen(NICKOLAS_IRC_NICK)];
    strcpy(command, "IDENTIFY ");
    strcat(command, NICKOLAS_IRC_NICK);
    strcat(command, " ");
    strcat(command, NICKOLAS_IRC_NICKSERV_PASSWORD);
    irc_cmd_msg(session, "NickServ", command);
}

void handleConnection(irc_session_t * session, const char * event, const char * origin, const char ** params, unsigned int count) {
    state = CONNECTED_NOAUTH;
    printf(" %s %s: ", event, origin);
    for(size_t i=0; i<count; i++) {
        printf("%s ", params[i]);
    }
    printf("\n");
    doStuff(session);
}

void changeNick(irc_session_t * session) {
    if(NULL == nicks || NULL == nicks->value) {
        puts("All nicks visted; quitting.");
        irc_cmd_quit(session, "Done");
        return;
    }
    // else
    if (verbosemode) {
        printf("irc_cmd_nick(%s)\n", nicks->value);
    }
    irc_cmd_nick(session, nicks->value);
}

void handleCode(irc_session_t * session, unsigned int event, const char * origin, const char ** params, unsigned int count) {
    (void) session;
    if(250 <= event && event <= 266) {
        printf(".");
        if(event == LIBIRC_RFC_RPL_ENDOFMOTD) {printf("\n");}
        return;
    } // Ignore server stats
    if(event == 375 || event == 372 || event == 376) {return;} // ignore MOTD
    if(event == 438) {
        // Nick changed too fast
        puts("Nick changed too quickly, cooling down for 21 seconds");
        sleep(21);
        changeNick(session);
        return;
    }
    if(event == 433) {
        // Nick in use
        printf("Nick %s in use; skipping.\n", nicks->value);
        nicks = nicks->next;
        changeNick(session);
    }
    if(verbosemode) {
        printf("RFC %d from %s:\t", event, origin);
        for(size_t i=0; i<count; i++) {
            printf(" %s", params[i]);
        }
        printf("\n");
    }
}

void handleNick(irc_session_t * session, const char * event, const char * origin, const char ** params, unsigned int count) {
    (void) event;
    (void) count;
    printf("Recieved: %s -> %s\n", origin, params[0]);
    if(strncmp(params[0],nicks->value, IRC_MAX_NICK_LENGTH) == 0) {
        if(nick_extradelay > 0) {
            sleep(2 + nick_extradelay);
        }
        else {sleep(2);}
        nick_extradelay = 0;
        nicks = nicks->next;
        if(NULL != nicks) {
            changeNick(session);
        }
        else {
            puts("All nicks visted; quitting.");
            irc_cmd_quit(session, "Done");
        }
    }
    else {
        printf("%s != %s\n", params[0], nicks->value);
        irc_cmd_quit(session, "Unexpected nick");
    }
}

void printNicks() {
    LinkedStringNode *current = nicks;
    printf("Nicks:\n");
    while(NULL != current && NULL != current->value) {
        printf("%s, ", current->value);
        current = current->next;
    }
    printf("\n");
}

void handleNoticeOrPM(irc_session_t * session, const char * event, const char * origin, const char ** params, unsigned int count) {
    (void) count;
    if (verbosemode) {
        printf("%s: %s: %s\n", event, origin, params[1]);
    }
    if(strcmp(origin, "NickServ") == 0) {
        char * strippedMessage; // C is an awful language and we should burn it to the ground and start afresh
        switch(state) {
            case CONNECTED_NOAUTH:
                // This'll be the auth message coming in
                if(strncmp("You are now identified for", params[1], 26) == 0) {
                    state = CONNECTED_NONICKS;
                    char command[5 + strlen(NICKOLAS_IRC_NICK)];
                    strcpy(command, "INFO ");
                    strcat(command, NICKOLAS_IRC_NICK);
                    irc_cmd_msg(session, "NickServ", command);
                }
                else {
                    puts("Unexpected message!");
                    exit(2);
                }
                break;
            case CONNECTED_NONICKS:
                strippedMessage = irc_color_strip_from_mirc(params[1]);
                if(strippedMessage == 0) {
                    printf("malloc failed in irc_color_strip_from_mirc\n");
                    free(strippedMessage);
                    exit(4);
                }
                // else...
                int ovector[4];
                int rc = pcre_exec(nick_re, 0, strippedMessage, strlen(strippedMessage), 0, 0, ovector, sizeof(ovector));
                if(rc >= 0) { // pattern matched
                    int nick_str_len = strlen(strippedMessage) - ovector[1];
                    
                    char nicks_str[nick_str_len + 1], *token;
                    snprintf(nicks_str, nick_str_len+1, "%.*s", nick_str_len, strippedMessage + ovector[1]);
                    token = strtok(nicks_str, " ");
                    while(NULL != token) {
                        if(NULL == nicks) {
                            // first batch of messages
                            nicks = malloc(sizeof(LinkedStringNode));
                            if(NULL == nicks) {
                                printf("malloc failed in LinkedStringNode");
                                free(nicks);
                                exit(4);
                            }
                            ll_create(&nicks, token);
                            printf("Initialising `nicks` with %s\n", token);
                        }
                        else {
                            printf("Appending %s\n", token);
                            ll_append(nicks, token);
                        }
                        token = strtok(NULL, " ");
                    }

                }
                else { // doesn't match Nicks:
                    if(strncmp(strippedMessage, "*** End of Info ***", 19) == 0) {
                        state = CONNECTED_PROCESSING;
                        printNicks();
                        changeNick(session);
                    }
                }
                free(strippedMessage);
                break;
            default:
                puts("Unexpected message!");
                break;
        }
    }
    fflush(stdin);
}

int main(int argc, char *argv[]) {
    if(argc < 3) {
        puts("Insufficient number of arguments");
        puts("Usage: nickolas username \"nickserv_password\" [-v | --verbose]\n");
        return 2;
    }
    //else 
    NICKOLAS_IRC_NICK = malloc(strlen(argv[1]) * sizeof(char)+1);
    NICKOLAS_IRC_NICKSERV_PASSWORD = malloc(strlen(argv[2]) * sizeof(char)+1);
    if(NULL == NICKOLAS_IRC_NICK || NULL == NICKOLAS_IRC_NICKSERV_PASSWORD) {
        free(NICKOLAS_IRC_NICK);
        free(NICKOLAS_IRC_NICKSERV_PASSWORD);
        puts("Malloc failed for arguments");
        return 1;
    }
    // else
    strcpy(NICKOLAS_IRC_NICK, argv[1]);
    strcpy(NICKOLAS_IRC_NICKSERV_PASSWORD, argv[2]);
    for(int i=3; i<argc; i++) {
        if(strcmp(argv[i],"-v") == 0 || strcmp(argv[i],"--verbose") == 0) {
            verbosemode = true;
        }
    }
    
    irc_callbacks_t callbacks;
    memset ( &callbacks, 0, sizeof(callbacks) );

    // Set up the mandatory events
    void (*connect_handler)(irc_session_t*, const char*, const char*, const char**, unsigned int);
    void (*code_handler)(irc_session_t*, unsigned int, const char*, const char**, unsigned int);

    connect_handler = &handleConnection;
    code_handler = &handleCode;

    callbacks.event_connect = connect_handler;
    callbacks.event_numeric = &handleCode;

    callbacks.event_notice = &handleNoticeOrPM;
    callbacks.event_privmsg = &handleNoticeOrPM;
    callbacks.event_nick = &handleNick;

    irc_session_t * session = irc_create_session( &callbacks );

    if ( !session ) {
        printf("No session!\n");
        return 1;
    }
    // else...
    irc_option_set( session, LIBIRC_OPTION_STRIPNICKS );
    if ( irc_connect (session, "irc.freenode.net", 6667, 0, "__nickolas__", NULL, NULL ) ) {
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
    }
    ll_free(nicks);
}
