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
/* music player */
#ifndef PLAYER_H
#define PLAYER_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

/* music player type */
#define MUSIC_TYPE_PLAYER (music_player_get_type())

G_DECLARE_FINAL_TYPE(MusicPlayer, music_player, MUSIC, PLAYER, GtkBin)

/* functions */
extern GtkWidget *music_player_new(void); /* create a new music player widget */
extern void music_player_play(MusicPlayer *); /* play */
extern void music_player_pause(MusicPlayer *); /* pause */
extern void music_player_stop(MusicPlayer *); /* stop */
extern void music_player_load_from_file(MusicPlayer *, char *); /* load from a file */

G_END_DECLS

#endif /* PLAYER_H */
