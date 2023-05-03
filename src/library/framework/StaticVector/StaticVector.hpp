#ifndef __BN3MONKEY_STATIC_VECTOR__
#define __BN3MONKEY_STATIC_VECTOR__

#include <string>
#include <cstring>
#include <new>
#include <initializer_list>

namespace Bn3Monkey
{
	template<typename Type, size_t MAX_SIZE>
	class Bn3StaticVector
	{
	public:
		using iterator = Type*;
		using const_iterator = const Type*;

		Bn3StaticVector(const Type* values, size_t length)
		{
			_length = length;
			memcpy(_data, values, sizeof(Type) * length);
		}
		Bn3StaticVector(std::initializer_list<Type> values)
		{
			std::copy(values.begin(), values.end(), (Type*)_data);
		}
		Bn3StaticVector() : _length(0) {}
		Bn3StaticVector(const Bn3StaticVector& other) : _length(other._length)
		{
			std::copy(other._data, other._data + sizeof(Type) * MAX_SIZE, _data);
		}
		Bn3StaticVector(Bn3StaticVector&& other) : _length(other._length)
		{
			std::copy(other._data, other._data + sizeof(Type) * MAX_SIZE, _data);
		}
		~Bn3StaticVector() {
			clear();
		}

		Bn3StaticVector& operator=(const Bn3StaticVector& other)
		{
			_length = other._length;
			std::copy(other._data, other._data + sizeof(Type) * MAX_SIZE, _data);
			return *this;
		}

		template<class... Args>
		void emplace_back(Args&&... args)
		{
			if (_length < MAX_SIZE)
			{
				new(_data + sizeof(Type)*_length) Type(std::forward<Args>(args)...);
				_length += 1;
			}
		}

		void push_back(const Type& value)
		{
			if (_length < MAX_SIZE)
			{
				memcpy(_data + sizeof(Type) * _length, &value, sizeof(Type));
				_length += 1;
			}
		}

		void push_back(const Bn3StaticVector& other)
		{
			if (_length + other._length <= MAX_SIZE)
			{
				memcpy(_data + sizeof(Type) * _length, other._data, sizeof(Type) * other._length);
				_length += other._length;
			}
		}

		Type& front() {
			return *std::launder(reinterpret_cast<Type*>(_data));
		}
		Type& back() {
			return *std::launder(reinterpret_cast<Type*>(_data + sizeof(Type)*(idx-1)));
		}

		Type& operator[](size_t idx) { 
			return *std::launder(reinterpret_cast<Type*>(_data + sizeof(Type)*idx));
		}
		const Type& operator[](size_t idx) const { 
			return *std::launder(reinterpret_cast<Type*>(_data + sizeof(Type) * idx));
		}

		inline size_t size() { return _length; }

		iterator begin() {
			return std::launder(reinterpret_cast<Type*>(_data));
		}

		iterator end() {
			return std::launder(reinterpret_cast<Type*>(_data + sizeof(Type)*_length));
		}

		const_iterator begin() const {
			return std::launder(reinterpret_cast<Type*>(_data));
		}

		const_iterator end() const {
			return std::launder(reinterpret_cast<Type*>(_data + sizeof(Type) * _length));
		}

		void clear() {
			for (Type* value = begin(); value != end(); value++)
			{
				value->~Type();
			}
			_length = 0;
			memset(_data, 0, sizeof(Type) * MAX_SIZE);
		}


		void copyFrom(const Type* values, size_t start, size_t end)
		{
			memcpy(_data + sizeof(Type) * start, values, sizeof(Type) * (end - start));
		}

		
		void copyTo(Type* values, size_t start, size_t end)
		{
			memcpy(values, _data + sizeof(Type) * start, sizeof(Type) * (end - start));
		}
		

	private:
		size_t _length;

		unsigned char _data[sizeof(Type) * MAX_SIZE];
#if _DEBUG
		Type (*_ptr)[MAX_SIZE] = std::launder(reinterpret_cast<Type(*)[MAX_SIZE]>(_data));
#endif
	};
}


#endif