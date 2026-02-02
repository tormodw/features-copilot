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

    static EventManager& getInstance();

    void subscribe(EventType type, EventHandler handler);
    void publish(const Event& event);

private:
    EventManager() = default;
    EventManager(const EventManager&) = delete;
    EventManager& operator=(const EventManager&) = delete;

    std::map<EventType, std::vector<EventHandler>> handlers_;
    std::mutex mutex_;
};

#endif // EVENT_MANAGER_H
