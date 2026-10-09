namespace Limen {

	template <typename T>
	class LArrat {
	private: //呃 size_t是一种无符号整数类型，通常用于表示对象的大小或索引。
		T* data = nullptr; //指向内存起点的指针
		size_t size; // 实际存储的元素个数
		size_t capacity; //内存总容量

		void reallocate(size_t new_capacity) {  //直接重新分配一整个更大的内存（小换大）
			T* new_data = new T[new_capacity];  //新的数组指针（大小是引入的new_capacity）
			for (size_t i = 0; i < size; ++i) {
				new_data[i] = data[i];
			} //迁移旧数据到新数组
			delete[] data; //释放旧内存
			data = new_data;
			capacity = new_capacity;  //换名字
		}
	public:
		LArrat() = default; //默认构造函数
		~LArrat() {
			delete[] data;
		}//析构函数，释放内存

		void push_back(const T& val) {  //在尾部加元素
			if (size == capacity) {
				size_t next_capacity = (capacity == 0) ? 1 : capacity * 2;
				//如果当前容量为0，则新容量为1，否则新容量为当前容量的两倍
				reallocate(next_capacity);//迁移
			}
			data[size] = val; //在尾部加元素
			size++; //计数器+1

		}
		T& operator[](size_t index) { //重载下标运算符 //近似于定义了一个函数
			return data[index];
		} //返回指定索引的元素

//获取现在存储的元素个数和容量
		size_t getSize() const {  //const 表示只读不能写
			return size;
		}
		size_t getCapacity() const {
			return capacity;
		}

		//设指向数组起点和终点的指针
		T* begin() { return data； }
		T* end() { return data + size； }
	}

}