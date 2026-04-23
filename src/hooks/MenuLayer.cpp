#include "../IntegratedDemonlist.hpp"
#include <Geode/modify/MenuLayer.hpp>

using namespace geode::prelude;

static std::array<TaskHolder<web::WebResponse>, 6> s_allListListeners;

class $modify(IDMenuLayer, MenuLayer) {
    bool init() {
        if (!MenuLayer::init()) return false;

        static bool s_started = false;
        if (!s_started) {
            s_started = true;
            if (!IntegratedDemonlist::allListLoaded) {
                IntegratedDemonlist::loadAllList(s_allListListeners, [] {}, [](int) {});
            }
        }

        return true;
    }
};
