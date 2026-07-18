#include "variable.h"
#include "ExprTree.h"

using namespace mup;
using namespace ECFlow;

// =====================================================================
// 全局变量定义 (g_OperatorArity)
// =====================================================================
const std::unordered_map<std::string, int> g_OperatorArity = {
    {"+", 2}, {"-", 2}, {"*", 2}, {"/", 2}, {"^", 2},
    {"==", 2}, {"!=", 2}, {"<", 2}, {">", 2}, {"<=", 2}, {">=", 2},
    {"and", 2}, {"or", 2}, {"not", 1}
};

// =====================================================================
// CalculationTree 成员函数实现
// =====================================================================

ExpressionTree::ExpressionTree()
    : m_result(0.0)
{
    m_parser.EnableOptimizer(true); // 开启优化
}

bool ExpressionTree::Compile(const std::string& expr_text) {
    try {
        m_parser.SetExpr(string_type(expr_text.c_str()));

        // 强制编译以验证语法
        m_parser.Eval();
        return true;
    }
    catch (ParserError& e) {
        std::cerr << "Compile Error: " << e.GetMsg() << std::endl;
        return false;
    }
}

void ExpressionTree::clearLinked()
{
    m_parser.ClearVar();
    m_variableStorage.clear();
}

bool ExpressionTree::BatchLinkVariables(const ElementNote* notes, const double** data_ptrs, const int var_count) {
    try {
        clearLinked();
        m_variableStorage.resize(var_count);
        m_variableName.resize(var_count);
        
        for (int i = 0; i < var_count; ++i) {
            m_variableStorage[i] = BuildValueFromData(notes[i], nullptr);
            m_variableName[i] = notes[i]._name;
            m_parser.DefineVar(m_variableName[i], Variable(&m_variableStorage[i]));
        }
        return true;
    }
    catch (...) {
        return false;
    }
}

bool ExpressionTree::UpdateVariables(const double** data_ptrs) {
    // 安全检查：确保外部传入的数据数量与内部存储的一致
    if (data_ptrs == nullptr || m_variableStorage.size() == 0) {
        return false;
    }

    try {
        // 遍历内部存储的变量和外部传入的指针
        // 注意：这里依赖于 m_variableStorage 的顺序与用户传入 data_ptrs 的顺序一致
        for (size_t i = 0; i < m_variableStorage.size(); ++i) {
            double* pExternal = const_cast<double*>(data_ptrs[i]); // 外部新数据
            mup::Value& valInternal = m_variableStorage[i];       // Parser 内部旧数据

            if (pExternal == nullptr) 
                continue;

            // 核心逻辑：直接内存拷贝
            // 因为我们知道 Value 内部存储的是 float_type (通常是 double)
            // 这种方式比通过 Parser 接口 SetValue 更快，且避免了类型转换开销

            // 情况 1: 标量 (Scalar)
            if (valInternal.IsScalar()) {
                valInternal = mup::Value(*pExternal); // 赋值触发内部拷贝
            }
            // 情况 2: 矩阵 (Matrix) / 数组
            // 注意：这里假设外部数组的大小与内部 Value 的大小一致
            else if (valInternal.IsMatrix()) {
                // 获取矩阵总元素数
                int rows = valInternal.GetRows();
                int cols = valInternal.GetCols();
                int counter = 0;

                for (int i = 0; i < rows; ++i) {
                    for (int j = 0; j < cols; ++j) {
                        valInternal.At(i, j) = mup::Value(pExternal[i * cols + j]);
                    }
                }
            }
        }
        return true;
    }
    catch (...) {
        return false;
    }
}

void ExpressionTree::Calculate() {
    try {
        // 此时 Eval 读取的是 m_variableStorage 中已经被 UpdateVariables 更新过的值
        m_result = m_parser.Eval();
    }
    catch (ParserError& e) {
        std::cerr << "Calc Error: " << e.GetMsg() << std::endl;
        m_result = Value(0.0);
    }
}

void ExpressionTree::getResult(double* result) {
    // 处理标量情况
    if (!m_result.IsMatrix()) {
        result[0] = m_result.GetFloat();
        return;
    }

    // 矩阵处理
    int rows = m_result.GetRows();
    int cols = m_result.GetCols();

    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            result[r * cols + c] = m_result.At(r, c).GetFloat();
        }
    }
}

int ExpressionTree::getSize() {
    if (m_result.IsMatrix()) {
        return m_result.GetRows() * m_result.GetCols();
    }
    return 1;
}

ExpressionTree* ExpressionTree::copy()
{
    // 创建新的计算树实例（初始化默认解析器配置）
    ExpressionTree* newTree = new ExpressionTree();
    if (newTree == nullptr) {
        std::cerr << "Copy Error: Failed to allocate memory for new CalculationTree" << std::endl;
        return nullptr;
    }

    try {
        // 深拷贝变量存储容器（m_variableStorage是vector，赋值为深拷贝）
        newTree->m_variableStorage = this->m_variableStorage;
        newTree->m_variableName = this->m_variableName;

        // 重新绑定变量到新解析器（关键：绑定到新实例的变量存储，而非原实例）
        for (int i = 0; i < m_variableStorage.size(); ++i) {
            newTree->m_parser.DefineVar(m_variableName[i], Variable(&newTree->m_variableStorage[i]));
        }

        // 拷贝已编译的表达式（若原实例有有效表达式）
        mup::string_type originalExpr = this->m_parser.GetExpr();
        if (!originalExpr.empty()) {
            // 重新编译表达式，确保新实例的解析树与原实例一致
            bool compileSuccess = newTree->Compile(std::string(originalExpr.c_str()));
            if (!compileSuccess) {
                std::cerr << "Copy Error: Failed to compile expression for new CalculationTree" << std::endl;
                delete newTree;
                return nullptr;
            }
        }

        // 深拷贝计算结果缓存（m_result是mup::Value，赋值为深拷贝）
        newTree->m_result = this->m_result;

        return newTree;
    }
    catch (mup::ParserError& e) {
        std::cerr << "Copy Error (ParserError): " << e.GetMsg() << std::endl;
        delete newTree;
        return nullptr;
    }
    catch (...) {
        std::cerr << "Copy Error: Unknown exception occurred during copy" << std::endl;
        delete newTree;
        return nullptr;
    }
}

// =====================================================================
// 私有辅助函数
// =====================================================================

mup::Value ExpressionTree::BuildValueFromData(const ElementNote& note, double* data_ptr) {
    // 根据 note 的 shape 信息构建 Value
    // 默认行为：构建标量
    if (note._length <= 1 ||( note._shape[0] <= 1 && note._shape[1] <= 1)) {
        // return mup::Value((float_type)(*data_ptr));
        return mup::Value((float_type)(0));
    }

    // 矩阵情况
    int rows = note._shape[0];
    int cols = note._shape[1];
    mup::Matrix<mup::Value> mat(rows, cols);
    const double* pData = static_cast<const double*>(data_ptr);

    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < cols; ++j) {
            //mat.At(i, j) = mup::Value(pData[i * cols + j]);
            mat.At(i, j) = mup::Value(0.0);
        }
    }

    return mup::Value(mat);
}