#include <gtest/gtest.h>
#include "csv_export.cpp"
#include <fstream>

TEST(CsvExportTest, PartsExport) {
    // Prepare a simple part record
    PartSiteRecord rec;
    rec.part_code = "P001";
    rec.part_type = "TYPE_A";
    rec.on_hand = 100.0;
    rec.low_level_code = 5;
    rec.round_to_integer = true;
    std::vector<PartSiteRecord> parts = {rec};
    // Export to CSV
    csv_export_parts(parts);
    // Verify file exists and content
    std::ifstream ifs("parts.csv");
    ASSERT_TRUE(ifs.is_open());
    std::string line;
    std::getline(ifs, line);
    EXPECT_EQ(line, "part_code,part_type,on_hand,low_level_code,round_to_integer");
    std::getline(ifs, line);
    EXPECT_EQ(line, "P001,TYPE_A,100.000000,5,true");
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
