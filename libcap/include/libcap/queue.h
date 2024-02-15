#ifndef CAPTURER_QUEUE_H
#define CAPTURER_QUEUE_H

#include <mutex>
#include <queue>
#include <utility>

template<class T> class lock_queue
{
public:
    using value_type      = T;
    using reference       = T&;
    using const_reference = const T&;

    [[nodiscard]] bool empty() const noexcept(noexcept(buffer_.empty()))
    {
        std::lock_guard lock(mtx_);
        return buffer_.empty();
    }

    [[nodiscard]] auto size() const noexcept(noexcept(buffer_.size()))
    {
        std::lock_guard lock(mtx_);
        return buffer_.size();
    }

    [[nodiscard]] value_type pop()
    {
        std::lock_guard lock(mtx_);

        value_type front = std::move(buffer_.front());
        buffer_.pop();
        return front;
    }

    void push(const value_type& value)
    {
        std::lock_guard lock(mtx_);
        buffer_.push(value);
    }

    void push(value_type&& value)
    {
        std::lock_guard lock(mtx_);
        buffer_.push(std::move(value));
    }

    template<class... Valty> decltype(auto) emplace(Valty&&...args)
    {
        std::lock_guard lock(mtx_);

        return buffer_.emplace(std::forward<Valty>(args)...);
    }

    void clear()
    {
        std::lock_guard lock(mtx_);
        buffer_ = {};
    }

private:
    std::queue<T>      buffer_;
    mutable std::mutex mtx_;
};

#endif //! CAPTURER_QUEUE_H