
int wmain(int argc, wchar_t* argv[])
{
    //
    //  A GUI application should use COINIT_APARTMENTTHREADED instead of COINIT_MULTITHREADED.
    //
    HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (FAILED(hr))
    {
        printf("Unable to initialize COM: %x\n", hr);
        result = hr;
        goto Exit;
    }

    //
    //  Now that we've parsed our command line, pick the device to render.
    //
    if (!PickDevice(&device, &isDefaultDevice, &role))
    {
        result = -1;
        goto Exit;
    }
//  Based on the input switches, pick the specified device to use.
//
bool PickDevice(IMMDevice **DeviceToUse, bool *IsDefaultDevice, ERole *DefaultDeviceRole)
{
    HRESULT hr;
    bool retValue = true;
    IMMDeviceEnumerator *deviceEnumerator = NULL;
    IMMDeviceCollection *deviceCollection = NULL;

    *IsDefaultDevice = false;   // Assume we're not using the default device.

    hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&deviceEnumerator));
    if (FAILED(hr))
    {
        printf("Unable to instantiate device enumerator: %x\n", hr);
        retValue = false;
        goto Exit;
    }

    IMMDevice *device = NULL;

    //
    //  First off, if none of the console switches was specified, use the console device.
    //
    if (!UseConsoleDevice && !UseCommunicationsDevice && !UseMultimediaDevice && OutputEndpoint == NULL)
    {
        //
        //  The user didn't specify an output device, prompt the user for a device and use that.
        //
        hr = deviceEnumerator->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &deviceCollection);
        if (FAILED(hr))
        {
            printf("Unable to retrieve device collection: %x\n", hr);
            retValue = false;
            goto Exit;
        }

        printf("Select an output device:\n");
        printf("    0:  Default Console Device\n");
        printf("    1:  Default Communications Device\n");
        printf("    2:  Default Multimedia Device\n");
        UINT deviceCount;
        hr = deviceCollection->GetCount(&deviceCount);
        if (FAILED(hr))
        {
            printf("Unable to get device collection length: %x\n", hr);
            retValue = false;
            goto Exit;
        }
        for (UINT i = 0 ; i < deviceCount ; i += 1)
        {
            LPWSTR deviceName;

            deviceName = GetDeviceName(deviceCollection, i);
            if (deviceName == NULL)
            {
                retValue = false;
                goto Exit;
            }
            printf("    %d:  %S\n", i + 3, deviceName);
            free(deviceName);
        }
        wchar_t choice[10];
        _getws_s(choice);   // Note: Using the safe CRT version of _getws.

        long deviceIndex;
        wchar_t *endPointer;

        deviceIndex = wcstoul(choice, &endPointer, 0);
        if (deviceIndex == 0 && endPointer == choice)
        {
            printf("unrecognized device index: %S\n", choice);
            retValue = false;
            goto Exit;
        }
        switch (deviceIndex)
        {
        case 0:
            UseConsoleDevice = 1;
            break;
        case 1:
            UseCommunicationsDevice = 1;
            break;
        case 2:
            UseMultimediaDevice = 1;
            break;
        default:
            hr = deviceCollection->Item(deviceIndex - 3, &device);
            if (FAILED(hr))
            {
                printf("Unable to retrieve device %d: %x\n", deviceIndex - 3, hr);
                retValue = false;
                goto Exit;
            }
            break;
        }
    } 
    else if (OutputEndpoint != NULL)
    {
        hr = deviceEnumerator->GetDevice(OutputEndpoint, &device);
        if (FAILED(hr))
        {
            printf("Unable to get endpoint for endpoint %S: %x\n", OutputEndpoint, hr);
            retValue = false;
            goto Exit;
        }
    }

    if (device == NULL)
    {
        ERole deviceRole = eConsole;    // Assume we're using the console role.
        if (UseConsoleDevice)
        {
            deviceRole = eConsole;
        }
        else if (UseCommunicationsDevice)
        {
            deviceRole = eCommunications;
        }
        else if (UseMultimediaDevice)
        {
            deviceRole = eMultimedia;
        }
        hr = deviceEnumerator->GetDefaultAudioEndpoint(eRender, deviceRole, &device);
        if (FAILED(hr))
        {
            printf("Unable to get default device for role %d: %x\n", deviceRole, hr);
            retValue = false;
            goto Exit;
        }
        *IsDefaultDevice = true;
        *DefaultDeviceRole = deviceRole;
    }

    *DeviceToUse = device;
    retValue = true;
Exit:
    SafeRelease(&deviceCollection);
    SafeRelease(&deviceEnumerator);

    return retValue;
}

    printf("Render a %d hz Sine wave for %d seconds\n", TargetFrequency, TargetDurationInSec);

    //
    //  Instantiate a renderer and play a sound for TargetDuration seconds
    //
    //  Configure the renderer to enable stream switching on the specified role if the user specified one of the default devices.
    //
    {
        CWASAPIRenderer *renderer = new (std::nothrow) CWASAPIRenderer(device, isDefaultDevice, role);
CWASAPIRenderer::CWASAPIRenderer(IMMDevice *Endpoint, bool EnableStreamSwitch, ERole EndpointRole) : 
    _RefCount(1),
    _Endpoint(Endpoint),
    _AudioClient(NULL),
    _RenderClient(NULL),
    _RenderThread(NULL),
    _ShutdownEvent(NULL),
    _MixFormat(NULL),
    _RenderBufferQueue(0),
    _EnableStreamSwitch(EnableStreamSwitch),
    _EndpointRole(EndpointRole),
    _StreamSwitchEvent(NULL),
    _StreamSwitchCompleteEvent(NULL),
    _AudioSessionControl(NULL),
    _DeviceEnumerator(NULL),
    _InStreamSwitch(false)
{
    _Endpoint->AddRef();    // Since we're holding a copy of the endpoint, take a reference to it.  It'll be released in Shutdown();
}
        if (renderer == NULL)
        {
            printf("Unable to allocate renderer\n");
            return -1;
        }

        if (renderer->Initialize(TargetLatency))
        {
//  Initialize the renderer.
//
bool CWASAPIRenderer::Initialize(UINT32 EngineLatency)
{
    if (EngineLatency < 50)
    {
        printf("Engine latency in shared mode timer driven cannot be less than 50ms\n");
        return false;
    }
    //
    //  Create our shutdown event - we want auto reset events that start in the not-signaled state.
    //
    _ShutdownEvent = CreateEventEx(NULL, NULL, 0, EVENT_MODIFY_STATE | SYNCHRONIZE);
    if (_ShutdownEvent == NULL)
    {
        printf("Unable to create shutdown event: %d.\n", GetLastError());
        return false;
    }

    //
    //  Create our stream switch event- we want an auto reset event that starts in the not-signaled state.
    //  Note that we create this event even if we're not going to stream switch - that's because the event is used
    //  in the main loop of the renderer and thus it has to be set.
    //
    _StreamSwitchEvent = CreateEventEx(NULL, NULL, 0, EVENT_MODIFY_STATE | SYNCHRONIZE);
    if (_StreamSwitchEvent == NULL)
    {
        printf("Unable to create stream switch event: %d.\n", GetLastError());
        return false;
    }

    //
    //  Now activate an IAudioClient object on our preferred endpoint and retrieve the mix format for that endpoint.
    //
    HRESULT hr = _Endpoint->Activate(__uuidof(IAudioClient), CLSCTX_INPROC_SERVER, NULL, reinterpret_cast<void **>(&_AudioClient));
    if (FAILED(hr))
    {
        printf("Unable to activate audio client: %x.\n", hr);
        return false;
    }

    hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&_DeviceEnumerator));
    if (FAILED(hr))
    {
        printf("Unable to instantiate device enumerator: %x\n", hr);
        return false;
    }

    //
    // Load the MixFormat.  This may differ depending on the shared mode used
    //
    if (!LoadFormat())
//  Retrieve the format we'll use to render samples.
//
//  We use the Mix format since we're rendering in shared mode.
//
bool CWASAPIRenderer::LoadFormat()
{
    HRESULT hr = _AudioClient->GetMixFormat(&_MixFormat);
    if (FAILED(hr))
    {
        printf("Unable to get mix format on audio client: %x.\n", hr);
        return false;
    }

    _FrameSize = _MixFormat->nBlockAlign;
    if (!CalculateMixFormatType())
//  Crack open the mix format and determine what kind of samples are being rendered.
//
bool CWASAPIRenderer::CalculateMixFormatType()
{
    if (_MixFormat->wFormatTag == WAVE_FORMAT_PCM || 
        _MixFormat->wFormatTag == WAVE_FORMAT_EXTENSIBLE &&
            reinterpret_cast<WAVEFORMATEXTENSIBLE *>(_MixFormat)->SubFormat == KSDATAFORMAT_SUBTYPE_PCM)
    {
        if (_MixFormat->wBitsPerSample == 16)
        {
            _RenderSampleType = SampleType16BitPCM;
        }
        else
        {
            printf("Unknown PCM integer sample type\n");
            return false;
        }
    }
    else if (_MixFormat->wFormatTag == WAVE_FORMAT_IEEE_FLOAT ||
             (_MixFormat->wFormatTag == WAVE_FORMAT_EXTENSIBLE &&
               reinterpret_cast<WAVEFORMATEXTENSIBLE *>(_MixFormat)->SubFormat == KSDATAFORMAT_SUBTYPE_IEEE_FLOAT))
    {
        _RenderSampleType = SampleTypeFloat;
    }
    else 
    {
        printf("unrecognized device format.\n");
        return false;
    }
    return true;
}
    {
        return false;
    }
    return true;
}
    {
        printf("Failed to load the mix format \n");
        return false;
    }

    //
    //  Remember our configured latency in case we'll need it for a stream switch later.
    //
    _EngineLatencyInMS = EngineLatency;

    if (!InitializeAudioEngine())
//  Initialize WASAPI in timer driven mode.
//
bool CWASAPIRenderer::InitializeAudioEngine()
{
    HRESULT hr = _AudioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, 
        AUDCLNT_STREAMFLAGS_NOPERSIST, 
        _EngineLatencyInMS*10000, 
        0, 
        _MixFormat, 
        NULL);

    if (FAILED(hr))
    {
        printf("Unable to initialize audio client: %x.\n", hr);
        return false;
    }

    //
    //  Retrieve the buffer size for the audio client.
    //
    hr = _AudioClient->GetBufferSize(&_BufferSize);
    if(FAILED(hr))
    {
        printf("Unable to get audio client buffer: %x. \n", hr);
        return false;
    }

    hr = _AudioClient->GetService(IID_PPV_ARGS(&_RenderClient));
    if (FAILED(hr))
    {
        printf("Unable to get new render client: %x.\n", hr);
        return false;
    }

    return true;
}
    {
        return false;
    }

    if (_EnableStreamSwitch)
    {
        if (!InitializeStreamSwitch())
bool CWASAPIRenderer::InitializeStreamSwitch()
{
    HRESULT hr = _AudioClient->GetService(IID_PPV_ARGS(&_AudioSessionControl));
    if (FAILED(hr))
    {
        printf("Unable to retrieve session control: %x\n", hr);
        return false;
    }

    //
    //  Create the stream switch complete event- we want a manual reset event that starts in the not-signaled state.
    //
    _StreamSwitchCompleteEvent = CreateEventEx(NULL, NULL, CREATE_EVENT_INITIAL_SET | CREATE_EVENT_MANUAL_RESET, EVENT_MODIFY_STATE | SYNCHRONIZE);
    if (_StreamSwitchCompleteEvent == NULL)
    {
        printf("Unable to create stream switch event: %d.\n", GetLastError());
        return false;
    }
    //
    //  Register for session and endpoint change notifications.  
    //
    //  A stream switch is initiated when we receive a session disconnect notification or we receive a default device changed notification.
    //
    hr = _AudioSessionControl->RegisterAudioSessionNotification(this);
    if (FAILED(hr))
    {
        printf("Unable to register for stream switch notifications: %x\n", hr);
        return false;
    }

    hr = _DeviceEnumerator->RegisterEndpointNotificationCallback(this);
    if (FAILED(hr))
    {
        printf("Unable to register for stream switch notifications: %x\n", hr);
        return false;
    }

    return true;
}
        {
            return false;
        }
    }

    return true;
}
            //
            //  We've initialized the renderer.  Once we've done that, we know some information about the
            //  mix format and we can allocate the buffer that we're going to render.
            //
            //
            //  The buffer is going to contain "TargetDuration" seconds worth of PCM data.  That means 
            //  we're going to have TargetDuration*samples/second frames multiplied by the frame size.
            //
            UINT32 renderBufferSizeInBytes = (renderer->BufferSizePerPeriod()  * renderer->FrameSize());
            size_t renderDataLength = (renderer->SamplesPerSecond() * TargetDurationInSec * renderer->FrameSize()) + (renderBufferSizeInBytes-1);
            size_t renderBufferCount = renderDataLength / (renderBufferSizeInBytes);
            //
            //  Render buffer queue. Because we need to insert each buffer at the end of the linked list instead of at the head, 
            //  we keep a pointer to a pointer to the variable which holds the tail of the current list in currentBufferTail.
            //
            RenderBuffer *renderQueue = NULL;
            RenderBuffer **currentBufferTail = &renderQueue;

            double theta = 0;

            for (size_t i = 0 ; i < renderBufferCount ; i += 1)
            {
                RenderBuffer *renderBuffer = new (std::nothrow) RenderBuffer();
                if (renderBuffer == NULL)
                {
                    printf("Unable to allocate render buffer\n");
                    return -1;
                }
                renderBuffer->_BufferLength = renderBufferSizeInBytes;
                renderBuffer->_Buffer = new (std::nothrow) BYTE[renderBufferSizeInBytes];
                if (renderBuffer->_Buffer == NULL)
                {
                    printf("Unable to allocate render buffer\n");
                    return -1;
                }
                //
                //  Generate tone data in the buffer.
                //
                switch (renderer->SampleType())
    RenderSampleType SampleType() { return _RenderSampleType; }
    UINT32 FrameSize() { return _FrameSize; }
    WORD ChannelCount() { return _MixFormat->nChannels; }
    UINT32 SamplesPerSecond() { return _MixFormat->nSamplesPerSec; }
                {
                case CWASAPIRenderer::SampleTypeFloat:
                    GenerateSineSamples<float>(renderBuffer->_Buffer, renderBuffer->_BufferLength, TargetFrequency,
                                                renderer->ChannelCount(), renderer->SamplesPerSecond(), &theta);
                    break;
                case CWASAPIRenderer::SampleType16BitPCM:
                    GenerateSineSamples<short>(renderBuffer->_Buffer, renderBuffer->_BufferLength, TargetFrequency,
                                                renderer->ChannelCount(), renderer->SamplesPerSecond(), &theta);
                    break;
                }
template <typename T>
void GenerateSineSamples(BYTE *Buffer, size_t BufferLength, DWORD Frequency, WORD ChannelCount, DWORD SamplesPerSecond, double *InitialTheta)
{
    double sampleIncrement = (Frequency * (M_PI*2)) / (double)SamplesPerSecond;
    T *dataBuffer = reinterpret_cast<T *>(Buffer);
    double theta = (InitialTheta != NULL ? *InitialTheta : 0);

    for (size_t i = 0 ; i < BufferLength / sizeof(T) ; i += ChannelCount)
    {
        double sinValue = sin( theta );
        for(size_t j = 0 ;j < ChannelCount; j++)
        {
            dataBuffer[i+j] = Convert<T>(sinValue);
        }
        theta += sampleIncrement;
    }

    if (InitialTheta != NULL)
    {
        *InitialTheta = theta;
    }
}
                //
                //  Link the newly allocated and filled buffer into the queue.  
                //
                *currentBufferTail = renderBuffer;
                currentBufferTail = &renderBuffer->_Next;
            }

            //
            //  The renderer takes ownership of the render queue - it will free the items in the queue when it renders them.
            //
            if (renderer->Start(renderQueue))
//  Start rendering - Create the render thread and start rendering the buffer.
//
bool CWASAPIRenderer::Start(RenderBuffer *RenderBufferQueue)
{
    HRESULT hr;

    _RenderBufferQueue = RenderBufferQueue;

    //
    //  We want to pre-roll one buffer's worth of silence into the pipeline.  That way the audio engine won't glitch on startup.  
    //  We pre-roll silence instead of audio buffers because our buffer size is significantly smaller than the engine latency 
    //  and we can only pre-roll one buffer's worth of audio samples.
    //  
    //
    {
        BYTE *pData;
        hr = _RenderClient->GetBuffer(_BufferSize, &pData);
        if (FAILED(hr))
        {
            printf("Failed to get buffer: %x.\n", hr);
            return false;
        }
        hr = _RenderClient->ReleaseBuffer(_BufferSize, AUDCLNT_BUFFERFLAGS_SILENT);
        if (FAILED(hr))
        {
            printf("Failed to release buffer: %x.\n", hr);
            return false;
        }
    }
    //
    //  Now create the thread which is going to drive the renderer.
    //
    _RenderThread = CreateThread(NULL, 0, WASAPIRenderThread, this, 0, NULL);
    if (_RenderThread == NULL)
    {
        printf("Unable to create transport thread: %x.", GetLastError());
        return false;
    }

    //
    //  We're ready to go, start rendering!
    //
    hr = _AudioClient->Start();
    if (FAILED(hr))
    {
        printf("Unable to start render client: %x.\n", hr);
        return false;
    }

    return true;
}
            {
                do
                {
                    printf(".");
                    Sleep(1000);
                } while (--TargetDurationInSec);
                printf("\n");

                renderer->Stop();
//  Stop the renderer.
//
void CWASAPIRenderer::Stop()
{
    HRESULT hr;

    //
    //  Tell the render thread to shut down, wait for the thread to complete then clean up all the stuff we 
    //  allocated in Start().
    //
    if (_ShutdownEvent)
    {
        SetEvent(_ShutdownEvent);
    }

    hr = _AudioClient->Stop();
    if (FAILED(hr))
    {
        printf("Unable to stop audio client: %x\n", hr);
    }

    if (_RenderThread)
    {
        WaitForSingleObject(_RenderThread, INFINITE);

        CloseHandle(_RenderThread);
        _RenderThread = NULL;
    }

    //
    //  Drain the buffers in the render buffer queue.
    //
    while (_RenderBufferQueue != NULL)
    {
        RenderBuffer *renderBuffer = _RenderBufferQueue;
        _RenderBufferQueue = renderBuffer->_Next;
        delete renderBuffer;
    }

}
                renderer->Shutdown();
                SafeRelease(&renderer);
            }
        }
        else
        {
            renderer->Shutdown();
//  Shut down the render code and free all the resources.
//
void CWASAPIRenderer::Shutdown()
{
    if (_RenderThread)
    {
        SetEvent(_ShutdownEvent);
        WaitForSingleObject(_RenderThread, INFINITE);
        CloseHandle(_RenderThread);
        _RenderThread = NULL;
    }

    if (_ShutdownEvent)
    {
        CloseHandle(_ShutdownEvent);
        _ShutdownEvent = NULL;
    }

    if (_StreamSwitchEvent)
    {
        CloseHandle(_StreamSwitchEvent);
        _StreamSwitchEvent = NULL;
    }

    SafeRelease(&_Endpoint);
    SafeRelease(&_AudioClient);
    SafeRelease(&_RenderClient);

    if (_MixFormat)
    {
        CoTaskMemFree(_MixFormat);
        _MixFormat = NULL;
    }

    if (_EnableStreamSwitch)
    {
        TerminateStreamSwitch();
void CWASAPIRenderer::TerminateStreamSwitch()
{
    HRESULT hr;
    if (_AudioSessionControl != NULL)
    {
        hr = _AudioSessionControl->UnregisterAudioSessionNotification(this);
        if (FAILED(hr))
        {
            printf("Unable to unregister for session notifications: %x\n", hr);
        }
    }

    if (_DeviceEnumerator)
    {
        hr = _DeviceEnumerator->UnregisterEndpointNotificationCallback(this);
        if (FAILED(hr))
        {
            printf("Unable to unregister for endpoint notifications: %x\n", hr);
        }
    }

    if (_StreamSwitchCompleteEvent)
    {
        CloseHandle(_StreamSwitchCompleteEvent);
        _StreamSwitchCompleteEvent = NULL;
    }

    SafeRelease(&_AudioSessionControl);
    SafeRelease(&_DeviceEnumerator);
}
    }
}
            SafeRelease(&renderer);
        }
    }

Exit:
    SafeRelease(&device);
    CoUninitialize();
    return 0;
}



//  Render thread - processes samples from the audio engine
//
DWORD CWASAPIRenderer::WASAPIRenderThread(LPVOID Context)
{
    CWASAPIRenderer *renderer = static_cast<CWASAPIRenderer *>(Context);
    return renderer->DoRenderThread();
}

DWORD CWASAPIRenderer::DoRenderThread()
{
    bool stillPlaying = true;
    HANDLE waitArray[2] = {_ShutdownEvent, _StreamSwitchEvent};
    HANDLE mmcssHandle = NULL;
    DWORD mmcssTaskIndex = 0;

    HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (FAILED(hr))
    {
        printf("Unable to initialize COM in render thread: %x\n", hr);
        return hr;
    }

    if (!DisableMMCSS)
    {
        mmcssHandle = AvSetMmThreadCharacteristics(L"Audio", &mmcssTaskIndex);
        if (mmcssHandle == NULL)
        {
            printf("Unable to enable MMCSS on render thread: %d\n", GetLastError());
        }
    }

    while (stillPlaying)
    {
        HRESULT hr;
        //
        //  In Timer Driven mode, we want to wait for half the desired latency in milliseconds.
        //
        //  That way we'll wake up half way through the processing period to hand the 
        //  next set of samples to the engine.
        //
        DWORD waitResult = WaitForMultipleObjects(2, waitArray, FALSE, _EngineLatencyInMS/2);
        switch (waitResult)
        {
        case WAIT_OBJECT_0 + 0:     // _ShutdownEvent
            stillPlaying = false;       // We're done, exit the loop.
            break;
        case WAIT_OBJECT_0 + 1:     // _StreamSwitchEvent
            //
            //  We've received a stream switch request.
            //
            //  We need to stop the renderer, tear down the _AudioClient and _RenderClient objects and re-create them on the new.
            //  endpoint if possible.  If this fails, abort the thread.
            //
            if (!HandleStreamSwitchEvent())
            {
                stillPlaying = false;
            }
            break;
        case WAIT_TIMEOUT:          // Timeout
            //
            //  We need to provide the next buffer of samples to the audio renderer.  If we're done with our samples, we're done.
            //
            if (_RenderBufferQueue == NULL)
            {
                stillPlaying = false;
            }
            else
            {
                BYTE *pData;
                UINT32 padding;
                UINT32 framesAvailable;

                //
                //  We want to find out how much of the buffer *isn't* available (is padding).
                //
                hr = _AudioClient->GetCurrentPadding(&padding);
                if (SUCCEEDED(hr))
                {
                    //
                    //  Calculate the number of frames available.  We'll render
                    //  that many frames or the number of frames left in the buffer, whichever is smaller.
                    //
                    framesAvailable = _BufferSize - padding;

                    //
                    //  If the buffer at the head of the render buffer queue fits in the frames available, render it.  If we don't
                    //  have enough room to fit the buffer, skip this pass - we will have enough room on the next pass.
                    //
                    while (_RenderBufferQueue != NULL && (_RenderBufferQueue->_BufferLength <= (framesAvailable *_FrameSize)))
                    {
                        //
                        //  We know that the buffer at the head of the queue will fit, so remove it and write it into 
                        //  the engine buffer.  Continue doing this until we no longer can fit
                        //  the recent buffer into the engine buffer.
                        //
                        RenderBuffer *renderBuffer = _RenderBufferQueue;
                        _RenderBufferQueue = renderBuffer->_Next;

                        UINT32 framesToWrite = renderBuffer->_BufferLength / _FrameSize;
                        hr = _RenderClient->GetBuffer(framesToWrite, &pData);
                        if (SUCCEEDED(hr))
                        {
                            //
                            //  Copy data from the render buffer to the output buffer and bump our render pointer.
                            //
                            CopyMemory(pData, renderBuffer->_Buffer, framesToWrite*_FrameSize);
                            hr = _RenderClient->ReleaseBuffer(framesToWrite, 0);
                            if (!SUCCEEDED(hr))
                            {
                                printf("Unable to release buffer: %x\n", hr);
                                stillPlaying = false;
                            }
                        }
                        else
                        {
                            printf("Unable to release buffer: %x\n", hr);
                            stillPlaying = false;
                        }
                        //
                        //  We're done with this set of samples, free it.
                        //
                        delete renderBuffer;

                        //
                        //  Now recalculate the padding and frames available because we've consumed
                        //  some of the buffer.
                        //
                        hr = _AudioClient->GetCurrentPadding(&padding);
                        if (SUCCEEDED(hr))
                        {
                            //
                            //  Calculate the number of frames available.  We'll render
                            //  that many frames or the number of frames left in the buffer, 
                            //  whichever is smaller.
                            //
                            framesAvailable = _BufferSize - padding;
                        }
                        else
                        {
                            printf("Unable to get current padding: %x\n", hr);
                            stillPlaying = false;
                        }
                    }
                }
            }
            break;
        }
    }

    //
    //  Unhook from MMCSS.
    //
    if (!DisableMMCSS)
    {
        AvRevertMmThreadCharacteristics(mmcssHandle);
    }

    CoUninitialize();
    return 0;
}