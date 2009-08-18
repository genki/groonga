/* -*- c-basic-offset: 2; coding: utf-8 -*- */
/*
  Copyright (C) 2009  Yuto HAYAMIZU <y.hayamizu@gmail.com>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License version 2.1 as published by the Free Software Foundation.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <glib/gstdio.h>
#include <libsoup/soup.h>
#include <gcutter.h> /* must be included after memcached.h */

#include <unistd.h> /* for exec */
#include <sys/types.h>
#include <signal.h>

#include "../lib/grn-assertions.h"

#define GROONGA_TEST_PORT "4545"

/* globals */
static gchar *tmp_directory;

static GCutEgg *egg;

static SoupSession *session;

static SoupMessage *message;

void
cut_setup(void)
{
  GError *error = NULL;

  tmp_directory = g_build_filename(grn_test_get_base_dir(), "tmp", NULL);
  cut_remove_path(tmp_directory, NULL);
  if (g_mkdir_with_parents(tmp_directory, 0700) == -1) {
    cut_assert_errno();
  }

  session = NULL;
  message = NULL;
  
  egg = gcut_egg_new(GROONGA, "-s",
                     "-p", GROONGA_TEST_PORT,
                     "-n",
                     cut_take_printf("%s%s%s",
                                     tmp_directory,
                                     G_DIR_SEPARATOR_S,
                                     "http.db"),
                     NULL);
  gcut_egg_hatch(egg, &error);
  gcut_assert_error(error);

  session = soup_session_sync_new();

  sleep(1);
}

void
cut_teardown(void)
{
  if (egg) {
    g_object_unref(egg);
  }

  if (message) {
    g_object_unref(message);
  }

  if (session) {
    g_object_unref(session);
  }
  
  cut_remove_path(tmp_directory, NULL);
}

static void
assert_get(const char *path)
{
  guint status;
  
  message = soup_message_new("GET", cut_take_printf("%s%s", "http://localhost:" GROONGA_TEST_PORT, path));
    
  status = soup_session_send_message(session, message);

  cut_assert_equal_uint(SOUP_STATUS_OK, status);
}

void
test_get_root(void)
{
  assert_get("/");
  
  cut_assert_equal_string("text/javascript", soup_message_headers_get_content_type(message->response_headers, NULL));
  cut_assert_equal_memory("", 0, message->response_body->data, message->response_body->length);
}

void
test_get_status(void)
{
  assert_get("/status");
  
  cut_assert_equal_string("text/javascript", soup_message_headers_get_content_type(message->response_headers, NULL));
  cut_assert_match("{\"starttime\":\\d+,\"uptime\":\\d+}", message->response_body->data);
}

void
test_get_table_list(void)
{
  assert_get("/table_list");

  cut_assert_equal_string("text/javascript", soup_message_headers_get_content_type(message->response_headers, NULL));
  cut_assert_equal_string("[[\"id\",\"name\",\"path\",\"flags\",\"domain\"]]", message->response_body->data);
}
