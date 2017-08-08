# sender 
CLIENT_IP=10.99.40.255
gst-launch-1.0 nvcamerasrc fpsRange="30 30" intent=3 \
! nvvidconv flip-method=2 \
! 'video/x-raw(memory:NVMM), width=(int)1920, height=(int)1080, format=(string)I420, framerate=(fraction)30/1' \
! omxh264enc control-rate=2 bitrate=4000000 \
! 'video/x-h264, stream-format=(string)byte-stream' \
! h264parse \
! rtph264pay mtu=1400 \
! udpsink host=$CLIENT_IP port=5000 sync=false async=false
