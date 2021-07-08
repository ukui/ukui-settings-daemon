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
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>

#ifdef __cplusplus
extern "C" {
#endif
#include<time.h>
#if 1
#ifdef QT_NO_DEBUG
#define LOG_LEVEL LOG_INFO
#else
#define LOG_LEVEL LOG_DEBUG
#endif

#else
//__FILE__
#define LOG_LEVEL LOG_DEBUG
#endif

#if 0
#define USD_LOG(logLevel,...) {\
   syslog_info(logLevel, MODULE_NAME, __FILE__, __func__, __LINE__, ##__VA_ARGS__);\
}
#else
#define USD_LOG(logLevel,...) {\
   syslog_to_self_dir(logLevel, MODULE_NAME, __FILE__, __func__, __LINE__, ##__VA_ARGS__);\
}

#define USD_LOG_SHOW_OUTPUT(output) USD_LOG(LOG_DEBUG,":%s (%s)(%s) use [%s] mode at (%dx%d) id %d",  \
    output->name().toLatin1().data(), output->isConnected()? "connect":"disconnect", output->isEnabled()? "Enale":"Disable",    \
        output->currentModeId().toLatin1().data(), output->pos().x(), output->pos().y(),output->id())

#define USD_LOG_SHOW_PARAM1(a) USD_LOG(LOG_DEBUG,"%s : %d",#a,a)
#define USD_LOG_SHOW_PARAM2(a,b) USD_LOG(LOG_DEBUG,"%s : %d,%s : %d",#a,a,#b, b)
#define USD_LOG_SHOW_PARAM3(a,b,c) USD_LOG(LOG_DEBUG,"%s : %d,%s : %d",#a, a, #b, b, #c, c)
#define USD_LOG_SHOW_PARAM4(a,b,c,d) USD_LOG(LOG_DEBUG,"%s : %d,%s : %d",#a, a, #b, b, #c, c, #d, d)

#define USD_LOG(logLevel,...) syslog_info(logLevel, "MODULE_NAME", __FILE__, __func__, __LINE__, ##__VA_ARGS__);
#endif

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
void syslog_info(int logLevel, const char *moduleName, const char *fileName, const char *functionName, int line, const char* fmt, ...);
void syslog_to_self_dir(int logLevel, const char *moduleName, const char *fileName, const char *functionName, int line, const char* fmt, ...);

/*
* 检测进程是否存在
*/
int CheckProcessAlive(const char *pName);
#ifdef __cplusplus
}
#endif
#endif
