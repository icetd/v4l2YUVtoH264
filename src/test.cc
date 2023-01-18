#include <bits/stdint-uintn.h>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <linux/videodev2.h>
#include <log.h>
#include <x264encoder.h>
#include <V4l2Device.h>
#include <V4l2Capture.h>
#include <signal.h>
#include <unistd.h>

int LogLevel;
int stop = 0;

void sighandler(int)
{
	printf("SIGINT\n");
	stop = 1;
}

int main()
{	
	const char *in_devname = "/dev/video0";	
	v4l2IoType ioTypeIn  = IOTYPE_MMAP;
	int format = 0;
	int width = 960;
	int height = 540;
	int fps = 10;

	initLogger(INFO);
	V4L2DeviceParameters param(in_devname, V4L2_PIX_FMT_YUYV, width, height, fps, ioTypeIn, DEBUG);
	V4l2Capture *videoCapture = V4l2Capture::create(param);
	X264Encoder *encoder = new X264Encoder(width, height, X264_CSP_I422);
	uint8_t *h264_buf = (uint8_t*) malloc(videoCapture->getBufferSize());
	FILE *fp = fopen("test.h264", "w");

	if (videoCapture == nullptr) {
		LOG(WARN, "Cannot reading from V4l2 capture interface for device: %s", in_devname);
	} else {
		timeval tv;

		LOG(NOTICE, "Start reading from %s", in_devname);
		signal(SIGINT, sighandler);

		while(!stop) {
			tv.tv_sec = 1;
			tv.tv_usec = 0;
			int ret = videoCapture->isReadable(&tv);

			if(ret == 1) {
				uint8_t buffer[videoCapture->getBufferSize()];
				int resize = videoCapture->read((char*)buffer, sizeof(buffer));
				int size = encoder->encode(buffer, resize, h264_buf);
				fwrite(h264_buf, size, 1, fp);

				if (resize == -1) {
					LOG(NOTICE, "stop %s", strerror(errno));
					stop = 1;
				} else {
					LOG(NOTICE, "yuv422 frame size: %d", resize);
				}
			}else if (ret == -1) {
				LOG(NOTICE, "stop %s", strerror(errno));
				stop = 1;
			}
		}
		delete videoCapture;
		free(h264_buf);
		fclose(fp);
	}

	LOG(INFO, "=============test=============");
	return 0;
}
