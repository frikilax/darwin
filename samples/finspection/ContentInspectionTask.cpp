/// \file     ContentInspection.cpp
/// \authors  jjourdin
/// \version  1.0
/// \date     10/09/18
/// \license  GPLv3
/// \brief    Copyright (c) 2018 Advens. All rights reserved.

#include <string>
#include <thread>
#include <unistd.h>

#include "../../toolkit/lru_cache.hpp"
#include "../toolkit/rapidjson/document.h"
#include "ContentInspectionTask.hpp"
#include "Logger.hpp"
#include "Stats.hpp"
#include "protocol.h"
#include "AlertManager.hpp"

ContentInspectionTask::ContentInspectionTask(boost::asio::local::stream_protocol::socket& socket,
                               darwin::Manager& manager,
                               std::shared_ptr<boost::compute::detail::lru_cache<xxh::hash64_t, unsigned int>> cache,
                               std::mutex& cache_mutex,
                               Configurations& configurations)
        : Session{"content_inspection", socket, manager, cache, cache_mutex} {
    _is_cache = _cache != nullptr;
    _configurations = configurations;
}

long ContentInspectionTask::GetFilterCode() noexcept {
    return DARWIN_FILTER_CONTENT_INSPECTION;
}

void ContentInspectionTask::operator()() {
    DARWIN_LOGGER;
    DARWIN_LOG_DEBUG("ContentInspectionTask:: started task");
    bool is_log = GetOutputType() == darwin::config::output_type::LOG;

    for(Packet *pkt : _packetList) {
        unsigned int certitude = 0;
        int tcpStatus = 0;
        SetStartingTime();

        pkt->enterTime = std::time(NULL);
        pkt->hash = calculatePacketFlowHash(pkt);

        if(pkt->flags & PKT_HASH_READY && _configurations.yaraCnf->scanType == SCAN_STREAM) {
            pkt->flow = getOrCreateFlowFromHash(pkt);

            if(pkt->flow && pkt->proto == IPPROTO_TCP) {
                pthread_mutex_lock(&(pkt->flow->mFlow));
                tcpStatus = handleTcpFromPacket(pkt);
                pthread_mutex_unlock(&(pkt->flow->mFlow));

            }
        }

        if(pkt->payloadLen) {
            rapidjson::Document yaraMeta;

            if(_configurations.yaraCnf->scanType == SCAN_STREAM &&
               pkt->flow && pkt->proto == IPPROTO_TCP && tcpStatus != -1) {
                StreamBuffer *sb;
                pthread_mutex_lock(&(pkt->flow->mFlow));
                TcpSession *session = (TcpSession *)pkt->flow->protoCtx;
                if(getPacketFlowDirection(pkt->flow, pkt) == TO_SERVER) {
                    sb = session->cCon->streamBuffer;
                }
                else {
                    sb = session->sCon->streamBuffer;
                }
                pthread_mutex_lock(&(sb->mutex));
                pthread_mutex_unlock(&(pkt->flow->mFlow));

                yaraMeta = yaraScan(pkt->payload, pkt->payloadLen, sb);
                pthread_mutex_unlock(&(sb->mutex));
            }
            else if(_configurations.yaraCnf->scanType == SCAN_PACKET_ONLY ||
                    tcpStatus == -1){
                yaraMeta = yaraScan(pkt->payload, pkt->payloadLen, NULL);
            }

            if(yaraMeta.IsArray()) {
                rapidjson::StringBuffer buffer;
                rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
                yaraMeta.Accept(writer);

                certitude = 100;
                if (certitude >= _threshold and certitude < DARWIN_ERROR_RETURN){
                    STAT_MATCH_INC;
                    std::string alert_log = R"({"evt_id": ")" + Evt_idToString() + R"(", "time": ")" + darwin::time_utils::GetTime() +
                             R"(", "filter": ")" + GetFilterName() + R"(", "certitude": )" + std::to_string(certitude) + R"(, "yara_match": )" +
                             std::string(buffer.GetString()) +
                             "}";
                    DARWIN_RAISE_ALERT(alert_log);
                    if (is_log) {
                        _logs += alert_log + "\n";
                    }
                }
            }
        }

        _certitudes.push_back(certitude);
        DARWIN_LOG_INFO("ContentInspectionTask:: processed entry in "
                         + std::to_string(GetDurationMs()) + "ms, certitude: " + std::to_string(certitude));
        freePacket(pkt);
    }

    DARWIN_LOG_DEBUG("ContentInspectionTask:: task finished");
}

bool ContentInspectionTask::ParseBody() {
    DARWIN_LOGGER;
    _packetList.clear();
    DARWIN_LOG_DEBUG("ContentInspectionTask:: ParseBody: _raw_body: " + _raw_body);

    try {
        _logs.clear();
        std::size_t packetMeta = 0, packetMetaEnd;
        std::size_t packetData, packetDataEnd = 0;
        std::size_t openingBracket;

        do {
            packetMeta = _raw_body.find("\"{", packetDataEnd + 1);
            if(packetMeta == std::string::npos) {
                break;
            }
            STAT_INPUT_INC;

            packetMetaEnd = _raw_body.find("}\",", packetMeta);
            if(packetMetaEnd == std::string::npos) {
                DARWIN_LOG_WARNING("ContentInspectionTask:: parse fail 1");
                STAT_PARSE_ERROR_INC;
                break;
            }

            packetData = _raw_body.find("\"{", packetMetaEnd);
            if(packetData == std::string::npos) {
                DARWIN_LOG_WARNING("ContentInspectionTask:: parse fail 2");
                STAT_PARSE_ERROR_INC;
                break;
            }

            packetDataEnd = _raw_body.find("}\"", packetData);
            if(packetDataEnd == std::string::npos) {
                DARWIN_LOG_WARNING("ContentInspectionTask:: parse fail 3");
                STAT_PARSE_ERROR_INC;
                break;
            }

            _packetList.push_back(getImpcapData(
                    _raw_body.substr(packetMeta + 1, packetMetaEnd - packetMeta),
                    _raw_body.substr(packetData + 1, packetDataEnd - packetData)
            ));
        } while(true);

    } catch (...) {
        DARWIN_LOG_CRITICAL("ContentInspectionTask:: ParseBody: Unknown Error");
        return false;
    }

    return true;
}
