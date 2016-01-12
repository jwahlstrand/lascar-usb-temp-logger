/**
 * Copyright (c) 2010 Ángel Sancho angel.sancho@gmail.com
 * Improvements by Roberto Aguilar roberto.c.aguilar@gmail.com
 * Modified to upload data to InfluxDB by Jared Wahlstrand wahlstrj@umd.edu
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
 */

#include <stdio.h>
#include <string.h>
#include <glib.h>
#include <libsoup/soup.h>

/* human interface device API */
#include <hid.h>

/* lascar API */
#include "lascar.h"

#define MAX_ERRORS 10
#define SLEEP_TIME 1.0

static int n = 0;
static gboolean debug = FALSE;

static GOptionEntry entries[] = {
  {"num", 'n', 0, G_OPTION_ARG_INT, &n, "Number of readings to take (default: go on forever)", NULL },
  {"debug", 'd', 0, G_OPTION_ARG_NONE, &debug, "Enable debugging", NULL },
  { NULL }
};

int main(int argc, char *argv[])
{
    char packet[] = {0x00, 0x00, 0x00};
    HIDInterface* hid = NULL;
    hid_return ret;

    float temp, hum;

    int status = 0;
    int error_count = 0;

    int count = 0;

    GError *error = NULL;
    GOptionContext *context;

    context = g_option_context_new ("- collect temperature and humidity readings");
    g_option_context_add_main_entries (context, entries, NULL);
    if (!g_option_context_parse (context, &argc, &argv, &error))
    {
      g_print ("option parsing failed: %s\n", error->message);
      exit (1);
    }

    /* setup debugging to stderr if requested */
    if(debug) {
        hid_set_debug(HID_DEBUG_ALL);
        hid_set_debug_stream(stderr);
    }

    if((hid=init_termo(hid)) == NULL) {
        g_printerr("Device NOT present.\n");
        exit(-1);
    }
    
    /* parse config file */
    GKeyFile *gkf = g_key_file_new();
    g_key_file_load_from_file(gkf,"templogger.conf",G_KEY_FILE_NONE,&error);
    gchar *url = g_key_file_get_string(gkf,"influx","url",&error);
    int channel = 1;
    const gchar *channel_name = "Thor";
    const gchar *room = "Small\\ Lab";
    int port = g_key_file_get_integer(gkf,"influx","port",&error);
    gchar *db = g_key_file_get_string(gkf,"influx","database",&error);
    gchar *username = g_key_file_get_string(gkf,"influx","username",&error);
    gchar *password = g_key_file_get_string(gkf,"influx","password",&error);
    g_key_file_free(gkf);
    
    /* open session */
    guint session_status;
    SoupSession *session = soup_session_new();
    GString *uri = g_string_new("http://");
    g_string_append_printf(uri,"%s:%d/write?db=%s&u=%s&p=%s",url,port,db,username,password);

    g_message(uri->str);

    FILE *logfile = fopen("test.txt","w");
    GString *body = g_string_sized_new(1024);

    while(1) {
        status = 0;

        ret = get_reading(hid, packet, &temp, &hum, TRUE);
        if(ret != HID_RET_SUCCESS) {
            status = -1;
        }

        if(status == 0) {
            g_print("temp: %.1f, hum: %.1f\n", temp, hum);
            
            gint64 t = 1000*g_get_real_time();
            
            fprintf(logfile,"%" G_GINT64_FORMAT "\t%.1f\t%.1f\n",t, temp, hum);
            fflush(logfile);
            
            if(count % 60 == 0) {
            SoupRequestHTTP *request = soup_session_request_http(session,"POST",uri->str,&error);
            SoupMessage *message = soup_request_http_get_message(request);
            g_string_printf(body,"temp,channel=%d,channel_name=%s,room=%s",channel,channel_name,room);
            g_string_append_printf(body," value=%.1f %" G_GINT64_FORMAT,temp,t);
            g_string_append_printf(body,"\n");
            g_string_append_printf(body,"hum,channel=%d,channel_name=%s,room=%s",channel,channel_name,room);
            g_string_append_printf(body," value=%.1f %" G_GINT64_FORMAT,hum,t);
            
            g_print(body->str);
            
            soup_message_set_request(message,"application/binary",SOUP_MEMORY_COPY,body->str,body->len);
	        session_status = soup_session_send_message(session,message);
            g_message("HTTP response: %u",session_status);

            g_object_unref(message);
            }
            count ++;

            /* reset the error count on successful read */
            error_count = 0;
        } else if(error_count > MAX_ERRORS) {
            g_printerr("Too many errors to continue\n");
            exit(-1);
        } else {
            error_count++;
        }
        sleep(SLEEP_TIME);
    }

    restore_termo(hid);

    return 0;
}
