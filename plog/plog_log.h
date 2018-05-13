//修改自PLOG
//////////////////////////////////////////////////////////////////////////
//  PLOG - portable and simple log for C++
//  Documentation and sources: https://github.com/SergiusTheBest/NS_DLOG
//  License: MPL 2.0, http://mozilla.org/MPL/2.0/

#ifndef _PLOG_LOG_H_
#define _PLOG_LOG_H_

#include "../config.h"
#include "plog_logger.h"
#include "plog_init.h"

//////////////////////////////////////////////////////////////////////////
// Helper macros that get context info

#if _MSC_VER >= 1600 && !defined(__INTELLISENSE__) // >= Visual Studio 2010 and skip IntelliSense
#   define PLOG_GET_THIS()      __if_exists(this) { this } __if_not_exists(this) { 0 } 
#else
#   define PLOG_GET_THIS()      0
#endif

#ifdef _MSC_BUILD
#   define PLOG_GET_FUNC()      __FUNCTION__
#elif defined(__BORLANDC__)
#   define PLOG_GET_FUNC()      __FUNC__
#else
#   define PLOG_GET_FUNC()      __PRETTY_FUNCTION__
#endif

//////////////////////////////////////////////////////////////////////////
// Log severity level checker

#define IF_LOG_(instance, severity)     if (NS_DLOG::get<instance>() && NS_DLOG::get<instance>()->checkSeverity(severity))
#define IF_LOG(severity)                IF_LOG_(PLOG_DEFAULT_INSTANCE, severity)

//////////////////////////////////////////////////////////////////////////
// Main logging macros

#define LOG_(instance, severity)        IF_LOG_(instance, severity) (*NS_DLOG::get<instance>()) += NS_DLOG::Record(severity, __FILE__, PLOG_GET_FUNC(), __LINE__, PLOG_GET_THIS())
#define LOG(severity)                   LOG_(PLOG_DEFAULT_INSTANCE, severity)

#define LOG_TRACE                       LOG(NS_DLOG::trace)
#define LOG_DEBUG                       LOG(NS_DLOG::debug)
#define LOG_INFO                        LOG(NS_DLOG::info)
#define LOG_WARNING                     LOG(NS_DLOG::warning)
#define LOG_ERROR                       LOG(NS_DLOG::error)
#define LOG_FATAL                       LOG(NS_DLOG::fatal)

#define LOG_TRACE_(instance)            LOG_(instance, NS_DLOG::trace)
#define LOG_DEBUG_(instance)            LOG_(instance, NS_DLOG::debug)
#define LOG_INFO_(instance)             LOG_(instance, NS_DLOG::info)
#define LOG_WARNING_(instance)          LOG_(instance, NS_DLOG::warning)
#define LOG_ERROR_(instance)            LOG_(instance, NS_DLOG::error)
#define LOG_FATAL_(instance)            LOG_(instance, NS_DLOG::fatal)

#define LOGV                            LOG_TRACE
#define LOGD                            LOG_DEBUG
#define LOGI                            LOG_INFO
#define LOGW                            LOG_WARNING
#define LOGE                            LOG_ERROR
#define LOGF                            LOG_FATAL

#define LOGV_(instance)                 LOG_TRACE_(instance)
#define LOGD_(instance)                 LOG_DEBUG_(instance)
#define LOGI_(instance)                 LOG_INFO_(instance)
#define LOGW_(instance)                 LOG_WARNING_(instance)
#define LOGE_(instance)                 LOG_ERROR_(instance)
#define LOGF_(instance)                 LOG_FATAL_(instance)

//////////////////////////////////////////////////////////////////////////
// Conditional logging macros

#define LOG_IF_(instance, severity, condition)  if (condition) LOG_(instance, severity)
#define LOG_IF(severity, condition)             LOG_IF_(PLOG_DEFAULT_INSTANCE, severity, condition)

#define LOG_TRACE_IF(condition)                 LOG_IF(NS_DLOG::trace, condition)
#define LOG_DEBUG_IF(condition)                 LOG_IF(NS_DLOG::debug, condition)
#define LOG_INFO_IF(condition)                  LOG_IF(NS_DLOG::info, condition)
#define LOG_WARNING_IF(condition)               LOG_IF(NS_DLOG::warning, condition)
#define LOG_ERROR_IF(condition)                 LOG_IF(NS_DLOG::error, condition)
#define LOG_FATAL_IF(condition)                 LOG_IF(NS_DLOG::fatal, condition)

#define LOG_TRACE_IF_(instance, condition)      LOG_IF_(instance, NS_DLOG::trace, condition)
#define LOG_DEBUG_IF_(instance, condition)      LOG_IF_(instance, NS_DLOG::debug, condition)
#define LOG_INFO_IF_(instance, condition)       LOG_IF_(instance, NS_DLOG::info, condition)
#define LOG_WARNING_IF_(instance, condition)    LOG_IF_(instance, NS_DLOG::warning, condition)
#define LOG_ERROR_IF_(instance, condition)      LOG_IF_(instance, NS_DLOG::error, condition)
#define LOG_FATAL_IF_(instance, condition)      LOG_IF_(instance, NS_DLOG::fatal, condition)

#define LOGV_IF(condition)                      LOG_TRACE_IF(condition)
#define LOGD_IF(condition)                      LOG_DEBUG_IF(condition)
#define LOGI_IF(condition)                      LOG_INFO_IF(condition)
#define LOGW_IF(condition)                      LOG_WARNING_IF(condition)
#define LOGE_IF(condition)                      LOG_ERROR_IF(condition)
#define LOGF_IF(condition)                      LOG_FATAL_IF(condition)

#define LOGV_IF_(instance, condition)           LOG_TRACE_IF_(instance, condition)
#define LOGD_IF_(instance, condition)           LOG_DEBUG_IF_(instance, condition)
#define LOGI_IF_(instance, condition)           LOG_INFO_IF_(instance, condition)
#define LOGW_IF_(instance, condition)           LOG_WARNING_IF_(instance, condition)
#define LOGE_IF_(instance, condition)           LOG_ERROR_IF_(instance, condition)
#define LOGF_IF_(instance, condition)           LOG_FATAL_IF_(instance, condition)

#endif

