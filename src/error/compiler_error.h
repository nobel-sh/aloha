#ifndef ALOHA_COMPILER_ERROR_H_
#define ALOHA_COMPILER_ERROR_H_

#include "../frontend/location.h"
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#define ALOHA_ICE(msg)                                                           \
    do                                                                           \
    {                                                                            \
        std::cerr << "\n\033[1;31mICE\033[0m at " << __FILE__ << ":" << __LINE__ \
                  << " in " << __func__ << ":\n  " << (msg) << "\n";             \
        std::abort();                                                            \
    } while (0)

#define ALOHA_ICE_IF(cond, msg) \
    do                          \
    {                           \
        if (cond)               \
        {                       \
            ALOHA_ICE(msg);     \
        }                       \
    } while (0)

#define ALOHA_UNIMPLEMENTED()                                                  \
    do                                                                         \
    {                                                                          \
        std::cerr << "\n\033[1;33mUNIMPLEMENTED\033[0m at " << __FILE__ << ":" \
                  << __LINE__ << " in " << __func__ << "\n";                   \
        std::abort();                                                          \
    } while (0)

#define ALOHA_UNIMPLEMENTED_MSG(msg)                                             \
    do                                                                           \
    {                                                                            \
        std::cerr << "\n\033[1;33mUNIMPLEMENTED\033[0m at " << __FILE__ << ":"   \
                  << __LINE__ << " in " << __func__ << ":\n  " << (msg) << "\n"; \
        std::abort();                                                            \
    } while (0)

#define ALOHA_UNREACHABLE()                                                  \
    do                                                                       \
    {                                                                        \
        std::cerr << "\n\033[1;31mUNREACHABLE\033[0m at " << __FILE__ << ":" \
                  << __LINE__ << " in " << __func__ << "\n";                 \
        std::abort();                                                        \
    } while (0)

namespace Aloha
{

    struct ErrorEntry
    {
        Location loc;
        std::string message;

        ErrorEntry(Location l, std::string msg) : loc(l), message(std::move(msg)) {}
        ErrorEntry(uint32_t line, uint32_t col, std::string msg)
            : loc(line, col), message(std::move(msg)) {}
    };

    // Abstract base class for all error collectors
    class ErrorBase
    {
    public:
        virtual ~ErrorBase() = default;

        void add_error(Location loc, const std::string &msg)
        {
            errors_.emplace_back(loc, msg);
        }

        void add_error(uint32_t line, uint32_t col, const std::string &msg)
        {
            errors_.emplace_back(line, col, msg);
        }

        bool has_errors() const { return !errors_.empty(); }
        size_t count() const { return errors_.size(); }
        const std::vector<ErrorEntry> &get_errors() const { return errors_; }

        void print() const
        {
            for (const auto &e : errors_)
            {
                std::cerr << "\033[1;31m" << prefix() << "\033[0m ";
                if (e.loc.file_path)
                {
                    std::cerr << *e.loc.file_path << ":" << e.loc.line << ":" << e.loc.col << ": ";
                }
                std::cerr << e.message << "\n";
            }
        }

        void clear() { errors_.clear(); }

    protected:
        virtual const char *prefix() const = 0;
        std::vector<ErrorEntry> errors_;
    };

    class LexerError final : public ErrorBase
    {
    protected:
        const char *prefix() const override { return "lexer error"; }
    };

    class ParserError final : public ErrorBase
    {
    public:
        using ErrorBase::add_error;

        void add_error(const std::string &msg)
        {
            errors_.emplace_back(0, 0, msg);
            error_strings_.push_back(msg);
        }

        // Override to also track string versions for parser compatibility
        void add_error(Location loc, const std::string &msg)
        {
            ErrorBase::add_error(loc, msg);
            if (loc.file_path)
            {
                error_strings_.push_back("[" + *loc.file_path + ":" + std::to_string(loc.line) + ":" +
                                         std::to_string(loc.col) + "] " + msg);
            }
            else
            {
                error_strings_.push_back(msg);
            }
        }

        // Returns strings for parser compatibility
        const std::vector<std::string> &get_errors() const { return error_strings_; }
        const std::vector<ErrorEntry> &get_error_entries() const { return errors_; }

        void clear()
        {
            ErrorBase::clear();
            error_strings_.clear();
        }

    protected:
        const char *prefix() const override { return "parser error"; }

    private:
        std::vector<std::string> error_strings_;
    };

    class TyError final : public ErrorBase
    {
    protected:
        const char *prefix() const override { return "type error"; }
    };

    class AIRError final : public ErrorBase
    {
    protected:
        const char *prefix() const override { return "AIR error"; }
    };

    class CodegenError final : public ErrorBase
    {
    public:
        using ErrorBase::add_error;

        void add_error(const std::string &msg)
        {
            errors_.emplace_back(0, 0, msg);
        }

    protected:
        const char *prefix() const override { return "codegen error"; }
    };

    class ParserException : public std::runtime_error
    {
    public:
        explicit ParserException(const std::string &message)
            : std::runtime_error(message) {}
    };

} // namespace Aloha
#endif // ALOHA_COMPILER_ERROR_H_
