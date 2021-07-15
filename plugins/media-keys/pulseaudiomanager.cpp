#include "pulseaudiomanager.h"
#include <QDebug>

pa_cvolume g_GetPaCV;
pa_cvolume g_SetPaCV;

int g_mute;
int g_volume;

char p_sinkName[128]="";
int8_t g_testValue = 0;

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

    if ( nullptr != p_PaMl) {
        pa_mainloop_free(p_PaMl);
    }
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
     qDebug()<<"PaContextStateCallback......";

     switch (pa_context_get_state(p_PaCtx)) {
     case PA_CONTEXT_FAILED:
     case PA_CONTEXT_TERMINATED:
     case PA_CONTEXT_READY:

          *context_state = PulseAudioContextState::PULSE_CONTEXT_READY;
           qDebug()<<" ok at..."<<__LINE__;
         break;
     default:
         qDebug()<<" ok at..."<<__LINE__;
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

    p_PaOp= pa_context_get_sink_info_list(p_PaCtx, getSinkInfoCallback, NULL);

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

    g_testValue = 88;

    if (!success) {
        return;
    }

    qDebug()<<"set volume ok..."<<__LINE__<<g_testValue;
}

void pulseAudioManager::getSinkInfoCallback(pa_context *ctx, const pa_sink_info *si, int isLast, void *userdata)
{
    Q_UNUSED(ctx);
    Q_UNUSED(userdata);


    if (isLast != 0) {
        return;
    }

    qDebug()<<"get volume ok..."<<si->name<<__LINE__;
    g_GetPaCV.channels = si->volume.channels;
    g_mute = si->mute;

    for (int k = 0; k < si->volume.channels; k++) {
        g_GetPaCV.values[k]=si->volume.values[k];
    }



    memset(p_sinkName,0x00,sizeof(p_sinkName));
    memcpy(p_sinkName,si->name,strlen(si->name));

    qDebug()<<si->name<<si->n_volume_steps<<__LINE__;

}

void pulseAudioManager::getSinkVolumeAndSetCallback(pa_context *ctx, const pa_sink_info *si, int isLast, void *userdata)
{
    Q_UNUSED(si);
    Q_UNUSED(isLast);

  qDebug()<<"set volume ok..."<<__LINE__;
  pa_operation_unref(pa_context_set_sink_volume_by_name(ctx, p_sinkName, (const pa_cvolume *)userdata, paActionDoneCallback, NULL));

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
        g_SetPaCV.values[k] = (PA_VOLUME_NORM)/100*Volume;
    }

    qDebug()<<"SET volume .."<<p_sinkName<<Volume<<__LINE__;
    p_PaOp = pa_context_get_sink_info_by_name(p_PaCtx, p_sinkName, getSinkVolumeAndSetCallback, &g_SetPaCV);

    if (nullptr == p_PaOp) {
        qDebug()<<"SET volume .."<<p_sinkName<<Volume<<__LINE__;
        return;
    }

    while (pa_operation_get_state(p_PaOp) == PA_OPERATION_RUNNING) {
        pa_mainloop_iterate(p_PaMl, 1, nullptr);
    }
}


void pulseAudioManager::setMute(bool MuteState)
{
    Q_UNUSED(MuteState);
    g_testValue = 99;
   p_PaOp = pa_context_set_sink_mute_by_name(p_PaCtx, p_sinkName, MuteState, paActionDoneCallback, NULL);

   if ( nullptr == p_PaOp ) {
       qDebug()<<"SET MuteState .."<<p_sinkName<<MuteState<<__LINE__;
       return;
   }
    qDebug()<<"set volume ok..."<<__LINE__<<g_testValue;
   while (pa_operation_get_state(p_PaOp) == PA_OPERATION_RUNNING) {
       qDebug()<<"set volume ok..."<<__LINE__<<g_testValue;
       pa_mainloop_iterate(p_PaMl, 1, nullptr);
       qDebug()<<"set volume ok..."<<__LINE__<<g_testValue;
   }
    qDebug()<<"set volume ok..."<<__LINE__<<g_testValue;
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
    p_PaOp = pa_context_get_sink_info_by_name(p_PaCtx, p_sinkName, getSinkInfoCallback, NULL);
    if (nullptr == p_PaOp) {
        return 0;
    }

    while (pa_operation_get_state(p_PaOp) == PA_OPERATION_RUNNING) {
        pa_mainloop_iterate(p_PaMl, 1, nullptr);
    }
    return g_mute;
}

int pulseAudioManager::getVolume()
{
    int ret = 0;

    p_PaOp = pa_context_get_sink_info_by_name(p_PaCtx, p_sinkName, getSinkInfoCallback, NULL);
    if (nullptr == p_PaOp) {
        return 0;
    }

    while (pa_operation_get_state(p_PaOp) == PA_OPERATION_RUNNING) {
        pa_mainloop_iterate(p_PaMl, 1, nullptr);
    }

    ret = (g_GetPaCV.values[0]*1.0/(PA_VOLUME_NORM/100));

    qDebug()<<"getVolume"<<ret;
    return ret;
}

bool pulseAudioManager::getMuteAndVolume(int *volume, int *mute)
{
    getVolume();

    *volume = g_volume;
    *mute = g_mute;
}

