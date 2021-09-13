hzs@ubuntu:~$ mkdir video_server
hzs@ubuntu:~$ cd video_server/
解压 ffmpeg-snapshot.tar.bz2，opencv-4.2.0.zip，video_server_linux.tar.gz，到/home/hzs/video_server

建议先把ubuntu设置中的镜像源改为阿里云的加快下载速度

一、安装ffmpeg：
hzs@ubuntu:~/video_server$ cd ffmpeg
hzs@ubuntu:~/video_server/ffmpeg$ pwd
/home/hzs/video_server/ffmpeg

hzs@ubuntu:~/video_server/ffmpeg$ sudo apt-get update -qq && sudo apt-get -y install \
  autoconf \
  automake \
  build-essential \
  cmake \
  git-core \
  libass-dev \
  libfreetype6-dev \
  libsdl2-dev \
  libtool \
  libva-dev \
  libvdpau-dev \
  libvorbis-dev \
  libxcb1-dev \
  libxcb-shm0-dev \
  libxcb-xfixes0-dev \
  pkg-config \
  texinfo \
  wget \
  zlib1g-dev

hzs@ubuntu:~/video_server/ffmpeg$ sudo apt-get install nasm

hzs@ubuntu:~/video_server/ffmpeg$ sudo apt-get install yasm

hzs@ubuntu:~/video_server/ffmpeg$ sudo apt-get install libx264-dev

hzs@ubuntu:~/video_server/ffmpeg$ sudo apt-get install libx265-dev libnuma-dev

hzs@ubuntu:~/video_server/ffmpeg$ sudo apt-get install libvpx-dev

hzs@ubuntu:~/video_server/ffmpeg$ sudo apt-get install libfdk-aac-dev

hzs@ubuntu:~/video_server/ffmpeg$ sudo apt-get install libmp3lame-dev

hzs@ubuntu:~/video_server/ffmpeg$ sudo apt-get install libopus-dev

hzs@ubuntu:~/video_server/ffmpeg$ ./configure \
  --pkg-config-flags="--static" \
  --prefix=/home/nvidia/ffmpeg_build \
  --extra-libs="-lpthread -lm" \
  --enable-shared \
  --enable-gpl \
  --enable-libx264

hzs@ubuntu:~/video_server/ffmpeg$ make -j12

hzs@ubuntu:~/video_server/ffmpeg$ sudo make install

hzs@ubuntu:~/video_server/ffmpeg$ sudo vi /etc/ld.so.conf

在文件中添加路径：
/usr/local/lib

注意/usr/local/lib 目录是ffmpeg的安装目录，根据个人不同安装目录修改。

更新环境变量：
hzs@ubuntu:~/video_server/ffmpeg$ sudo ldconfig

执行ffmpeg
hzs@ubuntu:~/video_server/ffmpeg$ ffmpeg

出现以下内容证明安装成功
ffmpeg version N-95660-gfc7b6d5 Copyright (c) 2000-2019 the FFmpeg developers
  built with gcc 5.4.0 (Ubuntu 5.4.0-6ubuntu1~16.04.9) 20160609
  configuration: --pkg-config-flags=--static --extra-libs='-lpthread -lm' --enable-shared --enable-gpl --enable-libass --enable-libfdk-aac --enable-libfreetype --enable-libmp3lame --enable-libopus --enable-libvorbis --enable-libvpx --enable-libx264 --enable-libx265 --enable-nonfree
  libavutil      56. 35.101 / 56. 35.101
  libavcodec     58. 60.100 / 58. 60.100
  libavformat    58. 34.101 / 58. 34.101
  libavdevice    58.  9.100 / 58.  9.100
  libavfilter     7. 66.100 /  7. 66.100
  libswscale      5.  6.100 /  5.  6.100
  libswresample   3.  6.100 /  3.  6.100
  libpostproc    55.  6.100 / 55.  6.100
Hyper fast Audio and Video encoder
usage: ffmpeg [options] [[infile options] -i infile]... {[outfile options] outfile}...

Use -h to get full help or, even better, run 'man ffmpeg'



三、编译video_server

1. 编译jsoncpp

进入video_server_linux/3rdparty/jsoncpp-src-0.6.0-rc2 文件夹中，按照README.txt的文档操作，编译jsoncpp

hzs@ubuntu:~/video_server/opencv-4.2.0/build$ cd ../../video_server_linux/3rdparty/jsoncpp-src-0.6.0-rc2/
hzs@ubuntu:~/video_server/video_server_linux/3rdparty/jsoncpp-src-0.6.0-rc2$ python scons.py platform=linux-gcc （注意是python 不是python3）

编译好jsoncpp后在当前文件夹会出现libs/linux-gcc-5.4.0文件夹，然后将linux-gcc-5.4.0文件夹中的文件添加到/usr/local/lib中
hzs@ubuntu:~/video_server/video_server_linux/3rdparty/jsoncpp-src-0.6.0-rc2$ cd libs/linux-gcc-5.4.0/
hzs@ubuntu:~/video_server/video_server_linux/3rdparty/jsoncpp-src-0.6.0-rc2/libs/linux-gcc-5.4.0$ sudo cp libjson_linux-gcc-5.4.0_libmt.* /usr/local/lib/


hzs@ubuntu:~$ cd /home/hzs/video_server/video_server_linux
hzs@ubuntu:~/video_server/video_server_linux$ sudo apt-get install libboost1.58-all-dev


hzs@ubuntu:~/video_server/video_server_linux$ cd build/

先清空此文件夹
hzs@ubuntu:~/video_server/video_server_linux/build$ rm -rf *

hzs@ubuntu:~/video_server/video_server_linux/build$ cmake ..
hzs@ubuntu:~/video_server/video_server_linux/build$ make
...
[100%] Linking CXX executable video_server
[100%] Built target video_server

执行video_server
hzs@ubuntu:~/video_server/video_server_linux/build$ ./video_server 
Hello World!
开启视频流服务器，正在等在连接...


然后再windwos中打开test_video_server文件夹，修改dist/wfs.js中的第602行ar client = new WebSocket('ws://192.168.59.129:9002');中的IP地址，地址为video_server运行的ubuntu的IP地址，端口号可以在main.cpp中修改对应即可
最后双击打开playvmwarelinux.html可以预览即可，文件中有摄像头的地址，可以修改
