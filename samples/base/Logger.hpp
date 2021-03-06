/// \file     Logger.hpp
/// \authors  hsoszynski
/// \version  1.0
/// \date     09/04/18
/// \license  GPLv3
/// \brief    Copyright (c) 2018 Advens. All rights reserved.

#pragma once

# include <mutex>
# include <vector>
# include <string>
# include <fstream>
# include <iostream>

// Default log files

#  ifndef DARWIN_LOG_FILE
#   define DARWIN_LOG_FILE "/var/log/darwin/darwin.log"
#  endif //!DARWIN_LOG_FILE

#  ifndef DARWIN_LOGGER
#   define DARWIN_LOGGER darwin::logger::Logger& log = darwin::logger::Logger::instance()
#  endif //!DARWIN_LOGGER

#  ifndef DARWIN_LOG
#   define DARWIN_LOG(log_level, msg) log.log(log_level, msg)
#  endif //!DARWIN_LOG

#  ifndef DARWIN_LOG_DEBUG
#   define DARWIN_LOG_DEBUG(msg) log.log(darwin::logger::Debug, msg)
#  endif //!DARWIN_LOG_DEBUG

#  ifndef DARWIN_LOG_INFO
#   define DARWIN_LOG_INFO(msg) log.log(darwin::logger::Info, msg)
#  endif //!DARWIN_LOG_INFO

#  ifndef DARWIN_LOG_NOTICE
#   define DARWIN_LOG_NOTICE(msg) log.log(darwin::logger::Notice, msg)
#  endif //!DARWIN_LOG_NOTICE

#  ifndef DARWIN_LOG_WARNING
#   define DARWIN_LOG_WARNING(msg) log.log(darwin::logger::Warning, msg)
#  endif //!DARWIN_LOG_WARNING

#  ifndef DARWIN_LOG_ERROR
#   define DARWIN_LOG_ERROR(msg) log.log(darwin::logger::Error, msg)
#  endif //!DARWIN_LOG_ERROR

#  ifndef DARWIN_LOG_CRITICAL
#   define DARWIN_LOG_CRITICAL(msg) log.log(darwin::logger::Critical, msg)
#  endif //!DARWIN_LOG_CRITICAL

/// \namespace darwin
namespace darwin {

    /// \namespace logger
    namespace logger {
        /// \enum log_type
        enum log_type {
            Debug = 0,
            Info,
            Notice,
            Warning,
            Error,
            Critical
        };

        /// \class Logger
        class Logger {
        public:
            /// \brief Function to get the Logger or create it if he hasn't be created yet.
            /// \return Return the logger object.
            static Logger& instance() {
                static Logger log;
                return log;
            }

            /// \brief Function which permits to throw log.
            /// \param type is the level of log (DEBUG | INFO ... | CRITICAL)
            /// \param logMsg the message associated to the log to print.
            void log(log_type type, std::string const &logMsg);

            /// Set the logging level.
            ///
            /// \param type The log level to set.
            void setLevel(log_type type);

            /// Set the name of the module in the logger.
            ///
            /// \param name The name to set as the module name.
            void setName(std::string const& name);

            ///Rotate logs
            void RotateLogs();

        private:
            /// \brief This is the constructor of logger object.
            /// This constructor is private because only getLogger method can call it.
            Logger();

            /// \brief This is the destructor of logger object.
            ~Logger();

            /// \brief Copy operator.
            /// \param other, The logger to copy
            /// \return *this
            Logger &operator=(Logger const &other) {
                static_cast<void>(other);
                return *this;
            };

            /// \brief Do nothing just for good working of singleton
            /// \param other useless
            Logger(Logger const &other) { static_cast<void>(other); };

        private:
            log_type _logLevel;
            std::ofstream _file;
            std::string _name;
            std::mutex _fileMutex;
        };
    }
}