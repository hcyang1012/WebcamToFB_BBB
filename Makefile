all:0_webcam_capture 1_frame_buffer 2_webcam_to_fb 3_webcam_video_to_fb

0_webcam_capture: 0_webcam_capture.c
	gcc -o 0_webcam_capture 0_webcam_capture.c

1_frame_buffer: 1_frame_buffer.c
	gcc -o 1_frame_buffer 1_frame_buffer.c

2_webcam_to_fb:2_webcam_to_fb.c
	gcc -o 2_webcam_to_fb 2_webcam_to_fb.c

3_webcam_video_to_fb:3_webcam_video_to_fb.c
	gcc -o 3_webcam_video_to_fb 3_webcam_video_to_fb.c

clean:
	rm -rf *.bin 0_webcam_capture
	rm -rf 1_frame_buffer
	rm -rf 2_webcam_to_fb
	rm -rf 3_webcam_video_to_fb
