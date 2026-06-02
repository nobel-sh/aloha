#include "ast.h"

namespace aloha
{
    namespace ast
    {

        void StatementBlock::accept(ASTVisitor &visitor) { visitor.visit(this); }
        void ExpressionStatement::accept(ASTVisitor &visitor) { visitor.visit(this); }
        void Integer::accept(ASTVisitor &visitor) { visitor.visit(this); }
        void Float::accept(ASTVisitor &visitor) { visitor.visit(this); }
        void Boolean::accept(ASTVisitor &visitor) { visitor.visit(this); }
        void Null::accept(ASTVisitor &visitor) { visitor.visit(this); }
        void String::accept(ASTVisitor &visitor) { visitor.visit(this); }
        void UnaryExpression::accept(ASTVisitor &visitor) { visitor.visit(this); }
        void BinaryExpression::accept(ASTVisitor &visitor) { visitor.visit(this); }
        void Identifier::accept(ASTVisitor &visitor) { visitor.visit(this); }
        void EnumVariant::accept(ASTVisitor &visitor) { visitor.visit(this); }
        void MatchExpression::accept(ASTVisitor &visitor) { visitor.visit(this); }
        void StructFieldAccess::accept(ASTVisitor &visitor) { visitor.visit(this); }
        void StructFieldAssignment::accept(ASTVisitor &visitor) { visitor.visit(this); }
        void Declaration::accept(ASTVisitor &visitor) { visitor.visit(this); }
        void Assignment::accept(ASTVisitor &visitor) { visitor.visit(this); }
        void ArrayAssignment::accept(ASTVisitor &visitor) { visitor.visit(this); }
        void FunctionCall::accept(ASTVisitor &visitor) { visitor.visit(this); }
        void ReturnStatement::accept(ASTVisitor &visitor) { visitor.visit(this); }
        void BreakStatement::accept(ASTVisitor &visitor) { visitor.visit(this); }
        void ContinueStatement::accept(ASTVisitor &visitor) { visitor.visit(this); }
        void IfStatement::accept(ASTVisitor &visitor) { visitor.visit(this); }
        void MatchStatement::accept(ASTVisitor &visitor) { visitor.visit(this); }
        void WhileLoop::accept(ASTVisitor &visitor) { visitor.visit(this); }
        void ForLoop::accept(ASTVisitor &visitor) { visitor.visit(this); }
        void Function::accept(ASTVisitor &visitor) { visitor.visit(this); }
        void StructDecl::accept(ASTVisitor &visitor) { visitor.visit(this); }
        void EnumDecl::accept(ASTVisitor &visitor) { visitor.visit(this); }
        void ExternTypeDecl::accept(ASTVisitor &visitor) { visitor.visit(this); }
        void StructInstantiation::accept(ASTVisitor &visitor) { visitor.visit(this); }
        void NewObjectExpression::accept(ASTVisitor &visitor) { visitor.visit(this); }
        void Array::accept(ASTVisitor &visitor) { visitor.visit(this); }
        void ArrayAccess::accept(ASTVisitor &visitor) { visitor.visit(this); }
        void Program::accept(ASTVisitor &visitor) { visitor.visit(this); }
        void Import::accept(ASTVisitor &visitor) { visitor.visit(this); }

        // Constructors
        StatementBlock::StatementBlock(Location loc, std::vector<StmtPtr> stmts)
            : Statement(loc), m_statements(std::move(stmts)) {}

        ExpressionStatement::ExpressionStatement(Location loc, ExprPtr expr)
            : Statement(loc), m_expr(std::move(expr)) {}

        Integer::Integer(Location loc, int64_t val)
            : Expression(loc), m_value(val) {}

        Float::Float(Location loc, double val)
            : Expression(loc), m_value(val) {}

        Boolean::Boolean(Location loc, bool val) : Expression(loc), m_value(val) {}

        Null::Null(Location loc) : Expression(loc) {}

        String::String(Location loc, std::string val)
            : Expression(loc), m_value(std::move(val)) {}

        UnaryExpression::UnaryExpression(Location loc, Operator::Unary oper, ExprPtr expr)
            : Expression(loc), m_op(std::move(oper)), m_expr(std::move(expr)) {}

        BinaryExpression::BinaryExpression(Location loc, ExprPtr lhs, Operator::Binary oper,
                                           ExprPtr rhs)
            : Expression(loc), m_left(std::move(lhs)), m_op(std::move(oper)),
              m_right(std::move(rhs)) {}

        Identifier::Identifier(Location loc, std::string name)
            : Expression(loc), m_name(std::move(name)) {}

        EnumVariant::EnumVariant(Location loc, std::string enum_name,
                                 std::string variant_name)
            : Expression(loc), m_enum_name(std::move(enum_name)),
              m_variant_name(std::move(variant_name)) {}

        MatchExprArm::MatchExprArm(Location loc, std::string enum_name,
                                   std::string variant_name, ExprPtr value)
            : m_loc(loc), m_is_wildcard(false), m_enum_name(std::move(enum_name)),
              m_variant_name(std::move(variant_name)), m_value(std::move(value)) {}

        MatchExprArm::MatchExprArm(Location loc, ExprPtr value)
            : m_loc(loc), m_is_wildcard(true), m_value(std::move(value)) {}

        MatchExpression::MatchExpression(Location loc, ExprPtr scrutinee,
                                         std::vector<MatchExprArm> arms)
            : Expression(loc), m_scrutinee(std::move(scrutinee)),
              m_arms(std::move(arms)) {}

        StructFieldAccess::StructFieldAccess(Location loc, ExprPtr struct_expr,
                                             std::string field_name)
            : Expression(loc), m_struct_expr(std::move(struct_expr)),
              m_field_name(std::move(field_name)) {}

        StructFieldAssignment::StructFieldAssignment(Location loc, ExprPtr struct_expr,
                                                     std::string field_name,
                                                     ExprPtr value)
            : Statement(loc), m_struct_expr(std::move(struct_expr)),
              m_field_name(std::move(field_name)), m_value(std::move(value)) {}

        Declaration::Declaration(Location loc, std::string var_name,
                                 std::optional<Type> type, ExprPtr expr,
                                 bool is_mutable)
            : Statement(loc), m_variable_name(std::move(var_name)), m_type(type),
              m_expression(std::move(expr)), m_is_assigned(m_expression != nullptr),
              m_is_mutable(is_mutable) {}

        Assignment::Assignment(Location loc, std::string var_name, ExprPtr expr)
            : Statement(loc), m_variable_name(std::move(var_name)),
              m_expression(std::move(expr)) {}

        ArrayAssignment::ArrayAssignment(Location loc, std::string array_name,
                                         ExprPtr index_expr, ExprPtr value)
            : Statement(loc), m_array_name(std::move(array_name)),
              m_index_expr(std::move(index_expr)), m_value(std::move(value)) {}

        FunctionCall::FunctionCall(Location loc, std::unique_ptr<Identifier> func_name,
                                   std::vector<ExprPtr> args)
            : Expression(loc), m_func_name(std::move(func_name)),
              m_arguments(std::move(args)) {}

        ReturnStatement::ReturnStatement(Location loc, ExprPtr expr)
            : Statement(loc), m_expression(std::move(expr)) {}

        BreakStatement::BreakStatement(Location loc) : Statement(loc) {}

        ContinueStatement::ContinueStatement(Location loc) : Statement(loc) {}

        IfStatement::IfStatement(Location loc, ExprPtr cond,
                                 std::unique_ptr<StatementBlock> then_branch,
                                 std::unique_ptr<StatementBlock> else_branch)
            : Statement(loc), m_condition(std::move(cond)),
              m_then_branch(std::move(then_branch)),
              m_else_branch(std::move(else_branch)) {}
        bool IfStatement::has_else_branch() const { return m_else_branch != nullptr; }

        MatchArm::MatchArm(Location loc, std::string enum_name,
                           std::string variant_name,
                           std::unique_ptr<StatementBlock> body)
            : m_loc(loc), m_is_wildcard(false), m_enum_name(std::move(enum_name)),
              m_variant_name(std::move(variant_name)), m_body(std::move(body)) {}

        MatchArm::MatchArm(Location loc, std::unique_ptr<StatementBlock> body)
            : m_loc(loc), m_is_wildcard(true), m_body(std::move(body)) {}

        MatchStatement::MatchStatement(Location loc, ExprPtr scrutinee,
                                       std::vector<MatchArm> arms)
            : Statement(loc), m_scrutinee(std::move(scrutinee)),
              m_arms(std::move(arms)) {}

        WhileLoop::WhileLoop(Location loc, ExprPtr cond,
                             std::unique_ptr<StatementBlock> body)
            : Statement(loc), m_condition(std::move(cond)), m_body(std::move(body)) {}

        ForLoop::ForLoop(Location loc, std::unique_ptr<Declaration> init, ExprPtr cond,
                         std::unique_ptr<Declaration> inc, std::vector<StmtPtr> body)
            : Statement(loc), m_initializer(std::move(init)),
              m_condition(std::move(cond)), m_increment(std::move(inc)),
              m_body(std::move(body)) {}

        Parameter::Parameter(std::string name, Type type)
            : m_name(std::move(name)), m_type(type), m_loc(Location()) {}

        Parameter::Parameter(std::string name, Type type, std::string type_name)
            : m_name(std::move(name)), m_type(type), m_loc(Location()) {}

        Parameter::Parameter(Location loc, std::string name, Type type)
            : m_name(std::move(name)), m_type(type), m_loc(loc) {}

        Parameter::Parameter(Location loc, std::string name, Type type, std::string type_name)
            : m_name(std::move(name)), m_type(type), m_loc(loc) {}

        Function::Function(Location loc, std::unique_ptr<Identifier> func_name,
                           std::vector<Parameter> params, Type return_type,
                           std::unique_ptr<StatementBlock> body, bool is_extern)
            : Statement(loc), m_name(std::move(func_name)),
              m_parameters(std::move(params)), m_return_type(return_type),
              m_body(std::move(body)), m_is_extern(is_extern) {}

        Function::Function(Location loc, std::unique_ptr<Identifier> func_name,
                           std::vector<Parameter> params, Type return_type,
                           std::string return_type_name,
                           std::unique_ptr<StatementBlock> body, bool is_extern)
            : Statement(loc), m_name(std::move(func_name)),
              m_parameters(std::move(params)), m_return_type(return_type),
              m_body(std::move(body)), m_is_extern(is_extern) {}

        StructField::StructField(std::string name, Type type)
            : m_name(std::move(name)), m_type(type), m_loc(Location()) {}

        StructField::StructField(std::string name, Type type, std::string type_name)
            : m_name(std::move(name)), m_type(type), m_loc(Location()) {}

        StructField::StructField(Location loc, std::string name, Type type)
            : m_name(std::move(name)), m_type(type), m_loc(loc) {}

        StructField::StructField(Location loc, std::string name, Type type, std::string type_name)
            : m_name(std::move(name)), m_type(type), m_loc(loc) {}

        StructDecl::StructDecl(Location loc, std::string name,
                               std::vector<StructField> fields)
            : Statement(loc), m_name(std::move(name)), m_fields(std::move(fields)) {}

        EnumDecl::EnumDecl(Location loc, std::string name,
                           std::vector<std::string> variants)
            : Statement(loc), m_name(std::move(name)), m_variants(std::move(variants)) {}

        ExternTypeDecl::ExternTypeDecl(Location loc, std::string name)
            : Statement(loc), m_name(std::move(name)) {}

        StructInstantiation::StructInstantiation(Location loc, std::string name,
                                                 std::vector<FieldValue> values)
            : Expression(loc), m_struct_name(std::move(name)),
              m_field_values(std::move(values)) {}

        StructInstantiation::FieldValue::FieldValue(std::string name, ExprPtr value)
            : m_name(std::move(name)), m_value(std::move(value)) {}

        NewObjectExpression::NewObjectExpression(Location loc, std::string name,
                                                 ExprPtr arena,
                                                 std::vector<StructInstantiation::FieldValue> values)
            : Expression(loc), m_struct_name(std::move(name)),
              m_arena(std::move(arena)), m_field_values(std::move(values)) {}

        Array::Array(Location loc, std::vector<ExprPtr> members)
            : Expression(loc), m_members(std::move(members)), m_size(m_members.size()) {}

        ArrayAccess::ArrayAccess(Location loc, ExprPtr array_expr, ExprPtr index_expr)
            : Expression(loc), m_array_expr(std::move(array_expr)),
              m_index_expr(std::move(index_expr)) {}

        bool StatementBlock::is_empty() const { return m_statements.empty(); }
    }
} // namespace aloha
