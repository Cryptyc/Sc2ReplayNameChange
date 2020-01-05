#include <Windows.h>
#include <string>
#include <istream>
#include <fstream>
#include <iostream>
#include <ostream>

#include "StormLib.h"

#include <fstream>


static const std::string FileName = "replay.InitData";
//static const std::string ReplayFile = "Replay.SC2Replay";

//static const std::string OrigPlayer1 = "foo5679";
//static const std::string OrigPlayer2 = "foo5680";

//static const std::string Player1 = "LongPlayerName";
//static const std::string Player2 = "OddPlayerName";

std::ifstream::pos_type filesize(const char* filename)
{
	std::ifstream in(filename, std::ifstream::ate | std::ifstream::binary);
	return in.tellg();
}

static int RemoveFileFromArchive(const char *ArchiveName, const char *ArchivedFile)
{
	HANDLE hMpq = NULL;          // Open archive handle
	int    nError = ERROR_SUCCESS; // Result value
	if (nError == ERROR_SUCCESS)
	{
		if (!SFileOpenArchive(ArchiveName, 0, 0, &hMpq))
		{
			nError = GetLastError();
		}
	}

	if (nError == ERROR_SUCCESS)
	{
		if (!SFileRemoveFile(hMpq, ArchivedFile, 0))
		{
			nError = GetLastError();
		}
	}
	if (hMpq != NULL)
		SFileCloseArchive(hMpq);
	return nError;
}

//-----------------------------------------------------------------------------
// Extracts an archived file and saves it to the disk.
//
// Parameters :
//
//   char * szArchiveName  - Archive file name
//   char * szArchivedFile - Name/number of archived file.
//   char * szFileName     - Name of the target disk file.

static int ExtractFileFromArchive(const char * szArchiveName, const char * szArchivedFile, const char * szFileName)
{
	HANDLE hMpq = NULL;          // Open archive handle
	HANDLE hFile = NULL;          // Archived file handle
	HANDLE handle = NULL;          // Disk file handle
	int    nError = ERROR_SUCCESS; // Result value

								   // Open an archive, e.g. "d2music.mpq"
	if (nError == ERROR_SUCCESS)
	{
		if (!SFileOpenArchive(szArchiveName, 0, 0, &hMpq))
			nError = GetLastError();
	}

	// Open a file in the archive, e.g. "data\global\music\Act1\tristram.wav"
	if (nError == ERROR_SUCCESS)
	{
		if (!SFileOpenFileEx(hMpq, szArchivedFile, 0, &hFile))
			nError = GetLastError();
	}

	// Create the target file
	if (nError == ERROR_SUCCESS)
	{
		handle = CreateFile(szFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
		if (handle == INVALID_HANDLE_VALUE)
			nError = GetLastError();
	}

	// Read the file from the archive
	if (nError == ERROR_SUCCESS)
	{
		char  szBuffer[0x10000];
		DWORD dwBytes = 1;

		while (dwBytes > 0)
		{
			SFileReadFile(hFile, szBuffer, sizeof(szBuffer), &dwBytes, NULL);
			if (dwBytes > 0)
				WriteFile(handle, szBuffer, dwBytes, &dwBytes, NULL);
		}
	}

	// Cleanup and exit
	if (handle != NULL)
		CloseHandle(handle);
	if (hFile != NULL)
		SFileCloseFile(hFile);
	if (hMpq != NULL)
		SFileCloseArchive(hMpq);

	return nError;
}

static int SaveFileInArchive(const std::string &ArchiveName, const std::string &FileName, const std::string &InputFileName)
{
	HANDLE hMpq = NULL;          // Open archive handle
	HANDLE hFile = NULL;          // Archived file handle
	int    nError = ERROR_SUCCESS; // Result value

								   // Open an archive, e.g. "d2music.mpq"
	std::ifstream file(InputFileName, std::ios::binary);
	if (!file)
	{
		return ERROR_FILE_NOT_FOUND;
	}
	file.seekg(0, std::ios::end);
	size_t file_size = file.tellg();
	file.seekg(0, std::ios::beg);
	char *buffer = (char *)malloc(file_size);
	file.read(buffer, file_size);
	size_t chars_read = file.gcount();
	if (nError == ERROR_SUCCESS)
	{
		if (!SFileOpenArchive(ArchiveName.c_str(), 0, 0, &hMpq))
		{
			nError = GetLastError();
		}
	}
	if (nError == ERROR_SUCCESS)
	{
		if (!SFileCreateFile(hMpq, FileName.c_str(), 0, (DWORD)file_size, SFileGetLocale(), MPQ_FILE_COMPRESS, &hFile))
		{
			nError = GetLastError();
		}
	}
	if (nError == ERROR_SUCCESS)
	{
		if (!SFileWriteFile(hFile, buffer, (DWORD)file_size, MPQ_COMPRESSION_ZLIB))
		{
			nError = GetLastError();
		}
	}
	// Cleanup and exit
	if (hFile != NULL)
	{
		SFileCloseFile(hFile);
	}
	if (hMpq != NULL)
	{
		SFileCloseArchive(hMpq);
	}
	file.close();
	return nError;
}

int ReplaceString(const std::string &filename, const std::string &search_term1, const std::string &replace_term1, const std::string &search_term2, const std::string &replace_term2)
{
	std::ifstream file(filename, std::ios::binary);
	if (file)
	{
		file.seekg(0, std::ios::end);
		size_t file_size = file.tellg();
		file.seekg(0, std::ios::beg);
		std::string file_content;
		file_content.reserve(file_size);
		char *buffer = (char *)malloc(file_size);
		memset(buffer, NULL, file_size);

		std::streamsize chars_read;
		size_t search_term_size1 = search_term1.size();
		size_t search_term_size2 = search_term2.size();

		file.read(buffer, file_size);
		size_t new_size = file_size + replace_term1.size() - search_term1.size() + replace_term2.size() - search_term2.size();
		char *newbuffer = (char *)malloc(new_size);
		memset(newbuffer, NULL, new_size);
		int CurrentPos = 0;
		int OldBufferPos = 0;
		chars_read = file.gcount();
		file_content.append(buffer, chars_read);
		std::string::size_type offset = 0, found_at;
		found_at = file_content.find(search_term1, offset);
		if(found_at != std::string::npos)
		{
			std::cout << "Name: " << search_term1 << " Found at:" << found_at << std::endl;
			memcpy(newbuffer, buffer, found_at);
			CurrentPos = found_at;
			--CurrentPos;
			newbuffer[CurrentPos] = (char)replace_term1.size();
			++CurrentPos;
			memcpy(&newbuffer[found_at], replace_term1.c_str(), replace_term1.size());
			CurrentPos = CurrentPos + replace_term1.size();
			OldBufferPos = found_at + search_term1.size();

		}
		else
		{
			return -1;
		}
		offset = 0;
		found_at = file_content.find(search_term2, offset);
		if (found_at != std::string::npos)
		{
			std::cout << "Name: " << search_term2 << " Found at:" << found_at << std::endl;
			size_t newOffset = found_at - OldBufferPos;
			memcpy(&newbuffer[CurrentPos], &buffer[OldBufferPos], newOffset);
			CurrentPos += newOffset;
			char NewSize = (char)replace_term2.size();
			CurrentPos--;
			if (NewSize % 2 == 1)
			{
				newbuffer[CurrentPos] = 0x01;
				NewSize--;
			}
			else
			{
				newbuffer[CurrentPos] = 0x00; 
			}
			CurrentPos--;
			newbuffer[CurrentPos] = NewSize;
			CurrentPos = CurrentPos + 2;
			memcpy(&newbuffer[CurrentPos], replace_term2.c_str(), replace_term2.size());
			CurrentPos = CurrentPos + replace_term2.size();
			OldBufferPos = found_at + search_term2.size();
			size_t RemainingBytes = file_size - OldBufferPos;
			memcpy(&newbuffer[CurrentPos], &buffer[OldBufferPos], RemainingBytes);

		}
		else
		{
			return -1;
		}

		file.close();
		std::ofstream outfile(filename, std::ofstream::binary);
		if (outfile)
		{
			outfile.write(newbuffer, new_size);
		}
		free(buffer);
		free(newbuffer);
	}
	return 0;
}



int main(int argc, char **argv)
{
	const static std::string ReplayFile = argv[1];
	const static std::string OrigPlayer1 = argv[2];
	const static std::string Player1 = argv[3];
	const static std::string OrigPlayer2 = argv[4];
	const static std::string Player2 = argv[5];
	ExtractFileFromArchive(ReplayFile.c_str(), FileName.c_str(), FileName.c_str());
	ReplaceString(FileName, OrigPlayer1, Player1, OrigPlayer2, Player2);
	RemoveFileFromArchive(ReplayFile.c_str(), FileName.c_str());
	SaveFileInArchive(ReplayFile.c_str(), FileName.c_str(), FileName.c_str());
	DeleteFile(FileName.c_str());

}