#include "ast.h"
#include <iostream>
#include <string>

namespace aloha
{
    thread_local const TySpecArena *g_print_arena = nullptr;

    static std::string type_to_string(TySpecId id)
    {
        if (g_print_arena)
            return g_print_arena->to_string(id);
        return std::to_string(id);
    }

    void StatementBlock::write(std::ostream &os, unsigned long indent) const
    {
        os << std::string(indent, ' ') << "StatementBlock:{\n";
        for (const auto &stmt : m_statements)
        {
            stmt->write(os, indent + 2);
        }
        os << std::string(indent, ' ') << "}\n";
    }

    void ExpressionStatement::write(std::ostream &os, unsigned long indent) const
    {
        os << std::string(indent, ' ') << "ExpressionStatement:{\n";
        m_expr->write(os, indent + 2);
        os << std::string(indent, ' ') << "}\n";
    }

    void Integer::write(std::ostream &os, unsigned long indent) const
    {
        os << std::string(indent, ' ') << "Integer: " << m_value << "\n";
    }

    void Float::write(std::ostream &os, unsigned long indent) const
    {
        os << std::string(indent, ' ') << "Float: " << m_value << "\n";
    }

    void Boolean::write(std::ostream &os, unsigned long indent) const
    {
        os << std::string(indent, ' ') << "Boolean: " << (m_value ? "true" : "false")
           << "\n";
    }

    void String::write(std::ostream &os, unsigned long indent) const
    {
        os << std::string(indent, ' ') << "String: \"" << m_value << "\"\n";
    }

    void UnaryExpression::write(std::ostream &os, unsigned long indent) const
    {
        os << std::string(indent, ' ') << "UnaryExpression:{\n";
        os << std::string(indent + 2, ' ') << "Operator: " << m_op << "\n";
        os << std::string(indent + 2, ' ') << "Operand:{\n";
        m_expr->write(os, indent + 4);
        os << std::string(indent + 2, ' ') << "}\n";
        os << std::string(indent, ' ') << "}\n";
    }

    void BinaryExpression::write(std::ostream &os, unsigned long indent) const
    {
        os << std::string(indent, ' ') << "BinaryExpression:{\n";
        os << std::string(indent + 2, ' ') << "Left:{\n";
        m_left->write(os, indent + 4);
        os << std::string(indent + 2, ' ') << "}\n";
        os << std::string(indent + 2, ' ') << "Operator: " << m_op << "\n";
        os << std::string(indent + 2, ' ') << "Right:{\n";
        m_right->write(os, indent + 4);
        os << std::string(indent + 2, ' ') << "}\n";
        os << std::string(indent, ' ') << "}\n";
    }

    void Identifier::write(std::ostream &os, unsigned long indent) const
    {
        os << std::string(indent, ' ') << "Identifier: " << m_name << "\n";
    }

    void StructFieldAccess::write(std::ostream &os, unsigned long indent) const
    {
        os << std::string(indent, ' ') << "StructFieldAccess:{\n";
        os << std::string(indent + 2, ' ') << "Struct:{\n";
        m_struct_expr->write(os, indent + 4);
        os << std::string(indent + 2, ' ') << "}\n";
        os << std::string(indent + 2, ' ') << "Field: " << m_field_name << "\n";
        os << std::string(indent, ' ') << "}\n";
    }

    void StructFieldAssignment::write(std::ostream &os,
                                      unsigned long indent) const
    {
        os << std::string(indent, ' ') << "StructFieldAssignment:{\n";
        os << std::string(indent + 2, ' ') << "Struct:{\n";
        m_struct_expr->write(os, indent + 4);
        os << std::string(indent + 2, ' ') << "}\n";
        os << std::string(indent + 2, ' ') << "Field: " << m_field_name << "\n";
        os << std::string(indent + 2, ' ') << "Value:{\n";
        m_value->write(os, indent + 4);
        os << std::string(indent + 2, ' ') << "}\n";
        os << std::string(indent, ' ') << "}\n";
    }

    void Declaration::write(std::ostream &os, unsigned long indent) const
    {
        os << std::string(indent, ' ') << "Declaration:{\n";
        os << std::string(indent + 2, ' ') << "Name: " << m_variable_name << "\n";
        os << std::string(indent + 2, ' ')
           << "Type: " << (m_type ? type_to_string(*m_type) : "Inferred")
           << "\n";
        os << std::string(indent + 2, ' ')
           << "Mutable: " << (m_is_mutable ? "true" : "false") << "\n";
        if (m_expression)
        {
            os << std::string(indent + 2, ' ') << "Value:{\n";
            m_expression->write(os, indent + 4);
            os << std::string(indent + 2, ' ') << "}\n";
        }
        os << std::string(indent, ' ') << "}\n";
    }

    void Assignment::write(std::ostream &os, unsigned long indent) const
    {
        os << std::string(indent, ' ') << "Assignment:{\n";
        os << std::string(indent + 2, ' ') << "Variable: " << m_variable_name << "\n";
        os << std::string(indent + 2, ' ') << "Value:{\n";
        m_expression->write(os, indent + 4);
        os << std::string(indent + 2, ' ') << "}\n";
        os << std::string(indent, ' ') << "}\n";
    }

    void FunctionCall::write(std::ostream &os, unsigned long indent) const
    {
        os << std::string(indent, ' ') << "FunctionCall:{\n";
        os << std::string(indent + 2, ' ') << "Name: ";
        m_func_name->write(os, 0);
        os << std::string(indent + 2, ' ') << "Arguments:[\n";
        for (const auto &arg : m_arguments)
        {
            arg->write(os, indent + 4);
        }
        os << std::string(indent + 2, ' ') << "]\n";
        os << std::string(indent, ' ') << "}\n";
    }

    void ReturnStatement::write(std::ostream &os, unsigned long indent) const
    {
        os << std::string(indent, ' ') << "ReturnStatement:{\n";
        if (m_expression)
        {
            m_expression->write(os, indent + 2);
        }
        os << std::string(indent, ' ') << "}\n";
    }

    void IfStatement::write(std::ostream &os, unsigned long indent) const
    {
        os << std::string(indent, ' ') << "IfStatement:{\n";
        os << std::string(indent + 2, ' ') << "Condition:{\n";
        m_condition->write(os, indent + 4);
        os << std::string(indent + 2, ' ') << "}\n";
        os << std::string(indent + 2, ' ') << "ThenBranch:{\n";
        m_then_branch->write(os, indent + 4);
        os << std::string(indent + 2, ' ') << "}\n";
        if (m_else_branch)
        {
            os << std::string(indent + 2, ' ') << "ElseBranch:{\n";
            m_else_branch->write(os, indent + 4);
            os << std::string(indent + 2, ' ') << "}\n";
        }
        os << std::string(indent, ' ') << "}\n";
    }

    void WhileLoop::write(std::ostream &os, unsigned long indent) const
    {
        os << std::string(indent, ' ') << "WhileLoop:{\n";
        os << std::string(indent + 2, ' ') << "Condition:{\n";
        m_condition->write(os, indent + 4);
        os << std::string(indent + 2, ' ') << "}\n";
        os << std::string(indent + 2, ' ') << "Body:{\n";
        m_body->write(os, indent + 4);
        os << std::string(indent + 2, ' ') << "}\n";
        os << std::string(indent, ' ') << "}\n";
    }

    void ForLoop::write(std::ostream &os, unsigned long indent) const
    {
        os << std::string(indent, ' ') << "ForLoop:{\n";
        os << std::string(indent + 2, ' ') << "Initializer:{\n";
        m_initializer->write(os, indent + 4);
        os << std::string(indent + 2, ' ') << "}\n";
        os << std::string(indent + 2, ' ') << "Condition:{\n";
        m_condition->write(os, indent + 4);
        os << std::string(indent + 2, ' ') << "}\n";
        os << std::string(indent + 2, ' ') << "Increment:{\n";
        m_increment->write(os, indent + 4);
        os << std::string(indent + 2, ' ') << "}\n";
        os << std::string(indent + 2, ' ') << "Body:{\n";
        for (const auto &stmt : m_body)
        {
            stmt->write(os, indent + 4);
        }
        os << std::string(indent + 2, ' ') << "}\n";
        os << std::string(indent, ' ') << "}\n";
    }

    void Function::write(std::ostream &os, unsigned long indent) const
    {
        os << std::string(indent, ' ') << "Function:{\n";
        os << std::string(indent + 2, ' ') << "Name: ";
        m_name->write(os, 0);
        os << std::string(indent + 2, ' ') << "Parameters:[\n";
        for (const auto &param : m_parameters)
        {
            os << std::string(indent + 4, ' ') << param.m_name << ": "
               << type_to_string(param.m_type) << "\n";
        }
        os << std::string(indent + 2, ' ') << "]\n";
        os << std::string(indent + 2, ' ')
           << "ReturnType: " << type_to_string(m_return_type) << "\n";
        os << std::string(indent + 2, ' ') << "Body:{\n";
        m_body->write(os, indent + 4);
        os << std::string(indent + 2, ' ') << "}\n";
        os << std::string(indent, ' ') << "}\n";
    }

    void StructDecl::write(std::ostream &os, unsigned long indent) const
    {
        os << std::string(indent, ' ') << "StructDecl:{\n";
        os << std::string(indent + 2, ' ') << "Name: " << m_name << "\n";
        os << std::string(indent + 2, ' ') << "Fields:[\n";
        for (const auto &field : m_fields)
        {
            os << std::string(indent + 4, ' ') << field.m_name << ": "
               << type_to_string(field.m_type) << "\n";
        }
        os << std::string(indent + 2, ' ') << "]\n";
        os << std::string(indent, ' ') << "}\n";
    }

    void StructInstantiation::write(std::ostream &os, unsigned long indent) const
    {
        os << std::string(indent, ' ') << "StructInstantiation:{\n";
        os << std::string(indent + 2, ' ') << "Name: " << m_struct_name << "\n";
        os << std::string(indent + 2, ' ') << "Fields:[\n";
        for (const auto &field : m_field_values)
        {
            field->write(os, indent + 4);
        }
        os << std::string(indent + 2, ' ') << "]\n";
        os << std::string(indent, ' ') << "}\n";
    }

    void Array::write(std::ostream &os, unsigned long indent) const
    {
        os << std::string(indent, ' ') << "Array:{\n";
        os << std::string(indent + 2, ' ') << "Size: " << m_size << "\n";
        os << std::string(indent + 2, ' ') << "Elements:[\n";
        for (const auto &member : m_members)
        {
            member->write(os, indent + 4);
        }
        os << std::string(indent + 2, ' ') << "]\n";
        os << std::string(indent, ' ') << "}\n";
    }

    void ArrayAccess::write(std::ostream &os, unsigned long indent) const
    {
        os << std::string(indent, ' ') << "ArrayAccess:{\n";
        os << std::string(indent + 2, ' ') << "Array:{\n";
        m_array_expr->write(os, indent + 4);
        os << std::string(indent + 2, ' ') << "}\n";
        os << std::string(indent + 2, ' ') << "Index:{\n";
        m_index_expr->write(os, indent + 4);
        os << std::string(indent + 2, ' ') << "}\n";
        os << std::string(indent, ' ') << "}\n";
    }

    void Program::write(std::ostream &os, unsigned long indent) const
    {
        os << std::string(indent, ' ') << "Program:{\n";
        for (const auto &node : m_nodes)
        {
            node->write(os, indent + 2);
        }
        os << std::string(indent, ' ') << "}\n";
    }

    void Program::write(std::ostream &os, const TySpecArena &arena, unsigned long indent) const
    {
        // set the arena for printing
        g_print_arena = &arena;

        os << std::string(indent, ' ') << "Program:{\n";
        for (const auto &node : m_nodes)
        {
            node->write(os, indent + 2);
        }
        os << std::string(indent, ' ') << "}\n";

        g_print_arena = nullptr;
    }

    void Import::write(std::ostream &os, unsigned long indent) const
    {
        os << std::string(indent, ' ') << "Import: \"" << m_path << "\"\n";
    }

} // namespace aloha
