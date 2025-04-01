#define CINTERFACE
#include <Mmdeviceapi.h>
#include <audioclient.h>
#include <assert.h>
#include <math.h>
typedef unsigned int            uint32;

#ifndef WASAPI_FREQUENCY 
#define WASAPI_FREQUENCY 44100
#endif
#ifndef WASAPI_FRAME_SIZE
#define WASAPI_FRAME_SIZE (256)
#endif
static uint32 SpeakersCount = 2;
static const GUID		IID_IAudioClient = { 0x1CB9AD4C, 0xDBFA, 0x4c32, 0xB1, 0x78, 0xC2, 0xF5, 0x68, 0xA7, 0x03, 0xB2 };
static const GUID		IID_IAudioRenderClient = { 0xF294ACFC, 0x3146, 0x4483, 0xA7, 0xBF, 0xAD, 0xDC, 0xA7, 0xC2, 0x60, 0xE2 };
static const GUID		CLSID_MMDeviceEnumerator = { 0xBCDE0395, 0xE52F, 0x467C, 0x8E, 0x3D, 0xC4, 0x57, 0x92, 0x91, 0x69, 0x2E };
static const GUID		IID_IMMDeviceEnumerator = { 0xA95664D2, 0x9614, 0x4F35, 0xA7, 0x46, 0xDE, 0x8D, 0xB6, 0x36, 0x17, 0xE6 };
static const GUID		PcmSubformatGuid = { STATIC_KSDATAFORMAT_SUBTYPE_IEEE_FLOAT };
int main(int)
{
	IMMDeviceEnumerator *	devices;
	IMMDevice *				device;
	IAudioClient *			client;
	IAudioRenderClient *	render_client;
	REFERENCE_TIME			min_duration;

	CoInitialize(0);
	HRESULT hr;
	hr = CoCreateInstance(CLSID_MMDeviceEnumerator, NULL, CLSCTX_ALL, IID_IMMDeviceEnumerator, (void**)&devices);
	assert(hr == S_OK);
	hr = devices->lpVtbl->GetDefaultAudioEndpoint(devices, eRender, eMultimedia, &device);
	assert(hr == S_OK);
	// Get its IAudioClient (used to set audio format, latency, and start/stop)
	hr = device->lpVtbl->Activate(device, IID_IAudioClient, CLSCTX_ALL, 0, (void **)&client);
	assert(hr == S_OK);
	WAVEFORMATEXTENSIBLE wave;
	memset(&wave, 0, sizeof(WAVEFORMATEXTENSIBLE));
	wave.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
	wave.Format.cbSize = 22;
	wave.Format.nChannels = 2;
	wave.Format.nSamplesPerSec = WASAPI_FREQUENCY;
	wave.Format.wBitsPerSample = wave.Samples.wValidBitsPerSample = 32;
	wave.Format.nBlockAlign = 2 * (32 / 8);
	memcpy(&wave.SubFormat, &PcmSubformatGuid, sizeof(GUID));
	wave.Format.nAvgBytesPerSec = WASAPI_FREQUENCY  * wave.Format.nBlockAlign;
	WAVEFORMATEX* wave2;
	hr = client->lpVtbl->IsFormatSupported(client, AUDCLNT_SHAREMODE_SHARED, (WAVEFORMATEX *)&wave, &wave2);
	if (hr == S_FALSE)
	{
		wave.Format = *wave2;
		CoTaskMemFree(wave2);
	}
	client->lpVtbl->GetDevicePeriod(client, 0, &min_duration);
	hr = client->lpVtbl->Initialize(client, AUDCLNT_SHAREMODE_SHARED, 0, min_duration, min_duration, (WAVEFORMATEX *)&wave, 0);

	uint32	buffer_size;
	hr = client->lpVtbl->GetBufferSize(client, &buffer_size);
	assert(hr == S_OK);
	hr = client->lpVtbl->GetService(client, IID_IAudioRenderClient, (void **)&render_client);
	assert(hr == S_OK);
	BYTE * data;
	hr = client->lpVtbl->Start(client);
	assert(hr == S_OK);
	float x = 0;
	float dx = 2.f * 3.14159f * 440.f / float(wave.Format.nSamplesPerSec);
	while (true)
	{
		uint32	frames_to_deliver, padding;
		hr = client->lpVtbl->GetCurrentPadding(client, &padding);
		if (hr != S_OK)
		{
			continue;
		}
		frames_to_deliver = buffer_size - padding;
		if (frames_to_deliver < WASAPI_FRAME_SIZE)
		{
			Sleep(0);
			continue;
		}
//update buffer
		if (!(render_client->lpVtbl->GetBuffer(render_client, WASAPI_FRAME_SIZE, &data)))
		{
			float* f = (float*)data;
			for (uint32 n = 0; n < WASAPI_FRAME_SIZE * 2; n += 2)
			{
				float v1 = cosf(x) * 0.1f;
				float v2 = sinf(x) * 0.1f;
				x += dx;
				f[n] = v1;
				f[n + 1] = v2;
			}
			render_client->lpVtbl->ReleaseBuffer(render_client, WASAPI_FRAME_SIZE, 0);
		}
	}
}

