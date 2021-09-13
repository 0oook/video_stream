/**
 * Created by 胡泽双 on 2020/6/19
 *
 * @Time: 10:50 上午
 * @Project: websocket_test
 * @File: main.cpp
 * @IDE: CLion
 * @mail: godhudad@Gmail.com
 */

#include "VideoStreamServer.h"


int main() {
    VideoStreamServer video_server;
    video_server.run(9002);
    return 0;
}

