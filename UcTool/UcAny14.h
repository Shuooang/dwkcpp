// any14.h
#pragma once

#include <typeinfo>
#include <memory>
#include <utility>
#include <stdexcept>

namespace std14 {

	class bad_any_cast : public std::bad_cast {
	public:
		const char* what() const noexcept override {
			return "std14::bad_any_cast";
		}
	};

	class any {
	private:
		struct placeholder {
			virtual ~placeholder() = default;
			virtual const std::type_info& type() const noexcept = 0;
			virtual std::unique_ptr<placeholder> clone() const = 0;
		};

		template<typename T>
		struct holder : placeholder {
			T value;

			holder(const T& v) : value(v) {}
			holder(T&& v) : value(std::move(v)) {}

			const std::type_info& type() const noexcept override {
				return typeid(T);
			}
			std::unique_ptr<placeholder> clone() const override {
				return std::unique_ptr<placeholder>(new holder(value));
			}
		};

		std::unique_ptr<placeholder> content;

	public:
		any() noexcept : content(nullptr) {}

		any(const any& other)
			: content(other.content ? other.content->clone() : nullptr) {
		}

		any(any&& other) noexcept = default;

		template<typename T>
		any(T&& value)
			: content(new holder<typename std::decay<T>::type>(std::forward<T>(value))) {
		}

		any& operator=(const any& rhs) {
			any(rhs).swap(*this);
			return *this;
		}

		any& operator=(any&& rhs) noexcept {
			content = std::move(rhs.content);
			return *this;
		}

		void swap(any& other) noexcept {
			content.swap(other.content);
		}

		bool has_value() const noexcept {
			return static_cast<bool>(content);
		}

		const std::type_info& type() const noexcept {
			return content ? content->type() : typeid(void);
		}

		void reset() noexcept {
			content.reset();
		}

		template<typename T>
		friend T any_cast(const any& operand);

	};

	template<typename T>
	T any_cast(const any& operand) {
		if (!operand.content)
			throw bad_any_cast();
		if (operand.content->type() != typeid(T))
			throw bad_any_cast();
		auto held = static_cast<any::holder<typename std::decay<T>::type>*>(operand.content.get());
		return held->value;
	}

	template<typename T>
	T any_cast(any& operand) {
		return any_cast<T>(const_cast<const any&>(operand));
	}

	template<typename T>
	T any_cast(any&& operand) {
		return any_cast<T>(operand);
	}

} // namespace std14
