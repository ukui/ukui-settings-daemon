/* -*- Mode: C; indent-tabs-mode: nil; tab-width: 4 -*-
 * -*- coding: utf-8 -*-
 *
 * Copyright (C) 2020 KylinSoft Co., Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef CLIB_SYSLOG_H
#define CLIB_SYSLOG_H
#include <stdio.h>
#include <stdarg.h>
#include <syslog.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef QT_NO_DEBUG
#define LOG_LEVEL LOG_INFO
#else
#define LOG_LEVEL LOG_DEBUG
#endif
//__FILE__
#define CT_SYSLOG(logLevel,...) {\
    syslog_info(logLevel, "", __func__, __LINE__, ##__VA_ARGS__);\
}

/*
 * 日志参数初始化
 * @param category: 标签
 * @param facility: 设备文件
 * @return
 *      void
 */
void syslog_init(const char *category, int facility);

/*
 * 日志输出到system log，默认LOG_INFO级别
 * @param loglevel: 日志级别
 * @param file: 文件名
 * @param function: 函数
 * @param line: 函数
 * @param fmt: 格式化字符串
 * @return
 *      void
 */
void syslog_info(int logLevel, const char *file, const char *function, int line, const char* fmt, ...);

#ifdef __cplusplus
}
#endif
#endif
