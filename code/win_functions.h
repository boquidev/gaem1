

internal File_data 
win_read_file(String filename, Memory_arena* arena)
{
	File_data result = {0};

	HANDLE file_handle = CreateFileA(filename.text, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
	if(file_handle == INVALID_HANDLE_VALUE)
	{
		DWORD error = GetLastError();
		ASSERT(false);
	}
	LARGE_INTEGER file_size;
	if( GetFileSizeEx(file_handle, &file_size) )
	{
		u32 file_size_32 = (u32)file_size.QuadPart;
		result.data = arena_push_size(arena, file_size_32);
		if(result.data)
		{
			DWORD bytes_read;
			if(ReadFile(file_handle, result.data, file_size_32, &bytes_read, 0))
				if((file_size_32 == bytes_read))
				result.size = file_size_32;

			
			else
				arena_pop_back_size(arena, file_size_32);
		}
	}
	ASSERT(result.size);
	CloseHandle(file_handle);

	return result;
}

internal bool
win_write_file(String filename, void* data, u32 file_size)
{
	b32 result = false;

	HANDLE file_handle = CreateFileA(filename.text, GENERIC_WRITE, FILE_SHARE_WRITE, 0, CREATE_ALWAYS, 0,0);
	if(file_handle != INVALID_HANDLE_VALUE)
	{
		DWORD bytes_written;
		if(WriteFile(file_handle, data, file_size, &bytes_written, 0))
		{
			result = (bytes_written == file_size);
		}else{
			DWORD error = GetLastError();
			ASSERT(false);
		}

		CloseHandle(file_handle);
	}
	else
	{
		DWORD error = GetLastError();
		ASSERT(false);
	}

	return result;
}

internal Int2
win_get_client_sizes(HWND window)
{
	Int2 result = {0};
	RECT rect;
	GetClientRect(window, &rect);
	result.x = rect.right - rect.left;
	result.y = rect.bottom - rect.top;
	
	return result;
}
