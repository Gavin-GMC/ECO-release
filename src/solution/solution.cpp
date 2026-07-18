#include <stdexcept>
#include <utility>
#include "Solution.h"
#include "ecflow-basicfunc.h"

using namespace ECFlow;

 // 默认构造函数实现
Solution::Solution()
{
    _size = 0;
    _object_number = 0;
    decoder_pointer = nullptr;
    result = nullptr;
    fitness = nullptr;
}

// 析构函数实现
Solution::~Solution()
{
    delete[] result;
    delete[] fitness;
}

// 深拷贝构造(修 SOLUTION-RULE-OF-THREE)
Solution::Solution(const Solution& source)
{
    _size = 0; _object_number = 0;
    result = nullptr; fitness = nullptr;
    decoder_pointer = source.decoder_pointer;   // shared_ptr:共享解码器
    copy(source);                               // 深拷贝 result/fitness
}

// 移动构造:窃取源缓冲
Solution::Solution(Solution&& source) noexcept
{
    _size = source._size; _object_number = source._object_number;
    result = source.result; fitness = source.fitness;
    decoder_pointer = std::move(source.decoder_pointer);
    source._size = 0; source._object_number = 0;
    source.result = nullptr; source.fitness = nullptr;
}

// 深拷贝赋值
Solution& Solution::operator=(const Solution& source)
{
    if (this != &source)
    {
        decoder_pointer = source.decoder_pointer;
        copy(source);   // setSize 处理 result/fitness 重分配 + memcpy
    }
    return *this;
}

// 移动赋值
Solution& Solution::operator=(Solution&& source) noexcept
{
    if (this != &source)
    {
        delete[] result; delete[] fitness;
        _size = source._size; _object_number = source._object_number;
        result = source.result; fitness = source.fitness;
        decoder_pointer = std::move(source.decoder_pointer);
        source._size = 0; source._object_number = 0;
        source.result = nullptr; source.fitness = nullptr;
    }
    return *this;
}

// 获取结果数组长度
int Solution::getSolutionSize()
{
    return _size;
}

// 获取适应度数组长度
int Solution::getObjectNumber()
{
    return _object_number;
}

// 设置数组长度（int参数版）
void Solution::setSize(int size, int object_number)
{
    if (_size != size)
    {
        delete[] result;
        _size = size;
        result = new double[_size];
    }
    if (_object_number != object_number)
    {
        delete[] fitness;
        _object_number = object_number;
        fitness = new double[_object_number];
    }
}

// 设置数组长度（Solution参数版）
void Solution::setSize(const Solution& source)
{
    setSize(source._size, source._object_number);
}

// 设置解码器指针（指针参数版）
void Solution::setDecoder(std::shared_ptr<SolutionDecoder> pointer)
{
    if(pointer==nullptr)
        throw std::invalid_argument("setDecoder: pointer to SolutionDecoder cannot be nullptr!");

    decoder_pointer = pointer;
    setSize(decoder_pointer->getSolutionSize(), decoder_pointer->getObjectNumber());
}

// 设置解码器指针,并进行解映射
void Solution::setDecoder(std::shared_ptr<SolutionDecoder> pointer, const variableMapTable& map)
{
    // 缓存变量用于迁移
    double* result_memory = result;
    result = nullptr;
    _size = 0;

    setDecoder(pointer);

    // 开始映射
    for (int i = 0; i < map.begin_index.size(); i++)
    {
        if (map.map_index[i] > -1) // 迁移
        {
            memcpy(result + map.begin_index[i], result_memory + map.map_index[i], map.map_length[i] * sizeof(double));
        }
        else // 全零初始化
        {
            for (int k = 0; k < map.map_length[i]; k++)
            {
                result[map.begin_index[i] + k] = 0;
            }
        }
    }

    delete[] result_memory; // 清理缓存
}

// 设置解码器指针（Solution参数版）
void Solution::setDecoder(const Solution& source)
{
    setDecoder(source.decoder_pointer);
}

// 深拷贝（Solution对象版）
void Solution::copy(const Solution& copy_source)
{
    setSize(copy_source._size, copy_source._object_number);
    // 深拷贝结果数组和适应度数组
    memcpy(result, copy_source.result, _size * sizeof(double));
    memcpy(fitness, copy_source.fitness, _object_number * sizeof(double));
    // 浅拷贝解码器指针
    decoder_pointer = copy_source.decoder_pointer;
}

// 深拷贝（外部数组版）
void Solution::copy(const double* source_result, const double* source_fitness)
{
    memcpy(result, source_result, _size * sizeof(double));
    memcpy(fitness, source_fitness, _object_number * sizeof(double));
}

// 浅拷贝
void Solution::shallowCopy(const Solution& copy_source)
{
    _size = copy_source._size;
    _object_number = copy_source._object_number;
    result = copy_source.result;
    fitness = copy_source.fitness;
    decoder_pointer = copy_source.decoder_pointer;
}

// 浅清空
void Solution::shallowClear()
{
    _size = 0;
    _object_number = 0;
    result = nullptr;
    fitness = nullptr;
    decoder_pointer = nullptr;
}

// 交换成员变量
void Solution::swap(Solution& copy_source)
{
    std::swap(_size, copy_source._size);
    std::swap(_object_number, copy_source._object_number);
    std::swap(result, copy_source.result);
    std::swap(fitness, copy_source.fitness);
    std::swap(decoder_pointer, copy_source.decoder_pointer);
}

// []运算符重载
double& Solution::operator[](const int index)
{
    return result[index];
}

// ==运算符重载
bool Solution::operator==(const Solution& a) const
{
    return memcmp(this->result, a.result, _size * sizeof(double)) == 0;
}