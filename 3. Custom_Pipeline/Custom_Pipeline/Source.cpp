#include <gst/gst.h>

#ifdef __APPLE__
#include <TargetConditionals.h>
#endif

typedef class _GSData {
public:
    GstElement* pipeline;
    GstElement* source;
    GstElement* blur;
    GstElement* vertigotv;
    GstElement* sink;
} Data;


int program_main(int argc, char* argv[])
{
    Data element{};
    GstBus* bus;
    GstMessage* msg;
    GstStateChangeReturn ret;

    /* Initialize GStreamer */
    gst_init(&argc, &argv);

    /* Create the elements */
    element.source = gst_element_factory_make("videotestsrc", "source");
    element.blur = gst_element_factory_make("videobox", "blur");
    element.vertigotv = gst_element_factory_make("vertigotv", "vertigotv");
    element.sink = gst_element_factory_make("autovideosink", "sink");

    /* Create the empty pipeline */
    element.pipeline = gst_pipeline_new("test-pipeline");

    if (!element.pipeline || !element.source || !element.blur || !element.vertigotv || !element.sink) {
        g_printerr("Not all elements could be created.\n");
        return -1;
    }

    /* Build the pipeline */
    gst_bin_add_many(GST_BIN(element.pipeline), element.source, element.blur, element.vertigotv, element.sink, NULL);

    /* Link the elements */
    if (!gst_element_link_many(element.source, element.blur, element.vertigotv, element.sink, NULL)) {
        g_printerr("Elements could not be linked.\n");
        gst_object_unref(element.pipeline);
        return -1;
    }

    /* Modify the source's properties */
    g_object_set(element.source, "pattern", 0, NULL);

    /* Set the cropping filter properties */
    g_object_set(element.blur, "left", -50, "top", -50, "right", -50, "bottom", -50, NULL);

    /* Start playing */
    ret = gst_element_set_state(element.pipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        g_printerr("Unable to set the pipeline to the playing state.\n");
        gst_object_unref(element.pipeline);
        return -1;
    }

    /* Wait until error or EOS */
    bus = gst_element_get_bus(element.pipeline);
    msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE,
        static_cast<GstMessageType>(GST_MESSAGE_ERROR | GST_MESSAGE_EOS));

    /* Parse message */
    if (msg != NULL) {
        GError* err;
        gchar* debug_info;

        switch (GST_MESSAGE_TYPE(msg)) {
        case GST_MESSAGE_ERROR:
            gst_message_parse_error(msg, &err, &debug_info);
            g_printerr("Error received from element %s: %s\n",
                GST_OBJECT_NAME(msg->src), err->message);
            g_printerr("Debugging information: %s\n",
                debug_info ? debug_info : "none");
            g_clear_error(&err);
            g_free(debug_info);
            break;
        case GST_MESSAGE_EOS:
            g_print("End-Of-Stream reached.\n");
            break;
        default:
            /* We should not reach here because we only asked for ERRORs and EOS */
            g_printerr("Unexpected message received.\n");
            break;
        }
        gst_message_unref(msg);
    }

    /* Free resources */
    gst_object_unref(bus);
    gst_element_set_state(element.pipeline, GST_STATE_NULL);
    gst_object_unref(element.pipeline);
    return 0;
}

int main(int argc, char* argv[])
{
#if defined(__APPLE__) && TARGET_OS_MAC && !TARGET_OS_IPHONE
    return gst_macos_main(program_main, argc, argv, NULL);
#else
    return program_main(argc, argv);
#endif
}