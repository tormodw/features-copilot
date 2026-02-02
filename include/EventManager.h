#ifndef EVENT_MANAGER_H
#define EVENT_MANAGER_H

#include "Event.h"
#include <functional>
#include <vector>
#include <map>
#include <mutex>

class EventManager {
public:
    using EventHandler = std::function<void(const Event&)>;

    static EventManager& getInstance() {
        static EventManager instance;
        return instance;
    }

    void subscribe(EventType type, EventHandler handler) {
        std::lock_guard<std::mutex> lock(mutex_);
        handlers_[type].push_back(handler);
    }

    void publish(const Event& event) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = handlers_.find(event.type);
        if (it != handlers_.end()) {
            for (const auto& handler : it->second) {
                handler(event);
            }
        }
    }

private:
    EventManager() = default;
    EventManager(const EventManager&) = delete;
    EventManager& operator=(const EventManager&) = delete;

    std::map<EventType, std::vector<EventHandler>> handlers_;
    std::mutex mutex_;
};

#endif // EVENT_MANAGER_H
