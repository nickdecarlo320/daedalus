#include "stdafx.h"
#include "Plugins/AudioPlugin.h"


#include "Config/ConfigOptions.h"
#include "Core/Memory.h"
#include "Debug/DBGConsole.h"
#include "HLEAudio/AudioBuffer.h"
#include "HLEAudio/audiohle.h"
#include "Utility/FramerateLimiter.h"
#include "Utility/Thread.h"
#include "Utility/Timing.h"


EAudioPluginMode gAudioPluginEnabled = APM_DISABLED;

#define DEBUG_AUDIO 0

#if DEBUG_AUDIO
#define DPF_AUDIO(...) do {printf(__VA_ARGS__); } while(0)
#else
#define DPF_AUDIO(...) do { (void)sizeof(__VA_ARGS__); } while(0)
#endif

static const u32 kOutputFrequency = 44100;
static const u32 kAudioBufferSize = 1024 * 1024; // Circular buffer length
static const u32 kNumChannels = 2;

/*
How much input we try to keep buffered in the sychronisation code.
Setting this too low we run the risk of skipping
setting this too high we'll run the risk of being laggy
*/

static const u32 kMAxBufferLengthMs = 30;


class AudioPluginSDL : public CAudioPlugin
{
public:
		AudioPluginSDL();
		virtual ~AudioPluginSDL();

		virtual bool StartEmulation();
		virtual void StopEmulation();

		virtual void DacrateChanged(int system_type);
		virtual void LenChanged();
		virtual u32 ReadLength() { return 0; }
		virtual EProcessResult ProcessAList();

		void AddBuffer( void * ptr, u32 length);

		void StopAudio();
		void StartAudio();

	private:
		CAudioBuffer mAudioBuffer;
		u32 mFrequency;
		ThreadHandle mAudioThread;
		volatile bool mKeepRunning;
		volatile u32 mBufferLenMs;

	};

AudioPluginSDL::AudioPluginSDL()
:mAudioBuffer ( kAudioBufferSize)
, mFrequency (44100)
, mAudioThread( kInvalidThreadHandle )
, mKeepRunning (false)
, mBufferLenMs (0)
{}

AudioPluginSDL::~AudioPluginSDL()
{
	StopAudio();
}

bool AudioPluginSDL::StartEmulation()
{
	return true;
}

void AudioPluginSDL::StopEmulation()
{
	Audio_Reset();
	StopAudio();
}

void AudioPluginSDL::DacrateChanged(int system_type)
{
	u32 clock {(system_type ==ST_NTSC) ? VI_NTSC_CLOCK : VI_PAL_CLOCK};
	u32 dacrate {Memory_AI_GetRegister(AI_DACRATE_REG)};
	u32 frequency {clock / (dacrate + 1)};

	DBGConsole_Msg(0, "Audio Frequency %d", frequency);
	mFrequency = frequency;
}

void AudioPluginSDL::LenChanged()
{
	if (gAudioPluginEnabled > APM_DISABLED)
	{
		u32 address {Memory_AI_GetRegister(AI_DRAM_ADDR_REG) & 0xFFFFFF};
		u32 length {Memory_AI_GetRegister(AI_LEN_REG)};

		AddBuffer( g_pu8RamBase + address, length);
	}
	else
	{
		StopAudio();
	}
}

EProcessResult AudioPluginSDL::ProcessAList()
{
		Memory_SP_SetRegisterBits(SP_STATUS_REG, SP_STATUS_HALT);

		EProcessResult result = PR_NOT_STARTED;

		switch (gAudioPluginEnabled)
		{
				case APM_DISABLED:
					result = PR_COMPLETED;
					break;
				case APM_ENABLED_ASYNC:
					DAEDALUS_ERROR("ASync audio is not implemented");
					Audio_Ucode();
					result = PR_COMPLETED;
					break;
				case APM_ENABLED_SYNC:
					Audio_Ucode();
					result = PR_COMPLETED;
					break;
		}
		return result;
}
void AudioPluginSDL::AddBuffer( void * ptr, u32 length)
{
	if (length == 0)
		return;

		if (mAudioThread == kInvalidThreadHandle)
			StartAudio();

		u32 num_samples {length / sizeof (Sample)};
		mAudioBuffer.AddSamples( reinterpret_cast<const Sample *>(ptr), num_samples, mFrequency, kOutputFrequency);
		u32 remaining_samples {mAudioBuffer.GetNumBufferedSamples()};
		mBufferLenMs = (1000 * remaining_samples) / kOutputFrequency;
		float ms {(float)num_samples * 1000.0f / (float)mFrequency};
		DPF_AUDIO("Queing %d Samples @%dHz - %.2fms - bufferlen now %d\n", num_samples, mFrequency, ms, mBufferLenMs);
	}


void AudioPluginSDL::StartAudio()
{
	mKeepRunning = true;
//	mAudioThread == CreateThread("Audio", &AudioThread, this);
	if (mAudioThread == kInvalidThreadHandle)
	{
		DBGConsole_Msg(0, "Failed to start audio thread");
		mKeepRunning = false;
		FramerateLimiter_SetAuxillarySyncFunction(NULL, NULL);
	}
}

void AudioPluginSDL::StopAudio()
{
	if (mAudioThread == kInvalidThreadHandle)
		return;

		mKeepRunning = false;

		if (mAudioThread != kInvalidThreadHandle)
		{
			JoinThread(mAudioThread, -1);
			mAudioThread = kInvalidThreadHandle;
		}

		FramerateLimiter_SetAuxillarySyncFunction(NULL, NULL);
}
CAudioPlugin * CreateAudioPlugin()
{
	return NULL;
}
