#pragma once


#include <windows.h>
#include <stdio.h>
#include <functional>
#include <vector>
#include <thread>
#include <mutex>

class FileNotifier
{
public:
   FileNotifier(const char* path);
   ~FileNotifier();

   void add_notification(const char* file_name, std::function<void (void)> f, int discriminant);

   void pool();
private:
   void thread_func();

   HANDLE dir_to_watch;
   struct Entry
   {
      std::string file_name;
      std::function<void(void)> call_back;
      int discriminant;
   };
   struct Request
   {
      int discriminant;
      std::function<void(void)> call_back;
   };

   void add_request(Entry const& entry);
   void remove_request(Entry const& entry);

   std::vector<Entry> entry;
   std::vector<Request> pending_request;
   std::thread request_thread;
   std::mutex mutex;
   int count = 0;
};