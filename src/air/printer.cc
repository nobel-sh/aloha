#include "printer.h"

namespace aloha
{
    namespace air
    {

        Printer::Printer(std::ostream &os, const TyTable *ty_table)
            : os(os), ty_table(ty_table), indent(0) {}

        void Printer::print(Module *module)
        {
            if (!module)
                return;
            module->accept(*this);
        }

        void Printer::write_indent()
        {
            os << std::string(indent, ' ');
        }

        std::string Printer::ty_name(TyId ty) const
        {
            if (ty_table)
            {
                return ty_table->ty_name(ty);
            }
            return std::to_string(static_cast<uint32_t>(ty));
        }

        void Printer::visit(IntegerLiteral *node)
        {
            write_indent();
            os << "IntegerLiteral: " << node->m_value << "\n";
        }

        void Printer::visit(FloatLiteral *node)
        {
            write_indent();
            os << "FloatLiteral: " << node->m_value << "\n";
        }

        void Printer::visit(StringLiteral *node)
        {
            write_indent();
            os << "StringLiteral: \"" << node->m_value << "\"\n";
        }

        void Printer::visit(BoolLiteral *node)
        {
            write_indent();
            os << "BoolLiteral: " << (node->m_value ? "true" : "false") << "\n";
        }

        void Printer::visit(VarRef *node)
        {
            write_indent();
            os << "VarRef: " << node->m_name << " (id=" << node->m_var_id
               << ", ty=" << ty_name(node->m_ty) << ")\n";
        }

        void Printer::visit(BinaryOp *node)
        {
            write_indent();
            os << "BinaryOp: " << BinaryOp::op_to_string(node->m_op) << " (ty="
               << ty_name(node->m_ty) << ")\n";

            indent += 2;
            write_indent();
            os << "Left:\n";
            indent += 2;
            node->m_left->accept(*this);
            indent -= 2;

            write_indent();
            os << "Right:\n";
            indent += 2;
            node->m_right->accept(*this);
            indent -= 2;

            indent -= 2;
        }

        void Printer::visit(UnaryOp *node)
        {
            write_indent();
            os << "UnaryOp: " << UnaryOp::op_to_string(node->m_op) << " (ty="
               << ty_name(node->m_ty) << ")\n";

            indent += 2;
            node->m_operand->accept(*this);
            indent -= 2;
        }

        void Printer::visit(Call *node)
        {
            write_indent();
            os << "Call: " << node->m_function_name << " (id=" << node->m_func_id
               << ", ty=" << ty_name(node->m_ty) << ")\n";

            indent += 2;
            write_indent();
            os << "Arguments:\n";
            indent += 2;
            for (const auto &arg : node->m_arguments)
            {
                arg->accept(*this);
            }
            indent -= 2;
            indent -= 2;
        }

        void Printer::visit(StructInstantiation *node)
        {
            write_indent();
            os << "StructInstantiation: " << node->m_struct_name << " (id="
               << node->m_struct_id << ", ty=" << ty_name(node->m_ty) << ")\n";

            indent += 2;
            write_indent();
            os << "Fields:\n";
            indent += 2;
            for (const auto &value : node->m_field_values)
            {
                value->accept(*this);
            }
            indent -= 2;
            indent -= 2;
        }

        void Printer::visit(FieldAccess *node)
        {
            write_indent();
            os << "FieldAccess: " << node->m_field_name << " (index=" << node->m_field_index
               << ", ty=" << ty_name(node->m_ty) << ")\n";

            indent += 2;
            node->m_object->accept(*this);
            indent -= 2;
        }

        void Printer::visit(ArrayExpr *node)
        {
            write_indent();
            os << "ArrayExpr: (ty=" << ty_name(node->m_ty) << ", elements=" << node->m_elements.size() << ")\n";

            indent += 2;
            for (const auto &elem : node->m_elements)
            {
                elem->accept(*this);
            }
            indent -= 2;
        }

        void Printer::visit(VarDecl *node)
        {
            write_indent();
            os << "VarDecl: " << node->m_name << " (id=" << node->m_var_id
               << ", ty=" << ty_name(node->m_var_ty)
               << ", mutable=" << (node->m_is_mutable ? "true" : "false") << ")\n";

            if (node->m_initializer)
            {
                indent += 2;
                write_indent();
                os << "Initializer:\n";
                indent += 2;
                node->m_initializer->accept(*this);
                indent -= 2;
                indent -= 2;
            }
        }

        void Printer::visit(Assignment *node)
        {
            write_indent();
            os << "Assignment: " << node->m_var_name << " (id=" << node->m_var_id << ")\n";

            indent += 2;
            node->m_value->accept(*this);
            indent -= 2;
        }

        void Printer::visit(FieldAssignment *node)
        {
            write_indent();
            os << "FieldAssignment: " << node->m_field_name << " (index=" << node->m_field_index
               << ")\n";

            indent += 2;
            write_indent();
            os << "Object:\n";
            indent += 2;
            node->m_object->accept(*this);
            indent -= 2;

            write_indent();
            os << "Value:\n";
            indent += 2;
            node->m_value->accept(*this);
            indent -= 2;

            indent -= 2;
        }

        void Printer::visit(ArrayAccess *node)
        {
            write_indent();
            os << "ArrayAccess: (ty=" << ty_name(node->m_ty) << ")\n";

            indent += 2;
            write_indent();
            os << "Array:\n";
            indent += 2;
            node->m_array_expr->accept(*this);
            indent -= 2;

            write_indent();
            os << "Index:\n";
            indent += 2;
            node->m_index_expr->accept(*this);
            indent -= 2;

            indent -= 2;
        }

        void Printer::visit(Return *node)
        {
            write_indent();
            os << "Return:\n";

            if (node->m_value)
            {
                indent += 2;
                node->m_value->accept(*this);
                indent -= 2;
            }
        }

        void Printer::visit(If *node)
        {
            write_indent();
            os << "If:\n";

            indent += 2;
            write_indent();
            os << "Condition:\n";
            indent += 2;
            node->m_condition->accept(*this);
            indent -= 2;

            write_indent();
            os << "Then:\n";
            indent += 2;
            for (const auto &stmt : node->m_then_branch)
            {
                stmt->accept(*this);
            }
            indent -= 2;

            if (!node->m_else_branch.empty())
            {
                write_indent();
                os << "Else:\n";
                indent += 2;
                for (const auto &stmt : node->m_else_branch)
                {
                    stmt->accept(*this);
                }
                indent -= 2;
            }

            indent -= 2;
        }

        void Printer::visit(ExprStmt *node)
        {
            write_indent();
            os << "ExprStmt:\n";
            indent += 2;
            node->m_expression->accept(*this);
            indent -= 2;
        }

        void Printer::visit(Function *node)
        {
            write_indent();
            os << "Function: " << node->m_name << " (id=" << node->m_func_id
               << ", return=" << ty_name(node->m_return_ty)
               << ", extern=" << (node->m_is_extern ? "true" : "false") << ")\n";

            indent += 2;
            write_indent();
            os << "Params:\n";
            indent += 2;
            for (const auto &param : node->m_params)
            {
                write_indent();
                os << param.m_name << " (id=" << param.m_var_id
                   << ", ty=" << ty_name(param.m_ty)
                   << ", mutable=" << (param.m_is_mutable ? "true" : "false") << ")\n";
            }
            indent -= 2;

            if (!node->m_body.empty())
            {
                write_indent();
                os << "Body:\n";
                indent += 2;
                for (const auto &stmt : node->m_body)
                {
                    stmt->accept(*this);
                }
                indent -= 2;
            }

            indent -= 2;
        }

        void Printer::visit(StructDecl *node)
        {
            write_indent();
            os << "StructDecl: " << node->m_name << " (id=" << node->m_struct_id
               << ", ty=" << ty_name(node->m_ty_id) << ")\n";

            indent += 2;
            write_indent();
            os << "Fields:\n";
            indent += 2;
            for (const auto &field : node->m_fields)
            {
                write_indent();
                os << field.m_name << " (index=" << field.m_index
                   << ", ty=" << ty_name(field.m_ty) << ")\n";
            }
            indent -= 2;
            indent -= 2;
        }

        void Printer::visit(Module *node)
        {
            write_indent();
            os << "Module: " << node->m_name << "\n";

            indent += 2;
            if (!node->m_imports.empty())
            {
                write_indent();
                os << "Imports:\n";
                indent += 2;
                for (const auto &imp : node->m_imports)
                {
                    write_indent();
                    os << imp << "\n";
                }
                indent -= 2;
            }

            if (!node->m_structs.empty())
            {
                write_indent();
                os << "Structs:\n";
                indent += 2;
                for (const auto &s : node->m_structs)
                {
                    s->accept(*this);
                }
                indent -= 2;
            }

            if (!node->m_functions.empty())
            {
                write_indent();
                os << "Functions:\n";
                indent += 2;
                for (const auto &f : node->m_functions)
                {
                    f->accept(*this);
                }
                indent -= 2;
            }
            indent -= 2;
        }

    } // namespace air
} // namespace aloha
