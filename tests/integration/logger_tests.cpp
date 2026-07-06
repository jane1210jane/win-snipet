#include <filesystem>
#include <fstream>
#include "../../src/infrastructure/file_logger.h"
#include "../test_runner.h"
using namespace snippet;
TEST(NFR03_logger_records_diagnostics_and_rotates_without_content_parameter) {
 auto dir=std::filesystem::temp_directory_path()/L"SnippetToolTests-logger";std::filesystem::remove_all(dir);
 FileLogger logger(dir/L"app.log",128,3);for(int i=0;i<20;++i)logger.Write(LogLevel::Error,L"CFG-LOAD-001",L"XML_PARSE",13);
 REQUIRE(std::filesystem::exists(dir/L"app.log"));REQUIRE(std::filesystem::exists(dir/L"app.log.1"));
 REQUIRE(std::filesystem::directory_iterator(dir)!=std::filesystem::directory_iterator{});
 std::ifstream file(dir/L"app.log");std::string content((std::istreambuf_iterator<char>(file)),{});REQUIRE(content.find("CFG-LOAD-001")!=std::string::npos);
}
