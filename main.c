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
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <gtk/gtk.h>
#include <gst/gst.h>
#include <cairo.h>
#include "player.h"

static GtkWidget *win; /* main window */
static GtkWidget *vbox; /* vertical box */
static GtkWidget *menubar; /* menubar */
static GtkWidget *filemenu; /* the file menu */
static GtkWidget *filemenuitem; /* the menuitem for the file menu */
static GtkWidget *albumimage; /* a drawing area to display the album image if there is one */
static GtkWidget *player; /* music player widget */

/* end of application */
static void destroy(GtkWidget *w, gpointer d) { gtk_main_quit(); }

/* add menu item */
static void add_menu_item(GtkWidget *menu, char *label, GCallback callback) {

	/* create menu item */
	GtkWidget *menuitem = gtk_menu_item_new_with_label(label);
	
	/* set activate signal if a proper callback was passed */
	if (callback != NULL) g_signal_connect(G_OBJECT(menuitem), "activate", callback, NULL);

	/* add menu item */
	gtk_container_add(GTK_CONTAINER(menu), menuitem);
}

/* load file dialog */
static void load_file(GtkWidget *w, gpointer d) {

	/* create dialog */
	GtkWidget *diag = gtk_file_chooser_dialog_new("Open File", GTK_WINDOW(win), GTK_FILE_CHOOSER_ACTION_OPEN, "_Cancel", GTK_RESPONSE_CANCEL, "_Open", GTK_RESPONSE_ACCEPT, NULL);
	
	int res = gtk_dialog_run(GTK_DIALOG(diag));
	
	/* accept */
	if (res == GTK_RESPONSE_ACCEPT) {

		GtkFileChooser *ch = GTK_FILE_CHOOSER(diag);
		char *fn = gtk_file_chooser_get_filename(ch);
		
		/* check if file eixsts */
		struct stat st;
		if (stat(fn, &st) >= 0) {

			/* load file */
			music_player_load_from_file(MUSIC_PLAYER(player), fn);
		}
		
		g_free(fn);
	}
	
	/* destroy dialog */
	gtk_widget_destroy(diag);
}

static void draw(GtkWidget *w, cairo_t *cr, gpointer d) {

	/* get widget allocation */
	GtkAllocation a;
	gtk_widget_get_allocation(w, &a);
		
	/* fill context */
	cairo_set_source_rgb(cr, 0, 0, 0);
	cairo_rectangle(cr, 0, 0, a.width, a.height);
	cairo_fill(cr);
}

int main(int argc, char *argv[]) {
	
	gtk_init(&argc, &argv);
	gst_init(&argc, &argv);
	
	/* create window */
	win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_default_size(GTK_WINDOW(win), 640, 480);
	gtk_window_set_wmclass(GTK_WINDOW(win), "music", "music");
	
	/* create box */
	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_container_add(GTK_CONTAINER(win), vbox);
	
	/* create menus */
	menubar = gtk_menu_bar_new();
	
	filemenu = gtk_menu_new();
	filemenuitem = gtk_menu_item_new_with_label("File");
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(filemenuitem), filemenu);
	
	gtk_container_add(GTK_CONTAINER(menubar), filemenuitem);
	
	/* menu items */
	add_menu_item(filemenu, "Open File", G_CALLBACK(load_file));
	
	add_menu_item(filemenu, "Quit", G_CALLBACK(destroy));
	
	gtk_box_pack_start(GTK_BOX(vbox), menubar, 0, 1, 0);
	
	/* drawing area */
	albumimage = gtk_drawing_area_new();
	gtk_box_pack_start(GTK_BOX(vbox), albumimage, 1, 1, 0);
	
	g_signal_connect(G_OBJECT(albumimage), "draw", G_CALLBACK(draw), NULL);
	
	/* music player */
	player = music_player_new();
	music_player_load_from_file(MUSIC_PLAYER(player), "/home/elliot/Music/showbiz/Track_11.wav.mp3");
	gtk_box_pack_start(GTK_BOX(vbox), player, 0, 1, 0);
	
	/* show widgets */
	gtk_widget_show_all(win);
	
	/* connect signals */
	g_signal_connect(G_OBJECT(win), "destroy", G_CALLBACK(destroy), NULL);
	
	/* run gtk */
	gtk_main();
}
