#include "file_logger.h"
#include <Windows.h>
#include <fstream>
#include <sstream>
namespace snippet {namespace {
std::string Utf8(const std::wstring& value){if(value.empty())return {};const int size=WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,value.data(),static_cast<int>(value.size()),nullptr,0,nullptr,nullptr);std::string out(static_cast<size_t>(size),'\0');if(size>0)WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,value.data(),static_cast<int>(value.size()),out.data(),size,nullptr,nullptr);return out;}
}
void FileLogger::Rotate() const {if(!std::filesystem::exists(path_)||std::filesystem::file_size(path_)<maxBytes_)return;for(unsigned i=generations_;i>1;--i){auto from=path_;from+=L"."+std::to_wstring(i-1);auto to=path_;to+=L"."+std::to_wstring(i);std::error_code ec;std::filesystem::remove(to,ec);if(std::filesystem::exists(from))std::filesystem::rename(from,to,ec);}auto first=path_;first+=L".1";std::error_code ec;std::filesystem::remove(first,ec);std::filesystem::rename(path_,first,ec);}
void FileLogger::Write(LogLevel level,const std::wstring& eventId,const std::wstring& diagnostic,unsigned long error) const noexcept {try{std::filesystem::create_directories(path_.parent_path());Rotate();SYSTEMTIME now{};GetLocalTime(&now);const char* label=level==LogLevel::Error?"ERROR":level==LogLevel::Warning?"WARN":"INFO";std::ofstream file(path_,std::ios::app|std::ios::binary);file<<now.wYear<<'-'<<now.wMonth<<'-'<<now.wDay<<'T'<<now.wHour<<':'<<now.wMinute<<':'<<now.wSecond<<' '<<label<<' '<<Utf8(eventId)<<' '<<Utf8(diagnostic)<<' '<<error<<'\n';}catch(...){}}
}
