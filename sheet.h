#pragma once

#include "cell.h"
#include "common.h"

#include <functional>

class SheetHasher {
public:
    size_t operator()(Position pos) const {
        return hasher_(pos.col) + hasher_(pos.row) * 1000;
    }
private:
    std::hash<double> hasher_;
};

class Sheet : public SheetInterface {
public:
    Sheet()
    {
        sheet_.reserve(Position::MAX_ROWS - 1);
    }

    ~Sheet();

    using CellPtr = std::unique_ptr<Cell>;

    void SetCell(Position pos, std::string text) override;

    const CellInterface* GetCell(Position pos) const override;
    CellInterface* GetCell(Position pos) override;

    void ClearCell(Position pos) override;

    Size GetPrintableSize() const override;

    void PrintValues(std::ostream& output) const override;
    void PrintTexts(std::ostream& output) const override;


    void InvalidateCellsByPos(const Position& pos);
    const std::unordered_map<Position, std::unordered_set<Position, SheetHasher>, SheetHasher> GetSheetDepCells() const;
    const std::unordered_set <Position, SheetHasher> GetDepCellByPos(const Position& pos) const;
    void AddDependencedCell(const Position& pos_key, const Position& pos_value);
    void DeleteDependencedCell(const Position& pos);

    // Можете дополнить ваш класс нужными полями и методами


private:
    std::vector<std::vector<CellPtr>> sheet_;
    std::unordered_map<Position, std::unordered_set<Position, SheetHasher>, SheetHasher> sheet_dependenced_cells_;
    void ResizeSheet(Position pos);

    // Можете дополнить ваш класс нужными полями и методами
};