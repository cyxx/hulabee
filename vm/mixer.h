
#ifndef MIXER_H__
#define MIXER_H__

#include "intern.h"

enum {
	MIXER_CHANNEL_STATUS_DONE = 1,
	MIXER_CHANNEL_STATUS_PLAYING,
	MIXER_CHANNEL_STATUS_PAUSED,
};

typedef void (*MixerLockProc)(int);

int Mixer_Init(int hz, MixerLockProc lockProc);
int Mixer_Fini();

int Mixer_CreateChannel();
void Mixer_DestroyChannel(int channel);

int Mixer_Open(int channel, int asset, const uint8_t *buffer, int size);
int Mixer_Play(int channel);
int Mixer_Stop(int channel);
int Mixer_GetStatus(int channel);
int Mixer_IsPlaying(int asset);
int Mixer_SetVolume(int channel, float volume);
int Mixer_SetPan(int channel, float pan);
int Mixer_MixStereoS16(int16_t *samples, int len);

#endif
