gst-launch-1.0 -ve udpsrc port=5000 \
! application/x-rtp, media=video, clock-rate=90000, encoding-name=H264, payload=96 \
! rtph264depay \
! h264parse \
! avdec_h264 \
! videoconvert \
! ximagesink sync=false
