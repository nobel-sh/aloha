#ifndef ALOHA_AST_OPERATOR_H_
#define ALOHA_AST_OPERATOR_H_

#include <string>
#include <optional>
#include "../frontend/token.h"

namespace aloha
{
    namespace ast
    {
        namespace Operator
        {
            enum class Binary
            {
                PLUS,
                MINUS,
                MULTIPLY,
                DIVIDE,
                MODULO,
                EQUAL,
                NOT_EQUAL,
                LESS,
                LESS_EQUAL,
                GREATER,
                GREATER_EQUAL,
            };

            enum class Unary
            {
                NEGATE,
                NOT
            };

            std::optional<Binary> token_to_binary_op(const Token &token);
            std::string string(Binary op);
            std::optional<Unary> token_to_unary_op(const Token &token);
            std::string string(Unary op);

        } // namespace Operator

    } // namespace ast
} // namespace aloha

#endif // ALOHA_AST_OPERATOR_H_
