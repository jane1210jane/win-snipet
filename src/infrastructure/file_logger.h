#pragma once
#include <filesystem>
#include <string>
namespace snippet {
enum class LogLevel {Info,Warning,Error};
class FileLogger {
public:explicit FileLogger(std::filesystem::path path,std::uintmax_t maxBytes=1024*1024,unsigned generations=3):path_(std::move(path)),maxBytes_(maxBytes),generations_(generations){}
 void Write(LogLevel,const std::wstring& eventId,const std::wstring& diagnostic,unsigned long win32Error=0) const noexcept;
private:void Rotate() const;std::filesystem::path path_;std::uintmax_t maxBytes_;unsigned generations_;
};
}
