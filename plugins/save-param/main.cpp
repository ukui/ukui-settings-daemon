/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * Copyright (C) 2019 Tianjin KYLIN Information Technology Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#include <QApplication>
#include "save-screen.h"
#include "clib-syslog.h"

bool g_isGet = false;
bool g_isSet = false;

static void parse_args (int argc, char *argv[])
{
    if (argc == 1) return;

    for (int i = 1; i < argc; ++i) {
        if (0 == QString::compare(QString(argv[i]).trimmed(), QString("--get"))) {
            g_isGet = true;
        } else if (0 == QString::compare(QString(argv[i]).trimmed(), QString("--set"))) {
            g_isSet = true;
        } else {
            if (argc > 1) {
//                print_help();
                USD_LOG(LOG_DEBUG, " Unsupported command line arguments: '%s'", argv[i]);
                exit(0);
            }
        }
    }
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    parse_args(argc, argv);
    SaveScreenParam saveParam;

    saveParam.setIsGet(g_isGet);
    saveParam.setIsSet(g_isSet);

    saveParam.getConfig();
    return app.exec();
}
