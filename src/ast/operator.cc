#include "operator.h"
#include "../error/internal.h"

namespace aloha
{
    namespace ast
    {
        namespace Operator
        {
            std::optional<Binary> token_to_binary_op(const Token &token)
            {
                switch (token.kind)
                {
                case TokenKind::PLUS:
                    return Binary::PLUS;
                case TokenKind::MINUS:
                    return Binary::MINUS;
                case TokenKind::STAR:
                    return Binary::MULTIPLY;
                case TokenKind::SLASH:
                    return Binary::DIVIDE;
                case TokenKind::PERCENT:
                    return Binary::MODULO;
                case TokenKind::EQUAL_EQUAL:
                    return Binary::EQUAL;
                case TokenKind::NOT_EQUAL:
                    return Binary::NOT_EQUAL;
                case TokenKind::LESS_THAN:
                    return Binary::LESS;
                case TokenKind::LESS_EQUAL:
                    return Binary::LESS_EQUAL;
                case TokenKind::GREATER_THAN:
                    return Binary::GREATER;
                case TokenKind::GREATER_EQUAL:
                    return Binary::GREATER_EQUAL;
                default:
                    return std::nullopt;
                }
            }

            std::string string(Binary op)
            {
                switch (op)
                {
                case Binary::PLUS:
                    return "+";
                case Binary::MINUS:
                    return "-";
                case Binary::MULTIPLY:
                    return "*";
                case Binary::DIVIDE:
                    return "/";
                case Binary::MODULO:
                    return "%";
                case Binary::EQUAL:
                    return "==";
                case Binary::NOT_EQUAL:
                    return "!=";
                case Binary::LESS:
                    return "<";
                case Binary::LESS_EQUAL:
                    return "<=";
                case Binary::GREATER:
                    return ">";
                case Binary::GREATER_EQUAL:
                    return ">=";
                default:
                    ALOHA_UNREACHABLE();
                }
            }

            std::optional<Unary> token_to_unary_op(const Token &token)
            {
                switch (token.kind)
                {
                case TokenKind::MINUS:
                    return Unary::NEGATE;
                case TokenKind::BANG:
                    return Unary::NOT;
                default:
                    return std::nullopt;
                }
            }

            std::string string(Unary op)
            {
                switch (op)
                {
                case Unary::NEGATE:
                    return "-";
                case Unary::NOT:
                    return "!";
                default:
                    ALOHA_UNREACHABLE();
                }
            }

        } // namespace Operator

    } // namespace ast
} // namespace aloha
