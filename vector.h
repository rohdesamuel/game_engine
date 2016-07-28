#ifndef VECTOR__H
#define VECTOR__H

#include "std.h"

template <class Ty_, class Allocator_ = std::allocator<Ty_>>
class Vector
{
private:
	UINT min_size_ = 16;

	Ty_* data_;

	volatile UINT count_;
	volatile UINT capacity_;

	Allocator_ allocator_;

	void increase_size() {
		if (count_ >= capacity_) {
			capacity_ *= 2;
			copy_new_data(capacity_ / 2);
			increase_size();
		}
	}

	void decrease_size() {
		if (count_ < capacity_ && count_ > min_size_) {
			UINT old_capacity = capacity_;
			capacity_ = capacity_ / 2 == 0 ? 1 : capacity_ / 2;
			copy_new_data(old_capacity);
			decrease_size();
		}
	}

	void copy_new_data(UINT old_capacity) {
		Ty_* new_data = allocator_.allocate(capacity_);
		for (unsigned i = 0; i < old_capacity; ++i) {
			//new_data[i] = data_[i];
			memcpy(new_data + i, data_ + i, sizeof(Ty_));
		}
		allocator_.deallocate(data_, old_capacity);
		data_ = new_data;
	}

	void copy(const Vector<Ty_>& other) {
		capacity_ = other.capacity_;
		count_ = other.count_;
		data_ = allocator_.allocate(capacity_);
		for (unsigned i = 0; i < capacity_; ++i) {
			data_[i] = other[i];
		}
	}

	void move(Vector<Ty_>&& other) {
		capacity_ = other.capacity_;
		count_ = other.count_;
		data_ = other.data_;
		other.count_ = 0;
		other.data_ = nullptr;
	}

	void destroy() {
		if (data_) {
			allocator_.deallocate(data_, capacity_);
			data_ = nullptr;
		}
		count_ = 0;
		capacity_ = 0;
	}

public:
	typedef Ty_* iterator;

	Vector() :count_(0) {
		capacity_ = min_size_;
		data_ = allocator_.allocate(capacity_);
		for (int i = 0; i < capacity_; ++i) {
			new (data_ + i) Ty_();
		}
	}

	Vector(const Vector<Ty_>& other) {
		copy(other);
	}

	Vector(Vector<Ty_>&& other) {
		move(other);
	}

	Vector(const std::vector<Ty_>& other) :count_(0) {
		capacity_ = min_size_;
		data_ = allocator_.allocate(capacity_);
		for (auto& item : other) {
			push_back(item);
		}
	}

	Vector(std::vector<Ty_>&& other) :count_(0) {
		capacity_ = min_size_;
		data_ = allocator_.allocate(capacity_);
		for (auto& item : other) {
			push_back(item);
		}
	}


	~Vector() {
		destroy();
	}

	void push_back(const Ty_& data) {
		increase_size();
		data_[count_++] = data;
	}

	void push_back(Ty_&& data) {
		increase_size();
		data_[count_++] = std::move(data);
	}

	void pop_back() {
		--count_;
		decrease_size();
	}

	void resize(UINT new_size) {
		if (new_size > count_) {
			count_ = new_size;
			increase_size();
		}
		else  if (new_size < count_) {
			count_ = new_size;
			decrease_size();
		}
	}

	void reserve(UINT capacity) {
		if (capacity_ < capacity && capacity != 0) {
			UINT old_capacity = capacity_;
			capacity_ = capacity;
			int count = 0;
			while (capacity_ != 0) {
				capacity_ >>= 1;
				++count;
			}
			capacity_ = 1 << count;
			copy_new_data(old_capacity);
		}
	}

	

	void clear() {
		count_ = 0;
		capacity_ = min_size_;
	}

	Ty_& operator[](unsigned index) volatile {
		return data_[index];
	}

	Vector<Ty_>& operator=(const Vector<Ty_>& other) {
		if (this != &other) {
			destroy();
			copy(other);
		}
		return *this;
	}

	Vector<Ty_>& operator=(Vector<Ty_>&& other) {
		if (this != &other) {
			destroy();
			move(std::move(other));
		}
		return *this;
	}

	inline Ty_& front() volatile {
		return data_[0];
	}

	inline Ty_& back() volatile {
		return data_[count_ - 1];
	}

	inline iterator begin() volatile {
		return data_;
	}

	inline iterator end() volatile {
		return data_ + count_;// +1;
	}

	inline bool empty() volatile {
		return count_ == 0;
	}

	inline unsigned size() volatile {
		return count_;
	}
};


#endif /*VECTOR__H*/