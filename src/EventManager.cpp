#include "EventManager.h"

EventManager& EventManager::getInstance() {
    static EventManager instance;
    return instance;
}

void EventManager::subscribe(EventType type, EventHandler handler) {
    std::lock_guard<std::mutex> lock(mutex_);
    handlers_[type].push_back(handler);
}

void EventManager::publish(const Event& event) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = handlers_.find(event.type);
    if (it != handlers_.end()) {
        for (const auto& handler : it->second) {
            handler(event);
        }
    }
}
