/*  XMMS2 - X Music Multiplexer System
 *  Copyright (C) 2003	Peter Alm, Tobias Rundstr�m, Anders Gustafsson
 * 
 *  PLUGINS ARE NOT CONSIDERED TO BE DERIVED WORK !!!
 * 
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *                   
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 */




#ifndef __XMMS_TRANSPORT_H__
#define __XMMS_TRANSPORT_H__

#include <glib.h>
#include "xmms/xmms_plugin.h"
#include "xmms/xmms_error.h"
#include "xmms/xmms_medialib.h"

typedef struct xmms_transport_St xmms_transport_t;

#define XMMS_TRANSPORT_SEEK_SET 0
#define XMMS_TRANSPORT_SEEK_END 1
#define XMMS_TRANSPORT_SEEK_CUR 2

gint xmms_transport_read (xmms_transport_t *transport, gchar *buffer, guint len, xmms_error_t *error);
gint xmms_transport_peek (xmms_transport_t *transport, gchar *buffer, guint len, xmms_error_t *error);
guint64 xmms_transport_size (xmms_transport_t *transport);
guint xmms_transport_buffersize (xmms_transport_t *transport);
const gchar *xmms_transport_url_get (const xmms_transport_t *const transport);
xmms_medialib_entry_t xmms_transport_medialib_entry_get (const xmms_transport_t *const transport);
void xmms_transport_buffering_start (xmms_transport_t *transport);

#define XMMS_TRANSPORT_MAX_LINE_SIZE 1024
gchar *xmms_transport_read_line (xmms_transport_t *transport, gchar *line, xmms_error_t *err);

xmms_plugin_t *xmms_transport_plugin_get (const xmms_transport_t *transport);

gboolean xmms_transport_seek (xmms_transport_t *transport, gint offset, gint whence);

gboolean xmms_transport_can_seek (xmms_transport_t *transport);
gboolean xmms_transport_islocal (xmms_transport_t *transport);
gboolean xmms_transport_isstream (xmms_transport_t *transport);
gboolean xmms_transport_iseos (xmms_transport_t *transport);

guint64 xmms_transport_tell (xmms_transport_t *transport); 

#endif
