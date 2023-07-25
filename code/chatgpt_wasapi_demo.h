#include <windows.h>
#include <audioclient.h>
#include <mmdeviceapi.h>

// Define the audio callback class
class AudioCallback : public IAudioRenderClient
{
public:
    // Implement the necessary interface methods (not all are shown here)
    STDMETHODIMP QueryInterface(REFIID riid, void** ppvObject) { return E_NOTIMPL; }
    STDMETHODIMP_(ULONG) AddRef() { return 1; }
    STDMETHODIMP_(ULONG) Release() { return 1; }

    STDMETHODIMP GetBuffer(UINT32 NumFramesRequested, BYTE** ppData) { return E_NOTIMPL; }
    STDMETHODIMP ReleaseBuffer(UINT32 NumFramesWritten, DWORD dwFlags) { return E_NOTIMPL; }

    STDMETHODIMP GetMixFormat(WAVEFORMATEX** ppFormat)
    {
        // Specify the audio format you want to use (e.g., 16-bit PCM, 44100 Hz)
        static const WAVEFORMATEX waveFormat =
        {
            WAVE_FORMAT_PCM,       // Format tag
            2,                     // Channels (stereo)
            44100,                 // Samples per second (Hz)
            44100 * 2 * 2,         // Average bytes per second
            2 * 2,                 // Block alignment (bytes per sample * channels)
            16,                    // Bits per sample
            0                      // Extra size (set to 0 for PCM)
        };
        *ppFormat = const_cast<WAVEFORMATEX*>(&waveFormat);
        return S_OK;
    }
};

int main()
{
    HRESULT hr;
    CoInitializeEx(NULL, COINIT_MULTITHREADED);

    // Create an instance of the audio client
    IAudioClient* pAudioClient = NULL;
    hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&pEnumerator);
    if (FAILED(hr)) return 1;

    // Get the default audio output device
    IMMDevice* pDevice = NULL;
    hr = pEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &pDevice);
    if (FAILED(hr)) return 1;

    // Activate the audio client
    hr = pDevice->Activate(__uuidof(IAudioClient), CLSCTX_ALL, NULL, (void**)&pAudioClient);
    if (FAILED(hr)) return 1;

    // Initialize the audio client with shared mode and default audio format
    hr = pAudioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_EVENTCALLBACK, 0, 0, &waveFormat, 0);
    if (FAILED(hr)) return 1;

    // Get the buffer size
    UINT32 bufferSize;
    hr = pAudioClient->GetBufferSize(&bufferSize);
    if (FAILED(hr)) return 1;

    // Create the render client
    IAudioRenderClient* pRenderClient = NULL;
    hr = pAudioClient->GetService(__uuidof(IAudioRenderClient), (void**)&pRenderClient);
    if (FAILED(hr)) return 1;

    // Start audio playback
    hr = pAudioClient->Start();
    if (FAILED(hr)) return 1;

    // Main audio loop
    const int numFrames = bufferSize / sizeof(float);
    float* pBuffer;
    while (true)
    {
        // Get the available buffer
        hr = pRenderClient->GetBuffer(numFrames, (BYTE**)&pBuffer);
        if (FAILED(hr)) return 1;

        // TODO: Fill pBuffer with your audio data (e.g., from a file, synthesizer, etc.)

        // Release the buffer
        hr = pRenderClient->ReleaseBuffer(numFrames, 0);
        if (FAILED(hr)) return 1;
    }

    // Cleanup
    pRenderClient->Release();
    pAudioClient->Release();
    pDevice->Release();
    CoUninitialize();
    return 0;
}

int main()
{
    HRESULT hr;
    CoInitializeEx(NULL, COINIT_MULTITHREADED);

    // Create an instance of the audio client
    IAudioClient* pAudioClient = NULL;
    hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&pEnumerator);
    if (FAILED(hr)) return 1;

    // Get the default audio output device
    IMMDevice* pDevice = NULL;
    hr = pEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &pDevice);
    if (FAILED(hr)) return 1;

    // Activate the audio client
    hr = pDevice->Activate(__uuidof(IAudioClient), CLSCTX_ALL, NULL, (void**)&pAudioClient);
    if (FAILED(hr)) return 1;

    // Initialize the audio client with shared mode and default audio format
    WAVEFORMATEX* pMixFormat = NULL;
    hr = pAudioClient->GetMixFormat(&pMixFormat);
    if (FAILED(hr)) return 1;

    hr = pAudioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, 0, 0, 0, pMixFormat, NULL);
    CoTaskMemFree(pMixFormat); // Free the allocated memory for the mix format
    if (FAILED(hr)) return 1;

    // Get the buffer size
    UINT32 bufferSize;
    hr = pAudioClient->GetBufferSize(&bufferSize);
    if (FAILED(hr)) return 1;

    // Create the render client
    IAudioRenderClient* pRenderClient = NULL;
    hr = pAudioClient->GetService(__uuidof(IAudioRenderClient), (void**)&pRenderClient);
    if (FAILED(hr)) return 1;

    // Start audio playback
    hr = pAudioClient->Start();
    if (FAILED(hr)) return 1;

    // Main audio loop
    const int numFrames = bufferSize / sizeof(float);
    float* pBuffer;
    while (true)
    {
        // Get the available buffer
        hr = pRenderClient->GetBuffer(numFrames, (BYTE**)&pBuffer);
        if (FAILED(hr)) return 1;

        // TODO: Fill pBuffer with your audio data (e.g., from a file, synthesizer, etc.)

        // Release the buffer
        hr = pRenderClient->ReleaseBuffer(numFrames, 0);
        if (FAILED(hr)) return 1;
    }

    // Cleanup
    pRenderClient->Release();
    pAudioClient->Release();
    pDevice->Release();
    CoUninitialize();
    return 0;
}


void AudioThread(IAudioRenderClient* pRenderClient, UINT32 numFrames)
{
    // TODO: Implement your audio processing logic here.
    // Fill the 'pBuffer' with the appropriate audio data.
    // In a real application, you might read audio from a file, generate audio in real-time, etc.

    while (true)
    {
        float* pBuffer;
        HRESULT hr = pRenderClient->GetBuffer(numFrames, (BYTE**)&pBuffer);
        if (FAILED(hr))
        {
            // Handle error, sleep, or exit the thread gracefully.
            break;
        }

        // TODO: Fill pBuffer with your audio data (e.g., from a file, synthesizer, etc.)

        hr = pRenderClient->ReleaseBuffer(numFrames, 0);
        if (FAILED(hr))
        {
            // Handle error, sleep, or exit the thread gracefully.
            break;
        }
    }
}

int main()
{
    HRESULT hr;
    CoInitializeEx(NULL, COINIT_MULTITHREADED);

    // Create an instance of the audio client
    IAudioClient* pAudioClient = NULL;
    hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&pEnumerator);
    if (FAILED(hr)) return 1;

    // Get the default audio output device
    IMMDevice* pDevice = NULL;
    hr = pEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &pDevice);
    if (FAILED(hr)) return 1;

    // Activate the audio client
    hr = pDevice->Activate(__uuidof(IAudioClient), CLSCTX_ALL, NULL, (void**)&pAudioClient);
    if (FAILED(hr)) return 1;

    // Initialize the audio client with shared mode and default audio format
    WAVEFORMATEX* pMixFormat = NULL;
    hr = pAudioClient->GetMixFormat(&pMixFormat);
    if (FAILED(hr)) return 1;

    hr = pAudioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, 0, 0, 0, pMixFormat, NULL);
    CoTaskMemFree(pMixFormat); // Free the allocated memory for the mix format
    if (FAILED(hr)) return 1;

    // Get the buffer size
    UINT32 bufferSize;
    hr = pAudioClient->GetBufferSize(&bufferSize);
    if (FAILED(hr)) return 1;

    // Create the render client
    IAudioRenderClient* pRenderClient = NULL;
    hr = pAudioClient->GetService(__uuidof(IAudioRenderClient), (void**)&pRenderClient);
    if (FAILED(hr)) return 1;

    // Start audio playback
    hr = pAudioClient->Start();
    if (FAILED(hr)) return 1;

    // Create a separate thread for audio processing
    std::thread audioThread(AudioThread, pRenderClient, bufferSize / sizeof(float));

    // Wait for the audio processing thread to finish (optional)
    audioThread.join();

    // Cleanup
    pRenderClient->Release();
    pAudioClient->Release();
    pDevice->Release();
    CoUninitialize();
    return 0;
}
