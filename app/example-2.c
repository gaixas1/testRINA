#include <gtk/gtk.h>

void on_window_destroy (GtkWidget *object, gpointer user_data) {
    gtk_main_quit();
}


int main (int argc, char *argv[]) {
    GtkBuilder *builder;
    GtkWidget *window;

    gtk_init (&argc, &argv);

    builder = gtk_builder_new ();
    gtk_builder_add_from_file (builder, "interf.glade", NULL);

    window = GTK_WIDGET (gtk_builder_get_object (builder, "window1"));
    g_signal_connect (window, "destroy", G_CALLBACK (on_window_destroy), NULL);

    gtk_widget_show (window);
    gtk_main ();

    return 0;
}
