#include "printer.h"

namespace AIR
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
        os << "IntegerLiteral: " << node->value << "\n";
    }

    void Printer::visit(FloatLiteral *node)
    {
        write_indent();
        os << "FloatLiteral: " << node->value << "\n";
    }

    void Printer::visit(StringLiteral *node)
    {
        write_indent();
        os << "StringLiteral: \"" << node->value << "\"\n";
    }

    void Printer::visit(BoolLiteral *node)
    {
        write_indent();
        os << "BoolLiteral: " << (node->value ? "true" : "false") << "\n";
    }

    void Printer::visit(VarRef *node)
    {
        write_indent();
        os << "VarRef: " << node->name << " (id=" << node->var_id
           << ", ty=" << ty_name(node->ty) << ")\n";
    }

    void Printer::visit(BinaryOp *node)
    {
        write_indent();
        os << "BinaryOp: " << BinaryOp::op_to_string(node->op) << " (ty="
           << ty_name(node->ty) << ")\n";

        indent += 2;
        write_indent();
        os << "Left:\n";
        indent += 2;
        node->left->accept(*this);
        indent -= 2;

        write_indent();
        os << "Right:\n";
        indent += 2;
        node->right->accept(*this);
        indent -= 2;

        indent -= 2;
    }

    void Printer::visit(UnaryOp *node)
    {
        write_indent();
        os << "UnaryOp: " << UnaryOp::op_to_string(node->op) << " (ty="
           << ty_name(node->ty) << ")\n";

        indent += 2;
        node->operand->accept(*this);
        indent -= 2;
    }

    void Printer::visit(Call *node)
    {
        write_indent();
        os << "Call: " << node->function_name << " (id=" << node->func_id
           << ", ty=" << ty_name(node->ty) << ")\n";

        indent += 2;
        write_indent();
        os << "Arguments:\n";
        indent += 2;
        for (const auto &arg : node->arguments)
        {
            arg->accept(*this);
        }
        indent -= 2;
        indent -= 2;
    }

    void Printer::visit(StructInstantiation *node)
    {
        write_indent();
        os << "StructInstantiation: " << node->struct_name << " (id="
           << node->struct_id << ", ty=" << ty_name(node->ty) << ")\n";

        indent += 2;
        write_indent();
        os << "Fields:\n";
        indent += 2;
        for (const auto &value : node->field_values)
        {
            value->accept(*this);
        }
        indent -= 2;
        indent -= 2;
    }

    void Printer::visit(FieldAccess *node)
    {
        write_indent();
        os << "FieldAccess: " << node->field_name << " (index=" << node->field_index
           << ", ty=" << ty_name(node->ty) << ")\n";

        indent += 2;
        node->object->accept(*this);
        indent -= 2;
    }

    void Printer::visit(VarDecl *node)
    {
        write_indent();
        os << "VarDecl: " << node->name << " (id=" << node->var_id
           << ", ty=" << ty_name(node->var_ty)
           << ", mutable=" << (node->is_mutable ? "true" : "false") << ")\n";

        if (node->initializer)
        {
            indent += 2;
            write_indent();
            os << "Initializer:\n";
            indent += 2;
            node->initializer->accept(*this);
            indent -= 2;
            indent -= 2;
        }
    }

    void Printer::visit(Assignment *node)
    {
        write_indent();
        os << "Assignment: " << node->var_name << " (id=" << node->var_id << ")\n";

        indent += 2;
        node->value->accept(*this);
        indent -= 2;
    }

    void Printer::visit(FieldAssignment *node)
    {
        write_indent();
        os << "FieldAssignment: " << node->field_name << " (index=" << node->field_index
           << ")\n";

        indent += 2;
        write_indent();
        os << "Object:\n";
        indent += 2;
        node->object->accept(*this);
        indent -= 2;

        write_indent();
        os << "Value:\n";
        indent += 2;
        node->value->accept(*this);
        indent -= 2;

        indent -= 2;
    }

    void Printer::visit(Return *node)
    {
        write_indent();
        os << "Return:\n";

        if (node->value)
        {
            indent += 2;
            node->value->accept(*this);
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
        node->condition->accept(*this);
        indent -= 2;

        write_indent();
        os << "Then:\n";
        indent += 2;
        for (const auto &stmt : node->then_branch)
        {
            stmt->accept(*this);
        }
        indent -= 2;

        if (!node->else_branch.empty())
        {
            write_indent();
            os << "Else:\n";
            indent += 2;
            for (const auto &stmt : node->else_branch)
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
        node->expression->accept(*this);
        indent -= 2;
    }

    void Printer::visit(Function *node)
    {
        write_indent();
        os << "Function: " << node->name << " (id=" << node->func_id
           << ", return=" << ty_name(node->return_ty)
           << ", extern=" << (node->is_extern ? "true" : "false") << ")\n";

        indent += 2;
        write_indent();
        os << "Params:\n";
        indent += 2;
        for (const auto &param : node->params)
        {
            write_indent();
            os << param.name << " (id=" << param.var_id
               << ", ty=" << ty_name(param.ty)
               << ", mutable=" << (param.is_mutable ? "true" : "false") << ")\n";
        }
        indent -= 2;

        if (!node->body.empty())
        {
            write_indent();
            os << "Body:\n";
            indent += 2;
            for (const auto &stmt : node->body)
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
        os << "StructDecl: " << node->name << " (id=" << node->struct_id
           << ", ty=" << ty_name(node->ty_id) << ")\n";

        indent += 2;
        write_indent();
        os << "Fields:\n";
        indent += 2;
        for (const auto &field : node->fields)
        {
            write_indent();
            os << field.name << " (index=" << field.index
               << ", ty=" << ty_name(field.ty) << ")\n";
        }
        indent -= 2;
        indent -= 2;
    }

    void Printer::visit(Module *node)
    {
        write_indent();
        os << "Module: " << node->name << "\n";

        indent += 2;
        if (!node->imports.empty())
        {
            write_indent();
            os << "Imports:\n";
            indent += 2;
            for (const auto &imp : node->imports)
            {
                write_indent();
                os << imp << "\n";
            }
            indent -= 2;
        }

        if (!node->structs.empty())
        {
            write_indent();
            os << "Structs:\n";
            indent += 2;
            for (const auto &s : node->structs)
            {
                s->accept(*this);
            }
            indent -= 2;
        }

        if (!node->functions.empty())
        {
            write_indent();
            os << "Functions:\n";
            indent += 2;
            for (const auto &f : node->functions)
            {
                f->accept(*this);
            }
            indent -= 2;
        }
        indent -= 2;
    }

} // namespace AIR
