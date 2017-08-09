#include <gst/gst.h>
#include <gst/gstminiobject.h>
#include <cv.h>
#include <highgui.h>
#include <opencv2/opencv.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <time.h>

static GMainLoop *loop;
// open camera
cv::VideoCapture cap(0);
GstBuffer *buffer;

//GstBuffer *buffer = gst_buffer_new_allocate (NULL, 640 * 360 * 3, NULL);
static void cb_need_data (GstElement *appsrc){
     static GstClockTime timestamp = 0;
     guint size,depth,height,width,step,channels;
     GstFlowReturn ret;
     IplImage* img;
     guchar *data1;

     GstMapInfo map;
     cv::Mat frame, dst;
     IplImage copy;
     cap >> frame;
     cv::resize(frame, dst, cv::Size(640, 360), 0, 0, cv::INTER_LINEAR);
     copy = dst;
     img =  &copy;
     channels  = img->nChannels;
     data1 = (guchar *)img->imageData;
     gst_buffer_map (buffer, &map, GST_MAP_WRITE);
     memcpy( (guchar *)map.data, data1,  gst_buffer_get_size( buffer ) );
     GST_BUFFER_PTS (buffer) = timestamp;
     GST_BUFFER_DURATION (buffer) = gst_util_uint64_scale_int (1, GST_SECOND, 30);
     timestamp += GST_BUFFER_DURATION (buffer);
     g_signal_emit_by_name (appsrc, "push-buffer", buffer, &ret);
     //gst_buffer_unref (buffer);
     if (ret!=0) {
         g_main_loop_quit (loop);
     }
}

gint main (gint   argc, gchar *argv[]){
  GstElement *pipeline, *appsrc, *conv, *encoder, *rtppay, *udpsink;
  /* init GStreamer */
  gst_init (&argc, &argv);
  loop = g_main_loop_new (NULL, FALSE);
  /* setup pipeline */
  pipeline = gst_pipeline_new ("pipeline");
  appsrc = gst_element_factory_make ("appsrc", "source");
  conv = gst_element_factory_make ("videoconvert", "conv");
  encoder = gst_element_factory_make ("x264enc", "encoder");
  rtppay = gst_element_factory_make ("rtph264pay", "rtppay");
  udpsink = gst_element_factory_make ("udpsink", "sink");

  g_object_set( encoder, "tune", 4,
  "threads", 4,
  "key-int-max", 15,
  "intra-refresh", true,
  "speed-preset", 5,
  NULL);
  g_object_set( udpsink, "host", "10.99.40.255", NULL );
  g_object_set( udpsink, "port", 5000, NULL );
  g_object_set( udpsink, "sync", false, NULL );
  g_object_set( udpsink, "async", false, NULL );
  /* setup */
  g_object_set (G_OBJECT (appsrc), "caps",
        gst_caps_new_simple ("video/x-raw",
                     "format", G_TYPE_STRING, "BGR",
                     "width", G_TYPE_INT, 640,
                     "height", G_TYPE_INT, 360,
                     "framerate", GST_TYPE_FRACTION, 30, 1,
                     NULL), NULL);
  gst_bin_add_many (GST_BIN (pipeline), appsrc, conv, encoder, rtppay, udpsink, NULL);
  gst_element_link_many (appsrc, conv, encoder, rtppay, udpsink , NULL);
  /* setup appsrc */
  g_object_set (G_OBJECT (appsrc),
        "stream-type", 0,
        "format", GST_FORMAT_TIME, NULL);
  buffer = gst_buffer_new_allocate (NULL, 640 * 360 * 3, NULL);
  g_signal_connect (appsrc, "need-data", G_CALLBACK (cb_need_data), NULL);

  /* play */
  std::cout << "start sender pipeline" << std::endl;
  gst_element_set_state (pipeline, GST_STATE_PLAYING);
  g_main_loop_run (loop);
  /* clean up */
  gst_element_set_state (pipeline, GST_STATE_NULL);
  gst_object_unref (GST_OBJECT (pipeline));
  g_main_loop_unref (loop);
  gst_buffer_unref(buffer);
  return 0;
}
