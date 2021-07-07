#include "common.h"
#include "test_runner_p.h"


inline std::ostream& operator<<(std::ostream& output, Position pos) {
    return output << "(" << pos.row << ", " << pos.col << ")";
}

inline Position operator"" _pos(const char* str, std::size_t) {
    return Position::FromString(str);
}

inline std::ostream& operator<<(std::ostream& output, Size size) {
    return output << "(" << size.rows << ", " << size.cols << ")";
}

inline std::ostream& operator<<(std::ostream& output, const CellInterface::Value& value) {
    std::visit(
        [&](const auto& x) {
            output << x;
        },
        value);
    return output;
}

namespace {

    void TestEmpty() {
        auto sheet = CreateSheet();
        ASSERT_EQUAL(sheet->GetPrintableSize(), (Size{ 0, 0 }));
    }

    void TestInvalidPosition() {
        auto sheet = CreateSheet();
        try {
            sheet->SetCell(Position{ -1, 0 }, "");
        }
        catch (const InvalidPositionException&) {
        }
        try {
            sheet->GetCell(Position{ 0, -2 });
        }
        catch (const InvalidPositionException&) {
        }
        try {
            sheet->ClearCell(Position{ Position::MAX_ROWS, 0 });
        }
        catch (const InvalidPositionException&) {
        }
    }

    void TestSetCellPlainText() {
        auto sheet = CreateSheet();

        auto checkCell = [&](Position pos, std::string text) {
            sheet->SetCell(pos, text);
            CellInterface* cell = sheet->GetCell(pos);
            ASSERT(cell != nullptr);
            ASSERT_EQUAL(cell->GetText(), text);
            ASSERT_EQUAL(std::get<std::string>(cell->GetValue()), text);
        };

        checkCell("A1"_pos, "Hello");
        checkCell("A1"_pos, "World");
        checkCell("B2"_pos, "Purr");
        checkCell("A3"_pos, "Meow");

        const SheetInterface& constSheet = *sheet;
        ASSERT_EQUAL(constSheet.GetCell("B2"_pos)->GetText(), "Purr");

        sheet->SetCell("A3"_pos, "'=escaped");
        CellInterface* cell = sheet->GetCell("A3"_pos);
        ASSERT_EQUAL(cell->GetText(), "'=escaped");
        ASSERT_EQUAL(std::get<std::string>(cell->GetValue()), "=escaped");
    }

    void TestClearCell() {
        auto sheet = CreateSheet();

        sheet->SetCell("C2"_pos, "Me gusta");
        sheet->ClearCell("C2"_pos);
        ASSERT(sheet->GetCell("C2"_pos) == nullptr);

        sheet->ClearCell("A1"_pos);
        sheet->ClearCell("J10"_pos);
    }
    void TestPrint() {
        auto sheet = CreateSheet();
        sheet->SetCell("A2"_pos, "meow");
        sheet->SetCell("B2"_pos, "=1+2");
        sheet->SetCell("A1"_pos, "=1/0");

        ASSERT_EQUAL(sheet->GetPrintableSize(), (Size{ 2, 2 }));

        std::ostringstream texts;
        sheet->PrintTexts(texts);
        std::cout << texts.str() << std::endl;

        ASSERT_EQUAL(texts.str(), "=1/0\t\nmeow\t=1+2\n");

        std::ostringstream values;
        sheet->PrintValues(values);
        ASSERT_EQUAL(values.str(), "#DIV/0!\t\nmeow\t3\n");

        sheet->ClearCell("B2"_pos);
        ASSERT_EQUAL(sheet->GetPrintableSize(), (Size{ 2, 1 }));
    }

    void TestCellsInFormula() {
        auto sheet = CreateSheet();
        sheet->SetCell("A1"_pos, "=A2 + 3");
        sheet->SetCell("B1"_pos, "=A1 * 3");
        sheet->SetCell("C1"_pos, "=B1 / C10");

        std::ostringstream values;
        sheet->PrintValues(values);
    }

    void TestCurcularDependecy() {
        auto sheet = CreateSheet();
        sheet->SetCell("A2"_pos, "3");
        sheet->SetCell("C2"_pos, "=A3 / A2");
        sheet->SetCell("C4"_pos, "=C2 + 8");

        try {
            sheet->SetCell("A3"_pos, "=C4 - 1");
        }

        catch (CircularDependencyException& e) {
            std::cout << e.what() << std::endl;
        }

        std::ostringstream values;
        std::ostringstream texts;
        sheet->PrintValues(values);
        ASSERT_EQUAL(values.str(), "\t\t\t\n3\t\t0\n0\t\t\n\t\t8\n");

        sheet->PrintTexts(texts);
        ASSERT_EQUAL(texts.str(), "\t\t\t\n3\t\t=A3/A2\n\t\t\n\t\t=C2+8\n");

    }   

    void TestInvalidateCache() {
        auto sheet = CreateSheet();
        sheet->SetCell("A2"_pos, "3");
        sheet->SetCell("C2"_pos, "=A3 / A2");
        sheet->SetCell("C4"_pos, "=C2 + 8");
       
        std::ostringstream values;

        sheet->PrintValues(values);
        ASSERT_EQUAL(values.str(), "\t\t\t\n3\t\t0\n0\t\t\n\t\t8\n");
        sheet->SetCell("C2"_pos, "=20 / A2");
        sheet->SetCell("A2"_pos, "100");

        std::ostringstream values1;

        sheet->PrintValues(values1);
        ASSERT_EQUAL(values1.str(), "\t\t\t\n100\t\t0.2\n0\t\t\n\t\t8.2\n");
    }

    void TestPrintExceptionValue() {
        auto sheet = CreateSheet();
        sheet->SetCell("A1"_pos, "text");
        sheet->SetCell("B1"_pos, "=100");
        sheet->SetCell("C1"_pos, "=A1 / B1");
        sheet->SetCell("D1"_pos, "=C1 - 20");

        std::ostringstream values;

        sheet->PrintValues(values);
        ASSERT_EQUAL(values.str(), "text\t100\t#VALUE!\t#VALUE!\n");
    }

    void TestPrintExceptionRef() {
        auto sheet = CreateSheet();
        sheet->SetCell("A1"_pos, "250");
        sheet->SetCell("B1"_pos, "=100");
        sheet->SetCell("C1"_pos, "=A1 / B1 + ZZZ2");
        std::ostringstream values;

        sheet->PrintValues(values);
        ASSERT_EQUAL(values.str(), "250\t100\t#REF!\n");
    }

    void TestPrintInvalidFormula() {
        auto sheet = CreateSheet();
        sheet->SetCell("A1"_pos, "=ZZZ2");
        sheet->SetCell("A2"_pos, "=A1+*");

        //sheet->SetCell("B1"_pos, "=100");
        //sheet->SetCell("C1"_pos, "=A1 / B1 + ZZZ2");
        sheet->GetCell({ 50000, 65000 });
        std::ostringstream values;

        sheet->PrintValues(std::cout);
        //ASSERT_EQUAL(values.str(), "250\t100\t#REF!\n");
    }

}  // namespace



int main() {
    TestRunner tr;
    RUN_TEST(tr, TestEmpty);
    RUN_TEST(tr, TestInvalidPosition);
    RUN_TEST(tr, TestSetCellPlainText);
    RUN_TEST(tr, TestClearCell);
    RUN_TEST(tr, TestPrint);
    RUN_TEST(tr, TestCellsInFormula);
    RUN_TEST(tr, TestCurcularDependecy);
    RUN_TEST(tr, TestInvalidateCache);
    RUN_TEST(tr, TestPrintExceptionValue);
    RUN_TEST(tr, TestPrintExceptionRef);
    RUN_TEST(tr, TestPrintInvalidFormula);
    return 0;
}