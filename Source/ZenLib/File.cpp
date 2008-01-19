// ZenLib::File - File functions
// Copyright (C) 2007-2008 Jerome Martinez, Zen@MediaArea.net
//
// This software is provided 'as-is', without any express or implied
// warranty.  In no event will the authors be held liable for any damages
// arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
//
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//---------------------------------------------------------------------------
#include "ZenLib/Conf_Internal.h"
#ifdef __BORLANDC__
    #pragma hdrstop
#endif
//---------------------------------------------------------------------------
//#undef ZENLIB_STANDARD
//---------------------------------------------------------------------------
#ifdef ZENLIB_USEWX
    #include <wx/file.h>
    #include <wx/filename.h>
    #include <wx/utils.h>
#else //ZENLIB_USEWX
    #ifdef ZENLIB_STANDARD
        /*
        #ifdef WINDOWS
            #include <io.h>
        #else
            #include <stdio.h>
        #endif
        #include <fcntl.h>
        #include <sys/stat.h>
        #ifndef O_BINARY
            #define O_BINARY 0
        #endif
        */
        #include <sys/stat.h>
        #include <fstream>
        using namespace std;
    #elif defined WINDOWS
        #undef __TEXT
        #include <windows.h>
    #endif
#endif //ZENLIB_USEWX
#include "ZenLib/File.h"
#include <ZenLib/OS_Utils.h>
#include <map>
//---------------------------------------------------------------------------

namespace ZenLib
{

//***************************************************************************
// Pointers
//***************************************************************************

#ifdef ZENLIB_USEWX
    static std::map<void*, void*> File_Handle_Static;
#else //ZENLIB_USEWX
    #ifdef ZENLIB_STANDARD
        //static std::map<void*, int> File_Handle_Static;
        static std::map<void*, fstream*> File_Handle_Static;
    #elif defined WINDOWS
        static std::map<void*, void*> File_Handle_Static;
    #endif
#endif //ZENLIB_USEWX

//***************************************************************************
// Constructor/Destructor
//***************************************************************************

//---------------------------------------------------------------------------
File::File()
{
    #ifdef ZENLIB_USEWX
        File_Handle_Static[this]=NULL;
    #else //ZENLIB_USEWX
        #ifdef ZENLIB_STANDARD
            //File_Handle_Static[this]=-1;
            File_Handle_Static[this]=NULL;
        #elif defined WINDOWS
            File_Handle_Static[this]=NULL;
        #endif
    #endif //ZENLIB_USEWX
}

File::File(Ztring File_Name, access_t Access)
{
    if (!Open(File_Name, Access))
        #ifdef ZENLIB_USEWX
            File_Handle_Static[this]=NULL;
        #else //ZENLIB_USEWX
            #ifdef ZENLIB_STANDARD
                //File_Handle_Static[this]=-1;
                File_Handle_Static[this]=NULL;
            #elif defined WINDOWS
                File_Handle_Static[this]=NULL;
            #endif
        #endif //ZENLIB_USEWX
}

//---------------------------------------------------------------------------
File::~File()
{
    Close();
}

//***************************************************************************
// Open/Close
//***************************************************************************

//---------------------------------------------------------------------------
bool File::Open (Ztring File_Name, access_t Access)
{
    #ifdef ZENLIB_USEWX
        File_Handle_Static[this]=(void*)new wxFile();
        if (((wxFile*)File_Handle_Static[this])->Open(File_Name.c_str(), (wxFile::OpenMode)Access)==0)
        {
            //Sometime the file is locked for few milliseconds, we try again later
            wxMilliSleep(1000);
            if (((wxFile*)File_Handle_Static[this])->Open(File_Name.c_str(), (wxFile::OpenMode)Access)==0)
                //File is not openable
                return false;
        }
        return true;
    #else //ZENLIB_USEWX
        #ifdef ZENLIB_STANDARD
            /*
            int access;
            switch (Access)
            {
                case Access_Read         : access=O_BINARY|O_RDONLY          ; break;
                case Access_Write        : access=O_BINARY|O_WRONLY|O_CREAT|O_TRUNC  ; break;
                case Access_Read_Write   : access=O_BINARY|O_RDWR  |O_CREAT  ; break;
                case Access_Write_Append : access=O_BINARY|O_WRONLY|O_CREAT|O_APPEND ; break;
                default                  : access=0                          ; break;
            }
            #ifdef UNICODE
                File_Handle_Static[this]=open(File_Name.To_Local().c_str(), access);
            #else
                File_Handle_Static[this]=open(File_Name.c_str(), access);
            #endif //UNICODE
            return File_Handle_Static[this]!=-1;
            */
            ios_base::openmode mode;
            switch (Access)
            {
                case Access_Read         : mode=ios_base::binary|ios_base::in          ; break;
                case Access_Write        : mode=ios_base::binary|/*ios_base::in|*/ios_base::out; break;
                case Access_Write_Append : mode=ios_base::binary|/*ios_base::in|*/ios_base::out|ios_base::app ; break; //Fail with Linux
                default                  :                           ; break;
            }
            #ifdef UNICODE
                File_Handle_Static[this]=new fstream(File_Name.To_Local().c_str(), mode);
            #else
                File_Handle_Static[this]=new fstream(File_Name.c_str(), mode);
            #endif //UNICODE
            if (!((fstream*)File_Handle_Static[this])->is_open())
            {
                delete (fstream*)File_Handle_Static[this]; File_Handle_Static[this]=NULL;
                return false;
            }
            return true;
        #elif defined WINDOWS
            DWORD dwDesiredAccess, dwShareMode, dwCreationDisposition;
            switch (Access)
            {
                case Access_Read         : dwDesiredAccess=GENERIC_READ;  dwShareMode=FILE_SHARE_READ;  dwCreationDisposition=OPEN_EXISTING; break;
                case Access_Write        : dwDesiredAccess=GENERIC_WRITE; dwShareMode=0;                dwCreationDisposition=OPEN_ALWAYS;   break;
                case Access_Write_Append : dwDesiredAccess=GENERIC_WRITE; dwShareMode=0;                dwCreationDisposition=OPEN_ALWAYS;   break;
                default                  : dwDesiredAccess=0;             dwShareMode=0;                dwCreationDisposition=0;             break;
            }

            #ifdef UNICODE
                if (IsWin9X())
                    File_Handle_Static[this]=CreateFileA(File_Name.To_Local().c_str(), dwDesiredAccess, dwShareMode, NULL, dwCreationDisposition, 0, NULL);
                else
                    File_Handle_Static[this]=CreateFileW(File_Name.c_str(), dwDesiredAccess, dwShareMode, NULL, dwCreationDisposition, 0, NULL);
            #else
                File_Handle_Static[this]=CreateFile(File_Name.c_str(), dwDesiredAccess, dwShareMode, NULL, dwCreationDisposition, 0, NULL);
            #endif //UNICODE
            if (File_Handle_Static[this]==INVALID_HANDLE_VALUE)
            {
                /*
                //Sometime the file is locked for few milliseconds, we try again later
                char lpMsgBuf[1000];
                DWORD dw = GetLastError();
                FormatMessageA(
                FORMAT_MESSAGE_FROM_SYSTEM,
                NULL,
                dw,
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                lpMsgBuf,
                1000, NULL );
                */
                Sleep(1000);
                #ifdef UNICODE
                    if (IsWin9X())
                        File_Handle_Static[this]=CreateFileA(File_Name.To_Local().c_str(), dwDesiredAccess, dwShareMode, NULL, dwCreationDisposition, 0, NULL);
                    else
                        File_Handle_Static[this]=CreateFileW(File_Name.c_str(), dwDesiredAccess, dwShareMode, NULL, dwCreationDisposition, 0, NULL);
                #else
                    File_Handle_Static[this]=CreateFile(File_Name.c_str(), dwDesiredAccess, dwShareMode, NULL, dwCreationDisposition, 0, NULL);
                #endif //UNICODE
            }
            if (File_Handle_Static[this]==INVALID_HANDLE_VALUE)
                //File is not openable
                return false;

            //Append
            if (Access==Access_Write_Append)
                GoTo(0, FromEnd);

            return true;
        #endif
    #endif //ZENLIB_USEWX
}

//---------------------------------------------------------------------------
bool File::Create (Ztring File_Name, bool OverWrite)
{
    #ifdef ZENLIB_USEWX
        File_Handle_Static[this]=(void*)new wxFile();
        if (((wxFile*)File_Handle_Static[this])->Create(File_Name.c_str(), OverWrite)==0)
        {
            //Sometime the file is locked for few milliseconds, we try again later
            wxMilliSleep(3000);
            if (((wxFile*)File_Handle_Static[this])->Create(File_Name.c_str(), OverWrite)==0)
                //File is not openable
                return false;
        }
        return true;
    #else //ZENLIB_USEWX
        #ifdef ZENLIB_STANDARD
            /*
            int access;
            switch (OverWrite)
            {
                case false        : access=O_BINARY|O_CREAT|O_WRONLY|O_EXCL ; break;
                default           : access=O_BINARY|O_CREAT|O_WRONLY|O_TRUNC; break;
            }
            #ifdef UNICODE
                File_Handle_Static[this]=open(File_Name.To_Local().c_str(), access);
            #else
                File_Handle_Static[this]=open(File_Name.c_str(), access);
            #endif //UNICODE
            return  File_Handle_Static[this]!=-1;
            */
            /*ios_base::openmode mode;

            switch (OverWrite)
            {
                //case false         : mode=          ; break;
                default                  : mode=0                            ; break;
            }*/
            #ifdef UNICODE
                File_Handle_Static[this]=new fstream(File_Name.To_Local().c_str(), ios_base::binary|ios_base::in);
            #else
                File_Handle_Static[this]=new fstream(File_Name.c_str(), ios_base::binary|ios_base::in);
            #endif //UNICODE
            return ((fstream*)File_Handle_Static[this])->is_open();
        #elif defined WINDOWS
            DWORD dwDesiredAccess, dwShareMode, dwCreationDisposition;
            switch (OverWrite)
            {
                case false        : dwDesiredAccess=GENERIC_WRITE; dwShareMode=0;                dwCreationDisposition=CREATE_NEW;    break;
                default           : dwDesiredAccess=GENERIC_WRITE; dwShareMode=0;                dwCreationDisposition=CREATE_ALWAYS; break;
            }

            #ifdef UNICODE
                if (IsWin9X())
                    File_Handle_Static[this]=CreateFileA(File_Name.To_Local().c_str(), dwDesiredAccess, dwShareMode, NULL, dwCreationDisposition, 0, NULL);
                else
                    File_Handle_Static[this]=CreateFileW(File_Name.c_str(), dwDesiredAccess, dwShareMode, NULL, dwCreationDisposition, 0, NULL);
            #else
                File_Handle_Static[this]=CreateFile(File_Name.c_str(), dwDesiredAccess, dwShareMode, NULL, dwCreationDisposition, 0, NULL);
            #endif //UNICODE
            if (File_Handle_Static[this]==INVALID_HANDLE_VALUE)
            {
                //Sometime the file is locked for few milliseconds, we try again later
                Sleep(3000);
                #ifdef UNICODE
                    if (IsWin9X())
                        File_Handle_Static[this]=CreateFileA(File_Name.To_Local().c_str(), dwDesiredAccess, dwShareMode, NULL, dwCreationDisposition, 0, NULL);
                    else
                        File_Handle_Static[this]=CreateFileW(File_Name.c_str(), dwDesiredAccess, dwShareMode, NULL, dwCreationDisposition, 0, NULL);
                #else
                    File_Handle_Static[this]=CreateFile(File_Name.c_str(), dwDesiredAccess, dwShareMode, NULL, dwCreationDisposition, 0, NULL);
                #endif //UNICODE
            }
            if (File_Handle_Static[this]==INVALID_HANDLE_VALUE)
                //File is not openable
                return false;
            return true;
        #endif
    #endif //ZENLIB_USEWX
}

//---------------------------------------------------------------------------
void File::Close ()
{
    #ifdef ZENLIB_USEWX
        delete (wxFile*)File_Handle_Static[this]; File_Handle_Static[this]=NULL;
    #else //ZENLIB_USEWX
        #ifdef ZENLIB_STANDARD
            //close(File_Handle_Static[this]); File_Handle_Static[this]=-1;
            delete (fstream*)File_Handle_Static[this]; File_Handle_Static[this]=NULL;
        #elif defined WINDOWS
            CloseHandle(File_Handle_Static[this]); File_Handle_Static[this]=NULL;
        #endif
    #endif //ZENLIB_USEWX
    File_Handle_Static.erase(this);
}

//***************************************************************************
// Read/Write
//***************************************************************************

//---------------------------------------------------------------------------
size_t File::Read (int8u* Buffer, size_t Buffer_Size_Max)
{
    #ifdef ZENLIB_USEWX
        if (File_Handle_Static[this]==NULL)
    #else //ZENLIB_USEWX
        #ifdef ZENLIB_STANDARD
            //if (File_Handle_Static[this]==-1)
            if (File_Handle_Static[this]==NULL)
        #elif defined WINDOWS
            if (File_Handle_Static[this]==NULL)
        #endif
    #endif //ZENLIB_USEWX
        return 0;

    #ifdef ZENLIB_USEWX
        return ((wxFile*)File_Handle_Static[this])->Read(Buffer, Buffer_Size_Max);
    #else //ZENLIB_USEWX
        #ifdef ZENLIB_STANDARD
            //return read((int)File_Handle_Static[this], Buffer, Buffer_Size_Max);
            ((fstream*)File_Handle_Static[this])->read((char*)Buffer, Buffer_Size_Max);
            return ((fstream*)File_Handle_Static[this])->gcount();
        #elif defined WINDOWS
            DWORD Buffer_Size;
            if (ReadFile(File_Handle_Static[this], Buffer, (DWORD)Buffer_Size_Max, &Buffer_Size, NULL))
                return Buffer_Size;
            else
                return 0;
        #endif
    #endif //ZENLIB_USEWX
}

//---------------------------------------------------------------------------
size_t File::Write (const int8u* Buffer, size_t Buffer_Size)
{
    #ifdef ZENLIB_USEWX
        if (File_Handle_Static[this]==NULL)
    #else //ZENLIB_USEWX
        #ifdef ZENLIB_STANDARD
            //if (File_Handle_Static[this]==-1)
            if (File_Handle_Static[this]==NULL)
        #elif defined WINDOWS
            if (File_Handle_Static[this]==NULL)
        #endif
    #endif //ZENLIB_USEWX
        return 0;

    #ifdef ZENLIB_USEWX
        return ((wxFile*)File_Handle_Static[this])->Write(Buffer, Buffer_Size);
    #else //ZENLIB_USEWX
        #ifdef ZENLIB_STANDARD
            //return write(File_Handle_Static[this], Buffer, Buffer_Size);
            ((fstream*)File_Handle_Static[this])->write((char*)Buffer, Buffer_Size);
            return ((fstream*)File_Handle_Static[this])->bad()?0:Buffer_Size;
        #elif defined WINDOWS
            DWORD Buffer_Size_Written;
            if (WriteFile(File_Handle_Static[this], Buffer, (DWORD)Buffer_Size, &Buffer_Size_Written, NULL))
                return Buffer_Size_Written;
            else
                return 0;
        #endif
    #endif //ZENLIB_USEWX
}

//---------------------------------------------------------------------------
size_t File::Write (Ztring ToWrite)
{
    std::string AnsiString=ToWrite.To_UTF8();
    return Write((const int8u*)AnsiString.c_str(), AnsiString.size());
}

//***************************************************************************
// Moving
//***************************************************************************

//---------------------------------------------------------------------------
bool File::GoTo (int64s Position, move_t MoveMethod)
{
    #ifdef ZENLIB_USEWX
        return ((wxFile*)File_Handle_Static[this])->Seek(Position, (wxSeekMode)MoveMethod)!=wxInvalidOffset; //move_t and wxSeekMode are same
    #else //ZENLIB_USEWX
        #ifdef ZENLIB_STANDARD
            /*
            int fromwhere;
            switch (MoveMethod)
            {
                case FromBegin : fromwhere=SEEK_SET; break;
                case FromCurrent : fromwhere=SEEK_CUR; break;
                case FromEnd : fromwhere=SEEK_END; break;
                default : fromwhere=SEEK_CUR; break;
            }
            return lseek(File_Handle_Static[this], Position, fromwhere)!=-1;
            */
            ios_base::seekdir dir;
            switch (MoveMethod)
            {
                case FromBegin   : dir=ios_base::beg; break;
                case FromCurrent : dir=ios_base::cur; break;
                case FromEnd     : dir=ios_base::end; break;
                default          : dir=ios_base::beg;
            }
            ((fstream*)File_Handle_Static[this])->seekg(Position, dir);
            return !((fstream*)File_Handle_Static[this])->fail();
        #elif defined WINDOWS
            LARGE_INTEGER GoTo; GoTo.QuadPart=Position;
            //return SetFilePointerEx(File_Handle_Static[this], GoTo, NULL, MoveMethod)!=0; //Not on win9X
            GoTo.LowPart=SetFilePointer(File_Handle_Static[this], GoTo.LowPart, &GoTo.HighPart, MoveMethod);
            if (GoTo.LowPart==INVALID_SET_FILE_POINTER && GetLastError()!=NO_ERROR)
                return false;
            else
                return true;
        #endif
    #endif //ZENLIB_USEWX
}

//---------------------------------------------------------------------------
int64u File::Position_Get ()
{
    #ifdef ZENLIB_USEWX
        return (int64u)-1;
    #else //ZENLIB_USEWX
        #ifdef ZENLIB_STANDARD
            return ((fstream*)File_Handle_Static[this])->tellg();
        #elif defined WINDOWS
            return (int64u)-1;
        #endif
    #endif //ZENLIB_USEWX
}

//***************************************************************************
// Attributes
//***************************************************************************

//---------------------------------------------------------------------------
int64u File::Size_Get()
{
    #ifdef ZENLIB_USEWX
        if (File_Handle_Static[this]==NULL)
    #else //ZENLIB_USEWX
        #ifdef ZENLIB_STANDARD
            //if (File_Handle_Static[this]==-1)
            if (File_Handle_Static[this]==NULL)
        #elif defined WINDOWS
            if (File_Handle_Static[this]==NULL)
        #endif
    #endif //ZENLIB_USEWX
        return 0;

    #ifdef ZENLIB_USEWX
        return ((wxFile*)File_Handle_Static[this])->Length();
    #else //ZENLIB_USEWX
        #ifdef ZENLIB_STANDARD
            /*
            int CurrentPos=lseek(File_Handle_Static[this], 0, SEEK_CUR);
            int64u File_Size;
            File_Size=lseek(File_Handle_Static[this], 0, SEEK_END);
            lseek(File_Handle_Static[this], CurrentPos, SEEK_SET);
            */
            streampos CurrentPos=((fstream*)File_Handle_Static[this])->tellg();
            ((fstream*)File_Handle_Static[this])->seekg(0, ios_base::end);
            int64u File_Size;
            File_Size=((fstream*)File_Handle_Static[this])->tellg();
            ((fstream*)File_Handle_Static[this])->seekg(CurrentPos, ios_base::beg);
            return File_Size;
        #elif defined WINDOWS
            int64u File_Size;
            DWORD High;DWORD Low=GetFileSize(File_Handle_Static[this], &High);
            File_Size=0x100000000ULL*High+Low;
            return File_Size;
        #endif
    #endif //ZENLIB_USEWX
}

//---------------------------------------------------------------------------
bool File::Opened_Get()
{
    #ifdef ZENLIB_USEWX
        return File_Handle_Static[this]!=NULL;
    #else //ZENLIB_USEWX
        #ifdef ZENLIB_STANDARD
            //return File_Handle_Static[this]!=-1;
            return File_Handle_Static[this]!=NULL && ((fstream*)File_Handle_Static[this])->is_open();
        #elif defined WINDOWS
            return File_Handle_Static[this]!=NULL;
        #endif
    #endif //ZENLIB_USEWX
}

//***************************************************************************
// Helpers
//***************************************************************************

//---------------------------------------------------------------------------
int64u File::Size_Get(const Ztring &File_Name)
{
    File F(File_Name);
    return F.Size_Get();
}

//---------------------------------------------------------------------------
bool File::Exists(const Ztring &File_Name)
{
    if (File_Name.find(_T('*'))!=std::string::npos || File_Name.find(_T('?'))!=std::string::npos)
        return false;

    #ifdef ZENLIB_USEWX
        wxFileName FN(File_Name.c_str());
        return FN.FileExists();
    #else //ZENLIB_USEWX
        #ifdef ZENLIB_STANDARD
            struct stat buffer;
            int         status;
            #ifdef UNICODE
                status=stat(File_Name.To_Local().c_str(), &buffer);
            #else
                status=stat(File_Name.c_str(), &buffer);
            #endif //UNICODE
            return status==0 && S_ISREG(buffer.st_mode);
        #elif defined WINDOWS
            #ifdef UNICODE
                DWORD FileAttributes;
                if (IsWin9X())
                    FileAttributes=GetFileAttributesA(File_Name.To_Local().c_str());
                else
                    FileAttributes=GetFileAttributesW(File_Name.c_str());
            #else
                DWORD FileAttributes=GetFileAttributes(File_Name.c_str());
            #endif //UNICODE
            return ((FileAttributes!=INVALID_FILE_ATTRIBUTES) && !(FileAttributes&FILE_ATTRIBUTE_DIRECTORY));
        #endif
    #endif //ZENLIB_USEWX
}

//---------------------------------------------------------------------------
bool File::Copy(const Ztring &Source, const Ztring &Destination, bool OverWrite)
{
    #ifdef ZENLIB_USEWX
        return wxCopyFile(Source.c_str(), Destination.c_str(), OverWrite);
    #else //ZENLIB_USEWX
        #ifdef ZENLIB_STANDARD
            return false;
        #elif defined WINDOWS
            #ifdef UNICODE
                if (IsWin9X())
                    return CopyFileA(Source.To_Local().c_str(), Destination.To_Local().c_str(), !OverWrite)!=0;
                else
                    return CopyFileW(Source.c_str(), Destination.c_str(), !OverWrite)!=0;
            #else
                return CopyFile(Source.c_str(), Destination.c_str(), !OverWrite)!=0;
            #endif //UNICODE
        #endif
    #endif //ZENLIB_USEWX
}

//---------------------------------------------------------------------------
bool File::Move(const Ztring &Source, const Ztring &Destination, bool OverWrite)
{
    #ifdef ZENLIB_USEWX
        if (OverWrite && Exists(Destination))
            wxRemoveFile(Destination.c_str());
        return wxRenameFile(Source.c_str(), Destination.c_str());
    #else //ZENLIB_USEWX
        #ifdef ZENLIB_STANDARD
            return false;
        #elif defined WINDOWS
            #ifdef UNICODE
                if (IsWin9X())
                    return CopyFileA(Source.To_Local().c_str(), Destination.To_Local().c_str(), !OverWrite)!=0;
                else
                    return CopyFileW(Source.c_str(), Destination.c_str(), !OverWrite)!=0;
            #else
                return CopyFile(Source.c_str(), Destination.c_str(), !OverWrite)!=0;
            #endif //UNICODE
        #endif
    #endif //ZENLIB_USEWX
}

//---------------------------------------------------------------------------
bool File::Delete(const Ztring &File_Name)
{
    #ifdef ZENLIB_USEWX
        return wxRemoveFile(File_Name.c_str());
    #else //ZENLIB_USEWX
        #ifdef ZENLIB_STANDARD
            return false;
        #elif defined WINDOWS
            #ifdef UNICODE
                if (IsWin9X())
                    return DeleteFileA(File_Name.To_Local().c_str())!=0;
                else
                    return DeleteFileW(File_Name.c_str())!=0;
            #else
                return DeleteFile(File_Name.c_str())!=0;
            #endif //UNICODE
        #endif
    #endif //ZENLIB_USEWX
}

//***************************************************************************
//
//***************************************************************************

} //namespace
