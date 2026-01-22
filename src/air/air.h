#ifndef AIR_AIR_H_
#define AIR_AIR_H_

#include "decl.h"
#include "visitor.h"
#include "../ty/ty.h"
#include "../frontend/location.h"
#include <memory>
#include <string>
#include <vector>

namespace AIR
{

  class Node
  {
  public:
    Location loc;

    explicit Node(const Location &loc) : loc(loc) {}
    virtual ~Node() = default;

    virtual void accept(AIRVisitor &visitor) = 0;
  };

  class Expr : public Node
  {
  public:
    TyId ty;

    Expr(const Location &loc, TyId ty) : Node(loc), ty(ty) {}
    virtual ~Expr() = default;
  };

  class Stmt : public Node
  {
  public:
    explicit Stmt(const Location &loc) : Node(loc) {}
    virtual ~Stmt() = default;
  };

  using ExprPtr = std::unique_ptr<Expr>;
  using StmtPtr = std::unique_ptr<Stmt>;
  using FunctionPtr = std::unique_ptr<Function>;
  using StructDeclPtr = std::unique_ptr<StructDecl>;

} // namespace AIR

#endif // AIR_AIR_H_
