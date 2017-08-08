CLIENT_IP=10.99.40.255
gst-launch-1.0 -ve v4l2src \
! video/x-raw, framerate=30/1 \
! videoconvert \
! x264enc noise-reduction=10000 speed-preset=fast tune=zerolatency byte-stream=true threads=4 key-int-max=15 intra-refresh=true  \
! rtph264pay pt=96 \
! udpsink host=$CLIENT_IP port=5000 sync=false async=false
