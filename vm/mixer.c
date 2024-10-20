
#include "mixer.h"
#include "util.h"

#define DR_MP3_IMPLEMENTATION
#define DR_MP3_NO_STDIO
#include "dr_mp3.h"

#define DR_WAV_IMPLEMENTATION
#define DR_WAV_NO_STDIO
#include "dr_wav.h"

#define MAX_CHANNELS 16

enum {
	TAG_RIFF = 0x46464952,
	TAG_WAVE = 0x45564157,
	TAG_fact = 0x74636166,
	TAG_fmt  = 0x20746D66,
	TAG_data = 0x61746164,
};

enum {
	TYPE_UNKNOWN = 0,
	TYPE_MP3,
	TYPE_WAV
};

struct mixer_channel_t {
	int asset;
	const uint8_t *buffer;
	uint32_t offset, size;
	int type;
	bool stereo;
	union {
		drmp3 mp3;
		drwav wav;
	} state;
	int status;
	struct mixer_channel_t *next;
};

static struct mixer_channel_t _channels[MAX_CHANNELS];
static struct mixer_channel_t *_next_channel;
static int _hz;
static MixerLockProc _lock;

static struct mixer_channel_t *find_free_channel() {
	struct mixer_channel_t *channel = _next_channel;
	if (channel) {
		_next_channel = _next_channel->next;
		channel->next = 0;
	} else {
		warning("find_free_channel MAX_CHANNELS");
	}
	return channel;
}

static void free_channel(struct mixer_channel_t *channel) {
	channel->next = _next_channel;
	_next_channel = channel;
}

static struct mixer_channel_t *get_channel(int channel) {
	assert(channel > 0 && channel < MAX_CHANNELS);
	struct mixer_channel_t *ch = &_channels[channel];
	return ch;
}

int Mixer_Init(int hz, MixerLockProc lock) {
	_hz = hz;
	_next_channel = &_channels[1];
	for (int i = 1; i < MAX_CHANNELS - 1; ++i) {
		_channels[i].next = &_channels[i + 1];
	}
	_lock = lock;
	return 0;
}

int Mixer_Fini() {
	return 0;
}

int Mixer_CreateChannel() {
	struct mixer_channel_t *ch = find_free_channel();
	return ch ? ch - _channels : 0;
}

void Mixer_DestroyChannel(int channel) {
	if (channel != 0) {
		struct mixer_channel_t *ch = get_channel(channel);
		assert(ch->next == 0);
		free_channel(ch);
	}
}

static int init_channel_mp3(struct mixer_channel_t *ch, int asset, const uint8_t *buffer, int size) {
	if (!drmp3_init_memory(&ch->state.mp3, buffer, size, 0)) {
		warning("Failed to load MP3 from asset:%d size:%d", asset, size);
		return -1;
	} else {
		if (ch->state.mp3.sampleRate != _hz) {
			warning("Unsupported sample rate %d for MP3", ch->state.mp3.sampleRate);
			return -1;
		}
	}
	ch->type   = TYPE_MP3;
	ch->buffer = buffer;
	ch->offset = 0;
	ch->size   = size;
	ch->stereo = (ch->state.mp3.channels == 2);
	return 0;
}

static int init_channel_wav(struct mixer_channel_t *ch, int asset, const uint8_t *buffer, int size) {
	if (!drwav_init_memory(&ch->state.wav, buffer, size, 0)) {
		warning("Failed to load WAV from asset:%d size:%d", asset, size);
		return -1;
	} else {
		if (ch->state.wav.sampleRate != _hz) {
			warning("Unsupported sample rate %d for WAV", ch->state.wav.sampleRate);
			return -1;
		}
	}
	ch->type   = TYPE_WAV;
	ch->buffer = buffer;
	ch->offset = 0;
	ch->size   = size;
	ch->stereo = (ch->state.wav.channels == 2);
	return 0;
}

int Mixer_Open(int channel, int asset, const uint8_t *buffer, int size) {
	_lock(1);
	struct mixer_channel_t *ch = get_channel(channel);
	ch->type = TYPE_UNKNOWN;
	ch->asset  = asset;
	ch->status = MIXER_CHANNEL_STATUS_DONE;
	uint32_t len, tag = READ_LE_UINT32(buffer);
	if (tag == TAG_RIFF) {
		int offset = 4;
		const uint32_t total = READ_LE_UINT32(buffer + offset) + 8; offset += 4;
		assert(READ_LE_UINT32(buffer + offset) == TAG_WAVE); offset += 4;
		/* find 'fmt ' */
		int compression = -1;
		while (offset < total && compression < 0) {
			tag = READ_LE_UINT32(buffer + offset); offset += 4;
			len = READ_LE_UINT32(buffer + offset); offset += 4;
			if (tag == TAG_fmt) {
				compression = READ_LE_UINT16(buffer + offset);
				break;
			}
			offset += len;
		}
		if (compression == 0x55) {
			/* find 'data' */
			while (offset < total) {
				tag = READ_LE_UINT32(buffer + offset); offset += 4;
				len = READ_LE_UINT32(buffer + offset); offset += 4;
				if (tag == TAG_data) {
					init_channel_mp3(ch, asset, buffer + offset, len);
					break;
				}
				offset += len;
			}
		} else {
			init_channel_wav(ch, asset, buffer, size);
		}
	} else {
		init_channel_mp3(ch, asset, buffer, size);
	}
	_lock(0);
	return 0;
}

int Mixer_Play(int channel) {
	_lock(1);
	struct mixer_channel_t *ch = get_channel(channel);
	if (ch->buffer) {
		ch->status = MIXER_CHANNEL_STATUS_PLAYING;
	}
	_lock(0);
	return 0;
}

int Mixer_Stop(int channel) {
	_lock(1);
	struct mixer_channel_t *ch = get_channel(channel);
	free_channel(ch);
	ch->buffer = 0;
	ch->status = MIXER_CHANNEL_STATUS_DONE;
	_lock(0);
	return 0;
}

int Mixer_GetStatus(int channel) {
	_lock(1);
	struct mixer_channel_t *ch = get_channel(channel);
	const int status = ch->status;
	_lock(0);
	return status;
}

int Mixer_IsPlaying(int asset) {
	for (int i = 1; i < MAX_CHANNELS; ++i) {
		struct mixer_channel_t *channel = &_channels[i];
		if (channel->buffer && channel->asset == asset) {
			return channel->status;
		}
	}
	return MIXER_CHANNEL_STATUS_DONE;
}

static int16_t clipS16(int sample) {
	return ((sample < SHRT_MIN) ? SHRT_MIN : ((sample > SHRT_MAX) ? SHRT_MAX : sample));
}

int Mixer_MixStereoS16(int16_t *samples, int len) {
	memset(samples, 0, sizeof(int16_t) * 2 * len);
	int16_t *buffer = alloca(sizeof(int16_t) * 2 * len);
	for (int i = 1; i < MAX_CHANNELS; ++i) {
		struct mixer_channel_t *ch = &_channels[i];
		if (ch->buffer && ch->status == MIXER_CHANNEL_STATUS_PLAYING) {
			int count = 0;
			switch (ch->type) {
			case TYPE_MP3:
				count = drmp3_read_pcm_frames_s16(&ch->state.mp3, len, buffer);
				break;
			case TYPE_WAV:
				count = drwav_read_pcm_frames_s16(&ch->state.wav, len, buffer);
				break;
			}
			if (count == 0) {
				ch->status = MIXER_CHANNEL_STATUS_DONE;
				continue;
			}
			if (ch->stereo) {
				for (int i = 0; i < count * 2; ++i) {
					samples[i] = clipS16(samples[i] + buffer[i]);
				}
			} else {
				for (int i = 0; i < count * 2; ++i) {
					samples[i] = clipS16(samples[i] + buffer[i / 2]);
				}
			}
		}
	}
	return 0;
}
