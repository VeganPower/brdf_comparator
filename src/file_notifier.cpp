#include "file_notifier.hpp"

#include <string>
#include <algorithm>

void print_error(DWORD error)
{
   char lpBuffer[2048] = { 0 };
   FormatMessage(
      FORMAT_MESSAGE_FROM_SYSTEM,
      (void*)&error,
      error,
      NULL,
      lpBuffer,
      2048,
      nullptr
   );
   printf(lpBuffer);
}

FileNotifier::FileNotifier(char const* dir_path)
{
   dir_to_watch = CreateFile(dir_path, FILE_LIST_DIRECTORY, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
   if (dir_to_watch == INVALID_HANDLE_VALUE)
   {
      print_error(GetLastError());
   }
   request_thread = std::thread([this] { this->thread_func(); });
}

FileNotifier::~FileNotifier()
{
   CancelIoEx(dir_to_watch, nullptr);
   request_thread.join();
   CloseHandle(dir_to_watch);
}

void FileNotifier::add_request(Entry const& entry)
{
   bool add_request = entry.discriminant == 0;
   if (entry.discriminant != 0)
   {
      add_request = std::find_if(pending_request.begin(), pending_request.end(), [&](Request const & r)
      {
         return r.discriminant == entry.discriminant;
      }) == pending_request.end();
   }
   if (add_request)
   {
      pending_request.push_back(Request{entry.discriminant, entry.call_back});
   }
}

void FileNotifier::remove_request(Entry const& entry)
{
   auto request = pending_request.begin();
   do
   {
      request = std::find_if(pending_request.begin(), pending_request.end(), [&](Request const & r)
      {
         return r.discriminant == entry.discriminant;
      });
      if (request != pending_request.end())
      {
         pending_request.erase(request);
      }
   }
   while(request != pending_request.end());
}

void FileNotifier::thread_func()
{
   DWORD byte_returned = 0;

   DWORD notify_buffer[1024];
   const DWORD fild_flags = FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_CREATION | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_ATTRIBUTES | FILE_NOTIFY_CHANGE_SIZE | FILE_NOTIFY_CHANGE_CREATION;
   while (true)
   {
      if (ReadDirectoryChangesW(dir_to_watch, (void*)&notify_buffer, sizeof(notify_buffer), FALSE, fild_flags, &byte_returned, nullptr, nullptr) == 0)
      {
         DWORD error_code = GetLastError();
         if (error_code != ERROR_OPERATION_ABORTED)
         {
            print_error(error_code);
         }
         break;
      }
      count = 15;
      FILE_NOTIFY_INFORMATION* notify = (FILE_NOTIFY_INFORMATION*)notify_buffer;
      while (true)
      {

         std::lock_guard<std::mutex> guard(mutex);
         std::wstring ws(notify->FileName, notify->FileNameLength / 2);
         std::string file_name(ws.begin(), ws.end());
         for (auto e : entry)
         {
            if (e.file_name == file_name)
            {
               if (notify->Action == FILE_ACTION_REMOVED)
               {
				   remove_request(e);
               } else
               {
				   add_request(e);
               }
            }
         }
         size_t next_offset = notify->NextEntryOffset / sizeof(DWORD);
         if (next_offset == 0)
         {
            break;
         }
         notify = (FILE_NOTIFY_INFORMATION*)(notify_buffer + next_offset);
      }
   }
}

void FileNotifier::add_notification(const char* file_name, std::function<void(void)> f, int discriminant)
{
   std::lock_guard<std::mutex> guard(mutex);
   entry.push_back(Entry{file_name, f, discriminant });
}


void FileNotifier::pool()
{
   std::lock_guard<std::mutex> guard(mutex);
   if (count == 0)
   {
      for (auto p : pending_request)
      {
         p.call_back();
      }
      pending_request.clear();
   }else
   {
      count--;
   }
}