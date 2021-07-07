#pragma once

#include "common.h"
#include "formula.h"

#include <optional>
#include <functional>
#include <unordered_set>

class Sheet;

class CellImpl {
public:
	virtual CellInterface::Value ImplGetValue() const = 0;
	virtual std::string ImplGetText() const = 0;
	virtual std::vector<Position> GetReferencedCells() const = 0;
	virtual bool isCacheValid() const {
		return true;
	}
	virtual void ResetCache() {
		return;
	}

	virtual ~CellImpl() = default;

};

class CellEmptyImpl : public CellImpl {
public:
	CellEmptyImpl() {
		empty_ = "";
	}

	CellInterface::Value ImplGetValue() const override {
		return 0.0;
	}

	std::string ImplGetText() const override {
		return empty_;
	}

	std::vector<Position> GetReferencedCells() const override {
		return {};
	}

private:
	std::string empty_;
};

class CellTextImpl : public CellImpl {
public:
	CellTextImpl(std::string& text)
		:text_value_(std::move(text))
	{
		if (text_value_[0] == ESCAPE_SIGN) {
			has_apostroph_ = true;
		}
	}

	CellInterface::Value ImplGetValue() const override {
		if (has_apostroph_) {
			std::string sub_str = text_value_;
			sub_str.erase(0, 1);
			return sub_str;
		}

		return text_value_;
	}

	std::string ImplGetText() const override {
		return text_value_;
	}

	std::vector<Position> GetReferencedCells() const override {
		return {};
	}

private:
	std::string text_value_;
	bool has_apostroph_ = false;
};


class CellFormulaImpl : public CellImpl {
public:
	CellFormulaImpl(std::string& expr, SheetInterface& sheet)
		:formula_(ParseFormula(expr)),
		 sheet_(sheet)
	{
	}

	CellInterface::Value ImplGetValue() const override {
		
		if (!cache_value_) {
			const auto val = formula_->Evaluate(sheet_);

			if (std::holds_alternative<double>(val)) {
				cache_value_ = std::get<double>(val);
			}
			else if (std::holds_alternative<FormulaError>(val)) {
				cache_value_ = std::get<FormulaError>(val);
			}
			//return 0.0;

			//cache_value_ = std::visit(CellCacheValueGetter{}, formula_->Evaluate(sheet_));
		}

		return *cache_value_;
	}

	std::string ImplGetText() const override {
		return FORMULA_SIGN + formula_.get()->GetExpression();
	}

	std::vector<Position> GetReferencedCells() const override {
		return formula_.get()->GetReferencedCells();
	}

	bool isCacheValid() const override {
		return cache_value_.has_value();
	}

	void ResetCache() override {
		cache_value_.reset();
	}

private:
	std::unique_ptr<FormulaInterface> formula_;
	SheetInterface& sheet_;
	mutable std::optional<CellInterface::Value> cache_value_;
};


class Cell : public CellInterface {
public:
	using CellPtr = std::unique_ptr<Cell>;

    Cell(SheetInterface& sheet, std::string text) 
        :sheet_(sheet)
    {
		cell_value_ = std::make_unique<CellEmptyImpl>();
		Set(text);
    };

    ~Cell();

    void Set(std::string text);
    void Clear();

    Value GetValue() const override;
    std::string GetText() const override;

    std::vector<Position> GetReferencedCells() const override;

    bool hasCircularDependency(const Cell* main_cell, const Position& pos) const;
	void InvalidateCache();


private:
    SheetInterface& sheet_;
    std::unique_ptr<CellImpl> cell_value_;
};