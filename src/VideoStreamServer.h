/**
 * Created by 胡泽双 on 2020/6/19
 *
 * @Time: 10:48 上午
 * @Project: video_server
 * @File: video_server.h
 * @IDE: CLion
 * @mail: godhudad@Gmail.com
 */

#ifndef VIDEO_SERVER_SRC_VIDEOSERVER_H
#define VIDEO_SERVER_SRC_VIDEOSERVER_H

#include <set>
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include <utility>
#include <json/json.h>
#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/daily_file_sink.h"

extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavdevice/avdevice.h"
#include "libavutil/log.h"
#include "libavutil/imgutils.h"
}

typedef websocketpp::server<websocketpp::config::asio> server;

using websocketpp::connection_hdl;
using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;


typedef std::set<connection_hdl, std::owner_less<connection_hdl>> con_set;

/**
 * @description 存储连接句柄的数据结构，将同一个摄像头对应的连接句柄存储到同一个容器中，方便发送数据包
 * @date 2020/6/21
 * @time 12:56 下午
 * @version 1.0.0
 */
struct connInfo {
    std::string camId;  // 摄像头ID
    std::string camAddress;  // 摄像头对应的rtsp地址
    con_set conSet;  // 存放对应客户端连接句柄的集合
};

/**
 * @description 通过仿函数的形式来区分客户端连接时，需要连接的摄像头是否已经存在对应的数据结构中
 * @date 2020/6/21
 * @time 12:58 下午
 * @version 1.0.0
 */
struct ifConMatch {
    std::string camId;
    std::string camAddress;

    explicit ifConMatch(std::string cam_id, std::string cam_address) : camId(std::move(cam_id)),
                                                                       camAddress(std::move(cam_address)) {}

    /**
     * @description 仿函数
     * @param connInfo_object: 需要判断的connInfo对象
     * @date 2020/6/21
     * @time 1:09 下午
     * @return 返回判断的connInfo对象是否与对应的摄像头ID或者摄像头rtsp地址匹配
     * @version 0.0.2
     */
    bool operator()(const connInfo &connInfo_object) const {
        return (connInfo_object.camId == camId && connInfo_object.camAddress == camAddress);
    }
};

/**
 * @description 判断连接句柄是否匹配
 * @date 2020/6/21
 * @time 1:12 下午
 * @version 0.0.1
 */
struct ifHdlMatch {
    connection_hdl HDL;

    explicit ifHdlMatch(connection_hdl hdl) : HDL(std::move(hdl)) {}

    /**
     * @description 判断connInfo_object中是否存在对应的hdl连接句柄
     * @param connInfo_object: 需要进行判断的connIfo对象
     * @date 2020/6/21
     * @time 1:13 下午
     * @return connIfo对象中是否存在对应的hdl连接句柄
     * @version 0.0.1
     */
    bool operator()(const connInfo &connInfo_object) const {
        auto it = connInfo_object.conSet.find(HDL);
        return (it != connInfo_object.conSet.end());
    }
};

/**
 * @description 视频流服务器类
 * @date 2020/6/21
 * @time 12:38 下午
 * @version 0.1.1
 */
class VideoStreamServer {
public:
    VideoStreamServer();

    /**
     * @description 客户端连接时执行的动作，目前没有用到
     * @param hdl: 客户端连接的句柄
     * @date 2020/6/21
     * @time 12:47 下午
     * @return 无
     * @version 0.0.1
     */
    void on_open(const connection_hdl &hdl);

    /**
     * @description 客户端关闭连接时执行的动作，需要将其对应的连接句柄从vector中删除
     * @param hdl: 客户端关闭的连接句柄
     * @date 2020/6/21
     * @time 12:37 下午
     * @return 无
     * @version 0.1.1
     */
    void on_close(const connection_hdl &hdl);

    /**
     * @description 客户端连接后发送的消息在这里进行处理
     * @param hdl: 发送消息的客户端连接句柄
     * @param msg: 客户端发送过来的消息
     * @date 2020/6/21
     * @time 12:48 下午
     * @return 无
     * @version 0.0.1
     */
    void on_message(const connection_hdl &hdl, const server::message_ptr &msg);

    /**
     * @description 运行websocket服务器
     * @param port: 端口号
     * @date 2020/6/21
     * @time 12:50 下午
     * @return 无
     * @version 0.1.1
     */
    void run(const uint16_t &port);

    [[noreturn]] void monitorSystemInfo();

    /**
     * @description 调用FFmpeg从摄像头取流，并将取到的数据包通过websocket服务器发送出去
     * @param cam_id: 摄像头对应的ID
     * @param cam_url: 摄像头对应的rtsp地址
     * @date 2020/6/21
     * @time 12:53 下午
     * @return 无
     * @version 1.0.1
     */
    void send_packet(const std::string &cam_id, const std::string &cam_url);

private:
    // websocket服务器对象
    server m_server;

    // 存放连接信息的vector
    std::vector<connInfo> connInfoVector;

    // vector的锁
    std::mutex connInfoVectorMutex;

    // 控制台log
    std::shared_ptr<spdlog::sinks::stdout_color_sink_mt> console_sink{std::make_shared<spdlog::sinks::stdout_color_sink_mt>()};

    // 文件log
    std::shared_ptr<spdlog::sinks::daily_file_sink_mt> daily_sink{std::make_shared<spdlog::sinks::daily_file_sink_mt>("logs/log.txt", 0, 0)};

    // logger对象，包含了控制台logger和文件logger
    spdlog::logger logger{"videoServerLogger", {console_sink, daily_sink}};
};

#endif //VIDEO_SERVER_SRC_VIDEOSERVER_H
