#include "formula.h"
#include "FormulaAST.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <sstream>

using namespace std::literals;

std::ostream& operator<<(std::ostream& output, FormulaError fe) {
    FormulaError::Category cat = fe.GetCategory();
    std::string val = "";
    switch (cat) {
    case FormulaError::Category::Div0:
        val = "#DIV/0!";
        break;
    case FormulaError::Category::Ref:
        val = "#REF!";
        break;
    case FormulaError::Category::Value:
        val = "#VALUE!";
        break;
    default:
        break;
    }

    return output << val;
}

namespace {
    class Formula : public FormulaInterface {
    public:
        // Реализуйте следующие методы:
        explicit Formula(std::string expression)
            :ast_(ParseFormulaAST(std::move(expression))),
             referenced_cells_(ast_.GetCells().begin(), ast_.GetCells().end())
        {
        
;       }

        Value Evaluate(const SheetInterface& sheet) const override{

            try {
                return ast_.Execute(
                    [&sheet](const Position& pos) {
                        if (sheet.GetCell(pos) == nullptr) {
                            return 0.0;
                        }
                        auto val = sheet.GetCell(pos)->GetValue();

                        if (std::holds_alternative<double>(val)) {
                            return std::get<double>(val);
                        }
                        else if (std::holds_alternative<std::string>(val)) {
                            try {
                                return std::stod(std::move(std::get<std::string>(val)));
                            }
                            catch (...) {
                                throw FormulaError(FormulaError::Category::Value);
                            }
                        }
                        else {
                            throw std::get<FormulaError>(val);
                        }
                        return 0.0;
                        //return std::visit(CellValueGetter(), sheet.GetCell(pos) == nullptr ? 0.0 : sheet.GetCell(pos)->GetValue());
                        //Почему-то тренажер не пропускал решение в эту одну строчку
                    });
            }

            catch (FormulaError& fe) {
                return fe;
            }
        }

        std::string GetExpression() const override {
            std::stringstream out;
            ast_.PrintFormula(out);
            return out.str();
        }

        std::vector<Position> GetReferencedCells() const override {
            return referenced_cells_;
        }


    private:
        FormulaAST ast_;
        std::vector<Position> referenced_cells_;
    };
}  // namespace

std::unique_ptr<FormulaInterface> ParseFormula(std::string expression) {
    return std::make_unique<Formula>(std::move(expression));
}