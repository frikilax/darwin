/// \file     ConnectionSupervisionTask.hpp
/// \authors  jjourdin
/// \version  1.0
/// \date     22/05/19
/// \license  GPLv3
/// \brief    Copyright (c) 2018 Advens. All rights reserved.

#pragma once

#include <string>
#include "protocol.h"
#include "Session.hpp"

#include "../../toolkit/lru_cache.hpp"
#include "../../toolkit/RedisManager.hpp"
#include "../toolkit/rapidjson/document.h"

#define DARWIN_FILTER_CONNECTION 0x636E7370

// To create a usable task method you MUST inherit from darwin::thread::Task publicly.
// The code bellow show all what's necessary to have a working task.
// For more information about Tasks, please refer to the class definition.

class ConnectionSupervisionTask : public darwin::Session {
public:
    explicit ConnectionSupervisionTask(boost::asio::local::stream_protocol::socket& socket,
                                       darwin::Manager& manager,
                                       std::shared_ptr<boost::compute::detail::lru_cache<xxh::hash64_t, unsigned int>> cache,
                                       std::mutex& cache_mutex,
                                       unsigned int expire);
    ~ConnectionSupervisionTask() override = default;

public:
    // You need to override the functor to compile and be executed by the thread
    void operator()() override;

protected:
    /// Return filter code
    long GetFilterCode() noexcept override;

private:
    /// Parse a line in the body.
    bool ParseLine(rapidjson::Value &line) final;

    /// Read a connection description from the session and
    /// perform a redis lookup.
    ///
    /// \return true on success, false otherwise.
    unsigned int REDISLookup(const std::string& connection) noexcept;

private:
    unsigned int _redis_expire;
    std::string _connection;
};
