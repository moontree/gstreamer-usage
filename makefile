executes = receiver sender_with_callback sender_with_loop
CFLAGS = `pkg-config --cflags opencv gstreamer-1.0`
LDFLAGS = `pkg-config --libs opencv gstreamer-1.0`
BUILD_ROOT = ./examples
all : $(BUILD_ROOT) $(executes)
receiver :
	g++ -o examples/receiver gst-parse-launch/receiver.cpp $(CFLAGS) $(LDFLAGS)
sender_with_callback :
	g++ -o examples/sender_with_callback senders/sender_with_callback.c $(CFLAGS) $(LDFLAGS)
sender_with_loop :
	g++ -o examples/sender_with_loop senders/sender_with_loop.c $(CFLAGS) $(LDFLAGS)

clean :
	rm -rf examples
$(BUILD_ROOT):
	mkdir -p $(BUILD_ROOT)
