
enum Render_sample_type{
	SAMPLE_TYPE_F32,
	SAMPLE_TYPE_S16,
};

struct Samples_buffer
{
	f32* data;
	u32 pos;
	u32 last_filled_pos;
	u32 count;
};

struct Samples_stream
{
	f32* data;
	u32 pos;
	u32 count;
};

struct Wasapi_renderer{
	u32 ref_count;
	//
	//  Core Audio Rendering member variables.
	//
	IMMDevice * endpoint;
	IAudioClient *audio_client;
	IAudioRenderClient *render_client;

	u32 frame_size;
	Render_sample_type render_sample_type;
	u32 engine_latency_ms;
	
	WAVEFORMATEX *mix_format;

	//  Render buffer management.
	u32 client_buffer_size;

	Samples_buffer samples_buffer;
	
	HANDLE      render_thread;
	HANDLE      shutdown_event;
	HANDLE 		stream_switch_event;
};

#if 0
DWORD WINAPI
wasapi_render_thread(LPVOID context)
{
	Wasapi_renderer* renderer = (Wasapi_renderer*)context;
	b32 still_playing = true;

	HANDLE wait_array[1] = {renderer->shutdown_event};

	HRESULT hr = CoInitializeEx(0, COINIT_MULTITHREADED);
	if(FAILED(hr)) return hr;
	// We want to make sure that our timer resolution is a multiple of the latency,
	// otherwise the system timer cadence will
	// cause us to starve the renderer.

	//  Set the system timer to 1ms as a worst case value.
	timeBeginPeriod(1);
	
	#if USE_MMCSS
		DWORD mmcss_task_index = 0;
		HANDLE mmcss_handle = AvSetMmThreadCharacteristicsA("Audio", &mmcss_task_index);
		ASSERT(mmcss_handle);
	#endif

	//  When running in timer mode, wait for half the configured latency.
	while(still_playing){
		DWORD wait_result = WaitForMultipleObjects(1, wait_array, false, renderer->engine_latency_ms/2);
		switch(wait_result){
			//TODO: WTF
			case WAIT_OBJECT_0 + 0:{ // shutdown event
				still_playing = false;
			}break;
			case WAIT_TIMEOUT:{
				//  We need to provide the next buffer of samples to the audio renderer.  
				// If we're done with our samples, we're done.
				if(!renderer->render_buffer_queue[0])
					still_playing = false;
				else{
					u8* p_data;
					u32 padding;
					u32 frames_available;

					//  We want to find out how much of the buffer *isn't* available (is padding).
					hr = renderer->audio_client->GetCurrentPadding(&padding);
					ASSERTHR(hr);

					// Calculate the number of frames available.  We'll render whichever is smaller between
					// the number of frames available and the number of frames left in the buffer
					frames_available = renderer->buffer_size - padding;

					// If the buffer at the head of the render buffer queue fits in the frames available, 
					// render it.  If we don't have enough room to fit the buffer, 
					// skip this pass - we will have enough room on the next pass.
					while(renderer->render_buffer_queue[0] && 
						(renderer->render_buffer_queue[0]->size <= (frames_available*renderer->frame_size)))
					{
						// We know that the buffer at the head of the queue will fit, 
						// so remove it and write it into 
						// the engine buffer.  Continue doing this until we no longer can fit
						// the recent buffer into the engine buffer.
						Buffer* render_buffer = LIST_POP_FRONT(renderer->render_buffer_queue);

						u32 frames_to_write = render_buffer->size / renderer->frame_size;
						hr = renderer->render_client->GetBuffer(frames_to_write, &p_data);
						ASSERTHR(hr);
						still_playing = !FAILED(hr);

						copy_mem(render_buffer->data, p_data, frames_to_write * renderer->frame_size);
						hr = renderer->render_client->ReleaseBuffer(frames_to_write, 0);
						ASSERTHR(hr);
						still_playing = !FAILED(hr);

						//TODO: free render_buffer

						hr = renderer->audio_client->GetCurrentPadding(&padding);
						frames_available = renderer->buffer_size - padding;

						ASSERTHR(hr);
						still_playing = !FAILED(hr);
						
						if(!((Buffer*)(render_buffer+1)))
							break;
					}

				}
			}break;
			default:
				ASSERT(false);
			break;
		}
	}
	#if USE_MMCSS
		AvRevertMmThreadCharacteristics(mmcss_handle);
	#endif 

	timeEndPeriod(1);

	CoUninitialize();
	return 0;
}
#endif