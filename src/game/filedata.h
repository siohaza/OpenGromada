#ifndef GAME_FILEDATA_H
#define GAME_FILEDATA_H

void FileData_Save(const char* p_path, const char* p_value);
const char* FileData_Load(const char* p_path, const char* p_default);
int FileData_FileExists(const char* p_filename);
const char* FileData_SaveFolder();

#endif
