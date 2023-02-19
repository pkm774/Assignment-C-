#include <gst/gst.h>

#ifdef __APPLE__
#include <TargetConditionals.h>
#endif

typedef struct _Data
{
    GstElement* pipeline;
    GstElement* source;
    GstElement* tee;

    GstElement* blur_queue;
    GstElement* blur;
    GstElement* output1;

    GstElement* verti_queue;
    GstElement* vertigotv;
    GstElement* output2;
} Data;

static void on_pad_added(GstElement* element, GstPad* pad, gpointer data)
{
    Data* pipeline = (Data*)data;
    GstPad* sink_pad = gst_element_get_static_pad(pipeline->tee, "sink");
    GstPadLinkReturn ret;

    GstCaps* pad_caps = gst_pad_query_caps(pad, NULL);
    GstStructure* pad_struct = gst_caps_get_structure(pad_caps, 0);
    const gchar* pad_type = gst_structure_get_name(pad_struct);

    if (g_str_has_prefix(pad_type, "video/x-raw")) {
        ret = gst_pad_link(pad, sink_pad);
        if (GST_PAD_LINK_FAILED(ret)) {
            g_printerr("Failed to link source and tee.\n");
            gst_object_unref(pipeline->pipeline);
        }
    }

    gst_caps_unref(pad_caps);
    gst_object_unref(sink_pad);
}

int program_main(int argc, char* argv[])
{
    Data data{};
    GstPad* tee_blur_pad{}, * tee_verti_pad{};
    GstPad* queue_blur_pad{}, * queue_verti_pad{};

    GstBus* bus;
    GstMessage* msg;
    GstStateChangeReturn ret;

    /* Initialize custom data structure */
    memset(&data, 0, sizeof(data));

    /* Initialize GStreamer */
    gst_init(&argc, &argv);

    data.source = gst_element_factory_make("uridecodebin", "source");
    data.tee = gst_element_factory_make("tee", "tee");

    data.blur_queue = gst_element_factory_make("queue", "blur_queue");
    data.blur = gst_element_factory_make("videobalance", "blur");
    data.output1 = gst_element_factory_make("autovideosink", "output1");

    data.verti_queue = gst_element_factory_make("queue", "verti_queue");
    data.vertigotv = gst_element_factory_make("vertigotv", "vertigotv");
    data.output2 = gst_element_factory_make("autovideosink", "output2");

    /* Create new empty pipeline */
    data.pipeline = gst_pipeline_new("newpipeline");

    if (!data.pipeline || !data.source || !data.tee || !data.blur_queue || !data.blur ||
        !data.output1 || !data.verti_queue || !data.vertigotv || !data.output2) {
        g_printerr("Not all elements could be created.\n");
        return -1;
    }

    /* Link all elements that can be automatically linked because they have "Always" pads */
    gst_bin_add_many(GST_BIN(data.pipeline), data.source, data.tee, data.blur_queue, data.blur, data.output1,
        data.verti_queue, data.vertigotv, data.output2, NULL);

    g_signal_connect(data.source, "pad-added", G_CALLBACK(on_pad_added), &data);

    /* Manually link the Tee, which has "Request" pads */
    tee_blur_pad = gst_element_request_pad_simple(data.tee, "src_%u");
    g_print("Obtained request pad %s for audio branch.\n", gst_pad_get_name(tee_blur_pad));
    queue_blur_pad = gst_element_get_static_pad(data.blur_queue, "sink");
    if (gst_pad_link(tee_blur_pad, queue_blur_pad) != GST_PAD_LINK_OK) {
        g_printerr("Failed to link tee and blur queue.\n");
        gst_object_unref(data.pipeline);
        return -1;
    }
    gst_object_unref(tee_blur_pad);

    tee_verti_pad = gst_element_request_pad_simple(data.tee, "src_%u");
    g_print("Obtained request pad %s for video branch.\n", gst_pad_get_name(tee_verti_pad));
    queue_verti_pad = gst_element_get_static_pad(data.verti_queue, "sink");
    if (gst_pad_link(tee_verti_pad, queue_verti_pad) != GST_PAD_LINK_OK) {
        g_printerr("Failed to link tee and verti queue.\n");
        gst_object_unref(data.pipeline);
        return -1;
    }
    gst_object_unref(tee_verti_pad);

    if (gst_pad_link(tee_blur_pad, queue_blur_pad) != GST_PAD_LINK_OK ||
        gst_pad_link(tee_verti_pad, queue_verti_pad) != GST_PAD_LINK_OK) {
        g_printerr("Tee could not be linked\n");
        gst_object_unref(data.pipeline);
        return -1;
    }

    if (!gst_element_link(data.source, data.tee)) {
        g_printerr("Failed to link source and tee.\n");
        gst_object_unref(data.pipeline);
        return -1;
    }

    if (!gst_element_link_many(data.blur_queue, data.blur, data.output1, NULL)) {
        g_printerr("Failed to link blur_queue, blur, and output1.\n");
        gst_object_unref(data.pipeline);
        return -1;
    }

    if (!gst_element_link_many(data.verti_queue, data.vertigotv, data.output2, NULL)) {
        g_printerr("Failed to link verti_queue, vertigotv, and output2.\n");
        gst_object_unref(data.pipeline);
        return -1;
    }

    /* Set the URI to play */
    g_object_set(G_OBJECT(data.source), "uri", "https://www.freedesktop.org/software/gstreamer-sdk/data/media/sintel_trailer-480p.webm", NULL);

    /* Set the blur properties */
    g_object_set(G_OBJECT(data.blur), "saturation", 0.0, NULL);

    /* Start playing */
    ret = gst_element_set_state(data.pipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        g_printerr("unable to set the pipeline to playing state");
        gst_object_unref(data.pipeline);
        return -1;
    }

    /* Instruct the bus to emit signals for each received message, and connect to the interesting signals */
    bus = gst_element_get_bus(data.pipeline);
    msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE,
        static_cast<GstMessageType>(GST_MESSAGE_ERROR | GST_MESSAGE_EOS));

    /* Wait until error or EOS */
    bus = gst_element_get_bus(data.pipeline);
    msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE, (GstMessageType)(GST_MESSAGE_ERROR | GST_MESSAGE_EOS));

    /* Parse message */
    if (msg != NULL) {
        GError* err;
        gchar* debug_info;

        switch (GST_MESSAGE_TYPE(msg)) {
        case GST_MESSAGE_ERROR:
            gst_message_parse_error(msg, &err, &debug_info);
            g_printerr("Error received from element %s: %s\n", GST_OBJECT_NAME(msg->src), err->message);
            g_printerr("Debugging information: %s\n", debug_info ? debug_info : "none");
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

    gst_message_unref(msg);
    gst_object_unref(bus);
    gst_element_set_state(data.pipeline, GST_STATE_NULL);
    gst_object_unref(data.pipeline);
    gst_element_release_request_pad(data.tee, tee_blur_pad);
    gst_element_release_request_pad(data.tee, tee_verti_pad);
    gst_object_unref(queue_blur_pad);
    gst_object_unref(queue_verti_pad);

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
