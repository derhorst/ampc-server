/* ampc
   Based on ympd (http://www.ympd.org), (c) 2013-2014 by Andrew Karpow <andy@ndyk.de>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/time.h>
#include <pthread.h>

#include "mongoose.h"
#include "http_server.h"
#include "mpd_client.h"
#include "config.h"

extern char *optarg;

int force_exit = 0;
char *music_dir = NULL;

char * getConcatString( const char *str1, const char *str2)
{
    char *finalString = NULL;
    size_t n = 0;

    if ( str1 ) n += strlen( str1 );
    if ( str2 ) n += strlen( str2 );

    if ( ( str1 || str2 ) && ( finalString = malloc( n + 1 ) ) != NULL )
    {
        *finalString = '\0';

        if ( str1 ) strcpy( finalString, str1 );
        if ( str2 ) strcat( finalString, str2 );
    }

    return finalString;
}

void bye()
{
    force_exit = 1;
}

static int server_callback(struct mg_connection *c, enum mg_event ev) {
    switch(ev) {
        case MG_CLOSE:
            mpd_close_handler(c);
            return MG_TRUE;
        case MG_REQUEST:
            /* if the uri starts with /cover/ look for the cover and send it to the client*/
            if (strncmp("/cover/", c->uri, strlen("/cover/")) == 0 && music_dir != NULL) {
              // cut the filename
              char *chptr = strrchr(c->uri, '/');

              char * folder = getConcatString(music_dir, strndup(c->uri+7, (chptr - c->uri - 7)));
              char * cover = getConcatString(folder, "/folder.jpg");
              mg_send_file(c, cover, NULL);
              return MG_MORE;
            }
            if (c->is_websocket) {
                c->content[c->content_len] = '\0';
                if(c->content_len)
                    return callback_mpd(c);
                else
                    return MG_TRUE;
            } else
#ifdef WITH_DYNAMIC_ASSETS
                return MG_FALSE;
#else
                return callback_http(c);
#endif
        case MG_AUTH:
            return MG_TRUE;
        default:
            return MG_FALSE;
    }
}

int main(int argc, char **argv)
{
    int n, option_index = 0;
    struct mg_server *server = mg_create_server(NULL, server_callback);
    unsigned int current_timer = 0, last_timer = 0;
    char *run_as_user = NULL;
    char const *error_msg = NULL;
    char *webport = "8080";

    atexit(bye);
#ifdef WITH_DYNAMIC_ASSETS
    mg_set_option(server, "document_root", SRC_PATH);
#endif

    mpd.port = 6600;
    strcpy(mpd.host, "127.0.0.1");

    static struct option long_options[] = {
        {"host",            required_argument, 0, 'h'},
        {"port",            required_argument, 0, 'p'},
        {"music-directory", required_argument, 0, 'd'},
        {"webport",         required_argument, 0, 'w'},
        {"user",            required_argument, 0, 'u'},
        {"version",         no_argument,       0, 'v'},
        {"help",            no_argument,       0,  0 },
        {0,                 0,                 0,  0 }
    };

    while((n = getopt_long(argc, argv, "h:p:d:w:u:v",
                long_options, &option_index)) != -1) {
        switch (n) {
            case 'h':
                strncpy(mpd.host, optarg, sizeof(mpd.host));
                break;
            case 'p':
                mpd.port = atoi(optarg);
                break;
            case 'd':
                music_dir = strdup(optarg);
                break;
            case 'w':
                webport = strdup(optarg);
                break;
            case 'u':
                run_as_user = strdup(optarg);
                break;
            case 'v':
                fprintf(stdout, "ampc  %d.%d.%d\n"
                        "Copyright (C) 2014 Andrew Karpow <andy@ndyk.de>\n"
                        "built " __DATE__ " "__TIME__ " ("__VERSION__")\n",
                        AMPC_VERSION_MAJOR, AMPC_VERSION_MINOR, AMPC_VERSION_PATCH);
                return EXIT_SUCCESS;
                break;
            default:
                fprintf(stderr, "Usage: %s [OPTION]...\n\n"
                        " -h, --host <host>\t\tconnect to mpd at host [localhost]\n"
                        " -p, --port <port>\t\tconnect to mpd at port [6600]\n"
                        " -d, --music-directory <dir>\tUsed to look up cover-art. Covers have to be\n"
                                              "\t\t\t\tin the same folder as the song and named\n"
                                              "\t\t\t\t'folder.jpg' (embedded covers are not supported)\n"
                        " -w, --webport [ip:]<port>\tlisten interface/port for webserver [8080]\n"
                        " -u, --user <username>\t\tdrop priviliges to user after socket bind\n"
                        " -V, --version\t\t\tget version\n"
                        " --help\t\t\t\tthis help\n"
                        , argv[0]);
                return EXIT_FAILURE;
        }

        if(error_msg)
        {
            fprintf(stderr, "Mongoose error: %s\n", error_msg);
            return EXIT_FAILURE;
        }
    }

    error_msg = mg_set_option(server, "listening_port", webport);
    if(error_msg) {
        fprintf(stderr, "Mongoose error: %s\n", error_msg);
        return EXIT_FAILURE;
    }

    /* drop privilges at last to ensure proper port binding */
    if(run_as_user != NULL) {
        error_msg = mg_set_option(server, "run_as_user", run_as_user);
        free(run_as_user);
        if(error_msg)
        {
            fprintf(stderr, "Mongoose error: %s\n", error_msg);
            return EXIT_FAILURE;
        }
    }

    while (!force_exit) {
        mg_poll_server(server, 200);
        current_timer = time(NULL);
        if(current_timer - last_timer)
        {
            last_timer = current_timer;
            mpd_poll(server);
        }
    }

    mpd_disconnect();
    mg_destroy_server(&server);

    return EXIT_SUCCESS;
}
