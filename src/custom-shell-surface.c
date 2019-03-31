#include "custom-shell-surface.h"

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkwayland.h>

static const char *custom_shell_surface_key = "wayland_custom_shell_surface";

struct _CustomShellSurfacePrivate
{
    GtkWindow *gtk_window;
    struct wl_surface *wl_surface;
};

static void
custom_shell_surface_on_window_destroy (CustomShellSurface *self)
{
    self->virtual->finalize (self);
    g_free (self->private);
    g_free (self);
}

static void
custom_shell_surface_on_window_realize (GtkWindow *gtk_window, CustomShellSurface *self)
{
    GdkWindow *gdk_window = gtk_widget_get_window (GTK_WIDGET (gtk_window));
    g_return_if_fail (gdk_window);

    self->private->wl_surface = gdk_wayland_window_get_wl_surface (gdk_window);
    g_return_if_fail (self->private->wl_surface);

    gdk_wayland_window_set_use_custom_surface (gdk_window);
}

static void
custom_shell_surface_on_window_map (GtkWindow *gtk_window, CustomShellSurface *self)
{
    g_return_if_fail (self->private->wl_surface);
    self->virtual->map (self, self->private->wl_surface);

    wl_surface_commit (self->private->wl_surface);
    wl_display_roundtrip (gdk_wayland_display_get_wl_display (gdk_display_get_default ()));
}

void
custom_shell_surface_init (CustomShellSurface *self, GtkWindow *gtk_window)
{
    g_assert (self->virtual); // Subclass should have set this up first

    self->private = g_new0 (CustomShellSurfacePrivate, 1);
    self->private->gtk_window = gtk_window;
    self->private->wl_surface = NULL;

    g_return_if_fail (gtk_window);
    g_return_if_fail (!gtk_widget_get_mapped (GTK_WIDGET (gtk_window)));
    gtk_window_set_decorated (gtk_window, FALSE);
    g_object_set_data_full (G_OBJECT (gtk_window),
                            custom_shell_surface_key,
                            self,
                            (GDestroyNotify) custom_shell_surface_on_window_destroy);
    g_signal_connect (gtk_window, "realize", G_CALLBACK (custom_shell_surface_on_window_realize), self);
    g_signal_connect (gtk_window, "map", G_CALLBACK (custom_shell_surface_on_window_map), self);

    if (gtk_widget_get_realized (GTK_WIDGET (gtk_window))) {
        // We must be in the process of realizing now
        custom_shell_surface_on_window_realize (gtk_window, self);
    }
}

CustomShellSurface *
gtk_window_get_custom_shell_surface (GtkWindow *gtk_window)
{
    if (!gtk_window)
        return NULL;

    return g_object_get_data (G_OBJECT (gtk_window), custom_shell_surface_key);
}

GtkWindow *
custom_shell_surface_get_gtk_window (CustomShellSurface *self)
{
    g_return_val_if_fail (self, NULL);
    return self->private->gtk_window;
}