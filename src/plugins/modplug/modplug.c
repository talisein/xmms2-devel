/*  XMMS2 - X Music Multiplexer System
 *  Copyright (C) 2003	Peter Alm, Tobias Rundstr�m, Anders Gustafsson
 * 
 *  PLUGINS ARE NOT CONSIDERED TO BE DERIVED WORK !!!
 */





#include "xmms/xmms.h"
#include "xmms/plugin.h"
#include "xmms/decoder.h"
#include "xmms/util.h"
#include "xmms/playlist.h"
#include "xmms/transport.h"
#include <modplug.h>

#include <glib.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>

/*
 * Type definitions
 */

typedef struct xmms_modplug_data_St {
	ModPlug_Settings settings;
	ModPlugFile *mod;
	gchar *buffer;
	gint buffer_length;
} xmms_modplug_data_t;

/*
 * Function prototypes
 */

static gboolean xmms_modplug_can_handle (const gchar *mimetype);
static gboolean xmms_modplug_new (xmms_decoder_t *decoder, const gchar *mimetype);
static gboolean xmms_modplug_decode_block (xmms_decoder_t *decoder);
static void xmms_modplug_get_media_info (xmms_decoder_t *decoder);
static void xmms_modplug_destroy (xmms_decoder_t *decoder);
static gboolean xmms_modplug_init (xmms_decoder_t *decoder);
static gboolean xmms_modplug_seek (xmms_decoder_t *decoder, guint samples);

/*
 * Plugin header
 */

xmms_plugin_t *
xmms_plugin_get (void)
{
	xmms_plugin_t *plugin;

	plugin = xmms_plugin_new (XMMS_PLUGIN_TYPE_DECODER, "modplug",
			"MODPLUG decoder " XMMS_VERSION,
			"modplug");

	xmms_plugin_info_add (plugin, "URL", "http://www.xmms.org/");
	xmms_plugin_info_add (plugin, "Author", "XMMS Team");
	xmms_plugin_info_add (plugin, "License", "GPL");

	xmms_plugin_method_add (plugin, XMMS_PLUGIN_METHOD_CAN_HANDLE, xmms_modplug_can_handle);
	xmms_plugin_method_add (plugin, XMMS_PLUGIN_METHOD_NEW, xmms_modplug_new);
	xmms_plugin_method_add (plugin, XMMS_PLUGIN_METHOD_DECODE_BLOCK, xmms_modplug_decode_block);
	xmms_plugin_method_add (plugin, XMMS_PLUGIN_METHOD_DESTROY, xmms_modplug_destroy);
	xmms_plugin_method_add (plugin, XMMS_PLUGIN_METHOD_GET_MEDIAINFO, xmms_modplug_get_media_info);
	xmms_plugin_method_add (plugin, XMMS_PLUGIN_METHOD_INIT, xmms_modplug_init);
	xmms_plugin_method_add (plugin, XMMS_PLUGIN_METHOD_SEEK, xmms_modplug_seek);

	xmms_plugin_properties_add (plugin, XMMS_PLUGIN_PROPERTY_FAST_FWD);
	xmms_plugin_properties_add (plugin, XMMS_PLUGIN_PROPERTY_REWIND);

	return plugin;
}

static void
xmms_modplug_destroy (xmms_decoder_t *decoder)
{
	xmms_modplug_data_t *data;

	g_return_if_fail (decoder);

	data = xmms_decoder_private_data_get (decoder);
	g_return_if_fail (data);

	
	if (data->buffer)
		g_free (data->buffer);

	if (data->mod)
		ModPlug_Unload (data->mod);

	g_free (data);

}

static void
xmms_modplug_get_media_info (xmms_decoder_t *decoder)
{
	xmms_playlist_entry_t *entry;
	xmms_modplug_data_t *data;
	gchar tmp[256];

	g_return_if_fail (decoder);

	if (!xmms_modplug_init (decoder))
		return;

	data = xmms_decoder_private_data_get (decoder);

	entry = xmms_playlist_entry_new (NULL);

	/* */
	g_snprintf (tmp, sizeof (tmp), "%d", ModPlug_GetLength (data->mod));
	xmms_playlist_entry_property_set (entry,
	                                  XMMS_PLAYLIST_ENTRY_PROPERTY_DURATION,
	                                  tmp);

	g_snprintf (tmp, sizeof (tmp), "%s", ModPlug_GetName (data->mod));
	xmms_playlist_entry_property_set (entry,
					  XMMS_PLAYLIST_ENTRY_PROPERTY_TITLE,
					  tmp);
	xmms_decoder_entry_mediainfo_set (decoder, entry);

	xmms_object_unref (entry);

	return;
}

static gboolean
xmms_modplug_seek (xmms_decoder_t *decoder, guint samples)
{
	xmms_modplug_data_t *data;
	
	g_return_val_if_fail (decoder, FALSE);

	data = xmms_decoder_private_data_get (decoder);

	ModPlug_Seek (data->mod, (int) ((gdouble)1000 * samples / 44100));

	return TRUE;
}

static gboolean
xmms_modplug_can_handle (const gchar *mimetype)
{
	g_return_val_if_fail (mimetype, FALSE);
	
	if ((g_strcasecmp (mimetype, "audio/xm") == 0))
		return TRUE;

	if ((g_strcasecmp (mimetype, "audio/s3m") == 0))
		return TRUE;

	if ((g_strcasecmp (mimetype, "audio/it") == 0))
		return TRUE;

	if ((g_strcasecmp (mimetype, "audio/mod") == 0))
		return TRUE;

	if ((g_strcasecmp (mimetype, "audio/med") == 0))
		return TRUE;

	return FALSE;

}

static gboolean
xmms_modplug_new (xmms_decoder_t *decoder, const gchar *mimetype)
{
	xmms_modplug_data_t *data;

	g_return_val_if_fail (decoder, FALSE);
	g_return_val_if_fail (mimetype, FALSE);

	data = g_new0 (xmms_modplug_data_t, 1);

	xmms_decoder_private_data_set (decoder, data);
	
	return TRUE;
}

static gboolean
xmms_modplug_init (xmms_decoder_t *decoder)
{
	xmms_transport_t *transport;
	xmms_modplug_data_t *data;
	gint len = 0;

	g_return_val_if_fail (decoder, FALSE);
	
	transport = xmms_decoder_transport_get (decoder);
	g_return_val_if_fail (transport, FALSE);
	
	data = xmms_decoder_private_data_get (decoder);
	g_return_val_if_fail (decoder, FALSE);


	data->settings.mResamplingMode = MODPLUG_RESAMPLE_FIR;
	data->settings.mChannels = 2;
	data->settings.mBits = 16;
	data->settings.mFrequency = 44100;
	/* more? */
	ModPlug_SetSettings(&data->settings);


	data->buffer_length = xmms_transport_size (transport);
	data->buffer = g_malloc (data->buffer_length);
		
	while (len < data->buffer_length) {
		gint ret = xmms_transport_read (transport, data->buffer + len,
						data->buffer_length);
		
		if ( ret < 0 ) {
			g_free (data->buffer);
			data->buffer = NULL;
			return FALSE;
		}
		len += ret;
		g_assert (len >= 0);
	}
	
	data->mod = ModPlug_Load(data->buffer, data->buffer_length);
	if (!data->mod) {
		XMMS_DBG ("Error loading mod");
		return FALSE;
	}

	return TRUE;
}


static gboolean
xmms_modplug_decode_block (xmms_decoder_t *decoder)
{
	xmms_modplug_data_t *data;
	gint16 out[4096];
	gint ret;

	data = xmms_decoder_private_data_get (decoder);

	ret = ModPlug_Read(data->mod, out, 4096);

	if (!ret)
		return FALSE; /* eos */

	xmms_decoder_write (decoder, (gchar *)out, ret);

	return TRUE;
}

