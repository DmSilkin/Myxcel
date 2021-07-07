#include "sheet.h"

#include "cell.h"
#include "common.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include <optional>

using namespace std::literals;

Sheet::~Sheet() {}

void Sheet::SetCell(Position pos, std::string text) {
    if (!pos.IsValid()) {
        throw InvalidPositionException("Out of range");
    }

    const auto existing_cell = GetCell(pos);
    if (existing_cell && existing_cell->GetText() == text) {
        return;
    }

    if (existing_cell) {
        std::string old_text = dynamic_cast<Cell*>(existing_cell)->GetText();
        InvalidateCellsByPos(pos);
        DeleteDependencedCell(pos);
        dynamic_cast<Cell*>(existing_cell)->Set(std::move(text));
        if (dynamic_cast<Cell*>(existing_cell)->hasCircularDependency(dynamic_cast<Cell*>(existing_cell), pos)) {
            dynamic_cast<Cell*>(existing_cell)->Set(std::move(old_text));
            throw CircularDependencyException("Circular Exception!");
        }

        for (const auto ref_pos : dynamic_cast<Cell*>(existing_cell)->GetReferencedCells()) {
            AddDependencedCell(ref_pos, pos);
        }
    }
    else {
        ResizeSheet(pos);

        auto tmp_cell = std::make_unique<Cell>(*this, text);

        if (tmp_cell.get()->hasCircularDependency(tmp_cell.get(), pos)) {
            throw CircularDependencyException("Circular Exception!");
        }

        for (const auto ref_pos : tmp_cell.get()->GetReferencedCells()) {
            AddDependencedCell(ref_pos, pos);
        }

        sheet_[pos.row][pos.col] = std::move(tmp_cell);
    }
}

const CellInterface* Sheet::GetCell(Position pos) const {
    if (!pos.IsValid()) {
        throw InvalidPositionException("Out of range");
    }

    try {
        return sheet_.at(pos.row).at(pos.col).get();
    }
    catch (...) {
        return nullptr;
        //return std::make_unique<Cell>(*const_cast<Sheet*>(this)).get();
    }


}
CellInterface* Sheet::GetCell(Position pos) {
    if (!pos.IsValid()) {
        throw InvalidPositionException("Out of range");
    }

    try {
        return sheet_.at(pos.row).at(pos.col).get();
    }
    catch (std::out_of_range&) {
        return nullptr;
    }
}

void Sheet::ClearCell(Position pos) {
    if (!pos.IsValid()) {
        throw InvalidPositionException("Out of range");
    }

    try {
        sheet_.at(pos.row).at(pos.col).reset();
    }
    catch (std::out_of_range&) {
        return;
    }
}

Size Sheet::GetPrintableSize() const {
    int max_row = 0;
    int max_col = 0;

    int cur_row = 1;
    for (const auto& row : sheet_) {
        int cur_col = 1;
        
        for (const auto& col : row) {
            if (col != nullptr) {
                if (cur_col > max_col) {
                    max_col = cur_col;
                }
                max_row = cur_row;
            }
            ++cur_col;
        }

        ++cur_row;
    }
    
    return Size{max_row, max_col};
}

void Sheet::PrintValues(std::ostream& output) const {
    for (int i = 0; i < GetPrintableSize().rows; i++) {
        for (size_t j = 0; j < sheet_.at(i).size(); j++) {
            if (sheet_.at(i).at(j) != nullptr) {
                std::visit([&output](const auto& value) { output << value; }, sheet_.at(i).at(j).get()->GetValue());
            }

            if (j != sheet_.at(i).size() - 1) {
                output << '\t';
            }
        }
        for (int k = sheet_.at(i).size(); k < GetPrintableSize().cols; k++) {
            output << '\t';
        }
        output << '\n';
    }
}
void Sheet::PrintTexts(std::ostream& output) const {
    for (int i = 0; i < GetPrintableSize().rows; i++) {
        for (size_t j = 0; j < sheet_.at(i).size(); j++) {
            if (sheet_.at(i).at(j) != nullptr) {
                 output << sheet_.at(i).at(j).get()->GetText();
            }
            if (j != sheet_.at(i).size() - 1) {
                output << '\t';
            }
        }
        for (int k = sheet_.at(i).size(); k < GetPrintableSize().cols; k++) {
            output << '\t';
        }
        output << '\n';
    }
}

std::unique_ptr<SheetInterface> CreateSheet() {
    return std::make_unique<Sheet>();
}

void Sheet::ResizeSheet(Position pos) {
    if (static_cast<size_t>(pos.row) >= sheet_.size()) {
        if (pos.row + 1 > Position::MAX_ROWS) {
            throw TableTooBigException("Too big");
        }
        else {
            sheet_.resize(pos.row + 1);
        }
    }


    if (static_cast<size_t>(pos.col) >= sheet_.at(pos.row).size()) {
        if (pos.col + 1 > Position::MAX_COLS) {
            throw TableTooBigException("Too big");
        }
        else {
            sheet_.at(pos.row).resize(pos.col + 1);
        }
    }
}

const std::unordered_map<Position, std::unordered_set<Position, SheetHasher>, SheetHasher> Sheet::GetSheetDepCells() const {
    return sheet_dependenced_cells_;
}


void Sheet::AddDependencedCell(const Position& pos_key, const Position& pos_value) {
    sheet_dependenced_cells_[pos_key].insert(pos_value);
}

void Sheet::DeleteDependencedCell(const Position& pos) {
    sheet_dependenced_cells_.erase(pos);
}

const std::unordered_set <Position, SheetHasher> Sheet::GetDepCellByPos(const Position& pos) const {
    try {
        return sheet_dependenced_cells_.at(pos);
    }

    catch (...) {
        return {};
    }
}

void Sheet::InvalidateCellsByPos(const Position& pos) {
    const auto dependenced_cells = GetDepCellByPos(pos);

    for (const auto cell_pos : dependenced_cells) {
        const auto cell = GetCell(cell_pos);
        dynamic_cast<Cell*>(cell)->InvalidateCache();

        InvalidateCellsByPos(cell_pos);
    }
}