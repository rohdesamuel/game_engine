#ifndef QUEUE__H
#define QUEUE__H

#include "vector.h"

template <class Ty_>
class Queue
{
private:
	Vector<Ty_> data_;
	unsigned max_size_;
	volatile unsigned push_index_;
	volatile unsigned pop_index_;
	volatile unsigned size_;
public:
	Queue(int queue_capacity) :max_size_(queue_capacity), size_(0), push_index_(0), pop_index_(0) {
	}

	Ty_& peek() {
		return data_.front();
	}

	Ty_& dequeue() {
		Ty_& ret = data_[pop_index_];
		if (size_ != 0) {
			pop_index_ = (pop_index_ + 1) % max_size_;
			--size_;
		}
		return ret;
	}

	void enqueue(Ty_& val) {
		if (size_ < max_size_) {
			if (push_index_ == data_.size()) {
				data_.push_back(val);
			}
			else {
				data_[push_index_] = val;
			}
			push_index_ = (push_index_ + 1) % max_size_;
			++size_;
			//AssertOrDie(size_ <= 2);
		}
	}

	void enqueue(Ty_&& val) {
		if (size_ < max_size_) {
			if (push_index_ == data_.size()) {
				data_.push_back(val);
			}
			else {
				data_[push_index_] = std::move(val);
			}
			push_index_ = (push_index_ + 1) % max_size_;
			++size_;
			//AssertOrDie(size_ <= 2);
		}
	}

	unsigned size() {
		return size_;
	}

	bool empty() {
		return size_ == 0;
	}

	void reserve(UINT capacity) {
		data_.reserve(capacity);
	}
};

#endif /*FAST_QUEUE__H*/