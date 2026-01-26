#ifndef AIR_PRINTER_H_
#define AIR_PRINTER_H_

#include "air.h"
#include "expr.h"
#include "stmt.h"
#include "visitor.h"
#include "../ty/ty.h"
#include <ostream>
#include <string>

namespace AIR
{

    class Printer : public AIRVisitor
    {
    public:
        explicit Printer(std::ostream &os, const TyTable *ty_table = nullptr);

        void print(Module *module);

        // expression visitors
        void visit(IntegerLiteral *node) override;
        void visit(FloatLiteral *node) override;
        void visit(StringLiteral *node) override;
        void visit(BoolLiteral *node) override;
        void visit(VarRef *node) override;
        void visit(BinaryOp *node) override;
        void visit(UnaryOp *node) override;
        void visit(Call *node) override;
        void visit(StructInstantiation *node) override;
        void visit(FieldAccess *node) override;
        void visit(ArrayExpr *node) override;

        // statement visitors
        void visit(VarDecl *node) override;
        void visit(Assignment *node) override;
        void visit(FieldAssignment *node) override;
        void visit(Return *node) override;
        void visit(If *node) override;
        void visit(ExprStmt *node) override;

        // declaration visitors
        void visit(Function *node) override;
        void visit(StructDecl *node) override;
        void visit(Module *node) override;

    private:
        std::ostream &os;
        const TyTable *ty_table;
        unsigned long indent;

        void write_indent();
        std::string ty_name(TyId ty) const;
    };

} // namespace AIR

#endif // AIR_PRINTER_H_
