#ifndef __FUNCTION_HPP__
#define __FUNCTION_HPP__

/* Lightweight std::function‑like wrapper without pulling in the C++ stdlib. */

template<typename T> struct remove_reference      { using type = T; };
template<typename T> struct remove_reference<T&>  { using type = T; };
template<typename T> struct remove_reference<T&&> { using type = T; };

template<typename T>
constexpr T&& forward(typename remove_reference<T>::type& t) {
    return static_cast<T&&>(t);
}

template<typename T>
constexpr T&& forward(typename remove_reference<T>::type&& t) {
    return static_cast<T&&>(t);
}

template<typename Signature = void()>
class Function;

/* Non-void return type */
template<typename R, typename... Args>
class Function<R(Args...)> {
    class BaseCallable {
    public:
        virtual ~BaseCallable() {}
        virtual R call(Args&&... args) const = 0;
        virtual BaseCallable* clone() const = 0;
    };

    template<typename T>
    class Callable : public BaseCallable {
        T callable;

    public:
        Callable(T callable) : callable(callable) {}

        R call(Args&&... args) const override {
            return callable(forward<Args>(args)...);
        }

        BaseCallable* clone() const override {
            return new Callable<T>(callable);
        }
    };

    BaseCallable* callable;

public:
    Function() : callable(nullptr) {}

    Function(const Function& other) {
        callable = other.callable ? other.callable->clone() : nullptr;
    }

    template<typename T>
    Function(T lambda) : callable(new Callable<T>(lambda)) {}

    ~Function() {
        delete callable;
    }

    Function& operator=(const Function& other) = delete; // Copy assignment

    Function(Function&& other) : callable(other.callable) {
        other.callable = nullptr;
    }

    Function& operator=(Function&& other) {
        if (this != &other) {
            delete callable;
            callable = other.callable;
            other.callable = nullptr;
        }
        return *this;
    }

    R operator()(Args... args) const {
        if (!callable) {
            return R();
        }
        return callable->call(forward<Args>(args)...);
    }

    explicit operator bool() const {
        return callable != nullptr;
    }
};

/* void return type specialization */
template<typename... Args>
class Function<void(Args...)> {
    class BaseCallable {
    public:
        virtual ~BaseCallable() {}
        virtual void call(Args&&... args) const = 0;
        virtual BaseCallable* clone() const = 0;
    };

    template<typename T>
    class Callable : public BaseCallable {
        T callable;

    public:
        Callable(T callable) : callable(callable) {}

        void call(Args&&... args) const override {
            callable(forward<Args>(args)...);
        }

        BaseCallable* clone() const override {
            return new Callable<T>(callable);
        }
    };

    BaseCallable* callable;

public:
    Function() : callable(nullptr) {}

    Function(const Function& other) {
        callable = other.callable ? other.callable->clone() : nullptr;
    }

    template<typename T>
    Function(T lambda) : callable(new Callable<T>(lambda)) {}

    ~Function() {
        delete callable;
    }

    Function& operator=(const Function& other) = delete; // Copy assignment

    Function(Function&& other) : callable(other.callable) {
        other.callable = nullptr;
    }

    Function& operator=(Function&& other) {
        if (this != &other) {
            delete callable;
            callable = other.callable;
            other.callable = nullptr;
        }
        return *this;
    }

    void operator()(Args... args) const {
        if (callable) {
            callable->call(forward<Args>(args)...);
        }
    }

    explicit operator bool() const {
        return callable != nullptr;
    }
};

#endif // __FUNCTION_HPP__
