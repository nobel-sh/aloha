#ifndef ALOHA_DIAGNOSTIC_H_
#define ALOHA_DIAGNOSTIC_H_

#include "../frontend/location.h"
#include <string>
#include <vector>
#include <optional>

namespace aloha
{

    enum class DiagnosticSeverity
    {
        Warning,
        Error,
    };

    enum class DiagnosticPhase
    {
        Lexer,
        Parser,
        SymbolBinding,
        TypeResolution,
        TypeChecking,
        AIRBuilding,
        Codegen
    };

    struct Diagnostic
    {
        DiagnosticSeverity severity;
        DiagnosticPhase phase;
        Location location;
        std::string message;

        Diagnostic(DiagnosticSeverity sev, DiagnosticPhase ph,
                   Location loc, std::string msg)
            : severity(sev), phase(ph), location(loc), message(std::move(msg)) {}
    };

} // namespace aloha

#endif // ALOHA_DIAGNOSTIC_H_
