#ifndef PLUSEAUDIOMANAGER_H
#define PLUSEAUDIOMANAGER_H

#include <QObject>

#include <pulse/error.h>
#include <pulse/pulseaudio.h>
#include <pulse/simple.h>

extern "C" {
    #include "clib-syslog.h"
}

class pulseAudioManager : public QObject
{
    Q_OBJECT

public:
    pulseAudioManager(QObject *parent = nullptr);
    ~pulseAudioManager();

    void initPulseAudio();

    void setMicMute(bool MuteState);

    void setMute(bool MuteState);

    void upVolume(int PerVolume);

    void lowVolume(int PerVolume);

    void setVolume(int Volume);

    bool getMute();

    bool getMicMute();

    int getVolume();

    int getMaxVolume();

    int getStepVolume();

    int getMinVolume();

    bool getMuteAndVolume(int *volume, int *mute);

    void paCvOperationHandle(pa_operation *paOp);

    bool getSourceMute();

    void setSourceMute(bool Mute);


//    pulseAudioManager* getIntance();
   static void PaContextStateCallback(pa_context* pa_ctx, void* userdata);
   static void getSinkInfoCallback(pa_context *ctx, const pa_sink_info *si, int isLast, void *userdata);
   static void getSinkVolumeAndSetCallback(pa_context *ctx, const pa_sink_info *si, int isLast, void *userdata);

   static void contextDrainComplete(pa_context *ctx, void *userdata);
   static void completeAction();
   static void paActionDoneCallback(pa_context *ctx, int success, void *userdata);

   static void getSourceInfoCallback(pa_context *ctx, const pa_source_info *si, int isLast, void *userdata);
private:

    enum class PulseAudioContextState {
        PULSE_CONTEXT_INITIALIZING,
        PULSE_CONTEXT_READY,
        PULSE_CONTEXT_FINISHED
    };

    const char *description = "audio recorder";



    pa_mainloop* p_PaMl = nullptr;
    pa_operation* p_PaOp = nullptr;
    pa_context* p_PaCtx = nullptr;
    pa_mainloop_api* p_PaMlApi = nullptr;

    QString mSinkName;
    pulseAudioManager *mInstance = nullptr;

};

#endif // PLUSEAUDIOMANAGER_H
