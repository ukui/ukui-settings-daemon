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
#include "clib-syslog.h"

static char sysCategory[128] = {0};
static int sysFacility = 0;

void syslog_init(const char *category, int facility)
{
    if (NULL == category) {
        return;
    }

    memset(sysCategory, 0, sizeof sysCategory);
    strncpy(sysCategory, category, sizeof sysCategory - 1);
    sysFacility = facility;
}

void syslog_info(int logLevel, const char *fileName, const char *functionName, int line, const char* fmt, ...)
{
    if (logLevel > LOG_LEVEL) return;

    char buf[2048] = {0};
    char *logLevelstr = NULL;
    unsigned long tagLen = 0;
    va_list para;
    va_start(para, fmt);

    memset(buf, 0, sizeof buf);

    openlog("", LOG_NDELAY, sysFacility);
    switch (logLevel) {
    case LOG_EMERG:
        logLevelstr = "EMERG";
        break;
    case LOG_ALERT:
        logLevelstr = "ALERT";
        break;
    case LOG_CRIT:
        logLevelstr = "CRIT";
        break;
    case LOG_ERR:
        logLevelstr = "ERROR";
        break;
    case LOG_WARNING:
        logLevelstr = "WARNING";
        break;
    case LOG_NOTICE:
        logLevelstr = "NOTICE";
        break;
    case LOG_INFO:
        logLevelstr = "INFO";
        break;
    case LOG_DEBUG:
        logLevelstr = "DEBUG";
        break;
    default:
        logLevelstr = "UNKNOWN";

    }
    snprintf(buf, sizeof buf - 1, "%s [%s] %s %s line:%-5d ", logLevelstr, sysCategory, fileName, functionName, line);
    tagLen = strlen(buf);
    vsnprintf(buf + tagLen, sizeof buf - 1 - tagLen, (const char*)fmt, para);
    closelog();
    va_end(para);
}
