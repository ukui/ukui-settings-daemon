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
    syslog(logLevel, "%s", buf);
    closelog();
    va_end(para);
}
