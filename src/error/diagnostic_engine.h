#ifndef ALOHA_DIAGNOSTIC_ENGINE_H_
#define ALOHA_DIAGNOSTIC_ENGINE_H_

#include "diagnostic.h"
#include <functional>
#include <fstream>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace aloha
{

    class DiagnosticEngine
    {
    private:
        std::vector<Diagnostic> diagnostics_;
        size_t error_count_ = 0;
        size_t warning_count_ = 0;
        size_t max_errors_ = 20;
        bool treat_warnings_as_errors_ = false;
        mutable std::unordered_map<std::string, std::vector<std::string>> source_cache_;

    public:
        DiagnosticEngine() = default;

        void set_max_errors(size_t max) { max_errors_ = max; }
        void set_warnings_as_errors(bool val) { treat_warnings_as_errors_ = val; }

        void report(Diagnostic diag)
        {
            if (diag.severity == DiagnosticSeverity::Error)
            {
                ++error_count_;
            }
            else if (diag.severity == DiagnosticSeverity::Warning)
            {
                ++warning_count_;
                if (treat_warnings_as_errors_)
                {
                    diag.severity = DiagnosticSeverity::Error;
                    ++error_count_;
                }
            }
            else if (diag.severity == DiagnosticSeverity::Note)
            {
            }

            diagnostics_.push_back(std::move(diag));
        }

        void error(DiagnosticPhase phase, Location loc, std::string msg)
        {
            report(Diagnostic(DiagnosticSeverity::Error, phase, loc, std::move(msg)));
        }

        void warning(DiagnosticPhase phase, Location loc, std::string msg)
        {
            report(Diagnostic(DiagnosticSeverity::Warning, phase, loc, std::move(msg)));
        }

        void note(DiagnosticPhase phase, Location loc, std::string msg)
        {
            report(Diagnostic(DiagnosticSeverity::Note, phase, loc, std::move(msg)));
        }

        bool has_errors() const { return error_count_ > 0; }
        bool reached_error_limit() const { return error_count_ >= max_errors_; }
        size_t error_count() const { return error_count_; }
        size_t warning_count() const { return warning_count_; }
        const std::vector<Diagnostic> &all() const { return diagnostics_; }

        void print_all(std::ostream &os = std::cerr) const
        {
            for (const auto &diag : diagnostics_)
            {
                print_diagnostic(os, diag, 0);
            }

            if (error_count_ > 0 || warning_count_ > 0)
            {
                os << "\n";
                if (error_count_ > 0)
                {
                    os << error_count_ << " error(s)";
                }
                if (warning_count_ > 0)
                {
                    if (error_count_ > 0)
                        os << ", ";
                    os << warning_count_ << " warning(s)";
                }
                os << " generated.\n";
            }
        }

        void clear()
        {
            diagnostics_.clear();
            error_count_ = 0;
            warning_count_ = 0;
        }

    private:
        void print_diagnostic(std::ostream &os, const Diagnostic &diag, size_t indent) const
        {
            std::string prefix(indent * 2, ' ');

            const char *color = "";
            const char *label = "";
            switch (diag.severity)
            {
            case DiagnosticSeverity::Note:
                color = "\033[1;36m";
                label = "note";
                break;
            case DiagnosticSeverity::Warning:
                color = "\033[1;35m";
                label = "warning";
                break;
            case DiagnosticSeverity::Error:
                color = "\033[1;31m";
                label = "error";
                break;
            }
            const char *reset = "\033[0m";

            os << prefix;

            if (diag.location.file_path)
            {
                os << *diag.location.file_path << ":";
            }
            os << diag.location.line << ":" << diag.location.col << ": ";

            os << color << label << reset;

            os << ": " << diag.message << "\n";
            print_source_excerpt(os, diag, prefix);
        }

        std::optional<std::string> get_source_line(const Location &location) const
        {
            if (!location.file_path || location.line == 0)
            {
                return std::nullopt;
            }

            const std::string &path = *location.file_path;
            auto cache_it = source_cache_.find(path);
            if (cache_it == source_cache_.end())
            {
                std::ifstream file(path);
                if (!file.is_open())
                {
                    return std::nullopt;
                }

                std::vector<std::string> lines;
                std::string line;
                while (std::getline(file, line))
                {
                    lines.push_back(line);
                }

                cache_it = source_cache_.emplace(path, std::move(lines)).first;
            }

            const auto &lines = cache_it->second;
            size_t index = static_cast<size_t>(location.line - 1);
            if (index >= lines.size())
            {
                return std::nullopt;
            }

            return lines[index];
        }

        static size_t decimal_width(uint32_t value)
        {
            size_t width = 1;
            while (value >= 10)
            {
                value /= 10;
                ++width;
            }
            return width;
        }

        void print_source_excerpt(std::ostream &os, const Diagnostic &diag,
                                  const std::string &prefix) const
        {
            auto source_line = get_source_line(diag.location);
            if (!source_line.has_value())
            {
                return;
            }

            size_t line_width = decimal_width(diag.location.line);
            os << prefix << std::string(line_width, ' ') << " |\n";
            os << prefix << diag.location.line << " | " << *source_line << "\n";

            size_t caret_column = diag.location.col > 0
                                      ? static_cast<size_t>(diag.location.col - 1)
                                      : 0;
            os << prefix << std::string(line_width, ' ') << " | "
               << std::string(caret_column, ' ') << "^\n";
        }
    };

} // namespace aloha

#endif // ALOHA_DIAGNOSTIC_ENGINE_H_
