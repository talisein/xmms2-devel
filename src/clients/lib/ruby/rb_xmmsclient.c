/*  XMMS2 - X Music Multiplexer System
 *  Copyright (C) 2003, 2004 Peter Alm, Tobias Rundstr�m, Anders Gustafsson
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

#include <xmmsclient/xmmsclient.h>
#include <xmms/xmms_output.h>

#include <ruby.h>
#include <stdbool.h>

#include "rb_xmmsclient.h"
#include "rb_result.h"

#define METHOD_ADD_HANDLER(name, unref) \
	RbXmmsClient *xmms = NULL; \
	xmmsc_result_t *res; \
	VALUE o; \
\
	Data_Get_Struct (self, RbXmmsClient, xmms); \
\
	res = xmmsc_##name (xmms->real); \
\
	o = TO_XMMS_CLIENT_RESULT (res, true, unref); \
	rb_ary_push (xmms->results, o); \
\
	return o; \

void Init_Result (VALUE m, VALUE e);

static VALUE eXmmsClientError;

static void c_mark (RbXmmsClient *xmms)
{
	rb_gc_mark (xmms->results);

	if (!NIL_P (xmms->disconnect_cb))
		rb_gc_mark (xmms->disconnect_cb);
}

static void c_free (RbXmmsClient *xmms)
{
	xmmsc_unref (xmms->real);

	free (xmms);
}

static VALUE c_alloc (VALUE klass)
{
	RbXmmsClient *xmms;

	return Data_Make_Struct (klass, RbXmmsClient,
	                         c_mark, c_free, xmms);
}

/*
 * call-seq:
 *  XmmsClient::XmmsClient.new(name) -> xc
 *
 * Creates an XmmsClient::XmmsClient object.
 */
static VALUE c_init (VALUE self, VALUE name)
{
	RbXmmsClient *xmms = NULL;

	Data_Get_Struct (self, RbXmmsClient, xmms);

	if (!(xmms->real = xmmsc_init (StringValuePtr (name)))) {
		rb_raise (rb_eNoMemError, "failed to allocate memory");
		return Qnil;
	}

	xmms->results = rb_ary_new ();
	xmms->disconnect_cb = Qnil;

	return self;
}

/*
 * call-seq:
 *  xc.connect([path]) -> self
 *
 * Connects _xc_ to the XMMS2 daemon listening at _path_.
 * If _path_ isn't given, the default path is used.
 */
static VALUE c_connect (int argc, VALUE *argv, VALUE self)
{
	VALUE path;
	RbXmmsClient *xmms = NULL;
	char *p = NULL;

	Data_Get_Struct (self, RbXmmsClient, xmms);

	rb_scan_args (argc, argv, "01", &path);

	if (!NIL_P (path))
		p = StringValuePtr (path);

	if (!xmmsc_connect (xmms->real, p))
		rb_raise (eXmmsClientError, "cannot connect to daemon");

	return self;
}

/*
 * call-seq:
 *  xc.disconnect -> self
 *
 * Disconnects _xc_ from the XMMS2 daemon.
 */
static VALUE c_disconnect (VALUE self)
{
	RbXmmsClient *xmms = NULL;

	Data_Get_Struct (self, RbXmmsClient, xmms);

	xmmsc_disconnect (xmms->real);

	return self;
}

static void on_disconnect (void *data)
{
	VALUE self = (VALUE) data;
	RbXmmsClient *xmms = NULL;

	Data_Get_Struct (self, RbXmmsClient, xmms);

	rb_funcall (xmms->disconnect_cb, rb_intern ("call"), 0);
}

/*
 * call-seq:
 *  xc.on_disconnect { }
 *
 * Sets the block that's executed when _xc_ is disconnected from the
 * XMMS2 daemon.
 */
static VALUE c_on_disconnect (VALUE self)
{
	RbXmmsClient *xmms = NULL;

	if (!rb_block_given_p ())
		return Qnil;

	Data_Get_Struct (self, RbXmmsClient, xmms);

	xmms->disconnect_cb = rb_block_proc ();

	xmmsc_disconnect_callback_set (xmms->real,
	                               on_disconnect, (void *) self);

	return self;
}

/*
 * call-seq:
 *  xc.last_error -> string or nil
 *
 * Returns the last error that occured in _xc_ or +nil+, if no error
 * occured yet.
 */
static VALUE c_last_error_get (VALUE self)
{
	RbXmmsClient *xmms = NULL;
	const char *s;

	Data_Get_Struct (self, RbXmmsClient, xmms);

	s = xmmsc_get_last_error (xmms->real);

	return s ? rb_str_new2 (s) : Qnil;
}

/*
 * call-seq:
 *  xc.quit -> result
 *
 * Shuts down the XMMS2 daemon.
 */
static VALUE c_quit (VALUE self)
{
	METHOD_ADD_HANDLER (quit, true);
}

/*
 * call-seq:
 *  xc.playback_start -> result
 *
 * Starts playback.
 */
static VALUE c_playback_start (VALUE self)
{
	METHOD_ADD_HANDLER (playback_start, true);
}

/*
 * call-seq:
 *  xc.playback_pause -> result
 *
 * Pauses playback.
 */
static VALUE c_playback_pause (VALUE self)
{
	METHOD_ADD_HANDLER (playback_pause, true);
}

/*
 * call-seq:
 *  xc.playback_stop -> result
 *
 * Stops playback.
 */
static VALUE c_playback_stop (VALUE self)
{
	METHOD_ADD_HANDLER (playback_stop, true);
}

/*
 * call-seq:
 *  xc.playback_tickle -> result
 *
 * Advances to the next playlist entry.
 */
static VALUE c_playback_tickle (VALUE self)
{
	METHOD_ADD_HANDLER (playback_tickle, true);
}

/*
 * call-seq:
 *  xc.playback_status -> result
 *
 * Retrieves the playback status.
 */
static VALUE c_playback_status (VALUE self)
{
	METHOD_ADD_HANDLER (playback_status, false);
}

/*
 * call-seq:
 *  xc.broadcast_playback_status -> result
 *
 * Retrieves the playback status as a broadcast.
 */
static VALUE c_broadcast_playback_status (VALUE self)
{
	METHOD_ADD_HANDLER (broadcast_playback_status, false);
}

/*
 * call-seq:
 *  xc.playback_playtime -> result
 *
 * Retrieves the playtime.
 */
static VALUE c_playback_playtime (VALUE self)
{
	METHOD_ADD_HANDLER (playback_playtime, true);
}

/*
 * call-seq:
 *  xc.signal_playback_playtime -> result
 *
 * Retrieves the playtime as a signal.
 */
static VALUE c_signal_playback_playtime (VALUE self)
{
	METHOD_ADD_HANDLER (signal_playback_playtime, true);
}

/*
 * call-seq:
 *  xc.playback_current_id -> result
 *
 * Retrieves the media id of the currently played track.
 */
static VALUE c_playback_current_id (VALUE self)
{
	METHOD_ADD_HANDLER (playback_current_id, true);
}

/*
 * call-seq:
 *  xc.broadcast_playback_current_id -> result
 *
 * Retrieves the media id of the currently played track as a broadcast.
 */
static VALUE c_broadcast_playback_current_id (VALUE self)
{
	METHOD_ADD_HANDLER (broadcast_playback_current_id, false);
}

/*
 * call-seq:
 *  xc.broadcast_playback_current_id -> result
 *
 * Retrieves the media id of the currently played track as a broadcast.
 */
static VALUE c_broadcast_configval_changed (VALUE self)
{
	METHOD_ADD_HANDLER (broadcast_configval_changed, false);
}

/*
 * call-seq:
 *  xc.playback_seek_ms(ms) -> result
 *
 * Seek to the song position given in ms.
 */
static VALUE c_playback_seek_ms (VALUE self, VALUE ms)
{
	VALUE o;
	RbXmmsClient *xmms = NULL;
	xmmsc_result_t *res;

	Data_Get_Struct (self, RbXmmsClient, xmms);

	Check_Type (ms, T_FIXNUM);

	res = xmmsc_playback_seek_ms (xmms->real, NUM2UINT (ms));

	o = TO_XMMS_CLIENT_RESULT (res, true, true);
	rb_ary_push (xmms->results, o);

	return o;
}

/*
 * call-seq:
 *  xc.playback_seek_samples(samples) -> result
 *
 * Seek to the song position given in samples.
 */
static VALUE c_playback_seek_samples (VALUE self, VALUE samples)
{
	VALUE o;
	RbXmmsClient *xmms = NULL;
	xmmsc_result_t *res;

	Data_Get_Struct (self, RbXmmsClient, xmms);

	Check_Type (samples, T_FIXNUM);

	res = xmmsc_playback_seek_samples (xmms->real, NUM2UINT (samples));

	o = TO_XMMS_CLIENT_RESULT (res, true, true);
	rb_ary_push (xmms->results, o);

	return o;
}

static VALUE c_broadcast_playlist_changed (VALUE self)
{
	METHOD_ADD_HANDLER(broadcast_playlist_changed, false);
}

static VALUE c_playlist_current_pos (VALUE self)
{
	METHOD_ADD_HANDLER(playlist_current_pos, true);
}

static VALUE c_broadcast_playlist_current_pos (VALUE self)
{
	METHOD_ADD_HANDLER(broadcast_playlist_current_pos, false);
}

static VALUE c_broadcast_medialib_entry_changed (VALUE self)
{
	METHOD_ADD_HANDLER(broadcast_medialib_entry_changed, false);
}

static VALUE c_playlist_shuffle (VALUE self)
{
	METHOD_ADD_HANDLER(playlist_shuffle, true);
}

static VALUE c_playlist_list (VALUE self)
{
	METHOD_ADD_HANDLER(playlist_list, true);
}

static VALUE c_playlist_clear (VALUE self)
{
	METHOD_ADD_HANDLER(playlist_clear, true);
}

static VALUE c_playlist_set_next (VALUE self, VALUE pos)
{
	VALUE o;
	RbXmmsClient *xmms = NULL;
	xmmsc_result_t *res;

	Data_Get_Struct (self, RbXmmsClient, xmms);

	Check_Type (pos, T_FIXNUM);

	res = xmmsc_playlist_set_next (xmms->real, FIX2INT (pos));

	o = TO_XMMS_CLIENT_RESULT (res, true, true);
	rb_ary_push (xmms->results, o);

	return o;
}

static VALUE c_playlist_set_next_rel (VALUE self, VALUE pos)
{
	VALUE o;
	RbXmmsClient *xmms = NULL;
	xmmsc_result_t *res;

	Data_Get_Struct (self, RbXmmsClient, xmms);

	Check_Type (pos, T_FIXNUM);

	res = xmmsc_playlist_set_next_rel (xmms->real, FIX2INT (pos));

	o = TO_XMMS_CLIENT_RESULT (res, true, true);
	rb_ary_push (xmms->results, o);

	return o;
}

static VALUE c_playlist_add (VALUE self, VALUE arg)
{
	VALUE o;
	RbXmmsClient *xmms = NULL;
	xmmsc_result_t *res;
	bool is_str;

	if (!NIL_P (rb_check_string_type (arg)))
		is_str = true;
	else if (rb_obj_is_kind_of (arg, rb_cFixnum))
		is_str = false;
	else {
		rb_raise (eXmmsClientError, "unsupported argument");
		return Qnil;
	}

	Data_Get_Struct (self, RbXmmsClient, xmms);

	if (is_str)
		res = xmmsc_playlist_add (xmms->real, StringValuePtr (arg));
	else
		res = xmmsc_playlist_add_id (xmms->real, NUM2UINT (arg));

	o = TO_XMMS_CLIENT_RESULT (res, true, true);
	rb_ary_push (xmms->results, o);

	return o;
}

static VALUE c_playlist_remove (VALUE self, VALUE pos)
{
	VALUE o;
	RbXmmsClient *xmms = NULL;
	xmmsc_result_t *res;

	Check_Type (pos, T_FIXNUM);

	Data_Get_Struct (self, RbXmmsClient, xmms);

	res = xmmsc_playlist_remove (xmms->real, NUM2UINT (pos));

	o = TO_XMMS_CLIENT_RESULT (res, true, true);
	rb_ary_push (xmms->results, o);

	return o;
}

static VALUE c_playlist_move (VALUE self, VALUE id, VALUE moves)
{
	VALUE o;
	RbXmmsClient *xmms = NULL;
	xmmsc_result_t *res;

	Check_Type (id, T_FIXNUM);
	Check_Type (moves, T_FIXNUM);

	Data_Get_Struct (self, RbXmmsClient, xmms);

	res = xmmsc_playlist_move (xmms->real, NUM2UINT (id),
	                           NUM2INT (moves));

	o = TO_XMMS_CLIENT_RESULT (res, true, true);
	rb_ary_push (xmms->results, o);

	return o;
}

static VALUE c_playlist_sort (VALUE self, VALUE property)
{
	VALUE o;
	RbXmmsClient *xmms = NULL;
	xmmsc_result_t *res;

	StringValue (property);

	Data_Get_Struct (self, RbXmmsClient, xmms);

	res = xmmsc_playlist_sort (xmms->real, StringValuePtr (property));

	o = TO_XMMS_CLIENT_RESULT (res, true, true);
	rb_ary_push (xmms->results, o);

	return o;
}

static VALUE c_medialib_select (VALUE self, VALUE query)
{
	VALUE o;
	RbXmmsClient *xmms = NULL;
	xmmsc_result_t *res;

	StringValue (query);

	Data_Get_Struct (self, RbXmmsClient, xmms);

	res = xmmsc_medialib_select (xmms->real, StringValuePtr (query));

	o = TO_XMMS_CLIENT_RESULT (res, true, true);
	rb_ary_push (xmms->results, o);

	return o;
}

static VALUE c_medialib_playlist_save_current (VALUE self, VALUE name)
{
	VALUE o;
	RbXmmsClient *xmms = NULL;
	xmmsc_result_t *res;

	StringValue (name);

	Data_Get_Struct (self, RbXmmsClient, xmms);

	res = xmmsc_medialib_playlist_save_current (xmms->real,
	                                            StringValuePtr (name));

	o = TO_XMMS_CLIENT_RESULT (res, true, true);
	rb_ary_push (xmms->results, o);

	return o;
}

static VALUE c_medialib_playlist_load (VALUE self, VALUE name)
{
	VALUE o;
	RbXmmsClient *xmms = NULL;
	xmmsc_result_t *res;

	StringValue (name);

	Data_Get_Struct (self, RbXmmsClient, xmms);

	res = xmmsc_medialib_playlist_load (xmms->real,
	                                    StringValuePtr (name));

	o = TO_XMMS_CLIENT_RESULT (res, true, true);
	rb_ary_push (xmms->results, o);

	return o;
}

static VALUE c_medialib_add_entry (VALUE self, VALUE url)
{
	VALUE o;
	RbXmmsClient *xmms = NULL;
	xmmsc_result_t *res;

	StringValue (url);

	Data_Get_Struct (self, RbXmmsClient, xmms);
	res = xmmsc_medialib_add_entry (xmms->real, StringValuePtr (url));

	o = TO_XMMS_CLIENT_RESULT (res, true, true);
	rb_ary_push (xmms->results, o);

	return o;
}

static VALUE c_medialib_get_info (VALUE self, VALUE id)
{
	VALUE o;
	RbXmmsClient *xmms = NULL;
	xmmsc_result_t *res;

	Data_Get_Struct (self, RbXmmsClient, xmms);

	Check_Type (id, T_FIXNUM);

	res = xmmsc_medialib_get_info (xmms->real, FIX2INT (id));

	o = TO_XMMS_CLIENT_RESULT (res, true, true);
	rb_ary_push (xmms->results, o);

	return o;
}

static VALUE c_medialib_add_to_playlist (VALUE self, VALUE query)
{
	VALUE o;
	RbXmmsClient *xmms = NULL;
	xmmsc_result_t *res;

	StringValue (query);

	Data_Get_Struct (self, RbXmmsClient, xmms);

	res = xmmsc_medialib_add_to_playlist (xmms->real,
	                                      StringValuePtr (query));

	o = TO_XMMS_CLIENT_RESULT (res, true, true);
	rb_ary_push (xmms->results, o);

	return o;
}

static VALUE c_medialib_playlist_import (VALUE self, VALUE playlist,
                                         VALUE url)
{
	VALUE o;
	RbXmmsClient *xmms = NULL;
	xmmsc_result_t *res;

	StringValue (playlist);
	StringValue (url);

	Data_Get_Struct (self, RbXmmsClient, xmms);

	res = xmmsc_medialib_playlist_import (xmms->real,
	                                      StringValuePtr (playlist),
	                                      StringValuePtr (url));

	o = TO_XMMS_CLIENT_RESULT (res, true, true);
	rb_ary_push (xmms->results, o);

	return o;
}

static VALUE c_medialib_playlist_export (VALUE self, VALUE playlist,
                                         VALUE mime)
{
	VALUE o;
	RbXmmsClient *xmms = NULL;
	xmmsc_result_t *res;

	StringValue (playlist);
	StringValue (mime);

	Data_Get_Struct (self, RbXmmsClient, xmms);
	res = xmmsc_medialib_playlist_import (xmms->real,
	                                      StringValuePtr (playlist),
	                                      StringValuePtr (mime));

	o = TO_XMMS_CLIENT_RESULT (res, true, true);
	rb_ary_push (xmms->results, o);

	return o;
}

static VALUE c_medialib_path_import (VALUE self, VALUE path)
{
	VALUE o;
	RbXmmsClient *xmms = NULL;
	xmmsc_result_t *res;

	StringValue (path);

	Data_Get_Struct (self, RbXmmsClient, xmms);

	res = xmmsc_medialib_path_import (xmms->real,
	                                  StringValuePtr (path));

	o = TO_XMMS_CLIENT_RESULT (res, true, true);
	rb_ary_push (xmms->results, o);

	return o;
}

static VALUE c_medialib_rehash (VALUE self, VALUE id)
{
	VALUE o;
	RbXmmsClient *xmms = NULL;
	xmmsc_result_t *res;

	Check_Type (id, T_FIXNUM);

	Data_Get_Struct (self, RbXmmsClient, xmms);

	res = xmmsc_medialib_rehash (xmms->real, FIX2INT (id));

	o = TO_XMMS_CLIENT_RESULT (res, true, true);
	rb_ary_push (xmms->results, o);

	return o;
}

static VALUE c_configval_get (VALUE self, VALUE key)
{
	VALUE o;
	RbXmmsClient *xmms = NULL;
	xmmsc_result_t *res;

	Data_Get_Struct (self, RbXmmsClient, xmms);

	Check_Type (key, T_STRING);

	res = xmmsc_configval_get (xmms->real, StringValuePtr (key));

	o = TO_XMMS_CLIENT_RESULT (res, true, true);
	rb_ary_push (xmms->results, o);

	return o;
}

static VALUE c_configval_set (VALUE self, VALUE key, VALUE val)
{
	VALUE o;
	RbXmmsClient *xmms = NULL;
	xmmsc_result_t *res;

	Data_Get_Struct (self, RbXmmsClient, xmms);

	Check_Type (key, T_STRING);
	Check_Type (val, T_STRING);

	res = xmmsc_configval_set (xmms->real, StringValuePtr (key),
	                           StringValuePtr (val));

	o = TO_XMMS_CLIENT_RESULT (res, true, true);
	rb_ary_push (xmms->results, o);

	return o;
}

static VALUE c_signal_visualisation_data (VALUE self)
{
	METHOD_ADD_HANDLER(signal_visualisation_data, true);
}

void Init_XmmsClient (VALUE mXmmsClient)
{
	VALUE c;

	c = rb_define_class_under (mXmmsClient, "XmmsClient", rb_cObject);

	rb_define_alloc_func (c, c_alloc);
	rb_define_method (c, "initialize", c_init, 1);
	rb_define_method (c, "connect", c_connect, -1);
	rb_define_method (c, "disconnect", c_disconnect, 0);
	rb_define_method (c, "on_disconnect", c_on_disconnect, 0);
	rb_define_method (c, "last_error", c_last_error_get, 0);

	rb_define_method (c, "quit", c_quit, 0);
	rb_define_method (c, "playback_start", c_playback_start, 0);
	rb_define_method (c, "playback_pause", c_playback_pause, 0);
	rb_define_method (c, "playback_stop", c_playback_stop, 0);
	rb_define_method (c, "playback_tickle", c_playback_tickle, 0);
	rb_define_method (c, "broadcast_playback_status",
	                  c_broadcast_playback_status, 0);
	rb_define_method (c, "playback_status", c_playback_status, 0);
	rb_define_method (c, "playback_playtime", c_playback_playtime, 0);
	rb_define_method (c, "signal_playback_playtime",
	                  c_signal_playback_playtime, 0);
	rb_define_method (c, "playback_current_id",
	                  c_playback_current_id, 0);
	rb_define_method (c, "broadcast_playback_current_id",
	                  c_broadcast_playback_current_id, 0);
	rb_define_method (c, "playback_seek_ms", c_playback_seek_ms, 1);
	rb_define_method (c, "playback_seek_samples",
	                  c_playback_seek_samples, 1);

	rb_define_method (c, "broadcast_playlist_changed",
	                  c_broadcast_playlist_changed, 0);
	rb_define_method (c, "playlist_current_pos",
	                  c_playlist_current_pos, 0);
	rb_define_method (c, "broadcast_playlist_current_pos",
	                  c_broadcast_playlist_current_pos, 0);
	rb_define_method (c, "broadcast_medialib_entry_changed",
	                  c_broadcast_medialib_entry_changed, 0);

	rb_define_method (c, "playlist_shuffle", c_playlist_shuffle, 0);
	rb_define_method (c, "playlist_list", c_playlist_list, 0);
	rb_define_method (c, "playlist_clear", c_playlist_clear, 0);
	rb_define_method (c, "playlist_set_next", c_playlist_set_next, 1);
	rb_define_method (c, "playlist_set_next_rel",
	                  c_playlist_set_next_rel, 1);
	rb_define_method (c, "playlist_add", c_playlist_add, 1);
	rb_define_method (c, "playlist_remove", c_playlist_remove, 1);
	rb_define_method (c, "playlist_move", c_playlist_move, 2);
	rb_define_method (c, "playlist_sort", c_playlist_sort, 1);

	rb_define_method (c, "medialib_select", c_medialib_select, 1);
	rb_define_method (c, "medialib_playlist_save_current",
	                  c_medialib_playlist_save_current, 1);
	rb_define_method (c, "medialib_playlist_load",
	                  c_medialib_playlist_load, 1);
	rb_define_method (c, "medialib_add_entry", c_medialib_add_entry, 1);
	rb_define_method (c, "medialib_get_info", c_medialib_get_info, 1);
	rb_define_method (c, "medialib_add_to_playlist",
	                  c_medialib_add_to_playlist, 1);
	rb_define_method (c, "medialib_playlist_import",
	                  c_medialib_playlist_import, 2);
	rb_define_method (c, "medialib_playlist_export",
	                  c_medialib_playlist_export, 2);
	rb_define_method (c, "medialib_path_import", c_medialib_path_import, 1);
	rb_define_method (c, "medialib_rehash", c_medialib_rehash, 1);

	rb_define_method (c, "signal_visualisation_data",
	                  c_signal_visualisation_data, 0);

	rb_define_method (c, "configval_get", c_configval_get, 1);
	rb_define_method (c, "configval_set", c_configval_set, 2);
	rb_define_method (c, "broadcast_configval_changed",
	                  c_broadcast_configval_changed, 0);

	rb_define_const (c, "PLAY",
	                 INT2FIX (XMMS_OUTPUT_STATUS_PLAY));
	rb_define_const (c, "STOP",
	                 INT2FIX (XMMS_OUTPUT_STATUS_STOP));
	rb_define_const (c, "PAUSE",
	                 INT2FIX (XMMS_OUTPUT_STATUS_PAUSE));

	eXmmsClientError = rb_define_class_under (mXmmsClient,
	                                          "XmmsClientError",
	                                          rb_eStandardError);

	Init_Result (mXmmsClient, eXmmsClientError);
}
