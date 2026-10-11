#include "Limen/Core/Core.h"
#include <cstdint>
#include <initializer_list>
#include <utility>

namespace Limen {

	template <typename T>
	class LArray {
	private: //呃 uin32_t是一种无符号整数类型，通常用于表示对象的大小或索引。
		T* data = nullptr; //指向内存起点的指针
		uin32_t size; // 实际存储的元素个数
		uin32_t capacity; //内存总容量

		void reallocate(uin32_t new_capacity) {  //直接重新分配一整个更大的内存（小换大）
			T* new_data = new T[new_capacity];  //新的数组指针（大小是引入的new_capacity）
			for (uin32_t i = 0; i < size; ++i) {
				new_data[i] = data[i];
			} //迁移旧数据到新数组
			delete[] data; //释放旧内存
			data = new_data;
			capacity = new_capacity;  //换名字
		}
	public:
		LArray() = default; //默认构造器

		//定长容器（explicit防止隐式类型转换）
		explicit LArray(uin32_t n) { 
			if (n > 0) {
				data = new T[n](); //分配内存并初始化为默认值
				size = n;
				capacity = n;
			}
		}
		//定长并初始化为同一个指定值
		LArray(uin32_t n, const T& val) { 
			if (n > 0) {
				data = new T[n]; 
				for (size_t i = 0; i < n; ++i) {
					data[i] = val; 
				}
				size = n;
				capacity = n;
			}
		}
		//给一组数据直接按需构造
		LArray(std::initializer_list<T> list) {
			size = list.size();
			capacity = size;
			data = new T[capacity];
			uin32_t i = 0;
			for (const T& item : list) {
				data[i] = item;
				i++;
			}
		}
		//复制别人的容器
		LArray(const LArray& other) {
		size = other.size;
		capacity = other.capacity;
		if (capacity > 0) {
			data = new T[capacity]; // 开一块新内存
			for (uint32_t i = 0; i < size; ++i) {
				data[i] = other.data[i]; }// 开抄
		}
	}

		~LArray() {
			delete[] data;
		}//析构函数，释放内存

		//在尾部添加一个元素
		void push_back(const T& val) {  
			if (size == capacity) {
				uin32_t next_capacity = (capacity == 0) ? 1 : capacity * 2;
				//如果当前容量为0，则新容量为1，否则新容量为当前容量的两倍
				reallocate(next_capacity);//迁移
			}
			data[size] = val; //把元素搬运进去
			size++; //计数器+1
		}

		template <typename... Args> //传几个参数就接收几个の模板
		void emplace_back(Args&&... args) {  //万能引用，传变量/临时变量都直接引用
			if (size == capacity) {
				uin32_t next_capacity = (capacity == 0) ? 1 : capacity * 2;
				reallocate(next_capacity);
			}
			//在 &data[size] 这个内存地址上，直接用 args 构造对象
			new (&data[size]) T(std::forward<Args>(args)...); //（...是解包）
			size++;
		}


		T& operator[](uin32_t index) { //重载下标运算符 //近似于定义了一个函数
			return data[index];
		} //返回指定索引的元素
	
		//获取现在存储的元素个数和容量
		uin32_t getSize() const {  //const 表示只读不能写
			return size;
		}
		uin32_t getCapacity() const {
			return capacity;
		}

		//1、普通迭代器，设指向数组起点和终点的指针，支持读写
		T* begin() { return data； }
		T* end() { return data + size； }

		// 2. 常量迭代器，用于只读遍历
		const T* cbegin() const { return data; }
		const T* cend() const { return data + size; }

		// 为了兼容老代码， 给const 修饰的对象提供只读版本
		const T* begin() const { return data; }
		const T* end() const { return data + size; }

		// 3. 反向迭代器（支持从后往前遍历：for(auto it = v.rbegin(); it != v.rend(); ++it)）
		std::reverse_iterator<T*> rbegin() { return std::reverse_iterator<T*>(end()); }
		std::reverse_iterator<T*> rend() { return std::reverse_iterator<T*>(begin()); }

		// 4. 常量反向迭代器
		std::reverse_iterator<const T*> crbegin() const { return std::reverse_iterator<const T*>(cend()); }
		std::reverse_iterator<const T*> crend() const { return std::reverse_iterator<const T*>(cbegin()); }
	}

}