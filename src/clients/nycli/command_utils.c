/*  XMMS2 - X Music Multiplexer System
 *  Copyright (C) 2003-2013 XMMS2 Team
 *
 *  PLUGINS ARE NOT CONSIDERED TO BE DERIVED WORK !!!
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 */

#include "command_utils.h"

#include "cli_infos.h"

#define command_arg_get(ctx, at) (ctx)->argv[(at) + 1]

gboolean
command_flag_boolean_get (command_context_t *ctx, const gchar *name, gboolean *v)
{
	command_argument_t *arg;
	gboolean retval = FALSE;

	arg = (command_argument_t *) g_hash_table_lookup (ctx->flags, name);
	if (arg && arg->type == COMMAND_ARGUMENT_TYPE_BOOLEAN) {
		*v = arg->value.vbool;
		retval = TRUE;
	}

	return retval;
}

gboolean
command_flag_int_get (command_context_t *ctx, const gchar *name, gint *v)
{
	command_argument_t *arg;
	gboolean retval = FALSE;

	/* A negative value means the value was not set. */
	arg = (command_argument_t *) g_hash_table_lookup (ctx->flags, name);
	if (arg && arg->type == COMMAND_ARGUMENT_TYPE_INT && arg->value.vint >= 0) {
		*v = arg->value.vint;
		retval = TRUE;
	}

	return retval;
}

gboolean
command_flag_string_get (command_context_t *ctx, const gchar *name, const gchar **v)
{
	command_argument_t *arg;
	gboolean retval = FALSE;

	arg = (command_argument_t *) g_hash_table_lookup (ctx->flags, name);
	if (arg && arg->type == COMMAND_ARGUMENT_TYPE_STRING && arg->value.vstring) {
		*v = arg->value.vstring;
		retval = TRUE;
	}

	return retval;
}

/* Extract the flag value as a list of string items.
 * Warning: the resulting string must be freed using g_strfreev() !*/
gboolean
command_flag_stringlist_get (command_context_t *ctx, const gchar *name, const gchar ***v)
{
	const gchar *full;
	gboolean retval = FALSE;

	if (command_flag_string_get (ctx, name, &full) && full) {
		/* Force cast to suppress warning, Don't panic! */
		*v = (const gchar **) g_strsplit (full, ",", MAX_STRINGLIST_TOKENS);
		retval = TRUE;
	}

	return retval;
}

gboolean
command_flag_stringarray_get (command_context_t *ctx, const gchar *name, const gchar ***v)
{
	command_argument_t *arg;
	gboolean retval = FALSE;

	arg = (command_argument_t *) g_hash_table_lookup (ctx->flags, name);
	if (arg && arg->type == COMMAND_ARGUMENT_TYPE_STRING_ARRAY && arg->value.vstringv) {
		*v = (const gchar **) arg->value.vstringv;
		retval = TRUE;
	}

	return retval;
}

gchar *
command_name_get (command_context_t *ctx)
{
	return ctx->name;
}

gint
command_arg_count (command_context_t *ctx)
{
	return ctx->argc - 1;
}

gchar **
command_argv_get (command_context_t *ctx)
{
	return ctx->argv + 1;
}

gboolean
command_arg_int_get (command_context_t *ctx, gint at, gint *v)
{
	gboolean retval = FALSE;

	if (at < command_arg_count (ctx)) {
		*v = strtol (command_arg_get (ctx, at), NULL, 10);
		retval = TRUE;
	}

	return retval;
}

gboolean
command_arg_string_get (command_context_t *ctx, gint at, const gchar **v)
{
	gboolean retval = FALSE;

	if (at < command_arg_count (ctx)) {
		*v = command_arg_get (ctx, at);
		retval = TRUE;
	}

	return retval;
}

/* Grab all the arguments after the index as a single string.
 * Warning: the string must be freed manually afterwards!
 */
gboolean
command_arg_longstring_get (command_context_t *ctx, gint at, gchar **v)
{
	gboolean retval = FALSE;

	if (at < command_arg_count (ctx)) {
		*v = g_strjoinv (" ", &(command_arg_get (ctx, at)));
		retval = TRUE;
	}

	return retval;
}

/* Escape characters in toescape with escape_char.
 */
static gchar *
strescape (gchar *s, const gchar *toescape, gchar escape_char)
{
	gint len;
	gchar *t, *r;

	t = s;
	for (len = 0; *t != '\0'; len++, t++) {
		if (strchr (toescape, *t)) {
			len++;
		}
	}
	r = g_new0 (gchar, len+1);
	t = r;
	while (*s) {
		if (strchr (toescape, *s)) {
			*t = escape_char;
			t++;
		}
		*t = *s;
		s++;
		t++;
	}

	return r;
}

/* Like command_arg_longstring_get but escape spaces with '\'.
 */
gboolean
command_arg_longstring_get_escaped (command_context_t *ctx, gint at, gchar **v)
{
	gboolean retval = FALSE;
	gchar **args;
	gint i, len, count = command_arg_count (ctx);

	len = count-at+1;
	if (at < count) {
		args = g_new0 (gchar *, len);
		args[len-1] = NULL;
		for (i = at; i < count; i++) {
			args[i-at] = strescape (command_arg_get (ctx, i), " ", '\\');
		}
		*v = g_strjoinv (" ", args);

		for (i = at; i < count; i++) {
			g_free (args[i-at]);
		}
		g_free (args);

		retval = TRUE;
	}

	return retval;
}

static guint
parse_time_sep (const gchar *s, gchar **endptr)
{
	gint i;
	guint v;
	const gchar *t;

	v = 0;
	t = s;
	for (i = 0; i < 3 && *t != '\0'; i++) {
		v = v*60 + strtol (t, endptr, 10);
		t = *endptr+1;
	}

	return v;
}

/*
 * Parse time expressions of the form:
 *
 * e0: [0-9]*:[0-9]*:[0-9]
 * e1: ([0-9]*(hour|h|min|m|sec|s))*
 *
 * RFC: Be more restrictive?
 *
 *    Actually it accepts expressions like:
 *         1hour2min1hour1s for 2hour2min1s
 *         1min2hour7sec for 2hour1min7sec
 *
 */
static guint
parse_time (gchar *s, gchar **endptr, const gint *mul, const gchar **sep)
{
	gint i;
	guint n, v;

	if (strchr (s, ':') != NULL) {
		return parse_time_sep (s, endptr);
	}

	n = 0;
	v = 0;
	while (*s) {
		if ('0' <= *s && *s <= '9') {
			n = n*10 + (*s - '0');
			s++;
		} else {
			for (i = 0; sep[i] != NULL; i++) {
				if (g_str_has_prefix (s, sep[i])) {
					v += mul[i]*n;
					s += strlen (sep[i]);
					n = 0;
					break;
				}
			}
			if (sep[i] == NULL) {
				break;
			}
		}
	}
	v += n;
	*endptr = s;

	return v;
}

/* Parse a time value, either an absolute position or an offset. */
gboolean
command_arg_time_get (command_context_t *ctx, gint at, command_arg_time_t *v)
{
	gboolean retval = FALSE;
	const gchar *s;
	gchar *endptr;

	const gchar *separators[] = {"hour", "h", "min", "m", "sec", "s", NULL};
	const gint multipliers[] = {3600, 3600, 60, 60, 1, 1};

	if (at < command_arg_count (ctx) && command_arg_string_get (ctx, at, &s)) {
		gchar *time_arg = g_strdup (s);
		if (*time_arg == '+' || *time_arg == '-') {
			v->type = COMMAND_ARG_TIME_OFFSET;
			/* v->value.offset = strtol (s, &endptr, 10); */
			v->value.offset = parse_time (time_arg + 1, &endptr, multipliers, separators);
			if (*time_arg == '-') {
				v->value.offset = -v->value.offset;
			}
		} else {
			/* FIXME: always signed long int anyway? */
			v->type = COMMAND_ARG_TIME_POSITION;
			/* v->value.pos = strtol (s, &endptr, 10); */
			v->value.pos = parse_time (time_arg, &endptr, multipliers, separators);
		}

		/* FIXME: We can have cleverer parsing for '2:17' or '3min' etc */
		if (*endptr == '\0') {
			retval = TRUE;
		}

		g_free (time_arg);
	}

	return retval;
}

/** Try to parse a collection pattern from the arguments and return
 *  success status. The resulting coll structure is saved to the v
 *  argument. In case of error, the error message is printed on
 *  stdout.
 */
gboolean
command_arg_pattern_get (command_context_t *ctx, gint at, xmmsc_coll_t **v,
                         gboolean warn)
{
	gchar *pattern = NULL;
	gboolean success = TRUE;

	/* FIXME: Need a more elegant error system. */
	/* FIXME(g): Escape tokens? command_arg_longstring_get_escaped ? */

	command_arg_longstring_get_escaped (ctx, at, &pattern);
	if (!pattern) {
		if (warn) g_printf (_("Error: you must provide a pattern!\n"));
		success = FALSE;
	} else if (!xmmsc_coll_parse (pattern, v)) {
		if (warn) g_printf (_("Error: failed to parse the pattern!\n"));
		success = FALSE;
	}

	g_free (pattern);

	return success;
}

/** Try to parse a positions expression from the arguments and return
 *  success status. The resulting positions structure is saved to the p
 *  argument.
 */
gboolean
command_arg_positions_get (command_context_t *ctx, gint at,
                           playlist_positions_t **p, gint currpos)
{
	gchar *expression = NULL;
	gboolean success = TRUE;

	command_arg_longstring_get_escaped (ctx, at, &expression);
	if (!expression || !playlist_positions_parse (expression, p, currpos)) {
		success = FALSE;
	}

	g_free (expression);

	return success;
}

/**
 * Apply the default ordering to a collection query, that is, its results will
 * be ordered by artist, and then album. To make this result more readable,
 * compilation albums are placed at the end to get proper album grouping.
 *
 * @param query A collection to be ordered.
 * @return The input collection with the default ordering.
 */
xmmsv_coll_t *
coll_apply_default_order (xmmsv_coll_t *query)
{
	xmmsv_coll_t *compilation, *compilation_sorted;
	xmmsv_coll_t *regular, *regular_sorted;
	xmmsv_coll_t *complement, *concatenated;
	xmmsv_t *fields, *artist_order, *compilation_order, *regular_order;

	/* All various artists entries that match the user query. */
	compilation = xmmsv_coll_new (XMMS_COLLECTION_TYPE_MATCH);
	xmmsv_coll_add_operand (compilation, query);
	xmmsv_coll_attribute_set (compilation, "field", "compilation");
	xmmsv_coll_attribute_set (compilation, "value", "1");

	/* All entries that aren't various artists, or don't match the user query */
	complement = xmmsv_coll_new (XMMS_COLLECTION_TYPE_COMPLEMENT);
	xmmsv_coll_add_operand (complement, compilation);

	/* All entries that aren't various artists, and match the user query */
	regular = xmmsv_coll_new (XMMS_COLLECTION_TYPE_INTERSECTION);
	xmmsv_coll_add_operand (regular, query);
	xmmsv_coll_add_operand (regular, complement);
	xmmsv_coll_unref (complement);

	compilation_order = xmmsv_build_list (
	        XMMSV_LIST_ENTRY_STR ("album"),
	        XMMSV_LIST_ENTRY_STR ("partofset"),
	        XMMSV_LIST_ENTRY_STR ("tracknr"),
	        XMMSV_LIST_END);

	compilation_sorted = xmmsv_coll_add_order_operators (compilation,
	                                                     compilation_order);
	xmmsv_coll_unref (compilation);
	xmmsv_unref (compilation_order);

	fields = xmmsv_build_list (XMMSV_LIST_ENTRY_STR ("album_artist_sort"),
	                           XMMSV_LIST_ENTRY_STR ("album_artist"),
	                           XMMSV_LIST_ENTRY_STR ("artist_sort"),
	                           XMMSV_LIST_ENTRY_STR ("artist"),
	                           XMMSV_LIST_END);

	artist_order = xmmsv_build_dict (XMMSV_DICT_ENTRY_STR ("type", "value"),
	                                 XMMSV_DICT_ENTRY ("field", fields),
	                                 XMMSV_DICT_END);

	regular_order = xmmsv_build_list (
	        XMMSV_LIST_ENTRY_STR ("album"),
	        XMMSV_LIST_ENTRY_STR ("partofset"),
	        XMMSV_LIST_ENTRY_STR ("tracknr"),
	        XMMSV_LIST_END);

	xmmsv_list_insert (regular_order, 0, artist_order);
	xmmsv_unref (artist_order);

	regular_sorted = xmmsv_coll_add_order_operators (regular, regular_order);
	xmmsv_coll_unref (regular);
	xmmsv_unref (regular_order);

	concatenated = xmmsv_coll_new (XMMS_COLLECTION_TYPE_UNION);
	xmmsv_coll_add_operand (concatenated, regular_sorted);
	xmmsv_coll_unref (regular_sorted);
	xmmsv_coll_add_operand (concatenated, compilation_sorted);
	xmmsv_coll_unref (compilation_sorted);

	return concatenated;
}
