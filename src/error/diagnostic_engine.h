#ifndef ALOHA_DIAGNOSTIC_ENGINE_H_
#define ALOHA_DIAGNOSTIC_ENGINE_H_

#include "diagnostic.h"
#include <vector>
#include <functional>
#include <iostream>

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
        }
    };

} // namespace aloha

#endif // ALOHA_DIAGNOSTIC_ENGINE_H_
