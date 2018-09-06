#pragma once

#include <memory>
#include <typeindex>
#include <stdexcept>
#include <iostream>
struct Qu1etAny
{
	Qu1etAny(void) : m_tpIndex(std::type_index(typeid(void))) {}
	Qu1etAny(const Qu1etAny& that) : m_ptr(that.Clone()), m_tpIndex(that.m_tpIndex) {}
	Qu1etAny(Qu1etAny && that) : m_ptr(std::move(that.m_ptr)), m_tpIndex(that.m_tpIndex) {}
	template<typename U, class = typename std::enable_if<!std::is_same<typename std::decay<U>::type, Qu1etAny>::value, U>::type> Qu1etAny(U && value) :
		m_ptr(new Derived < typename std::decay<U>::type>(std::forward<U>(value))),
		m_tpIndex(std::type_index(typeid(typename std::decay<U>::type))){}
	bool IsNull() const { return !bool(m_ptr); }
	template<class U> bool Is() const
	{
		return m_tpIndex == std::type_index(typeid(U));
	}
	template<class U>
	U& AnyCast()
	{
		if (!Is<U>())
		{
			std::cout << "can not cast " << typeid(U).name() << " to " << m_tpIndex.name() << std::endl;
			throw std::logic_error{ "bad cast" };
		}
		auto derived = dynamic_cast<Derived<U>*> (m_ptr.get());
		return derived->m_value;
	}
	Qu1etAny& operator=(const Qu1etAny& a)
	{
		if (m_ptr == a.m_ptr)
			return *this;
		m_ptr = a.Clone();
		m_tpIndex = a.m_tpIndex;
		return *this;
	}

	bool operator==(const Qu1etAny& a)
	{
		if (m_ptr == a.m_ptr && m_tpIndex == a.m_tpIndex)
		{
			return true;
		}
		return false;
	}

private:
	struct Base;
	typedef std::unique_ptr<Base> BasePtr;
	struct Base
	{
		virtual ~Base() {}
		virtual BasePtr Clone() const = 0;
	};
	template<typename T>
	struct Derived : Base
	{
		template<typename U>
		Derived(U && value) : m_value(std::forward<U>(value)) { }
		BasePtr Clone() const
		{
			return BasePtr(new Derived<T>(m_value));
		}
		T m_value;
	};
	BasePtr Clone() const
	{
		if (m_ptr != nullptr)
			return m_ptr->Clone();
		return nullptr;
	}
	BasePtr m_ptr;
	std::type_index m_tpIndex;
};