// ===========================================================================
//  out-txt.h  --  plain-text / CSV table writers for the analysis layer
//  Part of ECFlow (migrated & cleaned in v4 from stable out-txt.h / out-excel.h).
//  Convention: string-table + optional sub-header, one delimiter per column.
//    - writeTableToTxt : tab-separated  (.txt)
//    - writeTableToCSV : comma-separated (.csv)   [moved here from out-excel.h]
//  Layout of a table:  header[optimizer] x subheader[metric] grid,
//  rowtitles[problem] on the left, tabledata[row][col] the cells.
// ===========================================================================
#pragma once
#include <string>
#include <fstream>
#include <vector>

namespace FileOut
{
    // Emit the header/subheader/body table using `delim` as the column separator.
    inline bool _writeDelimited(const std::vector<std::string>& headers,
        const std::vector<std::string>& subheaders,
        const std::vector<std::string>& rowtitles,
        const std::vector<std::vector<std::string>>& tabledata,
        const std::string& filepath,
        const char* delim)
    {
        std::ofstream out(filepath);
        if (!out.is_open())
            return false;

        const int header_n = static_cast<int>(headers.size());
        const int sub_n = static_cast<int>(subheaders.size());
        const int columns = (sub_n == 0) ? header_n : header_n * sub_n;

        // top header: each optimizer name spans its sub-columns
        const int span = (header_n == 0) ? 0 : (columns - header_n) / header_n;
        out << delim;
        for (int col = 0; col < header_n; ++col)
        {
            out << headers[col];
            for (int i = 0; i < span; ++i)
                out << delim;
            if (col < header_n - 1)
                out << delim;
        }
        out << "\n";

        // sub-header: metric names repeated under each optimizer
        if (sub_n != 0)
        {
            out << delim;
            for (int col = 0; col < header_n; ++col)
            {
                for (int sub = 0; sub < sub_n; ++sub)
                {
                    out << subheaders[sub];
                    if (sub < sub_n - 1)
                        out << delim;
                }
                if (col < header_n - 1)
                    out << delim;
            }
            out << "\n";
        }

        // body: one row per problem (+ trailing summary rows: Rank / +/=/- ...)
        for (int row = 0; row < static_cast<int>(rowtitles.size()); ++row)
        {
            out << rowtitles[row] << delim;
            if (tabledata[row].empty())      // missing data for this row
            {
                out << "\n";
                continue;
            }
            const int cells = static_cast<int>(tabledata[row].size());
            const int cell_span = (columns - cells) / cells;
            for (int col = 0; col < cells; ++col)
            {
                out << tabledata[row][col];
                for (int i = 0; i < cell_span; ++i)
                    out << delim;
                if (col < cells - 1)
                    out << delim;
            }
            out << "\n";
        }

        out.close();
        return true;
    }

    // Tab-separated text table.
    inline bool writeTableToTxt(const std::vector<std::string>& headers,
        const std::vector<std::string>& subheaders,
        const std::vector<std::string>& rowtitles,
        const std::vector<std::vector<std::string>>& tabledata,
        const std::string& filepath)
    {
        return _writeDelimited(headers, subheaders, rowtitles, tabledata, filepath, "\t");
    }

    // Comma-separated table (spreadsheet-friendly).
    inline bool writeTableToCSV(const std::vector<std::string>& headers,
        const std::vector<std::string>& subheaders,
        const std::vector<std::string>& rowtitles,
        const std::vector<std::vector<std::string>>& tabledata,
        const std::string& filepath)
    {
        return _writeDelimited(headers, subheaders, rowtitles, tabledata, filepath, ",");
    }
}
