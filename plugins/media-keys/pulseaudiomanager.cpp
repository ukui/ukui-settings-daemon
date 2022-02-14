/* -*- Mode: C++; indent-tabs-mode: nil; tab-width: 4 -*-
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

#include "pulseaudiomanager.h"
#include <QDebug>

pa_cvolume g_GetPaCV;
pa_cvolume g_SetPaCV;

pa_channel_map g_sinkMap;

int g_mute;
int g_volume;

float g_balance = 0.0f;//声道平衡，设置之前先计算出先前的声道平衡值，设置时，带入计算即可。。
char g_sinkName[128] = "";
char g_sourceName[128] = "";
bool g_sourceMute = false;

pulseAudioManager::pulseAudioManager(QObject *parent)
    :QObject(parent)
{
    initPulseAudio();
}

pulseAudioManager::~pulseAudioManager()
{
    if ( nullptr != p_PaCtx ) {
        pa_context_set_state_callback(p_PaCtx, NULL, NULL);
        pa_context_disconnect(p_PaCtx);
        pa_context_unref(p_PaCtx);
    }
    pa_signal_done();
    if (nullptr != p_PaMl) {
        pa_mainloop_free(p_PaMl);
    }
    g_balance = 0;
    memset(&g_GetPaCV,0x00,sizeof(g_GetPaCV));
    memset(&g_SetPaCV,0x00,sizeof(g_SetPaCV));
    memset(&g_sinkMap,0x00,sizeof(g_sinkMap));

    memset(g_sinkName,0x00,sizeof(g_sinkName));
    memset(g_sourceName,0x00,sizeof(g_sourceName));
}

//pulseAudioManager *pulseAudioManager::getIntance()
//{
//    if (nullptr == mInstance) {
//        mInstance = new pulseAudioManager(nullptr);
//    }

//    return mInstance;
//}


void pulseAudioManager::PaContextStateCallback(pa_context* p_PaCtx, void* userdata)
{
     PulseAudioContextState* context_state = (PulseAudioContextState*)userdata;

     switch (pa_context_get_state(p_PaCtx)) {
     case PA_CONTEXT_FAILED:
     case PA_CONTEXT_TERMINATED:
     case PA_CONTEXT_READY:
          *context_state = PulseAudioContextState::PULSE_CONTEXT_READY;
         break;
     default:
         break;
     }
}


void pulseAudioManager::initPulseAudio()
{
    p_PaMl = pa_mainloop_new();

    if (!(p_PaMl)) {
        return;
    }

    p_PaMlApi = pa_mainloop_get_api(p_PaMl);
    if (!p_PaMlApi) { return ; }
    p_PaCtx = pa_context_new(p_PaMlApi, description);
      if (!(p_PaCtx)) { return ; }
    PulseAudioContextState context_state =
            PulseAudioContextState::PULSE_CONTEXT_INITIALIZING;

    pa_signal_init(p_PaMlApi);
//    pa_disable_sigpipe();



    pa_context_set_state_callback(p_PaCtx, PaContextStateCallback,
                                  &context_state);

    if (pa_context_connect(p_PaCtx, nullptr, PA_CONTEXT_NOFLAGS, NULL) < 0) {
        return ;
    }

    while (context_state == PulseAudioContextState::PULSE_CONTEXT_INITIALIZING) {
        pa_mainloop_iterate(p_PaMl, 1, NULL);
    }

    if (context_state == PulseAudioContextState::PULSE_CONTEXT_FINISHED) {
        return ;
    }

    p_PaOp = pa_context_get_server_info(p_PaCtx, getServerInfoCallback, NULL);

    while (pa_operation_get_state(p_PaOp) == PA_OPERATION_RUNNING) {
        pa_mainloop_iterate(p_PaMl, 1, nullptr);
    }

}

void pulseAudioManager::contextDrainComplete(pa_context *ctx, void *userdata)
{
    Q_UNUSED(userdata);
    pa_context_disconnect(ctx);
}

void pulseAudioManager::completeAction( )
{
//    if (!(o = pa_context_drain(p_PaCtx, contextDrainComplete, NULL)))
//        pa_context_disconnect(p_PaCtx);
//    else
//        pa_operation_unref(o);
}


void pulseAudioManager::paActionDoneCallback(pa_context *ctx, int success, void *userdata)
{
    Q_UNUSED(userdata);
    Q_UNUSED(ctx);
    if (!success) {
        return;
    }

}

void pulseAudioManager::getSourceInfoCallback(pa_context *ctx, const pa_source_info *so, int isLast, void *userdata)
{
    Q_UNUSED(ctx);
    Q_UNUSED(userdata);

    if (isLast != 0) {
        return;
    }
    g_sourceMute = so->mute;
}

void pulseAudioManager:: getServerInfoCallback(pa_context *ctx, const pa_server_info *si, void *userdata)
{
    Q_UNUSED(ctx);
    Q_UNUSED(userdata);
    memset(g_sinkName,0x00,sizeof(g_sinkName));
    memcpy(g_sinkName,si->default_sink_name,strlen(si->default_sink_name));
    memset(g_sourceName,0x00,sizeof(g_sourceName));
    memcpy(g_sourceName,si->default_source_name,strlen(si->default_source_name));

}


void pulseAudioManager::getSinkInfoCallback(pa_context *ctx, const pa_sink_info *si, int isLast, void *userdata)
{
    Q_UNUSED(ctx);
    Q_UNUSED(userdata);

    if (isLast != 0) {
        return;
    }
    g_sinkMap.channels = si->channel_map.channels;
    for (int i = 0; i < si->channel_map.channels; ++i) {
        g_sinkMap.map[i] = si->channel_map.map[i];
    }

    g_GetPaCV.channels = si->volume.channels;
    g_mute = si->mute;

    for (int k = 0; k < si->volume.channels; k++) {
        g_GetPaCV.values[k]=si->volume.values[k];
    }

    g_balance = pa_cvolume_get_balance(&g_GetPaCV,&g_sinkMap);

}

void pulseAudioManager::getSinkVolumeAndSetCallback(pa_context *ctx, const pa_sink_info *si, int isLast, void *userdata)
{
    Q_UNUSED(si);
    Q_UNUSED(isLast);

    pa_operation_unref(pa_context_set_sink_volume_by_name(ctx, g_sinkName, (const pa_cvolume *)userdata, paActionDoneCallback, NULL));
}

void pulseAudioManager::paCvOperationHandle(pa_operation *paOp)
{
    pa_operation_unref(paOp);
}

void pulseAudioManager::setVolume(int Volume)
{
    Q_UNUSED(Volume);
    g_SetPaCV.channels = g_GetPaCV.channels;

    for (int k = 0; k < g_GetPaCV.channels; k++) {
        g_SetPaCV.values[k] = Volume;
    }
    pa_cvolume *pcv = pa_cvolume_set_balance(&g_SetPaCV, &g_sinkMap, g_balance);

    if (NULL == pcv) {
        USD_LOG(LOG_ERR, "pa_cvolume_set_balance error!");
        return;
    }

    p_PaOp = pa_context_get_sink_info_by_name(p_PaCtx, g_sinkName, getSinkVolumeAndSetCallback, pcv);


    if (nullptr == p_PaOp) {
        USD_LOG(LOG_ERR, "pa_context_get_sink_info_by_name error![%s]",g_sinkName);
        return;
    }

    while (pa_operation_get_state(p_PaOp) == PA_OPERATION_RUNNING) {
        pa_mainloop_iterate(p_PaMl, 1, nullptr);
    }

}


void pulseAudioManager::setMute(bool MuteState)
{
    Q_UNUSED(MuteState);

    USD_LOG(LOG_DEBUG,"set %s is %d", g_sinkName, MuteState);
    p_PaOp = pa_context_set_sink_mute_by_name(p_PaCtx, g_sinkName, MuteState, paActionDoneCallback, NULL);

    if ( nullptr == p_PaOp ) {

        return;
    }

    while (pa_operation_get_state(p_PaOp) == PA_OPERATION_RUNNING) {
        pa_mainloop_iterate(p_PaMl, 1, nullptr);
    }
}

void pulseAudioManager::upVolume(int PerVolume)
{
    Q_UNUSED(PerVolume);
}

void pulseAudioManager::lowVolume(int PerVolume)
{
    Q_UNUSED(PerVolume);
}

bool pulseAudioManager::getMute()
{
    p_PaOp = pa_context_get_sink_info_by_name(p_PaCtx, g_sinkName, getSinkInfoCallback, NULL);
    if (nullptr == p_PaOp) {
        return 0;
    }

    while (pa_operation_get_state(p_PaOp) == PA_OPERATION_RUNNING) {
        pa_mainloop_iterate(p_PaMl, 1, nullptr);
    }

    return g_mute;
}
bool pulseAudioManager::getMicMute()
{
    p_PaOp = pa_context_get_source_info_by_name(p_PaCtx, g_sourceName, getSourceInfoCallback, NULL);
    if (nullptr == p_PaOp) {
        return 0;
    }

    while (pa_operation_get_state(p_PaOp) == PA_OPERATION_RUNNING) {
        pa_mainloop_iterate(p_PaMl, 1, nullptr);
    }
    return g_sourceMute;
}

void pulseAudioManager::setMicMute(bool MuteState)
{
    p_PaOp = pa_context_set_source_mute_by_name(p_PaCtx, g_sourceName, MuteState, paActionDoneCallback, NULL);
    if ( nullptr == p_PaOp ) {

        return;
    }

    while (pa_operation_get_state(p_PaOp) == PA_OPERATION_RUNNING) {
        pa_mainloop_iterate(p_PaMl, 1, nullptr);
    }
}

int pulseAudioManager::getVolume()
{
    int ret = 0;

    p_PaOp = pa_context_get_sink_info_by_name(p_PaCtx, g_sinkName, getSinkInfoCallback, NULL);
    if (nullptr == p_PaOp) {
        return 0;
    }

    while (pa_operation_get_state(p_PaOp) == PA_OPERATION_RUNNING) {
        pa_mainloop_iterate(p_PaMl, 1, nullptr);
    }

    ret = g_GetPaCV.values[0]>g_GetPaCV.values[1] ?g_GetPaCV.values[0]:g_GetPaCV.values[1];

    return ret;
}

int pulseAudioManager::getMaxVolume()
{
    return PA_VOLUME_NORM;
}

int pulseAudioManager::getStepVolume()
{

    return PA_VOLUME_NORM/100;
}

int pulseAudioManager::getMinVolume()
{
    return PA_VOLUME_NORM/500;
}

bool pulseAudioManager::getMuteAndVolume(int *volume, int *mute)
{
    getVolume();

    *volume = g_volume;
    *mute = g_mute;
}

bool pulseAudioManager::getSourceMute()
{
    p_PaOp = pa_context_get_source_info_by_name(p_PaCtx, g_sourceName, getSourceInfoCallback, NULL);

    if (nullptr == p_PaOp) {
        return 0;
    }

    while (pa_operation_get_state(p_PaOp) == PA_OPERATION_RUNNING) {
        pa_mainloop_iterate(p_PaMl, 1, nullptr);
    }
}

void pulseAudioManager::setSourceMute(bool Mute)
{
   pa_context_set_source_mute_by_name(p_PaCtx, g_sourceName, Mute, paActionDoneCallback, NULL);

   if ( nullptr == p_PaOp ) {

       return;
   }

   while (pa_operation_get_state(p_PaOp) == PA_OPERATION_RUNNING) {
       pa_mainloop_iterate(p_PaMl, 1, nullptr);
   }
}
