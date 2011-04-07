#include <gtk/gtk.h>
#include "config.h"

static GtkWidget *about_window = NULL;

void fc_ip_about()
{
    /* This code has been taken from J. Nick Koston's Mikmod plugin
     * which comes with the XMMS distribution. */
    
    GtkWidget *dialog_vbox1;
    GtkWidget *hbox1;
    GtkWidget *label1;
    GtkWidget *dialog_action_area1;
    GtkWidget *about_exit;
    GtkStyle *style;

    if (!about_window)
    {
        about_window = gtk_dialog_new();
        g_object_set_data(G_OBJECT(about_window), "about_window", about_window);
        gtk_window_set_title(GTK_WINDOW(about_window), "About Future Composer plugin");
        gtk_window_set_resizable(GTK_WINDOW(about_window), FALSE);
        gtk_window_set_position(GTK_WINDOW(about_window), GTK_WIN_POS_MOUSE);
        g_signal_connect(G_OBJECT(about_window), "destroy", G_CALLBACK(gtk_widget_destroyed), &about_window);
        gtk_container_set_border_width(GTK_CONTAINER(about_window), 10);

        dialog_vbox1 = GTK_DIALOG(about_window)->vbox;
        g_object_set_data(G_OBJECT(about_window), "dialog_vbox1", dialog_vbox1);
        gtk_widget_show(dialog_vbox1);
        gtk_container_set_border_width(GTK_CONTAINER(dialog_vbox1), 5);

        hbox1 = gtk_hbox_new(FALSE, 0);
        g_object_set_data(G_OBJECT(about_window), "hbox1", hbox1);
        gtk_widget_show(hbox1);
        gtk_box_pack_start(GTK_BOX(dialog_vbox1), hbox1, TRUE, TRUE, 0);
        gtk_container_set_border_width(GTK_CONTAINER(hbox1), 5);
        gtk_widget_realize(about_window);
        
        label1 = gtk_label_new("Future Composer player plugin\nVersion " VERSION "\nhttp://xmms-fc.sourceforge.net\nCreated by Michael Schwendt.\n\nMuch room for improvement.\nHave a go if you like to.\n");
        g_object_set_data(G_OBJECT(about_window), "label1", label1);
        gtk_widget_show(label1);
        gtk_box_pack_start(GTK_BOX(hbox1), label1, TRUE, TRUE, 0);

        dialog_action_area1 = GTK_DIALOG(about_window)->action_area;
        g_object_set_data(G_OBJECT(about_window), "dialog_action_area1", dialog_action_area1);
        gtk_widget_show(dialog_action_area1);
        gtk_container_set_border_width(GTK_CONTAINER(dialog_action_area1), 10);

        about_exit = gtk_button_new_with_label("Ok");
        g_signal_connect_swapped(G_OBJECT(about_exit), "clicked",
                                G_CALLBACK(gtk_widget_destroy),
                                G_OBJECT(about_window));

        g_object_set_data(G_OBJECT(about_window), "about_exit", about_exit);
        gtk_widget_show(about_exit);
        gtk_box_pack_start(GTK_BOX(dialog_action_area1), about_exit, TRUE, TRUE, 0);

        gtk_widget_show(about_window);
    }
    else
    {
        gdk_window_raise(about_window->window);
    }
}
