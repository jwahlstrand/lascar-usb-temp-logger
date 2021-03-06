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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <glib.h>
#include <libsoup/soup.h>

/* lascar API */
#include "lascar.h"

#define MAX_ERRORS 10
#define SLEEP_TIME 1.0 /* time between readings, in seconds */
#define UPLOAD_TIME 60.0 /* time between uploadings, in seconds */

static gboolean log_local = FALSE;
static gboolean testing = FALSE;
static gboolean debug = FALSE;

static GOptionEntry entries[] = {
  {"log", 'l', 0, G_OPTION_ARG_NONE &log_local, "Log data locally as well", NULL},
  {"test", 't', 0, G_OPTION_ARG_NONE, &testing, "Testing mode (i.e. don't actually upload data)", NULL},
  {"debug", 'd', 0, G_OPTION_ARG_NONE, &debug, "Enable debugging", NULL },
  { NULL }
};

int main(int argc, char *argv[])
{
    char packet[] = {0x00, 0x00, 0x00};

    float temp, hum;

    int error_count = 0;

    int count = 1;

    GError *error = NULL;
    GOptionContext *context;

    context = g_option_context_new ("- collect temperature and humidity readings");
    g_option_context_add_main_entries (context, entries, NULL);
    if (!g_option_context_parse (context, &argc, &argv, &error))
    {
      g_print ("option parsing failed: %s\n", error->message);
      exit (1);
    }
    g_option_context_free(context);
    set_debug(debug);
    hid_device *handle;
    gboolean found = search_for_device();

    if(!found) {
        g_printerr("Sensor not found, aborting.\n");
        exit(-1);
    }
    
    // Open the device using the VID and PID
    handle = open_lascar();
    if(handle == NULL) {
      g_printerr("Error opening sensor.\n");
      exit(-1);
    }
    
    SoupSession *session = NULL;
    int channel;
    gchar *channel_name;
    gchar *room;
    GString *uri, *body;
    
    /* parse config file */
    GKeyFile *gkf = g_key_file_new();
    g_key_file_load_from_data_dirs(gkf,"templogger.conf",NULL,G_KEY_FILE_NONE,&error);
    if(error!=NULL) {
        g_printerr("Can't load configuration file, not uploading to server.\n");
        g_key_file_free(gkf);
    }
    else {
      gchar *url = g_key_file_get_string(gkf,"influx","url",NULL);
      channel = g_key_file_get_integer(gkf,"channel","channel_num",NULL);
      channel_name = g_key_file_get_string(gkf,"channel","channel_name",NULL);
      room = g_key_file_get_string(gkf,"channel","room",NULL);
      int port = g_key_file_get_integer(gkf,"influx","port",NULL);
      gchar *db = g_key_file_get_string(gkf,"influx","database",NULL);
      gchar *username = g_key_file_get_string(gkf,"influx","username",NULL);
      gchar *password = g_key_file_get_string(gkf,"influx","password",NULL);
      g_key_file_free(gkf);
    
      /* open session */
    
      session = soup_session_new();
      uri = g_string_new("http://");
      g_string_append_printf(uri,"%s:%d/write?db=%s&u=%s&p=%s",url,port,db,username,password);

      g_message(uri->str);
      body = g_string_new("");
	  g_print("Uploading as channel %d, name %s, room %s",channel, channel_name, room);
    }
    
    FILE *logfile = NULL;
    if(log_local) {
        logfile = fopen("log.txt","a");
    }
    
    const int upload_freq = floor(UPLOAD_TIME/SLEEP_TIME);
    
    while(1) {
        int ret = get_reading(handle, packet, &temp, &hum, TRUE);
        if(ret >= 0) {
            gint64 t = 1000*g_get_real_time();
            
            if(log_local) {
              fprintf(logfile,"%" G_GINT64_FORMAT "\t%.1f\t%.1f\n",t, temp, hum);
              fflush(logfile);
            }
            
            if(debug)
              g_print("%" G_GINT64_FORMAT "\t%.1f\t%.1f\n",t, temp, hum);
            
            if(session && (count % upload_freq == 0)) {
            SoupRequestHTTP *request = soup_session_request_http(session,"POST",uri->str,NULL);
            SoupMessage *message = soup_request_http_get_message(request);
            g_string_append_printf(body,"temp,channel=%d,channel_name=%s,room=%s",channel,channel_name,room);
            g_string_append_printf(body," value=%.1f %" G_GINT64_FORMAT,temp,t);
            g_string_append_printf(body,"\n");
            g_string_append_printf(body,"hum,channel=%d,channel_name=%s,room=%s",channel,channel_name,room);
            g_string_append_printf(body," value=%.1f %" G_GINT64_FORMAT,hum,t);
            g_string_append_printf(body,"\n");
            
            if(debug)
              g_message(body->str);
            
            if(!testing) {
                soup_message_set_request(message,"application/binary",SOUP_MEMORY_COPY,body->str,body->len);
                guint session_status = soup_session_send_message(session,message);
                if(session_status == 204) { /* message was received */
                    //g_print("received status %d\n",session_status);
                    g_string_erase(body,0,-1); /* clear the string */
                }
                else {
                  g_print("no connection to server");
                }
                /* otherwise, keep it and attempt to resend next time */
            }
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
        if(testing && count>60)
          break;
        g_usleep(SLEEP_TIME*1000000);
    }
    if(session) {
      g_string_free(uri,TRUE);
      g_string_free(body,TRUE);
      g_object_unref(session);
    }
    hid_close(handle);
    hid_exit();
    
    return 0;
}
