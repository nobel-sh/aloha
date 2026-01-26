#include "ast.h"

namespace aloha
{

    void StatementBlock::accept(ASTVisitor &visitor) { visitor.visit(this); }
    void ExpressionStatement::accept(ASTVisitor &visitor) { visitor.visit(this); }
    void Integer::accept(ASTVisitor &visitor) { visitor.visit(this); }
    void Float::accept(ASTVisitor &visitor) { visitor.visit(this); }
    void Boolean::accept(ASTVisitor &visitor) { visitor.visit(this); }
    void String::accept(ASTVisitor &visitor) { visitor.visit(this); }
    void UnaryExpression::accept(ASTVisitor &visitor) { visitor.visit(this); }
    void BinaryExpression::accept(ASTVisitor &visitor) { visitor.visit(this); }
    void Identifier::accept(ASTVisitor &visitor) { visitor.visit(this); }
    void StructFieldAccess::accept(ASTVisitor &visitor) { visitor.visit(this); }
    void StructFieldAssignment::accept(ASTVisitor &visitor) { visitor.visit(this); }
    void Declaration::accept(ASTVisitor &visitor) { visitor.visit(this); }
    void Assignment::accept(ASTVisitor &visitor) { visitor.visit(this); }
    void FunctionCall::accept(ASTVisitor &visitor) { visitor.visit(this); }
    void ReturnStatement::accept(ASTVisitor &visitor) { visitor.visit(this); }
    void IfStatement::accept(ASTVisitor &visitor) { visitor.visit(this); }
    void WhileLoop::accept(ASTVisitor &visitor) { visitor.visit(this); }
    void ForLoop::accept(ASTVisitor &visitor) { visitor.visit(this); }
    void Function::accept(ASTVisitor &visitor) { visitor.visit(this); }
    void StructDecl::accept(ASTVisitor &visitor) { visitor.visit(this); }
    void StructInstantiation::accept(ASTVisitor &visitor) { visitor.visit(this); }
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

    String::String(Location loc, std::string val)
        : Expression(loc), m_value(std::move(val)) {}

    UnaryExpression::UnaryExpression(Location loc, std::string oper, ExprPtr expr)
        : Expression(loc), m_op(std::move(oper)), m_expr(std::move(expr)) {}

    BinaryExpression::BinaryExpression(Location loc, ExprPtr lhs, std::string oper,
                                       ExprPtr rhs)
        : Expression(loc), m_left(std::move(lhs)), m_op(std::move(oper)),
          m_right(std::move(rhs)) {}

    Identifier::Identifier(Location loc, std::string name)
        : Expression(loc), m_name(std::move(name)) {}

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

    FunctionCall::FunctionCall(Location loc, std::unique_ptr<Identifier> func_name,
                               std::vector<ExprPtr> args)
        : Expression(loc), m_func_name(std::move(func_name)),
          m_arguments(std::move(args)) {}

    ReturnStatement::ReturnStatement(Location loc, ExprPtr expr)
        : Statement(loc), m_expression(std::move(expr)) {}

    IfStatement::IfStatement(Location loc, ExprPtr cond,
                             std::unique_ptr<StatementBlock> then_branch,
                             std::unique_ptr<StatementBlock> else_branch)
        : Statement(loc), m_condition(std::move(cond)),
          m_then_branch(std::move(then_branch)),
          m_else_branch(std::move(else_branch)) {}
    bool IfStatement::has_else_branch() const { return m_else_branch != nullptr; }

    WhileLoop::WhileLoop(Location loc, ExprPtr cond,
                         std::unique_ptr<StatementBlock> body)
        : Statement(loc), m_condition(std::move(cond)), m_body(std::move(body)) {}

    ForLoop::ForLoop(Location loc, std::unique_ptr<Declaration> init, ExprPtr cond,
                     std::unique_ptr<Declaration> inc, std::vector<StmtPtr> body)
        : Statement(loc), m_initializer(std::move(init)),
          m_condition(std::move(cond)), m_increment(std::move(inc)),
          m_body(std::move(body)) {}

    Parameter::Parameter(std::string name, Type type)
        : m_name(std::move(name)), m_type(type) {}

    Parameter::Parameter(std::string name, Type type, std::string type_name)
        : m_name(std::move(name)), m_type(type) {}

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
        : m_name(std::move(name)), m_type(type) {}

    StructField::StructField(std::string name, Type type, std::string type_name)
        : m_name(std::move(name)), m_type(type) {}

    StructDecl::StructDecl(Location loc, std::string name,
                           std::vector<StructField> fields)
        : Statement(loc), m_name(std::move(name)), m_fields(std::move(fields)) {}

    StructInstantiation::StructInstantiation(Location loc, std::string name,
                                             std::vector<ExprPtr> values)
        : Expression(loc), m_struct_name(std::move(name)),
          m_field_values(std::move(values)) {}

    Array::Array(Location loc, std::vector<ExprPtr> members)
        : Expression(loc), m_members(std::move(members)), m_size(m_members.size()) {}

    ArrayAccess::ArrayAccess(Location loc, ExprPtr array_expr, ExprPtr index_expr)
        : Expression(loc), m_array_expr(std::move(array_expr)),
          m_index_expr(std::move(index_expr)) {}

    bool StatementBlock::is_empty() const { return m_statements.empty(); }

} // namespace aloha