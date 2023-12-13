/*
*	ChessMaster, a free UCI / Xboard chess engine
*	Copyright (C) 2023 Ilyin Yegor
*
*	ChessMaster is free software : you can redistribute it and /or modify
*	it under the terms of the GNU General Public License as published by
*	the Free Software Foundation, either version 3 of the License, or
*	(at your option) any later version.
*
*	ChessMaster is distributed in the hope that it will be useful,
*	but WITHOUT ANY WARRANTY; without even the implied warranty of
*	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
*	GNU General Public License for more details.
*
*	You should have received a copy of the GNU General Public License
*	along with ChessMaster. If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once
#include <cassert>
#include <concepts>
#include <ostream>

#include "Types.h"
#include "HighAssert.h"

/*
*	EnumWrap.h contains a base class used to wrap some enumeration.
*
*	It has no appliances as is and must be used to inherit some classes
*	from it.
* 
*	Usage:
*		WRAPPED_ENUM_BEGIN(Enum, int)
*			NAME,
*			VAL,
*			...
*		WRAPPED_ENUM_IMPL
*			constexpr ... - methods here
*		WRAPPED_ENUM_END(Enum, int)
* 
*	Types defined this way can be used as if enum classes, but:
*		1) They can have methods.
*		2) By default, you can iterate over their values with Name::iter() and Name::reverseIter().
*/

template<class T> requires requires(T t) { t.toString(); }
inline std::ostream& operator<<(std::ostream& out, const T& value) noexcept {
	return out << value.toString();
}

template<std::integral T>
class EnumWrap {
protected:
	template<class Value> requires (std::is_enum_v<Value> && sizeof(Value) == sizeof(T))
	struct EnumIterator final {
	public:
		using value_type = T;
		using pointer = const T*;
		using reference = const T&;
		using difference_type = T;
		using Iterator_category = std::random_access_iterator_tag;

	private:
		T m_value;

	public:
		INLINE constexpr EnumIterator() noexcept : m_value(0) { }
		INLINE constexpr EnumIterator(const EnumIterator& other) noexcept : m_value(other.m_value) { }
		INLINE constexpr explicit EnumIterator(const T val) noexcept : m_value(val) { }
		INLINE constexpr explicit EnumIterator(const Value val) noexcept : m_value(val) { }

		INLINE constexpr EnumIterator& operator=(const EnumIterator other) noexcept {
			m_value = other.m_value;
			return *this;
		}

		CM_PURE constexpr bool operator==(const EnumIterator other) const noexcept {
			return m_value == other.m_value;
		}

		CM_PURE constexpr auto operator<=>(const EnumIterator other) const noexcept {
			return m_value <=> other.m_value;
		}

		CM_PURE constexpr Value operator*() const noexcept {
			return Value(m_value);
		}

		CM_PURE constexpr Value operator[](const T val) const noexcept {
			return Value(T(m_value) + val);
		}

		INLINE constexpr EnumIterator& operator++() noexcept {
			++m_value;
			return *this;
		}

		INLINE constexpr EnumIterator operator++(int) noexcept {
			EnumIterator<Value> tmp = this;
			++m_value;
			return tmp;
		}

		INLINE constexpr EnumIterator& operator--() noexcept {
			--m_value;
			return *this;
		}

		INLINE constexpr EnumIterator operator--(int) noexcept {
			EnumIterator<Value> tmp = this;
			--m_value;
			return tmp;
		}

		CM_PURE constexpr EnumIterator operator+(const T value) const noexcept {
			return EnumIterator(m_value + value);
		}

		CM_PURE constexpr EnumIterator operator-(const T value) const noexcept {
			return EnumIterator(m_value - value);
		}

		INLINE constexpr EnumIterator& operator+=(const T value) noexcept {
			m_value += value;
			return *this;
		}

		INLINE constexpr EnumIterator& operator-=(const T value) noexcept {
			m_value -= value;
			return *this;
		}
	};

	template<class Value, T End> requires std::is_enum_v<Value>
	struct EnumIterationRange final {
	public:
		using Iterator = EnumIterator<Value>;

	public:
		CM_PURE constexpr Iterator begin() const noexcept {
			return Iterator(T(0));
		}

		CM_PURE constexpr Iterator end() const noexcept {
			return Iterator(End);
		}

		CM_PURE constexpr T size() const noexcept {
			return End;
		}
	};

	template<class Value, T End> requires std::is_enum_v<Value>
	struct EnumReverseIterationRange final {
	public:
		using Iterator = EnumIterator<Value>;
		using ReverseIterator = std::reverse_iterator<Iterator>;

	public:
		CM_PURE constexpr ReverseIterator begin() const noexcept {
			return ReverseIterator(Iterator(End));
		}

		CM_PURE constexpr ReverseIterator end() const noexcept {
			return ReverseIterator(Iterator(T(0)));
		}

		CM_PURE constexpr T size() const noexcept {
			return End;
		}
	};

protected:
	T m_value = 0;

public:
	INLINE constexpr EnumWrap() noexcept = default;
	INLINE constexpr EnumWrap(const EnumWrap&) noexcept = default;
	INLINE constexpr EnumWrap(EnumWrap&&) noexcept = default;

	INLINE constexpr EnumWrap(const T value) noexcept : m_value(value) { }

	INLINE constexpr EnumWrap& operator=(const EnumWrap other) noexcept {
		m_value = T(other);
		return *this;
	}

	CM_PURE constexpr operator T() const noexcept {
		return m_value;
	}

	CM_PURE constexpr explicit operator bool() const noexcept {
		return m_value != 0;
	}
};

#define WRAPPED_ENUM_BEGIN(name, base_type) \
struct name final : public EnumWrap<base_type> { \
	enum Value : base_type {

#define WRAPPED_ENUM_IMPL \
		VALUES_COUNT\
	};

#define WRAPPED_ENUM_IMPL_WITH(specific_val) \
		VALUES_COUNT, specific_val\
	};

#define WRAPPED_ENUM_END_WITH(name, base_type, specific_val)\
	constexpr name() noexcept = default;\
	\
	INLINE explicit constexpr name(const base_type value) noexcept : EnumWrap<base_type>(value) { }\
	\
	INLINE constexpr name(const Value value) noexcept : EnumWrap<base_type>(value) {\
		ASSERT(value < Value::VALUES_COUNT || value == specific_val); \
	}\
	\
	INLINE constexpr name& operator=(const name other) noexcept {\
		m_value = other;\
		return *this;\
	}\
	\
	CM_PURE constexpr Value value() const noexcept {\
		return Value(m_value);\
	}\
	\
	CM_PURE constexpr static auto iter() noexcept {\
		return EnumWrap<base_type>::EnumIterationRange<Value, Value::VALUES_COUNT>();\
	}\
	\
	CM_PURE constexpr static auto reverseIter() noexcept {\
		return EnumWrap<base_type>::EnumReverseIterationRange<Value, Value::VALUES_COUNT>();\
	}\
};

#define WRAPPED_ENUM_END(name, base_type)\
	constexpr name() noexcept = default;\
	\
	INLINE explicit constexpr name(const base_type value) noexcept : name(Value(value)) { }\
	\
	INLINE constexpr name(const Value value) noexcept : EnumWrap<base_type>(value) {\
		ASSERT(value < Value::VALUES_COUNT); \
	}\
	\
	INLINE constexpr name& operator=(const name other) noexcept {\
		m_value = other;\
		return *this;\
	}\
	\
	CM_PURE constexpr Value value() const noexcept {\
		return Value(m_value);\
	}\
	\
	CM_PURE constexpr static auto iter() noexcept {\
		return EnumWrap<base_type>::EnumIterationRange<Value, Value::VALUES_COUNT>();\
	}\
	\
	CM_PURE constexpr static auto reverseIter() noexcept {\
		return EnumWrap<base_type>::EnumReverseIterationRange<Value, Value::VALUES_COUNT>();\
	}\
};
