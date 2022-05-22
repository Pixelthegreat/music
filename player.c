/*
 * Copyright 2022 Elliot Kohlmyer
 *
 * This file is part of Music.
 *
 * Music is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * Music is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Music.  If not, see <https://www.gnu.org/licenses/>.
 */
/* music player widget */
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>
#include <gtk/gtk.h>
#include <gst/gst.h>
#include <cairo.h>
#include "player.h"

/* structure */
struct _MusicPlayer {
	GtkBin bin; /* parent */
	GtkWidget *box; /* main box */
	GtkWidget *playbtn; /* button to play */
	GtkWidget *pausebtn; /* button to pause */
	GtkWidget *stopbtn; /* button to stop */
	GtkWidget *pos; /* position in song */
	GtkWidget *volume; /* volume control */
	GstElement *playbin; /* gstreamer audio streaming element */
	GstBus *bus; /* bus for playbin */
	GstState state; /* state of playbin */
	char *current; /* current file */
	gint64 duration; /* duration of file */
	gulong pos_update_id; /* id for position update */
};

G_DEFINE_TYPE(MusicPlayer, music_player, GTK_TYPE_BIN)

static void _music_player_play(GtkWidget *, MusicPlayer *);
static void _music_player_pause(GtkWidget *, MusicPlayer *);
static void _music_player_stop(GtkWidget *, MusicPlayer *);

static int update(MusicPlayer *self) {

	/* not in paused or playing */
	if (self->state < GST_STATE_PAUSED || self->current == NULL)
		return 1;
	
	/* if we didn't get the duration already */
	if (!GST_CLOCK_TIME_IS_VALID(self->duration)) {

		/* get duration */
		gst_element_query_duration(self->playbin, GST_FORMAT_TIME, &self->duration);
		
		gtk_range_set_range(GTK_RANGE(self->pos), 0, (gdouble)self->duration / GST_SECOND);
	}
	
	/* get position */
	gint64 pos;
	gst_element_query_position(self->playbin, GST_FORMAT_TIME, &pos);
	
	/* block signal handler for value-changed */
	g_signal_handler_block(G_OBJECT(self->pos), self->pos_update_id);
	
	/* set position in range */
	gtk_range_set_value(GTK_RANGE(self->pos), (gdouble)pos / GST_SECOND);
	
	/* unblock signal handler */
	g_signal_handler_unblock(G_OBJECT(self->pos), self->pos_update_id);

	return 1;
}

static void error(GstBus *bus, GstMessage *msg, MusicPlayer *self) {

	gst_element_set_state(self->playbin, GST_STATE_READY);
	self->state = GST_STATE_READY;
}

static void eos(GstBus *bus, GstMessage *msg, MusicPlayer *self) {

	gst_element_set_state(self->playbin, GST_STATE_READY);
	self->state = GST_STATE_READY;
}

static void state_changed(GstBus *bus, GstMessage *msg, MusicPlayer *self) {
}

static void application(GstBus *bus, GstMessage *msg, MusicPlayer *self) {
}

static void value_changed(GtkWidget *w, MusicPlayer *self) {

	if (self->current == NULL) return;
	
	/* seek to position */
	double value = gtk_range_get_value(GTK_RANGE(w));
	
	gst_element_seek_simple(self->playbin, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_KEY_UNIT, (gint64)value * GST_SECOND);
}

/* finalize */
static void music_player_finalize(GObject *o) {

	MusicPlayer *self = MUSIC_PLAYER(o);

	if (self->current != NULL) free(self->current);
}

/* class initializer */
static void music_player_class_init(MusicPlayerClass *klass) {

	GObjectClass *cls = G_OBJECT_CLASS(klass);
	cls->finalize = music_player_finalize;
}

/* instance init */
static void music_player_init(MusicPlayer *self) {

	self->playbin = gst_element_factory_make("playbin", "playbin");
	self->bus = gst_element_get_bus(self->playbin);
	self->duration = GST_CLOCK_TIME_NONE;
	
	/* create widgets */
	self->box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	self->playbtn = gtk_button_new_from_icon_name("media-playback-start", GTK_ICON_SIZE_SMALL_TOOLBAR);
	self->pausebtn = gtk_button_new_from_icon_name("media-playback-pause", GTK_ICON_SIZE_SMALL_TOOLBAR);
	self->stopbtn = gtk_button_new_from_icon_name("media-playback-stop", GTK_ICON_SIZE_SMALL_TOOLBAR);
	self->pos = gtk_scale_new(GTK_ORIENTATION_HORIZONTAL, NULL);
	self->volume = gtk_scale_new(GTK_ORIENTATION_HORIZONTAL, NULL);
	
	gtk_button_set_relief(GTK_BUTTON(self->playbtn), GTK_RELIEF_NONE);
	gtk_button_set_relief(GTK_BUTTON(self->pausebtn), GTK_RELIEF_NONE);
	gtk_button_set_relief(GTK_BUTTON(self->stopbtn), GTK_RELIEF_NONE);
	
	gtk_scale_set_draw_value(GTK_SCALE(self->pos), 0);
	gtk_scale_set_draw_value(GTK_SCALE(self->volume), 0);
	gtk_range_set_range(GTK_RANGE(self->pos), 0, 100);
	gtk_range_set_range(GTK_RANGE(self->volume), 0, 100);
	
	gtk_widget_set_size_request(self->volume, 100, -1);
	gtk_range_set_value(GTK_RANGE(self->volume), 100);
	
	gtk_box_pack_start(GTK_BOX(self->box), self->playbtn, 0, 0, 0);
	gtk_box_pack_start(GTK_BOX(self->box), self->pausebtn, 0, 0, 0);
	gtk_box_pack_start(GTK_BOX(self->box), self->stopbtn, 0, 0, 0);
	gtk_box_pack_start(GTK_BOX(self->box), self->pos, 1, 1, 0);
	gtk_box_pack_start(GTK_BOX(self->box), self->volume, 0, 0, 0);
	
	/* add widget to bin */
	gtk_container_add(GTK_CONTAINER(self), self->box);

	/* connect signals */
	g_signal_connect(G_OBJECT(self->playbtn), "clicked", G_CALLBACK(_music_player_play), self);
	g_signal_connect(G_OBJECT(self->pausebtn), "clicked", G_CALLBACK(_music_player_pause), self);
	g_signal_connect(G_OBJECT(self->stopbtn), "clicked", G_CALLBACK(_music_player_stop), self);
	self->pos_update_id = g_signal_connect(G_OBJECT(self->pos), "value-changed", G_CALLBACK(value_changed), self);
	gst_bus_add_signal_watch(self->bus);
	g_signal_connect(G_OBJECT(self->bus), "message::error", G_CALLBACK(error), self);
	g_signal_connect(G_OBJECT(self->bus), "message::eos", G_CALLBACK(eos), self);
	g_signal_connect(G_OBJECT(self->bus), "message::state-changed", G_CALLBACK(state_changed), self);
	g_signal_connect(G_OBJECT(self->bus), "message::application", G_CALLBACK(application), self);
	
	/* add timeout function */
	g_timeout_add_seconds(1, G_SOURCE_FUNC(update), self);
}

/* create a new music player widget */
extern GtkWidget *music_player_new(void) {

	return GTK_WIDGET(g_object_new(MUSIC_TYPE_PLAYER, NULL));
}

static void _music_player_play(GtkWidget *w, MusicPlayer *self) { music_player_play(self); }
static void _music_player_pause(GtkWidget *w, MusicPlayer *self) { music_player_pause(self); }
static void _music_player_stop(GtkWidget *w, MusicPlayer *self) { music_player_stop(self); }

/* play */
extern void music_player_play(MusicPlayer *self) {

	if (self->current != NULL) {

		gst_element_set_state(self->playbin, GST_STATE_PLAYING);
		self->state = GST_STATE_PLAYING;
	}
}

/* pause */
extern void music_player_pause(MusicPlayer *self) {

	if (self->current != NULL && self->state == GST_STATE_PLAYING) {

		gst_element_set_state(self->playbin, GST_STATE_PAUSED);
		self->state = GST_STATE_PAUSED;
	}
}

/* stop */
extern void music_player_stop(MusicPlayer *self) {

	if (self->current != NULL) {

		gst_element_set_state(self->playbin, GST_STATE_READY);
		self->state = GST_STATE_READY;
	}
}

/* load music */
extern void music_player_load_from_file(MusicPlayer *self, char *fn) {

	music_player_stop(self);

	/* check file */
	struct stat st;
	if (stat(fn, &st) < 0) return;
	
	if (self->current != NULL) free(self->current);
	self->duration = GST_CLOCK_TIME_NONE;
	
	/* create uri */
	char *s = malloc(strlen(fn) + 8);
	strcpy(s, "file://");
	strcat(s, fn);
	
	/* set property */
	self->current = s;
	GValue v = G_VALUE_INIT;
	g_value_init(&v, G_TYPE_STRING);
	g_value_set_static_string(&v, s);
	g_object_set_property(G_OBJECT(self->playbin), "uri", &v);
}
