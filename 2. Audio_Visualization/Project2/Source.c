#include <gst/gst.h>

// playbin flags
typedef enum
{
    // Enable rendering of visualizations when there is no video stream.
    GST_PLAY_FLAG_VIS = (1 << 3)
} GstPlayFlags;

// Return TRUE if this is a Visualization element
static gboolean filter_vis_features(GstPluginFeature* feature, gpointer data)
{
    GstElementFactory* factory;

    if (!GST_IS_ELEMENT_FACTORY(feature))
        return FALSE;

    factory = GST_ELEMENT_FACTORY(feature);

    if (!g_strrstr(gst_element_factory_get_klass(factory), "Visualization"))
        return FALSE;

    return TRUE;
}

int main(int argc, char* argv[])
{
    GstElement* pipeline, * vis_plugin;
    GstBus* bus;
    GstMessage* msg;
    GList* list, * walk;
    GstElementFactory* selected_factory = NULL;
    guint flags;

    // Initialize GStreamer
    gst_init(&argc, &argv);

    // Get a list of all visualization plugins
    list = gst_registry_feature_filter(gst_registry_get(), filter_vis_features, FALSE, NULL);

    // Select Frequency spectrum scope visualization plugin as default
    for (walk = list; walk != NULL; walk = g_list_next(walk))
    {
        const gchar* name;
        GstElementFactory* factory;

        factory = GST_ELEMENT_FACTORY(walk->data);
        name = gst_element_factory_get_longname(factory);

        if (selected_factory == NULL || g_str_has_prefix(name, "Frequency"))
        {
            selected_factory = factory;
        }
    }

    // Print if no visualization plugin found
    if (!selected_factory)
    {
        g_print("No visualization plugins found!\n");
        return -1;
    }

    // We have now selected a factory for the visualization element
    g_print("Selected '%s' as default visualizer\n", gst_element_factory_get_longname(selected_factory));

    vis_plugin = gst_element_factory_create(selected_factory, NULL);
    if (!vis_plugin)
        return -1;

    // Build the pipeline
    pipeline = gst_parse_launch("playbin uri=https://drive.google.com/u/0/uc?id=1BmcncnOu5MAEnrx5UfIj6GzLsli5QgKi&export=download", NULL);


    // Set the visualization flag
    g_object_get(pipeline, "flags", &flags, NULL);
    flags |= GST_PLAY_FLAG_VIS;
    g_object_set(pipeline, "flags", flags, NULL);

    // set vis plugin for playbin
    g_object_set(pipeline, "vis-plugin", vis_plugin, NULL);

    // Start playing
    gst_element_set_state(pipeline, GST_STATE_PLAYING);
    g_print("Playing Audio\n");

    // Wait until error or EOS
    bus = gst_element_get_bus(pipeline);
    msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE, GST_MESSAGE_ERROR | GST_MESSAGE_EOS);

    // Free resources
    if (msg != NULL)
        gst_message_unref(msg);

    gst_plugin_feature_list_free(list);
    gst_object_unref(bus);
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);

    return 0;
}