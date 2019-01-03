#ffmpeg -i clock.webm -acodec AAC -vcodec h264 video.ts
ffmpeg -i video.ts -acodec copy -vcodec copy video.mp4

