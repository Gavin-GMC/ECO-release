// ===========================================================================
//  out-latex.h  --  LaTeX table writer for the analysis layer
//  Part of ECFlow (migrated & cleaned in v4 from stable out-latex.h).
//  Emits a standalone LaTeX document with a booktabs-style / full-grid table.
//  `bolddata[row]` lists the flat cell indices (o*subcols + metric) to \textbf.
// ===========================================================================
#pragma once
#include <string>
#include <fstream>
#include <vector>

namespace FileOut
{
    inline bool writeTableToLaTeX(const std::vector<std::string>& headers,
        const std::vector<std::string>& subheaders,
        const std::vector<std::string>& rowtitles,
        const std::vector<std::vector<std::string>>& tabledata,
        const std::string& filepath,
        const std::vector<std::vector<int>>& bolddata,
        bool ThreeLine = true)
    {
        std::ofstream out(filepath);
        if (!out.is_open())
            return false;

        const int sub_n = (subheaders.empty()) ? 1 : static_cast<int>(subheaders.size());
        const int header_n = static_cast<int>(headers.size());
        const int max_columns = sub_n * header_n;

        // preamble
        out << "\\documentclass{article}\n";
        out << "\\usepackage[utf8]{inputenc}\n";
        out << "\\usepackage{geometry}\n";
        out << "\\usepackage{multirow}\n";
        out << "\\geometry{a4paper, margin=1in}\n";
        out << "\\begin{document}\n\n";

        out << "\\begin{table}[ht]\n\\centering\n";

        // column format
        std::string format = "|c|";
        if (ThreeLine)
        {
            const std::string block = std::string(sub_n, 'c') + "|";
            for (int i = 0; i < header_n; ++i)
                format += block;
        }
        else
        {
            for (int i = 0; i < max_columns; ++i)
                format += "c|";
        }
        out << "\\begin{tabular}{" << format << "}\n\\hline\n";

        // header (with optional multicolumn sub-header)
        if (subheaders.empty())
        {
            out << " & ";
            for (int col = 0; col < header_n; ++col)
            {
                out << headers[col];
                if (col < header_n - 1) out << " & ";
            }
            out << " \\\\ \\hline\n";
        }
        else
        {
            out << "\\multirow{2}{*}{ } & ";
            for (int col = 0; col < header_n; ++col)
            {
                out << "\\multicolumn{" << subheaders.size() << "}{c|}{" << headers[col] << "}";
                if (col < header_n - 1) out << " & ";
            }
            out << " \\\\ \\cline{2-" << std::to_string(1 + max_columns) << "}\n";

            out << " & ";
            for (int col = 0; col < header_n; ++col)
            {
                for (int sub = 0; sub < static_cast<int>(subheaders.size()); ++sub)
                {
                    out << subheaders[sub];
                    if (sub < static_cast<int>(subheaders.size()) - 1) out << " & ";
                }
                if (col < header_n - 1) out << " & ";
            }
            out << " \\\\ \\hline\n";
        }

        // body
        for (int row = 0; row < static_cast<int>(rowtitles.size()); ++row)
        {
            const int cells = static_cast<int>(tabledata[row].size());
            std::vector<bool> bold(cells, false);
            if (row < static_cast<int>(bolddata.size()))
                for (int idx : bolddata[row])
                    if (idx >= 0 && idx < cells)
                        bold[idx] = true;

            out << rowtitles[row] << " & ";

            if (cells != max_columns)     // summary row: spread cells across the grid
            {
                const int merges = (cells == 0) ? 1 : max_columns / cells;
                for (int col = 0; col < cells; ++col)
                {
                    out << "\\multicolumn{" << std::to_string(merges) << "}{c|}{" << tabledata[row][col] << "}";
                    if (col < cells - 1) out << " & ";
                }
            }
            else
            {
                for (int col = 0; col < cells; ++col)
                {
                    out << (bold[col] ? "\\textbf{" + tabledata[row][col] + "}" : tabledata[row][col]);
                    if (col < cells - 1) out << " & ";
                }
            }

            const bool last = (row == static_cast<int>(rowtitles.size()) - 1);
            if (last || !ThreeLine || cells != static_cast<int>(tabledata[row + 1].size()))
                out << " \\\\ \\hline\n";
            else
                out << " \\\\ \n";
        }

        out << "\\end{tabular}\n";
        out << "\\caption{Table}\n";
        out << "\\label{table:example}\n";
        out << "\\end{table}\n\n";
        out << "\\end{document}\n";

        out.close();
        return true;
    }
}
