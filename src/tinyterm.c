/* tinyterm.c
 *
 * Copyright (C) 2013 MelKori
 * Originally taken from Sebastian Linke's tinyterm
 *
 * This is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Library General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <gdk/gdkkeysyms.h>
#include <vte/vte.h>
#include <wordexp.h>
#include "config.h"

#ifndef TINYTERM_SCROLLBAR
#define TINYTERM_SCROLLBAR 0
#endif

static void
xdg_open_selection_cb (GtkClipboard *clipboard, const char *string, gpointer data)
{
    char *command;
    wordexp_t result;
    gboolean spawn;
    GError *spawn_error = NULL;

    command = g_strconcat ("xdg-open ", string, NULL);
    switch (wordexp (command, &result, WRDE_NOCMD)) {
        case 0:
            break;
        case WRDE_BADCHAR:
            fprintf (stderr, "'%s' contains an invalid character\n", string);
            goto finalize;
        case WRDE_CMDSUB:
            fprintf (stderr, "'%s' uses command substitution, which is not allowed\n", string);
            goto finalize;
        case WRDE_NOSPACE:
            fprintf (stderr, "Could not allocate enough memory when parsing '%s'\n", string);
            goto finalize;
        case WRDE_SYNTAX:
            fprintf (stderr, "Syntax error in '%s'\n", string);
            goto finalize;
    }
    spawn = g_spawn_async (NULL, result.we_wordv, NULL, G_SPAWN_SEARCH_PATH,
                           NULL, NULL, NULL, &spawn_error);
    if (!spawn) {
        fprintf (stderr, "%s\n", spawn_error->message);
        g_error_free (spawn_error);
    }
    finalize:
        wordfree (&result);
}

static void
xdg_open_selection (GtkWidget *terminal)
{
    GdkDisplay *display;
    GtkClipboard *clipboard;

    vte_terminal_copy_primary (VTE_TERMINAL (terminal));
    display = gtk_widget_get_display (terminal);
    clipboard = gtk_clipboard_get_for_display (display, GDK_SELECTION_PRIMARY);
    gtk_clipboard_request_text (clipboard, xdg_open_selection_cb, NULL);
}

static gboolean
on_key_press (GtkWidget *terminal, GdkEventKey *event)
{
    if (event->state == (GDK_CONTROL_MASK | GDK_SHIFT_MASK)) {
        switch (event->keyval) {
            case GDK_C:
                vte_terminal_copy_clipboard (VTE_TERMINAL (terminal));
                return TRUE;
            case GDK_V:
                vte_terminal_paste_clipboard (VTE_TERMINAL (terminal));
                return TRUE;
            case GDK_X:
                xdg_open_selection (terminal);
                return TRUE;
        }
    }
    return FALSE;
}

int
main (int argc, char *argv[])
{
    GtkWidget *window, *terminal, *scrollbar, *design;
    GError *icon_error = NULL;
    GdkPixbuf *icon;
    GdkGeometry geo_hints;

    /* Init gtk and all widgets */
    gtk_init (&argc, &argv);
    window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    terminal = vte_terminal_new ();
    scrollbar = gtk_vscrollbar_new (VTE_TERMINAL (terminal)->adjustment);
    design = gtk_hbox_new (FALSE, 0);

    /* Set window icon */
    icon = gdk_pixbuf_new_from_file (TINYTERM_ICON_PATH, &icon_error);
    if (!icon) {
        fprintf (stderr, "%s\n", icon_error->message);
        g_error_free (icon_error);
    }
    gtk_window_set_icon (GTK_WINDOW (window), icon);

    /* Set window title */
    gtk_window_set_title (GTK_WINDOW (window), "TinyTerm");

    /* Set scrollback lines */
    vte_terminal_set_scrollback_lines (VTE_TERMINAL (terminal), TINYTERM_SCROLLBACK_LINES);

    /* Apply geometry hints to handle terminal resizing */
    geo_hints.base_width = VTE_TERMINAL (terminal)->char_width;
    geo_hints.base_height = VTE_TERMINAL (terminal)->char_height;
    geo_hints.min_width = VTE_TERMINAL (terminal)->char_width;
    geo_hints.min_height = VTE_TERMINAL (terminal)->char_height;
    geo_hints.width_inc = VTE_TERMINAL (terminal)->char_width;
    geo_hints.height_inc = VTE_TERMINAL (terminal)->char_height;
    gtk_window_set_geometry_hints (GTK_WINDOW (window), terminal, &geo_hints,
                                   GDK_HINT_RESIZE_INC | GDK_HINT_MIN_SIZE | GDK_HINT_BASE_SIZE);

    /* Open a standard shell */
    #if VTE_CHECK_VERSION(0,25,0)
    char **arg = NULL;
    g_shell_parse_argv(g_getenv("SHELL"), NULL, &arg, NULL);
    
    vte_terminal_fork_command_full(VTE_TERMINAL (terminal), VTE_PTY_DEFAULT, NULL, arg, NULL,
                                   G_SPAWN_CHILD_INHERITS_STDIN|G_SPAWN_SEARCH_PATH|G_SPAWN_FILE_AND_ARGV_ZERO,
                                   NULL, NULL, NULL, NULL);
    #else
    vte_terminal_fork_command (VTE_TERMINAL (terminal),
                               NULL,  // binary to run (NULL=user's shell)
                               NULL,  // arguments
                               NULL,  // environment
                               NULL,  // dir to start (NULL=CWD)
                               TRUE,  // log session to lastlog
                               TRUE,  // log session to utmp/utmpx log
                               TRUE); // log session to wtmp/wtmpx log
    #endif
                               
    /* Turn Off cursor blinking */
    vte_terminal_set_cursor_blink_mode(VTE_TERMINAL(terminal), VTE_CURSOR_BLINK_OFF); 

    /* Connect signals */
    g_signal_connect (window, "delete-event", gtk_main_quit, NULL);
    g_signal_connect (terminal, "child-exited", gtk_main_quit, NULL);
    g_signal_connect (terminal, "key-press-event", G_CALLBACK (on_key_press), NULL);

    /* Set selection behavior for double-clicks */
    vte_terminal_set_word_chars (VTE_TERMINAL (terminal), TINYTERM_WORD_CHARS);

    /* Put all widgets together and show the result */
    gtk_box_pack_start (GTK_BOX (design), terminal, TRUE, TRUE, 0);
#if TINYTERM_SCROLLBAR
    gtk_box_pack_start (GTK_BOX (design), scrollbar, FALSE, FALSE, 0);
#endif
    gtk_container_add (GTK_CONTAINER (window), design);
    gtk_widget_show_all (window);
    gtk_main ();

    return 0;
}
