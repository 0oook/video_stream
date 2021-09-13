/**
 * Created by 胡泽双 on 2020/6/19
 *
 * @Time: 10:48 上午
 * @Project: websocket_test
 * @File: video_server.cpp
 * @IDE: CLion
 * @mail: godhudad@Gmail.com
 */

#include "VideoStreamServer.h"


VideoStreamServer::VideoStreamServer() {
    // 设置日志等级
    console_sink->set_level(spdlog::level::trace);
    daily_sink->set_level(spdlog::level::trace);
    spdlog::set_level(spdlog::level::trace);

    av_log_set_level(AV_LOG_ERROR);

    // 设置日志刷新缓冲区的等级，一有这个等级的日志出现便刷新日志缓冲区，以防有一些日志不写入日志文件的情况
    logger.flush_on(spdlog::level::debug);

    logger.info("Init video server!");
    m_server.init_asio();

    // 设置端口重利用
    m_server.set_reuse_addr(true);
    m_server.set_open_handler(bind(&VideoStreamServer::on_open, this, ::_1));
    m_server.set_close_handler(bind(&VideoStreamServer::on_close, this, ::_1));
    m_server.set_message_handler(bind(&VideoStreamServer::on_message, this, ::_1, ::_2));

    // 设置mserver的日志等级（注意mserver的日志跟我用的spdlog日志不是一个日志），取消打印每次的数据接受发送时的frame_header和frame_payload信息
    m_server.clear_access_channels(websocketpp::log::alevel::frame_header);
    m_server.clear_access_channels(websocketpp::log::alevel::frame_payload);

    logger.info("Init ok!");
}

void VideoStreamServer::on_open(const connection_hdl &hdl) {
    // On open 函数，在客户端请求连接时执行，目前没有用到
    logger.info("On open!");
}

void VideoStreamServer::on_close(const connection_hdl &hdl) {
    logger.info("On close!!!!!!!!!!!!!!!!");
    bool delete_it_from_vector = false;
    std::lock_guard<std::mutex> vlock(connInfoVectorMutex);
    auto it = std::find_if(connInfoVector.begin(), connInfoVector.end(), ifHdlMatch(hdl));
    if (it != connInfoVector.end()) {
        it->conSet.erase(hdl);
        if (it->conSet.empty()) delete_it_from_vector = true;
    }
    if (delete_it_from_vector) {
        connInfoVector.erase(it);
    };
    logger.info("connInfoVector size: {0:d}", connInfoVector.size());
    for (auto &conn_info_object:connInfoVector) {
        logger.info("conn info object conSet size: {0:d}", conn_info_object.conSet.size());
    }
}

void VideoStreamServer::on_message(const connection_hdl &hdl, const server::message_ptr &msg) {
    try {
        // 解析客户端传过来的json字符串，取出cam_id和cam_url。
        Json::Reader jsonReader;
        Json::Value jsonValue;
        auto recv_msg = msg->get_payload();
        if (!recv_msg.empty() && jsonReader.parse(recv_msg, jsonValue, true)) {
            std::string cam_id;
            if (jsonValue["c"]["cam_id"].isInt()) {
                cam_id = std::to_string(jsonValue["c"]["cam_id"].asInt());
                logger.warn("Receive int cam id({0}) from client, already cast it to string!", cam_id);
            }
            else if (jsonValue["c"]["cam_id"].isDouble()) {
                cam_id = std::to_string(jsonValue["c"]["cam_id"].asDouble());
                logger.warn("Receive double cam id({0}) from client, already cast it to string!", cam_id);
            }
            else if (jsonValue["c"]["cam_id"].isString()) {
                cam_id = jsonValue["c"]["cam_id"].asString();
            }
            else {
                cam_id = "unknown";
            }

            std::string cam_url = jsonValue["c"]["address"].asString();

            logger.info("cam_id: {0}, cam url: {1}", cam_id, cam_url);

            // handle msg whether new thread.
            {
                std::lock_guard<std::mutex> vlock(connInfoVectorMutex);
                auto it = find_if(connInfoVector.begin(), connInfoVector.end(), ifConMatch(cam_id, cam_url));
                if (it != connInfoVector.end()) {
                    // 存在
                    it->conSet.insert(hdl);
                    logger.info("This cam_id({0}) or cam url({1}) already exists, now it's connections size: {2:d}",
                                cam_id, cam_url, it->conSet.size());
                }
                else {
                    // 不存在
                    con_set con_set_object{hdl};
                    connInfo conn_info_object{cam_id, cam_url, con_set_object};
                    connInfoVector.push_back(conn_info_object);
                    logger.info("This cam id({0}) or cam url({1}) don't exist!", cam_id, cam_url);
                    for (auto &iitt: connInfoVector) {
                        logger.info("This cam id({0}) or cam url({1}) conn size: {2:d}!", iitt.camId, iitt.camAddress,
                                    iitt.conSet.size());
                    }
                    // 新建线程连接摄像头并转发数据
                    std::thread sendPacketThread(bind(&VideoStreamServer::send_packet, this, cam_id, cam_url));
                    sendPacketThread.detach();
                }
            }
        }
        else {
            logger.warn("Error message!");
        }
    }
    catch (std::exception e) {
        logger.warn("Handle client message error: {0}", e.what());
    }
}

void VideoStreamServer::run(const uint16_t &port) {
    // std::thread tt(bind(&VideoStreamServer::monitorSystemInfo, this));
    m_server.listen(port);
    m_server.start_accept();
    m_server.run();
    // tt.join();
}

[[noreturn]] void VideoStreamServer::monitorSystemInfo(){
    while (true) {
        logger.info("");
        logger.info("");
        logger.info("---------------------------------------------------------------------------------------");
        logger.info("|                                     System Info                                     |");
        logger.info("---------------------------------------------------------------------------------------");
        {
            std::lock_guard<std::mutex> vlock(connInfoVectorMutex);
            logger.info("|\tconnInfoVector size: {0:d}", connInfoVector.size());
            for (const auto &it: connInfoVector) {
                logger.info("|\tcam id: {0} ===> conn size: {1:d}", it.camId, it.conSet.size());
            }
        }
        logger.info("---------------------------------------------------------------------------------------");
        logger.info("");
        logger.info("");
        sleep(3);
    }
};

void VideoStreamServer::send_packet(const std::string &cam_id, const std::string &cam_url) {
    logger.info("Init ffmpeg!");

    // 开始初始化ffmpeg。
    AVFormatContext *pFormatCtx = nullptr;
    int i, videoindex;
    AVCodecContext *pCodecCtx = nullptr;
    AVCodec *pCodec = nullptr;
    AVPacket *packet = nullptr;
    const char *Curl = cam_url.c_str();
    // av_register_all();
    avformat_network_init();
    pFormatCtx = avformat_alloc_context();

    // 设置ffmpeg打开摄像头的参数。
    AVDictionary *options = nullptr;
    av_dict_set(&options, "buffer_size", "1024000", 0); //设置缓存大小，1080p可将值调大
    av_dict_set(&options, "rtsp_transport", "tcp", 0);  // 以tcp的方式打开
    av_dict_set(&options, "stimeout", std::to_string(3 * 1000000).c_str(), 0);  // 连接超时时间
    av_dict_set(&options, "max_delay", "10000", 0);
    av_dict_set(&options, "preset", "superfast", 0);
    av_dict_set(&options, "probesize", "4096", 0);



    init_ffmpeg:

    // 这段代码主要用于通过goto标签走过来时判断一下客户端是否还在连接，避免出现客户端推出，该线程中ffmpeg还在一直
    // 重连摄像头不退出的情况
    {
        std::lock_guard<std::mutex> vlock(connInfoVectorMutex);
        auto it = find_if(connInfoVector.begin(), connInfoVector.end(), ifConMatch(cam_id, cam_url));
        if (it != connInfoVector.end()) {
            if (it->conSet.empty()) {
                logger.info("<cam_id: {0} || cam_address: {1}> has no connections, this thread terminaled", cam_id, cam_url);
                av_dict_free(&options);
                avcodec_close(pCodecCtx);
                av_packet_free(&packet);
                avformat_close_input(&pFormatCtx);
                return;
            }
        }
        else {
            logger.info("connInfoVector doesn't have <cam_id: {0} || cam_address: {1}>, this thread terminaled", cam_id, cam_url);
            av_dict_free(&options);
            avcodec_close(pCodecCtx);
            av_packet_free(&packet);
            avformat_close_input(&pFormatCtx);
            return;
        }
    }

    //打开网络rtsp流。
    if (avformat_open_input(&pFormatCtx, Curl, nullptr, &options) != 0) {
        av_dict_free(&options);
        avcodec_close(pCodecCtx);
        av_packet_free(&packet);
        avformat_close_input(&pFormatCtx);
        logger.info("<cam_id: {0} || cam_address: {1}> couldn't open input stream, retrying 3 seconds later!", cam_id, cam_url);
        sleep(3);
        goto init_ffmpeg;
    }
    pFormatCtx->probesize = 1000 * 1024;
    pFormatCtx->max_analyze_duration = 10 * AV_TIME_BASE;
    if (avformat_find_stream_info(pFormatCtx, nullptr) < 0) {
        av_dict_free(&options);
        avcodec_close(pCodecCtx);
        av_packet_free(&packet);
        avformat_close_input(&pFormatCtx);
        logger.info("<cam_id: {0} || cam_address: {1}> couldn't find stream information, retrying 3 seconds later!",
                    cam_id, cam_url);
        sleep(3);
        goto init_ffmpeg;
    }
    videoindex = -1;
    for (i = 0; i < pFormatCtx->nb_streams; ++i)
        if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoindex = i;
            break;
        }
    if (videoindex == -1) {
        av_dict_free(&options);
        avcodec_close(pCodecCtx);
        av_packet_free(&packet);
        avformat_close_input(&pFormatCtx);
        logger.info("<cam_id: {0} || cam_address: {1}> doesn't find a video stream, retrying 3 seconds later!", cam_id,
                    cam_url);
        sleep(3);
        goto init_ffmpeg;
    }
    pCodecCtx = avcodec_alloc_context3(nullptr);
    avcodec_parameters_to_context(pCodecCtx, pFormatCtx->streams[videoindex]->codecpar);

    pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
    if (pCodec == nullptr) {
        av_dict_free(&options);
        avcodec_close(pCodecCtx);
        av_packet_free(&packet);
        avformat_close_input(&pFormatCtx);
        logger.info("<cam_id: {0} || cam_address: {1}> doesn't find a codec, retrying 3 seconds later!", cam_id,
                    cam_url);
        sleep(3);
        goto init_ffmpeg;
    }
    if (avcodec_open2(pCodecCtx, pCodec, nullptr) < 0) {
        av_dict_free(&options);
        avcodec_close(pCodecCtx);
        av_packet_free(&packet);
        avformat_close_input(&pFormatCtx);
        logger.info("<cam_id: {0} || cam_address: {1}> couldn't open codec, retrying 3 seconds later!", cam_id,
                    cam_url);
        sleep(3);
        goto init_ffmpeg;
    }
    packet = (AVPacket *) av_malloc(sizeof(AVPacket));

    logger.info("Init ffmpeg success!");

    // 开始取流并转发数据包
    while (true) {
        if (av_read_frame(pFormatCtx, packet) >= 0) {
            // 从摄像头中取到了数据，则遍历对应的连接集合，并发送数据包。
            std::lock_guard<std::mutex> vlock(connInfoVectorMutex);
            auto itc = find_if(connInfoVector.begin(), connInfoVector.end(), ifConMatch(cam_id, cam_url));
            if (itc != connInfoVector.end()) {
                if (!itc->conSet.empty()) {
                    for (auto &conn: itc->conSet) {
                        try {
                            m_server.send(conn, packet->data, packet->size, websocketpp::frame::opcode::binary);
                        }
                        catch (websocketpp::exception const &e) {
                            logger.info("<cam_id: {0} || cam_address: {1}> websocket error: {2}!", cam_id, cam_url,
                                        e.what());
                        }
                        catch (...) {
                            logger.info("<cam_id: {0} || cam_address: {1}> send error!", cam_id, cam_url);
                        }
                    }
                    // usleep(1000*40);
                }
                else {
                    // 如果发包过程中所有的连接都断开了，则退出该发包线程。
                    logger.info("<cam_id: {0} || cam_address: {1}> has no connections!", cam_id, cam_url);
                    break;
                }
            }
            else {
                logger.info("connInfoVector doesn't have <cam_id: {0} || cam_address: {1}>!", cam_id, cam_url);
                break;
            }
            av_packet_unref(packet);
        }
        else {
            // 从摄像头中未取到数据，则重连摄像头。
            av_dict_free(&options);
            avcodec_close(pCodecCtx);
            av_packet_free(&packet);
            avformat_close_input(&pFormatCtx);
            logger.info("<cam_id: {0} || cam_address: {1}> thread av read frame failed, going to reinit ffmpeg and find stream info!", cam_id, cam_url);
            goto init_ffmpeg;
        }
    }

    // 释放资源，退出线程。
    av_dict_free(&options);
    avcodec_close(pCodecCtx);
    av_packet_free(&packet);
    avformat_close_input(&pFormatCtx);
    logger.info("<cam_id: {0} || cam_address: {1}> thread terminaled!", cam_id, cam_url);
}
