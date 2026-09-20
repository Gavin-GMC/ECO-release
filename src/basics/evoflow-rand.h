// SPDX-License-Identifier: BSD-3-Clause

#pragma once
#include <random>
#include <algorithm>   // std::shuffle
namespace EvoFlow
{
	/**
	* @brief ��������������������ӣ����ڸ���������У�
	*
	* �ú�����Ϊȫ��Ĭ�ϵ��������������std::mt19937������ָ�����ӣ�
	* ��ͬ���ӻ�������ȫһ�µ�������У������ڲ���/���ֳ�����
	*
	* @param seed �޷����������͵�����ֵ��֧������Ǹ�����
	*/
	void set_seed(unsigned int seed);

	/**
	 * @brief ��������о���������ң�ԭ���޸ģ�
	 *
	 * ʹ��ȫ��Ĭ�������������������Ԫ�ؽ��о���ϴ�ƣ����Һ��Ԫ�طֲ����Ͼ��ȷֲ���
	 * ��ָ��ֱ����Ϊ������ʵ�����ʹ�ã���������ɿ�����Ԫ�����͡�
	 *
	 * @tparam T ����Ԫ�ص����ͣ�֧������ɿ��������ͣ�int/double/�Զ���ṹ��ȣ�
	 * @param arr ָ��������Ԫ�ص�ָ�룬��n>0����Ϊnullptr
	 * @param n �����Ԫ�ظ�������n=0�����޲���
	 * @throw std::invalid_argument ��n>0��arrΪnullptrʱ�׳����쳣
	 */
	template<class T>
	void shuffle(T* arr, size_t n);

	/**
	 * @brief ���� [0.0, 1.0) ��Χ�ڵ����������������ҿ���
	 *
	 * �� get_real() �����ļ򻯰棬�̶�����0��1֮��ľ��ȷֲ���������
	 *
	 * @return double ��Χ�� [0.0, 1.0) ֮������������
	 */
	// ============================================================================
	// ⚠ 迁移语义差异(有意保持,勿轻改):rand01() 现为**半开区间 [0,1)**
	//   (uniform_real_distribution(0,1) → 可取 0,不取 1)。
	//   原始 EvoFlow 为**双闭 [0,1]**(rand()/RAND_MAX,端点 0 与 1 均可取)。
	//   v2 迁移改为 [0,1) 并保持,因其:① 是主流 RNG 约定;② 更安全——`int(rand01()*N)`
	//   恒落 [0, N-1];若还原成闭 [0,1],偶发的 rand01()==1 会使 `*N` 得到 N → **索引越界**。
	//   ⇒ 需要严格双开 (0,1)(如 log/Box-Muller)的场景请用 rand01_()。
	// ============================================================================
	double rand01();

	// rand01_: 双开区间 (0,1) 随机数(排除 0 与 1),供 log / Box-Muller 等避免边界。
	//   原语义 = (rand()+1)/(RAND_MAX+2),严格 (0,1) 开;此处对 rand01 用拒绝采样等价实现(均匀性不变)。
	//   自原 basicfunc.h 恢复(新 basics 迁移遗漏);inline 于头,不重建 libbasics。
	//   注:新 rand01() = uniform_real[0,1) 半开(原为闭 [0,1]),故实际只会拒到 0;>=1 分支为语义保险。
	inline double rand01_() { double v; do { v = rand01(); } while (v <= 0.0 || v >= 1.0); return v; }

	/**
	 * @brief ���� [0, INT_MAX] ��Χ�ڵķǸ��������
	 *
	 * ���ɷ�Χ [0, std::numeric_limits<int>::max()]����Ϊ�Ǹ��������� wide_rand() % N ȡģ�������±꣬�ʱ���Ǹ���
	 *
	 * @return int ���ȷֲ��� [0, INT_MAX] �ڵķǸ��������
	 */
	int wide_rand();

	/**
	 * @brief ����ָ�������� [low, high] �ڵ��������
	 *
	 * ���ɵ��������� low �� high �����˵㣬�ֲ����Ͼ��ȷֲ���
	 *
	 * @param low �����½磨������
	 * @param high �����Ͻ磨������
	 * @return int ��Χ�� [low, high] ֮����������
	 * @throw std::invalid_argument �� low > high ʱ�׳����쳣
	 */
	int get_int(int low, int high);

	/**
	 * @brief ����ָ������ҿ����� [low, high) �ڵ����������
	 *
	 * ���ɵĸ��������� low �������� high��Ĭ������ [0.0, 1.0) ��Χ�ĸ�������
	 *
	 * @param low �����½磨��������Ĭ��ֵ 0.0
	 * @param high �����Ͻ磨����������Ĭ��ֵ 1.0
	 * @return double ��Χ�� [low, high) ֮������������
	 * @throw std::invalid_argument �� low >= high ʱ�׳����쳣
	 */
	double get_rand_real(double low = 0.0, double high = 1.0);

	/**
	 * @brief ����ָ����������̬�ֲ�����˹�ֲ������������
	 *
	 * �ײ�ʹ�� std::normal_distribution ʵ�֣�Ĭ�����ɱ�׼��̬�ֲ�����ֵ0����׼��1����
	 * ��̬���÷ֲ��������������ܣ�����̬�޸ľ�ֵ�ͱ�׼�������
	 *
	 * @param mean ��̬�ֲ��ľ�ֵ������ֵ����Ĭ��ֵ 0.0
	 * @param stddev ��̬�ֲ��ı�׼�����Ǹ�����Ĭ��ֵ 1.0
	 * @return double ����ָ����̬�ֲ������������
	 * @throw std::invalid_argument �� stddev < 0 ʱ�׳����쳣
	 */
	double get_normal(double mean = 0.0, double stddev = 1.0);

	/**
	* @brief ��ȡȫ��Ĭ�� std::mt19937 �������������ǰ���õ�����ֵ
	*
	* �ú������س�ʼ��/����ȫ�������������ʱ��ʹ�õ�����ֵ���޷����������ͣ���
	* ��δ�ֶ����� set_seed() �޸����ӣ��򷵻س�������ʱ�Զ���ʼ���İ�ȫ����ֵ��
	* �����ֶ����� set_seed()���򷵻����һ�����õ�����ֵ�������ڸ���������С�
	*
	* @return unsigned int ȫ�� std::mt19937 ��������ǰ��Ч������ֵ���������� seed() ���������ֵһ��
	* @note 1. ���ص�����ֵ����Ӧȫ�����������Զ��������������������д洢�ͻ�ȡ��
	* @note 2. ���� std::mt19937 �������ڲ��洢Ϊ������ʽ���˴����ص��ǡ���ʼ�����ӡ��������ڲ�״̬����
	*          �����������³�ʼ���������Ը������У��޷���ԭ���������м�״̬��
	* @note 3. ��ȫ��������δ��ʽ�������ӣ����ص���ͨ����Դʱ������ɵİ�ȫ��ʼ������
	*/
	unsigned int get_random_seed();

	/**
	 * @brief ��ȡȫ��Ĭ�ϵ� std::mt19937 �����������ʵ��������
	 *
	 * ����ȫ�־�̬�� std::mt19937 ����������
	 * ȫ��Ψһ���������ڹᴩ�������򣬿������Զ�������ֲ����ֶ�����������״̬�ȸ߼�������
	 *
	 * @return std::mt19937& ȫ�� std::mt19937 �������ķ�const���ã�֧���޸�������״̬���� reseed������operator()��
	 * @warning 1. ��������Ϊȫ�ֹ�����Դ�����̲߳�������ʱ**���̰߳�ȫ**�������м������� std::mutex����
	 * @warning 2. ���ص����������ͣ���ֹ������������std::mt19937 �����ᵼ����������ظ�����
	 * @warning 3. ������ʵ�����������������һ�£������ֶ�����
	 */
	std::mt19937& get_random_engine();

	// ============================================================================
	// per-子群引擎切换(2.5.2):运行期"当前活跃引擎"机制
	// ----------------------------------------------------------------------------
	//   上面所有 RNG 自由函数(rand01/get_int/get_rand_real/wide_rand/get_normal/
	//   shuffle)内部都改用"当前活跃引擎":为空时用全局默认引擎(random_engine_),
	//   非空时用它所指的引擎。指针是 **thread_local**——并行时每线程各持一份,
	//   互不覆盖。子群执行前用 EngineScope 把它指向自己的持久 _engine,使深在
	//   workflow 里的通用 RNG 调用透明落到本子群引擎(调用点无需知道子群身份)。
	// ============================================================================

	// 设置/读取本线程"当前活跃引擎"指针(nullptr=用全局默认引擎)。
	void set_current_engine(std::mt19937* e);
	std::mt19937* get_current_engine();

	// 用与全局 set_seed 相同的方式给任意引擎播种(内部走 randomlib::set_seed);
	// 保证 index==0 的子群引擎与全局默认引擎逐位一致。定义在 .cpp(不在头暴露 randlib)。
	void seed_engine(std::mt19937& engine, unsigned int seed);

	// RAII:进入作用域设当前引擎,退出还原前值(可嵌套;线程独立)。
	class EngineScope
	{
		std::mt19937* _prev;
	public:
		explicit EngineScope(std::mt19937* e) : _prev(get_current_engine()) { set_current_engine(e); }
		~EngineScope() { set_current_engine(_prev); }
		EngineScope(const EngineScope&) = delete;
		EngineScope& operator=(const EngineScope&) = delete;
	};

	// 由基种子与子群序号派生该子群的确定性种子:
	//   index==0 → base(单子群与改造前逐位一致);index>0 → 用大奇数错开各子群的流。
	// 只在播种时用到子群序号;运行期不再需要序号(全靠 EngineScope 切换)。
	inline unsigned int derive_subpop_seed(unsigned int base, int index)
	{
		return index == 0 ? base : (base + (unsigned int)index * 2654435761u);
	}

	// shuffle: template defined in-header so all TUs can instantiate it
	template<class T>
	void shuffle(T* arr, size_t n)
	{
		std::shuffle(arr, arr + n, get_random_engine());
	}
}
