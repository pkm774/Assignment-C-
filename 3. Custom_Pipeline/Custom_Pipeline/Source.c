#include <gst/gst.h>

#ifdef __APPLE__
#include <TargetConditionals.h>
#endif

typedef struct _Data
{
    GstElement* pipeline;
    GstElement* source;
    GstElement* tee;
    GstElement* blur;
    GstElement* output1;
    GstElement* vertigotv;
    GstElement* output2;
} Data;

int program_main(int argc, char* argv[])
{
    Data data;
    memset(&data, 0, sizeof(data));

    GstBus* bus;
    GstMessage* msg;
    GstStateChangeReturn ret;

    gst_init(&argc, &argv);

    /* Create the elements */
    data.pipeline = gst_pipeline_new("new-pipeline");

    data.source = gst_element_factory_make("uridecodebin", "source");
    g_object_set(G_OBJECT(data.source), "uri", "https://www.freedesktop.org/software/gstreamer-sdk/data/media/sintel_trailer-480p.webm", NULL);

    data.tee = gst_element_factory_make("tee", "splitter");

    data.blur = gst_element_factory_make("videobalance", "blur");
    g_object_set(G_OBJECT(data.blur), "saturation", 0.0, NULL);

    data.vertigotv = gst_element_factory_make("vertigotv", "vertigotv");

    data.output1 = gst_element_factory_make("autovideosink", "output1");
    data.output2 = gst_element_factory_make("autovideosink", "output2");

    gst_bin_add_many(GST_BIN(data.pipeline), data.source, data.tee, data.blur, data.vertigotv, data.output1, data.output2, NULL);

    gst_element_link_many(data.source, data.tee, NULL);

    GstPad* tee_pad = gst_element_request_pad_simple(data.tee, "src_%u");
    GstPad* blur_pad = gst_element_get_static_pad(data.blur, "sink");
    GstPad* vertigotv_pad = gst_element_get_static_pad(data.vertigotv, "sink");

    gst_element_link_pads(data.tee, gst_pad_get_name(tee_pad), data.blur, "sink");
    gst_element_link_pads(data.tee, gst_pad_get_name(tee_pad), data.vertigotv, "sink");

    /* Start playing */
    ret = gst_element_set_state(data.pipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        g_printerr("Unable to set the pipeline to the playing state.\n");
        gst_object_unref(data.pipeline);
        return -1;
    }

    g_print("Pipeline is now playing\n");

    gst_element_link_many(data.blur, data.output1, NULL);
    gst_element_link_many(data.vertigotv, data.output2, NULL);

    g_print("Elements linked\n");

    /* Wait until error or EOS */
    bus = gst_element_get_bus(data.pipeline);
    msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE,
        GST_MESSAGE_ERROR | GST_MESSAGE_EOS);

    g_print("Waiting for bus messages\n");

    /* Release the request pads */
    gst_element_release_request_pad(data.tee, tee_pad);

    g_print("Request pads released\n");

    /* Free resources */
    if (msg != NULL)
    {
        gst_message_unref(msg);
    }

    gst_object_unref(tee_pad);
    gst_object_unref(blur_pad);
    gst_object_unref(vertigotv_pad);
    gst_element_set_state(data.pipeline, GST_STATE_NULL);
    gst_object_unref(GST_OBJECT(data.pipeline));

    g_print("Exiting program\n");

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
