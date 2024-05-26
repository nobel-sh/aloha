#include "ast.h"

// Number
void Number::write(std::ostream& os, int indent) const {
    os << std::string(indent, ' ') << "Number: " << value << std::endl;
}

// BinaryExpression
void BinaryExpression::write(std::ostream& os, int indent) const {
    os << std::string(indent, ' ') << "Binary Expression: " << op << std::endl;
    left->write(os, indent + 2);
    right->write(os, indent + 2);
}

// Identifier
void Identifier::write(std::ostream& os, int indent) const {
    os << std::string(indent, ' ') << "Identifier: " << name << std::endl;
}

// Assignment
void Assignment::write(std::ostream& os, int indent) const {
    os << std::string(indent, ' ') << "Assignment: " << variableName << std::endl;
    expression->write(os, indent + 2);
}

// FunctionCall
void FunctionCall::write(std::ostream& os, int indent) const {
    os << std::string(indent, ' ') << "Function Call: " << functionName << std::endl;
    for (const auto& arg : arguments) {
        arg->write(os, indent + 2);
    }
}

// ReturnStatement
void ReturnStatement::write(std::ostream& os, int indent) const {
    os << std::string(indent, ' ') << "Return Statement:" << std::endl;
    expression->write(os, indent + 2);
}

// IfStatement
void IfStatement::write(std::ostream& os, int indent) const {
    os << std::string(indent, ' ') << "If Statement:" << std::endl;
    os << std::string(indent, ' ') << "Condition:" << std::endl;
    condition->write(os, indent + 2);
    os << std::string(indent, ' ') << "Then Branch:" << std::endl;
    for (const auto& stmt : thenBranch) {
        stmt->write(os, indent + 2);
    }
    os << std::string(indent, ' ') << "Else Branch:" << std::endl;
    for (const auto& stmt : elseBranch) {
        stmt->write(os, indent + 2);
    }
}

// WhileLoop
void WhileLoop::write(std::ostream& os, int indent) const {
    os << std::string(indent, ' ') << "While Loop:" << std::endl;
    os << std::string(indent, ' ') << "Condition:" << std::endl;
    condition->write(os, indent + 2);
    os << std::string(indent, ' ') << "Body:" << std::endl;
    for (const auto& stmt : body) {
        stmt->write(os, indent + 2);
    }
}

// ForLoop
void ForLoop::write(std::ostream& os, int indent) const {
    os << std::string(indent, ' ') << "For Loop:" << std::endl;
    os << std::string(indent, ' ') << "Initializer:" << std::endl;
    initializer->write(os, indent + 2);
    os << std::string(indent, ' ') << "Condition:" << std::endl;
    condition->write(os, indent + 2);
    os << std::string(indent, ' ') << "Increment:" << std::endl;
    increment->write(os, indent + 2);
    os << std::string(indent, ' ') << "Body:" << std::endl;
    for (const auto& stmt : body) {
        stmt->write(os, indent + 2);
    }
}

// Function
void Function::write(std::ostream& os, int indent) const {
    os << std::string(indent, ' ') << "Function: " << name << std::endl;
    os << std::string(indent, ' ') << "Parameters:" << std::endl;
    for (const auto& param : parameters) {
        os << std::string(indent + 2, ' ') << param.first << ": " << param.second << std::endl;
    }
    os << std::string(indent, ' ') << "Return Type: " << returnType << std::endl;
    os << std::string(indent, ' ') << "Body:" << std::endl;
    for (const auto& stmt : body) {
        stmt->write(os, indent + 2);
    }
}

// Program
void Program::write(std::ostream& os, int indent) const {
    os << std::string(indent, ' ') << "Program:" << std::endl;
    for (const auto& node : nodes) {
        node->write(os, indent + 2);
    }
}
