# Gstreamer Usage Example
This repo provides some code to combine opencv and gstreamer-1.0 to write frames to client ip and port. 
- "scripts" : the commands you can use in command line directly. 
- "senders" : different ways to push data to pipeline, with callback or loop
- "gst-parse-launch" : use gst-parse-launch() function to build pipeline

## Issues
### out of memory
gst_buffer_ref() seems not work correctly or something else causes memory increase all the time, in the original example, buffer is created and allocated in the loop/ callback function,
and released after data in buffer has been pushed, as following
```
static void cb_need_data (GstElement *appsrc){
    ...
    GstBuffer *buffer;
    buffer = gst_buffer_new_allocate (NULL, buffer_size, NULL);
    ...
    g_signal_emit_by_name (appsrc, "push-buffer", buffer, &ret);
    gst_buffer_unref (buffer);
}
```
when I read the function in detail, this is the description,

>
Decreases the refcount of the buffer. If the refcount reaches 0, the buffer will be freed.
If GST_BUFFER_MALLOCDATA() is non-NULL, this pointer will also be freed at this time.

Buffers are usually freed by unreffing them with gst_buffer_unref(). When the refcount drops to 0, any memory and metadata pointed to by the buffer is
unreffed as well. Buffers allocated from a #GstBufferPool will be returned to the pool when the refcount drops to 0.

which means that when I called the function, the ref count of buffer decreases by 1, so the failed reason maybe the original ref count is more than 1?



gst_buffer_unref() called gst_mini_object_unref() in gstminiobject.c, implement as following
```
void
gst_mini_object_unref (GstMiniObject * mini_object)
{
  gint old_refcount, new_refcount;

  g_return_if_fail (mini_object != NULL);
  g_return_if_fail (GST_MINI_OBJECT_REFCOUNT_VALUE (mini_object) > 0);

  old_refcount = g_atomic_int_add (&mini_object->refcount, -1);
  new_refcount = old_refcount - 1;

  g_return_if_fail (old_refcount > 0);

  GST_CAT_TRACE (GST_CAT_REFCOUNTING, "%p unref %d->%d",
      mini_object, old_refcount, new_refcount);

  GST_TRACER_MINI_OBJECT_UNREFFED (mini_object, new_refcount);

  if (new_refcount == 0) {
    gboolean do_free;

    if (mini_object->dispose)
      do_free = mini_object->dispose (mini_object);
    else
      do_free = TRUE;

    /* if the subclass recycled the object (and returned FALSE) we don't
     * want to free the instance anymore */
    if (G_LIKELY (do_free)) {
      /* there should be no outstanding locks */
      g_return_if_fail ((g_atomic_int_get (&mini_object->lockstate) & LOCK_MASK)
          < 4);

      if (mini_object->n_qdata) {
        call_finalize_notify (mini_object);
        g_free (mini_object->qdata);
      }
      GST_TRACER_MINI_OBJECT_DESTROYED (mini_object);
      if (mini_object->free)
        mini_object->free (mini_object);
    }
  }
}

```
However, when I print the ref count out, the number is 2, but the second gst_buffer_unref() throws an assertion error:
*(sender_with_callback:9043): GStreamer-CRITICAL : gst_mini_object_unref: assertion 'mini_object->refcount > 0' failed*， which means it's called in somewhere, but the memory it used is not been returned. 
something happened?

```
buffer = gst_buffer_new_allocate (NULL, size, NULL);
gst_buffer_map (buffer, &map, GST_MAP_WRITE);
memcpy( (guchar *)map.data, data1,  gst_buffer_get_size( buffer ) );
GST_BUFFER_PTS (buffer) = timestamp;
GST_BUFFER_DURATION (buffer) = gst_util_uint64_scale_int (1, GST_SECOND, 30);
timestamp += GST_BUFFER_DURATION (buffer); // ref count is 1
g_signal_emit_by_name (appsrc, "push-buffer", buffer, &ret); // ref count is 2
gst_buffer_unref(buffer); // ref count is 1

```

**current solution is to use buffer as global variable**

### latency
When use command line of gst-launch, the video stream is in real time, but when using gst to send stream, there has latency of about 2s, which is boring!
This problem is to be fixed.

### opencv videowriter write to gstremaer
It's possible in theory, but I haven't try it out.
Todo
