#include <Geode/utils/web.hpp>

struct IDListDemon {
    int id = 0;
    int position = 0;
    int tier = -1;
    std::string name;

    IDListDemon() = default;
    IDListDemon(int id, int position, std::string name, int tier = -1)
        : id(id), position(position), tier(tier), name(std::move(name)) {}

    bool operator==(const IDListDemon& other) const {
        return id == other.id && position == other.position;
    }
};

struct IDDemonPack {
    std::string name;
    std::string tierName;
    std::vector<int> levels;
    double points = 0.0;
    int tier = 0;
};

namespace IntegratedDemonlist {
    extern std::vector<IDListDemon> aredl;
    extern std::vector<IDDemonPack> aredlPacks;
    extern std::vector<IDListDemon> pemonlist;
    extern std::vector<IDListDemon> allList;
    extern bool aredlLoaded;
    extern bool pemonlistLoaded;
    extern bool allListLoaded;

    void loadAREDL(geode::async::TaskHolder<geode::utils::web::WebResponse>&, geode::Function<void()>, geode::CopyableFunction<void(int)>);
    void loadAREDLPacks(geode::async::TaskHolder<geode::utils::web::WebResponse>&, geode::Function<void()>, geode::CopyableFunction<void(int)>);
    void loadPemonlist(geode::async::TaskHolder<geode::utils::web::WebResponse>&, geode::Function<void()>, geode::CopyableFunction<void(int)>);
    void loadAllList(geode::async::TaskHolder<geode::utils::web::WebResponse>&, geode::Function<void()>, geode::CopyableFunction<void(int)>);
}
