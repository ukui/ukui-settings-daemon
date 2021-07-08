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


//LOG_DEBUG,最低级
void syslog_info(int logLevel, const char *moduleName, const char *fileName, const char *functionName, int line, const char* fmt, ...)
{
    if (logLevel > LOG_LEVEL) return;
    static char hadInit=0;
    char buf[2048] = {0};
    char *logLevelstr = NULL;
    unsigned long tagLen = 0;
    va_list para;
    va_start(para, fmt);

    if (!hadInit){
        hadInit = 1;
        syslog_init("ukui-settings-daemon", LOG_LOCAL6);
    }

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
    snprintf(buf, sizeof buf - 1, "%s [%s] %s->%s %s line:%-5d ", logLevelstr, sysCategory, moduleName,fileName, functionName, line);
    tagLen = strlen(buf);
    vsnprintf(buf + tagLen, sizeof buf - 1 - tagLen, (const char*)fmt, para);

    printf ("%s\n",buf);

    closelog();
    va_end(para);
}

//////////////////flock////////////////////////////
/// \brief is_leap_year
/// \param year
/// \return
///
int is_leap_year(int year){
    if(year%400==0 || (year%4==0 && year%100!=0)){
        return 1;
    }
    return 0;
}
///
/// \brief nolocks_localtime
/// \param tmp
/// \param t
/// \param tz
/// \param dst
///
void nolocks_localtime(struct tm *tmp, time_t t, time_t tz, int dst) {
     const time_t secs_min = 60;
     const time_t secs_hour = 3600;
     const time_t secs_day = 3600*24;

     t -= tz;                           /* Adjust for timezone. */
     t += 3600*dst;                     /* Adjust for daylight time. */
     time_t days = t / secs_day;        /* Days passed since epoch. */
     time_t seconds = t % secs_day;     /* Remaining seconds. */

     tmp->tm_isdst = dst;

     tmp->tm_hour = seconds / secs_hour;
     tmp->tm_min = (seconds % secs_hour) / secs_min;
     tmp->tm_sec = (seconds % secs_hour) % secs_min;
     /* 1/1/1970 was a Thursday, that is, day 4 from the POV of the tm structure * where sunday = 0, so to calculate the day of the week we have to add 4 * and take the modulo by 7. */
     tmp->tm_wday = (days+4)%7;
     /* Calculate the current year. */
     tmp->tm_year = 1970;
     while(1) {
          /* Leap years have one day more. */
          time_t days_this_year = 365 + is_leap_year(tmp->tm_year);
          if (days_this_year > days) break;
          days -= days_this_year;
          tmp->tm_year++;
   }
   tmp->tm_yday = days;  /* Number of day of the current year. */

    /* We need to calculate in which month and day of the month we are. To do * so we need to skip days according to how many days there are in each * month, and adjust for the leap year that has one more day in February. */
    int mdays[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    mdays[1] += is_leap_year(tmp->tm_year);
    tmp->tm_mon = 0;
    while(days >= mdays[tmp->tm_mon]) {
        days -= mdays[tmp->tm_mon];
        tmp->tm_mon++;
    }
    tmp->tm_mday = days+1;  /* Add 1 since our 'days' is zero-based. */
    tmp->tm_year -= 1970;   /* Surprisingly tm_year is year-1900. */
}

///
/// \brief CreateDir
/// \param sPathName
/// \return
///
int CreateDir(const char *sPathName)
  {
      char DirName[256];
      strcpy(DirName, sPathName);
      int i,len = strlen(DirName);

      for(i=1; i<len; i++)
      {
          if(DirName[i]=='/')
          {
              DirName[i] = 0;
              if ( 0 != access(DirName, F_OK))
              {
                  if(mkdir(DirName, 0755)==-1)
                  {
                      printf("mkdir   error\n");
                      return -1;
                  }else{
//                        printf("DirName %s  create OK\n",DirName);
                  }
              }
               else{
//                  printf("Dir %s exist\n",DirName);
              }
               DirName[i] = '/';
          }

      }

      return 0;
}

///
/// \brief checkLogDir
/// \param dir
/// \param logFileName
///
void checkLogDir(const char *dir, char *logFileName){

    char *path = getenv("HOME");
    char logDir[128]="";

    memset(logDir,0x00,sizeof(logDir));
//    printf("HOME is %s \n", getenv("HOME"));
    snprintf(logDir,sizeof(logDir),"%s/.log/%s/",path,dir);

     if (0 != access(logDir,F_OK)){
        CreateDir(logDir);
   }else{
//        printf("dir %s exist\n", logDir);
   }

   memcpy(logFileName, logDir, strlen(logDir));
}
//按照星期进行处理
char getWeek(){
    time_t t;
    struct tm tmTime;
    time(&t);

//    localtime_r(&t, tmTime);//虽然线程安全但是容易死锁

    nolocks_localtime(&tmTime,t,-8*3600,0);
//    printf("[%d-%02d-%02d %02d:%02d:%02d]today is %d\n",tmTime.tm_year+1970,tmTime.tm_mon+1,tmTime.tm_mday,tmTime.tm_hour,tmTime.tm_min,tmTime.tm_sec,tmTime.tm_wday);

    return tmTime.tm_wday;
}

//加写锁
int wlock(int fd, int wait) {
    struct flock lock;
    lock.l_type = F_WRLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;
    lock.l_pid = -1;
    return fcntl(fd, wait ? F_SETLKW : F_SETLK, &lock);
}

//加读锁
int rlock(int fd, int wait) {
    struct flock lock;
    lock.l_type = F_RDLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;
    lock.l_pid = -1;
    return fcntl(fd, wait ? F_SETLKW : F_SETLK, &lock);
}

//解锁
int ulock(int fd) {
    struct flock lock;
    lock.l_type = F_UNLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;
    lock.l_pid = -1;
    return fcntl(fd, F_SETLK, &lock);
}
/*
以星期为划分，最多存七天的日志
当本次星期时间与上次不一致且并非初始值时，以清空文件的方式打开文件。
由于settings-daemon日志并不频繁，所以不需要进行缓冲区设置。
*/

void write_log_to_file(char *buf, __uint16_t buf_len)
{
    static int lastWeekDay = 0xff;
    const char *pWeekName[7] = {"SUN.log","MON.log","TUE.log","WED.log","THU.log","FRI.log","SAT.log"};
    FILE *lockfp;
    int fd;
    int writeLen = buf_len;
    int rtWeekDay;
    char logFileName[128];
    char logMsg[2048];
    time_t t;
    struct tm tmTime;

    t = writeLen;
    time(&t);
//    localtime_r(&t, tmTime);//虽然线程安全但是容易死锁
    memset(logMsg,0x00,sizeof(logMsg));

    nolocks_localtime(&tmTime,t,-8*3600,0);

    rtWeekDay = getWeek();
    memset(logFileName,0x00,sizeof(logFileName));
    checkLogDir("usd", logFileName);

    strcat(logFileName,pWeekName[rtWeekDay]);

    if(rtWeekDay!=lastWeekDay && lastWeekDay!=0xff){
        //清空文件打开，
        fd = open(logFileName, O_TRUNC|O_RDWR);
    }
    else {
        //追加打开
        fd = open(logFileName, O_RDWR | O_CREAT|O_APPEND, S_IRUSR | S_IWUSR);
    }
    lastWeekDay = rtWeekDay;
    if (wlock(fd, 1) == -1) {
        return;
    }
    //写并同步！
    lockfp = fdopen(fd,"w+");
    snprintf(logMsg,sizeof(logMsg),"{%d-%02d-%02d %02d:%02d:%02d}:%s\n",tmTime.tm_year+1970, tmTime.tm_mon+1, tmTime.tm_mday,tmTime.tm_hour, tmTime.tm_min,tmTime.tm_sec,buf);
   writeLen = write(fd,(const void*)logMsg,strlen(logMsg));

    printf("%s",logMsg);

    fflush(lockfp);
    //上锁
    ulock(fd);
    fclose (lockfp);
}

void syslog_to_self_dir(int logLevel, const char *moduleName, const char *fileName, const char *functionName, int line, const char* fmt, ...)
{
    char buf[2048] = {0};
    char *logLevelstr = NULL;
    unsigned long tagLen = 0;
    va_list para;
    va_start(para, fmt);

    memset(buf, 0, sizeof buf);

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

    snprintf(buf, sizeof buf - 1, "[%s] %s->%s %s line:%-5d", logLevelstr, moduleName,fileName, functionName, line);
    tagLen = strlen(buf);
    vsnprintf(buf + tagLen, sizeof buf - 1 - tagLen, (const char*)fmt, para);

    write_log_to_file(buf,strlen(buf));

    va_end(para);
}




//////////Other function////////////////
int CheckProcessAlive(const char *pName){
    int ret = 0;
    char Cmd[512] = {0};
    char *pAck = NULL;
    char CmdAck[12];
    FILE * pPipe;

    ret = *pAck;
    if (strlen(pName) > 400) {
        return 0;
    }

    //ef列出所有进程，|grep %s | 仅显示含%s的进程， |grep -v grep过滤掉含grep的进程（本条指令），|wc -l 查看行数
    sprintf(Cmd, "ps -ef |grep %s|grep -v grep|wc -l",pName);

    pPipe = popen(Cmd,"r");
    if (pPipe) {
        pAck = fgets(CmdAck,sizeof(CmdAck),pPipe);
        ret = atoi(CmdAck);
        pclose(pPipe);
    }

    return ret;
}
