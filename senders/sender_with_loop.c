#include <gst/gst.h>
#include <cv.h>
#include <highgui.h>
#include <opencv2/opencv.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <gst/app/gstappsink.h>
#include <gst/app/gstappsrc.h>
#include <time.h>

cv::VideoCapture cap(0);
gint main (gint   argc, gchar *argv[]){
  GstElement *pipeline, *appsrc, *conv, *encoder, *rtppay, *udpsink;
  /* init GStreamer */
  gst_init (NULL, NULL);
  /* setup pipeline */
  pipeline = gst_pipeline_new ("pipeline");
  appsrc = gst_element_factory_make ("appsrc", "source");
  std::cout << appsrc << std::endl;
  conv = gst_element_factory_make ("videoconvert", "conv");
  encoder = gst_element_factory_make ("x264enc", "encoder");
  rtppay = gst_element_factory_make ("rtph264pay", "rtppay");
  udpsink = gst_element_factory_make ("udpsink", "sink");

  static GstClockTime timestamp = 0;
  g_object_set( encoder, "tune", "zerolatency", NULL );
  g_object_set( encoder, "threads", 4, NULL );
  g_object_set( encoder, "key-int-max", 15, NULL );
  g_object_set( encoder, "intra-refresh", true, NULL );
  g_object_set( encoder, "speed-preset", "fast", NULL );
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
  /* play */
  std::cout << "start sender pipeline" << std::endl;
  gst_element_set_state (pipeline, GST_STATE_PLAYING);
  GstBuffer *buffer;
  buffer = gst_buffer_new_allocate (NULL, 640*360*3, NULL);
  while(1){
      guint size,depth,height,width,step,channels;
      GstFlowReturn ret;
      IplImage* img;
      IplImage* ori_img;
      guchar *data1;
      GstMapInfo map;
      cv::Mat frame, dst;
      IplImage copy;
      cap >> frame;
      cv::resize(frame, dst, cv::Size(640, 360), 0, 0, cv::INTER_LINEAR);
      copy = dst;
      img =  &copy;
      height = 360;
      width = 640;
      channels  = img->nChannels;
      data1 = (guchar *)img->imageData;
      size = height*width*channels;
      gst_buffer_map (buffer, &map, GST_MAP_WRITE);
      memcpy( (guchar *)map.data, data1,  gst_buffer_get_size( buffer ) );
      GST_BUFFER_PTS (buffer) = timestamp;
      GST_BUFFER_DURATION (buffer) = gst_util_uint64_scale_int (1, GST_SECOND, 30);
      timestamp += GST_BUFFER_DURATION (buffer);
      g_signal_emit_by_name (appsrc, "push-buffer", buffer, &ret);
  }
  /* clean up */
  gst_element_set_state (pipeline, GST_STATE_NULL);
  gst_object_unref (GST_OBJECT (pipeline));

  return 0;
}
