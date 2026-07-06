#pragma once
#include <filesystem>
#include <optional>
#include "../domain/model.h"
#include "../application/snippet_service.h"
namespace snippet {
struct LoadResult { std::optional<AppSettings> settings; std::wstring error; [[nodiscard]] bool ok() const noexcept{return settings.has_value()&&error.empty();} };
class XmlSettingsRepository final:public ISettingsStore { public: explicit XmlSettingsRepository(std::filesystem::path path):path_(std::move(path)){} [[nodiscard]] LoadResult Load() const; [[nodiscard]] std::wstring Save(const AppSettings&) override; private: std::filesystem::path path_; };
}
